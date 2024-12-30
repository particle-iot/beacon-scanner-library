/*
 * Copyright (c) 2024 Particle Industries, Inc.
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
 * contributed by Gustavo Gonnet (gusgonnet@gmail.com) 2024 in order to use ThermoPro BLE temperature sensors
 * with Particle devices.
 */

#ifndef THERMOPRO_H
#define THERMOPRO_H

#include "beacon.h"

class ThermoPro : public Beacon
{
public:
    ThermoPro() : Beacon(SCAN_THERMOPRO) {};
    ~ThermoPro() = default;

    void toJson(JSONWriter *writer) const override;

    int getBatteryLevel() const { return batteryLevel; }
    float getTemperatureC() const { return temperature; }                // celsius
    float getTemperatureF() const { return (temperature * 9 / 5 + 32); } // fahrenheit
    int getHumidity() const { return humidity; }

private:
    int batteryLevel = 0;
    float temperature = 0;
    int humidity = 0;

    friend class Beaconscanner;
    static Vector<ThermoPro> beacons;
    void populateData(const BleScanResult *scanResult) override;
    static bool isBeacon(const BleScanResult *scanResult);
    static void addOrUpdate(const BleScanResult *scanResult);
    bool parseThermoProAdvertisement(const uint8_t *buf, size_t len);
};

#endif