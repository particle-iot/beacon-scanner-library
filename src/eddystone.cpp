#include "eddystone.h"

void Eddystone::populateData(const BleScanResult *scanResult)
{
    address = scanResult->address;
    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    uint8_t count = scanResult->advertisingData.get(BleAdvertisingDataType::SERVICE_DATA, buf, sizeof(buf));
    if (count > 3 && buf[0] == 0xAA && buf[1] == 0xFE) // Eddystone UUID
    {
        switch (buf[2])
        {
        case 0x00:
            uid.populateData(buf, scanResult->rssi);
            break;
        case 0x10:
            Log.info("Found Eddystone URL");
            break;
        case 0x20:
            Log.info("Found Eddystone TLM");
        }
    }
}

bool Eddystone::isBeacon(const BleScanResult *scanResult)
{
    if (scanResult->advertisingData.contains(BleAdvertisingDataType::SERVICE_DATA))
    {
        uint8_t buf[BLE_MAX_ADV_DATA_LEN];
        uint8_t count = scanResult->advertisingData.get(BleAdvertisingDataType::SERVICE_DATA, buf, BLE_MAX_ADV_DATA_LEN);
        if (count > 3 && buf[0] == 0xAA && buf[1] == 0xFE) // Eddystone UUID
            return true;
    }
    return false;
}

void Eddystone::toJson(JSONBufferWriter *writer)
{
        writer->name(address.toString()).beginObject();
        if (uid.found) 
        {
            writer->name("uid").beginObject();
            writer->name("power").value(uid.getPower());
            writer->name("namespace").value(uid.namespaceString());
            writer->name("instance").value(uid.instanceString());
            writer->name("rssi").value(uid.getRssi());
            writer->endObject();
        }
        writer->endObject();
}

void Eddystone::Uid::populateData(uint8_t *buf, int8_t rssi)
{
    found = true;
    power = (int8_t)buf[3];
    memcpy(name,buf+4,10);
    memcpy(instance, buf+14,6);
    this->rssi+=rssi;
    rssi_count++;
}