/* beacon-scanner library by Mariano Goluboff
 */

#ifndef BEACON_SCANNER_H
#define BEACON_SCANNER_H

#include "Particle.h"
#include "iBeacon-scan.h"
#include "kontaktTag.h"
#include "eddystone.h"

// This is the type that will be returned in the callback function, whether a tag has
// entered the area of the device, or left the area.
typedef enum {
  NEW        = 0x01,
  REMOVED    = 0x02
} callback_type;

typedef void (*BeaconScanCallback)(Beacon& beacon, callback_type type);
typedef void (*CustomBeaconCallback)(const BleScanResult *scanResult);

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

  /**
   * The device will scan for BLE advertisements, and use Particle.publish() to send the data to the cloud.
   * 
   * This is a blocking call.
   * 
   * @param duration  How long to scan for, in seconds
   * @param flags     Which type of beacons to scan for
   * @param eventName Name of the event to publish, will be appended with -<beacon type>
   * @param pFlags    Publish flags, such as PRIVATE
   * @param memory_saver  Publish more often, to reduce the size of the Vectors
   */
  void scanAndPublish(uint16_t duration, int flags, const char* eventName, PublishFlags pFlags, bool memory_saver = false);
  /**
   * The device will scan for BLE advertisements, and store the results in the Vectors for each beacon type.
   * 
   * This is a blocking call.
   * 
   * @param duration  How long to scan for, in seconds. Default: 5 seconds
   * @param flags     Which type of beacons to scan for. Default: all
   */
  void scan(uint16_t duration = 5, int flags = (SCAN_IBEACON | SCAN_KONTAKT | SCAN_EDDYSTONE));

  /**
   * The device will continuously scan on a separate thread, not blocking the main application. The
   * beacons will be stored in the corresponding Vectors.
   * 
   * It is recommended that when using continuous mode, the application call Scanner.loop() periodically
   * (maybe in the application's loop()) so that beacons that go out of range can be removed from
   * the vectors. Calling loop() is required for callbacks to function.
   * 
   * @param flags   Which type of beacons to scan for. Default: all
   */
  void startContinuous(int flags = (SCAN_IBEACON | SCAN_KONTAKT | SCAN_EDDYSTONE));
  /**
   * Suspend the thread that scans continuously.
   */
  void stopContinuous();
  /**
   * For continuous mode, set the period, in seconds, after which a device is considered
   * to have missed a scan. Default is 10.
   */
  void setScanPeriod(uint8_t seconds) {
    if (seconds > 0) _scan_period = seconds;
   }
  /**
   * When in continuous mode, a beacon will be removed from the Vector of beacons after
   * we have missed seeing it for X number of scan periods. Must be 1 or larger. This will
   * also affect when callbacks are issued for beacons going out of range.
   * 
   * Must periodically call Scanner.loop() for this to function.
   * 
   * @param count   Number of missed scan periods after which beacon will be removed. Default is 1.
   */
  void setMissedCount(uint8_t count) { 
    if (count > 0) _clear_missed = count; 
  };
  /** 
   * Register a callback that will be called when a new beacon enters or leaves the area.
   * Leaving the area is determined by the missed count previously set.
   * 
   * This works in continuous mode only. Must periodically call Scanner.loop() for this to
   * function.
   * 
   * @param callback  The function to be called
   */
  void setCallback(BeaconScanCallback callback) { _callback = callback; };
  /**
   * Call loop from the application in order to have callbacks as well as missed beacon
   * removal work.
   * 
   * Must call it periodically while in continuous mode.
   */
  void loop();

  /**
   * Register a callback that will be called when a broadcast is scanned, but it doesn't
   * match any of the known beacons. This is intended for applications that want to detect
   * custom beacons without modifying the library itself.
   */
  void setCallback(CustomBeaconCallback callback) { _customCallback = callback; };
  /**
   * Calling this will automatically publish the beacons that have been scanned. It will also
   * consume all stored beacons in the vectors.
   * 
   * This function is normally used after running scan(). It can also be used during continuous
   * mode, but it is recommended to not use both this function and callbacks at the same time, since this call
   * consumes the vectors, "ENTER" type callbacks will be issued when the beacons are detected again.
   * 
   * @param eventName the name of the event to publish. The library will add -<beacon-type> to the event name
   * @param type      the type of beacons to publish. If blank, it'll publish all
   */
  void publish(const char* eventName, int type = (SCAN_IBEACON | SCAN_KONTAKT | SCAN_EDDYSTONE));

  /**
   * Get Vectors of the tags that have been detected
   * 
   */
  Vector<KontaktTag> getKontaktTags() {return kSensors;};
  Vector<iBeaconScan> getiBeacons() {return iBeacons;};
  Vector<Eddystone> getEddystone() {return eBeacons;};

  template<typename T> static String getJson(Vector<T>* beacons, uint8_t count, void* context);

  JSONBufferWriter *writer;

  private:
    bool _publish, _memory_saver, _run, _scan_done;
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
    BeaconScanCallback _callback;
    CustomBeaconCallback _customCallback;
    Beaconscanner() :
        _run(false),
        _scan_done(false),
        _clear_missed(1),
        _scan_period(10),
        _thread(nullptr),
        _callback(nullptr),
        _customCallback(nullptr) {};
};

#define Scanner Beaconscanner::instance()

#endif