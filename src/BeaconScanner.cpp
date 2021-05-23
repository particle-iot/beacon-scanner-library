/* beacon-scanner library by Mariano Goluboff
 */

#include "BeaconScanner.h"
#include "os-version-macros.h"

#define PUBLISH_CHUNK 622
#define IBEACON_JSON_SIZE 119
#define KONTAKT_JSON_SIZE 82
#define EDDYSTONE_JSON_SIZE 260
#define LAIRDBT510_JSON_SIZE 100

#define IBEACON_CHUNK       ( PUBLISH_CHUNK / IBEACON_JSON_SIZE )
#define KONTAKT_CHUNK       ( PUBLISH_CHUNK / KONTAKT_JSON_SIZE )
#define EDDYSTONE_CHUNK     ( PUBLISH_CHUNK / EDDYSTONE_JSON_SIZE )
#define LAIRDBT510_CHUNK    ( PUBLISH_CHUNK / LAIRDBT510_JSON_SIZE )

#define IBEACON_NONSAVER    ( 5000 / IBEACON_JSON_SIZE )
#define KONTAKT_NONSAVER    ( 5000 / KONTAKT_JSON_SIZE )
#define EDDYSTONE_NONSAVER  ( 5000 / EDDYSTONE_JSON_SIZE )
#define LAIRDBT510_NONSAVER ( 5000 / LAIRDBT510_JSON_SIZE )

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
     *  
     *  This executes in the BLE thread, so must make sure to not do anything that consumes too much time or blocks.
     */
    Beaconscanner *ctx = (Beaconscanner *)context;
#ifdef SUPPORT_IBEACON
    if ((ctx->_flags & SCAN_IBEACON) && iBeaconScan::isBeacon(scanResult) && !ctx->iPublished.contains(ADDRESS(scanResult)))
    {
        iBeaconScan new_beacon;
        for (uint8_t i = 0; i < ctx->iBeacons.size(); i++)
        {
            if (ctx->iBeacons.at(i).getAddress() == ADDRESS(scanResult))
            {
                new_beacon = ctx->iBeacons.takeAt(i);
                new_beacon.newly_scanned = false;
                break;
            }
        }
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        ctx->iBeacons.append(new_beacon);
    }
#endif
#ifdef SUPPORT_KONTAKT
    if ((ctx->_flags & SCAN_KONTAKT) && KontaktTag::isTag(scanResult) && !ctx->kPublished.contains(ADDRESS(scanResult)))
    {
        KontaktTag new_beacon;
        for (uint8_t i = 0; i < ctx->kSensors.size(); i++)
        {
            if (ctx->kSensors.at(i).getAddress() == ADDRESS(scanResult))
            {
                new_beacon = ctx->kSensors.takeAt(i);
                new_beacon.newly_scanned = false;
                break;
            }
        }
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        ctx->kSensors.append(new_beacon);
    }
#endif
#ifdef SUPPORT_EDDYSTONE 
    if ((ctx->_flags & SCAN_EDDYSTONE) && Eddystone::isBeacon(scanResult) && !ctx->ePublished.contains(ADDRESS(scanResult)))
    {
        Eddystone new_beacon;
        for (uint8_t i = 0; i < ctx->eBeacons.size(); i++)
        {
            if (ctx->eBeacons.at(i).getAddress() == ADDRESS(scanResult))
            {
                new_beacon = ctx->eBeacons.takeAt(i);
                new_beacon.newly_scanned = false;
                break;
            }
        }
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        ctx->eBeacons.append(new_beacon);
    } else if (ctx->_customCallback) {
        ctx->_customCallback(scanResult);
    }
