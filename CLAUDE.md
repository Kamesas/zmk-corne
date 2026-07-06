# zmk-corne — Claude project context

ZMK firmware for a handwired split keyboard built in a Dactyl-Manuform-style case
with **nice_nano v2** controllers on both halves. Branch in active development:
`corne-dactyl` (do not work on `main`).

## Hardware

- **MCUs:** nice_nano v2 (nRF52840) on both halves
- **Matrix:** 4 rows × 6 cols per half, `col2row`
  - Rows: **P1.07**, P0.29, P0.02, P1.15
  - Cols: P0.17, P0.20, P0.22, P0.24, P1.00, P0.11
  - **P0.31 is reserved for VBAT sense** — see Battery below. Row 0 was
    originally on P0.31 and moved to P1.07 to free the ADC.
- **OLED display:** 0.91" SSD1306, 128×32, I²C @ `0x3C`. 4-pin module with
  ~6.5 kΩ onboard pull-ups. Module's "SCK" label = SCL.
- **Display location:** **left half (central).** Was originally on the right;
  moved because the peripheral build can't link layer state.
- **Display wiring (left half):**
  - VCC → 3.3V
  - GND → GND
  - SDA → **P1.13**
  - SCK (SCL) → **P1.11**
- **Battery:** LiPo on the right half wired to nice_nano v2 `B+`/`B-` pads.
  The `nice_nano//zmk` board variant (rev 2.0.0) measures the cell through
  **VDDH** (`zmk,battery-nrf-vddh` in the upstream board overlay) — no ADC
  pin involved. The P0.31 voltage divider is the **v1** circuit; on v2,
  P0.13 is the ext-power control. So freeing P0.31 was not required for
  battery reporting — the row-0 move to P1.07 is harmless but P0.31 is
  usable again if a pin is ever needed. `CONFIG_ZMK_BATTERY_REPORTING=y`
  is enough. Left half has no battery yet.
- **Encoder:** removed for now. Will be relocated to the right half later.
  Original pin set was P1.07 (A) / P1.02 (B) / P1.01 (SW). P1.07 is now
  used by row 0, so a returning encoder needs a new "A" pin. Candidates:
  P1.04 or P1.06 (clean), P0.09 or P0.10 (requires
  `CONFIG_NFCT_PINS_AS_GPIOS=y`). B and SW can still use P1.02 / P1.01.

## Build / flash

GitHub Actions builds artifacts on push. UF2 outputs:
- `corne_handwired_left-nice_nano_v2-zmk.uf2`
- `corne_handwired_right-nice_nano_v2-zmk.uf2`
- `settings_reset-...-zmk.uf2`

After major config changes, flash `settings_reset` on both halves first, then
the new firmware on each. Power-cycle to re-pair.

## Current state (working)

- Branch: `corne-dactyl`, status: **vertical custom display + battery widget**.
- Custom 32×128 vertical status screen on the left-half OLED, ported from
  the nice_view canvas pattern. Three 32×32 canvases drawn upright then
  rotated 270° onto the 128×32 framebuffer:
  - **top:** endpoint icon + caps lock icon, both Montserrat 14, stacked.
    Caps icon only shown when active.
  - **middle:** large active-layer digit (Montserrat 28).
  - **bottom:** `L XX` / `R XX` battery percentages (Montserrat 14).
    `XX` becomes `--` when the cell isn't present / reads 0.
- Keyboard fully functional; right thumb (DEL) cold-joint fixed by user.

## Critical build pitfalls (kept here so we don't relearn them)

1. **Peripheral build omits keymap and layer events.**
   `zmk/app/CMakeLists.txt:48` gates `keymap.c` and `events/layer_state_changed.c`
   behind `(NOT CONFIG_ZMK_SPLIT) OR CONFIG_ZMK_SPLIT_ROLE_CENTRAL`. The peripheral
   image *cannot* link `zmk_keymap_highest_layer_active()` or
   `zmk_event_zmk_layer_state_changed`. **Any layer-aware widget must run on
   central.** This is why the display moved from right to left.

2. **Shield-folder `.conf` with a base name is NEVER merged.**
   A `.conf` inside `boards/shields/corne_handwired/` is only merged when named
   with the FULL shield name (`corne_handwired_left.conf` / `_right.conf`).
   The base-name sharing trick (`corne_handwired.conf` applies to both halves)
   works only in `config/`. So: runtime settings for both halves go in
   `config/corne_handwired.conf`; shield defaults go in `Kconfig.defconfig`.

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
  `WORK_QUEUE_DEDICATED`, `LV_USE_LABEL`, `LV_USE_CANVAS`, Montserrat 14/28,
  and the central-only split battery symbols (`_FETCHING` / `_PROXY`).

### Pitfalls already hit

- Forgetting `zephyr_library_include_directories(${CMAKE_SOURCE_DIR}/include)`
  causes `<zmk/display.h>: No such file` at compile time.
- `lv_disp_set_rotation()` still hangs under `LV_CONF_MINIMAL`. Don't use it.
- `LV_Z_MEM_POOL_SIZE` must be 8192 once `lv_canvas_create` is in play —
  4096 silently hangs the central.
- Adding a Montserrat size requires both `LV_FONT_MONTSERRAT_NN=y` and the
  `LV_FONT_DEFAULT` choice already pointing at an enabled font.
- Peripheral battery event (`zmk_peripheral_battery_state_changed`) and its
  `as_*` helper only exist when one of the
  `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_{FETCHING,PROXY}` configs is
  enabled. Always guard the usage with `IS_ENABLED(...)` so the central
  build doesn't fail to link when peripheral fetching is off.

## Display feature plan

**Phase 1 + 2 (shipped):**
- ✅ Endpoint / connection icon (USB / BLE / unbonded / disconnected)
- ✅ Layer indication (large digit)
- ✅ Caps-lock indicator (now stacked with endpoint icon at top)
- ✅ Battery percentage L / R (`zmk_battery_state_changed` + peripheral
  fetching via `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING`)

**Phase 3 (next, additive on the same canvas pattern):**
- Modifier display (Shift/Ctrl/Alt/GUI)
- WPM counter
- Peripheral-half connection icon (right-half link health, distinct from BLE
  endpoint icon — uses `zmk_split_bt_central_status_changed` on central)
- Glyph-based battery icons (currently text only)

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
  corne_handwired.dtsi          — shared matrix-transform + kscan
  corne_handwired_left.overlay  — left half: kscan cols + i2c0 + SSD1306
  corne_handwired_right.overlay — right half: kscan cols only (col-offset=6)
  corne_handwired.keymap        — 7 layers, combos, mod-taps
config/
  corne_handwired.conf          — runtime config for both halves (sleep, BLE,
                                  USB logging, battery reporting)
  west.yml                      — west manifest (zmk @ main)
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
