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

#include "iBeacon-scan.h"

Vector<iBeaconScan> iBeaconScan::beacons;

void iBeaconScan::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);
    uint8_t custom_data[BLE_MAX_ADV_DATA_LEN];
    ADVERTISING_DATA(scanResult).customData(custom_data, sizeof(custom_data));
    snprintf(uuid, sizeof(uuid), "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                custom_data[4], custom_data[5], custom_data[6], custom_data[7], custom_data[8], custom_data[9], custom_data[10], custom_data[11], custom_data[12],
                custom_data[13], custom_data[14], custom_data[15], custom_data[16], custom_data[17], custom_data[18], custom_data[19]);
    major = custom_data[20] * 256 + custom_data[21];
    minor = custom_data[22] * 256 + custom_data[23];
    power = (int8_t)custom_data[24];
}

bool iBeaconScan::isBeacon(const BleScanResult *scanResult)
{
    uint8_t custom_data[BLE_MAX_ADV_DATA_LEN];

    if (ADVERTISING_DATA(scanResult).customData(custom_data, sizeof(custom_data)) == 25)
    {
        if (custom_data[0] == 0x4c && custom_data[1] == 0x00 && custom_data[2] == 0x02 && custom_data[3] == 0x15)
        {
            return true;
        }
    }
    return false;
}

void iBeaconScan::toJson(JSONWriter *writer) const
{
        writer->name(address.toString()).beginObject();
        writer->name("uuid").value(getUuid());
        writer->name("major").value(getMajor());
        writer->name("minor").value(getMinor());
        writer->name("power").value(getPower());
        writer->name("rssi").value(getRssi());
        writer->endObject();
}

void iBeaconScan::addOrUpdate(const BleScanResult *scanResult)
{
    uint8_t i;
    for (i = 0; i < beacons.size(); ++i) {
        if (beacons.at(i).getAddress() == ADDRESS(scanResult)) {
            break;
        }
    }
    if (i == beacons.size()) {
        iBeaconScan new_beacon;
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        beacons.append(new_beacon);
    } else {
        iBeaconScan& beacon = beacons.at(i);
        beacon.newly_scanned = false;
        beacon.populateData(scanResult);
        beacon.missed_scan = 0;
    }
}