#pragma once
#ifdef GPS_WRAPPER_ENABLE

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "Sensor_wrapper.h"

class Gps_Wrapper : public Sensor_Wrapper
{
private:
    SFE_UBLOX_GNSS _gps;

public:
    // this is a base for the other 2 configs i2c and uart
    struct Gps_Config
    {
        uint16_t timeout;             // Time it takes for anything to timeout
        uint16_t measurement_rate;    // how often measurement will be taken in ms
        uint8_t navigation_frequency; // how often tu updated navigation in s
        dynModel dynamic_model;       // DYN_MODEL_AIRBORNE2g
        uint8_t com_settings;         // COM_TYPE_UBX
        bool auto_pvt;                // true
    };
    struct Gps_Config_I2C
    {
        Gps_Config config;
        TwoWire *wire;
        int i2c_address = 0x42;
    };
    struct Gps_Config_UART
    {
        Gps_Config config;
        HardwareSerial *serial;
        // Constructor that takes Gps_Config instance and a HardwareSerial reference
    };
    struct Gps_Data
    {
        double lat;               // Latitude
        double lng;               // Longitude
        float altitude;           // Altitude
        int satellites;           // Satellites in view
        float speed;              // Speed
        float heading;            // Heading
        float pdop;               // GPS Precision
        unsigned long epoch_time; // Time in unix
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
    };
    Gps_Data data;

    bool begin(HardwareSerial &serial, int gps_serial_rx, int gps_serial_tx);
    bool begin(TwoWire &wire, int i2c_address);
    void readGps();
    /**
     * @brief
     *
     * @param config
     * @return true config saved
     * @return false config not saved
     */
    bool configure(const Gps_Config &config);
};
#endif