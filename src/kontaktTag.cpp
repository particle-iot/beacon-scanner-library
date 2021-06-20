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

#include "kontaktTag.h"

Vector<KontaktTag> KontaktTag::beacons;

void KontaktTag::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);
    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::SERVICE_DATA, buf, sizeof(buf));
    uint8_t cursor = 0;
    if (count > 3 && buf[0] == 0x6A && buf[1] == 0xFE) // Kontakt UUID
    {
        cursor += 2;
        if (buf[cursor] == 0x03) // Telemetry v1 packet
        // Definition is here: https://developer.kontakt.io/hardware/packets/telemetry/
        {
            cursor++;
            while (cursor < count)
            {
                uint8_t size = buf[cursor];
                cursor++;
                switch (buf[cursor++])
                {
                case 0x01:       // System health
                    cursor += 4; // Advance to battery level
                    battery = buf[cursor++];
                    break;
                case 0x02:
                    accel_sensitivity = buf[cursor++];
                    x_axis = buf[cursor++];
                    y_axis = buf[cursor++];
                    z_axis = buf[cursor++];
                    accel_last_double_tap = buf[cursor] + buf[cursor + 1] * 256;
                    cursor += 2;
                    accel_last_movement = buf[cursor] + buf[cursor + 1] * 256;
                    cursor += 2;
                    accel_data = true;
                    break;
                case 0x05: // Light and temp
                    // Light sensor is at cursor
                    cursor++;
                    // Temp in C is at cursor
                    temperature = buf[cursor++];
                    break;
                case 0x0D: // Button press
                    button_time = buf[cursor] + buf[cursor + 1] * 256;
                    cursor += 2;
                    break;
                default:
                    cursor--;
                    char nbuf[100];
                    for (uint8_t i = 0; i < size; i++)
                    {
                        snprintf(nbuf + i * 2, sizeof(nbuf), "%02X", buf[cursor + i]);
                    }
                    nbuf[size * 2] = '\0';
                    Log.info("%s", nbuf);
                    cursor += size;
                }
            }
        }
    }
}

bool KontaktTag::isTag(const BleScanResult *scanResult)
{
    if (ADVERTISING_DATA(scanResult).contains(BleAdvertisingDataType::SERVICE_DATA))
    {
        uint8_t buf[BLE_MAX_ADV_DATA_LEN];
        uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::SERVICE_DATA, buf, BLE_MAX_ADV_DATA_LEN);
        if (count > 3 && buf[0] == 0x6A && buf[1] == 0xFE) // Kontakt UUID
            return true;
    }
    return false;
}

void KontaktTag::toJson(JSONWriter *writer) const
{
        writer->name(address.toString()).beginObject();
        if (battery != 0xFF)
            writer->name("batt").value(battery);
        if (temperature != 0xFF)
            writer->name("temp").value(temperature);
        if (button_time != 0xFFFF)
            writer->name("button").value(button_time);
        if (accel_data)
        {
            writer->name("x_axis").value(x_axis);
            writer->name("y_axis").value(y_axis);
            writer->name("z_axis").value(z_axis);
        }
        writer->name("rssi").value(getRssi());
        writer->endObject();
}

void KontaktTag::addOrUpdate(const BleScanResult *scanResult) {
    uint8_t i;
    for (i = 0; i < beacons.size(); i++)
    {
        if (beacons.at(i).getAddress() == ADDRESS(scanResult))
        {
            break;
        }
    }
    if (i == beacons.size()) {
        KontaktTag new_beacon;
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        beacons.append(new_beacon);
    } else {
        KontaktTag& beacon = beacons.at(i);
        beacon.newly_scanned = false;
        beacon.populateData(scanResult);
        beacon.missed_scan = 0;
    }
}