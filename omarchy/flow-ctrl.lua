-- Flow Ctrl on Omarchy: Hyprland binds for the chords the cyd2usb-linux firmware sends.
-- Installed as ~/.config/hypr/flow-ctrl.lua and loaded from ~/.config/hypr/bindings.lua with
--   require("hypr.flow-ctrl")
-- Dictation goes to whichever backend Omarchy has selected (Setup > Defaults > Dictation).
-- https://github.com/WOODWERD/cyd-flow-ctrl

-- HOLD TO TALK: the board holds Ctrl + Alt + T for as long as your finger is on the bar.
o.bind_hold("CTRL + ALT + T", "Flow Ctrl: start dictation", "flow-ctrl-dictate start",
  "Flow Ctrl: stop dictation", "flow-ctrl-dictate stop")

-- Hands-free: one chord to lock in, the same chord to stop and type it all.
o.bind("CTRL + ALT + H", "Flow Ctrl: hands-free dictation", "flow-ctrl-dictate toggle")

-- Discard during hands-free: cancel, nothing is typed.
o.bind("CTRL + ALT + X", "Flow Ctrl: cancel dictation", "flow-ctrl-dictate cancel")

-- Transforms: copy the selection the same way Omarchy's universal copy does (Ctrl + Shift + C in
-- a terminal), then hand it to flow-ctrl-transform, which rewrites it with Claude and pastes it back.
local function is_terminal()
  local window = hl.get_active_window()
  for _, tag in ipairs((window and window.tags) or {}) do
    if tag:gsub("%*$", "") == "terminal" then
      return true
    end
  end
  return false
end

local function transform(name)
  return function()
    local mods = is_terminal() and "CTRL SHIFT" or "CTRL"
    hl.exec_cmd("wl-copy --clear")
    hl.timer(function()
      hl.dispatch(hl.dsp.send_key_state({ mods = mods, key = "C", state = "down" }))
      hl.timer(function()
        hl.dispatch(hl.dsp.send_key_state({ mods = mods, key = "C", state = "up" }))
        hl.timer(function()
          hl.exec_cmd("flow-ctrl-transform --copied " .. name)
        end, { timeout = 150, type = "oneshot" })
      end, { timeout = 50, type = "oneshot" })
    end, { timeout = 120, type = "oneshot" })   -- let the board's Ctrl + Alt come up first
  end
end

o.bind("CTRL + ALT + 2", "Flow Ctrl: Prompt Engineer", transform("prompt-engineer"))
o.bind("CTRL + ALT + 3", "Flow Ctrl: Drawing Note", transform("drawing-note"))
o.bind("CTRL + ALT + 4", "Flow Ctrl: Client Email", transform("client-email"))
