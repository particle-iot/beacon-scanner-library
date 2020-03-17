#include "Particle.h"
#include "BeaconScanner.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;
Beaconscanner scanner;

void setup() {
}

unsigned long scannedTime = 0;

void loop() {
  if (Particle.connected() && (millis() - scannedTime) > 10000) {
    scannedTime = millis();
    scanner.scan(5, SCAN_IBEACON | SCAN_KONTAKT);
    Vector<KontaktTag> tags = scanner.getKontaktTags();
    while(!tags.isEmpty())
    {
      KontaktTag tag = tags.takeFirst();
      Log.info("Address: %s, Temperature: %u", tag.getAddress().toString().c_str(), tag.getTemperature());
    }
    Vector<iBeaconScan> beacons = scanner.getiBeacons();
    while(!beacons.isEmpty())
    {
      iBeaconScan beacon = beacons.takeFirst();
      Log.info("Address: %s, major: %u, minor: %u", beacon.getAddress().toString().c_str(), beacon.getMajor(), beacon.getMinor()); 
  }
}