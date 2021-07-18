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

#include "Particle.h"
#include "BeaconScanner.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler(LOG_LEVEL_TRACE);

void eventCallback(LairdBt510& beacon, lairdbt510_event_type evt) {
  /**
   * This callback receives events from the Laird BT510 beacon.
   * See the documentation for the beacon to see the difference between
   * events and alarms.
   */
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
  /**
   * This callback receives alarms from the Laird BT510 beacon. It's important
   * to note that the library doesn't implement alarm filtering or hysteresis.
   * If the beacon sends an alarm, it is passed to the callback. The appplication
   * may implement some filtering (or set/clear hysteresis) to avoid repeat
   * alerting, if that's desirable.
   */
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
  /**
   * Function that receives a JSON object to configure the nearby beacons
   * 
   * The field names are the ones that can be set on the beacon. Refer
   * to Laird's data sheet for more details on what they each do.
   */
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
    /**
     * The passkey field is the current passkey to be used for the connection.
     * It doesn't change the configuration on the device.
     */
    else if (iter.name() == "passkey") {
      config.currentPasskey(iter.value().toString().data());
    }
    /**
     * The newPasskey field is the new passkey to be configured onto the device.
     * This changes the configuration.
     */
    else if (iter.name() == "newPasskey") {
      config.newPasskey(iter.value().toString().data());
    }
    else if (iter.name() == "useCodedPhy") {
      config.useCodedPhy(iter.value().toBool());
    }
    /**
     * The sensorAddress field doesn't change anything on the beacon itself.
     * Rather, this field is set in order to specify the address of the beacon
     * that should be configured. If omitted, then all the detected beacons
     * nearby will be reconfigured.
     */
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

int setScanPhy(String command) {
  /**
   * Function to change the PHY that is used for scanning. Use the following values:
   * 1 - use the 1 MBPS PHY, which is the standard pre-BT 5.0 that most devices support
   * 4 - use the Coded PHY, new in BT 5.0, which gives you Long Range support
   * 5 - use both PHYs to scan
   */
  int phy = command.toInt();
  BLE.setScanPhy(static_cast<BlePhy>(phy));
  return 0;
}

void setup() {
  Particle.connect();
  BLE.on();
  Scanner.startContinuous();
  LairdBt510::setEventCallback(eventCallback);
  LairdBt510::setAlarmCallback(alarmCallback);
  Particle.function("lairdBt510Config", configDevice);
  Particle.function("Scan PHY", setScanPhy);
}

void loop() {
  Scanner.loop();
}
