# ESP32 Pet App Integration Progress

Last updated: 2026-09-30

## Current checkpoint

Work is paused at Stage 2 (read-only BLE status). Do not start remote care commands until Stage 2 has passed the team's hardware acceptance.

## Stage 0 — inventory

- Firmware is the existing PlatformIO/Arduino project for NodeMCU-32S. The original joystick, OLED, care simulation, sound, Preferences/NVS, and A/B memorial storage remain in place.
- Flutter app source was already present at `esp32_pet_app/` in the main repository and matched the Downloads copy. It is kept separate from PlatformIO `src/`, `include/`, and `lib/`.
- Firmware previously had no BLE module. The Flutter prototype used a different, unimplemented command/status shape.
- Chrome Web is the first selected build target because it is available in the current environment. Team BLE hardware has not been confirmed.

## Stage 1 — shared protocol

- Added `esp32_pet_app/PROTOCOL.md`, shared by firmware and Flutter.
- Preserved the existing BLE service/characteristic UUIDs.
- Defined device info, status query/notification, command IDs/results/errors, required pet fields, and 4-byte fragment framing.
- Both implementations use 16 JSON bytes per fragment, which fits the default ATT MTU 23. Complete JSON is limited to 1024 bytes.
- Added Flutter generated-file ignores while keeping `pubspec.lock` trackable.

## Stage 2 — read-only BLE implementation

- Added `src/ble/BleLink.h` and `src/ble/BleLink.cpp`. BLE callbacks enqueue received frames; the application loop processes commands and publishes state. No BLE path writes NVS or changes gameplay.
- Connected BLE service startup and per-loop updates in `src/main.cpp`.
- Pinned ArduinoJson 6.21.5 in `platformio.ini`.
- Replaced the old Flutter A/B/C and reset prototype with real connection status, device/status requests, command-result correlation, fragment reassembly, full snapshot validation, and stale-data labeling after disconnect.
- Fixed the no-data care bars to remain empty and still; they animate/fill only after valid ESP32 readings arrive.
- Added the Flutter Web platform skeleton and resolved `pubspec.lock` for FlutterBluePlus 2.3.13.
- `esp32_pet_app/README.md` and this file describe the current checkpoint.

## Checks completed

- PlatformIO `pio run -e esp32dev`: passed with the pinned ESP32 platform/framework. RAM: 53,748 bytes (16.4%); Flash: 1,216,921 of 1,310,720 bytes (92.8%), leaving about 93.8 KB in the configured application partition. Most of the increase from the pre-BLE 27.0% build comes from the full ESP32 BLE stack. Future feature work must track this limit; likely remedies are migrating to NimBLE or deliberately changing the partition layout and its OTA trade-off.
- Flutter `flutter analyze`: passed with no issues.
- Flutter `flutter build web --release`: passed.
- Build outputs and PlatformIO tool packages were placed in the temporary `D:\ESP32Pet_stage2_build` directory to avoid low free space on C:. The temporary directory is not part of the project.
- No firmware was flashed, no NVS was cleared, and no hardware was reset.

## Still required before Stage 3

Team hardware acceptance is pending. Flashing and device operation must be performed deliberately by a team member:

1. Build the firmware with `pio run -e esp32dev` (build only).
2. Flash the normal firmware using the team's usual procedure; do not erase flash.
3. Start the app with `cd esp32_pet_app` and `flutter run -d chrome` on a machine/browser with BLE support.
4. Confirm first connection, real values, joystick-driven state changes, disconnect, and reconnect/latest snapshot.
5. Record the results and any failures. Do not enable remote care commands before these checks pass.

The following are not yet hardware-verified: BLE discovery/connection, notifications and framing under real radio conditions, joystick state updates, disconnect/reconnect, and offline gameplay while the app is disconnected.

## Build versions observed

- Flutter 3.29.3 / Dart 3.7.2
- FlutterBluePlus 2.3.13
- PlatformIO Core 6.1.19
- PlatformIO Espressif32 7.1.3; Arduino-ESP32 package `4.20017.260907+sha.dcc1105b`
- U8g2 2.36.18; ArduinoJson 6.21.5
