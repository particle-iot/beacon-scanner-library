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
 * contributed by Gustavo Gonnet (gusgonnet@gmail.com) 2024 in order to use Goovee BLE temperature sensors
 * with Particle devices.
 */

#ifndef GOOVEE_H
#define GOOVEE_H

#include "beacon.h"

class Goovee : public Beacon
{
public:
    Goovee() : Beacon(SCAN_GOOVEE) {};
    ~Goovee() = default;

    void toJson(JSONWriter *writer) const override;

    int getBatteryLevel() const { return batteryLevel; }
    float getTemperatureC() const { return temperature; }                // celsius
    float getTemperatureF() const { return (temperature * 9 / 5 + 32); } // fahrenheit
    float getHumidity() const { return humidity; }

private:
    int batteryLevel = 0;
    float temperature = 0;
    float humidity = 0;

    friend class Beaconscanner;
    static Vector<Goovee> beacons;
    void populateData(const BleScanResult *scanResult) override;
    static bool isBeacon(const BleScanResult *scanResult);
    static void addOrUpdate(const BleScanResult *scanResult);
    bool parseGooveeAdvertisement(const uint8_t *buf, size_t len);
    void decode_temp_humid(uint8_t temp_humid_bytes[3], float *temperature, float *humidity);
};

#endif