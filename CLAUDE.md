# zmk-corne — Claude project context

ZMK firmware for a handwired split keyboard built in a Dactyl-Manuform-style case
with **nice_nano v2** controllers on both halves. Branch in active development:
`corne-dactyl` (do not work on `main`).

## Hardware

- **MCUs:** nice_nano v2 (nRF52840) on both halves
- **Matrix:** 4 rows × 6 cols per half, `col2row`, no diodes per col offset on left
  - Rows: P0.31, P0.29, P0.02, P1.15
  - Cols: P0.17, P0.20, P0.22, P0.24, P1.00, P0.11
- **OLED display:** 0.91" SSD1306, 128×32, I²C @ `0x3C`, 4-pin module with onboard
  ~6.5 kΩ pull-ups. Module labels SCK (= SCL) and SDA.
- **Display location:** **left half** (central). Display was originally on the right
  but moved to the left — see "Why the display lives on the central" below.
- **Display wiring (left half):**
  - VCC → 3.3V
  - GND → GND
  - SDA → **P1.13**
  - SCK (SCL) → **P1.11**
- **Encoder:** removed for now. Will be relocated to the right half later.

## Build / flash

GitHub Actions builds artifacts on push. UF2 outputs:
- `corne_handwired_left-nice_nano_v2-zmk.uf2`
- `corne_handwired_right-nice_nano_v2-zmk.uf2`
- `settings_reset-...-zmk.uf2`

**Flash order after major changes:** settings_reset on both halves first, then
the new firmware on each. Power-cycle to re-pair.

## Critical build pitfalls

1. **Peripheral build omits keymap and layer events.**
   `zmk/app/CMakeLists.txt:48` gates `keymap.c` and `events/layer_state_changed.c`
   behind `(NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL`. The peripheral
   image *cannot* link `zmk_keymap_highest_layer_active()` or
   `zmk_event_zmk_layer_state_changed`. Any layer-aware widget must run on central.

2. **Shield `.conf` does not merge reliably.**
   Defaults belong in `boards/shields/corne_handwired/Kconfig.defconfig`, not in
   `corne_handwired.conf`. The build's merge log only consistently picks up
   `config/<shield>.conf` and `prj.conf`.

3. **`ZMK_DISPLAY` implies `LV_CONF_MINIMAL`.**
   That defaults `LV_USE_LABEL=n` and Montserrat fonts to `n`. Custom status
   screens that use labels must re-enable them in `Kconfig.defconfig`:
   ```
   config LV_USE_LABEL
       default y
   config LV_FONT_MONTSERRAT_14
       default y
   ```

4. **ZMK custom widgets need ZMK's private headers.**
   `CMakeLists.txt` adds `${ZEPHYR_BASE}/../zmk/app/include` to the include path so
   widgets can `#include <zmk/display.h>` etc.

## Why the display lives on the central (left)

The original plan put the OLED on the right (peripheral) half. That hit the
peripheral-build pitfall above: the layer widget couldn't link. Rather than
write a custom split BLE service to forward layer state, we moved the display to
the left/central. Same pin numbers (P1.13/P1.11) — the user rewired physically.

## Display software stack

- **Driver:** `solomon,ssd1306fb` (Zephyr's SSD1306 framebuffer driver)
- **GUI:** LVGL 9 (rotation API is `LV_DISP_ROTATION_90`, *not* the older `LV_DISP_ROT_90`)
- **Status screen choice:** `ZMK_DISPLAY_STATUS_SCREEN_CUSTOM`, implementing
  `zmk_display_status_screen()` in `boards/shields/corne_handwired/custom_status_screen.c`
- **LVGL config (in `Kconfig.defconfig` under `if LVGL`):**
  - `LV_Z_VDB_SIZE = 64`
  - `LV_DPI_DEF = 148`
  - `LV_Z_BITS_PER_PIXEL = 1`
  - `LV_COLOR_DEPTH_1`
  - `LV_USE_LABEL = y`, `LV_FONT_MONTSERRAT_14 = y`

## Display feature plan

**Phase 1 — vertical 32×128 layout, top-to-bottom:**
- "AS" logo (top)
- Layer number, large font (center)
- Peripheral connection icon (bottom) — ✓ / ✗ for right-half link

**Phase 2:**
- Caps-lock indicator
- Modifier display (Shift/Ctrl/Alt/GUI)
- WPM counter
- Battery widget (left half)

## Open issues / current state

- **Custom status screen hangs the central on boot.**
  Built-in status screen boots fine, custom screen causes USB to fail with
  `error -110` and tiny squares on the OLED. Currently bisecting:
  - Step 0 (verified): built-in status screen — boots, icons render.
  - Step 1 (pushed in `ee78f77`): custom screen with **only rotation + "AS" label**,
    no layer widget. Next step depends on result.
  - If Step 1 boots → widget is the culprit. Reintroduce layer widget piece by piece.
  - If Step 1 hangs vertical → the rotation call. Try driver-level rotation instead.

## File layout

```
boards/shields/corne_handwired/
  Kconfig.defconfig           — shield-side defaults (display, LVGL)
  Kconfig.shield              — shield declarations
  CMakeLists.txt              — gates custom screen sources on CHOICE_CUSTOM
  corne_handwired.conf        — runtime config (sleep, BLE, USB logging)
  corne_handwired.dtsi        — shared matrix-transform + kscan
  corne_handwired_left.overlay  — left half: kscan cols + i2c0 + SSD1306
  corne_handwired_right.overlay — right half: kscan cols only (col-offset=6)
  corne_handwired.keymap      — 7 layers, combos, mod-taps
  custom_status_screen.c      — implements zmk_display_status_screen()
  widgets/
    layer_status.{c,h}        — layer-number widget (central-only)
```

## Working with the user

- User's host: Linux. Running `west` builds via GitHub Actions, not locally.
- User flashes UF2 over USB (no debugger).
- Without USB CDC the user has no serial output, so `CONFIG_ZMK_USB_LOGGING=y` is
  on but only useful when the firmware actually finishes booting. If USB shows
  `error -110` in `dmesg`, the central is hanging before USB CDC comes up.
- User's English isn't native; messages are short and sometimes typo'd. Confirm
  intent rather than guessing.
