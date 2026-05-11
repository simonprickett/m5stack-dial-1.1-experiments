#pragma once
#include <Arduino.h>
#include <vector>

struct Config {
  String wifiSsid;
  String wifiPassword;
  String deviceId;
  String gcUrl;
  String gcPath;
  int    gcPort;
  String gcUser;
  String gcPass;
  int    encoderSensitivity;
};

struct MenuItem {
  String                labelKey;
  String                labelValue;
  String                displayName;
  String                image;
  std::vector<MenuItem> children;
};

struct NavFrame {
  std::vector<MenuItem>* items;
  int                    selectedIndex;
};

struct ImageAsset {
  const char*     name;
  const uint16_t* data;
};
