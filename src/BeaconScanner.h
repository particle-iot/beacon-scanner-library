/* beacon-scanner library by Mariano Goluboff
 */

#ifndef BEACON_SCANNER_H
#define BEACON_SCANNER_H

#include "Particle.h"
#include "iBeacon-scan.h"
#include "kontaktTag.h"
#include "eddystone.h"


typedef enum ble_scanner_config_t {
  SCAN_IBEACON         = 0x01,
  SCAN_KONTAKT         = 0x02,
  SCAN_EDDYSTONE       = 0x04
} ble_scanner_config_t;

class Beaconscanner
{
public:
  Beaconscanner() {};
  ~Beaconscanner() = default;

  void scanAndPublish(uint16_t duration, int flags, const char* eventName, PublishFlags pFlags, bool memory_saver = false);
  void scan(uint16_t duration = 5, int flags = (SCAN_IBEACON | SCAN_KONTAKT | SCAN_EDDYSTONE));

  Vector<KontaktTag> getKontaktTags() {return kSensors;};
  Vector<iBeaconScan> getiBeacons() {return iBeacons;};
  Vector<Eddystone> getEddystone() {return eBeacons;};

  template<typename T> static String getJson(Vector<T>* beacons, uint8_t count, void* context);

  JSONBufferWriter *writer;

  private:
    bool _publish, _memory_saver;
    int _flags;
    PublishFlags _pFlags;
    Vector<BleAddress> kPublished, iPublished, ePublished;
    const char* _eventName;
    Vector<KontaktTag> kSensors;
    Vector<iBeaconScan> iBeacons;
    Vector<Eddystone> eBeacons;
    static void scanChunkResultCallback(const BleScanResult *scanResult, void *context);
    void publish(int type);
    void customScan(uint16_t interval);
};

#endif