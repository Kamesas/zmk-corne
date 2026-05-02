# zmk-corne — Claude project context

ZMK firmware for a handwired split keyboard built in a Dactyl-Manuform-style case
with **nice_nano v2** controllers on both halves. Branch in active development:
`corne-dactyl` (do not work on `main`).

## Hardware

- **MCUs:** nice_nano v2 (nRF52840) on both halves
- **Matrix:** 4 rows × 6 cols per half, `col2row`
  - Rows: P0.31, P0.29, P0.02, P1.15
  - Cols: P0.17, P0.20, P0.22, P0.24, P1.00, P0.11
- **OLED display:** 0.91" SSD1306, 128×32, I²C @ `0x3C`. 4-pin module with
  ~6.5 kΩ onboard pull-ups. Module's "SCK" label = SCL.
- **Display location:** **left half (central).** Was originally on the right;
  moved because the peripheral build can't link layer state.
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

After major config changes, flash `settings_reset` on both halves first, then
the new firmware on each. Power-cycle to re-pair.

## Current state (working)

- Branch: `corne-dactyl`, status: **shipped vertical custom display**.
- Custom 32×128 vertical status screen on the left-half OLED, ported from
  the nice_view canvas pattern. Three 32×32 canvases drawn upright then
  rotated 270° onto the 128×32 framebuffer:
  - left: "AS" monogram (Montserrat 22)
  - center: large active-layer digit (Montserrat 28)
  - right: endpoint/connection icon — USB / WIFI / CLOSE / SETTINGS
- Keyboard fully functional; right thumb (DEL) cold-joint fixed by user.

## Critical build pitfalls (kept here so we don't relearn them)

1. **Peripheral build omits keymap and layer events.**
   `zmk/app/CMakeLists.txt:48` gates `keymap.c` and `events/layer_state_changed.c`
   behind `(NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL`. The peripheral
   image *cannot* link `zmk_keymap_highest_layer_active()` or
   `zmk_event_zmk_layer_state_changed`. **Any layer-aware widget must run on
   central.** This is why the display moved from right to left.

2. **Shield `.conf` does not merge reliably.**
   Defaults belong in `boards/shields/corne_handwired/Kconfig.defconfig`, not in
   `corne_handwired.conf`. The build's merge log only consistently picks up
   `config/<shield>.conf` and `prj.conf`.

3. **`ZMK_DISPLAY` implies `LV_CONF_MINIMAL`.** Strips out most LVGL features.
   For the **built-in** status screen this is fine. For a **custom** screen
   you must, at minimum:
   - Re-enable label + a Montserrat font + the `LV_FONT_DEFAULT` choice
   - Bump `LV_Z_MEM_POOL_SIZE` to 8192 (default 4096 silently hangs the central
     when `lv_label_create` allocates internally)
   - Set `ZMK_DISPLAY_WORK_QUEUE_DEDICATED`
   - Avoid `lv_disp_set_rotation()` — it hangs the central under
     `LV_CONF_MINIMAL`. Use canvas-level rotation like nice_view if you need
     vertical orientation.

## Custom vertical screen — how it's wired

Reference for any future widget work on this shield. Pattern lifted from
`zmk/app/boards/shields/nice_view/widgets/`.

- `widgets/util.{c,h}` — `rotate_canvas()` (sw_rotate 270° on a square L8
  buffer) and small wrappers around `lv_draw_*` that drive a canvas via a
  layer. `CANVAS_SIZE = 32`.
- `widgets/status.{c,h}` — three-canvas widget. Each section draws upright
  into its 32×32 canvas, then `rotate_canvas()` rolls the buffer 270°
  before LVGL composites onto the framebuffer.
- `custom_status_screen.c` — strong override of `zmk_display_status_screen()`,
  enabled by `CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y`.
- `CMakeLists.txt` — gates sources on `CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM`,
  guards `widgets/status.c` on central only (peripheral can't link layer
  events), and adds `${CMAKE_SOURCE_DIR}/include` so shield sources can find
  `<zmk/...>` headers (they compile under the zephyr target, not app).
- `Kconfig.defconfig` — `STATUS_SCREEN_CUSTOM`, mem pool 8192,
  `WORK_QUEUE_DEDICATED`, `LV_USE_LABEL`, `LV_USE_CANVAS`, Montserrat 14/22/28.

### Pitfalls already hit

- Forgetting `zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)`
  causes `<zmk/display.h>: No such file` at compile time.
- `lv_disp_set_rotation()` still hangs under `LV_CONF_MINIMAL`. Don't use it.
- `LV_Z_MEM_POOL_SIZE` must be 8192 once `lv_canvas_create` is in play —
  4096 silently hangs the central.
- Adding a Montserrat size requires both `LV_FONT_MONTSERRAT_NN=y` and the
  `LV_FONT_DEFAULT` choice already pointing at an enabled font.

## Display feature plan

**Phase 1 (shipped):**
- ✅ Endpoint / connection icon (USB / WIFI / CLOSE / SETTINGS)
- ✅ Layer indication (large digit)
- ✅ Custom vertical layout with AS monogram

**Phase 2 (next, additive on the same canvas pattern):**
- Caps-lock indicator
- Modifier display (Shift/Ctrl/Alt/GUI)
- WPM counter
- Battery icons (when battery is added)
- Peripheral-half connection icon (right-half link health, distinct from BLE
  endpoint icon — uses `zmk_split_bt_central_status_changed` on central)

## File layout

```
boards/shields/corne_handwired/
  Kconfig.defconfig             — shield defaults (display + LVGL)
  Kconfig.shield                — shield declarations
  CMakeLists.txt                — wires custom_status_screen + widgets,
                                  gated on CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM
  custom_status_screen.c        — strong override of zmk_display_status_screen
  widgets/status.{c,h}          — three-canvas vertical status widget (central)
  widgets/util.{c,h}            — rotate_canvas + lv_canvas helpers
  corne_handwired.conf          — runtime config (sleep, BLE, USB logging)
  corne_handwired.dtsi          — shared matrix-transform + kscan
  corne_handwired_left.overlay  — left half: kscan cols + i2c0 + SSD1306
  corne_handwired_right.overlay — right half: kscan cols only (col-offset=6)
  corne_handwired.keymap        — 7 layers, combos, mod-taps
```

## Working with the user

- User's host: Linux. Builds run via GitHub Actions; no local west builds.
- User flashes UF2 over USB; no debugger / no serial unless USB CDC enumerates.
- `CONFIG_ZMK_USB_LOGGING=y` is on but only useful once boot completes. If
  `dmesg` shows `error -110` on a USB connect, the central is hanging before
  USB CDC comes up.
- User's English isn't native; messages are short and sometimes typo'd —
  confirm intent rather than guessing.
- The user has limited patience for many small "step N" commits. Prefer one
  large, well-researched commit over a long bisection.
