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

    if (parseRuuviAdvertisement(buf, count))
    {
        Log.info("temperature=%.2f humidity=%.2f pressure=%.2f", temperature, humidity, pressure);
        Log.info("format=%u accelX=%.2f accelY=%.2f accelZ=%.2f battery=%.2f txPower=%.2f movements=%u sequence=%lu mac=%s", format, accelerationX, accelerationY, accelerationZ, batteryVoltage, txPower, movementCounter, measurementSequenceNumber, mac);
    }
    else
    {
        Log.error("Parsing of the Ruuvi sensor advertisement failed");
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

    // Payload data
    // example: 0516142d8fc4c7fc9401c8fffc8ed6f6cba8fc4c2295c482
    // 05 Data format (8bit)
    format = buf[index];
    index++;

    // next 1-2 bytes are the Temperature in 0.005 degrees Celsius
    temperature = (buf[index] << 8 | buf[index + 1]) * 0.005;
    index += 2;

    // next 2 bytes are the Humidity (16bit unsigned) in 0.0025% (0-163.83% range, though realistically 0-100%)
    humidity = (buf[index] << 8 | buf[index + 1]) * 0.0025;
    index += 2;

    // next 2 bytes are the Pressure (16bit unsigned) in 1 Pa units, with offset of -50 000 Pa
    pressure = ((buf[index] << 8 | buf[index]) + 50000);
    index += 2;

    // next 2 bytes are the Acceleration-X (Most Significant Byte first)
    accelerationX = (buf[index] << 8 | buf[index + 1]);
    index += 2;

    // next 2 bytes are the Acceleration-Y (Most Significant Byte first)
    accelerationY = (buf[index] << 8 | buf[index + 1]);
    index += 2;

    // next 2 bytes are the Acceleration-Z (Most Significant Byte first)
    accelerationZ = (buf[index] << 8 | buf[index + 1]);
    index += 2;

    // next 2 bytes are the Power info (11+5bit unsigned), first 11 bits is the battery voltage above 1.6V, in millivolts (1.6V to 3.646V range).
    // Last 5 bits unsigned are the TX power above -40dBm, in 2dBm steps. (-40dBm to +20dBm range)
    batteryVoltage = (((buf[index] << 8 | buf[index + 1]) >> 5) + 1600) / 1000.0;
    txPower = (buf[index] << 8 | buf[index + 1]) & 0x1F;
    index += 2;

    // next byte is the Movement counter (8 bit unsigned), incremented by motion detection interrupts from accelerometer
    movementCounter = buf[index];
    index++;

    // next 2 bytes are the Measurement sequence number (16 bit unsigned), each time a measurement is taken, this is incremented by one, used for measurement de-duplication. Depending on the transmit interval, multiple packets with the same measurements can be sent, and there may be measurements that never were sent.
    measurementSequenceNumber = (buf[index] << 8 | buf[index + 1]);
    index += 2;

    // next 6 bytes are the MAC address (48bit)
    snprintf(mac, sizeof(mac), "%02x%02x%02x%02x%02x%02x",
             buf[index], buf[index + 1], buf[index + 2], buf[index + 3], buf[index + 4], buf[index + 5]);
    index += 6;

    return true;
}
