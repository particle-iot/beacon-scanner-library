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

#include "ruuvi.h"

Vector<Ruuvi> Ruuvi::beacons;
#define MAX_MANUFACTURER_DATA_LEN 37

void Ruuvi::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);

    uint8_t buf[MAX_MANUFACTURER_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, MAX_MANUFACTURER_DATA_LEN);

    if (!parseRuuviAdvertisement(buf, count))
    {
        Log.error("Ruuvi: advertisement parsing failed");
    }
}

bool Ruuvi::isBeacon(const BleScanResult *scanResult)
{
    uint8_t buf[MAX_MANUFACTURER_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, MAX_MANUFACTURER_DATA_LEN);

    if (count > 3 && buf[0] == 0x99 && buf[1] == 0x04) // Ruuvi UUID is 0x9904
    {
        char hexString[MAX_MANUFACTURER_DATA_LEN * 2 + 1] = {0}; // Each byte -> 2 hex chars, +1 for null terminator
        for (size_t i = 0; i < count; i++)
        {
            char hex[3];
            snprintf(hex, sizeof(hex), "%02x", buf[i]);
            strcat(hexString, hex);
        }
        Log.trace("Ruuvi sensor found at %s (%s)", ADDRESS(scanResult).toString().c_str(), hexString);
        return true;
    }
    return false;
}

void Ruuvi::toJson(JSONWriter *writer) const
{
    writer->name(address.toString()).beginObject();
    writer->name("temperature").value(getTemperature());
    writer->endObject();
}

void Ruuvi::addOrUpdate(const BleScanResult *scanResult)
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
        Ruuvi new_beacon;
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        beacons.append(new_beacon);
    }
    else
    {
        Ruuvi &beacon = beacons.at(i);
        beacon.newly_scanned = false;
        beacon.populateData(scanResult);
        beacon.missed_scan = 0;
    }
}

// The Ruuvi data is in this format:
// https://docs.ruuvi.com/communication/bluetooth-advertisements/data-format-5-rawv2
// example: 99040516142d8fc4c7fc9401c8fffc8ed6f6cba8fc4c2295c482
bool Ruuvi::parseRuuviAdvertisement(const uint8_t *buf, size_t len)
{
    if (len < 26)
    {
        Log.info("len is less than 26, too short for a Ruuvi sensor, skipping");
        return false;
    }

    int index = 0;

    // Manufacturer ID, least significant byte first: 0x0499 = Ruuvi Innovations Ltd
    uint16_t manufacturerId = (buf[index + 1] << 8) | buf[index];
    if (manufacturerId != 0x0499)
    {
        Log.info("manufacturer ID is not 0x0499 - Ruuvi Innovations Ltd, skipping");
        return false;
    }
    index += 2;

    // Data format (8bit)
    uint8_t format = buf[index];
    index++;

    if (format != 5)
    {
        Log.info("Data format is not 5, skipping");
        return false;
    }

    // Temperature in 0.005 degrees Celsius
    int16_t rawTemperature = (buf[index] << 8) | buf[index + 1];
    temperature = rawTemperature * 0.005;
    index += 2;

    // Humidity (16bit unsigned) in 0.0025% (0-163.83% range, though realistically 0-100%)
    uint16_t rawHumidity = (buf[index] << 8) | buf[index + 1];
    humidity = rawHumidity * 0.0025;
    index += 2;

    // Pressure (16bit unsigned) in 1 Pa units, with offset of -50 000 Pa
    uint16_t rawPressure = (buf[index] << 8) | buf[index + 1];
    pressure = rawPressure + 50000;
    index += 2;

    // Acceleration-X (Most Significant Byte first)
    int16_t rawAccelerationX = (buf[index] << 8) | buf[index + 1];
    accelerationX = rawAccelerationX / 1000.0;
    index += 2;

    // Acceleration-Y (Most Significant Byte first)
    int16_t rawAccelerationY = (buf[index] << 8) | buf[index + 1];
    accelerationY = rawAccelerationY / 1000.0;
    index += 2;

    // Acceleration-Z (Most Significant Byte first)
    int16_t rawAccelerationZ = (buf[index] << 8) | buf[index + 1];
    accelerationZ = rawAccelerationZ / 1000.0;
    index += 2;

    // Power info (11+5bit unsigned)
    uint16_t rawPowerInfo = (buf[index] << 8) | buf[index + 1];
    batteryVoltage = ((rawPowerInfo >> 5) + 1600) / 1000.0;
    txPower = (rawPowerInfo & 0x1F) * 2 - 40;
    index += 2;

    // Movement counter (8 bit unsigned)
    movementCounter = buf[index];
    index++;

    // Measurement sequence number (16 bit unsigned)
    measurementSequenceNumber = (buf[index] << 8) | buf[index + 1];
    index += 2;

    // MAC address (48bit)
    snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
             buf[index], buf[index + 1], buf[index + 2], buf[index + 3], buf[index + 4], buf[index + 5]);
    index += 6;

    // Log the parsed data
    Log.trace("T: %.2f, Humidity: %.2f, Pressure: %.2f, Accel: [%.3f, %.3f, %.3f] g, Batt: %.3f, TXPower: %d dBm, Move: %d, Seq: %d, MAC: %s",
             temperature, humidity, pressure, accelerationX, accelerationY, accelerationZ, batteryVoltage, txPower, movementCounter, measurementSequenceNumber, mac);

    return true;
}
