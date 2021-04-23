# Beacon Scanner

This library works with Particle Gen3 devices to scan for BLE advertisements and parses them for common beacon standards. Currently supported:
* iBeacon
* Eddystone UID, URL, and unencrypted TLM
* Kontakt.io beacons (tested with Asset Tag S18-3)

## Functions available

There are a few functions that an application can call depending on the needs of the use case.

### __NEW in Version 1.0.0__ Continuous threaded scanning

In this mode, the application no longer needs to block when doing a scan. Instead, start the continuous mode with
the following API. This will most likely go in `setup()` if the Application is always scanning:

```c++
Scanner.startContinuous();
```

Then, whenever the application can at any time get Vectors of the most recently scanned tags like this:

```c++
for (auto i : Scanner.getKontaktTags()) {
    Log.info("Address: %s, Temperature %u", i.getAddress().toString().c_str(), i.getTemperature());
}
```

Or have the library automatically publish:

```c++
Scanner.publish("all");
```

### Automatic Scan and Publish

In this mode, the library will scan for BLE advertisements, and use Particle.publish() to send the data to the cloud.

    
    void scanAndPublish(uint16_t duration, int flags, const char* eventName, PublishFlags pFlags, bool memory_saver)

    duration: How long to collect data, in seconds
    flags: Which type of beacons to publish. Use bitwise OR for multiple. e.g.: SCAN_KONTAKT | SCAN_IBEACON | SCAN_EDDYSTONE
    eventName: The cloud publish will use this event name, and add "-ibeacon","-kontakt","-eddystone"
    pFlags: Flags for the publish, e.g.: PRIVATE
    memory_saver: Default is false. If set to true, it will publish more often and use less memory. Caution, this means that some data might not be collected from beacons that advertise multiple times with different data.

The output of this on the console looks like (with eventName "test"):
![](img/kontakt-example.png)

![](img/ibeacon-example.png)

### Get a Vector of the detected tags

If the application needs to get the data, rather than automatically publishing it, this can be accomplished by first running a scan using the following function:

    void scan(uint16_t duration, int flags)

    duration: How long to collect data, in seconds (default: 5)
    flags: Which type of beacons to publish. Use bitwise OR for multiple. e.g.: SCAN_KONTAKT | SCAN_IBEACON | SCAN_EDDYSTONE (default: all)

And then the data for each supported type of advertiser can be retrieved as a Vector:

    Vector<KontaktTag> getKontaktTags();
    Vector<iBeacon> getiBeacons();
    Vector<Eddystone> getEddystone();


### A note on "duration"

This is how long the library will listen for beacons. However, during that time a beacon might advertise multiple times. The library will NOT publish every time the beacon advertises.

For a Kontakt tag, all the values will be based on the last received packet for each address detected.

For an iBeacon, all the values will be based on the last received packet except for RSSI. The RSSI will be an average of all the received values during the scan duration.

## Typical usage

```c++
#include "Particle.h"
#include "BeaconScanner.h"

SYSTEM_THREAD(ENABLED);

void setup() {
}

unsigned long scannedTime = 0;

void loop() {
    if (Particle.connected() && (millis() - scannedTime) > 10000) {
        scannedTime = millis();
        Scanner.scanAndPublish(5, SCAN_KONTAKT | SCAN_IBEACON | SCAN_EDDYSTONE, "test", PRIVATE);
    }
}
```

## Typical Tracker usage

```c++
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

void locationGenerationCallback(JSONWriter &writer, LocationPoint &point, const void *context)
{
    for (auto i : Scanner.getKontaktTags()) {
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
    Scanner.startContinuous();
}


void loop()
{
    Tracker::instance().loop();
}
```

## Examples

* __Log:__ Starts a scan for iBeacons, Kontakt tags, and Eddystone beacons, and then logs the address, major, and minor of the beacons, address and temperature of the tags, and address of Eddystone
* __Publish:__ Starts a scan which publishes all the types of devices
* __Tracker Continuous:__ With a Tracker, continuously scan and publish the most recently detected when the Tracker decides to publish
