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

### Serial monitor

Watch connection status and error output:

```bash
./build.sh monitor /dev/cu.usbmodem101
```

---

## Configuration — `config.h`

Copy `config_example.h` to `config.h` and fill in your values. This file is gitignored and must not be committed.

```cpp
#define WIFI_SSID            "your_wifi_ssid"
#define WIFI_PASSWORD        "your_wifi_password"
#define DEVICE_ID            "swagdial-1"
#define GC_HOST              "prometheus-prod-24-prod-eu-west-2.grafana.net"
#define GC_PATH              "/api/prom/push"
#define GC_PORT              443
#define GC_USER              "your_grafana_cloud_user_id"
#define GC_PASS              "your_grafana_cloud_api_token"
#define ENCODER_SENSITIVITY  2
```

| Field | Description |
|---|---|
| `WIFI_SSID` | WiFi network name |
| `WIFI_PASSWORD` | WiFi password |
| `DEVICE_ID` | Unique identifier for this device — used as a label on every metric |
| `GC_HOST` | Grafana Cloud Prometheus remote write hostname (no `https://`) |
| `GC_PATH` | Grafana Cloud remote write path |
| `GC_PORT` | Port — 443 for TLS |
| `GC_USER` | Grafana Cloud metrics user ID (numeric) |
| `GC_PASS` | Grafana Cloud API token with MetricsPublisher role |
| `ENCODER_SENSITIVITY` | Minimum encoder steps to register a turn. Lower = more sensitive. Default: `2` |

Config is compiled into the firmware — reflash to change it.

---

## Setting up multiple devices

All devices share the same firmware and configuration except for `DEVICE_ID`, which must be unique per unit so selections can be attributed to a specific device in Grafana.

**Suggested device IDs:** `swagdial-1` through `swagdial-7`.

### Steps for each device

1. Edit `config.h` — set `DEVICE_ID` to this device's ID (e.g. `"swagdial-2"`). All other fields stay the same across units.

2. Connect the device via USB and find its port:

   ```bash
   arduino-cli board list
   ```

3. Compile and flash:

   ```bash
   ./build.sh flash /dev/cu.usbmodem101
   ```

4. Watch the serial monitor to confirm it connects and shows `Connected!`:

   ```bash
   ./build.sh monitor /dev/cu.usbmodem101
   ```

5. Unplug and repeat for the next unit, incrementing the `DEVICE_ID`.

The port name (`/dev/cu.usbmodem101`) may differ each time you plug in a new device — always re-run `arduino-cli board list` to confirm before flashing.

---

## Items — `items.json`

Describes the swag hierarchy and the Prometheus labels to emit. `items.json` is the canonical source of truth (safe to commit); its content is also embedded as a string literal in `swagdial.ino` and compiled into the firmware. To change the items, edit both files and reflash.

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

Images are embedded directly in the firmware as `const` C byte arrays and drawn straight from flash — no filesystem required.

- Format: JPEG
- Required size: 240×240 pixels (the Dial's display is 240×240 round)
- If an item has no image (or its image isn't in `IMAGE_ASSETS`), the display falls back to the top-level category image; if that's also missing, a plain background is shown

### Adding an image

**1. Download a PNG from [Flaticon](https://www.flaticon.com) or another source.**

Free Flaticon icons require attribution — add a note to your documentation.

**2. Resize and convert to JPEG (built-in macOS tool, no install needed):**

```bash
sips -z 240 240 -s format jpeg tshirt.png --out tshirt.jpg
```

**3. Convert the JPEG to a C header file:**

```bash
xxd -i tshirt.jpg > swagdial/images/tshirt_jpg.h
```

**4. Edit the generated header — change `unsigned char` to `const unsigned char` on both lines:**

```cpp
// before
unsigned char tshirt_jpg[] = { ... };
unsigned int tshirt_jpg_len = 9923;

// after
const unsigned char tshirt_jpg[] = { ... };
const unsigned int tshirt_jpg_len = 9923;
```

This is required so the linker places the data in flash rather than SRAM. Without `const`, large images will exhaust the heap and prevent WiFi from connecting.

**5. Include the header in `swagdial.ino`:**

```cpp
#include "images/tshirt_jpg.h"
```

**6. Add an entry to the `IMAGE_ASSETS` array in `swagdial.ino`:**

```cpp
static const ImageAsset IMAGE_ASSETS[] = {
  { "tshirt.jpg", tshirt_jpg, tshirt_jpg_len },
  // add more here
};
```

The variable names (`tshirt_jpg`, `tshirt_jpg_len`) are generated by `xxd` from the filename — `xxd -i foo_bar.jpg` produces `foo_bar_jpg` and `foo_bar_jpg_len`.

**7. Flash the sketch:**

```bash
./build.sh flash /dev/cu.usbmodem101
```

### Naming convention

Use names that mirror the hierarchy for clarity:

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

- `device_id` — from `config.h`, identifies which physical device recorded the event
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
