# Claude session notes — swagdial

## What this project is

Conference swag selection device for GrafanaCon, running on M5Stack Dial 1.1 (ESP32-S3).
User rotates encoder to navigate a hierarchical menu of swag items (e.g. T-Shirt → Logo → Large),
presses button to select a leaf node, which fires a Prometheus remote write event to Grafana Mimir.

Sister project to `cheerdial/` in the same repo — refer to that for working examples of the
M5Dial/Prometheus library API patterns.

## Current state

- **Device is fully working**: menus, encoder, button, metric sending, and images all work.
- Images display at 70% scale, centred, with a solid black bar at the bottom for the item name.

### Files

- `swagdial.ino` — complete and working
- `build.sh` — arduino-cli wrapper with `setup / build / flash / monitor` commands
  - `data-flash` command still present but currently unused (images not working yet)
- `certificates.h` — DigiCert root cert for Grafana Cloud TLS (copied from cheerdial)
- `config_local.h` — **gitignored** — real WiFi + Grafana Cloud credentials (compile-time constants)
- `config_local_example.h` — template for config_local.h (committed)
- `items.json` — canonical items source (committed); also embedded as string literal in sketch
- `README.md` — documentation
- `CONTEXT.md` — original spec written by Simon
- `dashboards/` — placeholder folder, Grafana dashboard to be built later

## Architecture decisions (final)

### Config — compile-time header
Config is `#include "config_local.h"` (gitignored). `loadConfig()` just copies defines into the
Config struct. No filesystem dependency. `config_local_example.h` is the committed template.

### Items — embedded string literal
`ITEMS_JSON` is a raw string literal in `swagdial.ino` (copy of items.json content).
`loadItems()` parses it with ArduinoJson. No filesystem dependency.
To change items: edit the string in the sketch and reflash.

### Images — LittleFS, optional
LittleFS is initialised with `formatOnFail=true`. Success sets `imagesAvailable = true`.
Image loading in `displayCurrentItem()` is gated on `imagesAvailable`.
If LittleFS is unavailable or an image file is missing, falls back to solid colour background.

### Metric design
- Metric name: `gcon_swag_total` (in ITEMS_JSON)
- Value: always 1 per selection
- Labels: `device_id` (from config_local.h) + one label per hierarchy level
- Example: `gcon_swag_total{device_id="swagdial-1",category="tshirt",design="logo",size="l"} 1`

### Navigation
- Encoder scrolls through items at current level
- Button press on branch node (shows `>`) → navigate into children
- Button press on leaf node → send metric, return to root
- `< Back` is auto-injected as last item at every non-root level

### Prometheus client usage
Same patterns as cheerdial. `WriteRequest` and `TimeSeries` created locally in `sendMetric()`
on each press (required because labels are dynamic). See `sendMetric()`.

### Image display (known workaround)
`canvas.drawJpgFile(LittleFS, path, ...)` does NOT work — LovyanGFX's DataWrapperT<fs::LittleFSFS>
is abstract (missing virtual implementations) in M5GFX 0.2.19 / ESP32 core 3.3.8.
Workaround: read JPEG into malloc'd heap buffer, call `canvas.drawJpg(buf, size, ...)`.

### Build
- FQBN: `esp32:esp32:m5stack_dial`
- ArduinoJson pinned to 6.21.6 (v7 has breaking API changes)
- Port on Simon's Mac: `/dev/cu.usbmodem3101`

## LittleFS / image upload — current status (UNSOLVED)

Flashing pre-built filesystem images to the LittleFS partition doesn't work reliably on this
device/core combination. Investigation findings:

- Partition: `spiffs` label, type `data/spiffs`, offset `0x670000`, size `0x180000`
- Core: `esp32:esp32 3.3.8`, `CONFIG_LITTLEFS_READ_SIZE=128`, `CONFIG_LITTLEFS_WRITE_SIZE=128`,
  `CONFIG_LITTLEFS_PAGE_SIZE=256`
- mklittlefs 4.0.2 with -p 128 or -p 256: image writes successfully (hash verified),
  but library mounts an empty filesystem (`used=8192` = 2 blocks = superblock only)
- mkspiffs 0.2.3 with -p 256: SPIFFS.begin() fails entirely

Workaround: config and items embedded in firmware. Images deferred.

To add image support later:
- Try writing images to LittleFS FROM the sketch (format via library, then write files over serial
  or from a companion sketch) rather than using mklittlefs/esptool
- Or investigate arduino-esp32fs-plugin / platformio LittleFS upload approach

## Prometheus / Grafana Cloud API patterns (from cheerdial)

```cpp
// Client setup (not ConnectionConfig — direct setters)
client.setUrl(config.gcUrl.c_str());
client.setPath((char*)config.gcPath.c_str());   // note: cast to char* required
client.setPort(config.gcPort);
client.setUser(config.gcUser.c_str());
client.setPass(config.gcPass.c_str());
client.setDebug(Serial);
client.begin();

// Sending
PromClient::SendResult res = client.send(req);
if (res != PromClient::SendResult::SUCCESS) { Serial.println(client.errmsg); }

// Timestamp
ts.addSample(transport.getTimeMillis(), value);  // not millis()
```
