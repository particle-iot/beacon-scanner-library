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

SerialLogHandler logHandler;

void setup() {
}

unsigned long scannedTime = 0;

void loop() {
  if (Particle.connected() && (millis() - scannedTime) > 10000) {
    scannedTime = millis();
    Scanner.scan(5, SCAN_IBEACON | SCAN_KONTAKT);
    Vector<KontaktTag> tags = Scanner.getKontaktTags();
    while(!tags.isEmpty())
    {
      KontaktTag tag = tags.takeFirst();
      Log.info("Address: %s, Temperature: %d", tag.getAddress().toString().c_str(), tag.getTemperature());
    }
    Vector<iBeaconScan> beacons = Scanner.getiBeacons();
    while(!beacons.isEmpty())
    {
      iBeaconScan beacon = beacons.takeFirst();
      Log.info("Address: %s, major: %u, minor: %u", beacon.getAddress().toString().c_str(), beacon.getMajor(), beacon.getMinor()); 
    }
    Vector<Eddystone> ebeacons = Scanner.getEddystone();
    while(!ebeacons.isEmpty())
    {
      Eddystone ebeacon = ebeacons.takeFirst();
      Log.info("Address: %s", ebeacon.getAddress().toString().c_str());
    }
  }
}