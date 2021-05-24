#include "lairdbt510.h"

LairdBt510EventCallback LairdBt510::_eventCallback = nullptr;
LairdBt510EventCallback LairdBt510::_alarmCallback = nullptr;

void LairdBt510::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);
    uint8_t buf[38];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, 38);
    uint16_t prev_record = _record_number;
    if (count > 25) {   // Advertising data is correct, either table 1 or table 3
        bool prev_magnet = _magnet_state;
        uint16_t flags = buf[7] << 8 | buf[6];
        _magnet_state = (flags & (uint16_t)lairdbt510_flags::MAGNET_STATE);
        _record_number = buf[16] << 8 | buf[15];
        if (count == 26 && buf[2] == 0x01) {
            switch ((lairdbt510_event_type)buf[14])
            {
            case lairdbt510_event_type::TEMPERATURE:
            case lairdbt510_event_type::ALARM_HIGH_TEMP_1:
            case lairdbt510_event_type::ALARM_HIGH_TEMP_2:
            case lairdbt510_event_type::ALARM_HIGH_TEMP_CLEAR:
            case lairdbt510_event_type::ALARM_LOW_TEMP_1:
            case lairdbt510_event_type::ALARM_LOW_TEMP_2:
            case lairdbt510_event_type::ALARM_LOW_TEMP_CLEAR:
            case lairdbt510_event_type::ALARM_DELTA_TEMP:
                _temp = buf[22] << 8 | buf[21];
                break;
            case lairdbt510_event_type::MAGNET_PROXIMITY:
                _magnet_event = buf[21] == 0x01;
                break;
            case lairdbt510_event_type::MOVEMENT:
                break;
            case lairdbt510_event_type::BATTERY_GOOD:
            case lairdbt510_event_type::ADVERTISE_ON_BUTTON:
            case lairdbt510_event_type::BATTERY_BAD:
                _batt_voltage = buf[22] << 8 | buf[21];
                break;
            case lairdbt510_event_type::RESET:      // Reset
                break;
            default:
                break;
            }
            if (_eventCallback && _record_number != prev_record)
                _eventCallback(*this, (lairdbt510_event_type)buf[14]);
        }
        if (_alarmCallback != nullptr) {
            if (flags & (uint16_t)lairdbt510_flags::LOW_BATTERY_ALARM)
                _alarmCallback(*this, lairdbt510_event_type::BATTERY_BAD);
            if (flags & (uint16_t)lairdbt510_flags::HIGH_TEMP_ALARM_0)
                _alarmCallback(*this, lairdbt510_event_type::ALARM_HIGH_TEMP_1);
            if (flags & (uint16_t)lairdbt510_flags::HIGH_TEMP_ALARM_1)
                _alarmCallback(*this, lairdbt510_event_type::ALARM_HIGH_TEMP_2);
            if (flags & (uint16_t)lairdbt510_flags::LOW_TEMP_ALARM_0)
                _alarmCallback(*this, lairdbt510_event_type::ALARM_LOW_TEMP_1);
            if (flags & (uint16_t)lairdbt510_flags::LOW_TEMP_ALARM_1)
                _alarmCallback(*this, lairdbt510_event_type::ALARM_LOW_TEMP_2);
            if (flags & (uint16_t)lairdbt510_flags::DELTA_TEMP_ALARM)
                _alarmCallback(*this, lairdbt510_event_type::ALARM_DELTA_TEMP);
            if (flags & (uint16_t)lairdbt510_flags::MOVEMENT_ALARM)
                _alarmCallback(*this, lairdbt510_event_type::MOVEMENT);
            if (prev_magnet != _magnet_state)
                _alarmCallback(*this, lairdbt510_event_type::MAGNET_PROXIMITY);
        }
    }
}

bool LairdBt510::isBeacon(const BleScanResult *scanResult)
{
    if (ADVERTISING_DATA(scanResult).contains(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA))
    {
        uint8_t buf[9];
        ADVERTISING_DATA(scanResult).get(buf, 9);
        if (buf[0] == 0x02 && buf[1] == 0x01 && buf[2] == 0x06 && buf[3] == 0x1b && buf[4] == 0xFF &&
            buf[5] == 0x77 && buf[6] == 0x00 && (buf[7] == 0x01 || buf[7] == 0x02) && buf[8] == 0x00) { 
                return true;
            }
    
    } 
    return false;
}

void LairdBt510::toJson(JSONWriter *writer) const
{
        writer->name(address.toString()).beginObject();
        writer->name("rssi").value(getRssi());
        writer->endObject();
}