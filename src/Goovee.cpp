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

#include "Goovee.h"

Vector<Goovee> Goovee::beacons;
#define MAX_MANUFACTURER_DATA_LEN 37

void Goovee::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);

    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, BLE_MAX_ADV_DATA_LEN);

    if (!parseGooveeAdvertisement(buf, count))
    {
        Log.error("Goovee: advertisement parsing failed");
    }
}

bool Goovee::isBeacon(const BleScanResult *scanResult)
{
    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, BLE_MAX_ADV_DATA_LEN);

    String name = scanResult->advertisingData().deviceName();

    if (name.length() >= 7)
    {
        String prefix = name.substring(0, 7); // Extract the first 7 characters
        if (prefix == "GVH5105")
        {
            Log.trace("Goovee sensor found");
            return true;
        }
    }
    return false;
}

void Goovee::toJson(JSONWriter *writer) const
{
    writer->name(address.toString()).beginObject();
    writer->name("temperatureCelsius").value(getTemperatureC());
    writer->name("temperatureFahrenheit").value(getTemperatureF());
    writer->name("humidity").value(getHumidity());
    writer->endObject();
}

void Goovee::addOrUpdate(const BleScanResult *scanResult)
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
        Goovee new_beacon;
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        beacons.append(new_beacon);
    }
    else
    {
        Goovee &beacon = beacons.at(i);
        beacon.newly_scanned = false;
        beacon.populateData(scanResult);
        beacon.missed_scan = 0;
    }
}

// parses the Goovee Data format
// example - Advertising Data: C2 CC 00 2A 22 0B 01
// CC 00 is the temperature in Celsius (little endian) (one needs to divide by 10)
// 2a is the humidity
bool Goovee::parseGooveeAdvertisement(const uint8_t *buf, size_t len)
{
    // Ensure the buffer has enough data for temperature and humidity
    // At least 8 bytes needed to parse temp and humidity
    // 01 00 01 01 03 22 8C 64
    if (len == 8)
    {
        uint8_t temp_humid_bytes[3] = {buf[4], buf[5], buf[6]};
        temperature = 0.0;
        humidity = 0.0;

        // Decode temperature and humidity
        decode_temp_humid(temp_humid_bytes, &temperature, &humidity);

        batteryLevel = buf[7] & 0x7F;

        Log.trace("Parsed Goovee Advertisement: Temperature: %.1f C / %.1f F, Humidity: %.1f%%, batteryLevel: %d%%", temperature, (temperature * 9 / 5 + 32), humidity, batteryLevel);
        return true;
    }
    else
    {
        Log.error("Invalid advertisement data length: %d", len);
        return false;
    }
}

void Goovee::decode_temp_humid(uint8_t temp_humid_bytes[3], float *temperature, float *humidity)
{
    // Combine the 3 bytes into a 24-bit number
    uint32_t base_num = (temp_humid_bytes[0] << 16) | (temp_humid_bytes[1] << 8) | temp_humid_bytes[2];

    bool is_negative = base_num & 0x800000;
    uint32_t temp_as_int = base_num & 0x7FFFFF;

    // Calculate the temperature as a float
    *temperature = (temp_as_int / 1000.0) / 10.0;
    if (is_negative)
    {
        *temperature = -*temperature;
    }

    // Calculate the humidity
    *humidity = (temp_as_int % 1000) / 10.0;
}
