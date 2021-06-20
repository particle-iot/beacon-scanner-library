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

#include "eddystone.h"

Vector<Eddystone> Eddystone::beacons;

void Eddystone::populateData(const BleScanResult *scanResult)
{
    address = ADDRESS(scanResult);
    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::SERVICE_DATA, buf, sizeof(buf));
    if (count > 2 && buf[0] == 0xAA && buf[1] == 0xFE) // Eddystone UUID
    {
        switch (buf[2])
        {
        case 0x00:
            if (count > 19)
                uid.populateData(buf, RSSI(scanResult));
            break;
        case 0x10:
            if (count > 5)
                url.populateData(buf, RSSI(scanResult), count);
            break;
        case 0x20:
            if (count == 16)      // According to the spec, packet length must be 16
                tlm.populateData(buf);
            break;
#ifdef SUPPORT_KKMSMART
        case 0x21:
            if (count >= 5) kkm.populateData(buf, count);
            break;
#endif
        default:
            Log.info("Eddystone format not supported: %02X", buf[2]);
            break;
        }
    }
}

bool Eddystone::isBeacon(const BleScanResult *scanResult)
{
    if (ADVERTISING_DATA(scanResult).contains(BleAdvertisingDataType::SERVICE_DATA))
    {
        uint8_t buf[BLE_MAX_ADV_DATA_LEN];
        uint8_t count = ADVERTISING_DATA(scanResult).get(BleAdvertisingDataType::SERVICE_DATA, buf, BLE_MAX_ADV_DATA_LEN);
        if (count > 3 && buf[0] == 0xAA && buf[1] == 0xFE) // Eddystone UUID
            return true;
    }
    return false;
}

void Eddystone::toJson(JSONWriter *writer) const
{
        writer->name(address.toString()).beginObject();
        if (uid.found) 
        {
            writer->name("uid").beginObject();
            writer->name("power").value(uid.getPower());
            writer->name("namespace").value(uid.namespaceString());
            writer->name("instance").value(uid.instanceString());
            writer->name("rssi").value(uid.getRssi());
            writer->endObject();
        }
        if (url.found)
        {
            writer->name("url").beginObject();
            writer->name("url").value(url.urlString());
            writer->name("power").value(url.getPower());
            writer->name("rssi").value(url.getRssi());
            writer->endObject();
        }
        if (tlm.found)
        {
            writer->name("tlm").beginObject();
            writer->name("vbatt").value(tlm.getVbatt());
            writer->name("temp").value(tlm.getTemp());
            writer->name("adv_cnt").value((unsigned int)tlm.getAdvCnt());
            writer->name("sec_cnt").value((unsigned int)tlm.getSecCnt());
            writer->endObject();
        }
#ifdef SUPPORT_KKMSMART
        if (kkm.found)
        {
            writer->name("kkm").beginObject();
            writer->name("vbatt").value(kkm.getVbatt());
            writer->name("temp").value(kkm.getTemp());
            if (kkm.hasAccelData()) {
                writer->name("x_axis").value(kkm.getAccelXaxis());
                writer->name("y_axis").value(kkm.getAccelYaxis());
                writer->name("z_axis").value(kkm.getAccelZaxis());
            }
            writer->endObject();
        }
#endif
        writer->endObject();
}

void Eddystone::Uid::populateData(uint8_t *buf, int8_t rssi)
{
    found = true;
    power = (int8_t)buf[3];
    memcpy(name,buf+4,10);
    memcpy(instance, buf+14,6);
    this->rssi+=rssi;
    rssi_count++;
}

void Eddystone::Url::populateData(uint8_t *buf, int8_t rssi, uint8_t packet_size)
{
    found = true;
    power = (int8_t)buf[3];
    scheme = (uint8_t)buf[4];
    locator_size = packet_size - 5;
    memcpy(locator, buf+5,locator_size);
    this->rssi+=rssi;
    rssi_count++;
}

