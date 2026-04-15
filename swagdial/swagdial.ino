#include "M5Dial.h"
#include "certificates.h"
#include "config_local.h"
#include "images/tshirt_jpg.h"
#include "images/sticker_jpg.h"
#include "images/coffee_jpg.h"
#include "images/patch_jpg.h"
#include "images/crochet_jpg.h"
#include "images/keychain_jpg.h"
#include "images/coin_jpg.h"
#include "images/back_jpg.h"
#include <ArduinoJson.h>
#include <PromLokiTransport.h>
#include <PrometheusArduino.h>
#include <vector>
#include "types.h"

// Items hierarchy embedded in firmware — edit and reflash to change.
static const char ITEMS_JSON[] = R"({
  "metric": "gcon_swag",
  "items": [
    {
      "label_key": "category",
      "label_value": "sticker",
      "display_name": "Sticker",
      "image": "sticker.jpg",
      "children": [
        {
          "label_key": "design",
          "label_value": "logos",
          "display_name": "Logos",
          "children": [
            { "label_key": "item", "label_value": "mimir",   "display_name": "Mimir",   "image": "sticker_logos_mimir.jpg"   },
            { "label_key": "item", "label_value": "grafana", "display_name": "Grafana", "image": "sticker_logos_grafana.jpg" },
            { "label_key": "item", "label_value": "tempo",   "display_name": "Tempo",   "image": "sticker_logos_tempo.jpg"   }
          ]
        },
        {
          "label_key": "design",
          "label_value": "grot",
          "display_name": "Grot",
          "children": [
            { "label_key": "item", "label_value": "sunglasses", "display_name": "Sunglasses" },
            { "label_key": "item", "label_value": "guitar",     "display_name": "Guitar"     },
            { "label_key": "item", "label_value": "lgbt",       "display_name": "LGBT"       }
          ]
        }
      ]
    },
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
    },
    {
      "label_key": "category",
      "label_value": "patch",
      "display_name": "Patch",
      "image": "patch.jpg",
      "children": [
        { "label_key": "type", "label_value": "contributor", "display_name": "Contributor", "image": "patch.jpg" },
        { "label_key": "type", "label_value": "champion",    "display_name": "Champion",    "image": "patch.jpg" }
      ]
    },
    {
      "label_key": "category",
      "label_value": "coin",
      "display_name": "Coin",
      "image": "coin.jpg"
    },
    {
      "label_key": "category",
      "label_value": "keychain",
      "display_name": "Keychain",
      "image": "keychain.jpg",
      "children": [
        { "label_key": "design", "label_value": "logo", "display_name": "Logo", "image": "keychain.jpg" },
        { "label_key": "design", "label_value": "doom", "display_name": "Doom", "image": "keychain.jpg" }
      ]
    },
    {
      "label_key": "category",
      "label_value": "crochet",
      "display_name": "Crochet",
      "image": "crochet.jpg"
    },
    {
      "label_key": "category",
      "label_value": "coffee",
      "display_name": "Coffee",
      "image": "coffee.jpg",
      "children": [
        { "label_key": "drink", "label_value": "americano",  "display_name": "Americano",  "image": "coffee.jpg" },
        { "label_key": "drink", "label_value": "latte",      "display_name": "Latte",      "image": "coffee.jpg" },
        { "label_key": "drink", "label_value": "espresso",   "display_name": "Espresso",   "image": "coffee.jpg" },
        { "label_key": "drink", "label_value": "cappuccino", "display_name": "Cappuccino", "image": "coffee.jpg" },
        { "label_key": "drink", "label_value": "mocha",      "display_name": "Mocha",      "image": "coffee.jpg" }
      ]
    }
  ]
})";

// ─── Image assets ─────────────────────────────────────────────────────────────

static const ImageAsset IMAGE_ASSETS[] = {
  { "tshirt.jpg",   tshirt_jpg,   tshirt_jpg_len   },
  { "sticker.jpg",  sticker_jpg,  sticker_jpg_len  },
  { "coffee.jpg",   coffee_jpg,   coffee_jpg_len   },
  { "patch.jpg",    patch_jpg,    patch_jpg_len    },
  { "crochet.jpg",  crochet_jpg,  crochet_jpg_len  },
  { "keychain.jpg", keychain_jpg, keychain_jpg_len },
  { "coin.jpg",     coin_jpg,     coin_jpg_len     },
  { "back.jpg",     back_jpg,     back_jpg_len     },
};

static const ImageAsset* findImage(const String& name) {
  for (auto& asset : IMAGE_ASSETS) {
    if (name == asset.name) return &asset;
  }
  return nullptr;
}

// ─── Globals ──────────────────────────────────────────────────────────────────

Config                 config;
String                 metricName;
std::vector<MenuItem>  rootItems;
std::vector<NavFrame>  navStack;
std::vector<MenuItem>* currentItems = nullptr;
int                    currentIndex  = 0;
long                   lastEncoderPos = 0;

M5GFX             display;
M5Canvas          canvas(&display);
PromLokiTransport transport;
PromClient        client(transport);

