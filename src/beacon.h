#ifndef BEACON_H
#define BEACON_H

#include "Particle.h"

typedef enum ble_scanner_config_t {
  SCAN_IBEACON         = 0x01,
  SCAN_KONTAKT         = 0x02,
  SCAN_EDDYSTONE       = 0x04
} ble_scanner_config_t;

class Beacon {
public:
    int8_t missed_scan;
    BleAddress getAddress() const { return address;}
    virtual void toJson(JSONWriter *writer) const {
        writer->name(address.toString()).beginObject();
        writer->endObject();
    };
    bool newly_scanned;
    ble_scanner_config_t type;

    Beacon(ble_scanner_config_t _type) :
        newly_scanned(true),
        type(_type) {};

protected:
    BleAddress address;
};

#endif