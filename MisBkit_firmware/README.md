# MisBKit Firmware

A PlatformIO-based ESP32 firmware. Receives JSON commands over WebSocket, drives Dynamixel and PWM motors, and reads units from Grove ports. Written in C++17 on the Arduino framework.

The codebase is organized into layers:
- **Network and services** — Wi-Fi, WebSocket, HTTP, OTA, and SPIFFS file serving
- **Command pipeline** — queued WebSocket frames, JSON validation, and command dispatch
- **Orchestrator** — lifecycle, command routing, polling, telemetry, and subsystem coordination
- **Managers** — Dynamixel motors, RC motors, and Grove ports/units
- **Persistence** — kit and network configuration loading; complete runtime configuration saving

Build environments and library dependencies are in [`platformio.ini`](platformio.ini).

---

## Firmware Architecture

The implemented runtime is structured as follows:

```text
                          +-------+
                          | Wi-Fi |
                          +---+---+
                              |
                +-------------+-------------+
                |                           |
        +-------v--------+          +-------v-------+
        | WebSocket /ws  |          |   HTTP :80    |
        +-------+--------+          +-------+-------+
                |                           |
        +-------v--------+          +-------v--------+
        | WebSocketProto |          | WebInterface   |
        | (ICommunication|          | - embedded SPA |
        |  Protocol)     |          | - SPIFFS upload|
        +-------+--------+          | - OTA update   |
                |                   +-------+--------+
        +-------v--------+                  |
        | CircularBuffer |          +-------v--------+
        +-------+--------+          | SPIFFS files   |
                |                   | and firmware   |
        +-------v--------+          | update         |
        | Command        |          +----------------+
        | JSON validation|
        | cmd/cmds       |
        +-------+--------+
                |
        +-------v----------------+
        | MisBKit orchestrator   |
        | initialize() / update()|
        +-------+----------------+
                |
    +-----------+------------+------------------+----------------+
    |                        |                  |                |
+---v-----------+    +-------v------+    +------v---------+  +--v-----------+
| MotorManager  |    | RCManager    |    | SensorManager  |  | Persistence  |
| Dynamixel     |    | PWM channels |    | Grove ports    |  | SPIFFS       |
| control       |    | ServoEasing  |    | and units      |  | /config.json |
+---+-----------+    +-------+------+    +------+---------+  +--------------+
    |                        |                  |
+---v-----------+    +-------v------+    +------v---------+
| Dynamixel bus |    | PWM outputs  |    | Grove hardware |
| half-duplex   |    |              |    | I2C/analog/etc.|
+---------------+    +--------------+    +----------------+

                       +------------------------+
                       | MisBKit update loop    |
                       | - one queued command   |
                       | - sensor polling       |
                       | - scans/reboot/LED     |
                       | - motor position data  |
                       +------------------------+
```

### Layer details

**1. Network and services** — `NetworkHandler` tries the configured Wi-Fi router and falls back to an access point. `WebSocketProtocol` serves the `/ws` endpoint and allows one active client. `WebInterface` serves the embedded SPA, handles file uploads to SPIFFS, and configures OTA updates over HTTP. `WebSocketProtocol` implements the `ICommunicationProtocol` interface, but `MisBKit` currently owns and uses the concrete WebSocket implementation directly; replacing the transport would require changes to the orchestrator wiring.

**2. Command pipeline** — WebSocket callbacks copy incoming data into a fixed-size buffer and queue commands in a circular buffer. The main loop processes one queued command per update. `Command` validates JSON and distinguishes a single `cmd` object from a `cmds` batch; `MisBKit` maps command names to enum values and dispatches actions to the relevant subsystem. Fragmented WebSocket frames are accumulated, but the current queue operation uses the final callback length rather than the accumulated frame length, so fragmented or oversized frames are not fully robust.

**3. Orchestrator** — A single `MisBKit` object owns all runtime subsystems and is instantiated globally in `main.cpp`. Initialization mounts SPIFFS, loads kit and network configuration, initializes Wi-Fi, registers WebSocket callbacks, starts HTTP services, and initializes sensor, Dynamixel, and RC managers. Sensor and RC configuration deserialization during startup is currently disabled, although both configurations are written when saving. Each update processes one queued command, polls sensors, handles scans/reboots/LED state, and periodically sends motor position telemetry. Curve playback commands and implementation exist, but the periodic `update_curve_play()` call is disabled.

**4. Managers** — Three managers own and drive the hardware subsystems:
- **MotorManager** — Drives Dynamixel motors over a half-duplex serial bus. Each motor is wrapped in a class that exposes wheel/joint modes, position and speed control, and status reads.
- **RCManager** — Controls up to four PWM servo channels with easing curves, configurable angle limits, and per-channel speed constraints.
- **SensorManager** — Owns an array of ports (one per Grove connector). Each port holds zero or more units. Units implement begin, sample, and write methods. The manager ticks each unit at its configured sample period.

**5. Persistence and hardware** — `PersistantConfiguration` mounts SPIFFS and reads or writes `/config.json`. The current startup path applies kit and network settings only; sensor and RC settings are saved but not restored from the file. Board-specific GPIO assignments are in a compile-time header. The Dynamixel bus uses a half-duplex direction-control pin, and the firmware updates a status LED. There is no separate dedicated clear-button handling path in the firmware; button units are supported as Grove sensor units.

---

See [CODING_CONVENTIONS.md](CODING_CONVENTIONS.md) for coding standards.