// ─── Navigation helpers ───────────────────────────────────────────────────────

int totalItems() {
  if (!currentItems) return 0;
  return (int)currentItems->size() + (navStack.empty() ? 0 : 1);
}

bool isBackItem(int idx) {
  return !navStack.empty() && currentItems && idx == (int)currentItems->size();
}

bool isLeafItem(int idx) {
  if (!currentItems || isBackItem(idx)) return false;
  return (*currentItems)[idx].children.empty();
}

void resetEncoder() {
  M5Dial.Encoder.write(0);
  lastEncoderPos = 0;
}

void navigateInto() {
  navStack.push_back({currentItems, currentIndex});
  currentItems = &(*currentItems)[currentIndex].children;
  currentIndex = 0;
  resetEncoder();
}

void navigateBack() {
  if (navStack.empty()) return;
  NavFrame frame = navStack.back();
  navStack.pop_back();
  currentItems = frame.items;
  currentIndex = frame.selectedIndex;
  resetEncoder();
}

void navigateToRoot() {
  navStack.clear();
  currentItems = &rootItems;
  currentIndex = 0;
  resetEncoder();
}

// ─── Config & items loading ───────────────────────────────────────────────────

void loadConfig() {
  config.wifiSsid           = WIFI_SSID;
  config.wifiPassword       = WIFI_PASSWORD;
  config.deviceId           = DEVICE_ID;
  config.gcUrl              = GC_HOST;
  config.gcPath             = GC_PATH;
  config.gcPort             = GC_PORT;
  config.gcUser             = GC_USER;
  config.gcPass             = GC_PASS;
  config.encoderSensitivity = ENCODER_SENSITIVITY;
}

void parseMenuItems(JsonArray arr, std::vector<MenuItem>& items) {
  for (JsonObject obj : arr) {
    MenuItem item;
    item.labelKey    = obj["label_key"].as<String>();
    item.labelValue  = obj["label_value"].as<String>();
    item.displayName = obj["display_name"].as<String>();
    item.image       = obj["image"] | "";
    if (obj.containsKey("children")) {
      parseMenuItems(obj["children"].as<JsonArray>(), item.children);
    }
    items.push_back(item);
  }
}

bool loadItems() {
  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, ITEMS_JSON);
  if (err) {
    Serial.print("items parse error: ");
    Serial.println(err.c_str());
    return false;
  }
  metricName = doc["metric"].as<String>();
  parseMenuItems(doc["items"].as<JsonArray>(), rootItems);
  return !rootItems.empty();
}

// ─── Metrics ──────────────────────────────────────────────────────────────────

String buildLabels() {
  String s = "{device_id=\"" + config.deviceId + "\"";
  for (auto& frame : navStack) {
    MenuItem& item = (*frame.items)[frame.selectedIndex];
    s += "," + item.labelKey + "=\"" + item.labelValue + "\"";
  }
  MenuItem& leaf = (*currentItems)[currentIndex];
  s += "," + leaf.labelKey + "=\"" + leaf.labelValue + "\"";
  s += "}";
  return s;
}

bool sendMetric() {
  String labels = buildLabels();
  WriteRequest req(1);
  TimeSeries ts(1, metricName.c_str(), labels.c_str());
  req.addTimeSeries(ts);
  ts.addSample(transport.getTimeMillis(), 1);
  PromClient::SendResult res = client.send(req);
  if (res != PromClient::SendResult::SUCCESS) {
    Serial.println(client.errmsg);
    return false;
  }
  return true;
}

// ─── Display ──────────────────────────────────────────────────────────────────

// Colours — use color888() to ensure correct conversion for the sprite colour depth
#define CLR_ERROR   M5Dial.Display.color888(200,   0,   0)
#define CLR_OK      M5Dial.Display.color888(  0, 160,   0)
#define CLR_INFO    M5Dial.Display.color888(  0,   0, 120)
#define CLR_NEUTRAL M5Dial.Display.color888( 60,  60,  60)
#define CLR_BRANCH  M5Dial.Display.color888(  0,   0,  80)
#define CLR_LEAF    M5Dial.Display.color888(  0,  80,   0)
#define CLR_BACK    M5Dial.Display.color888( 50,  50,  50)

void showStatus(const String& msg, uint32_t bg) {
  int w = M5Dial.Display.width();
  int h = M5Dial.Display.height();
  display.startWrite();
  canvas.deleteSprite();
  canvas.createSprite(w, h);
  canvas.fillSprite(bg);
  canvas.setFont(&fonts::Orbitron_Light_24);
  canvas.setTextColor(M5Dial.Display.color888(255, 255, 255));
  canvas.setTextDatum(middle_center);
  canvas.drawString(msg, w / 2, h / 2);
  canvas.pushSprite(0, 0);
  display.endWrite();
}

