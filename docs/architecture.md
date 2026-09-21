# Architecture Notes

The current firmware intentionally contains only the boot sequence. Future code should be split by responsibility as each feature becomes defined:

- Pet System: state, actions, and growth rules
- Display System: OLED rendering
- Input System: buttons and joystick
- Sound System: buzzer output
- Save System: persistent state storage
- Game System: mini-game rules

Environmental sensors belong to Phase 1B and should not be added until their requirements are confirmed.

Prefer a non-blocking update loop. Each subsystem should expose a small update or service interface and receive time from the main loop rather than owning long delays.
