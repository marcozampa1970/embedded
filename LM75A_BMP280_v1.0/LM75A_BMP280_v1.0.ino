#include <Wire.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <lm75.h>
#include <inttypes.h>
#include <Wire.h>

// BMP280 *********************
// Connect the CSB pin to GND to have SPI and to VCC(3V3) for I2C.
// The 7-bit device address is 111011x. The 6 MSB bits are fixed. The last bit is changeable by SDO value and can be changed during operation.
// Connecting SDO to GND results in slave address 1110110 (0x76), connecting it to VCC results in slave address 1110111 (0x77), which is the same as BMP180’s I²C address.

// address BMP280  1110111 


// LM75A *********************
// http://www.esp8266learning.com/esp8266-lm75-temperature-sensor-example.php
// https://github.com/jlz3008/lm75

// I set device address to 0 soldering these pins (see above picture):
// 1. A0 and GND together.
// 2. A1 and GND together.
// 3. A2 and GND together.
// After soldering all three pins - A0 , A1 and A2 to one of GND or VCC the board is ready for use.

// 1 0 0 1 A2 A1 A0
// address LM75A    1001000 


Adafruit_BMP280 bmp; // I2C
TempI2C_LM75 termo = TempI2C_LM75(0x48, TempI2C_LM75::nine_bits);
 

void setup() {
 
  Serial.begin(115200);

  Serial.println("Start");
  
  Serial.println(F("BMP280 - test"));
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }

  // LM75A


  Serial.print("LM75A - actual temp ");
  Serial.print(termo.getTemp());
  Serial.println(" C");
  delay(2000);  
}

void loop() {
  Serial.print("BMP280 - Temperature = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" C");

  Serial.print("BMP280 - Pressure = ");
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");

  Serial.print("BMP280 - approx altitude = ");
  Serial.print(bmp.readAltitude(1013.25)); // this should be adjusted to your local forcase
  Serial.println(" m");


  Serial.print("LM75A - actual temp ");
  Serial.print(termo.getTemp());
  Serial.println(" C");
  delay(2000);  
}
