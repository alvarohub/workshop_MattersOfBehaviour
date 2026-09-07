#pragma once
#ifndef MBK_UNIT_MPU6886_H
#define MBK_UNIT_MPU6886_H

#include <Arduino.h>
#include <ArduinoLog.h>
#include <Wire.h>

#include "sensors/units/i2c/UnitI2C.h"
// implementation based on https://github.com/m5stack/M5Unit-Sonic/tree/master



class MPU6886 : public UnitI2C {
    //  Hardware
    static constexpr uint8_t k_address =  0x68;
    

    static constexpr const std::uint8_t DEV_ID_MPU6886 = 0x19;
    static constexpr const std::uint8_t DEV_ID_MPU6050 = 0x68;
    static constexpr const std::uint8_t DEV_ID_MPU9250 = 0x71;

    static constexpr const std::uint8_t REG_WHOAMI           = 0x75;
    static constexpr const std::uint8_t REG_ACCEL_INTEL_CTRL = 0x69;
    static constexpr const std::uint8_t REG_SMPLRT_DIV       = 0x19;
    static constexpr const std::uint8_t REG_INT_PIN_CFG      = 0x37;
    static constexpr const std::uint8_t REG_INT_ENABLE       = 0x38;
    static constexpr const std::uint8_t REG_ACCEL_XOUT_H     = 0x3B;
    static constexpr const std::uint8_t REG_ACCEL_XOUT_L     = 0x3C;
    static constexpr const std::uint8_t REG_ACCEL_YOUT_H     = 0x3D;
    static constexpr const std::uint8_t REG_ACCEL_YOUT_L     = 0x3E;
    static constexpr const std::uint8_t REG_ACCEL_ZOUT_H     = 0x3F;
    static constexpr const std::uint8_t REG_ACCEL_ZOUT_L     = 0x40;

    static constexpr const std::uint8_t REG_TEMP_OUT_H    = 0x41;
    static constexpr const std::uint8_t REG_TEMP_OUT_L    = 0x42;

    static constexpr const std::uint8_t REG_GYRO_XOUT_H   = 0x43;
    static constexpr const std::uint8_t REG_GYRO_XOUT_L   = 0x44;
    static constexpr const std::uint8_t REG_GYRO_YOUT_H   = 0x45;
    static constexpr const std::uint8_t REG_GYRO_YOUT_L   = 0x46;
    static constexpr const std::uint8_t REG_GYRO_ZOUT_H   = 0x47;
    static constexpr const std::uint8_t REG_GYRO_ZOUT_L   = 0x48;

    static constexpr const std::uint8_t REG_USER_CTRL     = 0x6A;
    static constexpr const std::uint8_t REG_PWR_MGMT_1    = 0x6B;
    static constexpr const std::uint8_t REG_PWR_MGMT_2    = 0x6C;
    static constexpr const std::uint8_t REG_CONFIG        = 0x1A;
    static constexpr const std::uint8_t REG_GYRO_CONFIG   = 0x1B;
    static constexpr const std::uint8_t REG_ACCEL_CONFIG  = 0x1C;
    static constexpr const std::uint8_t REG_ACCEL_CONFIG2 = 0x1D;
    static constexpr const std::uint8_t REG_LP_MODE_CFG   = 0x1E;



    static constexpr const std::uint8_t REG_GYRO_OFFSET   = 0x13;
   
    enum Ascale { 
        AFS_2G = 0, 
        AFS_4G, 
        AFS_8G, 
        AFS_16G
    };

    enum Gscale{ 
        GFS_250DPS = 0, 
        GFS_500DPS, 
        GFS_1000DPS, 
        GFS_2000DPS
    };

    enum Fodr{ 
        ODR_1kHz  = 0, 
        ODR_500Hz = 1, 
        ODR_250Hz = 3, 
        ODR_200Hz = 4, 
        ODR_125Hz = 7,
        ODR_100Hz = 9, 
        ODR_50Hz  = 19, 
        ODR_10Hz  = 99
    };



    float aRes, gRes; uint8_t imuId = 0;
    Gscale Gyscale;
    Ascale Acscale;
    float _accel[3]{0};
    float _gyro[3]{0};
    float _temp{0};
    
    bool getAccelAdc(int16_t* ax, int16_t* ay, int16_t* az) {
        uint8_t buf[6];
        if (readBytes(k_address, REG_ACCEL_XOUT_H, 6, buf) != WireStatus::SUCCESS) {
            return false;
        }
        *ax = ((int16_t)buf[0] << 8) | buf[1];
        *ay = ((int16_t)buf[2] << 8) | buf[3];
        *az = ((int16_t)buf[4] << 8) | buf[5];
        return true;
    }

