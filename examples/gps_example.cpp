
#include <Arduino.h>
#include <Gps_wrapper.h>

Gps_Wrapper::Gps_Config_UART gps_config{
    .config = {
        .timeout = 5000,                       // Time it takes for anything to timeout
        .measurement_rate = 500,               // how often measurement will be taken in ms
        .navigation_frequency = 2,             // how often tu updated navigation in s
        .dynamic_model = DYN_MODEL_AIRBORNE2g, // DYN_MODEL_AIRBORNE2g
        .com_settings = COM_TYPE_UBX,          // COM_TYPE_UBX
        .auto_pvt = true                       // true}
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

    gps_config.serial->begin(9600);
    delay(2000);
    if (!*(gps_config.serial))
    {
        Serial.println("gps beign failed");
    }

    gps = new Gps_Wrapper(nullptr, "GPS");

    if (gps->begin(gps_config))
    {
        Serial.println("gps error in begin");
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
        Serial.println("lat:" + String(data.Latitude .7) + "  lon:" + String(data.Longitude));
    }
    else
    {
        Serial.println("data not read");
    }
}