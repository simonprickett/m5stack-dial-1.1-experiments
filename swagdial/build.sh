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
  setup          Install board package and libraries (run once)
  build          Compile the sketch
  flash <port>   Compile and flash sketch to device
  monitor <port> Open serial monitor at 115200 baud

To find your device port after connecting the M5Dial via USB:
  arduino-cli board list

Examples:
  $(basename "$0") setup
  $(basename "$0") build
  $(basename "$0") flash /dev/cu.usbmodem101
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

check_arduino_cli

case "${1:-}" in
  setup)   cmd_setup ;;
  build)   cmd_build ;;
  flash)   cmd_flash "${2:-}" ;;
  monitor) cmd_monitor "${2:-}" ;;
  *)       usage; exit 1 ;;
esac
