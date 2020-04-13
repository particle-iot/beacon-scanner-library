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
    scanner.scanAndPublish(5, SCAN_KONTAKT | SCAN_IBEACON | SCAN_EDDYSTONE, "test", PRIVATE);
  }
}