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

#include "config.h"
#ifdef SUPPORT_LAIRDBT510
#include "lairdbt510.h"

#define RECEIVE_TIMEOUT_LOOPS     20    // Each loop is approximately 1 second
#define MAX_MANUFACTURER_DATA_LEN 37

LairdBt510EventCallback LairdBt510::_eventCallback = nullptr;
LairdBt510EventCallback LairdBt510::_alarmCallback = nullptr;
Vector<LairdBt510> LairdBt510::beacons;

void LairdBt510::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);
    uint8_t buf[MAX_MANUFACTURER_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, MAX_MANUFACTURER_DATA_LEN);
    uint16_t prev_record = _record_number;
    if (count > 25) {   // Advertising data is correct, either table 1 or table 3
        bool prev_magnet = _magnet_state;
        uint16_t flags = buf[7] << 8 | buf[6];
        _magnet_state = (flags & (uint16_t)lairdbt510_flags::MAGNET_STATE);
        _record_number = buf[16] << 8 | buf[15];
        lairdbt510_event_type event = (lairdbt510_event_type)buf[14];
        switch (event)
        {
        case lairdbt510_event_type::TEMPERATURE:
        case lairdbt510_event_type::ALARM_HIGH_TEMP_1:
        case lairdbt510_event_type::ALARM_HIGH_TEMP_2:
        case lairdbt510_event_type::ALARM_HIGH_TEMP_CLEAR:
        case lairdbt510_event_type::ALARM_LOW_TEMP_1:
        case lairdbt510_event_type::ALARM_LOW_TEMP_2:
        case lairdbt510_event_type::ALARM_LOW_TEMP_CLEAR:
        case lairdbt510_event_type::ALARM_DELTA_TEMP:
            _temp = buf[22] << 8 | buf[21];
            break;
        case lairdbt510_event_type::MAGNET_PROXIMITY:
            _magnet_event = buf[21] == 0x01;
            break;
        case lairdbt510_event_type::MOVEMENT:
            break;
        case lairdbt510_event_type::BATTERY_GOOD:
        case lairdbt510_event_type::ADVERTISE_ON_BUTTON:
        case lairdbt510_event_type::BATTERY_BAD:
            _batt_voltage = buf[22] << 8 | buf[21];
            break;
        case lairdbt510_event_type::RESET:
            break;
        default:
            break;
        }
        if (count == 37 && buf[2] == 0x02) { 
            // This is a Coded PHY advertisement, get the rest from the same buffer
            // TODO: Add extraction of firmware, configuration, and bootloader versions
            count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::COMPLETE_LOCAL_NAME, buf, MAX_MANUFACTURER_DATA_LEN);
            if (count > 0 && memcmp(buf, _name.data(), count)) {
                _name.clear();
                _name.append((const char*)buf, count);
                _name.append('\0');
                Log.trace("New device name: %s", _name.data());
            }
        }
        else if (count == 26 && buf[2] == 0x01) { // This is a 1MB PHY advertisement
            count = SCAN_RESPONSE(scanResult).get(BleAdvertisingDataType::COMPLETE_LOCAL_NAME, buf, MAX_MANUFACTURER_DATA_LEN);
            if (count == 0)
                count = SCAN_RESPONSE(scanResult).get(BleAdvertisingDataType::SHORT_LOCAL_NAME, buf, MAX_MANUFACTURER_DATA_LEN);
            if (count > 0 && memcmp(buf, _name.data(), count)) {
                _name.clear();
                _name.append((const char*)buf, count);
                _name.append('\0');
                Log.trace("New device name: %s", _name.data());
            }
        }
        if (_eventCallback && _record_number != prev_record)
            _eventCallback(*this, event);
        if (_alarmCallback != nullptr) {
            if (flags & (uint16_t)lairdbt510_flags::LOW_BATTERY_ALARM)
                _alarmCallback(*this, lairdbt510_event_type::BATTERY_BAD);
            if (flags & (uint16_t)lairdbt510_flags::HIGH_TEMP_ALARM_0)
                _alarmCallback(*this, lairdbt510_event_type::ALARM_HIGH_TEMP_1);
            if (flags & (uint16_t)lairdbt510_flags::HIGH_TEMP_ALARM_1)
                _alarmCallback(*this, lairdbt510_event_type::ALARM_HIGH_TEMP_2);
            if (flags & (uint16_t)lairdbt510_flags::LOW_TEMP_ALARM_0)
                _alarmCallback(*this, lairdbt510_event_type::ALARM_LOW_TEMP_1);
            if (flags & (uint16_t)lairdbt510_flags::LOW_TEMP_ALARM_1)
                _alarmCallback(*this, lairdbt510_event_type::ALARM_LOW_TEMP_2);
            if (flags & (uint16_t)lairdbt510_flags::DELTA_TEMP_ALARM)
                _alarmCallback(*this, lairdbt510_event_type::ALARM_DELTA_TEMP);
            if (flags & (uint16_t)lairdbt510_flags::MOVEMENT_ALARM)
                _alarmCallback(*this, lairdbt510_event_type::MOVEMENT);
            if (prev_magnet != _magnet_state)
                _alarmCallback(*this, lairdbt510_event_type::MAGNET_PROXIMITY);
        }
    }
}

