# Claude session notes — swagdial

## What this project is

Conference swag selection device for GrafanaCon, running on M5Stack Dial 1.1 (ESP32-S3).
User rotates encoder to navigate a hierarchical menu of swag items (e.g. Sticker → Logos → Mimir),
presses button to select a leaf node, which fires a Prometheus remote write event to Grafana Mimir.

Sister project to `cheerdial/` in the same repo — refer to that for working examples of the
M5Dial/Prometheus library API patterns.

## Current state

**Device is fully working.** Menus, encoder, button, metric sending, and images all verified on hardware.

### Files

- `swagdial.ino` — complete and working
- `build.sh` — arduino-cli wrapper with `setup / build / flash / monitor` commands
- `certificates.h` — DigiCert root cert for Grafana Cloud TLS (copied from cheerdial)
- `config_local.h` — **gitignored** — real WiFi + Grafana Cloud credentials (compile-time constants)
- `config_local_example.h` — template for config_local.h (committed)
- `items.json` — canonical items source (committed); content also embedded as string literal in sketch
- `images/tshirt_jpg.h` — t-shirt JPEG as C byte array (committed)
- `images/sticker_jpg.h` — sticker JPEG as C byte array (committed)
- `README.md` — full documentation
- `CONTEXT.md` — original spec written by Simon
- `dashboards/` — placeholder folder, Grafana dashboard to be built later

## Architecture decisions (final)

### Config — compile-time header
`#include "config_local.h"` (gitignored). `loadConfig()` copies defines into the Config struct.
`config_local_example.h` is the committed template.

### Items — embedded string literal
`ITEMS_JSON` is a raw string literal in `swagdial.ino` (kept in sync with `items.json`).
`loadItems()` parses it with ArduinoJson. To change items: edit both files and reflash.

### Images — PROGMEM byte arrays provisioned to LittleFS on first boot
Images live in `swagdial/images/` as `*_jpg.h` headers generated with `xxd -i`.
They are listed in `IMAGE_ASSETS[]` in the sketch and written to LittleFS on first boot
if not already present. Subsequent boots skip files that already exist.
Image display falls back to solid colour background if LittleFS unavailable or file missing.

#### Adding an image
1. `sips -z 240 240 -s format jpeg input.png --out output.jpg`
2. `xxd -i output.jpg > swagdial/images/output_jpg.h`
3. `#include "images/output_jpg.h"` in swagdial.ino
4. Add entry to `IMAGE_ASSETS[]`
5. Flash

#### Image display
Images drawn at 70% scale, centred:
```cpp
canvas.drawJpg(buf, sz, 0, 0, w, h, 0, 0, 0.7f, 0.7f, middle_center);
```
Note: `maxWidth/maxHeight` params clip rather than scale in this M5GFX version —
must use explicit `scale_x/scale_y` + `datum` to resize and centre.

### LittleFS filesystem upload (DO NOT USE)
mklittlefs and mkspiffs both fail to produce images compatible with the ESP32 Arduino
core 3.3.8 LittleFS runtime on the M5Stack Dial 1.1. The provisioning-from-PROGMEM
approach above is the reliable alternative.

### Metric design
- Metric name: `gcon_swag` (in ITEMS_JSON)
- Value: always 1 per selection
- Labels: `device_id` (from config_local.h) + one label per hierarchy level
- Example: `gcon_swag{device_id="swagdial-1",category="tshirt",design="logo",size="l"} 1`

### Navigation
- Sticker is first item (index 0), T-Shirt is second
- Encoder scrolls; button enters branch / selects leaf / goes back
- `< Back` auto-injected at every non-root level

### Build
- FQBN: `esp32:esp32:m5stack_dial`
- ArduinoJson pinned to 6.21.6 (v7 has breaking API changes)
- Port on Simon's Mac: `/dev/cu.usbmodem3101`

## Prometheus / Grafana Cloud API patterns (from cheerdial)

```cpp
client.setUrl(config.gcUrl.c_str());
client.setPath((char*)config.gcPath.c_str());   // cast to char* required
client.setPort(config.gcPort);
client.setUser(config.gcUser.c_str());
client.setPass(config.gcPass.c_str());
client.setDebug(Serial);
client.begin();

PromClient::SendResult res = client.send(req);
if (res != PromClient::SendResult::SUCCESS) { Serial.println(client.errmsg); }

ts.addSample(transport.getTimeMillis(), value);  // not millis()
```
