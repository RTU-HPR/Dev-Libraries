#include <Arduino.h>
#include <Wire.h>
#include <MS56XX.h>

// // Object
MS56XX ms56xx;

// Configuration (PFC)
MS56XX::MS56XX_Config config = {
    .wire = &Wire1,
    .i2c_address = MS56XX::MS56XX_I2C_ADDRESS::I2C_0x76,
    .ms56xx_type = MS56XX::MS56XX_TYPE::MS5611,
    .oversampling = MS56XX::MS56XX_OVERSAMPLING::OSR_STANDARD,
};

MS56XX::MS56XX_Data data;

void setup()
{
  // Wait for PC serial
  Serial.begin(115200);
  while (!Serial)
  {
    delay(100);
  }

  Serial.println("MS56XX Example");

  // Wire1
  Wire1.setSCL(15);
  Wire1.setSDA(14);
  Wire1.begin();

  // Initialize the sensor
  if (ms56xx.begin(config))
  {
    Serial.println("MS56XX initialization successful");
  }
  else
  {
    while (true)
    {
      Serial.println("MS56XX initialization failed");
      delay(1000);
    }
  }
}

void loop()
{
  ms56xx.read(data);
  Serial.println("Temperature: " + String(data.temperature) + " C");
  Serial.println("Pressure: " + String(data.pressure) + " Pa");
  Serial.println("Altitude: " + String(data.altitude) + " m");
  Serial.println();
  
  delay(1000);
}