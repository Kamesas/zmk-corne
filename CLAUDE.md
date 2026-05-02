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

- Branch: `corne-dactyl`, status: **shipped horizontal display**.
- Built-in ZMK status screen on the left-half OLED. Shows battery / output /
  connection icons and reflects layer state. Keyboard fully functional; right
  thumb (DEL) cold-joint fixed by user.
- Custom vertical status screen attempt was abandoned after extended bisection.
  See "Why the custom vertical screen was dropped" below.

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

## Why the custom vertical screen was dropped

We attempted a vertical 32×128 custom status screen. After a multi-step
bisection (boot hang → memory pool fix → label invisible → font fallback to
missing-glyph rectangles), it became clear that getting a custom screen with
labels working under `LV_CONF_MINIMAL` requires reproducing nice_view's full
widget pattern: `lv_canvas` instead of regular widgets, plus a custom
`rotate_canvas()` helper. That's a real port, not an incremental fix.

The built-in horizontal status screen already covers the practical part of
Phase 1 (icons + layer indication). Vertical orientation was a nice-to-have,
not a blocker. We chose to ship working horizontal and revisit vertical only
if requested.

### If you ever pick this up again

- Reference: `zmk/app/boards/shields/nice_view/widgets/status.c` and its
  `Kconfig.defconfig`.
- Don't try `lv_disp_set_rotation()` — it will hang.
- Don't bisect feature-by-feature. Port the whole nice_view pattern (canvases
  + `rotate_canvas` + the matching Kconfig: memory pool 8192, dedicated work
  queue, fonts) in one commit.
- Three canvases (top/middle/bottom) on a 32-wide × 128-tall logical canvas,
  each rotated 90° before being placed onto the 128×32 framebuffer.

### Prompt to use in a fresh session

Paste verbatim into a new Claude Code chat in this repo. The fresh session
will load this `CLAUDE.md` automatically.

```
Goal: Vertical 32×128 status screen on the left half's SSD1306 OLED,
showing AS logo (top), large layer digit (middle), peripheral connection
icon (bottom).

Constraints (read CLAUDE.md first — non-negotiable):
- Don't use lv_disp_set_rotation; it hangs under LV_CONF_MINIMAL.
- Use canvas-level rotation (lv_canvas + rotate helper) like nice_view.
- Set LV_Z_MEM_POOL_SIZE=8192, ZMK_DISPLAY_WORK_QUEUE_DEDICATED, fonts
  + LV_FONT_DEFAULT in Kconfig.defconfig from the start.
- LVGL 9 API only (LV_DISP_ROTATION_*, not LV_DISP_ROT_*).

Approach: port nice_view's pattern wholesale —
zmk/app/boards/shields/nice_view/widgets/status.c — adapting canvas
sizes for 128×32 instead of 160×68. ONE commit. No bisection.

If you can't get this working in one or two commits, stop and say so —
don't keep iterating with one-line guesses.
```

## Display feature plan (deferred)

**Phase 1 (shipped horizontal):**
- ✅ Battery / output / connection icons (built-in)
- ✅ Layer indication (built-in)

**Phase 2 (deferred until a vertical port is done):**
- Caps-lock indicator
- Modifier display (Shift/Ctrl/Alt/GUI)
- WPM counter
- Custom layout: AS logo, large layer digit, peripheral status

## File layout

```
boards/shields/corne_handwired/
  Kconfig.defconfig             — shield defaults (display + LVGL)
  Kconfig.shield                — shield declarations
  CMakeLists.txt                — empty placeholder; re-add custom screen
                                  sources here gated on
                                  CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM if
                                  the vertical port is revived.
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