#endif
#ifdef SUPPORT_LAIRDBT510
    if ((ctx->_flags & SCAN_LAIRDBT510) && LairdBt510::isBeacon(scanResult) && !ctx->lPublished.contains(ADDRESS(scanResult)))
    {
        LairdBt510 new_beacon;
        for (uint8_t i = 0; i < ctx->lBeacons.size(); i++)
        {
            if (ctx->lBeacons.at(i).getAddress() == ADDRESS(scanResult))
            {
                new_beacon = ctx->lBeacons.takeAt(i);
                new_beacon.newly_scanned = false;
                break;              
            }
        }
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        ctx->lBeacons.append(new_beacon);
    } else if (ctx->_customCallback) {
        ctx->_customCallback(scanResult);
    }
#endif
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
#ifdef SUPPORT_KONTAKT
    kPublished.clear();
    kSensors.clear();
#endif
#ifdef SUPPORT_IBEACON
    iPublished.clear();
    iBeacons.clear();
#endif
#ifdef SUPPORT_EDDYSTONE
    ePublished.clear();
    eBeacons.clear();
#endif
    long int scan_start = millis();
    BLE.scan(scanChunkResultCallback, this);

    while( (millis() - scan_start) < duration*1000)
    {
#ifdef SUPPORT_IBEACON
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
#endif
#ifdef SUPPORT_KONTAKT
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
#endif
#ifdef SUPPORT_EDDYSTONE
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
#endif
#ifdef SUPPORT_LAIRDBT510
        if (_publish && (
            (_memory_saver && lBeacons.size() >= LAIRDBT510_CHUNK) ||
            (!_memory_saver && lBeacons.size() >= LAIRDBT510_NONSAVER)
        ))
        {
            for (uint8_t i=0;i < LAIRDBT510_CHUNK;i++)
            {
                lPublished.append(lBeacons.at(i).getAddress());
            }
            publish(SCAN_LAIRDBT510);
        }
#endif
    }

    // don't allow scanning to continue after we're done listening to callbacks
    BLE.stopScanning();

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
#ifdef SUPPORT_IBEACON
    while (!iBeacons.isEmpty())
        publish(SCAN_IBEACON);
#endif
#ifdef SUPPORT_KONTAKT
    while (!kSensors.isEmpty())
        publish(SCAN_KONTAKT);
#endif
#ifdef SUPPORT_EDDYSTONE
    while (!eBeacons.isEmpty())
        publish(SCAN_EDDYSTONE);
#endif
#ifdef SUPPORT_LAIRDBT510
    while (!lBeacons.isEmpty())
        publish(SCAN_LAIRDBT510);
#endif
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
        _instance->_scan_done = true;
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

