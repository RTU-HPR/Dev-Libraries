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
        const int timeout;              // Time it takes for anything to timeout
        const int measurement_rate;     // how often measurement will be taken in ms
        const int navigation_frequency; // how often tu updated navigation in s
        const dynModel dynamic_model;       // DYN_MODEL_AIRBORNE2g
        const int com_settings;         // COM_TYPE_UBX
        const bool auto_pvt;                // true
    };
    struct Gps_Config_UART : Gps_Config
    {
        const HardwareSerial &serial;
    };
    struct Gps_Config_I2C : Gps_Config
    {
        const TwoWire &wire;
        const int i2c_address;
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
};

#endif