/*
  This library is used for the to read voltage from a voltage divider circuit.
*/
#pragma once
#ifdef ANALOG_VOLTAGE_READER_ENABLE

#include "Sensor_wrapper.h"

class AdcVoltage : public Sensor_Wrapper
{
private:
  int _pin;
  float _conversion_factor;

public:
  struct AdcVoltage_Config
  {
    int pin;                 // Pin to read from
    int adc_resolution;      // ADC resolution
    float reference_voltage; // MCU voltage
    int R1_value;            // Resistor connected to positive
    int R2_value;            // Resistor connected to ground
  };

  /**
   * @brief Structure to hold battery data.
   */
  struct AdcVoltage_Data
  {
    float voltage;
  };

  /**
   * @brief Construct a new AdcVoltage object.
   *
   * @param error_function The function to call when an error occurs.
   * @param sensor_name The name of the sensor.
   */
  AdcVoltage(void (*error_function)(String) = nullptr, String sensor_name = "Analog Voltage");

  /**
   * @brief Initializes the reader.
   *
   * @param config The configuration object containing the settings.
   * @return True if the initialization is successful, false otherwise.
   */
  bool begin(const AdcVoltage_Config &config);

  /**
   * @brief Reads the  voltage.
   *
   * @param data The data structure to store the read data in.
   * @return True if the data was successfully read, false otherwise.
   */
  bool read(AdcVoltage_Data &data);
};

#endif
