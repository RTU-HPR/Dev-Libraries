#pragma once
#ifdef GPS_WRAPPER_ENABLE

#include <Arduino.h>
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>

class Gps
{
  private:
    SFE_UBLOX_GNSS _gps;

    bool _gps_initialized = false;
    
    const int GPS_BEGIN_TIMEOUT = 5000;

  public:
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