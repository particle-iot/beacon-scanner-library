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

#include "BeaconScanner.h"
#include "os-version-macros.h"

#if SYSTEM_VERSION >= SYSTEM_VERSION_DEFAULT(3, 0, 0)
#define PUBLISH_CHUNK 1024
#else
#define PUBLISH_CHUNK 622
#endif
#define IBEACON_JSON_SIZE 119
#define KONTAKT_JSON_SIZE 93
#define EDDYSTONE_JSON_SIZE 260
#define LAIRDBT510_JSON_SIZE 100
#define BTHOME_JSON_SIZE 100
#define RUUVI_JSON_SIZE 100

#define IBEACON_CHUNK       ( PUBLISH_CHUNK / IBEACON_JSON_SIZE )
#define KONTAKT_CHUNK       ( PUBLISH_CHUNK / KONTAKT_JSON_SIZE )
#define EDDYSTONE_CHUNK     ( PUBLISH_CHUNK / EDDYSTONE_JSON_SIZE )
#define LAIRDBT510_CHUNK    ( PUBLISH_CHUNK / LAIRDBT510_JSON_SIZE )
#define BTHOME_CHUNK        ( PUBLISH_CHUNK / BTHOME_JSON_SIZE )
#define RUUVI_CHUNK         ( PUBLISH_CHUNK / RUUVI_JSON_SIZE )

#define IBEACON_NONSAVER    ( 5000 / IBEACON_JSON_SIZE )
#define KONTAKT_NONSAVER    ( 5000 / KONTAKT_JSON_SIZE )
#define EDDYSTONE_NONSAVER  ( 5000 / EDDYSTONE_JSON_SIZE )
#define LAIRDBT510_NONSAVER ( 5000 / LAIRDBT510_JSON_SIZE )
#define BTHOME_NONSAVER     ( 5000 / BTHOME_JSON_SIZE )
#define RUUVI_NONSAVER      ( 5000 / RUUVI_JSON_SIZE )

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
    BLE.getScanParameters(&scanParams);
#if SYSTEM_VERSION >= SYSTEM_VERSION_RC(3, 1, 0, 1)
    if (scanParams.scan_phys != BLE_PHYS_1MBPS && 
            scanParams.scan_phys != BLE_PHYS_CODED &&
            scanParams.scan_phys != (BLE_PHYS_1MBPS | BLE_PHYS_CODED))
                scanParams.scan_phys = BLE_PHYS_1MBPS;
#endif
    scanParams.interval = 80;   // 50ms
    scanParams.window = 40;     // 25ms
    scanParams.timeout = 15;    // 150ms
    scanParams.active = true;
    scanParams.filter_policy = BLE_SCAN_FP_ACCEPT_ALL;
    BLE.setScanParameters(&scanParams); 
}

void Beaconscanner::processScan(Vector<BleScanResult> scans) {
    while(!scans.isEmpty()) {
        BleScanResult scan = scans.takeFirst();
        const BleScanResult* scanResult = &scan;
        if (false) {}
#ifdef SUPPORT_IBEACON
        else if ((_flags & SCAN_IBEACON) && iBeaconScan::isBeacon(scanResult) && !iPublished.contains(ADDRESS(scanResult)))
        {
            iBeaconScan::addOrUpdate(scanResult);
        }
#endif
#ifdef SUPPORT_KONTAKT
        else if ((_flags & SCAN_KONTAKT) && KontaktTag::isTag(scanResult) && !kPublished.contains(ADDRESS(scanResult)))
        {
            KontaktTag::addOrUpdate(scanResult);
        }
#endif
#ifdef SUPPORT_EDDYSTONE 
        else if ((_flags & SCAN_EDDYSTONE) && Eddystone::isBeacon(scanResult) && !ePublished.contains(ADDRESS(scanResult)))
        {
            Eddystone::addOrUpdate(scanResult);
        }
#endif
#ifdef SUPPORT_LAIRDBT510
        else if ((_flags & SCAN_LAIRDBT510) && LairdBt510::isBeacon(scanResult) && !lPublished.contains(ADDRESS(scanResult)))
        {
            LairdBt510::addOrUpdate(scanResult);
        }
#endif
#ifdef SUPPORT_BTHOME
        else if ((_flags & SCAN_BTHOME) && BTHome::isBeacon(scanResult) && !sPublished.contains(ADDRESS(scanResult)))
        {
            BTHome::addOrUpdate(scanResult);
        }
#endif
#ifdef SUPPORT_RUUVI
        else if ((_flags & SCAN_RUUVI) && Ruuvi::isBeacon(scanResult) && !rPublished.contains(ADDRESS(scanResult)))
        {
            Ruuvi::addOrUpdate(scanResult);
        }
#endif
        else if (_customCallback) {
            _customCallback(scanResult);
        }
    }
}

