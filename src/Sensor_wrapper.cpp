#include "Sensor_wrapper.h"

void Sensor_Wrapper::error(String msg)
{
    msg = _sensor_name + " Error: " + msg;

    if (_error_function == nullptr)
    {
        Serial.println(msg);
    }
    else
    {
        _error_function(msg);
    }
}