/* beacon-scanner library by Mariano Goluboff
 */

#ifndef BEACON_SCANNER_H
#define BEACON_SCANNER_H

#include "Particle.h"
#include "iBeacon-scan.h"
#include "kontaktTag.h"

#define PUBLISH_CHUNK 622

typedef enum ble_scanner_config_t {
  SCAN_IBEACON         = 0x01,
  SCAN_KONTAKT         = 0x02
} ble_scanner_config_t;

class Beaconscanner
{
public:
  Beaconscanner() {};
  ~Beaconscanner() = default;

  void scanAndPublish(uint16_t duration, int flags, const char* eventName, PublishFlags pFlags);
  void scan(uint16_t duration, int flags);
  void scan(uint16_t duration) {scan(duration, 0xFF);};
  void scan() {scan(5,0xFF);};

  Vector<KontaktTag> getKontaktTags() {return kSensors;};
  Vector<iBeaconScan> getiBeacons() {return iBeacons;};

  String kontaktJson();
  String ibeaconJson();

  private:
    bool _iBeaconPublish, _kontaktPublish, _iBeaconScan, _kontaktScan;
    PublishFlags _pFlags;
    Vector<BleAddress> kPublished, iPublished;
    const char* _eventName;
    Vector<KontaktTag> kSensors;
    Vector<iBeaconScan> iBeacons;
    static void scanChunkResultCallback(const BleScanResult *scanResult, void *context);
    void customScan(uint16_t interval);
};

#endif