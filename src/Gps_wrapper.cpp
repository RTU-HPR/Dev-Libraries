#ifdef GPS_WRAPPER_ENABLE
#include "gps_wrapper.h"

bool Gps::begin(TwoWire &wire, int i2c_address = 0x42)
{
  // In a while loop, because it can take a few tries to work
  while (_gps.begin(wire, i2c_address) == false)
  {
  }

  // Set the I2C port to output UBX only (turn off NMEA noise)
  if (!_gps.setI2COutput(COM_TYPE_UBX))
  {
    Serial.println("GPS failed setting the I2C output");
    return false;
  }

  // How often (in ms) to update the GPS
  if (_gps.setMeasurementRate(500))
  {
    Serial.println("GPS failed setting the measurement rate");
    return false;
  }
  
  // Produce two solutions per second
  if (_gps.setNavigationFrequency(2))
  {
    Serial.println("GPS failed setting the navigation frequency");
    return false;
  }

  // Tell the GNSS to "send" each solution
  if (_gps.setAutoPVT(true))
  {
    Serial.println("GPS failed setting the auto PVT");
    return false;
  }           

  if (!_gps.setDynamicModel(DYN_MODEL_AIRBORNE2g)) // Set the dynamic model to Airborne2g
  {
    Serial.println("GPS failed setting the dynamic model");
    return false;
  }
  
  _gps.saveConfiguration(); //Save the current settings to flash and BBR
  _gps_initialized = true;

  return true;
}

bool Gps::begin(HardwareSerial &gps_serial, int gps_serial_rx, int gps_serial_tx)
{
  gps_serial.begin(38400, SERIAL_8N1, gps_serial_rx, gps_serial_tx);
  if (!_gps.begin(gps_serial))
  {
    Serial.println("GPS failed starting starting with 38400 baud rate");
    Serial.println("GPS trying to start with 9600 baud rate");

    gps_serial.begin(9600, SERIAL_8N1, gps_serial_rx, gps_serial_tx);

    if (_gps.begin(gps_serial)) 
    {
      Serial.println("GPS connected at 9600 baud");
    }
    else
    {
      Serial.println("GPS failed starting with 9600 baud rate");
      return false;
    }
  } 

  //Set the UART port to output UBX only
  if (gps_serial == Serial1)
  {
    if (!_gps.setUART1Output(COM_TYPE_UBX))
    {
      Serial.println("GPS failed setting the UART output");
      return false;
    }
  }
  else if (gps_serial == Serial2)
  {
    if (!_gps.setUART2Output(COM_TYPE_UBX))
    {
      Serial.println("GPS failed setting the UART output");
      return false;
    }
  }

  // Set the I2C port to output UBX only (turn off NMEA noise)
  if (!_gps.setI2COutput(COM_TYPE_UBX))
  {
    Serial.println("GPS failed setting the I2C output");
    return false;
  }

  // How often (in ms) to update the GPS
  if (_gps.setMeasurementRate(500))
  {
    Serial.println("GPS failed setting the measurement rate");
    return false;
  }
  
  // Produce two solutions per second
  if (_gps.setNavigationFrequency(2))
  {
    Serial.println("GPS failed setting the navigation frequency");
    return false;
  }

  // Tell the GNSS to "send" each solution
  if (_gps.setAutoPVT(true))
  {
    Serial.println("GPS failed setting the auto PVT");
    return false;
  }           

  if (!_gps.setDynamicModel(DYN_MODEL_AIRBORNE2g)) // Set the dynamic model to Airborne2g
  {
    Serial.println("GPS failed setting the dynamic model");
    return false;
  }

  _gps.saveConfiguration(); //Save the current settings to flash and BBR
  _gps_initialized = true;

  return true;
}

void Gps::readGps()
{
  if (!_gps_initialized)
  {
    return;
  }

  if (_gps.getPVT() && (_gps.getInvalidLlh() == false))
  {
    long new_gps_lat_raw = _gps.getLatitude();
    long new_gps_lng_raw = _gps.getLongitude();

    double new_gps_lat = new_gps_lat_raw / 10000000.0;
    double new_gps_lng = new_gps_lng_raw / 10000000.0;

    // SANITY CHECK
    // Check if location is 0 (not yet established) or somewhere in the northern eastern Europe
    if ((new_gps_lat == 0 && new_gps_lng == 0) || ((50 <= new_gps_lat && new_gps_lat <= 60) && (15 <= new_gps_lng && new_gps_lng <= 35)))
    {
        data.lat = new_gps_lat;
        data.lng = new_gps_lng;
        data.altitude = _gps.getAltitude() / 1000.0;
        data.satellites = _gps.getSIV();
        data.speed = _gps.getGroundSpeed() / 1000.0;
        data.heading = _gps.getHeading() / 10000.0;
        data.pdop = _gps.getPDOP() / 100.0;
        data.epoch_time = _gps.getUnixEpoch();
        data.year = _gps.getYear();
        data.month = _gps.getMonth();
        data.day = _gps.getDay();
        data.hour = _gps.getHour();
        data.minute = _gps.getMinute();
        data.second = _gps.getSecond();
    }
    else
    {
        Serial.println("GPS location is not correct: " + String(new_gps_lat, 6) + " " + String(new_gps_lng, 6));
    }
  }
}
#endif 