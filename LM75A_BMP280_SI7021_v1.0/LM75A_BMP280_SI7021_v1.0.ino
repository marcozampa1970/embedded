#include <Wire.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_Si7021.h"
#include <lm75.h>
#include <inttypes.h>
#include <Wire.h>

// BMP280 *********************
// https://learn.adafruit.com/adafruit-bme280-humidity-barometric-pressure-temperature-sensor-breakout/arduino-test
// Connect the CSB pin to GND to have SPI and to VCC(3V3) for I2C.
// The 7-bit device address is 111011x. The 6 MSB bits are fixed. The last bit is changeable by SDO value and can be changed during operation.
// Connecting SDO to GND results in slave address 1110110 (0x76), connecting it to VCC results in slave address 1110111 (0x77), which is the same as BMP180’s I²C address.

// address BMP280 1110111 --> (119)


// LM75A *********************
// http://www.esp8266learning.com/esp8266-lm75-temperature-sensor-example.php
// https://github.com/jlz3008/lm75

// I set device address to 0 soldering these pins (see above picture):
// 1. A0 and GND together.
// 2. A1 and GND together.
// 3. A2 and GND together.
// After soldering all three pins - A0 , A1 and A2 to one of GND or VCC the board is ready for use.

// 1 0 0 1 A2 A1 A0
// address LM75A  1001000 --> (72)

// Si7021 *********************
// https://learn.adafruit.com/adafruit-si7021-temperature-plus-humidity-sensor/arduino-code
// The Si7021 has a default I2C address of 0x40 and cannot be changed! --100 0000


// address Si7021  01000000 --> (64)



Adafruit_BMP280 sensorBMP280;
TempI2C_LM75 sensorLM75A = TempI2C_LM75(0x48, TempI2C_LM75::nine_bits);
Adafruit_Si7021 sensorSI7021 = Adafruit_Si7021();


void setup() {

  Serial.begin(115200);

  // wait for serial port to open
  while (!Serial) {
    delay(10);
  }

  /* Default settings from datasheet. */
  sensorBMP280.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                           Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                           Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                           Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                           Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  Serial.println(F("BMP280 - test"));
  if (!sensorBMP280.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (true);
  }

  delay(500);

  if (!sensorSI7021.begin()) {
    Serial.println("Did not find Si7021 sensor!");
    while (true);
  }

  delay(500);
}

void loop() {


  float temperatureBMP280 = sensorBMP280.readTemperature();
  float pressureBMP280 = sensorBMP280.readPressure();
  float altitudeBMP280 = sensorBMP280.readAltitude(1015); // this should be adjusted to your local forcase

  float temperatureLM75A = sensorLM75A.getTemp();

  float temperatureSI702 = sensorSI7021.readTemperature();
  float uumiditySI702 = sensorSI7021.readHumidity();

  Serial.println("*********************");

  Serial.print("BMP280 - Temperature = ");
  Serial.print(temperatureBMP280);
  Serial.println(" °C");

  Serial.print("LM75A - Temperature = ");
  Serial.print(temperatureLM75A);
  Serial.println(" °C");

  Serial.print("SI702 - Temperature: ");
  Serial.print(temperatureSI702, 2);
  Serial.println(" °C");

  Serial.print("SI702 - Humidity: ");
  Serial.println(uumiditySI702, 2);

  Serial.print("BMP280 - Pressure = ");
  Serial.print(pressureBMP280);
  Serial.println(" Pa");

  Serial.print("BMP280 - approx altitude = ");
  Serial.print(altitudeBMP280);
  Serial.println(" m");

  delay(30000);
}
