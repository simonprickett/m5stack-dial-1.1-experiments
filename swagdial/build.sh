#!/usr/bin/env bash
# build.sh — compile and flash swagdial for M5Stack Dial 1.1
set -euo pipefail

FQBN="esp32:esp32:m5stack_dial"
BOARD_URL="https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json"
SKETCH_DIR="$(cd "$(dirname "$0")" && pwd)"

# arduino-cli stores packages under ~/Library/Arduino15 on macOS and $ARDUINO15 elsewhere
if [[ -d "$HOME/Library/Arduino15" ]]; then
  ARDUINO15="$HOME/Library/Arduino15"
else
  ARDUINO15="$HOME/.arduino15"
fi

usage() {
  cat <<EOF
Usage: $(basename "$0") <command> [options]

Commands:
  setup                Install board package and libraries (run once)
  build                Compile the sketch
  flash <port>         Compile and flash sketch to device
  data-flash <port>    Build and flash the LittleFS filesystem image
  monitor <port>       Open serial monitor at 115200 baud

To find your device port after connecting the M5Dial via USB:
  arduino-cli board list

Examples:
  $(basename "$0") setup
  $(basename "$0") build
  $(basename "$0") flash /dev/cu.usbmodem101
  $(basename "$0") data-flash /dev/cu.usbmodem101
  $(basename "$0") monitor /dev/cu.usbmodem101
EOF
}

check_arduino_cli() {
  if ! command -v arduino-cli &>/dev/null; then
    echo "Error: arduino-cli not found."
    echo "Install with: brew install arduino-cli"
    exit 1
  fi
}

cmd_setup() {
  echo "==> Initialising arduino-cli config..."
  arduino-cli config init 2>/dev/null || true

  echo "==> Adding ESP32 board manager URL..."
  if ! arduino-cli config dump | grep -qF "$BOARD_URL"; then
    arduino-cli config add board_manager.additional_urls "$BOARD_URL"
  else
    echo "    (already present)"
  fi

  echo "==> Updating board index..."
  arduino-cli core update-index

  echo "==> Installing ESP32 core..."
  arduino-cli core install esp32:esp32

  echo "==> Installing libraries..."
  arduino-cli lib install "M5Unified"
  arduino-cli lib install "M5GFX"
  arduino-cli lib install "ArduinoJson@6.21.6"
  arduino-cli lib install "PrometheusArduino"
  arduino-cli lib install "PromLokiTransport"

  echo "==> Setup complete."
}

cmd_build() {
  echo "==> Compiling for $FQBN..."
  arduino-cli compile --fqbn "$FQBN" "$SKETCH_DIR"
  echo "==> Build OK."
}

cmd_flash() {
  local port="${1:-}"
  if [[ -z "$port" ]]; then
    echo "Error: port required. Find yours with: arduino-cli board list"
    exit 1
  fi
  cmd_build
  echo "==> Flashing to $port..."
  arduino-cli upload -p "$port" --fqbn "$FQBN" "$SKETCH_DIR"
  echo "==> Flash complete."
}

cmd_monitor() {
  local port="${1:-}"
  if [[ -z "$port" ]]; then
    echo "Error: port required. Find yours with: arduino-cli board list"
    exit 1
  fi
  echo "==> Opening serial monitor on $port (Ctrl+C to exit)..."
  arduino-cli monitor -p "$port" --config baudrate=115200
}

cmd_data_flash() {
  local port="${1:-}"
  if [[ -z "$port" ]]; then
    echo "Error: port required. Find yours with: arduino-cli board list"
    exit 1
  fi

  local data_dir="$SKETCH_DIR/data"
  if [[ ! -d "$data_dir" ]]; then
    echo "Error: $data_dir not found."
    echo "Create it with config.json, items.json, and an images/ subfolder — see README."
    exit 1
  fi

  echo "==> Locating build tools..."

  # mkspiffs is bundled with the ESP32 core
  local mkspiffs
  mkspiffs=$(find $ARDUINO15/packages/esp32/tools/mkspiffs \
    -name "mkspiffs" -type f 2>/dev/null | sort -V | tail -1)
  if [[ -z "$mkspiffs" ]]; then
    echo "Error: mkspiffs not found. Run ./build.sh setup first."
    exit 1
  fi

  # esptool: check PATH first, then fall back to the bundled binary
  local -a esptool_cmd
  if command -v esptool.py &>/dev/null; then
    esptool_cmd=(esptool.py)
  elif command -v esptool &>/dev/null; then
    esptool_cmd=(esptool)
  else
    local esptool_bin
    esptool_bin=$(find $ARDUINO15/packages/esp32/tools/esptool_py \
      -name "esptool" -not -name "*.py" -type f 2>/dev/null | sort -V | tail -1)
    if [[ -n "$esptool_bin" ]]; then
      esptool_cmd=("$esptool_bin")
    else
      echo "Error: esptool not found. Install with: pip3 install esptool"
      exit 1
    fi
  fi

  echo "    mkspiffs:   $mkspiffs"
  echo "    esptool:    ${esptool_cmd[*]}"

  # Locate the partition CSV for this board via boards.txt
  echo "==> Looking up partition table for $FQBN..."
  local board_name="${FQBN##*:}"
  local hw_dir
  hw_dir=$(find $ARDUINO15/packages/esp32/hardware/esp32 \
    -maxdepth 1 -mindepth 1 -type d 2>/dev/null | sort -V | tail -1)
  if [[ -z "$hw_dir" ]]; then
    echo "Error: ESP32 hardware directory not found. Run ./build.sh setup first."
    exit 1
  fi

  local csv_name
  csv_name=$(grep "^${board_name}\.build\.partitions=" "$hw_dir/boards.txt" \
    2>/dev/null | cut -d= -f2)
  if [[ -z "$csv_name" ]]; then
    echo "Error: board '$board_name' not found in boards.txt."
    echo "Verify your FQBN with: arduino-cli board listall | grep -i dial"
    exit 1
  fi

  local csv="$hw_dir/tools/partitions/${csv_name}.csv"
  if [[ ! -f "$csv" ]]; then
    echo "Error: partition CSV not found: $csv"
    exit 1
  fi
  echo "    partitions: $csv ($csv_name)"

  # Parse the LittleFS / spiffs row — columns: Name, Type, SubType, Offset, Size
  local part_line offset size
  part_line=$(grep -v "^#" "$csv" | grep -i "spiffs\|littlefs" | head -1)
  if [[ -z "$part_line" ]]; then
    echo "Error: no LittleFS/spiffs partition found in $csv"
    exit 1
  fi
  offset=$(echo "$part_line" | awk -F',' '{gsub(/ /,""); print $4}')
  size=$(echo "$part_line"   | awk -F',' '{gsub(/ /,""); print $5}')
  echo "    offset:     $offset  size: $size"

  # Build the SPIFFS image
  local img="$SKETCH_DIR/spiffs.bin"
  echo "==> Building SPIFFS image from $data_dir..."
  "$mkspiffs" -c "$data_dir" -s "$size" -b 4096 -p 256 "$img"

  # Flash
  echo "==> Flashing filesystem to $port..."
  "${esptool_cmd[@]}" --port "$port" write-flash "$offset" "$img"

  echo "==> Filesystem upload complete."
}

check_arduino_cli

case "${1:-}" in
  setup)      cmd_setup ;;
  build)      cmd_build ;;
  flash)      cmd_flash "${2:-}" ;;
  data-flash) cmd_data_flash "${2:-}" ;;
  monitor)    cmd_monitor "${2:-}" ;;
  *)          usage; exit 1 ;;
esac
