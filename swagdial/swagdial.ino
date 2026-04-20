#include "M5Dial.h"
#include "certificates.h"
#include "config.h"
#include "images/tshirt_rgb565.h"
#include "images/sticker_rgb565.h"
#include "images/coffee_rgb565.h"
#include "images/patch_rgb565.h"
#include "images/crochet_rgb565.h"
#include "images/keychain_rgb565.h"
#include "images/coin_rgb565.h"
#include "images/socks_rgb565.h"
#include "images/back_rgb565.h"
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
      "label_value": "coffee",
      "display_name": "Coffee",
      "image": "coffee.jpg",
      "children": [
        { "label_key": "drink", "label_value": "americano",  "display_name": "Americano",  "image": "coffee.jpg" },
        { "label_key": "drink", "label_value": "cappuccino", "display_name": "Cappuccino", "image": "coffee.jpg" },
        { "label_key": "drink", "label_value": "espresso",   "display_name": "Espresso",   "image": "coffee.jpg" },
        { "label_key": "drink", "label_value": "latte",      "display_name": "Latte",      "image": "coffee.jpg" },
        { "label_key": "drink", "label_value": "mocha",      "display_name": "Mocha",      "image": "coffee.jpg" }
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
      "label_value": "crochet",
      "display_name": "Crochet",
      "image": "crochet.jpg"
    },
    {
      "label_key": "category",
      "label_value": "keychain",
      "display_name": "Keychain",
      "image": "keychain.jpg",
      "children": [
        { "label_key": "design", "label_value": "doom", "display_name": "Doom", "image": "keychain.jpg" },
        { "label_key": "design", "label_value": "logo", "display_name": "Logo", "image": "keychain.jpg" }
      ]
    },
    {
      "label_key": "category",
      "label_value": "patch",
      "display_name": "Patch",
      "image": "patch.jpg",
      "children": [
        { "label_key": "type", "label_value": "champion",    "display_name": "Champion",    "image": "patch.jpg" },
        { "label_key": "type", "label_value": "contributor", "display_name": "Contributor", "image": "patch.jpg" }
      ]
    },
    {
      "label_key": "category",
      "label_value": "socks",
      "display_name": "Socks",
      "image": "socks.jpg"
    },
    {
      "label_key": "category",
      "label_value": "sticker",
      "display_name": "Sticker",
      "image": "sticker.jpg",
      "children": [
        {
          "label_key": "design",
          "label_value": "grafanacon",
          "display_name": "GrafanaCon"
        },
        {
          "label_key": "design",
          "label_value": "grot",
          "display_name": "Grot",
          "children": [
            { "label_key": "item", "label_value": "book",    "display_name": "Book"    },
            { "label_key": "item", "label_value": "hoodie",  "display_name": "Hoodie"  },
            { "label_key": "item", "label_value": "laptop",  "display_name": "Laptop"  },
            { "label_key": "item", "label_value": "meetups", "display_name": "Meetups" },
            { "label_key": "item", "label_value": "outline", "display_name": "Outline" },
            { "label_key": "item", "label_value": "plant",   "display_name": "Plant"   }
          ]
        },
        {
          "label_key": "design",
          "label_value": "logos",
          "display_name": "Logos",
          "children": [
            { "label_key": "item", "label_value": "grafana",   "display_name": "Grafana"   },
            { "label_key": "item", "label_value": "k6",        "display_name": "K6"        },
            { "label_key": "item", "label_value": "loki",      "display_name": "Loki"      },
            { "label_key": "item", "label_value": "mimir",     "display_name": "Mimir"     },
            { "label_key": "item", "label_value": "otel",      "display_name": "OTel"      },
            { "label_key": "item", "label_value": "pyroscope", "display_name": "Pyroscope" },
            { "label_key": "item", "label_value": "tempo",     "display_name": "Tempo"     }
          ]
        },
        {
          "label_key": "design",
          "label_value": "sci_fair",
          "display_name": "Sci Fair",
          "children": [
            { "label_key": "item", "label_value": "doom",     "display_name": "Doom"     },
            { "label_key": "item", "label_value": "lab_coat", "display_name": "Lab Coat" }
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
        { "label_key": "design", "label_value": "football",    "display_name": "Football"    },
        { "label_key": "design", "label_value": "grot",        "display_name": "Grot"        },
        { "label_key": "design", "label_value": "logo",        "display_name": "Logo"        },
        { "label_key": "design", "label_value": "logo_mosaic", "display_name": "Logo Mosaic" }
      ]
    }
  ]
})";

