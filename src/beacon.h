#ifndef BEACON_H
#define BEACON_H

#include "config.h"
#include "Particle.h"
#include "os-version-macros.h"

typedef enum ble_scanner_config_t {
  SCAN_IBEACON         = 0x01,
  SCAN_KONTAKT         = 0x02,
  SCAN_EDDYSTONE       = 0x04,
  SCAN_LAIRDBT510      = 0x08
} ble_scanner_config_t;

class Beacon {
public:
    int8_t missed_scan;
    BleAddress getAddress() const { return address;}
    int8_t getRssi() const {return (int8_t)(rssi/rssi_count);}
    virtual void toJson(JSONWriter *writer) const {
        writer->name(address.toString()).beginObject();
        writer->endObject();
    };
    virtual void populateData(const BleScanResult *scanResult) {
        rssi += RSSI(scanResult);
        rssi_count++;
        if (rssi_count > 5) {
            rssi = rssi/rssi_count;
            rssi_count = 1;
        }
    };
    bool newly_scanned;
    ble_scanner_config_t type;

    Beacon(ble_scanner_config_t _type) :
        newly_scanned(true),
        type(_type),
        rssi(0), 
        rssi_count(0) {};

protected:
    BleAddress address;
    int16_t rssi;
    uint8_t rssi_count;
};

#endif