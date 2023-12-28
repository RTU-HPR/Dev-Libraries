#ifdef ANALOG_VOLTAGE_READER_ENABLE
#include "Adc_Voltage.h"

AdcVoltage::AdcVoltage(void (*error_function)(String), String sensor_name) : Sensor_Wrapper(sensor_name, error_function)
{
  return;
}

bool AdcVoltage::begin(AdcVoltage_Config &config)
{
  // Copy the passed pin to object
  _pin = config.pin;

  // Calculate the conversion factor
  // C_F = ((R1 + R2) * REF_VOL) / (R2 * ADC_RES)
  _conversion_factor = config.R1_value + config.R2_value;
  _conversion_factor *= (float)config.reference_voltage;
  _conversion_factor /= (float)config.R2_value;
  _conversion_factor /= (float)config.adc_resolution;

  return true;
}

bool AdcVoltage::read(AdcVoltage_Data &data)
{
  // Read the voltage
  // U = ADC * C_F
  data.voltage = analogRead(_pin) * _conversion_factor;
  return true;
}

#endif