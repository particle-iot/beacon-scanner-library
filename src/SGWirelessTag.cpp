#include "config.h"
#ifdef SUPPORT_SGWIRELESS
#include "SGWirelessTag.h"

Vector<SGWirelessTag> SGWirelessTag::beacons;

void SGWirelessTag::populateData(const BleScanResult *scanResult)
{
    address = ADDRESS(scanResult);
    strcpy(name, ADVERTISING_DATA(scanResult).deviceName().c_str());
    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, sizeof(buf));

    Log.info("Advertising packet: %02x%02x %02x%02x %02x%02x %02x%02x (count: %u)", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7], count);
    if (count == 8 && buf[0] == 0x59 && buf[1] == 0x00) // MSD should be 8 bytes and start with the Nordic UUID
    {
        temperature_raw = (uint16_t)(buf[2] | buf[3] << 8);  // Swap bytes, as they are little-endian
        humidity_raw    = (uint16_t)(buf[4] | buf[5] << 8);
        battery         = (uint8_t)(buf[6]);

        temperature_f = (float)temperature_raw / 100.0;
        humidity_f    = (float)humidity_raw / 100.0;

        rssi = (int8_t)RSSI(scanResult);

        Log.info("{t_raw:%u, t:%0.2f, h_raw:%u, h:%0.2f, b:%u, rssi:%i}", temperature_raw, temperature_f, humidity_raw, humidity_f, battery, rssi);
    }
}

bool SGWirelessTag::isBeacon(const BleScanResult *scanResult)
{
    //Log.log("Advertised name: %s", scanResult->advertisingData.deviceName().c_str());
    return !strcmp(ADVERTISING_DATA(scanResult).deviceName().c_str(), "SGW8130");
}

void SGWirelessTag::toJson(JSONWriter *writer)
{
        writer->name(address.toString()).beginObject();
        writer->name("s").value(rssi);
        if (battery != 0xFF) {
            writer->name("b").value(battery);
        }
        if (temperature_raw != 0xFFFF) {
            writer->name("t").value(temperature_f);
        }
        if (humidity_raw != 0xFFFF) {
            writer->name("h").value(humidity_f);
        }
        writer->endObject();
}

void SGWirelessTag::addOrUpdate(const BleScanResult *scanResult)
{
    uint8_t i;
    for (i = 0; i < beacons.size(); ++i) {
        if (beacons.at(i).getAddress() == ADDRESS(scanResult)) {
            break;
        }
    }
    if (i == beacons.size()) {
        SGWirelessTag new_beacon;
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        beacons.append(new_beacon);
    } else {
        SGWirelessTag& beacon = beacons.at(i);
        beacon.newly_scanned = false;
        beacon.populateData(scanResult);
        beacon.missed_scan = 0;
    }
}

#endif