#ifndef BEACON_H
#define BEACON_H

#include "Particle.h"

class Beacon {
public:
    uint8_t missed_scan;
    BleAddress getAddress() const { return address;}
    virtual void toJson(JSONWriter *writer) const {
        writer->name(address.toString()).beginObject();
        writer->endObject();
    };

protected:
    BleAddress address;
};

#endif