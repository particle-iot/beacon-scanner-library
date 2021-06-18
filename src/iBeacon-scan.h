
#ifndef IBEACON_SCAN_H
#define IBEACON_SCAN_H

#include "beacon.h"

class iBeaconScan : public Beacon
{
public:
    iBeaconScan() : Beacon(SCAN_IBEACON) {};
    ~iBeaconScan() = default;

    void toJson(JSONWriter *writer) const override;

    const char* getUuid() const {return uuid;};
    uint16_t getMajor() const {return major;}
    uint16_t getMinor() const {return minor;}
    int8_t getPower() const {return power;}

private:
    char uuid[37];
    uint16_t major;
    uint16_t minor;
    int8_t power;
    friend class Beaconscanner;
    static Vector<iBeaconScan> beacons;
    void populateData(const BleScanResult *scanResult) override;
    static bool isBeacon(const BleScanResult *scanResult);
    static void addOrUpdate(const BleScanResult *scanResult);
};

#endif