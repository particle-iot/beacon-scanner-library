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

#ifndef KONTAKT_TAG_H
#define KONTAKT_TAG_H

#include "beacon.h"

class KontaktTag : public Beacon
{
public:
    KontaktTag() : Beacon(SCAN_KONTAKT)
    {
        battery = temperature = 0xFF;
        button_time = accel_last_double_tap = accel_last_movement = 0xFFFF;
        accel_data = false;
    };
    ~KontaktTag() = default;

    void toJson(JSONWriter *writer) const override;

    uint8_t getBattery() const { return battery; };
    int8_t getTemperature() const { return temperature; };
    uint8_t getAccelSensitivity() const { return accel_sensitivity; };
    uint16_t getButtonTime() const { return button_time; };
    uint16_t getAccelLastDoubleTap() const { return accel_last_double_tap; };
    uint16_t getAccelLastMovement() const { return accel_last_movement; };
    bool hasAccelData() const { return accel_data; };
    int8_t getAccelXaxis() const { return x_axis; };
    int8_t getAccelYaxis() const { return y_axis; };
    int8_t getAccelZaxis() const { return z_axis; };

private:
    uint8_t battery, accel_sensitivity;
    uint16_t button_time, accel_last_double_tap, accel_last_movement;
    int8_t x_axis, y_axis, z_axis, temperature;
    bool accel_data;
    friend class Beaconscanner;
    static Vector<KontaktTag> beacons;
    static bool isTag(const BleScanResult *scanResult);
    void populateData(const BleScanResult *scanResult) override;
    static void addOrUpdate(const BleScanResult *scanResult);
};

#endif