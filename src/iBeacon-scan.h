
#ifndef IBEACON_SCAN_H
#define IBEACON_SCAN_H

#include "beacon.h"

class iBeaconScan : public Beacon
{
public:
    iBeaconScan() {rssi=rssi_count=0;};
    ~iBeaconScan() = default;

    void populateData(const BleScanResult *scanResult);
    static bool isBeacon(const BleScanResult *scanResult);
    void toJson(JSONBufferWriter *writer);

    BleAddress getAddress() { return address;}
    char* getUuid() {return uuid;};
    uint16_t getMajor() {return major;}
    uint16_t getMinor() {return minor;}
    int8_t getPower() {return power;}
    int8_t getRssi() {return (int8_t)(rssi/rssi_count);}

private:
    BleAddress address;
    char uuid[37];
    uint16_t major;
    uint16_t minor;
    int8_t power;
    int16_t rssi;
    uint8_t rssi_count;
};

#endif