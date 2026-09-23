# Architecture Notes

Phase 1A defines hardware boundaries only. `HardwareConfig.h` is the sole
location for pin assignments and hardware tuning. The `hardware` modules own
the OLED I2C setup, joystick event conversion, and passive-buzzer timing.
`storage/Save` is a placeholder for the future Preferences/NVS boundary; it
does not define game data or a persistence format.

The loop remains non-blocking: `Input::update()` returns one abstract input
event, and `Sound::update()` advances any active tone sequence. Future game/UI
code should consume these interfaces rather than read GPIO, use LEDC, or call
an OLED library directly.

The Display module uses U8g2's SH1106 full-buffer driver. It owns I2C scanning,
initialization, buffered text, frames, lines, and screen updates. The OLED
address is retained centrally in `HardwareConfig::Oled` once confirmed on the
assembled hardware; display-library types never leave `Display.cpp`.

`ui/UiController` owns UI screen state, menu selection, transitions, and dirty
rendering. `main` forwards `InputEvent` values to it and services `Sound`; the
UI neither reads hardware pins nor depends on U8g2. It draws only through the
Display abstraction and redraws static screens only after a state change.

The application object in `main.cpp` owns one private `PetData` instance and
passes a const reference to `UiController`. Home reads derived need states,
sickness and level from that same pet on entry; it does not cache or copy pet
data. Live data-change invalidation is deferred until gameplay changes values.
`ui/PetIcons` contains bounded, monochrome primitive drawings, including the
replaceable 44 x 32 slime placeholder. Mood and stomach fullness stay on the
left; Dirty / Filthy show a broom in the second right alert slot. Starving
and Filthy add the same small critical exclamation mark. Clean and
SlightlyDirty leave the cleaning slot empty. Home layout stays in `UiController`;
`Display::drawSmallText` encapsulates the 5 x 7 font and restores the normal
font afterwards. The bottom line is decorative, not EXP progress. The status
card is visual only; Home Press still opens the unchanged main menu.
