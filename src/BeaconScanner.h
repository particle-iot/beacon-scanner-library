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
  /**
   * @brief Singleton class instance access for Beaconscanner.
   *
   * @return Beaconscanner&
   */
  static Beaconscanner& instance() {
    if (!_instance) {
      _instance = new Beaconscanner();
    }
    return *_instance;
  }

  void scanAndPublish(uint16_t duration, int flags, const char* eventName, PublishFlags pFlags, bool memory_saver = false);
  void scan(uint16_t duration = 5, int flags = (SCAN_IBEACON | SCAN_KONTAKT | SCAN_EDDYSTONE));
  void startContinuous(int flags = (SCAN_IBEACON | SCAN_KONTAKT | SCAN_EDDYSTONE));
  void stopContinuous();

  void publish(const char* eventName, int type = (SCAN_IBEACON | SCAN_KONTAKT | SCAN_EDDYSTONE));
  Vector<KontaktTag> getKontaktTags() {return kSensors;};
  Vector<iBeaconScan> getiBeacons() {return iBeacons;};
  Vector<Eddystone> getEddystone() {return eBeacons;};

  template<typename T> static String getJson(Vector<T>* beacons, uint8_t count, void* context);

  JSONBufferWriter *writer;

  private:
    bool _publish, _memory_saver, _run;
    int _flags;
    uint8_t _clear_missed, _scan_period;
    PublishFlags _pFlags;
    Vector<BleAddress> kPublished, iPublished, ePublished;
    const char* _eventName;
    Vector<KontaktTag> kSensors;
    Vector<iBeaconScan> iBeacons;
    Vector<Eddystone> eBeacons;
    Thread* _thread;
    static Beaconscanner* _instance;
    static void scanChunkResultCallback(const BleScanResult *scanResult, void *context);
    static void scan_thread(void* param);
    void publish(int type);
    void customScan(uint16_t interval);
    Beaconscanner() :
        _run(false),
        _clear_missed(1),
        _scan_period(10),
        _thread(nullptr) {};
};

#endif