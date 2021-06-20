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