/*
 * Author: Gustavo Gonnet
 * Date: June 2024
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

    Scanner.scan(1, SCAN_RUUVI);

    Vector<Ruuvi> beacons = Scanner.getRuuvi();
    while (!beacons.isEmpty())
    {
      Ruuvi beacon = beacons.takeFirst();
      Log.info("Ruuvi Address: %s, Temperature: %.2f, Humidity: %.2f", beacon.getAddress().toString().c_str(), beacon.getTemperature(), beacon.getHumidity());
    }
  }
}
