/*
 * Author: Gustavo Gonnet
 * Date: June 2024
 * Hackster project: https://www.hackster.io/gusgonnet/ble-door-window-monitor-12f054
 */

#include "Particle.h"
#include "BeaconScanner.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

void setup()
{
  // Select whether you want to use the internal or external antenna
  // For internal antenna, use BLE.selectAntenna(BleAntennaType::INTERNAL);
  BLE.selectAntenna(BleAntennaType::EXTERNAL);
}

void loop()
{
  static unsigned long scannedTime = 0;
  if (Particle.connected() && (millis() - scannedTime) > 1000)
  {
    Log.info("Scanning...");
    scannedTime = millis();

    Scanner.scan(1, SCAN_BTHOME);

    Vector<BTHome> beacons = Scanner.getBTHome();
    while (!beacons.isEmpty())
    {
      BTHome beacon = beacons.takeFirst();
      Log.info("BTHome Address: %s, Battery: %d, Button: %d, Window: %d, Rotation: %d", beacon.getAddress().toString().c_str(), beacon.getBatteryLevel(), beacon.getButtonEvent(), beacon.getWindowState(), beacon.getRotation());
    }
  }
}