void Beaconscanner::loop() {
#ifdef SUPPORT_IBEACON
    for (auto& i : iBeacons) {
        if (_callback && i.newly_scanned) {
            _callback(i, NEW);
            i.newly_scanned = false;
        }
    }
#endif
#ifdef SUPPORT_EDDYSTONE
    for (auto& e : eBeacons) {
        if (_callback && e.newly_scanned) {
            _callback(e, NEW);
            e.newly_scanned = false;
        }
    }
#endif
#ifdef SUPPORT_KONTAKT
    for (auto& k : kSensors) {
        if (_callback && k.newly_scanned) {
            _callback(k, NEW);
            k.newly_scanned = false;
        }
    }
#endif
#ifdef SUPPORT_LAIRDBT510
    for (auto& l : lBeacons) {
        if (_callback && l.newly_scanned) {
            _callback(l, NEW);
            l.newly_scanned = false;
        }
    }
#endif

    if (_scan_done) {
#ifdef SUPPORT_IBEACON
        for (auto& i : iBeacons) {
            if (i.missed_scan >= _clear_missed) {
                if (_callback) {
                    _callback(i, REMOVED);
                }
                i.missed_scan = -1; // Use an invalid value to mark for removal
            } else {
                i.missed_scan++;
            }
        }
#endif
#ifdef SUPPORT_EDDYSTONE
        for (auto& e : eBeacons) {
            if (e.missed_scan >= _clear_missed) {
                if (_callback) {
                    _callback(e, REMOVED);
                }
                e.missed_scan = -1;
            } else {
                e.missed_scan++;
            }
        }
#endif
#ifdef SUPPORT_KONTAKT
        for (auto& k : kSensors) {
            if (k.missed_scan >= _clear_missed) {
                if (_callback) {
                    _callback(k, REMOVED);
                } 
                k.missed_scan = -1;
            } else {
                k.missed_scan++;
            }
        }
#endif
#ifdef SUPPORT_LAIRDBT510
        for (auto& l : lBeacons) {
            if (l.missed_scan >= _clear_missed) {
                if (_callback) {
                    _callback(l, REMOVED);
                }
                l.missed_scan = -1;
            } else {
                l.missed_scan++;
            }
        }
#endif

        SINGLE_THREADED_BLOCK() {
#ifdef SUPPORT_IBEACON
            for (int i = 0; i < iBeacons.size(); i++) {
                if (iBeacons.at(i).missed_scan < 0) {
                    iBeacons.removeAt(i);
                    i--;
                }
            }
#endif
#ifdef SUPPORT_EDDYSTONE
            for (int i = 0; i < eBeacons.size(); i++) {
                if (eBeacons.at(i).missed_scan < 0) {
                    eBeacons.removeAt(i);
                    i--;
                }
            }
#endif
#ifdef SUPPORT_KONTAKT
            for (int i = 0; i < kSensors.size(); i++) {
                if (kSensors.at(i).missed_scan < 0) {
                    kSensors.removeAt(i);
                    i--;
                }
            }
        }
#endif
#ifdef SUPPORT_LAIRDBT510
            for (int i = 0; i < lBeacons.size(); i++) {
                if (lBeacons.at(i).missed_scan < 0) {
                    lBeacons.removeAt(i);
                    i--;
                }
            }
#endif
        _scan_done = false;
    }
}

void Beaconscanner::publish(const char* eventName, int type)
{
    _eventName = eventName;
#ifdef SUPPORT_IBEACON
    if (type & SCAN_IBEACON) publish(SCAN_IBEACON);
#endif
#ifdef SUPPORT_KONTAKT
    if (type & SCAN_KONTAKT) publish(SCAN_KONTAKT);
#endif
#ifdef SUPPORT_EDDYSTONE
    if (type & SCAN_EDDYSTONE) publish(SCAN_EDDYSTONE);
#endif
#ifdef SUPPORT_LAIRDBT510
    if (type & SCAN_LAIRDBT510) publish(SCAN_LAIRDBT510);
#endif
}

void Beaconscanner::publish(int type)
{
    char *buf = new char[PUBLISH_CHUNK];
    writer = new JSONBufferWriter(buf, PUBLISH_CHUNK);
    switch (type)
    {
#ifdef SUPPORT_IBEACON
        case SCAN_IBEACON:
            Particle.publish(String::format("%s-ibeacon", _eventName), getJson(&iBeacons, std::min(IBEACON_CHUNK, iBeacons.size()), this),_pFlags);
            break;
#endif
#ifdef SUPPORT_KONTAKT
        case SCAN_KONTAKT:
            Particle.publish(String::format("%s-kontakt", _eventName), getJson(&kSensors, std::min(KONTAKT_CHUNK, kSensors.size()), this),_pFlags);
            break;
#endif
#ifdef SUPPORT_EDDYSTONE
        case SCAN_EDDYSTONE:
            Particle.publish(String::format("%s-eddystone", _eventName), getJson(&eBeacons, std::min(EDDYSTONE_CHUNK, eBeacons.size()),this), _pFlags);
            break;
#endif
#ifdef SUPPORT_LAIRDBT510
        case SCAN_LAIRDBT510:
            Particle.publish(String::format("%s-lairdbt510", _eventName), getJson(&lBeacons, std::min(LAIRDBT510_CHUNK, lBeacons.size()), this), _pFlags);
            break;
#endif
        default:
            break;
    }
    delete writer;
    delete[] buf;
}
