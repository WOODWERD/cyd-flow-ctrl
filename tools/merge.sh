#!/usr/bin/env bash
# Package a PlatformIO build for release. Usage: tools/merge.sh [env] [version]   e.g. tools/merge.sh cyd2usb-win
#   flow-ctrl-<ver>-<env>.bin          full image at 0x0 — a brand-new board only. It also blanks the
#                                      NVS area (0x9000-0xDFFF), which wipes the Bluetooth pairing.
#   flow-ctrl-<ver>-<env>-update.bin   the app alone, at 0x10000 — updates; keeps the pairing.
#   flow-ctrl-<ver>-bootloader.bin / -partitions.bin / -boot_app0.bin
#                                      the other pieces, at 0x1000 / 0x8000 / 0xE000. The web flasher
#                                      (docs/flash/manifest*.json) writes these four at their own
#                                      addresses, so it never touches the pairing either.
set -e
ENV=${1:-cyd2usb-mac}; VER=${2:-v$(sed -n 's/.*FW_VERSION "\(.*\)"/\1/p' include/version.h)}
B=.pio/build/$ENV
APP0=$(find ~/.platformio/packages/framework-arduinoespressif32 -name boot_app0.bin | head -1)
ESPT=$(find ~/.platformio/packages/tool-esptoolpy -name esptool.py | head -1)
python3 "$ESPT" --chip esp32 merge_bin -o flow-ctrl-$VER-$ENV.bin --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 $B/bootloader.bin 0x8000 $B/partitions.bin 0xe000 "$APP0" 0x10000 $B/firmware.bin
cp $B/firmware.bin   flow-ctrl-$VER-$ENV-update.bin
cp $B/bootloader.bin flow-ctrl-$VER-bootloader.bin
cp $B/partitions.bin flow-ctrl-$VER-partitions.bin
cp "$APP0"           flow-ctrl-$VER-boot_app0.bin
ls -la flow-ctrl-$VER-$ENV.bin flow-ctrl-$VER-$ENV-update.bin flow-ctrl-$VER-bootloader.bin flow-ctrl-$VER-partitions.bin flow-ctrl-$VER-boot_app0.bin