bool LairdBt510::isBeacon(const BleScanResult *scanResult)
{
    uint8_t buf[9];
    size_t size = ADVERTISING_DATA(scanResult).get(buf, 9);
    if (size >= 9 && buf[0] == 0x02 && buf[1] == 0x01 && buf[2] == 0x06 && (buf[3] == 0x1b || buf[3] == 0x26) && buf[4] == 0xFF &&
            buf[5] == 0x77 && buf[6] == 0x00 && (buf[7] == 0x01 || buf[7] == 0x02) && buf[8] == 0x00) { 
                return true;
            }
    return false;
}

void LairdBt510::toJson(JSONWriter *writer) const
{
        writer->name(address.toString()).beginObject();
        writer->name("magnet_near").value(magnetNear());
        writer->name("temp").value(getTemperature());
        writer->name("record").value(getRecordNumber());
        writer->name("batt").value(getBattVoltage());
        writer->name("rssi").value(getRssi());
        writer->endObject();
}

void LairdBt510::addOrUpdate(const BleScanResult *scanResult) {
    uint8_t i;
    for (i = 0; i < beacons.size(); ++i)
    {
        if (beacons.at(i).getAddress() == ADDRESS(scanResult))
        {
            break;              
        }
    }
    if(i == beacons.size()) {
        LairdBt510 new_beacon;
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        beacons.append(new_beacon);
    } else {
        LairdBt510& beacon = beacons.at(i);
        beacon.newly_scanned = false;
        beacon.populateData(scanResult);
        beacon.missed_scan = 0;
    }
}

class JSONVectorWriter: public JSONWriter {
public:
    JSONVectorWriter(): v_(Vector<char>()) {}
    ~JSONVectorWriter() = default;
    Vector<char> vector() const {return v_;}
    size_t vectorSize() const {return v_.size();}

protected:
    virtual void write(const char *data, size_t size) override {
        v_.append(data, (int)size);
    }

private:
    Vector<char> v_;
};

void LairdBt510::onDataReceived(const uint8_t* data, size_t size, const BlePeerDevice& peer, void* context) {
    LairdBt510* ctx = (LairdBt510*)context;
    // TODO: Check the returned JSON to make sure it is ok
    Log.trace("Received %d bytes", size);
    uint8_t buf[size+1];
    memcpy(buf, data, size);
    buf[size] = 0;
    Log.trace((char*)buf);
    ctx->state_ = DISCONNECT;
}

void LairdBt510::onPairingEvent(const BlePairingEvent& event) {
    LairdBt510* dev = nullptr;
    for (auto& i : beacons) {
        if (i.getAddress() == event.peer.address()) {
            dev = &i;
        }
    }
    if (dev) {
        switch (event.type)
        {
        case BlePairingEventType::PASSKEY_INPUT:
            BLE.setPairingPasskey(event.peer, dev->config_.passkey_);
            Log.trace("Set pairing key for: %s", dev->getAddress().toString().c_str());
            break;
        case BlePairingEventType::STATUS_UPDATED:
        {
            switch (event.payload.status.status)
            {
// This is not supported on the P2/Photon 2                
// https://community.particle.io/t/beacon-scanner-lib-wont-compile-on-new-p2/64855/2?u=gusgonnet
#if !HAL_PLATFORM_RTL872X
            case BLE_GAP_SEC_STATUS_SUCCESS:
                dev->state_ = SENDING;
                Log.trace("Pairing complete for: %s", dev->getAddress().toString().c_str());
                break;
            case BLE_GAP_SEC_STATUS_CONFIRM_VALUE:
                dev->state_ = DISCONNECT;
                Log.error("Passkey is incorrect");
                if (dev->state_ != CLEANUP) {
                    auto p = Promise<bool>::fromDataPtr(dev->handler_data_);
                    p.setError(Error::INVALID_ARGUMENT);
                    dev->state_ = CLEANUP;
                }
                break;
#endif
            default:
                if (dev->state_ != CLEANUP) {
                    auto p = Promise<bool>::fromDataPtr(dev->handler_data_);
                    p.setError(Error::INVALID_ARGUMENT);
                    dev->state_ = CLEANUP;
                }
                Log.error("Other pairing error: %02X", event.payload.status.status);
                break;
            }
            break;
        }
        default:
            Log.trace("Other pairing event: %d", (uint8_t)event.type);
            break;
        }
    } else {
        Log.trace("Pairing event, but device not found");
    }
}

