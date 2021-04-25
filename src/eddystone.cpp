#include "os-version-macros.h"
#include "eddystone.h"

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
        default:
            Log.info("Eddystone format not supported");
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