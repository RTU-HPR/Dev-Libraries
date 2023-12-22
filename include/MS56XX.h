/*
  This library is used for the MS56XX series of pressure sensors.
  The MS5607 and MS5611 are supported.

  The original code comes from https://github.com/RobTillaart/MS5611/tree/master

  This code was modified by RTU HPR team to implement features and changes
  to better suit our use cases.

  The changes are as follows:
    - The begin function now takes in a config stuct that contains
      the required configuration parameters
    - Added altitude calculation
    - General comments
    - Simplified code
    - Removed unnecessary code
*/

#ifdef MS56XX_ENABLE

#include "Sensor_wrapper.h"
#include <Wire.h>

class MS56XX : public Sensor_Wrapper
{
private:
  // datasheet page 10
  const int MS56XX_CMD_READ_ADC = 0x00;
  const int MS56XX_CMD_READ_PROM = 0xA0;
  const int MS56XX_CMD_RESET = 0x1E;
  const int MS56XX_CMD_CONVERT_D1 = 0x40;
  const int MS56XX_CMD_CONVERT_D2 = 0x50;

  struct MS56XX_RunTimeVariables
  {
    int result;
    uint32_t lastRead;
  };
  MS56XX_RunTimeVariables runTimeVariables;

  // PROM buffer
  float C[7];

  // Functions from original library
  // Used to do calculations and read/write to the sensor
  void initConstants(uint8_t mathMode);
  void convert(const uint8_t addr, uint8_t bits);
  int command(const uint8_t command);
  uint16_t readProm(uint8_t reg);
  uint32_t readADC();

public:
  enum MS56XX_OVERSAMPLING
  {
    OSR_ULTRA_HIGH = 12, // 10 millis
    OSR_HIGH = 11,       //  5 millis
    OSR_STANDARD = 10,   //  3 millis
    OSR_LOW = 9,         //  2 millis
  };

  enum MS56XX_I2C_ADDRESS
  {
    I2C_0x76 = 0x76,
    I2C_0x77 = 0x77,
  };

  enum MS56XX_TYPE
  {
    MS5611 = 0,
    MS5607 = 1,
  };

  struct MS56XX_Config
  {
    TwoWire *wire;
    MS56XX_I2C_ADDRESS i2c_address;
    MS56XX_TYPE ms56xx_type;
    MS56XX_OVERSAMPLING oversampling;
  };

  MS56XX_Config _config;

  struct MS56XX_Data
  {
    float temperature;
    int32_t pressure;
    float altitude;
  };

  /**
   * @brief Initializes an instance of the MS56XX sensor.
   * 
   * @param error_function A pointer to an error handling function. Defaults to nullptr.
   * @param sensor_name The name of the sensor. Defaults to "MS56XX".
   */
  MS56XX(void (*error_function)(String) = nullptr, String sensor_name = "MS56XX");

  /**
   * @brief Initializes the MS56XX sensor with the specified configuration.
   * 
   * @param config The configuration settings for the sensor.
   * @return True if the initialization is successful, false otherwise.
   */
  bool begin(MS56XX_Config &config);

  /**
   * @brief Resets the MS56XX sensor to its default state.
   * 
   * @param altered_mode The altered calculation mode. If using MS5611 it will be set to 0.
   * If using MS5607 it will be set to 1.
   * @return True if the reset was successful, false otherwise.
   */
  bool reset(uint8_t altered_mode = 0);

  /**
   * @brief Reads data from the MS56XX sensor.
   * 
   * This function reads data from the MS56XX sensor and stores it in the provided data structure.
   * 
   * @param data The data structure to store the sensor readings.
   * @param outside_temperature The outside temperature in degrees Celsius used for altitude calculations (defaults to 15 degress Celsius).
   * @return true if the read operation was successful, false otherwise.
   */
  bool read(MS56XX_Data &data,  float outside_temperature = 15);
};

#endif