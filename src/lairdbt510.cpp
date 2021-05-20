#include "lairdbt510.h"

void LairdBt510::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);
    uint8_t buf[38];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, 38);
    if (count == 26 && buf[2] == 0x01) {
        switch ((lairdbt510_event_type)buf[14])
        {
        case lairdbt510_event_type::TEMPERATURE:       // Temperature event type
            _temp = buf[22] << 8 | buf[21];
            break;
        case lairdbt510_event_type::MAGNET_PROXIMITY:      // Magnet state event type
            _magnet = buf[21] == 0x00;
            break;
        case lairdbt510_event_type::MOVEMENT:      // Movement event type
            break;
        case lairdbt510_event_type::ALARM_HIGH_TEMP_1:      // Alarm high temp 1 event type
            break;
        case lairdbt510_event_type::ALARM_HIGH_TEMP_2:      // Alarm high temp 2 event type
            break;
        case lairdbt510_event_type::ALARM_HIGH_TEMP_CLEAR:      // Alarm High Temp clear
            break;
        case lairdbt510_event_type::ALARM_LOW_TEMP_1:      // Alarm Low Temp 1
            break;
        case lairdbt510_event_type::ALARM_LOW_TEMP_2:      // Alarm Low Temp 2
            break;
        case lairdbt510_event_type::ALARM_LOW_TEMP_CLEAR:      // Alarm Low Temp clear
            break;
        case lairdbt510_event_type::ALARM_DELTA_TEMP:      // Alarm delta temp
            break;
        case lairdbt510_event_type::BATTERY_GOOD:      // Battery good
            break;
        case lairdbt510_event_type::ADVERTISE_ON_BUTTON:      // Advertise on Button
            break;
        case lairdbt510_event_type::BATTERY_BAD:      // Battery bad
            break;
        case lairdbt510_event_type::RESET:      // Reset
            break;
        default:
            break;
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