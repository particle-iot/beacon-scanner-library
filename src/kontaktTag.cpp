#include "kontaktTag.h"

void KontaktTag::populateData(const BleScanResult *scanResult)
{
    address = scanResult->address;
    uint8_t buf[BLE_MAX_ADV_DATA_LEN];
    uint8_t count = scanResult->advertisingData.get(BleAdvertisingDataType::SERVICE_DATA, buf, sizeof(buf));
    uint8_t cursor = 0;
    if (count > 3 && buf[0] == 0x6A && buf[1] == 0xFE) // Kontakt UUID
    {
        cursor += 2;
        if (buf[cursor] == 0x03) // Telemetry v1 packet
        {
            cursor++;
            while (cursor < count)
            {
                uint8_t size = buf[cursor];
                cursor++;
                switch (buf[cursor++])
                {
                case 0x01:       // System health
                    cursor += 4; // Advance to battery level
                    battery = buf[cursor++];
                    break;
                case 0x02:
                    accel_sensitivity = buf[cursor++];
                    x_axis = buf[cursor++];
                    y_axis = buf[cursor++];
                    z_axis = buf[cursor++];
                    accel_last_double_tap = buf[cursor] + buf[cursor + 1] * 256;
                    cursor += 2;
                    accel_last_movement = buf[cursor] + buf[cursor + 1] * 256;
                    cursor += 2;
                    accel_data = true;
                    break;
                case 0x05: // Light and temp
                    // Light sensor is at cursor
                    cursor++;
                    // Temp in C is at cursor
                    temperature = buf[cursor++];
                    break;
                case 0x0D: // Button press
                    button_time = buf[cursor] + buf[cursor + 1] * 256;
                    cursor += 2;
                    break;
                default:
                    cursor--;
                    char nbuf[100];
                    for (uint8_t i = 0; i < size; i++)
                    {
                        snprintf(nbuf + i * 2, sizeof(nbuf), "%02X", buf[cursor + i]);
                    }
                    nbuf[size * 2] = '\0';
                    Log.info("%s", nbuf);
                    cursor += size;
                }
            }
        }
    }
}

bool KontaktTag::isTag(const BleScanResult *scanResult)
{
    if (scanResult->advertisingData.contains(BleAdvertisingDataType::SERVICE_DATA))
    {
        uint8_t buf[BLE_MAX_ADV_DATA_LEN];
        uint8_t count = scanResult->advertisingData.get(BleAdvertisingDataType::SERVICE_DATA, buf, BLE_MAX_ADV_DATA_LEN);
        if (count > 3 && buf[0] == 0x6A && buf[1] == 0xFE) // Kontakt UUID
            return true;
    }
    return false;
}

void KontaktTag::toJson(JSONBufferWriter *writer)
{
        writer->name(address.toString()).beginObject();
        if (battery != 0xFF)
            writer->name("batt").value(battery);
        if (temperature != 0xFF)
            writer->name("temp").value(temperature);
        if (button_time != 0xFFFF)
            writer->name("button").value(button_time);
        if (accel_data)
        {
            writer->name("x_axis").value(x_axis);
            writer->name("y_axis").value(y_axis);
            writer->name("z_axis").value(z_axis);
        }
        writer->endObject();
}