void displayCurrentItem() {
  if (!currentItems || totalItems() == 0) {
    showStatus("No items", CLR_ERROR);
    return;
  }

  int w  = M5Dial.Display.width();
  int h  = M5Dial.Display.height();
  int cx = w / 2;
  int cy = h / 2;

  bool   back = isBackItem(currentIndex);
  bool   leaf = isLeafItem(currentIndex);
  String name = back ? "< Back" : (*currentItems)[currentIndex].displayName;

  display.startWrite();
  canvas.deleteSprite();
  canvas.createSprite(w, h);

  canvas.fillSprite(M5Dial.Display.color888(255, 255, 255));

  if (back) {
    const ImageAsset* asset = findImage("back.jpg");
    if (asset) canvas.drawJpg(asset->data, asset->len, 0, -13, w, h, 0, 0, 0.7f, 0.7f, middle_center);
  } else {
    // Try item's own image, fall back to top-level category image if not found
    String imgName = (*currentItems)[currentIndex].image;
    const ImageAsset* asset = imgName.length() > 0 ? findImage(imgName) : nullptr;
    if (!asset && !navStack.empty()) {
      imgName = (*navStack[0].items)[navStack[0].selectedIndex].image;
      asset = imgName.length() > 0 ? findImage(imgName) : nullptr;
    }
    if (asset) canvas.drawJpg(asset->data, asset->len, 0, -13, w, h, 0, 0, 0.7f, 0.7f, middle_center);
  }

  // Solid bar at bottom so text is always readable
  canvas.fillRect(0, h - 47, w, 47, M5Dial.Display.color888(16, 24, 48));

  // Item name — shrink slightly for longer strings so they fit
  canvas.setFont(&fonts::Orbitron_Light_24);
  canvas.setTextColor(leaf ? M5Dial.Display.color888(255, 140, 0) : M5Dial.Display.color888(255, 255, 255));
  canvas.setTextDatum(bottom_center);
  canvas.setTextSize(name.length() >= 10 ? 0.72f : name.length() >= 8 ? 0.85f : 1.0f);
  canvas.drawString(name, cx, h - 8);
  canvas.setTextSize(1.0f);

  // Position indicator at top
  canvas.setFont(&fonts::Font2);
  canvas.setTextColor(M5Dial.Display.color888(255, 255, 255));
  canvas.setTextDatum(top_center);
  canvas.drawString(String(currentIndex + 1) + "/" + String(totalItems()), cx, 8);

  // Branch indicator — ">" on right edge means "press to enter"
  if (!back && !leaf) {
    canvas.setTextDatum(middle_right);
    canvas.drawString(">", w - 8, cy);
  }

  canvas.pushSprite(0, 0);
  display.endWrite();
}

// ─── Setup & loop ─────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);
  display.begin();

  loadConfig();

  if (!loadItems()) {
    Serial.println("items parse failed");
    showStatus("Items err", CLR_ERROR);
    while (true) delay(1000);
  }
  Serial.println("Items loaded OK");

  // Show device ID briefly so the operator knows which unit this is
  showStatus(config.deviceId, CLR_NEUTRAL);
  delay(3000);

  showStatus("Connecting", CLR_INFO);

  transport.setUseTls(true);
  transport.setCerts(grafanaCert, strlen(grafanaCert));
  transport.setWifiSsid(config.wifiSsid.c_str());
  transport.setWifiPass(config.wifiPassword.c_str());
  transport.setDebug(Serial);

  if (!transport.begin()) {
    Serial.println(transport.errmsg);
    showStatus("WiFi fail!", CLR_ERROR);
    while (true) delay(1000);
  }

  client.setUrl(config.gcUrl.c_str());
  client.setPath((char*)config.gcPath.c_str());
  client.setPort(config.gcPort);
  client.setUser(config.gcUser.c_str());
  client.setPass(config.gcPass.c_str());
  client.setDebug(Serial);

  if (!client.begin()) {
    showStatus("Client err", CLR_ERROR);
    while (true) delay(1000);
  }

  showStatus("Connected!", CLR_OK);
  M5Dial.Speaker.tone(8000, 200);
  delay(3000);

  navigateToRoot();
  displayCurrentItem();
}

void loop() {
  M5Dial.update();

  long pos  = M5Dial.Encoder.read();
  long diff = pos - lastEncoderPos;

  if (abs(diff) >= config.encoderSensitivity) {
    int dir  = (diff > 0) ? 1 : -1;
    currentIndex = (currentIndex + dir + totalItems()) % totalItems();
    M5Dial.Encoder.write(0);
    lastEncoderPos = 0;
    M5Dial.Speaker.tone(8000, 20);
    displayCurrentItem();
  }

  if (M5Dial.BtnA.wasClicked()) {
    if (isBackItem(currentIndex)) {
      navigateBack();
      displayCurrentItem();
    } else if (isLeafItem(currentIndex)) {
      showStatus("Sending...", CLR_INFO);
      if (sendMetric()) {
        showStatus("Thanks!", CLR_OK);
        M5Dial.Speaker.tone(8000, 300);
      } else {
        showStatus("Error!", CLR_ERROR);
      }
      delay(2000);
      navigateToRoot();
      displayCurrentItem();
    } else {
      navigateInto();
      displayCurrentItem();
    }
  }
}