    bool getGyroAdc(int16_t* gx, int16_t* gy, int16_t* gz) {
        uint8_t buf[6];
        if (readBytes(k_address, REG_GYRO_XOUT_H, 6, buf) != WireStatus::SUCCESS) {
            return false;
        }
        *gx = ((uint16_t)buf[0] << 8) | buf[1];
        *gy = ((uint16_t)buf[2] << 8) | buf[3];
        *gz = ((uint16_t)buf[4] << 8) | buf[5];
        return true;
    }

    bool getTempAdc(int16_t *t) {
        uint8_t buf[2];
        if (readBytes(k_address, REG_TEMP_OUT_H, 2, buf) != WireStatus::SUCCESS) {
            return false;
        }
        *t = ((uint16_t)buf[0] << 8) | buf[1];
        return true;
    }
    
    void updateGres() {
        switch (Gyscale) {
            case GFS_250DPS:
                gRes = 250.0/32768.0;
            break;
            case GFS_500DPS:
                gRes = 500.0/32768.0;
            break;
            case GFS_1000DPS:
                gRes = 1000.0/32768.0;
            break;
            case GFS_2000DPS:
                gRes = 2000.0/32768.0;
            break;
        }
    }

    void updateAres() {
        switch (Acscale) {
            case AFS_2G:
                aRes = 2.0/32768.0;
            break;
            case AFS_4G:
                aRes = 4.0/32768.0;
            break;
            case AFS_8G:
                aRes = 8.0/32768.0;
            break;
            case AFS_16G:
                aRes = 16.0/32768.0;
            break;
        }
    }
    
    void setGyroFsr(Gscale scale) {
        unsigned char regdata;
        regdata = (scale<<3);
        writeBytes(k_address, REG_GYRO_CONFIG, 1, &regdata);
        Gyscale = scale;
        updateGres();
    }

    void setAccelFsr(Ascale scale) {
        unsigned char regdata;
        regdata = (scale<<3);
        writeBytes(k_address, REG_ACCEL_CONFIG, 1, &regdata);
        updateAres();
    }

    bool getAccelData(float* ax, float* ay, float* az) {
        int16_t accX = 0;
        int16_t accY = 0;
        int16_t accZ = 0;
        if (!getAccelAdc(&accX, &accY, &accZ)) {
            return false;
        }
        *ax = (float)accX * aRes;
        *ay = (float)accY * aRes;
        *az = (float)accZ * aRes;
        return true;
    }
        
    bool getGyroData(float* gx, float* gy, float* gz) {
        int16_t gyroX = 0;
        int16_t gyroY = 0;
        int16_t gyroZ = 0;
        if (!getGyroAdc(&gyroX, &gyroY, &gyroZ)) {
            return false;
        }
        *gx = (float)gyroX * gRes;
        *gy = (float)gyroY * gRes;
        *gz = (float)gyroZ * gRes;
        return true;
    }

    bool getTempData(float *t) {
        int16_t temp = 0;
        if (!getTempAdc(&temp)) {
            return false;
        }
        *t = (float)temp / 326.8 + 25.0;
        return true;
    }

    void setGyroOffset(uint16_t x, uint16_t y, uint16_t z) {
        uint8_t buf_out[6];
        buf_out[0] = x >> 8;
        buf_out[1] = x & 0xff;
        buf_out[2] = y >> 8;
        buf_out[3] = y & 0xff;
        buf_out[4] = z >> 8;
        buf_out[5] = z & 0xff;
        writeBytes(k_address, REG_GYRO_OFFSET, 6, buf_out);
    }

public:

    MPU6886()           : UnitI2C(){}
    MPU6886(TwoWire& w) : UnitI2C(w) {}

