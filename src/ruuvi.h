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

/*
* Find Ruuvi devices at https://ruuvi.com/
* contributed by @gusgonnet (gusgonnet@gmail.com) 2024
*/

#ifndef RUUVI_H
#define RUUVI_H

#include "beacon.h"

class Ruuvi : public Beacon
{
public:
    Ruuvi() : Beacon(SCAN_RUUVI){};
    ~Ruuvi() = default;

    void toJson(JSONWriter *writer) const override;

    int getBattery() const { return battery; }

private:
    int packetId = 0;
    int battery = 0;

    friend class Beaconscanner;
    static Vector<Ruuvi> beacons;
    void populateData(const BleScanResult *scanResult) override;
    static bool isBeacon(const BleScanResult *scanResult);
    static void addOrUpdate(const BleScanResult *scanResult);
};

#endif