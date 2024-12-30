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

    Scanner.scan(1, SCAN_GOOVEE);

    Vector<Goovee> beacons = Scanner.getGoovee();
    while (!beacons.isEmpty())
    {
      Goovee beacon = beacons.takeFirst();
      Log.info("Goovee Address: %s, Temperature: %.1f C / %.1f F, Humidity: %.1f%%, batteryLevel: %d%%", beacon.getAddress().toString().c_str(), beacon.getTemperatureC(), beacon.getTemperatureF(), beacon.getHumidity(), beacon.getBatteryLevel());
    }
  }
}