void Beaconscanner::customScan(uint16_t duration, bool rate_limit)
{
    custom_scan_params();
#ifdef SUPPORT_KONTAKT
    kPublished.clear();
    KontaktTag::beacons.clear();
#endif
#ifdef SUPPORT_IBEACON
    iPublished.clear();
    iBeaconScan::beacons.clear();
#endif
#ifdef SUPPORT_EDDYSTONE
    ePublished.clear();
    Eddystone::beacons.clear();
#endif
#ifdef SUPPORT_LAIRDBT510
    lPublished.clear();
    LairdBt510::beacons.clear();
#endif
#ifdef SUPPORT_LAIRDBT510
    lPublished.clear();
    LairdBt510::beacons.clear();
#endif
#ifdef SUPPORT_BTHOME
    sPublished.clear();
    BTHome::beacons.clear();
#endif
#ifdef SUPPORT_RUUVI
    rPublished.clear();
    Ruuvi::beacons.clear();
#endif
    long int elapsed = millis();
    while(millis() - elapsed < duration*1000)
    {
        Vector<BleScanResult> cur_responses = BLE.scan();
        processScan(cur_responses);
#ifdef SUPPORT_IBEACON
        if (_publish && (  
            (_memory_saver && iBeaconScan::beacons.size() >= IBEACON_CHUNK) ||
            (!_memory_saver && iBeaconScan::beacons.size() >= IBEACON_NONSAVER)
            ) )
        {
            for (uint8_t i = 0; i < IBEACON_CHUNK; i++)
            {
                iPublished.append(iBeaconScan::beacons.at(i).getAddress());
            }
            publish(SCAN_IBEACON, rate_limit);
        }
#endif
#ifdef SUPPORT_KONTAKT
        if (_publish && (
            (_memory_saver && KontaktTag::beacons.size() >= KONTAKT_CHUNK) ||
            (!_memory_saver && KontaktTag::beacons.size() >= KONTAKT_NONSAVER)
        ) )
        {
            for (uint8_t i = 0; i < KONTAKT_CHUNK; i++)
            {
                kPublished.append(KontaktTag::beacons.at(i).getAddress());
            }
            publish(SCAN_KONTAKT, rate_limit);
        }
#endif
#ifdef SUPPORT_EDDYSTONE
        if (_publish && (
            (_memory_saver && Eddystone::beacons.size() >= EDDYSTONE_CHUNK) ||
            (!_memory_saver && Eddystone::beacons.size() >= EDDYSTONE_NONSAVER)
        ) )
        {
            for (uint8_t i=0;i < EDDYSTONE_CHUNK;i++)
            {
                ePublished.append(Eddystone::beacons.at(i).getAddress());
            }
            publish(SCAN_EDDYSTONE, rate_limit);
        }
#endif
#ifdef SUPPORT_LAIRDBT510
        if (_publish && (
            (_memory_saver && LairdBt510::beacons.size() >= LAIRDBT510_CHUNK) ||
            (!_memory_saver && LairdBt510::beacons.size() >= LAIRDBT510_NONSAVER)
        ))
        {
            for (uint8_t i=0;i < LAIRDBT510_CHUNK;i++)
            {
                lPublished.append(LairdBt510::beacons.at(i).getAddress());
            }
            publish(SCAN_LAIRDBT510, rate_limit);
        }
#endif
#ifdef SUPPORT_BTHOME
        if (_publish && (
            (_memory_saver && BTHome::beacons.size() >= BTHOME_CHUNK) ||
            (!_memory_saver && BTHome::beacons.size() >= BTHOME_NONSAVER)
        ))
        {
            for (uint8_t i=0;i < BTHOME_CHUNK;i++)
            {
                sPublished.append(BTHome::beacons.at(i).getAddress());
            }
            publish(SCAN_BTHOME, rate_limit);
        }
#endif
#ifdef SUPPORT_RUUVI
        if (_publish && (
            (_memory_saver && Ruuvi::beacons.size() >= RUUVI_CHUNK) ||
            (!_memory_saver && Ruuvi::beacons.size() >= RUUVI_NONSAVER)
        ))
        {
            for (uint8_t i=0;i < RUUVI_CHUNK;i++)
            {
                rPublished.append(Ruuvi::beacons.at(i).getAddress());
            }
            publish(SCAN_RUUVI, rate_limit);
        }
    }
#endif
}

