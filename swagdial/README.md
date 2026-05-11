# SwagDial

A conference swag selection interface for the [M5Stack Dial 1.1](https://shop.m5stack.com/products/m5stack-dial-v1-1) that records choices as Prometheus metrics via remote write to Grafana Mimir.

Attendees use the rotary encoder to navigate a configurable item hierarchy (e.g. T-Shirt → Logo → Large) and press the button to record their selection. Each selection sends a single metric data point to a configured Prometheus remote write endpoint with labels describing the full path through the hierarchy.

---

## Hardware

- M5Stack Dial 1.1 (ESP32-S3, 8 MB flash, 240×240 round GC9A01 display)

---

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

| Library | Version |
|---|---|
| ESP32 Arduino core (Espressif) | latest |
| M5Unified | latest |
| M5GFX | latest |
| ArduinoJson | 6.21.6 (pinned — v7 has breaking API changes) |
| PrometheusArduino | latest |
| PromLokiTransport | latest |

### Compile

```bash
./build.sh build
```

### Flash

Connect the M5Dial via USB and find its port:

```bash
arduino-cli board list
```

Then compile and flash:

```bash
./build.sh flash /dev/cu.usbmodem101
```

### Serial monitor

```bash
./build.sh monitor /dev/cu.usbmodem101
```

---

## Configuration — `config.h`

Edit `config.h` with your WiFi credentials and Grafana Cloud remote write details:

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
| `GC_PATH` | Remote write path |
| `GC_PORT` | Port — `443` for TLS |
| `GC_USER` | Grafana Cloud metrics user ID (numeric) |
| `GC_PASS` | Grafana Cloud API token with MetricsPublisher role |
| `ENCODER_SENSITIVITY` | Minimum encoder steps to register a turn. Lower = more sensitive. Default: `2` |

Configuration is compiled into the firmware — reflash to change it.

---

## Setting up multiple devices

All devices share the same firmware except for `DEVICE_ID`, which must be unique per unit so selections can be attributed to a specific device in Grafana.

For each device:

1. Edit `config.h` — set a unique `DEVICE_ID` (e.g. `"swagdial-2"`).
2. Connect via USB and find its port: `arduino-cli board list`
3. Flash: `./build.sh flash /dev/cu.usbmodem101`
4. Confirm `Connected!` appears on screen and in the serial monitor.
5. Unplug and repeat for the next unit.

The port name may change each time you plug in a new device — always re-run `arduino-cli board list` to confirm.

---

## Items — `items.json`

Describes the swag hierarchy and the Prometheus labels to emit. `items.json` is the canonical source of truth; its content is also embedded as a string literal in `swagdial.ino` and compiled into the firmware. **To change the items, edit both files and reflash.**

### Top-level structure

```json
{
  "metric": "gcon_swag",
  "items": [ ... ]
}
```

### Item structure

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
| `label_key` | Yes | Prometheus label name for this level |
| `label_value` | Yes | Prometheus label value |
| `display_name` | Yes | Text shown on screen |
| `image` | No | Image name as it appears in `IMAGE_ASSETS` in the sketch. Omit if no image. Sub-items without an image inherit the nearest ancestor's image automatically. |
| `children` | No | Nested items. Omit on leaf nodes. |

Items with `children` are **branch nodes** — pressing the button navigates into them.
Items without `children` are **leaf nodes** — pressing the button sends the metric.

A `< Back` item is automatically added at every non-root level.

### Example metric

Selecting "Large Logo T-Shirt" on a device with `DEVICE_ID="swagdial-1"` sends:

```
gcon_swag{device_id="swagdial-1",category="tshirt",design="logo",size="l"} 1
```

---

## Images

Images are stored as pre-decoded raw RGB565 pixel arrays in flash. This means rendering is a direct memory-to-display blit with no runtime heap allocation — image display is reliable regardless of how much heap is available for the WiFi/TLS stack.

- Source format: JPEG, 240×240 pixels
- Stored format: RGB565, 168×168 pixels (70% scale, centred and shifted up 13 px)
- Location: `images/*_rgb565.h`
- Conversion tool: `images/convert_to_rgb565.py` (requires Python 3 + Pillow)

### Adding an image

**1. Prepare a 240×240 JPEG.** Using macOS `sips`:

```bash
sips -z 240 240 -s format jpeg input.png --out tshirt.jpg
```

**2. Generate the JPEG header with `xxd`:**

```bash
xxd -i tshirt.jpg > images/tshirt_jpg.h
```

Edit the generated file — change `unsigned char` to `const unsigned char` on both the array and length lines.

**3. Run the conversion script to produce the RGB565 header:**

```bash
cd images
python3 convert_to_rgb565.py
```

This reads all `*_jpg.h` files in the `images/` directory and regenerates all `*_rgb565.h` files. The script requires Pillow (`pip3 install Pillow`).

**4. Include the RGB565 header in `swagdial.ino`:**

```cpp
#include "images/tshirt_rgb565.h"
```

**5. Add an entry to `IMAGE_ASSETS` in `swagdial.ino`:**

```cpp
static const ImageAsset IMAGE_ASSETS[] = {
  { "tshirt.jpg", tshirt_rgb565 },
  // ...
};
```

**6. Reference the image name in `items.json` and the embedded `ITEMS_JSON` string literal in `swagdial.ino`.**

**7. Flash:**

```bash
./build.sh flash /dev/cu.usbmodem101
```

---

## Usage

### Navigation

| Action | Effect |
|---|---|
| Rotate encoder | Scroll through items at the current level |
| Press button on a branch item (shows `>`) | Navigate into that item's children |
| Press button on a leaf item | Send the metric and return to root |
| Press button on `< Back` | Go up one level |

### Display

- Item name in **white** — branch node (press to enter)
- Item name in **orange** — leaf node (press to submit)
- Position indicator (e.g. `2/4`) at the top of the screen
- `>` on the right edge indicates a branch node
- Image fills the display behind a navy bar at the bottom containing the item name

---

## Metric design

Every selection sends a single sample with value `1`. Labels:

- `device_id` — from `config.h`
- One label per level of the hierarchy navigated, using the `label_key`/`label_value` pairs from `items.json`

Example queries:

```promql
# Total selections across all devices
sum(increase(gcon_swag[$__range]))

# T-shirt selections only
sum(increase(gcon_swag{category="tshirt"}[$__range]))

# Selections over time by category
sum by (category) (rate(gcon_swag[$__rate_interval]))
```
