# Flash it

The quickest way to put Flow Ctrl on a board: plug it in, open this page in **Chrome or Edge**
(Web Serial — Safari and Firefox can't), press the button, pick the port.

<script type="module" src="https://unpkg.com/esp-web-tools@10/dist/web/install-button.js?module"></script>
<esp-web-install-button manifest="manifest.json">
  <span slot="unsupported">This browser can't talk to serial ports — use Chrome or Edge.</span>
  <span slot="not-allowed">Serial access is blocked here (needs HTTPS).</span>
</esp-web-install-button>

!!! note "Which board"
    The prebuilt image is for the **USB-C / 2-USB** Cheap Yellow Display (ST7789 panel). The original
    micro-USB board needs the `cyd` build — see [Firmware & code](../firmware.md).

If the port doesn't appear, hold **BOOT** on the board while plugging it in, then try again. Some
boards need a USB-C → USB-A cable rather than C-to-C.

## From a terminal instead

```
pip install esptool
esptool.py --chip esp32 --port /dev/cu.usbserial-XXXX --baud 460800 write_flash 0x0 flow-ctrl-v0.9.3-cyd2usb.bin
```

The image is in the repo under `release/`, and attached to each GitHub release.

## Then

Pair it and set the one Wispr shortcut — [Getting started](../GETTING_STARTED.md#1-set-up-once).
