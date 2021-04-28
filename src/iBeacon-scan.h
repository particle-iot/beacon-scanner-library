
#ifndef IBEACON_SCAN_H
#define IBEACON_SCAN_H

#include "beacon.h"

class iBeaconScan : public Beacon
{
public:
    iBeaconScan() : Beacon(SCAN_IBEACON), rssi(0), rssi_count(0) {};
    ~iBeaconScan() = default;

    void populateData(const BleScanResult *scanResult);
    static bool isBeacon(const BleScanResult *scanResult);
    void toJson(JSONWriter *writer) const override;

    const char* getUuid() const {return uuid;};
    uint16_t getMajor() const {return major;}
    uint16_t getMinor() const {return minor;}
    int8_t getPower() const {return power;}
    int8_t getRssi() const {return (int8_t)(rssi/rssi_count);}

private:
    char uuid[37];
    uint16_t major;
    uint16_t minor;
    int8_t power;
    int16_t rssi;
    uint8_t rssi_count;
};

#endif