#ifndef LAIRD_BT510_H
#define LAIRD_BT510_H

#include "beacon.h"

class LairdBt510;

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

enum class lairdbt510_flags {
    RTC                 = 0x01,
    ACTIVE              = 0x02,
    ANY_FLAG_SET        = 0x04,
    LOW_BATTERY_ALARM   = 0x80,
    HIGH_TEMP_ALARM_0   = 0x100,
    HIGH_TEMP_ALARM_1   = 0x200,
    LOW_TEMP_ALARM_0    = 0x400,
    LOW_TEMP_ALARM_1    = 0x800,
    DELTA_TEMP_ALARM    = 0x1000,
    MOVEMENT_ALARM      = 0x4000,
    MAGNET_STATE        = 0x8000
};

typedef void (*LairdBt510EventCallback)(LairdBt510& beacon, lairdbt510_event_type evt);

class LairdBt510 : public Beacon
{
public:
    LairdBt510() : 
        Beacon(SCAN_LAIRDBT510)
        { };
    ~LairdBt510() = default;

    static bool isBeacon(const BleScanResult *scanResult);
    void populateData(const BleScanResult *scanResult) override;
    void toJson(JSONWriter *writer) const override;
    static void setEventCallback(LairdBt510EventCallback callback) { LairdBt510::_eventCallback = callback; };
    static void setAlarmCallback(LairdBt510EventCallback callback) { LairdBt510::_alarmCallback = callback; };

    int16_t getTemperature() const { return _temp; };
    bool magnetNear() const {return !_magnet_state;};
    uint16_t getRecordNumber() const { return _record_number; };
    uint16_t getBattVoltage() const { return _batt_voltage; };

private:
    int16_t _temp;
    uint16_t _record_number, _batt_voltage;
    bool _magnet_event, _magnet_state, _movement;
    static LairdBt510EventCallback _eventCallback, _alarmCallback;
};

#endif