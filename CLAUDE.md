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
   That defaults `LV_USE_LABEL=n` and Montserrat fonts to `n`. Re-enable
   what the screen needs in `Kconfig.defconfig`:
   ```
   config LV_USE_LABEL
       default y
   config LV_FONT_MONTSERRAT_14
       default y
   choice LV_FONT_DEFAULT
       default LV_FONT_DEFAULT_MONTSERRAT_14
   endchoice
   ```

   **Also bump the LVGL memory pool** — default 4096 is too small for
   custom screens with labels and silently hangs the central:
   ```
   config LV_Z_MEM_POOL_SIZE
       default 8192 if ZMK_DISPLAY_STATUS_SCREEN_CUSTOM
   choice ZMK_DISPLAY_WORK_QUEUE
       default ZMK_DISPLAY_WORK_QUEUE_DEDICATED
   endchoice
   ```

4. **`lv_disp_set_rotation()` HANGS the central under `LV_CONF_MINIMAL`.**
   This is the actual root cause of the central-boot freeze, not the font
   default. Upstream nice_view confirms the pattern: it does **not** call
   `lv_disp_set_rotation` — it uses canvas-level rotation via a custom
   `rotate_canvas()` helper (see
   `zmk/app/boards/shields/nice_view/widgets/status.c`). In `LV_CONF_MINIMAL`
   the LVGL software-rotation path is not wired up, so `lv_disp_set_rotation`
   wedges the next render and USB CDC never enumerates (`dmesg: error -110`).

5. **ZMK custom widgets need ZMK's private headers.**
   `CMakeLists.txt` adds `${ZEPHYR_BASE}/../zmk/app/include` to the include path so
   widgets can `#include <zmk/display.h>` etc.

## Why the display lives on the central (left)

The original plan put the OLED on the right (peripheral) half. That hit the
peripheral-build pitfall above: the layer widget couldn't link. Rather than
write a custom split BLE service to forward layer state, we moved the display to
the left/central. Same pin numbers (P1.13/P1.11) — the user rewired physically.

## Display software stack

- **Driver:** `solomon,ssd1306fb` (Zephyr's SSD1306 framebuffer driver)
- **GUI:** LVGL 9. **Do not use `lv_disp_set_rotation()` under `LV_CONF_MINIMAL`** — it
  hangs the central. For rotation, use canvas-level rotation like nice_view
  (or skip rotation and use horizontal layout).
- **Status screen choice:** `ZMK_DISPLAY_STATUS_SCREEN_CUSTOM`, implementing
  `zmk_display_status_screen()` in `boards/shields/corne_handwired/custom_status_screen.c`
- **LVGL config (in `Kconfig.defconfig` under `if LVGL`):**
  - `LV_Z_VDB_SIZE = 64`
  - `LV_DPI_DEF = 148`
  - `LV_Z_BITS_PER_PIXEL = 1`
  - `LV_COLOR_DEPTH_1`
  - `LV_USE_LABEL = y`, `LV_FONT_MONTSERRAT_14 = y`,
    `LV_FONT_DEFAULT = LV_FONT_DEFAULT_MONTSERRAT_14`

## Reference implementations

- **nice_view** (`zmk/app/boards/shields/nice_view/`) — canonical ZMK
  custom-screen vertical OLED. Uses canvas-level rotation, not
  `lv_disp_set_rotation`. Read its `widgets/status.c` and `Kconfig.defconfig`
  before any further display work.
- **zmk-nice-oled** (mctechnology17) — alternative reference for vertical
  OLED widgets, modular layout.

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

## Debug history (custom-status-screen central-boot hang)

Bisection log — what we tried and what each step revealed:

| Step | Commit | Build content | Result |
|------|--------|---------------|--------|
| 0 | `660074b` | Built-in status screen | Boots, icons render |
| 1 | `ee78f77` | Custom screen: rotation + "AS" label, no widget | Hangs (tiny squares, USB -110) |
| 2 | `8da7082` | Custom screen: label only, no rotation | Hangs |
| 3 | `b84aad3` | Custom screen: bare `lv_obj_create`, no children | Boots, blank powered display |
| 4 | `1f48fd2` | Rotation + label + `LV_FONT_DEFAULT_MONTSERRAT_14` choice set | Hangs |
| 5 | `3f5432d` | Label + font default, no rotation | Hangs |
| 6 | `417053b` | Label only, no rotation, **`LV_Z_MEM_POOL_SIZE=8192`** + dedicated work queue | **Boots**, blank/blue display |

**Confirmed root cause:** default `LV_Z_MEM_POOL_SIZE` (4096) is too small.
`lv_label_create` allocates internally and the failure path under
`LV_CONF_MINIMAL` silently hangs the central before USB enumerates. Bumping
to 8192 (matching upstream nice_view) unblocks boot.

**New issue (step 6):** display is on but "AS" doesn't render — only the
panel's blue/cyan background shows. Likely cause: under `LV_COLOR_DEPTH_1`
the default text color is the same as background, or the label's font/text
color needs to be set explicitly. Possible fixes:
- `lv_obj_set_style_text_color(label, lv_color_white(), 0)`
- Or set the screen background to black so the white-on-default works
- Check what nice_view does with text color in its widgets.

## Current state

- Branch: `corne-dactyl`, last commit `417053b` (Step 6 — boots, blank display).
- Hardware: left half rewired with display (SDA→P1.13, SCL→P1.11). Right half
  has display removed.
- Right thumb on left half (DEL) had a cold solder joint — user confirmed and
  fixed.
- Boot hang is solved. New problem: label exists in the source but doesn't
  render — display shows only the panel's blue background. Need to set
  text color / background style for monochrome.

## Next steps (depending on Step 5 result)

If Step 5 boots horizontally (rotation confirmed as culprit), choose one:

**Path A — horizontal layout, simpler.** Drop rotation entirely. Lay
"AS" / layer number / peripheral icon left-to-right across 128×32. Less
ambitious than vertical, but proven to work and quick to implement.

**Path B — replicate nice_view's canvas rotation.** Use `lv_canvas`
instead of regular widgets, port a `rotate_canvas()` helper. Gives true
vertical 32×128 layout but more code. Reference:
`zmk/app/boards/shields/nice_view/widgets/status.c`.

After picking a path:
1. Reintroduce layer widget. (`widgets/layer_status.{c,h}` already on disk;
   add back to `CMakeLists.txt`.)
2. Add peripheral connection icon (Phase 1 wrap-up).
3. Phase 2 widgets: caps, modifiers, WPM, battery.

If Step 5 still hangs, the hypothesis is wrong and we need to look at
LVGL memory pool sizing (nice_view uses 8192 bytes for custom screens; we
haven't set anything explicitly).

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
