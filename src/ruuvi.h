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
 * Find Ruuvi devices at https://ruuvi.com/
 * contributed by @gusgonnet (gusgonnet@gmail.com) 2024
 */

#ifndef RUUVI_H
#define RUUVI_H

#include "beacon.h"

class Ruuvi : public Beacon
{
public:
    Ruuvi() : Beacon(SCAN_RUUVI){};
    ~Ruuvi() = default;

    void toJson(JSONWriter *writer) const override;

    float getTemperature() const { return temperature; }
    float getHumidity() const { return humidity; }
    float getPressure() const { return pressure; }
    float getAccelerationX() const { return accelerationX; }
    float getAccelerationY() const { return accelerationY; }
    float getAccelerationZ() const { return accelerationZ; }
    float getBatteryVoltage() const { return batteryVoltage; }
    float getTxPower() const { return txPower; }
    int getMovementCounter() const { return movementCounter; }
    int getMeasurementSequenceNumber() const { return measurementSequenceNumber; }
    const char *getMac() const { return mac; }

private:
    uint8_t format;                     // byte 0: Data format (8bit)
    float temperature;                  // bytes 1-2: Temperature in 0.005 degrees Celsius
    float humidity;                     // bytes 3-4: Humidity (16bit unsigned) in 0.0025% (0-163.83% range, though realistically 0-100%)
    float pressure;                     // bytes 5-6: Pressure (16bit unsigned) in 1 Pa units, with offset of -50 000 Pa
    float accelerationX;                // bytes 7-8: Acceleration-X (Most Significant Byte first)
    float accelerationY;                // bytes 9-10: Acceleration-Y (Most Significant Byte first)
    float accelerationZ;                // bytes 11-12: Acceleration-Z (Most Significant Byte first)
    float batteryVoltage;               // bytes 13-14: Power info (11+5bit unsigned), first 11 bits is the battery voltage above 1.6V, in millivolts (1.6V to 3.646V range).
    int8_t txPower;                      // bytes 13-14: Last 5 bits unsigned are the TX power above -40dBm, in 2dBm steps. (-40dBm to +20dBm range)
    uint8_t movementCounter;           // byte 15: Movement counter (8 bit unsigned), incremented by motion detection interrupts from accelerometer
    uint16_t measurementSequenceNumber; // bytes 16-17: Measurement sequence number (16 bit unsigned), each time a measurement is taken, this is incremented by one, used for measurement de-duplication. Depending on the transmit interval, multiple packets with the same measurements can be sent, and there may be measurements that never were sent.
    char mac[18];                       // bytes 18-23: MAC address (48bit)

    friend class Beaconscanner;
    static Vector<Ruuvi> beacons;
    void populateData(const BleScanResult *scanResult) override;
    static bool isBeacon(const BleScanResult *scanResult);
    static void addOrUpdate(const BleScanResult *scanResult);
    bool parseRuuviAdvertisement(const uint8_t *buf, size_t len);
};

#endif