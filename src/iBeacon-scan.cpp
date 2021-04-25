#include "iBeacon-scan.h"
#include "os-version-macros.h"

void iBeaconScan::populateData(const BleScanResult *scanResult)
{
        address = ADDRESS(scanResult);
        uint8_t custom_data[BLE_MAX_ADV_DATA_LEN];
        ADVERTISING_DATA(scanResult).customData(custom_data, sizeof(custom_data));
        snprintf(uuid, sizeof(uuid), "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                 custom_data[4], custom_data[5], custom_data[6], custom_data[7], custom_data[8], custom_data[9], custom_data[10], custom_data[11], custom_data[12],
                 custom_data[13], custom_data[14], custom_data[15], custom_data[16], custom_data[17], custom_data[18], custom_data[19]);
        major = custom_data[20] * 256 + custom_data[21];
        minor = custom_data[22] * 256 + custom_data[23];
        power = (int8_t)custom_data[24];
        rssi += RSSI(scanResult);
        rssi_count++;
}

bool iBeaconScan::isBeacon(const BleScanResult *scanResult)
{
    uint8_t custom_data[BLE_MAX_ADV_DATA_LEN];

    if (ADVERTISING_DATA(scanResult).customData(custom_data, sizeof(custom_data)) == 25)
    {
        if (custom_data[0] == 0x4c && custom_data[1] == 0x00 && custom_data[2] == 0x02 && custom_data[3] == 0x15)
        {
            return true;
        }
    }
    return false;
}

void iBeaconScan::toJson(JSONWriter *writer) const
{
        writer->name(address.toString()).beginObject();
        writer->name("uuid").value(getUuid());
        writer->name("major").value(getMajor());
        writer->name("minor").value(getMinor());
        writer->name("power").value(getPower());
        writer->name("rssi").value(getRssi());
        writer->endObject();
}