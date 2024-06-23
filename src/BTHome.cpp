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

#include "BTHome.h"

Vector<BTHome> BTHome::beacons;
#define MAX_MANUFACTURER_DATA_LEN 37

void BTHome::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);

    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::SERVICE_DATA, buf, BLE_MAX_ADV_DATA_LEN);

    if (!parseBTHomeAdvertisement(buf, count))
    {
        Log.error("BTHome: advertisement parsing failed");
    }
}

bool BTHome::isBeacon(const BleScanResult *scanResult)
{
    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::SERVICE_DATA, buf, BLE_MAX_ADV_DATA_LEN);

    if (count > 3 && buf[0] == 0xD2 && buf[1] == 0xFC) // BTHome UUID is 0xFCD2
    {
        String hexString = "";
        for (size_t i = 0; i < count; i++)
        {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", buf[i]);
            hexString += hex;
        }
        Log.trace("BTHome sensor found: %s", hexString.c_str());
        return true;
    }
    return false;
}

void BTHome::toJson(JSONWriter *writer) const
{
    writer->name(address.toString()).beginObject();
    writer->name("batteryLevel").value(getBatteryLevel());
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

// parses the BTHome Data format specified at https://bthome.io/format/
// min supported length is 9 bytes
// Example: D2FC44002D01643A01 (here, a button is pressed: 3A is 01)
bool BTHome::parseBTHomeAdvertisement(const uint8_t *buf, size_t len)
{

    if (len < 9)
    {
        return false;
    }

    // first two bytes are the UUID
    if (buf[0] != 0xD2 || buf[1] != 0xFC)
    {
        return false;
    }

    // next byte is the BTHome Device Information (Example: 0x44)
    // Log.info("BTHome Device Information: %02X", buf[2]);

    // now parse the rest of the data
    size_t offset = 3; // start after UUID and device info

    while (offset + 1 < len)
    {
        uint8_t objectId = buf[offset++];
        parseField(objectId, buf, offset);
    }

    // Log the parsed data
    Log.trace("Parsed BTHome Advertisement: Packet ID: %d, Battery Level: %d%%, Illuminance: %.2f lx, Window State: %d, Button Event: %d, Rotation: %.1f",
              packetId, batteryLevel, illuminance * 0.01, windowState, buttonEvent, rotation * 0.1);

    return true;
}

void BTHome::parseField(uint8_t objectId, const uint8_t *buf, size_t &offset)
{
    switch (objectId)
    {

    case 0x00: // packet id (uint8, 1 byte)
        if (offset + 1 <= 31)
        {
            packetId = buf[offset];
            offset += 1;
        }
        break;

    case 0x01: // Measurement: Battery level (uint8, 1 byte)
        if (offset + 1 <= 31)
        {
            batteryLevel = buf[offset];
            offset += 1;
        }
        break;

    case 0x05: // Illuminance (uint24, 0.01)
        if (offset + 3 <= 31)
        {
            illuminance = littleEndianToUInt24(&buf[offset]);
            offset += 3;
        }
        break;

    case 0x2D: // Measurement: Window (uint8, 1 byte)
        if (offset + 1 <= 31)
        {
            windowState = buf[offset];
            offset += 1;
        }
        break;

    case 0x3A: // Event: button
        if (offset + 1 <= 31)
        {
            buttonEvent = buf[offset];
            offset += 1;
        }
        break;

    case 0x3F: // Rotation (sint16, 0.1)
        if (offset + 2 <= 31)
        {
            rotation = littleEndianToInt16(&buf[offset]);
            offset += 2;
        }
        break;

    // Add cases for other measurement types as needed
    default:
        Log.info("parseField() - Unknown measurement type: 0x%02X", objectId);
        return;
    }
}

int16_t BTHome::littleEndianToInt16(const uint8_t *data)
{
    return (int16_t)((data[1] << 8) | data[0]);
}

uint32_t BTHome::littleEndianToUInt24(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16);
}