    bool begin_impl() override { 
        unsigned char tempdata[1];
        unsigned char regdata;
        _logger.setPrefix(&MPU6886::prefix_print);

        Gyscale = GFS_2000DPS;
        Acscale = AFS_8G;
        
        readBytes(k_address, REG_WHOAMI, 1, tempdata);
        imuId = tempdata[0];
        _logger.traceln("MPU WHO_AM_I: 0x%02X", imuId);

        if (imuId != DEV_ID_MPU6886 && imuId != DEV_ID_MPU6050 && imuId != DEV_ID_MPU9250) {
            _logger.errorln("Unknown MPU WHO_AM_I: 0x%02X", imuId);
            _connected = false;
            return false;
        }

        regdata = 0x00;
        writeBytes(k_address, REG_PWR_MGMT_1, 1, &regdata);
        delay(10);

        regdata = (0x01<<7);
        writeBytes(k_address, REG_PWR_MGMT_1, 1, &regdata);
        delay(10);

        regdata = (0x01<<0);
        writeBytes(k_address, REG_PWR_MGMT_1, 1, &regdata);
        delay(10);

        // +- 8g
        regdata = 0x10;
        writeBytes(k_address, REG_ACCEL_CONFIG, 1, &regdata);

        // +- 2000 dps
        regdata = 0x18;
        writeBytes(k_address, REG_GYRO_CONFIG, 1, &regdata);

        // 1khz output
        regdata = 0x01;
        writeBytes(k_address, REG_CONFIG, 1, &regdata);

        // 2 div
        regdata = 0x01;
        writeBytes(k_address, REG_SMPLRT_DIV, 1, &regdata);

        regdata = 0x00;
        writeBytes(k_address, REG_INT_ENABLE, 1, &regdata);

        regdata = 0x00;
        writeBytes(k_address, REG_ACCEL_CONFIG2, 1, &regdata);

        regdata = 0x00;
        writeBytes(k_address, REG_USER_CTRL, 1, &regdata);

        regdata = 0x22;
        writeBytes(k_address, REG_INT_PIN_CFG, 1, &regdata);

        regdata = 0x01;
        writeBytes(k_address, REG_INT_ENABLE, 1, &regdata);

        setGyroFsr(Gyscale);
        setAccelFsr(Acscale);
        
        _connected = true;
        return _connected;
    }

    bool sample_impl(uint32_t now_ms) override {
        if (!ensure_connected(now_ms))
        {
            return false;
        }

        int16_t ax, ay, az = 0;
        if (!getAccelAdc(&ax, &ay, &az)) {
            note_disconnect(now_ms);
            return false;
        }
        _accel[0] = apply_lowpass(map_to_float(static_cast<float>(ax), static_cast<float>(-32768), static_cast<float>(32768)), _accel[0]); 
        _accel[1] = apply_lowpass(map_to_float(static_cast<float>(ay), static_cast<float>(-32768), static_cast<float>(32768)), _accel[1]); 
        _accel[2] = apply_lowpass(map_to_float(static_cast<float>(az), static_cast<float>(-32768), static_cast<float>(32768)), _accel[2]);


        int16_t gx = 0, gy = 0, gz = 0;
        if (!getGyroAdc(&gx, &gy, &gz)) {
            note_disconnect(now_ms);
            return false;
        }
        _gyro[0] = apply_lowpass(map_to_float(static_cast<float>(gx), static_cast<float>(-32768), static_cast<float>(32768)), _gyro[0]);
        _gyro[1] = apply_lowpass(map_to_float(static_cast<float>(gy), static_cast<float>(-32768), static_cast<float>(32768)), _gyro[1]);
        _gyro[2] = apply_lowpass(map_to_float(static_cast<float>(gz), static_cast<float>(-32768), static_cast<float>(32768)), _gyro[2]);

        float t = 0.0f;
        if (!getTempData(&t)) {
            note_disconnect(now_ms);
            return false;
        }
        _temp = apply_lowpass(map_to_float(t,-40.0F,85.0F), _temp);
     //   _logger.traceln("Ax: %d, Ay: %d, Az:%d, Gx: %d, Gy: %d, Gz: %d, Temp:%d",ax, ay, az, gx, gy, gz, t);
     //   _logger.traceln("Ax: %F, Ay: %F, Az:%F, Gx: %F, Gy: %F, Gz: %F, Temp:%F", _accel[0], _accel[1], _accel[2], _gyro[0], _gyro[1], _gyro[2], _temp);
        return true;
    }




    void write_json(JsonObject& dst) const override
    {   
        Unit::write_json(dst);
        
        for (size_t i = 0; i < 3; i++)
        {
            dst["val"].add(_accel[i]);
        }
        for (size_t i = 0; i < 3; i++)
        {
            dst["val"].add(_gyro[i]);
        }
        dst["val"].add( _temp);

    }

        static void prefix_print(Print* _logOutput, int logLevel){
      _logOutput->printf("[Unit - MPU6886] ");
  }
};

#endif  //MBK_UNIT_MPU6886_H