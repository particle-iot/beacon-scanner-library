#ifndef SGWIRELESS_TAG_H
#define SGWIRELESS_TAG_H

#include "beacon.h"

class SGWirelessTag : public Beacon
{
public:
    SGWirelessTag() :
        Beacon(SCAN_SGWIRELESS),
        battery(0xFF), 
        humidity_f(0.0f),
        temperature_f(0.0f),
        humidity_raw(0xFFFF),
        temperature_raw(0xFFFF)
    {};
    ~SGWirelessTag() = default;

    static bool isTag(const BleScanResult *scanResult);
    void toJson(JSONWriter *writer);

    const char* getName() const { return name; };
    int8_t getRssi() const { return rssi; };
    uint8_t getBattery() const { return battery; };
    float getTemperature() const { return temperature_f; };
    float getHumidity() const { return humidity_f; };

private:
    uint8_t battery;
    int8_t rssi;

    char name[8];
    float humidity_f, temperature_f;
    int16_t humidity_raw, temperature_raw;
    
    friend class Beaconscanner;
    static Vector<SGWirelessTag> beacons;
    void populateData(const BleScanResult *scanResult) override;
    static bool isBeacon(const BleScanResult *scanResult);
    static void addOrUpdate(const BleScanResult *scanResult);
};

#endif