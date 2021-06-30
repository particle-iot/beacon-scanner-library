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

#ifndef EDDYSTONE_H
#define EDDYSTONE_H

#include "beacon.h"

// Eddystone specification: https://github.com/google/eddystone/blob/master/protocol-specification.md

class Eddystone : public Beacon
{
public:
    Eddystone() : Beacon(SCAN_EDDYSTONE) {};
    ~Eddystone() = default;

    class Uid {
    public:
        Uid() {rssi=rssi_count=0;found=false;}
        ~Uid() = default;

        int8_t getRssi() const {return (int8_t)(rssi/rssi_count);}
        int8_t getPower() const {return power;}
        uint8_t* getNamespace() {return name;}
        uint8_t* getInstance() {return instance;}
        String namespaceString() const {
            return String::format("%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
                        name[0],name[1],name[2],name[3],name[4],
                        name[5],name[6],name[7],name[8],name[9]);
        }
        String instanceString() const {
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
        int8_t getRssi() const {return (int8_t)(rssi/rssi_count);}
        int8_t getPower() const {return power;}
        String urlString() const;
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

        float getVbatt() const {return (uint16_t)vbatt/(float)1000;}
        float getTemp() const {return (float)((int8_t)temp[0]+(uint8_t)temp[1]/(float)256);}
        uint32_t getAdvCnt() const {return adv_cnt;}
        uint32_t getSecCnt() const {return sec_cnt;}

        bool found;
        void populateData(uint8_t *buf);
    private:
        uint16_t vbatt;
        int8_t temp[2];
        uint32_t adv_cnt;
        uint32_t sec_cnt;
    };

#ifdef SUPPORT_KKMSMART
// https://www.kkmcn.com/waterproof-beacon-k8
    class Kkm {
    public:
        Kkm() {
            found = false;
            accel_data = false;
        }
        ~Kkm() = default;
        uint16_t getVbatt() const { return vbatt; };
        float getTemp() const { 
            if (temp_integer > 0) {
                return (float)(temp_integer+temp_fraction/(float)256); 
            }
            return (float)(temp_integer-temp_fraction/(float)256);
        };
        bool hasAccelData() const { return accel_data; };
        int16_t getAccelXaxis() const { return x_axis; };
        int16_t getAccelYaxis() const { return y_axis; };
        int16_t getAccelZaxis() const { return z_axis; };
        bool found;
        void populateData(uint8_t *buf, uint8_t size);
    private:
        uint16_t vbatt;
        int8_t temp_integer;
        uint8_t temp_fraction;
        int16_t x_axis, y_axis, z_axis;
        bool accel_data;
    };
#endif

    void toJson(JSONWriter *writer) const override;

    Uid getUid() const {return uid;}
    Url getUrl() const {return url;}
    Tlm getTlm() const {return tlm;}
#ifdef SUPPORT_KKMSMART
    Kkm getKkm() const {return kkm;}
#endif

private:
    Uid uid;
    Url url;
    Tlm tlm;
#ifdef SUPPORT_KKMSMART
    Kkm kkm;
#endif
    friend class Beaconscanner;
    static Vector<Eddystone> beacons;
    void populateData(const BleScanResult *scanResult) override;
    static bool isBeacon(const BleScanResult *scanResult);
    static void addOrUpdate(const BleScanResult *scanResult);
};

#endif