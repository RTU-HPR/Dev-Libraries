#ifdef GPS_WRAPPER_ENABLE
#include "gps_wrapper.h"

Gps_Wrapper::Gps_Wrapper(void (*error_function)(String), String sensor_name) : Sensor_Wrapper(sensor_name, error_function)
{
    return;
}

bool Gps_Wrapper::begin(Gps_Config_I2C config_i2c)
{
    // do the i2c specifics
    // start timeout
    unsigned long start_time = millis();
    while (!_gps.begin(*(config_i2c.wire), config_i2c.i2c_address))
    {
        if ((start_time + config_i2c.config.timeout) > millis())
        {
            error("I2C begin timeout");
            return false;
        }
        delay(5);
    }

    // configure module
    if (!configure(config_i2c.config))
    {
        error("Configure failed");
        return false;
    }

    set_initialized(true);
    return true;
}

bool Gps_Wrapper::begin(Gps_Config_UART config_uart)
{
    // do the uart specifics
    if (!*(config_uart.serial))
    {
        error("Serial not started: " + String(*(config_uart.serial)));
    }

    if (_gps.begin(*(config_uart.serial)))
    {
        //_gps.setSerialRate(38400);
    }
    else
    {
        error("Failed begin module");
        return false;
    }

    if (*(config_uart.serial) == Serial1)
    {
        if (!_gps.setUART1Output(config_uart.config.com_settings, config_uart.config.timeout))
        {
            error("Failed setting the UART1 output to: " + String(config_uart.config.com_settings));
            return false;
        }
    }
    else if (*(config_uart.serial) == Serial2)
    {
        if (!_gps.setUART2Output(config_uart.config.com_settings, config_uart.config.timeout))
        {
            error("Failed setting the UART2 output to: " + String(config_uart.config.com_settings));
            return false;
        }
    }
    else
    {
        error("Bad UART port: " + String(*(config_uart.serial)));
    }

    if (!_gps.saveConfiguration(config_uart.config.timeout))
    {
        error("Begin: Timeout when saving config");
        return false;
    }

    // configure the module
    if (!configure(config_uart.config))
    {
        error("Configure failed");
        return false;
    }

    set_initialized(true);
    return true;
}

bool Gps_Wrapper::read(Gps_Data &data, bool &position_valid, bool &time_valid)
{
    position_valid = false;
    time_valid = false;
    if (!get_initialized())
    {
        return false;
    }
    if (!_gps.getPVT())
    {
        return false;
    }

    if (_gps.getTimeValid())
    {
        data.epoch_time = _gps.getUnixEpoch();
        data.year = _gps.getYear();
        data.month = _gps.getMonth();
        data.day = _gps.getDay();
        data.hour = _gps.getHour();
        data.minute = _gps.getMinute();
        data.second = _gps.getSecond();
        time_valid = true;
    }
    if (_gps.getInvalidLlh() == false)
    {
        long new_gps_lat_raw = _gps.getLatitude();
        long new_gps_lng_raw = _gps.getLongitude();

        double new_gps_lat = new_gps_lat_raw / 10000000.0;
        double new_gps_lng = new_gps_lng_raw / 10000000.0;
        int new_satellites = _gps.getSIV();

        // SANITY CHECK
        // Check if location is somewhere in the northern eastern Europe adn we have more than 3 new_satellites
        if (new_satellites > 3)
        {
            if (((50 <= new_gps_lat && new_gps_lat <= 60) && (15 <= new_gps_lng && new_gps_lng <= 35)))
            {
                data.lat = new_gps_lat;
                data.lng = new_gps_lng;
                data.altitude = _gps.getAltitude() / 1000.0;
                data.satellites = new_satellites;
                data.speed = _gps.getGroundSpeed() / 1000.0;
                data.heading = _gps.getHeading() / 10000.0;
                data.pdop = _gps.getPDOP() / 100.0;
                position_valid = true;
            }
            else
            {
                error("GPS location doesn't meet minimum: " + String(new_gps_lat, 6) + " " + String(new_gps_lng, 6) + " " + String(new_satellites));
            }
        }
    }
    if (time_valid || position_valid)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool Gps_Wrapper::configure(const Gps_Config &config)
{

    // How often (in ms) to update the GPS
    if (!_gps.setMeasurementRate(config.measurement_rate, config.timeout))
    {
        error("Failed setting the measurement rate to: " + String(config.measurement_rate));
        return false;
    }

    // Produce two solutions per second
    if (!_gps.setNavigationFrequency(config.navigation_frequency, config.timeout))
    {
        error("Failed setting the navigation frequency to: " + String(config.navigation_frequency));
        return false;
    }

    // Tell the GNSS to "send" each solution
    if (!_gps.setAutoPVT(config.auto_pvt))
    {
        error("Failed setting the auto PVT to: " + String(config.auto_pvt));
        return false;
    }

    if (!_gps.setDynamicModel(config.dynamic_model, config.timeout)) // Set the dynamic model to Airborne2g or something else
    {
        error("Failed setting the dynamic model to: " + String(config.dynamic_model));
        return false;
    }

    // Set the I2C port to output UBX only (turn off NMEA noise)
    if (!_gps.setI2COutput(config.com_settings, config.timeout))
    {
        error("Failed setting I2C output to:" + String(config.com_settings));
        return false;
    }

    if (!_gps.saveConfiguration(config.timeout))
    {
        error("Config: Timeout when saving config: " + String(config.timeout));
        return false;
    }
    return true;
}
#endif
