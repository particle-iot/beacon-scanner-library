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

#include "bthome.h"

Vector<BTHome> BTHome::beacons;

void BTHome::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);
    uint8_t custom_data[BLE_MAX_ADV_DATA_LEN];
    ADVERTISING_DATA(scanResult).customData(custom_data, sizeof(custom_data));

    // init data to invalid values
    packetId = -1;
    battery = -1;
    buttonEvent = -1;
    windowState = -1;
    rotation = -1;
    illuminance = -1;
    
    battery = (int)custom_data[24];
}

bool BTHome::isBeacon(const BleScanResult *scanResult)
{
    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::SERVICE_DATA, buf, BLE_MAX_ADV_DATA_LEN);
    if (count > 3 && buf[0] == 0xD2 && buf[1] == 0xFC) // BTHome UUID is 0xFCD2
        return true;
    return false;
}

void BTHome::toJson(JSONWriter *writer) const
{
    writer->name(address.toString()).beginObject();
    writer->name("battery").value(getBattery());
    writer->endObject();
}

void BTHome::addOrUpdate(const BleScanResult *scanResult)
{
    uint8_t i;
    for (i = 0; i < beacons.size(); ++i)
    {
        if (beacons.at(i).getAddress() == ADDRESS(scanResult))
        {
            break;
        }
    }
    if (i == beacons.size())
    {
        BTHome new_beacon;
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        beacons.append(new_beacon);
    }
    else
    {
        BTHome &beacon = beacons.at(i);
        beacon.newly_scanned = false;
        beacon.populateData(scanResult);
        beacon.missed_scan = 0;
    }
}