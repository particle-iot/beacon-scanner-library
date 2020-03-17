#ifndef KONTAKT_TAG_H
#define KONTAKT_TAG_H

#include "Particle.h"

class KontaktTag
{
public:
    KontaktTag()
    {
        battery = temperature = 0xFF;
        button_time = accel_last_double_tap = accel_last_movement = 0xFFFF;
        accel_data = false;
    };
    ~KontaktTag() = default;

    static bool isTag(const BleScanResult *scanResult);
    void populateData(const BleScanResult *scanResult);
    void toJson(JSONBufferWriter *writer);

    BleAddress getAddress() { return address; };
    uint8_t getBattery() { return battery; };
    uint8_t getTemperature() { return temperature; };
    uint8_t getAccelSensitivity() { return accel_sensitivity; };
    uint16_t getButtonTime() { return button_time; };
    uint16_t getAccelLastDoubleTap() { return accel_last_double_tap; };
    uint16_t getAccelLastMovement() { return accel_last_movement; };
    int8_t getAccelXaxis() { return x_axis; };
    int8_t getAccelYaxis() { return y_axis; };
    int8_t getAccelZaxis() { return z_axis; };

private:
    BleAddress address;
    uint8_t battery, temperature, accel_sensitivity;
    uint16_t button_time, accel_last_double_tap, accel_last_movement;
    int8_t x_axis, y_axis, z_axis;
    bool accel_data;
};

#endif