void Beaconscanner::scanAndPublish(uint16_t duration, int flags, const char* eventName, PublishFlags pFlags, bool memory_saver, bool rate_limit)
{
    if (_run) return;
    _flags = flags;
    _publish = true;
    _eventName = eventName;
    _pFlags = pFlags;
    _memory_saver = memory_saver;
    customScan(duration, rate_limit);
#ifdef SUPPORT_IBEACON
    while (!iBeaconScan::beacons.isEmpty())
        publish(SCAN_IBEACON, rate_limit);
#endif
#ifdef SUPPORT_KONTAKT
    while (!KontaktTag::beacons.isEmpty())
        publish(SCAN_KONTAKT, rate_limit);
#endif
#ifdef SUPPORT_EDDYSTONE
    while (!Eddystone::beacons.isEmpty())
        publish(SCAN_EDDYSTONE, rate_limit);
#endif
#ifdef SUPPORT_LAIRDBT510
    while (!LairdBt510::beacons.isEmpty())
        publish(SCAN_LAIRDBT510, rate_limit);
#endif
#ifdef SUPPORT_BTHOME
    while (!BTHome::beacons.isEmpty())
        publish(SCAN_BTHOME, rate_limit);
#endif
#ifdef SUPPORT_RUUVI
    while (!Ruuvi::beacons.isEmpty())
        publish(SCAN_RUUVI, rate_limit);
#endif
}

