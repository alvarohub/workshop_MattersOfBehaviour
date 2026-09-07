# Coding Conventions

Prescriptive coding standards for the MisBKit firmware codebase.

## Naming

- **Classes / structs / types**: `PascalCase` (`MotorManager`, `UnitI2C`, `WebSocketProtocol`)
- **Methods / functions**: `snake_case` (`set_enabled`, `sample_impl`, `add_data_to_json`)
- **Private / protected member variables**: `_prefix` (`_enable`, `_wire`, `_logger`)
- **Enum values**: `PascalCase` within `enum class` (`UnitModel::SensorGeneric`, `WireStatus::NackAddress`)
- **Compile-time constants**: `static constexpr` with `k_` prefix (`k_max_channels`, `k_default_min_angle`)
- Avoid plain `enum` in new code — prefer `enum class`
- Avoid `SCREAMING_SNAKE_CASE` for constants in new code

## File Organization

- One class / struct per file preferred; multiple is acceptable when tightly coupled
- File name matches the primary type name
- `#pragma once` for header guards — no `#ifndef` guards

## Braces & Formatting

- **K&R style**: opening brace on the same line as the statement
  ```cpp
  if (condition) {
      do_something();
  } else {
      do_other();
  }
  ```

## Includes

- **Order**: system headers → third-party libraries → project-local (blank lines between groups)
- `#include <Arduino.h>` must come first when present
  ```cpp
  #include <Arduino.h>

  #include <ArduinoJson.h>
  #include <Dynamixel2Arduino.h>

  #include "Configuration.h"
  #include "MotorClass.h"
  ```

## Documentation

- **File headers**: required on all `.h` and `.cpp` files — `@file`, `@author`, `@brief`
- **Method docs**: required on public interfaces of manager / orchestrator classes; optional on private implementation details
- **Language**: English only for all comments and documentation

## Logging

- **ArduinoLog** is the primary logging system (`Log.infoln`, `Log.warningln`, `Log.errorln`, `Log.traceln`)
- No raw `Serial.print` / `Serial.println` for logging in new code

## Error Handling

- **Always log on error paths** — never silently swallow failures
- `Log.warningln` for recoverable issues
- `Log.errorln` for failures that prevent an action

## Memory Management

- **Prefer fixed-size arrays** for time-critical paths (motor control, sensor sampling)
- **Use `std::unique_ptr`** for ownership transfer (e.g., port unit lifecycle)
- Avoid `new` / `delete` directly
- Avoid heap allocation in hot loops

## Constants

- `static constexpr` with `k_` prefix for compile-time constants
- `#define` only for build-system macros (`MBK_PORT_COUNT`, `V_MAJ`) and board-specific pins

## Class Design

- **Prefer composition** for managers and system wiring
- **Use inheritance** for polymorphic hardware abstractions (units, protocols)

## Board-Specific Code

- `#ifdef` with PlatformIO board macros, isolated to `Pins.h` and manager headers
- Hardware limits (`MBK_PORT_COUNT`, `MAX_NUM_MOTOR`) are compile-time macros — never hardcode port or motor counts in logic
