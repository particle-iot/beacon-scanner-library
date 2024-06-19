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
 * Find BTHome BLE devices at https://www.bthome.com/en-us
 * contributed by @gusgonnet (gusgonnet@gmail.com) 2024 in order to use Shelly BLE devices with Particle devices.
 * https://kb.shelly.cloud/knowledge-base/shelly-ble-devices
 */

#ifndef BTHOME_H
#define BTHOME_H

#include "beacon.h"

class BTHome : public Beacon
{
public:
    BTHome() : Beacon(SCAN_BTHOME){};
    ~BTHome() = default;

    void toJson(JSONWriter *writer) const override;

    int getPacketId() const { return packetId; }
    int getBatteryLevel() const { return batteryLevel; }
    int getButtonEvent() const { return buttonEvent; }
    int getWindowState() const { return windowState; }
    int getRotation() const { return rotation; }
    int getIlluminance() const { return illuminance; }

private:
    int packetId = 0;
    int batteryLevel = 0;
    int buttonEvent = 0;
    int windowState = 0;
    int rotation = 0;
    int illuminance = 0;

    friend class Beaconscanner;
    static Vector<BTHome> beacons;
    void populateData(const BleScanResult *scanResult) override;
    static bool isBeacon(const BleScanResult *scanResult);
    static void addOrUpdate(const BleScanResult *scanResult);

    bool parseBTHomeAdvertisement(const uint8_t *buf, size_t len);
    void parseField(uint8_t objectId, const uint8_t *buf, size_t &offset);
    int16_t littleEndianToInt16(const uint8_t *data);
    uint32_t littleEndianToUInt24(const uint8_t *data);
};

#endif