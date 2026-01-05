# Corne Handwired Keyboard - ZMK Firmware

This repository contains the ZMK firmware configuration for a handwired Corne keyboard with Nice!Nano v2 controllers.

## Flashing Firmware (Bootloader)

### How to Enter Bootloader Mode

Each half of the keyboard must be flashed separately.

**Left Half:**
1. Hold the **left inner thumb key** (closest to the center, the ADJUST layer key)
2. While holding, press **Q**
3. The left half will enter bootloader mode and appear as a USB drive

**Right Half:**
1. Hold the **right inner thumb key** (closest to the center, the ADJUST/GUI key)
2. While holding, press **P**
3. The right half will enter bootloader mode and appear as a USB drive

### Flashing the Firmware

1. Build the firmware (GitHub Actions will build it when you push to the repository)
2. Download the `.uf2` files from the GitHub Actions artifacts
3. Enter bootloader mode on one half (see above)
4. The keyboard half will appear as a USB mass storage device (like a USB drive)
5. Drag and drop the corresponding `.uf2` file onto the drive
6. The device will automatically reboot with the new firmware
7. Repeat for the other half
8. **Important:** After flashing new firmware, clear the Bluetooth profiles to prevent cached behavior:
   - Hold left inner thumb (ADJUST layer) + press A
   - This clears any cached keyboard state that might conflict with the new firmware
   - Recommended even if you only use USB cable

**Files:**
- `corne_handwired_left-nice_nano_v2-zmk.uf2` - for the left half
- `corne_handwired_right-nice_nano_v2-zmk.uf2` - for the right half

## Bluetooth Pairing and Management

### Battery Requirement

**Important:** Bluetooth functionality requires a battery to be installed in each half of the keyboard.

- **Without battery:** Keyboard only works via USB cable (wired mode)
- **With battery:** Keyboard can use Bluetooth (wireless) OR USB cable
- The battery charges automatically when the USB cable is plugged in

**Battery specs:**
- Type: LiPo battery (3.7V)
- Common sizes: 301230 (110mAh), 401030 (150mAh), 801230 (300mAh)
- Connector: JST connector (compatible with Nice!Nano v2)

### Accessing Bluetooth Controls

All Bluetooth controls are on the **ADJUST layer**. To access this layer:
- Hold the **left inner thumb key** (momentary), OR
- Hold the **right inner thumb key** (momentary)

### Pairing to Devices

The keyboard can remember up to 5 Bluetooth devices.

**To pair to a new device:**

1. Hold either inner thumb key to access ADJUST layer
2. While holding, press one of the following keys to select a profile:
   - **W** = Profile 1
   - **E** = Profile 2
   - **R** = Profile 3
   - **T** = Profile 4
   - **Y** = Profile 5
3. Release the keys
4. The keyboard will enter pairing mode for that profile
5. On your computer/device, search for Bluetooth devices
6. Select "Corne Handwired" from the list
7. The keyboard is now paired to that profile

### Switching Between Paired Devices

**To switch to a different device:**

1. Hold either inner thumb key (ADJUST layer)
2. Press the corresponding profile key (W/E/R/T/Y)
3. The keyboard will switch to that device

### Switching Between Bluetooth and USB (Cable)

**USB connection always has priority:**
- When you plug in the USB cable, the keyboard will automatically use the USB connection
- Bluetooth will be ignored while the cable is connected
- When you unplug the USB cable, the keyboard will automatically reconnect to the last active Bluetooth profile

**To use your laptop via cable after using Bluetooth with phone:**
1. Simply plug in the USB cable to your laptop
2. The keyboard will immediately switch to USB mode
3. No need to change Bluetooth profiles

**To go back to Bluetooth (phone):**
1. Unplug the USB cable
2. The keyboard will automatically reconnect to the last Bluetooth device (your phone)

### Clearing a Bluetooth Profile

**To unpair/clear a profile:**

1. Hold either inner thumb key (ADJUST layer)
2. While holding, press **A** (this is Bluetooth Clear)
3. The current profile will be cleared and the keyboard will forget that device

**Note:** This only clears the currently selected profile. To clear all profiles, you need to select each one and clear it individually.

## Keyboard Layout

The keyboard uses a Ferris Sweep-inspired layout with home row mods and multiple layers.

### Thumb Keys (Left to Right)

**Left Hand:**
- Outer: Backspace (tap) / Layer 1 (hold)
- Middle: Delete (tap) / Layer 2 (hold)
- Inner: ADJUST layer (hold only)

**Right Hand:**
- Outer: Space (tap) / Layer 1 (hold)
- Middle: Enter (tap) / Layer 2 (hold)
- Inner: Win/Super key (tap) / ADJUST layer (hold)

### Home Row Mods

**Bottom Row:**
- Z: Shift (hold) / z (tap)
- X: Ctrl (hold) / x (tap)
- C: Alt (hold) / c (tap)
- V: Super/GUI (hold) / v (tap)
- M: Super/GUI (hold) / m (tap)
- , (comma): Alt (hold) / , (tap)
- . (period): Ctrl (hold) / . (tap)
- / (slash): Shift (hold) / / (tap)

### Combos

Press two keys simultaneously to trigger:
- **E + R** = [
- **U + I** = ]
- **C + V** = {
- **M + ,** = }
- **J + K** = ESC
- **K + L** = Backspace
- **D + F** = Caps Lock
- **S + D** = Tab
- **L + ;** = "
- **J + L** = Enter
- **Space + Enter** (middle thumbs) = Layer 5

## Layers

- **Layer 0**: Base layer (QWERTY)
- **Layer 1**: Symbols and navigation
- **Layer 2**: Numbers and window management (Super+1-5, Alt+1-5)
- **Layer 3**: Media controls (brightness, volume, playback)
- **Layer 4**: Navigation (Home, PgDn, PgUp, End, arrow keys, clipboard)
- **Layer 5**: Function keys (F1-F12)
- **ADJUST**: Bluetooth and bootloader controls

## Building Firmware

This repository is set up to build automatically using GitHub Actions.

1. Make changes to the keymap
2. Commit and push to the repository
3. GitHub Actions will automatically build the firmware
4. Download the `.uf2` files from the Actions tab
5. Flash to your keyboard using the bootloader instructions above

## Troubleshooting

**Keyboard not appearing as USB drive:**
- Make sure you're holding the inner thumb key BEFORE pressing Q or P
- Try a different USB cable
- Try a different USB port

**Bluetooth not connecting:**
- Clear the profile (hold inner thumb + A)
- Remove "Corne Handwired" from your device's Bluetooth settings
- Re-pair from scratch

**Keys not working as expected (especially C or comma activating wrong layer):**
- **FIRST:** Clear the Bluetooth profile even if using USB: Hold left inner thumb + A
- This clears cached keyboard state that can override firmware behavior
- After clearing profile, unplug and replug USB cable (or power cycle if on Bluetooth)
- If still not working: Check which Bluetooth profile you're on (try switching profiles)
- Last resort: Reflash the firmware and immediately clear all Bluetooth profiles

**C and comma keys activating Layer 2 instead of Alt:**
- This is a cached state issue
- Hold left inner thumb (ADJUST) + press A to clear the Bluetooth profile
- The fix works immediately even when using USB cable
- Always clear Bluetooth profiles after flashing new firmware

**Split halves not communicating:**
- Make sure both halves are powered on
- The left half is always the central (connected to computer)
- Reset both halves by re-flashing firmware
