/**
 * @file UnitI2C.h
 * @author Etienne Montenegro
 * @brief Intermediate base class for all I2C-connected units.
 *
 */

#pragma once
#ifndef MBK_UNIT_I2C_H
#define MBK_UNIT_I2C_H

#include <Arduino.h>
#include <Wire.h>

#include "sensors/units/Unit.h"

enum class WireStatus : uint8_t {
    SUCCESS      = 0,
    OVERFLOW     = 1,
    NACK_ADDRESS = 2,
    NACK_DATA    = 3,
    OTHER        = 4,
    TIMEOUT      = 5,
};

class UnitI2C : public Unit {
 protected:  
    TwoWire* _wire = &Wire;

    static constexpr uint32_t k_retry_min_ms = 250;
    static constexpr uint32_t k_retry_max_ms = 5000;

    bool     _connected      = false;
    uint32_t _retry_delay_ms = k_retry_min_ms;
    uint32_t _next_probe_ms  = 0;


    /**
     * @brief Record a disconnection and schedule the next probe with
     *        exponential back-off (doubles each failure, capped at
     *        k_retry_max_ms).
     */
    void note_disconnect(uint32_t now_ms) {
        _connected      = false;
        _next_probe_ms  = now_ms + _retry_delay_ms;
        _retry_delay_ms = (_retry_delay_ms < (k_retry_max_ms / 2))
                              ? (_retry_delay_ms * 2)
                              : k_retry_max_ms;
    }

    /**
     * @brief Attempt to re-initialise the device if the back-off window has
     *        elapsed.  Delegates to begin_impl() so each concrete unit reuses
     *        its own setup sequence.
     *
     * @return true  Device successfully re-initialised.
     * @return false Still within back-off window, or begin_impl() failed.
     */
    bool try_reconnect(uint32_t now_ms) {
        if (now_ms < _next_probe_ms) {
            return false;
        }
        
        _logger.infoln("Trying to initialize unit");
        if (!begin()) {
            _logger.warningln("Failed to reconnect with unit");
            note_disconnect(now_ms);
            return false;
        }

        _connected      = true;
        _retry_delay_ms = k_retry_min_ms;
        _next_probe_ms  = 0;
        return true;
    }

    WireStatus on_io_fail(WireStatus status, uint32_t now_ms) {
        if (status != WireStatus::SUCCESS) {
            _logger.warningln("I2C error: WireStatus=%d", static_cast<uint8_t>(status));
            note_disconnect(now_ms);
        }
        return status;
    }

    /**
     * @brief Convenience guard for the top of sample_impl().
     *        Returns true when the device is (or just became) reachable.
     */
    bool ensure_connected(uint32_t now_ms) {
        if(_connected) return true;

        return try_reconnect(now_ms);
    }

    /**
     * @brief Write bytes to a device register over I2C.
     * @return WireStatus::SUCCESS if acknowledged, specific error code otherwise.
     */
    WireStatus writeBytes(uint8_t addr, uint8_t reg, uint8_t length, uint8_t *buffer) {
        _wire->beginTransmission(addr);
        _wire->write(reg);
        for (uint8_t i = 0; i < length; i++) {
            _wire->write(buffer[i]);
        }
        return static_cast<WireStatus>(_wire->endTransmission());
    }

    /**
     * @brief Read bytes from a device register over I2C.
     *        Uses a repeated-start (endTransmission(false)) for correct
     *        register-read protocol.
     * @return WireStatus::SUCCESS if read completed, specific error code otherwise.
     */
    WireStatus readBytes(uint8_t addr, uint8_t reg, uint8_t length, uint8_t *buffer) {
        _wire->beginTransmission(addr);
        _wire->write(reg);
        WireStatus s = static_cast<WireStatus>(_wire->endTransmission());
        if (s != WireStatus::SUCCESS) {
            return s;
        }
        if (_wire->requestFrom(addr, length)!= length) {
            return WireStatus::OTHER;
        }
        for (uint8_t i = 0; i < length; i++) {
            buffer[i] = _wire->read();
        }
        return WireStatus::SUCCESS;
    }

 public:
    UnitI2C()            : Unit() {}
    explicit UnitI2C(TwoWire& w) : Unit(), _wire(&w) {}
};

#endif  // MBK_UNIT_I2C_H
