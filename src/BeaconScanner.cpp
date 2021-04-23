/* beacon-scanner library by Mariano Goluboff
 */

#include "BeaconScanner.h"
#include "os-version-macros.h"

#define PUBLISH_CHUNK 622
#define IBEACON_JSON_SIZE 119
#define KONTAKT_JSON_SIZE 82
#define EDDYSTONE_JSON_SIZE 260

#define IBEACON_CHUNK       ( PUBLISH_CHUNK / IBEACON_JSON_SIZE )
#define KONTAKT_CHUNK       ( PUBLISH_CHUNK / KONTAKT_JSON_SIZE )
#define EDDYSTONE_CHUNK     ( PUBLISH_CHUNK / EDDYSTONE_JSON_SIZE )

#define IBEACON_NONSAVER    ( 5000 / IBEACON_JSON_SIZE )
#define KONTAKT_NONSAVER    ( 5000 / KONTAKT_JSON_SIZE )
#define EDDYSTONE_NONSAVER  ( 5000 / EDDYSTONE_JSON_SIZE )

Beaconscanner *Beaconscanner::_instance = nullptr;

template<typename T>
String Beaconscanner::getJson(Vector<T>* beacons, uint8_t count, void *context)
{
    Beaconscanner *ctx = (Beaconscanner *)context;
    ctx->writer->beginObject();
    SINGLE_THREADED_BLOCK() {
    while(count > 0 && !beacons->isEmpty())
    {
        beacons->takeFirst().toJson(ctx->writer);
        count--;
    }
    }
    ctx->writer->endObject();
    return String::format("%.*s", ctx->writer->dataSize(), ctx->writer->buffer());
}

void Beaconscanner::scanChunkResultCallback(const BleScanResult *scanResult, void *context)
{
    /*
     *  Check if we're scanning for a type of beacon, and if it is that type.
     *  Create new instance, and populate with existing data if already in the Vector.
     *  Populate data into the instance with the info just received.
     *  Add instance back into the Vector
     *  Publish if the Vector size is getting too large, to stay under the cloud publish limit
     */
    Beaconscanner *ctx = (Beaconscanner *)context;
    if ((ctx->_flags & SCAN_IBEACON) && !ctx->iPublished.contains(ADDRESS(scanResult)) && iBeaconScan::isBeacon(scanResult))
    {
        iBeaconScan new_beacon;
        for (uint8_t i = 0; i < ctx->iBeacons.size(); i++)
        {
            if (ctx->iBeacons.at(i).getAddress() == ADDRESS(scanResult))
            {
                new_beacon = ctx->iBeacons.takeAt(i);
                break;
            }
        }
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        ctx->iBeacons.append(new_beacon);
    }
    else if ((ctx->_flags & SCAN_KONTAKT) && !ctx->kPublished.contains(ADDRESS(scanResult)) && KontaktTag::isTag(scanResult))
    {
        KontaktTag new_beacon;
        for (uint8_t i = 0; i < ctx->kSensors.size(); i++)
        {
            if (ctx->kSensors.at(i).getAddress() == ADDRESS(scanResult))
            {
                new_beacon = ctx->kSensors.takeAt(i);
                break;
            }
        }
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        ctx->kSensors.append(new_beacon);
    } else if ((ctx->_flags & SCAN_EDDYSTONE) && !ctx->ePublished.contains(ADDRESS(scanResult)) && Eddystone::isBeacon(scanResult))
    {
        Eddystone new_beacon;
        for (uint8_t i = 0; i < ctx->eBeacons.size(); i++)
        {
            if (ctx->eBeacons.at(i).getAddress() == ADDRESS(scanResult))
            {
                new_beacon = ctx->eBeacons.takeAt(i);
                break;
            }
        }
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        ctx->eBeacons.append(new_beacon);
    }
}

void custom_scan_params() {
    /*
     *  The callback appears to be called just once per MAC address per BLE.scan(callback) call.
     *  This is ok if the advertiser always sends the same info (like an iBeacon). Kontakt and
     *  Eddystone tags advertise on same MAC, with different packet types.
     * 
     *  To be able to get all the data, we have to scan multiple times for the duration.
     * 
     *  This function makes a short scan (150ms) and runs it for the number of seconds passed in.
     */
    BleScanParams scanParams;
    scanParams.size = sizeof(BleScanParams);
    scanParams.interval = 80;   // 50ms
    scanParams.window = 40;     // 25ms
    scanParams.timeout = 15;    // 150ms
    scanParams.active = false;
    scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    BLE.setScanParameters(&scanParams); 
}

