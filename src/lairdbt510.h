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

#ifndef LAIRD_BT510_H
#define LAIRD_BT510_H

#include "beacon.h"

class LairdBt510;
class LairdBt510Config;
class JSONVectorWriter;

class LairdBt510Config {
public:
    LairdBt510Config();
    ~LairdBt510Config() = default;
    /**
     * Set the passkey for pairing. Default is 123456.
     * It must be 6 numerical characters.
     */
    LairdBt510Config& currentPasskey(const char* passkey);
    /**
     * Set the name of the sensor. 23 characters max
     */
    LairdBt510Config& sensorName(const char* name);
    /**
     * Set the interval at which the sensor reads the
     * temperature.
     * @param seconds valid value between 0 and 86400
     */
    LairdBt510Config& tempSenseInterval(uint32_t seconds);
    /**
     * Set the interval at which the sensor reads the
     * battery voltage.
     * @param seconds valid value between 0 and 86400
     */
    LairdBt510Config& battSenseInterval(uint32_t seconds);
    /**
     * Set the threshold above which Alarm1 will be triggered
     * @param celsius between -128 and 127
     */
    LairdBt510Config& highTempAlarm1(int8_t celsius);
    /**
     * Set the threshold above which Alarm2 will be triggered.
     * This value must be higher than Alarm1 threshold, as 
     * that one is checked first.
     * @param celsius between -128 and 127
     */
    LairdBt510Config& highTempAlarm2(int8_t celsius);
    /**
     * Set the threshold below which Alarm1 will be triggered
     * @param celsius between -128 and 127
     */
    LairdBt510Config& lowTempAlarm1(int8_t celsius);
    /**
     * Set the threshold below which Alarm2 will be triggered.
     * This value must be lower than Alarm2 threshold, as
     * that one is checked first.
     * @param celsius between -128 and 127
     */
    LairdBt510Config& lowTempAlarm2(int8_t celsius);
    LairdBt510Config& deltaTempAlarm(uint8_t celsius);
    /**
     * When greater than 1, the temperature samples are averaged
     * @param count between 1 and 32
     */
    LairdBt510Config& tempAggregationCount(uint8_t count);
    /**
     * The interval between advertising packets.
     * @param ms between 20 and 10000, default is 1000
     */
    LairdBt510Config& advertisementInterval(uint16_t ms);
    /**
     * The BLE connection timeout. 0 means none
     * @param sec seconds for timeout, between 0 and 10000
     */
    LairdBt510Config& connectionTimeout(uint16_t sec);
    /**
     * The new passkey to program into the device.
     */
    LairdBt510Config& newPasskey(const char* passkey);
    /**
     * Use Coded PHY (Bluetooth 5.0 long range)
     */
    LairdBt510Config& useCodedPhy(bool coded);
    void createJson(JSONVectorWriter& writer, uint16_t& configId) const;
protected:
    Vector<char> name_, location_;
    uint32_t tempSenseInterval_, battSenseInterval_;
    uint16_t advInterval_, connTimeout_;
    uint8_t tempAggregationCount_, deltaTempAlarm_, configFlags_, coded_;
    int8_t  highTempAlarm1_, highTempAlarm2_, lowTempAlarm1_, lowTempAlarm2_;
    enum Bt510ConfigFields: uint8_t {
        NONE                      = 0,
        ConfigHighTempAlarm1      = 0x01,
        ConfigHighTempAlarm2      = 0x02,
        ConfigLowTempAlarm1       = 0x04,
        ConfigLowTempAlarm2       = 0x08,
        ConfigDeltaTempAlarm      = 0x10,
        ConfigNetworkId           = 0x20,
        ConfigNewPasskey          = 0x40
    };
    friend class LairdBt510;
    uint8_t passkey_[6], newPasskey_[6];
};

enum class lairdbt510_event_type {
    TEMPERATURE         = 1,
    MAGNET_PROXIMITY    = 2,
    MOVEMENT            = 3,
    ALARM_HIGH_TEMP_1   = 4,
    ALARM_HIGH_TEMP_2   = 5,
    ALARM_HIGH_TEMP_CLEAR = 6,
    ALARM_LOW_TEMP_1    = 7,
    ALARM_LOW_TEMP_2    = 8,
    ALARM_LOW_TEMP_CLEAR = 9,
    ALARM_DELTA_TEMP    = 10,
    BATTERY_GOOD        = 12,
    ADVERTISE_ON_BUTTON = 13,
    BATTERY_BAD         = 16,
    RESET               = 17
};

enum class lairdbt510_flags {
    RTC                 = 0x01,
    ACTIVE              = 0x02,
    ANY_FLAG_SET        = 0x04,
    LOW_BATTERY_ALARM   = 0x80,
    HIGH_TEMP_ALARM_0   = 0x100,
    HIGH_TEMP_ALARM_1   = 0x200,
    LOW_TEMP_ALARM_0    = 0x400,
    LOW_TEMP_ALARM_1    = 0x800,
    DELTA_TEMP_ALARM    = 0x1000,
    MOVEMENT_ALARM      = 0x4000,
    MAGNET_STATE        = 0x8000
};

typedef void (*LairdBt510EventCallback)(LairdBt510& beacon, lairdbt510_event_type evt);

class LairdBt510 : public Beacon
{
public:
    LairdBt510() : 
        Beacon(SCAN_LAIRDBT510),
        state_(IDLE),
        prev_state_(IDLE),
        configId_(0)
        { };
    ~LairdBt510() = default;

    void toJson(JSONWriter *writer) const override;

    // Register callbacks for events and alarms
    static void setEventCallback(LairdBt510EventCallback callback) { LairdBt510::_eventCallback = callback; };
    static void setAlarmCallback(LairdBt510EventCallback callback) { LairdBt510::_alarmCallback = callback; };

    // Get the sensor data
    int16_t getTemperature() const { return _temp; };
    bool magnetNear() const {return !_magnet_state;};
    uint16_t getRecordNumber() const { return _record_number; };
    uint16_t getBattVoltage() const { return _batt_voltage; };
    const char* getName() const { return _name.data(); };

    // Configure the device
    particle::Future<bool> configure(LairdBt510Config config);

private:
    void* handler_data_;
    friend class Beaconscanner;
    void loop();
    static bool isBeacon(const BleScanResult *scanResult);
    void populateData(const BleScanResult *scanResult) override;
    static Vector<LairdBt510> beacons;
    static void addOrUpdate(const BleScanResult *scanResult);
    int16_t _temp;
    uint16_t _record_number, _batt_voltage;
    bool _magnet_event, _magnet_state, _movement;
    static LairdBt510EventCallback _eventCallback, _alarmCallback;
    Vector<char> _name;
    static void onDataReceived(const uint8_t* data, size_t size, const BlePeerDevice& peer, void* context);
    static void onPairingEvent(const BlePairingEvent& event);
    static void onDisconnected(const BlePeerDevice& peer);
    enum State: uint8_t {
        IDLE, CONNECTING, PAIRING, SENDING, DISCONNECT, RECEIVING, CLEANUP
    } state_, prev_state_;
    BlePeerDevice peer_;
    BleCharacteristic tx, rx;
    LairdBt510Config config_;
    uint16_t configId_;
};

#endif