void Beaconscanner::scan(uint16_t duration, int flags)
{
    if (_run) return;
    _publish = false;
    _flags = flags;
    customScan(duration, false);
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
            Vector<BleScanResult> cur_responses = BLE.scan();
            _instance->processScan(cur_responses);
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
    for (auto& i : iBeaconScan::beacons) {
        if (_callback && i.newly_scanned) {
            _callback(i, NEW);
            i.newly_scanned = false;
        }
    }
#endif
#ifdef SUPPORT_EDDYSTONE
    for (auto& e : Eddystone::beacons) {
        if (_callback && e.newly_scanned) {
            _callback(e, NEW);
            e.newly_scanned = false;
        }
    }
#endif
#ifdef SUPPORT_KONTAKT
    for (KontaktTag& k : KontaktTag::beacons) {
        if (_callback && k.newly_scanned) {
            _callback(k, NEW);
            k.newly_scanned = false;
        }
    }
#endif
#ifdef SUPPORT_LAIRDBT510
    for (LairdBt510& l : LairdBt510::beacons) {
        if (_callback && l.newly_scanned) {
            _callback(l, NEW);
            l.newly_scanned = false;
        }
        l.loop();
    }
#endif
#ifdef SUPPORT_BTHOME
    for (BTHome& s : BTHome::beacons) {
        if (_callback && s.newly_scanned) {
            _callback(s, NEW);
            s.newly_scanned = false;
        }
    }
#endif
#ifdef SUPPORT_RUUVI
    for (Ruuvi& r : Ruuvi::beacons) {
        if (_callback && r.newly_scanned) {
            _callback(r, NEW);
            r.newly_scanned = false;
        }
    }
#endif

    if (_scan_done) {
#ifdef SUPPORT_IBEACON
        for (auto& i : iBeaconScan::beacons) {
            if (i.missed_scan >= _clear_missed) {
                if (_callback) {
                    _callback(i, REMOVED);
                }
                i.missed_scan = -1; // Use an invalid value to mark for removal
            } else {
                i.missed_scan++;
            }
        }
        SINGLE_THREADED_BLOCK() {
            for (int i = 0; i < iBeaconScan::beacons.size(); i++) {
                if (iBeaconScan::beacons.at(i).missed_scan < 0) {
                    iBeaconScan::beacons.removeAt(i);
                    i--;
                }
            }
        }
#endif
#ifdef SUPPORT_EDDYSTONE
        for (auto& e : Eddystone::beacons) {
            if (e.missed_scan >= _clear_missed) {
                if (_callback) {
                    _callback(e, REMOVED);
                }
                e.missed_scan = -1;
            } else {
                e.missed_scan++;
            }
        }
        SINGLE_THREADED_BLOCK() {
            for (int i = 0; i < Eddystone::beacons.size(); i++) {
                if (Eddystone::beacons.at(i).missed_scan < 0) {
                    Eddystone::beacons.removeAt(i);
                    i--;
                }
            }
        }
#endif
#ifdef SUPPORT_KONTAKT
        for (auto& k : KontaktTag::beacons) {
            if (k.missed_scan >= _clear_missed) {
                if (_callback) {
                    _callback(k, REMOVED);
                } 
                k.missed_scan = -1;
            } else {
                k.missed_scan++;
            }
        }
        SINGLE_THREADED_BLOCK() {
            for (int i = 0; i < KontaktTag::beacons.size(); i++) {
                if (KontaktTag::beacons.at(i).missed_scan < 0) {
                    KontaktTag::beacons.removeAt(i);
                    i--;
                }
            }
        }
#endif
#ifdef SUPPORT_LAIRDBT510
        for (auto& l : LairdBt510::beacons) {
            if (l.state_ == LairdBt510::State::IDLE && l.missed_scan >= _clear_missed) {
                if (_callback) {
                    _callback(l, REMOVED);
                }
                l.missed_scan = -1;
            } else {
                l.missed_scan++;
            }
        }
        SINGLE_THREADED_BLOCK() {
            for (int i = 0; i < LairdBt510::beacons.size(); i++) {
                if (LairdBt510::beacons.at(i).missed_scan < 0) {
                    LairdBt510::beacons.removeAt(i);
                    i--;
                }
            }
        }
#endif
#ifdef SUPPORT_BTHOME
        for (auto& s : BTHome::beacons) {
            if (s.missed_scan >= _clear_missed) {
                if (_callback) {
                    _callback(s, REMOVED);
                }
                s.missed_scan = -1;
            } else {
                s.missed_scan++;
            }
        }
        SINGLE_THREADED_BLOCK() {
            for (int i = 0; i < BTHome::beacons.size(); i++) {
                if (BTHome::beacons.at(i).missed_scan < 0) {
                    BTHome::beacons.removeAt(i);
                    i--;
                }
            }
        }
#endif
#ifdef SUPPORT_RUUVI
        for (auto& r : Ruuvi::beacons) {
            if (r.missed_scan >= _clear_missed) {
                if (_callback) {
                    _callback(r, REMOVED);
                }
                r.missed_scan = -1;
            } else {
                r.missed_scan++;
            }
        }
        SINGLE_THREADED_BLOCK() {
            for (int i = 0; i < Ruuvi::beacons.size(); i++) {
                if (Ruuvi::beacons.at(i).missed_scan < 0) {
                    Ruuvi::beacons.removeAt(i);
                    i--;
                }
            }
        }
#endif
        _scan_done = false;
    }
}