void Eddystone::Tlm::populateData(uint8_t *buf)
{
    if (buf[3] == 0x00)     // Version. Only one that exists right now
    {
        found = true;
        vbatt = (buf[4]<<8)+buf[5];
        memcpy(temp, buf+6, 2);
        adv_cnt = (buf[8]<<24)+(buf[9]<<16)+(buf[10]<<8)+buf[11];
        sec_cnt = (buf[12]<<24)+(buf[13]<<16)+(buf[14]<<8)+buf[15];
    }
}

#ifdef SUPPORT_KKMSMART
#define KKM_SENSOR_MASK_VOLTAGE     0x1
#define KKM_SENSOR_MASK_TEMP        0x2
#define KKM_SENSOR_MASK_HUME        0x4
#define KKM_SENSOR_MASK_ACC_AIX     0x8
void Eddystone::Kkm::populateData(uint8_t *buf, uint8_t size) {
    found = true;
    uint8_t cursor = 3;
    //uint8_t version = buf[cursor++];
    cursor++;   // Currently not using version. Remove this statement if version is uncommented out.
    uint8_t sensorMask = buf[cursor++];
    if ( (sensorMask & KKM_SENSOR_MASK_VOLTAGE) != 0) {
        if ( cursor + 2 > size) return;
        vbatt = buf[cursor] << 8 | buf[cursor+1];
        cursor += 2;
    }
    if ( (sensorMask & KKM_SENSOR_MASK_TEMP) != 0) {
        if ( cursor + 2 > size) return;
        temp_integer = buf[cursor++];
        temp_fraction = buf[cursor++]; 
    }
    if ( (sensorMask & KKM_SENSOR_MASK_HUME) != 0) {
        if (cursor + 2 > size) return;
        // TODO: Add humidity
        cursor +=2;
    }
    if ( (sensorMask & KKM_SENSOR_MASK_ACC_AIX) != 0) {
        if (cursor + 6 > size) return;
        accel_data = true;
        x_axis = buf[cursor] << 8 | buf[cursor+1];
        y_axis = buf[cursor+2] << 8 | buf[cursor+3];
        z_axis = buf[cursor+4] << 8 | buf[cursor+5];
        cursor += 6;
    }
}
#endif

String Eddystone::Url::urlString() const
{
    char buf[50];
    uint8_t cursor=0;
    switch(scheme)
    {
        case 0x00:
            cursor+=snprintf(buf,sizeof(buf),"http://www.");
            break;
        case 0x01:
            cursor+=snprintf(buf,sizeof(buf),"https://www.");
            break;
        case 0x02:
            cursor+=snprintf(buf,sizeof(buf),"http://");
            break;
        case 0x03:
            cursor+=snprintf(buf,sizeof(buf),"https://");
            break;
        default:
            break;
    };
    for(uint8_t i=0;i<locator_size;i++)
    {
        switch(locator[i])
        {
            case 0x00:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".com/");
                break;
            case 0x01:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".org/");
                break;
            case 0x02:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".edu/");
                break;
            case 0x03:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".net/");
                break;
            case 0x04:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".info/");
                break;
            case 0x05:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".biz/");
                break;
            case 0x06:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".gov/");
                break;
            case 0x07:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".com");
                break;
            case 0x08:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".org");
                break;
            case 0x09:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".edu");
                break;
            case 0x0a:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".net");
                break;
            case 0x0b:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".info");
                break;
            case 0x0c:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".biz");
                break;
            case 0x0d:
                cursor+=snprintf(buf+cursor,sizeof(buf)-cursor,".gov");
                break;
            default:
                buf[cursor++] = locator[i];
        }
    }
    return String::format("%.*s", cursor, buf);
}

void Eddystone::addOrUpdate(const BleScanResult *scanResult)
{
    uint8_t i;
    for (i = 0; i < beacons.size(); ++i) {
        if (beacons.at(i).getAddress() == ADDRESS(scanResult)) {
            break;
        }
    }
    if (i == beacons.size()) {
        Eddystone new_beacon;
        new_beacon.populateData(scanResult);
        new_beacon.missed_scan = 0;
        beacons.append(new_beacon);
    } else {
        Eddystone& beacon = beacons.at(i);
        beacon.newly_scanned = false;
        beacon.populateData(scanResult);
        beacon.missed_scan = 0;
    }
}