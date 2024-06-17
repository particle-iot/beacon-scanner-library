/*
 * Copyright (c) 2020 Particle Industries, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BEACON_H
#define BEACON_H

#include "config.h"
#include "Particle.h"
#include "os-version-macros.h"

typedef enum ble_scanner_config_t {
  SCAN_IBEACON         = 0x01,
  SCAN_KONTAKT         = 0x02,
  SCAN_EDDYSTONE       = 0x04,
  SCAN_LAIRDBT510      = 0x08,
  SCAN_BTHOME          = 0x10,
  SCAN_RUUVI           = 0x20
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
    virtual void populateData(const BleScanResult *scanResult) {
        rssi += RSSI(scanResult);
        rssi_count++;
        if (rssi_count > 5) {
            rssi = rssi/rssi_count;
            rssi_count = 1;
        }
    };
};

#endif