void Beaconscanner::publish(const char* eventName, int type, bool rate_limit)
{
    _eventName = eventName;
#ifdef SUPPORT_IBEACON
    if (type & SCAN_IBEACON) {
        while (!iBeaconScan::beacons.isEmpty()) {
            publish(SCAN_IBEACON, rate_limit);
        }
    }
#endif
#ifdef SUPPORT_KONTAKT
    if (type & SCAN_KONTAKT) {
        while (!KontaktTag::beacons.isEmpty()) {
            publish(SCAN_KONTAKT, rate_limit);
        }
    }
#endif
#ifdef SUPPORT_EDDYSTONE
    if (type & SCAN_EDDYSTONE) {
        while (!Eddystone::beacons.isEmpty()) {
            publish(SCAN_EDDYSTONE, rate_limit);
        }
    }
#endif
#ifdef SUPPORT_LAIRDBT510
    if (type & SCAN_LAIRDBT510) {
        while (!LairdBt510::beacons.isEmpty()) {
            publish(SCAN_LAIRDBT510, rate_limit);
        }
    }
#endif
#ifdef SUPPORT_BTHOME
    if (type & SCAN_BTHOME) {
        while (!BTHome::beacons.isEmpty()) {
            publish(SCAN_BTHOME, rate_limit);
        }
    }
#endif
#ifdef SUPPORT_RUUVI
    if (type & SCAN_RUUVI) {
        while (!Ruuvi::beacons.isEmpty()) {
            publish(SCAN_RUUVI, rate_limit);
        }
    }
#endif
}

void Beaconscanner::publish(int type, bool rate_limit)
{
    char *buf = new char[PUBLISH_CHUNK];
    writer = new JSONBufferWriter(buf, PUBLISH_CHUNK);
    while (millis() - _last_publish < 1000) {
        delay(50);
    }
    switch (type)
    {
#ifdef SUPPORT_IBEACON
        case SCAN_IBEACON:
            Particle.publish(String::format("%s-ibeacon", _eventName), getJson(&iBeaconScan::beacons, std::min(IBEACON_CHUNK, iBeaconScan::beacons.size()), this),_pFlags);
            break;
#endif
#ifdef SUPPORT_KONTAKT
        case SCAN_KONTAKT:
            Particle.publish(String::format("%s-kontakt", _eventName), getJson(&KontaktTag::beacons, std::min(KONTAKT_CHUNK, KontaktTag::beacons.size()), this),_pFlags);
            break;
#endif
#ifdef SUPPORT_EDDYSTONE
        case SCAN_EDDYSTONE:
            Particle.publish(String::format("%s-eddystone", _eventName), getJson(&Eddystone::beacons, std::min(EDDYSTONE_CHUNK, Eddystone::beacons.size()),this), _pFlags);
            break;
#endif
#ifdef SUPPORT_LAIRDBT510
        case SCAN_LAIRDBT510:
            Particle.publish(String::format("%s-lairdbt510", _eventName), getJson(&LairdBt510::beacons, std::min(LAIRDBT510_CHUNK, LairdBt510::beacons.size()), this), _pFlags);
            break;
#endif
#ifdef SUPPORT_BTHOME
        case SCAN_BTHOME:
            Particle.publish(String::format("%s-bthome", _eventName), getJson(&BTHome::beacons, std::min(BTHOME_CHUNK, BTHome::beacons.size()), this), _pFlags);
            break;
#endif
#ifdef SUPPORT_RUUVI
        case SCAN_RUUVI:
            Particle.publish(String::format("%s-ruuvi", _eventName), getJson(&Ruuvi::beacons, std::min(RUUVI_CHUNK, Ruuvi::beacons.size()), this), _pFlags);
            break;
#endif
        default:
            break;
    }
    _last_publish = millis();
    delete writer;
    delete[] buf;
}
