#include "lairdbt510.h"

LairdBt510EventCallback LairdBt510::_eventCallback = nullptr;
LairdBt510EventCallback LairdBt510::_alarmCallback = nullptr;

void LairdBt510::populateData(const BleScanResult *scanResult)
{
    Beacon::populateData(scanResult);
    address = ADDRESS(scanResult);
    uint8_t buf[38];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA, buf, 38);
    uint16_t prev_record = _record_number;
    if (count > 25) {   // Advertising data is correct, either table 1 or table 3
        bool prev_magnet = _magnet_state;
        uint16_t flags = buf[7] << 8 | buf[6];
        _magnet_state = (flags & (uint16_t)lairdbt510_flags::MAGNET_STATE);
        _record_number = buf[16] << 8 | buf[15];
        if (count == 26 && buf[2] == 0x01) {
            switch ((lairdbt510_event_type)buf[14])
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
            if (_eventCallback && _record_number != prev_record)
                _eventCallback(*this, (lairdbt510_event_type)buf[14]);
        }
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
    if (ADVERTISING_DATA(scanResult).contains(BleAdvertisingDataType::MANUFACTURER_SPECIFIC_DATA))
    {
        uint8_t buf[9];
        ADVERTISING_DATA(scanResult).get(buf, 9);
        if (buf[0] == 0x02 && buf[1] == 0x01 && buf[2] == 0x06 && buf[3] == 0x1b && buf[4] == 0xFF &&
            buf[5] == 0x77 && buf[6] == 0x00 && (buf[7] == 0x01 || buf[7] == 0x02) && buf[8] == 0x00) { 
                return true;
            }
    
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
    Log.trace("Received %d bytes", size);
    uint8_t buf[size+1];
    memcpy(buf, data, size);
    buf[size] = 0;
    Log.trace((char*)buf);
    ctx->state_ = DISCONNECT;
}

void LairdBt510::onPairingEvent(const BlePairingEvent& event, void* context) {
    LairdBt510* ctx = (LairdBt510*)context;
    Log.trace("Pairing event: %d", (uint8_t)event.type);
    BLE.setPairingPasskey(event.peer, (uint8_t *)"123456");
    if (event.type == BlePairingEventType::STATUS_UPDATED) {
        ctx->state_ = SENDING;
    }
}

void LairdBt510::loop() {
    unsigned int timer = System.uptime();
    if (state_ != prev_state_ || timer != System.uptime()) {
        prev_state_ = state_;
        switch (state_)
        {
        case CONNECTING:
            peer_ = BLE.connect(getAddress(), false);
            if (peer_.connected()) {
                state_ = PAIRING;
            }
            Log.trace("State connecting");
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
            state_ = RECEIVING;
            break;
        }
        case RECEIVING:
            break;
        case DISCONNECT:
            peer_.disconnect();
            state_ = IDLE;
            break;
        case IDLE:
            break;
        }
        timer = System.uptime();
    }
}

bool LairdBt510::configure(LairdBt510Config config) {
    if (state_ == IDLE) {
        BLE.onPairingEvent(onPairingEvent, this);
        BLE.setPairingIoCaps(BlePairingIoCaps::KEYBOARD_ONLY);
        config_ = config;
        state_ = CONNECTING;
        Log.trace("Set to connecting state %d", state_);
        return true;
    }
    else {
        return false;
    }
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
        configFlags_(Bt510ConfigFields::NONE)
        {};