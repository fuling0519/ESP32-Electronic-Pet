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
