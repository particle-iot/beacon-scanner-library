/* beacon-scanner library by Mariano Goluboff
 */

#include "BeaconScanner.h"

String Beaconscanner::kontaktJson()
{
    char buf[PUBLISH_CHUNK];
    JSONBufferWriter writer(buf, PUBLISH_CHUNK);
    writer.beginObject();
    while (!kSensors.isEmpty())
    {
        KontaktTag pub = kSensors.takeFirst();
        pub.toJson(&writer);
    }
    writer.endObject();
    return String::format("%.*s", writer.dataSize(), writer.buffer());
}

String Beaconscanner::ibeaconJson()
{
    char buf[PUBLISH_CHUNK];
    JSONBufferWriter writer(buf, PUBLISH_CHUNK);
    writer.beginObject();
    while (!iBeacons.isEmpty())
    {
        iBeaconScan pub = iBeacons.takeFirst();
        pub.toJson(&writer);
    }
    writer.endObject();
    return String::format("%.*s", writer.dataSize(), writer.buffer());
}

void Beaconscanner::scanChunkResultCallback(const BleScanResult *scanResult, void *context)
{
    Beaconscanner *ctx = (Beaconscanner *)context;
    if (ctx->_iBeaconScan && !ctx->iPublished.contains(scanResult->address) && iBeaconScan::isBeacon(scanResult))
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
        if (ctx->_iBeaconPublish && ctx->iBeacons.size() > 3)
        {
            for (uint8_t i = 0; i < ctx->iBeacons.size(); i++)
            {
                ctx->iPublished.append(ctx->iBeacons.at(i).getAddress());
            }
            Particle.publish(String::format("%s-ibeacon", ctx->_eventName), ctx->ibeaconJson(), ctx->_pFlags);
        }
    }
    if (ctx->_kontaktScan && !ctx->kPublished.contains(scanResult->address) && KontaktTag::isTag(scanResult))
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
        if (ctx->_kontaktPublish && ctx->kSensors.size() > 3)
        {
            for (uint8_t i = 0; i < ctx->kSensors.size(); i++)
            {
                ctx->kPublished.append(ctx->kSensors.at(i).getAddress());
            }
            Particle.publish(String::format("%s-kontakt", ctx->_eventName), ctx->kontaktJson(), ctx->_pFlags);
        }
    }
}

void Beaconscanner::customScan(uint16_t duration)
{
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
    long int elapsed = millis();
    while(millis() - elapsed < duration*1000)
    {
        BLE.scan(scanChunkResultCallback, this);
    }
}

void Beaconscanner::scanAndPublish(uint16_t duration, int flags, const char* eventName, PublishFlags pFlags)
{
    _iBeaconPublish = _iBeaconScan = (flags & SCAN_IBEACON);
    _kontaktPublish = _kontaktScan = (flags & SCAN_KONTAKT);
    _eventName = eventName;
    _pFlags = pFlags;
    customScan(duration);
    if (_iBeaconPublish && !iBeacons.isEmpty())
    {
        Particle.publish(String::format("%s-ibeacon",eventName),ibeaconJson(),_pFlags);
    }
    if (_kontaktPublish && !kSensors.isEmpty())
    {
        Particle.publish(String::format("%s-kontakt",eventName),kontaktJson(),_pFlags);
    }
}

void Beaconscanner::scan(uint16_t duration, int flags)
{
    _iBeaconPublish = _kontaktPublish = false;
    _iBeaconScan = (flags & SCAN_IBEACON);
    _kontaktScan = (flags & SCAN_KONTAKT);
    customScan(duration);
}
