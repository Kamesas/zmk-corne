# Battery indicator on display — plan

## Goal

Show right-half battery charge level on the left-half OLED. Architecture should
also support a left-half battery later (you plan to add one).

## Hardware constraint

On nice_nano v2, **P0.31 is the VBAT sense input** (via the onboard voltage
divider gated by P0.13). The matrix in `corne_handwired.dtsi` currently uses
P0.31 as **row 0**, so the onboard battery measurement is blocked on both
halves. Row 0 must move to a different pin.

## Pin choice for row 0

**Decision: P1.07.** Physically reachable from where row 0 lands on the
nice_nano in this build. Was the old encoder A pin, so it's already broken
out on the bottom pad row and easy to solder.

Implication: if the encoder is re-added later, it can't reuse the original
P1.07 / P1.02 / P1.01 trio. P1.02 and P1.01 are still free, so the encoder
needs one new pin for "A". Candidates when that day comes: **P1.04** or
**P1.06** (no extra config), or **P0.09 / P0.10** (require
`CONFIG_NFCT_PINS_AS_GPIOS=y`).

## What you do (hardware)

1. **Both halves:** desolder the row-0 wire from MCU pin P0.31, resolder it
   to **P1.07**. Without this, row 0 stops scanning on whichever half isn't
   rewired.
