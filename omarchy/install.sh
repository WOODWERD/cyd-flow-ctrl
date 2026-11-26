#!/bin/bash
# Set up Flow Ctrl on Omarchy: the helper commands, the Hyprland binds and the Transform prompts.
# Run from a clone of the repo:   ./omarchy/install.sh
# Safe to run again; it never overwrites prompts or settings you've edited.
# https://github.com/WOODWERD/cyd-flow-ctrl
set -euo pipefail

here=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
config=${XDG_CONFIG_HOME:-$HOME/.config}
bin=$HOME/.local/bin
cfg=$config/flow-ctrl
hypr=$config/hypr

say() { printf '  %s\n' "$*"; }
echo "Flow Ctrl for Omarchy"

# 1. Tools. Omarchy ships most of these; wtype is only needed on older releases.
missing=()
for c in curl jq wl-copy wtype; do command -v "$c" >/dev/null || missing+=("$c"); done
if ((${#missing[@]})); then
  pkgs=$(printf '%s\n' "${missing[@]}" | sed 's/^wl-copy$/wl-clipboard/' | sort -u | tr '\n' ' ')
  say "Installing $pkgs"
  sudo pacman -S --needed --noconfirm $pkgs
fi

# 2. Commands
mkdir -p "$bin"
install -m 755 "$here/bin/flow-ctrl-dictate" "$here/bin/flow-ctrl-transform" "$bin/"
say "Commands in $bin"

# 3. Prompts and settings (yours are kept)
mkdir -p "$cfg/transforms"
for f in "$here"/transforms/*.md; do
  [[ -e $cfg/transforms/${f##*/} ]] || cp "$f" "$cfg/transforms/"
done
if [[ ! -e $cfg/env ]]; then
  printf '%s\n' '# Flow Ctrl settings. Transforms need an Anthropic API key (console.anthropic.com).' \
    '# ANTHROPIC_API_KEY=sk-ant-...' '# FLOW_CTRL_MODEL=claude-opus-5-5' '# FLOW_CTRL_EFFORT=low' > "$cfg/env"
  chmod 600 "$cfg/env"
fi
say "Prompts in $cfg/transforms, settings in $cfg/env"

# 4. Hyprland binds: the Lua config on current Omarchy, hyprland.conf on older releases
if [[ -f $hypr/hyprland.lua ]]; then
  # full paths, so the binds work whatever PATH Hyprland was started with
  sed -e "s|\"flow-ctrl-|\"$bin/flow-ctrl-|g" "$here/flow-ctrl.lua" > "$hypr/flow-ctrl.lua"
  line='require("hypr.flow-ctrl")  -- Flow Ctrl desk button'
  touch "$hypr/bindings.lua"
  grep -qF 'require("hypr.flow-ctrl")' "$hypr/bindings.lua" || printf '\n%s\n' "$line" >> "$hypr/bindings.lua"
  say "Binds in $hypr/flow-ctrl.lua, loaded from bindings.lua"
elif [[ -f $hypr/hyprland.conf ]]; then
  sed -e "s|exec, flow-ctrl-|exec, $bin/flow-ctrl-|g" "$here/flow-ctrl.conf" > "$hypr/flow-ctrl.conf"
  grep -qF 'flow-ctrl.conf' "$hypr/hyprland.conf" || printf '\nsource = ~/.config/hypr/flow-ctrl.conf  # Flow Ctrl desk button\n' >> "$hypr/hyprland.conf"
  say "Binds in $hypr/flow-ctrl.conf, sourced from hyprland.conf"
else
  echo "No Hyprland config found in $hypr. Is this Omarchy?" >&2
  exit 1
fi
command -v hyprctl >/dev/null && hyprctl reload >/dev/null && say "Hyprland reloaded"

# 5. What's left
echo
command -v omarchy-dictation >/dev/null || command -v voxtype >/dev/null ||
  say "Next: pick a dictation backend in the Omarchy menu: Setup > Defaults > Dictation."
grep -q '^ANTHROPIC_API_KEY=' "$cfg/env" || say "For Transforms: put your Anthropic API key in $cfg/env"
say "Pair \"Flow Ctrl\" in Bluetooth settings (Super + Ctrl + B or the bar), then hold the bar and talk."
