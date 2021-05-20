#ifndef LAIRD_BT510_H
#define LAIRD_BT510_H

#include "beacon.h"

enum class lairdbt510_event_type {
    TEMPERATURE         = 1,
    MAGNET_PROXIMITY    = 2,
    MOVEMENT            = 3,
    ALARM_HIGH_TEMP_1   = 4,
    ALARM_HIGH_TEMP_2   = 5,
    ALARM_HIGH_TEMP_CLEAR = 6,
    ALARM_LOW_TEMP_1    = 7,
    ALARM_LOW_TEMP_2    = 8,
    ALARM_LOW_TEMP_CLEAR = 9,
    ALARM_DELTA_TEMP    = 10,
    BATTERY_GOOD        = 12,
    ADVERTISE_ON_BUTTON = 13,
    BATTERY_BAD         = 16,
    RESET               = 17
};

class LairdBt510 : public Beacon
{
public:
    LairdBt510() : Beacon(SCAN_LAIRDBT510)
    {

    };
    ~LairdBt510() = default;

    static bool isBeacon(const BleScanResult *scanResult);
    void populateData(const BleScanResult *scanResult) override;
    void toJson(JSONWriter *writer) const override;

    int8_t getTemperature() const { return _temp; };
    bool magnetNear() const {return _magnet;};

private:
    int16_t _temp;
    uint16_t _record_number;
    bool _magnet, _movement;
};

#endif