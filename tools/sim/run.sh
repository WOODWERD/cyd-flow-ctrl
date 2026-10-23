#!/usr/bin/env bash
# Render the real firmware's screens on a desktop (no board) and rebuild the docs images from them.
# Needs gcc/g++, git, python3 + Pillow. Usage (from the repo root): tools/sim/run.sh
set -e
ROOT=$(cd "$(dirname "$0")/../.." && pwd); S=$ROOT/tools/sim; W=$S/.build
mkdir -p $W/o
[ -d $W/lvgl ] || git clone -q --depth 1 --branch v9.5.0 https://github.com/lvgl/lvgl $W/lvgl
FL="-O1 -w -DLV_CONF_INCLUDE_SIMPLE -DLV_LVGL_H_INCLUDE_SIMPLE -I $S/stub -I $ROOT/include -I $ROOT/src -I $W/lvgl -I $W/lvgl/src"
export FL
find $W/lvgl/src -name '*.c' | xargs -P 4 -I{} sh -c 'o='$W'/o/$(echo {} | md5sum | cut -c1-12).o; [ -f $o ] || gcc -c $FL {} -o $o'
for f in $ROOT/src/assets/*.c; do gcc -c $FL $f -o $W/o/a_$(basename $f .c).o; done
g++ -std=gnu++17 -c $FL $S/stub/ble_kbd_impl.cpp -o $W/o/ble.o
g++ -std=gnu++17 -c $FL -include lvgl.h $S/harness.cpp -o $W/harness.o
g++ $W/harness.o $W/o/*.o -o $W/sim -lm
(cd $W && ./sim)
python3 $S/make_guide.py $W $ROOT/docs