2. **Right half:** confirm the LiPo is wired to the nice_nano's `B+` / `B-`
   pads (you've already done this — just sanity-check polarity).
3. **Left half (later, optional):** add a LiPo on the left half's `B+` /
   `B-` pads when you're ready; no extra wiring needed because P0.31 will
   already be free on that side too.

## What I do (firmware)

All changes live under `boards/shields/corne_handwired/`.

1. **`corne_handwired.dtsi`** — change row 0 from `&gpio0 31` to
   `&gpio1 7`. No other matrix changes; `col2row` and the matrix-transform
   stay the same.
2. **`corne_handwired.conf`** — add `CONFIG_ZMK_BATTERY_REPORTING=y`. The
   `vbatt` voltage-divider node is already defined in the upstream
   `nice_nano_v2.dts`, so once P0.31 is free and reporting is on, the
   peripheral starts publishing its level over BLE split and the central
   reports its own to the host.
3. **`Kconfig.defconfig`** — ensure battery-related symbols and any LVGL
   symbols I need for the glyph are on. (Battery glyph likely drawn with
   primitives, but I may add a small icon font.)
4. **`widgets/status.{c,h}`** — subscribe to:
   - `zmk_battery_state_changed` (central / left cell)
   - `zmk_peripheral_battery_state_changed` (right cell)

   Keep all existing indicators (caps lock, layer digit, endpoint). New
   3-canvas layout:

   | Canvas | Current        | Proposed                                              |
   | ------ | -------------- | ----------------------------------------------------- |
   | Top    | Caps lock 22px | **Caps + endpoint icons, both ~14px, stacked**        |
   | Middle | Layer digit    | Layer digit (unchanged)                               |
   | Bottom | Endpoint 22px  | **Battery for L and R cells**                         |

   Battery rendering: two short lines, **Montserrat 14**, format `L85` and
   `R92` (no `%` symbol so we stay inside the 32px-wide canvas with margin
   to spare). Requires enabling `LV_FONT_MONTSERRAT_14` in
   `Kconfig.defconfig`.

   The widget only runs on central — peripheral can't link these events
   (`CLAUDE.md` "Critical build pitfalls" #1).
5. **Graceful "no battery on left" state** — until you add the left cell,
   the central's reported % defaults to whatever the nrf VBAT divider reads.
   On nice_nano v2 with no LiPo connected, that's ~0%. To avoid a misleading
   "L 0%" reading, render the L slot as the USB plug glyph whenever USB is
   the active transport (powered via cable, no battery to show). When BLE
   *and* no left battery exists, leave it blank rather than 0%.

## Build & flash sequence (you)

Per `CLAUDE.md`:

1. Push the firmware commit; let GitHub Actions build.
2. Flash `settings_reset` UF2 on both halves.
3. Flash the new `corne_handwired_left-...` UF2 on the left half.
4. Flash the new `corne_handwired_right-...` UF2 on the right half.
5. Power-cycle both halves; let them re-pair.
6. Verify: all keys (especially row 0) work on both halves; display shows
   right-cell %; central reports its own % to host (visible in OS Bluetooth
   battery indicator).

## Risks / open questions

- **Visual layout at 32×32.** I haven't drawn a battery glyph in this canvas
  setup before. Expect at least one iteration on size / placement after the
  first flash.
- **First-flash row-0 dead.** If you flash firmware *before* rewiring, the
  whole top row stops working. Rewire first, then flash — or be prepared to
  use the bootloader reset to recover.
- **Peripheral battery reporting reliability.** Levels update on a relatively
  slow cadence (default ~60s) and only when the split link is up. Don't
  expect real-time movement.
- **Encoder pin set changes.** P1.07 is no longer available for the
  encoder. When you add it back, A goes on a different pin (P1.04 / P1.06 /
  P0.09 / P0.10); B and SW can still use P1.02 and P1.01.

## Final layout (confirmed)

- Row 0 pin: **P1.07** ✓
- Top canvas: caps lock + endpoint, both ~14px, stacked ✓
- Middle canvas: layer digit (unchanged) ✓
- Bottom canvas: `L85` / `R92` in Montserrat 14, no `%` ✓

---

## Tomorrow — handoff for a fresh chat

Firmware changes are committed in working tree (not yet pushed). Below is
everything needed to pick up cold.

### Files touched

| File                                                              | Purpose                                                  |
| ----------------------------------------------------------------- | -------------------------------------------------------- |
| `boards/shields/corne_handwired/corne_handwired.dtsi`             | Row 0 changed from `&gpio0 31` → `&gpio1 7`              |
| `boards/shields/corne_handwired/corne_handwired.conf`             | Added `ZMK_BATTERY_REPORTING`, peripheral fetching/proxy |
| `boards/shields/corne_handwired/Kconfig.defconfig`                | Swapped Montserrat 22 → 14, updated `LV_FONT_DEFAULT`    |
| `boards/shields/corne_handwired/widgets/util.h`                   | Added `battery_central` / `battery_peripheral` to state  |
| `boards/shields/corne_handwired/widgets/status.c`                 | New layout + battery listener (guarded on kconfigs)      |
| `CLAUDE.md`                                                       | Pin map / battery / encoder notes refreshed              |

### Hardware checklist (you, before flashing)

- [ ] **Left half:** desolder row-0 wire from P0.31, resolder to **P1.07**.
- [ ] **Right half:** desolder row-0 wire from P0.31, resolder to **P1.07**.
- [ ] Right-half LiPo on `B+`/`B-` pads with correct polarity (you said
      already done — quick sanity check before powering on).
- [ ] Visual: no shorts between P0.31 and any neighboring trace; P0.31 must
      now be untouched (the onboard divider is between it and the battery,
      no external wire there).

### Flash order

1. `git push` on `corne-dactyl`; wait for GitHub Actions to build.
2. Download artifacts: `settings_reset-...`, `corne_handwired_left-...`,
   `corne_handwired_right-...`.
3. Flash **settings_reset** on **both halves** first (double-tap reset to
   enter bootloader, drop UF2 on the USB drive). This clears stale BLE
   pairings — important because the host's BAS view of the keyboard
   changes (it'll start advertising peripheral battery as well).
4. Flash `corne_handwired_left-...` on the left.
5. Flash `corne_handwired_right-...` on the right.
6. Power-cycle both halves; let them re-pair (LED activity will settle).
7. Re-pair to the host: forget the existing keyboard entry on the OS,
   then pair fresh.

### What "working" looks like

- Top row keys fire on both halves (the rewire test).
- OLED shows: endpoint icon at top, layer digit in middle, `L--` / `R85`
  (or whatever) at bottom.
- Press Caps Lock on a connected host → caps icon appears below the
  endpoint icon on the top canvas.
- After a minute or so, the host's Bluetooth panel shows two battery
  levels for the keyboard (central + peripheral). That's the
  `LEVEL_PROXY` config working.
- The `R` value updates within ~60s of the right half draining or
  charging noticeably. Slow cadence is expected.

### If something is wrong

- **Top row dead on one half:** that half wasn't rewired or the new
  P1.07 joint isn't making contact. Re-check before assuming firmware.
- **`R--` stays forever:** central isn't fetching peripheral battery.
  Verify `CONFIG_ZMK_SPLIT_BLE_CENTRAL_BATTERY_LEVEL_FETCHING=y` is on
  the right half too (it's in the shared `.conf`, so it should be).
  Also confirm the split link is up — layer changes triggered by
  right-half keys should still propagate to the display.
- **`L` shows weird number with no left battery:** expected. Until you
  add a left LiPo, the central ADC reads whatever the divider floats at
  with no source — could be near-0 (shows `L--`) or some bogus value.
  Ignore until left cell is installed.
- **Central hangs at boot / OLED stays blank:** that's the
  `LV_Z_MEM_POOL_SIZE` / `lv_disp_set_rotation` failure mode from
  `CLAUDE.md`. Re-check we haven't touched those. We didn't, but worth
  knowing the signature.

### Known follow-ups that are *not* in this change

- No glyph-based battery icon — text only. Adding a battery shape would
  need either an icon font enabled under `LV_CONF_MINIMAL`, or a
  primitive-drawn outline + fill bar.
- No peripheral split-link health icon (separate from the endpoint icon).
- Encoder is not re-added. When you do, see `CLAUDE.md` Hardware /
  Encoder for the new pin set.
