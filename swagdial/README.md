# SwagDial

A conference swag selection interface for the [M5Stack Dial 1.1](https://shop.m5stack.com/products/m5stack-dial-v1-1) that records choices as Prometheus metrics via remote write to Grafana Mimir.

Attendees use the rotary encoder to navigate a configurable item hierarchy (e.g. T-Shirt → Logo → Large) and press the button to record their selection. Each selection sends a single metric data point to a configured Prometheus remote write endpoint with labels describing the full path through the hierarchy.

---

## Hardware

- M5Stack Dial 1.1

## Building and flashing

### Prerequisites

Install [arduino-cli](https://arduino.github.io/arduino-cli/) via [Homebrew](https://brew.sh):

```bash
brew install arduino-cli
```

### One-time setup

Installs the ESP32 board package and all required libraries:

```bash
./build.sh setup
```

This installs:

| Library | Version |
|---|---|
| ESP32 Arduino core (Espressif) | latest |
| M5Unified (M5Stack board + display support) | latest |
| M5GFX | latest |
| ArduinoJson | 6.21.6 (pinned — v7 has breaking API changes) |
| PrometheusArduino | latest |
| PromLokiTransport | latest |

After setup, confirm the board name resolves correctly:

```bash
arduino-cli board listall | grep -i dial
```

The script uses `FQBN=esp32:esp32:m5stack_dial` — update the variable at the top of `build.sh` if your board is listed under a different name.

### Compile

```bash
./build.sh build
```

### Flash the sketch

Connect the M5Dial via USB and find its port:

```bash
arduino-cli board list
```

Then compile and flash in one step:

```bash
./build.sh flash /dev/cu.usbmodem101
```

### Upload the filesystem (LittleFS)

`config.json`, `items.json`, and images live on the device's LittleFS filesystem and are uploaded separately from the sketch.

Create a `data/` directory inside `swagdial/` and populate it:

```
swagdial/data/
  config.json          ← copy of your real config.json (not config_example.json)
  items.json
  images/
    tshirt.jpg
    ...
```

Then flash the filesystem:

```bash
./build.sh data-flash /dev/cu.usbmodem101
```

This automatically locates `mklittlefs` and `esptool` from the ESP32 core installed by `setup`, reads the correct partition offset and size from the board's partition table, builds a LittleFS image, and flashes it.

### Serial monitor

Watch connection status and error output:

```bash
./build.sh monitor /dev/cu.usbmodem101
```

---

## Device filesystem layout

| Path on device | Description |
|---|---|
| `/config.json` | WiFi credentials and Prometheus endpoint — **never commit this** |
| `/items.json` | The swag item hierarchy |
| `/images/*.jpg` | Optional item images (240×240 px recommended) |

---

## Configuration — `config.json`

Copy `config_example.json` to `config.json` and fill in your values. This file is gitignored and must not be committed.

```json
{
  "wifi_ssid": "your_wifi_ssid",
  "wifi_password": "your_wifi_password",
  "device_id": "swagdial-1",
  "gc_host": "prometheus-prod-24-prod-eu-west-2.grafana.net",
  "gc_path": "/api/prom/push",
  "gc_port": 443,
  "gc_user": "your_grafana_cloud_user_id",
  "gc_pass": "your_grafana_cloud_api_token"
}
```

| Field | Description |
|---|---|
| `wifi_ssid` | WiFi network name |
| `wifi_password` | WiFi password |
| `device_id` | Unique identifier for this device — used as a label on every metric |
| `gc_host` | Grafana Cloud Prometheus remote write hostname (no `https://`) |
| `gc_path` | Grafana Cloud remote write path |
| `gc_port` | Port — 443 for TLS |
| `gc_user` | Grafana Cloud metrics user ID (numeric) |
| `gc_pass` | Grafana Cloud API token with MetricsPublisher role |
| `encoder_sensitivity` | Optional. Minimum encoder steps to register a turn. Lower = more sensitive. Default: `3` |

---

## Items — `items.json`

Describes the swag hierarchy and the Prometheus labels to emit. The file is loaded at startup and is safe to commit.

### Top-level structure

```json
{
  "metric": "gcon_swag",
  "items": [ ... ]
}
```

| Field | Description |
|---|---|
| `metric` | Prometheus metric name. Per Prometheus convention, use `snake_case` and suffix counters with `_total`. |
| `items` | Array of top-level menu items (see below). |

### Item structure

Each item in the hierarchy — at any depth — has the same shape:

```json
{
  "label_key":    "category",
  "label_value":  "tshirt",
  "display_name": "T-Shirt",
  "image":        "tshirt.jpg",
  "children": [ ... ]
}
```

| Field | Required | Description |
|---|---|---|
| `label_key` | Yes | Prometheus label name for this level of the hierarchy |
| `label_value` | Yes | Prometheus label value |
| `display_name` | Yes | Text shown on screen |
| `image` | No | JPEG filename (looked up under `/images/` on device flash). Omit or leave empty if no image. |
| `children` | No | Nested items. Omit on leaf nodes — pressing the button on a leaf records the metric. |

Items with `children` are **branch nodes**: pressing the button navigates into them.
Items without `children` are **leaf nodes**: pressing the button sends the metric.

A **Back** item is automatically added as the last entry at every non-root level, allowing the user to go up one level without making a selection.

### Example

```json
{
  "metric": "gcon_swag",
  "items": [
    {
      "label_key": "category",
      "label_value": "tshirt",
      "display_name": "T-Shirt",
      "image": "tshirt.jpg",
      "children": [
        {
          "label_key": "design",
          "label_value": "logo",
          "display_name": "Logo",
          "image": "tshirt_logo.jpg",
          "children": [
            { "label_key": "size", "label_value": "s",  "display_name": "Small"   },
            { "label_key": "size", "label_value": "m",  "display_name": "Medium"  },
            { "label_key": "size", "label_value": "l",  "display_name": "Large"   },
            { "label_key": "size", "label_value": "xl", "display_name": "X-Large" }
          ]
        }
      ]
    }
  ]
}
```

Selecting "Large Logo T-Shirt" on a device with `device_id = "swagdial-1"` would send:

```
gcon_swag{device_id="swagdial-1",category="tshirt",design="logo",size="l"} 1
```

---

## Images

- Format: JPEG
- Recommended size: 240×240 pixels (the Dial's display is 240×240 round)
- Location on device: `/images/<filename>`
- If an image file is not found or the `image` field is omitted, the display falls back to a coloured background with the item name

A simple naming convention that mirrors the hierarchy works well:

```
tshirt.jpg
tshirt_logo.jpg
sticker.jpg
sticker_logos_mimir.jpg
```

---

## Usage

### Navigation

| Action | Effect |
|---|---|
| Rotate encoder | Scroll through items at the current level |
| Press button on a branch item (shows `>`) | Navigate into that item's children |
| Press button on a leaf item | Record the selection and return to root |
| Press button on `< Back` | Go up one level |

### Display

- **Blue background** — branch item (has sub-items)
- **Green background** — leaf item (press to select)
- **Grey background** — Back item
- Position indicator (e.g. `2/4`) is shown at the top of the screen
- If an image is defined and found, it fills the display with a black text bar at the bottom

---

## Metric design

Every selection sends a single sample with value `1` to the configured remote write endpoint. No counters are stored on the device — aggregation is handled server-side by Mimir.

Labels on each metric point:

- `device_id` — from `config.json`, identifies which physical device recorded the event
- One label per level of the hierarchy the user navigated, using the `label_key`/`label_value` pairs from `items.json`

This structure supports queries like:

```promql
# Total selections across all devices and items
sum(increase(gcon_swag[$__range]))

# All t-shirt selections, any design or size
sum(increase(gcon_swag{category="tshirt"}[$__range]))

# Logo t-shirts only, any size
sum(increase(gcon_swag{category="tshirt",design="logo"}[$__range]))

# Large items only
sum(increase(gcon_swag{size="l"}[$__range]))

# Selections over time, broken down by category
sum by (category) (rate(gcon_swag[$__rate_interval]))
```
