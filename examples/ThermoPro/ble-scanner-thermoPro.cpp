/*
 * Author: Gustavo Gonnet gusgonnet@gmail.com
 * Date: December 2024
 */

#include "Particle.h"
#include "BeaconScanner.h"

SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

void setup()
{
}

void loop()
{
  static unsigned long scannedTime = 0;
  if (Particle.connected() && (millis() - scannedTime) > 1000)
  {
    Log.info("Scanning...");
    scannedTime = millis();

    Scanner.scan(1, SCAN_THERMOPRO);

    Vector<ThermoPro> beacons = Scanner.getThermoPro();
    while (!beacons.isEmpty())
    {
      ThermoPro beacon = beacons.takeFirst();
      Log.info("ThermoPro Address: %s, Temperature: %.1f C / %.1f F, Humidity: %u%%", beacon.getAddress().toString().c_str(), beacon.getTemperatureC(), beacon.getTemperatureF(), beacon.getHumidity());
    }
  }
}
