#!/usr/bin/env bash
# Build a single 0x0 flash image from the PlatformIO build. Usage: tools/merge.sh [env] [version]
set -e
ENV=${1:-cyd2usb}; VER=${2:-v0.1}
B=.pio/build/$ENV
APP0=$(find ~/.platformio/packages/framework-arduinoespressif32 -name boot_app0.bin | head -1)
ESPT=$(find ~/.platformio/packages/tool-esptoolpy -name esptool.py | head -1)
python3 "$ESPT" --chip esp32 merge_bin -o flow-ctrl-$VER-$ENV.bin --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 $B/bootloader.bin 0x8000 $B/partitions.bin 0xe000 "$APP0" 0x10000 $B/firmware.bin
ls -la flow-ctrl-$VER-$ENV.bin
