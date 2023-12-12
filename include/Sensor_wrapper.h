#pragma once
#include <Arduino.h>
class Sensor_Wrapper
{
    bool _initialized;
    String _sensor_name;
    void (*_error_function)(String);

public:
    /**
     * @brief Set the error output function object. If not set by default error will be output in serial port.
     * Note that if the function is inside a class then passing it to the set error output might be difficult
     *
     * @param func pointer to a function that you can pass the error string
     */
    void set_error_output_function(void (*error_function)(String)) { _error_function = error_function; }
    void error(String msg);

    String get_sensor_name() { return _sensor_name; }
    void set_initialized(bool initialized) { _initialized = initialized; }
    bool get_initialized() { return _initialized; }

    Sensor_Wrapper(String sensor_name, void (*error_function)(String) = nullptr)
    {
        _sensor_name = sensor_name;
        set_error_output_function(error_function);
        _initialized = false;
    }
};