void LairdBt510::onDisconnected(const BlePeerDevice& peer) {
    for (auto& i : beacons) {
        if (i.getAddress() == peer.address() && i.state_ != CLEANUP) {
            auto p = Promise<bool>::fromDataPtr(i.handler_data_);
            p.setError(Error::ABORTED);
            i.state_ = IDLE;
            break;
        }
    }
}

void LairdBt510::loop() {
    static unsigned int timer = System.uptime();
    static uint8_t timeout = 0;
    if (state_ != prev_state_ || timer != System.uptime()) {
        prev_state_ = state_;
        switch (state_)
        {
        case CONNECTING:
            peer_ = BLE.connect(getAddress(), false);
            if (peer_.connected()) {
                state_ = PAIRING;
            }
            break;
        case PAIRING:
            BLE.startPairing(peer_);
            Log.trace("Pairing");
            break;
        case SENDING:
        {
            peer_.discoverAllServices();
            peer_.discoverAllCharacteristics();
            peer_.getCharacteristicByUUID(rx, BleUuid("569a2001-b87f-490c-92cb-11ba5ea5167c"));
            peer_.getCharacteristicByUUID(tx, BleUuid("569a2000-b87f-490c-92cb-11ba5ea5167c"));
            tx.onDataReceived(onDataReceived, this);
            tx.subscribe(true);
            JSONVectorWriter writer_;
            config_.createJson(writer_, configId_);
            char buf[writer_.vectorSize()];
            for (size_t i = 0; i < writer_.vectorSize(); ++i) {
                buf[i] = writer_.vector().at(i);
            }
            Log.trace("Send value: %s", buf);
            Log.trace("set value return: %d",rx.setValue(reinterpret_cast<const uint8_t*>(writer_.vector().data()), writer_.vectorSize(), BleTxRxType::ACK));
            if (state_ == SENDING) {
                state_ = RECEIVING;
                timeout = 0;
            }
            break;
        }
        case RECEIVING:
        // The state change is handled automatically by the onReceive callback, but we should
        // have a timeout here in case something went wrong.
        {
            if (++timeout > RECEIVE_TIMEOUT_LOOPS) {
                state_ = DISCONNECT;
            }
            break;
        }
        case DISCONNECT:
        {
            auto p = Promise<bool>::fromDataPtr(handler_data_);
            if (timeout > RECEIVE_TIMEOUT_LOOPS) {
                p.setError(Error::TIMEOUT);
            } else {
                p.setResult(true);
            }
            state_ = CLEANUP;
            break;
        }
        case CLEANUP:
            state_ = IDLE;
            peer_.disconnect();
            break;
        case IDLE:
            break;
        }
        timer = System.uptime();
    }
}

particle::Future<bool> LairdBt510::configure(LairdBt510Config config) {
    Promise<bool> p;
    if (state_ == IDLE) {
        BLE.onPairingEvent(onPairingEvent);
        BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_ONLY);
        BLE.onDisconnected(onDisconnected);
        handler_data_ = p.dataPtr();
        config_ = config;
        state_ = CONNECTING;
    }
    else {
        p.setError(Error::BUSY);
    }
    return p.future();
}