// ─── Image assets ─────────────────────────────────────────────────────────────

static const ImageAsset IMAGE_ASSETS[] = {
  { "tshirt.jpg",   tshirt_rgb565   },
  { "sticker.jpg",  sticker_rgb565  },
  { "coffee.jpg",   coffee_rgb565   },
  { "patch.jpg",    patch_rgb565    },
  { "crochet.jpg",  crochet_rgb565  },
  { "keychain.jpg", keychain_rgb565 },
  { "coin.jpg",     coin_rgb565     },
  { "socks.jpg",    socks_rgb565    },
  { "back.jpg",     back_rgb565     },
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
  Serial.printf("send: heap=%d psram=%d\n", ESP.getFreeHeap(), ESP.getFreePsram());
  String labels = buildLabels();
  WriteRequest req(1);
  TimeSeries ts(1, metricName.c_str(), labels.c_str());
  req.addTimeSeries(ts);
  ts.addSample(transport.getTimeMillis(), 1);
  PromClient::SendResult res = client.send(req);
  if (res != PromClient::SendResult::SUCCESS) {
    Serial.printf("send failed: %s\n", client.errmsg);
    return false;
  }
  Serial.println("send OK");
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
  M5Dial.Display.fillScreen(bg);
  M5Dial.Display.setFont(&fonts::Orbitron_Light_24);
  M5Dial.Display.setTextColor(M5Dial.Display.color888(255, 255, 255));
  M5Dial.Display.setTextDatum(middle_center);
  M5Dial.Display.drawString(msg, w / 2, h / 2);
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

  // Resolve which image asset to display
  const ImageAsset* asset = nullptr;
  if (back) {
    asset = findImage("back.jpg");
  } else {
    String imgName = (*currentItems)[currentIndex].image;
    asset = imgName.length() > 0 ? findImage(imgName) : nullptr;
    if (!asset && !navStack.empty()) {
      imgName = (*navStack[0].items)[navStack[0].selectedIndex].image;
      asset = imgName.length() > 0 ? findImage(imgName) : nullptr;
    }
  }

  M5Dial.Display.startWrite();
  M5Dial.Display.fillScreen(M5Dial.Display.color888(255, 255, 255));

  if (asset) {
    M5Dial.Display.pushImage(36, 23, 168, 168, asset->data);
  }

  // Solid bar at bottom so text is always readable
  M5Dial.Display.fillRect(0, h - 47, w, 47, M5Dial.Display.color888(16, 24, 48));

  // Item name — shrink slightly for longer strings so they fit
  M5Dial.Display.setFont(&fonts::Orbitron_Light_24);
  M5Dial.Display.setTextColor(leaf ? M5Dial.Display.color888(255, 140, 0) : M5Dial.Display.color888(255, 255, 255));
  M5Dial.Display.setTextDatum(bottom_center);
  M5Dial.Display.setTextSize(name.length() >= 10 ? 0.72f : name.length() >= 8 ? 0.85f : 1.0f);
  M5Dial.Display.drawString(name, cx, h - 8);
  M5Dial.Display.setTextSize(1.0f);

  // Position indicator at top
  M5Dial.Display.setFont(&fonts::Font2);
  M5Dial.Display.setTextColor(M5Dial.Display.color888(255, 255, 255));
  M5Dial.Display.setTextDatum(top_center);
  M5Dial.Display.drawString(String(currentIndex + 1) + "/" + String(totalItems()), cx, 8);

  // Branch indicator — ">" on right edge means "press to enter"
  if (!back && !leaf) {
    M5Dial.Display.setTextDatum(middle_right);
    M5Dial.Display.drawString(">", w - 8, cy);
  }

  M5Dial.Display.endWrite();
}

// ─── Setup & loop ─────────────────────────────────────────────────────────────

void setup() {
  Serial.begin(115200);
  auto cfg = M5.config();
  M5Dial.begin(cfg, true, false);

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

  Serial.printf("heap after wifi: %d  maxAlloc: %d\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
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
    displayCurrentItem();
    M5Dial.Speaker.tone(8000, 20);
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