void Beaconscanner::customScan(uint16_t duration)
{
    custom_scan_params();
    kPublished.clear();
    iPublished.clear();
    ePublished.clear();
    kSensors.clear();
    iBeacons.clear();
    eBeacons.clear();
    long int elapsed = millis();
    while(millis() - elapsed < duration*1000)
    {
        BLE.scan(scanChunkResultCallback, this);
        if (_publish && (  
            (_memory_saver && iBeacons.size() >= IBEACON_CHUNK) ||
            (!_memory_saver && iBeacons.size() >= IBEACON_NONSAVER)
            ) )
        {
            for (uint8_t i = 0; i < IBEACON_CHUNK; i++)
            {
                iPublished.append(iBeacons.at(i).getAddress());
            }
            publish(SCAN_IBEACON);
        }
        if (_publish && (
            (_memory_saver && kSensors.size() >= KONTAKT_CHUNK) ||
            (!_memory_saver && kSensors.size() >= KONTAKT_NONSAVER)
        ) )
        {
            for (uint8_t i = 0; i < KONTAKT_CHUNK; i++)
            {
                kPublished.append(kSensors.at(i).getAddress());
            }
            publish(SCAN_KONTAKT);
        }
        if (_publish && (
            (_memory_saver && eBeacons.size() >= EDDYSTONE_CHUNK) ||
            (!_memory_saver && eBeacons.size() >= EDDYSTONE_NONSAVER)
        ) )
        {
            for (uint8_t i=0;i < EDDYSTONE_CHUNK;i++)
            {
                ePublished.append(eBeacons.at(i).getAddress());
            }
            publish(SCAN_EDDYSTONE);
        }
    }
}

void Beaconscanner::scanAndPublish(uint16_t duration, int flags, const char* eventName, PublishFlags pFlags, bool memory_saver)
{
    if (_run) return;
    _flags = flags;
    _publish = true;
    _eventName = eventName;
    _pFlags = pFlags;
    _memory_saver = memory_saver;
    customScan(duration);
    while (!iBeacons.isEmpty())
        publish(SCAN_IBEACON);
    while (!kSensors.isEmpty())
        publish(SCAN_KONTAKT);
    while (!eBeacons.isEmpty())
        publish(SCAN_EDDYSTONE);
}

void Beaconscanner::scan(uint16_t duration, int flags)
{
    if (_run) return;
    _publish= false;
    _flags = flags;
    customScan(duration);
}

void Beaconscanner::scan_thread(void *param) {
    while(true) {
        if (!_instance->_run) {
            os_thread_yield();
            continue;
        }
        custom_scan_params();
        long int elapsed = millis();
        while(_instance->_run && millis() - elapsed < _instance->_scan_period*1000) {
            BLE.scan(scanChunkResultCallback, _instance);
        }
        SINGLE_THREADED_BLOCK() {
            for (int i = 0; i < _instance->iBeacons.size(); i++) {
                if (_instance->iBeacons.at(i).missed_scan > _instance->_clear_missed) {
                    _instance->iBeacons.removeAt(i);
                    i--;
                } else {
                    _instance->iBeacons.at(i).missed_scan++;
                }
            }
            for (int i = 0; i < _instance->eBeacons.size(); i++) {
                if (_instance->eBeacons.at(i).missed_scan > _instance->_clear_missed) {
                    _instance->eBeacons.removeAt(i);
                    i--;
                } else {
                    _instance->eBeacons.at(i).missed_scan++;
                }
            }
            for (int i = 0; i < _instance->kSensors.size(); i++) {
                if (_instance->kSensors.at(i).missed_scan > _instance->_clear_missed) {
                    _instance->kSensors.removeAt(i);
                    i--;
                } else {
                    _instance->kSensors.at(i).missed_scan++;
                }
            }
        }
        os_thread_yield();
    }
}

void Beaconscanner::startContinuous(int flags) {
    _flags = flags;
    _run = true;
    if (_thread == nullptr) 
        _thread = new Thread("scan_thread", scan_thread);
}

void Beaconscanner::stopContinuous() {
    _run = false;
}

void Beaconscanner::publish(const char* eventName, int type)
{
    _eventName = eventName;
    if (type & SCAN_IBEACON) publish(SCAN_IBEACON);
    if (type & SCAN_KONTAKT) publish(SCAN_KONTAKT);
    if (type & SCAN_EDDYSTONE) publish(SCAN_EDDYSTONE);
}

void Beaconscanner::publish(int type)
{
    char *buf = new char[PUBLISH_CHUNK];
    writer = new JSONBufferWriter(buf, PUBLISH_CHUNK);
    switch (type)
    {
        case SCAN_IBEACON:
            Particle.publish(String::format("%s-ibeacon", _eventName), getJson(&iBeacons, std::min(IBEACON_CHUNK, iBeacons.size()), this),_pFlags);
            break;
        case SCAN_KONTAKT:
            Particle.publish(String::format("%s-kontakt", _eventName), getJson(&kSensors, std::min(KONTAKT_CHUNK, kSensors.size()), this),_pFlags);
            break;
        case SCAN_EDDYSTONE:
            Particle.publish(String::format("%s-eddystone", _eventName), getJson(&eBeacons, std::min(EDDYSTONE_CHUNK, eBeacons.size()),this), _pFlags);
            break;
        default:
            break;
    }
    delete writer;
    delete[] buf;
}
