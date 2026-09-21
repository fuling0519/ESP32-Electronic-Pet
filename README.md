# ESP32 Electronic Pet

## Project Overview

ESP32 Electronic Pet 是一個以 ESP32 NodeMCU-32S 為核心的獨立式電子寵物專案，預計透過 OLED、按鈕、搖桿與蜂鳴器提供互動體驗。

目前版本只包含 PlatformIO 專案骨架與 ESP32 啟動流程，不包含電子寵物遊戲邏輯。

## Features

| Feature | Status |
| --- | --- |
| Egg / Hatching | Planned for Phase 1A |
| Pet System | Planned for Phase 1A |
| Feeding | Planned for Phase 1A |
| Rest | Planned for Phase 1A |
| Mini Games | Planned for Phase 1A; rules not defined yet |
| Growth | Planned for Phase 1A |
| Save / Load | Planned for Phase 1A |
| Environmental Sensors | Planned for Phase 1B |
| Mobile App | Planned for Phase 2 |

## Hardware

Current planned hardware:

- ESP32 NodeMCU-32S
- OLED display
- Buttons
- Joystick
- Buzzer

Environmental sensors and MPU6050 are reserved for Phase 1B and are not included in the current firmware.

## Development Environment

- Visual Studio Code
- PlatformIO
- Arduino Framework
- ESP32 platform
- C++

The current firmware has no third-party library dependencies.

## Project Structure

```text
ESP32-Electronic-Pet/
├── src/                    Firmware entry point
│   └── main.cpp
├── include/                Project headers and local-only settings
│   └── secrets.example.h
├── lib/                    Future project-local libraries
├── test/                   Future PlatformIO tests
├── docs/                   Architecture and project notes
├── .gitignore              Git exclusions for PlatformIO and local files
├── platformio.ini          PlatformIO environment configuration
├── README.md               Project documentation
└── LICENSE                 License selection placeholder
```

Subsystems should be introduced only when their requirements are confirmed:

- Pet System
- Display System
- Input System
- Sound System
- Save System
- Game System

Sensor System belongs to Phase 1B.

## Development Phases

```text
Phase 1A
Core Electronic Pet

Phase 1B
Environmental Interaction

Phase 2
Mobile App / Connectivity
```

## Getting Started

1. Install Visual Studio Code and the PlatformIO extension.
2. Open the `ESP32-Electronic-Pet` folder in VS Code.
3. Connect an ESP32 NodeMCU-32S board.
4. Run PlatformIO **Build** to compile the firmware.
5. Run PlatformIO **Upload** to flash the board.
6. Open **Serial Monitor** at `115200` baud.

The initial serial output should be:

```text
================================
ESP32 Electronic Pet
System Booting...
================================
```

## Git Workflow

Use `main` for stable versions and create a feature branch for each isolated change:

```text
main
 └── feature/short-description
```

Suggested workflow:

```bash
git switch -c feature/short-description
# make and test the change
git add .
git commit -m "Describe the change"
git switch main
git merge feature/short-description
```

Do not commit `.pio/`, `.vscode/`, build output, or local credentials.

## Local Secrets

No Wi-Fi credentials, API keys, tokens, or other secrets are included. If a future feature needs them:

1. Copy `include/secrets.example.h` to `include/secrets.h`.
2. Put local-only values in `include/secrets.h`.
3. Never commit `include/secrets.h`.

## License

A license has not been selected yet. This repository must not be published as an officially licensed open-source project until the owner chooses one and replaces `LICENSE` with its full text.

Common choices:

- **MIT**: simple and permissive; allows reuse with attribution and provides limited warranty protection.
- **Apache-2.0**: permissive and includes an explicit patent grant; longer and more formal than MIT.
- **GPL-3.0**: requires distributed derivative works to remain under GPL-3.0; suitable when preserving software freedom is a priority.

The project owner should choose based on the intended contribution and distribution model before the first public release.
