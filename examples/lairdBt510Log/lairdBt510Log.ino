#include "Particle.h"
#include "BeaconScanner.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

void eventCallback(LairdBt510& beacon, lairdbt510_event_type evt) {
  switch (evt)
  {
  case lairdbt510_event_type::TEMPERATURE:
    Log.info("Temperature: %d hundredths of degree C", beacon.getTemperature());
    break;
  case lairdbt510_event_type::ALARM_HIGH_TEMP_1:
    Log.info("Temperature high alarm event! Temp: %d", beacon.getTemperature());
    break;
  case lairdbt510_event_type::BATTERY_GOOD:
  case lairdbt510_event_type::BATTERY_BAD:
    Log.info("New battery value: %u mV", beacon.getBattVoltage());
    break;
  case lairdbt510_event_type::MAGNET_PROXIMITY:
    Log.info("Magnet change event");
    break;
  case lairdbt510_event_type::MOVEMENT:
    Log.info("Movement event");
    break;
  default:
    Log.info("Event type: %u, Record number: %u", (uint16_t)evt, beacon.getRecordNumber());
    break;
  }
}

void alarmCallback(LairdBt510& beacon, lairdbt510_event_type alarm) {
  switch (alarm)
  {
    case lairdbt510_event_type::MOVEMENT:
      Log.info("Movement alarm!");
      break;
    case lairdbt510_event_type::ALARM_HIGH_TEMP_1:
      Log.info("High Temp alarm 1! Temp: %d", beacon.getTemperature());
      break;
    case lairdbt510_event_type::MAGNET_PROXIMITY:
      Log.info("Magnet state changed. Magnet %s near", beacon.magnetNear() ? "is" : "is not");
      break;
    default:
      Log.info("Another alarm");
      break;
  }
}

int configDevice(String command) {
  JSONValue obj = JSONValue::parseCopy(command);
  JSONObjectIterator iter(obj);
  LairdBt510Config config;
  BleAddress address;
  while(iter.next()) {
    if (iter.name() == "sensorName") {
      config.sensorName(iter.value().toString().data());
    }
    else if (iter.name() == "temperatureSenseInterval") {
      config.tempSenseInterval(iter.value().toInt());
    }
    else if (iter.name() == "batterySenseInterval") {
      config.battSenseInterval(iter.value().toInt());
    }
    else if (iter.name() == "highTemperatureAlarmThreshold1") {
      config.highTempAlarm1(iter.value().toInt());
    }
    else if (iter.name() == "highTemperatureAlarmThreshold2") {
      config.highTempAlarm2(iter.value().toInt());
    }
    else if (iter.name() == "lowTemperatureAlarmThreshold1") {
      config.lowTempAlarm1(iter.value().toInt());
    }
    else if (iter.name() == "lowTemperatureAlarmThreshold2") {
      config.lowTempAlarm2(iter.value().toInt());
    }
    else if (iter.name() == "deltaTemperatureAlarmThreshold") {
      config.deltaTempAlarm(iter.value().toInt());
    }
    else if (iter.name() == "sensorAddress") {
      auto addressString = String(iter.value().toString().data());
      address.set(addressString, BleAddressType::RANDOM_STATIC);
      Log.info("Only targeting %s", address.toString().c_str());
    }
  }
  for (auto& i : Scanner.getLairdBt510()) {
    auto addDevice = i.getAddress();
    if (address.isValid() && (address != addDevice)) {
      // Stop configuring the particular device if address is given and is not matched
      continue;
    }
    Log.info("Configuring %s", addDevice.toString().c_str());
    i.configure(config);
  }
  return 0;
}

void setup() {
  Particle.connect();
  BLE.on();
  Scanner.startContinuous();
  LairdBt510::setEventCallback(eventCallback);
  LairdBt510::setAlarmCallback(alarmCallback);
  Particle.function("lairdBt510Config", configDevice);
}

void loop() {
  Scanner.loop();
}
