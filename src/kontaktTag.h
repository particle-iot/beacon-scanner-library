#ifndef KONTAKT_TAG_H
#define KONTAKT_TAG_H

#include "beacon.h"

class KontaktTag : public Beacon
{
public:
    KontaktTag() : Beacon(SCAN_KONTAKT)
    {
        battery = temperature = 0xFF;
        button_time = accel_last_double_tap = accel_last_movement = 0xFFFF;
        accel_data = false;
    };
    ~KontaktTag() = default;

    static bool isTag(const BleScanResult *scanResult);
    void populateData(const BleScanResult *scanResult) override;
    void toJson(JSONWriter *writer) const override;

    uint8_t getBattery() const { return battery; };
    int8_t getTemperature() const { return temperature; };
    uint8_t getAccelSensitivity() const { return accel_sensitivity; };
    uint16_t getButtonTime() const { return button_time; };
    uint16_t getAccelLastDoubleTap() const { return accel_last_double_tap; };
    uint16_t getAccelLastMovement() const { return accel_last_movement; };
    bool hasAccelData() const { return accel_data; };
    int8_t getAccelXaxis() const { return x_axis; };
    int8_t getAccelYaxis() const { return y_axis; };
    int8_t getAccelZaxis() const { return z_axis; };

private:
    uint8_t battery, accel_sensitivity;
    uint16_t button_time, accel_last_double_tap, accel_last_movement;
    int8_t x_axis, y_axis, z_axis, temperature;
    bool accel_data;
};

#endif