/**
 * @file PortType.h
 * @author Etienne Montenegro
 * @brief Physical port type enum and string conversion helpers.
 */

#ifndef PORT_TYPE_H
#define PORT_TYPE_H

#include <stdint.h>
#include <cstring>

enum class PortType : uint8_t {
  none = 0,
  port_a,
  port_b,
  port_c,
};

inline const char* port_type_to_string(PortType type) {
  switch (type) {
    case PortType::port_a:
      return "portA";
    case PortType::port_b:
      return "portB";
    case PortType::port_c:
      return "portC";
    default:
      return "none";
  }
}

inline PortType port_type_from_string(const char* str) {
  if (!str || str[0] == '\0') {
    return PortType::none;
  }

  if (strcmp(str, "portA") == 0 || strcmp(str, "port_a") == 0 || strcmp(str, "A") == 0 || strcmp(str, "a") == 0) {
    return PortType::port_a;
  }
  if (strcmp(str, "portB") == 0 || strcmp(str, "port_b") == 0 || strcmp(str, "B") == 0 || strcmp(str, "b") == 0) {
    return PortType::port_b;
  }
  if (strcmp(str, "portC") == 0 || strcmp(str, "port_c") == 0 || strcmp(str, "C") == 0 || strcmp(str, "c") == 0) {
    return PortType::port_c;
  }

  return PortType::none;
}

#endif  // PORT_TYPE_H
