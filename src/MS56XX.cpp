#ifdef MS56XX_ENABLE
#include "MS56XX.h"

MS56XX::MS56XX(void (*error_function)(String), String sensor_name) : Sensor_Wrapper(sensor_name, error_function)
{
  return;
}

bool MS56XX::begin(MS56XX_Config &config)
{
  config.wire->beginTransmission(config.i2c_address);
  if (config.wire->endTransmission() != 0)
  {
    return false;
  }

  // Copy the passed config to object config, to be used later
  _config.i2c_address = config.i2c_address;
  _config.wire = config.wire;
  _config.ms56xx_type = config.ms56xx_type;
  _config.oversampling = config.oversampling;

  return reset(config.ms56xx_type);
}

bool MS56XX::reset(uint8_t mathMode)
{
  // Reset the sensor
  command(MS56XX_CMD_RESET);
  delay(50);

  // Initialize the PROM constant array
  initConstants(mathMode);

  // Read factory calibrations from PROM.
  bool PROM_OK = true;
  for (uint8_t reg = 0; reg < 7; reg++)
  {
    uint16_t tmp = readProm(reg);
    C[reg] *= tmp;
    if (reg > 0)
    {
      PROM_OK = PROM_OK && (tmp != 0);
    }
  }
  return PROM_OK;
}

bool MS56XX::read(MS56XX_Data &data, float outside_temperature)
{
  //  VARIABLES NAMES BASED ON DATASHEET
  //  ALL MAGIC NUMBERS ARE FROM DATASHEET

  // Read pressure registers
  convert(MS56XX_CMD_CONVERT_D1, _config.oversampling);
  if (runTimeVariables.result)
    return runTimeVariables.result;
  //  NOTE: D1 and D2 seem reserved in MBED (NANO BLE)
  uint32_t _D1 = readADC();
  if (runTimeVariables.result)
    return runTimeVariables.result;

  // Read temperature registers
  convert(MS56XX_CMD_CONVERT_D2, _config.oversampling);
  if (runTimeVariables.result)
    return runTimeVariables.result;
  uint32_t _D2 = readADC();
  if (runTimeVariables.result)
    return runTimeVariables.result;

  //  TEMP & PRESS MATH - PAGE 7/20
  float dT = _D2 - C[5];
  float new_temperature = 2000 + dT * C[6];

  float offset = C[2] + dT * C[4];
  float sens = C[1] + dT * C[3];

  //  SECOND ORDER COMPENSATION - PAGE 8/20
  //  COMMENT OUT < 2000 CORRECTION IF NOT NEEDED
  //  NOTE TEMPERATURE IS IN 0.01 C
  if (new_temperature < 2000)
  {
    float T2 = dT * dT * 4.6566128731E-10;
    float t = (new_temperature - 2000) * (new_temperature - 2000);
    float offset2 = 2.5 * t;
    float sens2 = 1.25 * t;
    //  COMMENT OUT < -1500 CORRECTION IF NOT NEEDED
    if (new_temperature < -1500)
    {
      t = (new_temperature + 1500) * (new_temperature + 1500);
      offset2 += 7 * t;
      sens2 += 5.5 * t;
    }
    new_temperature -= T2;
    offset -= offset2;
    sens -= sens2;
  }
  
  // Store the data in the provided data struct
  data.temperature = new_temperature * 0.01;
  data.pressure = (_D1 * sens * 4.76837158205E-7 - offset) * 3.051757813E-5;

  // Barometric formula
  // h = (RT/gM) * ln(p0/p)
  // 29.271267 = (R/gM)
  data.altitude = 29.271267 * (273.15 + outside_temperature) * log(101325 / (float)data.pressure);

  runTimeVariables.lastRead = millis();
  return true;
}

void MS56XX::convert(const uint8_t addr, uint8_t bits)
{
  //  values from page 3 datasheet - MAX column (rounded up)
  uint16_t del[5] = {600, 1200, 2300, 4600, 9100};

  uint8_t index = bits;
  if (index < 8)
    index = 8;
  else if (index > 12)
    index = 12;
  index -= 8;
  uint8_t offset = index * 2;
  command(addr + offset);

  uint16_t waitTime = del[index];
  uint32_t start = micros();
  //  while loop prevents blocking RTOS
  while (micros() - start < waitTime)
  {
    yield();
    delayMicroseconds(10);
  }
}

uint16_t MS56XX::readProm(uint8_t reg)
{
  //  last EEPROM register is CRC - Page 13 datasheet.
  uint8_t promCRCRegister = 7;
  if (reg > promCRCRegister)
    return 0;

  uint8_t offset = reg * 2;
  command(MS56XX_CMD_READ_PROM + offset);
  if (runTimeVariables.result == 0)
  {
    uint8_t length = 2;
    int bytes = _config.wire->requestFrom(static_cast<uint8_t>(_config.i2c_address), static_cast<uint8_t>(length));
    if (bytes >= length)
    {
      uint16_t value = _config.wire->read() * 256;
      value += _config.wire->read();
      return value;
    }
    return 0;
  }
  return 0;
}

uint32_t MS56XX::readADC()
{
  command(MS56XX_CMD_READ_ADC);
  if (runTimeVariables.result == 0)
  {
    uint8_t length = 3;
    int bytes = _config.wire->requestFrom(static_cast<uint8_t>(_config.i2c_address), static_cast<uint8_t>(length));
    if (bytes >= length)
    {
      uint32_t value = _config.wire->read() * 65536UL;
      value += _config.wire->read() * 256UL;
      value += _config.wire->read();
      return value;
    }
    return 0UL;
  }
  return 0UL;
}

int MS56XX::command(const uint8_t command)
{
  yield();
  _config.wire->beginTransmission(_config.i2c_address);
  _config.wire->write(command);
  runTimeVariables.result = _config.wire->endTransmission();
  return runTimeVariables.result;
}

void MS56XX::initConstants(uint8_t altered_mode)
{
  C[0] = 1;
  C[1] = 32768L;
  C[2] = 65536L;
  C[3] = 3.90625E-3;
  C[4] = 7.8125E-3;
  C[5] = 256;
  C[6] = 1.1920928955E-7;

  if (altered_mode == 1) 
  {
    C[1] = 65536L;
    C[2] = 131072L;
    C[3] = 7.8125E-3;
    C[4] = 1.5625e-2;
  }
}

#endif