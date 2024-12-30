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

#include "ThermoPro.h"

Vector<ThermoPro> ThermoPro::beacons;
#define MAX_MANUFACTURER_DATA_LEN 37

void ThermoPro::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);

    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, BLE_MAX_ADV_DATA_LEN);

    if (!parseThermoProAdvertisement(buf, count))
    {
        Log.error("ThermoPro: advertisement parsing failed");
    }
}

bool ThermoPro::isBeacon(const BleScanResult *scanResult)
{
    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, BLE_MAX_ADV_DATA_LEN);

    String name = scanResult->advertisingData().deviceName();

    if (name.length() >= 5)
    {
        String prefix = name.substring(0, 5); // Extract the first 5 characters
        if (prefix == "TP357" || prefix == "TP359")
        {
            Log.trace("ThermoPro sensor found");
            return true;
        }
    }
    return false;
}

void ThermoPro::toJson(JSONWriter *writer) const
{
    writer->name(address.toString()).beginObject();
    writer->name("temperatureCelsius").value(getTemperatureC());
    writer->name("temperatureFahrenheit").value(getTemperatureF());
    writer->name("humidity").value(getHumidity());
    writer->endObject();
}

void ThermoPro::addOrUpdate(const BleScanResult *scanResult)
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
        ThermoPro new_beacon;
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        beacons.append(new_beacon);
    }
    else
    {
        ThermoPro &beacon = beacons.at(i);
        beacon.newly_scanned = false;
        beacon.populateData(scanResult);
        beacon.missed_scan = 0;
    }
}

// parses the ThermoPro Data format
// example - Advertising Data: C2 CC 00 2A 22 0B 01
// CC 00 is the temperature in Celsius (little endian) (one needs to divide by 10)
// 2a is the humidity
bool ThermoPro::parseThermoProAdvertisement(const uint8_t *buf, size_t len)
{

    // Ensure the buffer has enough data for temperature and humidity
    // At least 4 bytes needed to parse temp and humidity
    if (len < 3)
    {
        Log.warn("Error parsing advertising data for the device.");
        return false;
    }

    // Parse temperature (bytes 1 and 2, little-endian)
    int16_t rawTemp = (buf[2] << 8) | buf[1]; // Combine bytes (little-endian)
    temperature = rawTemp / 10.0;      // Convert to Celsius

    // Parse humidity (byte 3)
    humidity = buf[3]; // Unsigned integer directly represents percentage

    // Log the parsed data
    Log.trace("Parsed ThermoPro Advertisement: Temperature: %.1f C / %.1f F, Humidity: %u%%", temperature, (temperature * 9 / 5 + 32), humidity);

    return true;
}
