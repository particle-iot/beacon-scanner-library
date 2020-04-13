
#ifndef EDDYSTONE_H
#define EDDYSTONE_H

#include "Particle.h"

// Eddystone specification: https://github.com/google/eddystone/blob/master/protocol-specification.md

class Eddystone
{
public:
    Eddystone() {};
    ~Eddystone() = default;

    class Uid {
    public:
        Uid() {rssi=rssi_count=0;found=false;}
        ~Uid() = default;

        int8_t getRssi() {return (int8_t)(rssi/rssi_count);}
        int8_t getPower() {return power;}
        uint8_t* getNamespace() {return name;}
        uint8_t* getInstance() {return instance;}
        String namespaceString() {
            return String::format("%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                        name[0],name[1],name[2],name[3],name[4],
                        name[5],name[6],name[7],name[8],name[9]);
        }
        String instanceString() {
            return String::format("%02X%02X%02X%02X%02X%02X",instance[0],instance[1],instance[2],instance[3],
                        instance[4],instance[5]);
        }
        void populateData(uint8_t *buf, int8_t rssi);

        bool found;
    private:
        int16_t rssi;
        uint8_t rssi_count;
        int8_t power;
        uint8_t name[10],instance[6];
    };

    class Url {
    public:
        Url() {
            found=false;
            rssi=rssi_count=0;
            }
        ~Url() = default;
        int8_t getRssi() {return (int8_t)(rssi/rssi_count);}
        int8_t getPower() {return power;}
        String urlString();
        bool found;
        void populateData(uint8_t *buf, int8_t rssi, uint8_t packet_size);
    private:
        int16_t rssi;
        uint8_t rssi_count;
        int8_t power;
        uint8_t scheme;
        uint8_t locator[17];
        uint8_t locator_size;
    };

    class Tlm {
    public:
        Tlm() {found=false;}
        ~Tlm() = default;

        float getVbatt() {return (uint16_t)vbatt/(float)1000;}
        float getTemp() {return (float)((int8_t)temp[0]+(uint8_t)temp[1]/(float)256);}
        uint32_t getAdvCnt() {return adv_cnt;}
        uint32_t getSecCnt() {return sec_cnt;}

        bool found;
        void populateData(uint8_t *buf);
    private:
        uint16_t vbatt;
        int8_t temp[2];
        uint32_t adv_cnt;
        uint32_t sec_cnt;
    };

    void populateData(const BleScanResult *scanResult);
    static bool isBeacon(const BleScanResult *scanResult);
    void toJson(JSONBufferWriter *writer);

    BleAddress getAddress() { return address;}
    Uid getUid() {return uid;}

private:
    BleAddress address;
    Uid uid;
    Url url;
    Tlm tlm;
};

#endif