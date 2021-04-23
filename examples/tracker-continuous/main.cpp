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

SerialLogHandler logHandler(115200, LOG_LEVEL_TRACE, {
    { "app.gps.nmea", LOG_LEVEL_INFO },
    { "app.gps.ubx",  LOG_LEVEL_INFO },
    { "ncp.at", LOG_LEVEL_INFO },
    { "net.ppp.client", LOG_LEVEL_INFO },
});


void locationGenerationCallback(JSONWriter &writer, LocationPoint &point, const void *context)
{
    for (auto i : Beaconscanner::instance().getKontaktTags()) {
        writer->name(address.toString()).beginObject();
        if (battery != 0xFF)
            writer->name("batt").value(battery);
        if (temperature != 0xFF)
            writer->name("temp").value(temperature);
        if (button_time != 0xFFFF)
            writer->name("button").value(button_time);
        if (accel_data)
        {
            writer->name("x_axis").value(x_axis);
            writer->name("y_axis").value(y_axis);
            writer->name("z_axis").value(z_axis);
        }
        writer->endObject();
      }
}

void setup()
{
    Tracker::instance().init();
    Tracker::instance().location.regLocGenCallback(locationGenerationCallback);
    BLE.on();
    Beaconscanner::instance().startContinuous();
}


void loop()
{
    Tracker::instance().loop();
}