LairdBt510Config& LairdBt510Config::currentPasskey(const char* passkey) {
    if (strlen(passkey) == 6) {
        for(size_t i = 0; i < 6; ++i) {
            if (passkey[i] < 0x30 || passkey[i] > 0x39) {
                Log.error("Passkey characters must be numbers");
                return *this;
            }
        }
        memcpy(passkey_, passkey, 6);
    } else {
        Log.error("Passkey is not 6 characters long");
    }
    return *this;
}
LairdBt510Config& LairdBt510Config::sensorName(const char* name) {
    name_.clear();
    name_.append(name, std::min(strlen(name)+1, (size_t)23));
    return *this;
}
LairdBt510Config& LairdBt510Config::tempSenseInterval(uint32_t seconds) {
    if (seconds <= 86400) tempSenseInterval_ = seconds; 
    return *this;
}
LairdBt510Config& LairdBt510Config::battSenseInterval(uint32_t seconds) {
    if (seconds <= 86400) battSenseInterval_ = seconds; 
    return *this;
}
LairdBt510Config& LairdBt510Config::highTempAlarm1(int8_t celsius) {
    configFlags_ = (configFlags_ | ConfigHighTempAlarm1);
    highTempAlarm1_ = celsius;
    return *this;
}
LairdBt510Config& LairdBt510Config::highTempAlarm2(int8_t celsius) {
    configFlags_ = (configFlags_ | ConfigHighTempAlarm2);
    highTempAlarm2_ = celsius;
    return *this;
}
LairdBt510Config& LairdBt510Config::lowTempAlarm1(int8_t celsius) {
    configFlags_ = (configFlags_ | ConfigLowTempAlarm1);
    lowTempAlarm1_ = celsius;
    return *this;
}
LairdBt510Config& LairdBt510Config::lowTempAlarm2(int8_t celsius) {
    configFlags_ = (configFlags_ | ConfigLowTempAlarm2);
    lowTempAlarm2_ = celsius;
    return *this;
}
LairdBt510Config& LairdBt510Config::deltaTempAlarm(uint8_t celsius) {
    configFlags_ = (configFlags_ | ConfigDeltaTempAlarm);
    deltaTempAlarm_ = celsius;
    return *this;
}
LairdBt510Config& LairdBt510Config::newPasskey(const char* passkey) {
    if (strlen(passkey) == 6) {
        for(size_t i = 0; i < 6; ++i) {
            if (passkey[i] < 0x30 || passkey[i] > 0x39) {
                Log.error("New passkey characters must be numbers");
                return *this;
            }
        }
        memcpy(newPasskey_, passkey, 6);
        configFlags_ = (configFlags_ | ConfigNewPasskey);
    } else {
        Log.error("New passkey is not 6 characters long");
    }
    return *this;
}
LairdBt510Config& LairdBt510Config::useCodedPhy(bool coded) {
    coded_ = coded ? 1 : 0;
    return *this;
}

void LairdBt510Config::createJson(JSONVectorWriter& writer, uint16_t& configId) const {
    writer.beginObject();
    writer.name("jsonrpc").value("2.0");
    writer.name("method").value("set");
    writer.name("params").beginObject();
    if (!name_.isEmpty()) writer.name("sensorName").value(name_.data());
    if (tempSenseInterval_ <= 86400) writer.name("temperatureSenseInterval").value((unsigned int)tempSenseInterval_);
    if (battSenseInterval_ <= 86400) writer.name("batterySenseInterval").value((unsigned int)battSenseInterval_);
    if (configFlags_ & ConfigHighTempAlarm1) writer.name("highTemperatureAlarmThreshold1").value((int)highTempAlarm1_);
    if (configFlags_ & ConfigHighTempAlarm2) writer.name("highTemperatureAlarmThreshold2").value((int)highTempAlarm2_);
    if (configFlags_ & ConfigLowTempAlarm1) writer.name("lowTemperatureAlarmThreshold1").value((int)lowTempAlarm1_);
    if (configFlags_ & ConfigLowTempAlarm2) writer.name("lowTemperatureAlarmThreshold2").value((int)lowTempAlarm2_);
    if (configFlags_ & ConfigDeltaTempAlarm) writer.name("deltaTemperatureAlarmThreshold").value((unsigned)deltaTempAlarm_);
    if (configFlags_ & ConfigNewPasskey) writer.name("passkey").value((const char *)newPasskey_, 6);
    if (coded_ < 2) writer.name("useCodedPhy").value((int)coded_);
    writer.endObject();
    writer.name("id").value(++configId);
    writer.endObject();
    writer.vector().append((char)0);
}

LairdBt510Config::LairdBt510Config():
        name_(Vector<char>()),
        tempSenseInterval_(0xFFFFFFFF),
        battSenseInterval_(0xFFFFFFFF),
        advInterval_(0xFFFF),
        connTimeout_(0xFFFF),
        tempAggregationCount_(0xFF),
        configFlags_(Bt510ConfigFields::NONE),
        coded_(2),
        passkey_{0x31, 0x32, 0x33, 0x34, 0x35, 0x36}
        {};

#endif // SUPPORT_LAIRDBT510