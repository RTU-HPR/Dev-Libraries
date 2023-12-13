
#include <Arduino.h>
#include <Gps_wrapper.h>
#include <EEPROM.h> // doesn't compile without this

const int GPS_RX = 9;
const int GPS_TX = 8;
const int BAUD_RATE_GPS = 9600; // this needs extra setup when changing

Gps_Wrapper::Gps_Config_UART gps_config{
    .config = {
        .timeout = 5000,                       // Time it takes for anything to timeout
        .measurement_rate = 500,               // how often measurement will be taken in ms
        .navigation_frequency = 2,             // how often tu updated navigation in s
        .dynamic_model = DYN_MODEL_AIRBORNE2g, // DYN_MODEL_AIRBORNE2g
        .com_settings = COM_TYPE_UBX,          // COM_TYPE_UBX
        .auto_pvt = true                       // for neo6m dont use this
    },
    .serial = &Serial1};

Gps_Wrapper *gps;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
    {
        delay(5);
    }
    delay(500);
    Serial.println("Starting gps");

    gps_config.serial->begin(BAUD_RATE_GPS, SERIAL_8N1, GPS_RX, GPS_TX);
    delay(2000);
    if (!*(gps_config.serial))
    {
        while (true)
        {
            Serial.println("gps serial fail begin");
            delay(1000);
        }
    }

    gps = new Gps_Wrapper(nullptr, "GPS");

    if (gps->begin(gps_config))
    {

        while (!gps->begin(gps_config))
        {
            Serial.println("gps error in begin");
            delay(1000);
        }
        Serial.println("gps revived in begin");
    }
    else
    {
        Serial.println("gps begin good");
    }
}
void loop()
{
    delay(500);
    Gps_Wrapper::Gps_Data data;
    if (gps->read(data))
    {
        Serial.println("lat:" + String(data.lat, 7) + "  lng:" + String(data.lng, 7) + " satellites:" + String(data.satellites) + " time:" + String(data.epoch_time));
    }
    else
    {
        Serial.println("data not read");
    }
}