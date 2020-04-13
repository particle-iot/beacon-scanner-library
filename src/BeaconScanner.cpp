/* beacon-scanner library by Mariano Goluboff
 */

#include "BeaconScanner.h"

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

template<typename T>
String Beaconscanner::getJson(Vector<T>* beacons, uint8_t count, void *context)
{
    Beaconscanner *ctx = (Beaconscanner *)context;
    Log.info("Going to get JSON");
    ctx->writer->beginObject();
    for(;count > 0;count--)
    {
        beacons->takeFirst().toJson(ctx->writer);
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
    if ((ctx->_flags & SCAN_IBEACON) && !ctx->iPublished.contains(scanResult->address) && iBeaconScan::isBeacon(scanResult))
    {
        iBeaconScan new_beacon;
        for (uint8_t i = 0; i < ctx->iBeacons.size(); i++)
        {
            if (ctx->iBeacons.at(i).getAddress() == scanResult->address)
            {
                new_beacon = ctx->iBeacons.takeAt(i);
                break;
            }
        }
        new_beacon.populateData(scanResult);
        ctx->iBeacons.append(new_beacon);
        if (ctx->_publish && (  
            (ctx->_memory_saver && ctx->iBeacons.size() >= IBEACON_CHUNK) ||
            (!ctx->_memory_saver && ctx->iBeacons.size() >= IBEACON_NONSAVER)
            ) )
        {
            for (uint8_t i = 0; i < IBEACON_CHUNK; i++)
            {
                ctx->iPublished.append(ctx->iBeacons.at(i).getAddress());
            }
            ctx->publish(SCAN_IBEACON);
        }
    } else if ((ctx->_flags & SCAN_KONTAKT) && !ctx->kPublished.contains(scanResult->address) && KontaktTag::isTag(scanResult))
    {
        KontaktTag new_tag;
        for (uint8_t i = 0; i < ctx->kSensors.size(); i++)
        {
            if (ctx->kSensors.at(i).getAddress() == scanResult->address)
            {
                new_tag = ctx->kSensors.takeAt(i);
                break;
            }
        }
        new_tag.populateData(scanResult);
        ctx->kSensors.append(new_tag);
        if (ctx->_publish && (
            (ctx->_memory_saver && ctx->kSensors.size() >= KONTAKT_CHUNK) ||
            (!ctx->_memory_saver && ctx->kSensors.size() >= KONTAKT_NONSAVER)
        ) )
        {
            for (uint8_t i = 0; i < KONTAKT_CHUNK; i++)
            {
                ctx->kPublished.append(ctx->kSensors.at(i).getAddress());
            }
            ctx->publish(SCAN_KONTAKT);
        }
    } else if ((ctx->_flags & SCAN_EDDYSTONE) && !ctx->ePublished.contains(scanResult->address) && Eddystone::isBeacon(scanResult))
    {
        Eddystone new_beacon;
        for (uint8_t i = 0; i < ctx->eBeacons.size(); i++)
        {
            if (ctx->eBeacons.at(i).getAddress() == scanResult->address)
            {
                new_beacon = ctx->eBeacons.takeAt(i);
                break;
            }
        }
        new_beacon.populateData(scanResult);
        ctx->eBeacons.append(new_beacon);
        if (ctx->_publish && (
            (ctx->_memory_saver && ctx->eBeacons.size() >= EDDYSTONE_CHUNK) ||
            (!ctx->_memory_saver && ctx->eBeacons.size() >= EDDYSTONE_NONSAVER)
        ) )
        {
            for (uint8_t i=0;i < EDDYSTONE_CHUNK;i++)
            {
                ctx->ePublished.append(ctx->eBeacons.at(i).getAddress());
            }
            ctx->publish(SCAN_EDDYSTONE);
        }
    }
}

void Beaconscanner::customScan(uint16_t duration)
{
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
    }
}

void Beaconscanner::scanAndPublish(uint16_t duration, int flags, const char* eventName, PublishFlags pFlags, bool memory_saver)
{
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
    _publish= false;
    _flags = flags;
    customScan(duration);
}

void Beaconscanner::publish(int type)
{
    char *buf = new char[PUBLISH_CHUNK];
    writer = new JSONBufferWriter(buf, PUBLISH_CHUNK);
    switch (type)
    {
        case SCAN_IBEACON:
            Particle.publish(String::format("%s-ibeacon",_eventName),getJson(&iBeacons, std::min(IBEACON_CHUNK, iBeacons.size()), this),_pFlags);
            break;
        case SCAN_KONTAKT:
            Particle.publish(String::format("%s-kontakt",_eventName),getJson(&kSensors, std::min(KONTAKT_CHUNK, kSensors.size()), this),_pFlags);
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
