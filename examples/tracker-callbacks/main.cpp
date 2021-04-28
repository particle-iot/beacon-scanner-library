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

#include "tracker_config.h"
#include "tracker.h"

#include "BeaconScanner.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

PRODUCT_ID(TRACKER_PRODUCT_ID);
PRODUCT_VERSION(TRACKER_PRODUCT_VERSION);

STARTUP(
    Tracker::startup();
);

Vector<BleAddress> entered, left;

void locationGenerationCallback(JSONWriter &writer, LocationPoint &point, const void *context)
{
    writer.name("entered").beginArray();
    while (!entered.isEmpty()) {
      writer.value(entered.takeFirst().toString());
    }
    writer.endArray();
    writer.name("left").beginArray();
    while (!left.isEmpty()) {
      writer.value(left.takeFirst().toString());
    }
    writer.endArray();
}

void onCallback(Beacon& beacon, callback_type type) {
  if (type == NEW && !entered.contains(beacon.getAddress())) {
    entered.append(beacon.getAddress());
  }
  if (type == REMOVED && !left.contains(beacon.getAddress())) {
    left.append(beacon.getAddress());
  }
}

void setup()
{
    Tracker::instance().init();
    Tracker::instance().location.regLocGenCallback(locationGenerationCallback);
    BLE.on();
    Scanner.setCallback(onCallback);
    Scanner.startContinuous();
}


void loop()
{
    Tracker::instance().loop();
    Scanner.loop();
}
