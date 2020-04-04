#include <Wire.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

/*
Connect the CSB pin to GND to have SPI and to VCC(3V3) for I2C.
The 7-bit device address is 111011x. The 6 MSB bits are fixed. The last bit is changeable by SDO
value and can be changed during operation.
Connecting SDO to GND results in slave address 1110110 (0x76), connecting it to VCC results in
slave address 1110111 (0x77), which is the same as BMP180’s I²C address.
*/


Adafruit_BMP280 bmp; // I2C

// Address 1110111

void setup() {
  Serial.begin(115200);
  Serial.println(F("BMP280 test"));

  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP280 sensor, check wiring!");
    while (1);
  }
}

void loop() {
  Serial.print("Temperature = ");
  Serial.print(bmp.readTemperature());
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bmp.readPressure());
  Serial.println(" Pa");

  Serial.print("Approx altitude = ");
  Serial.print(bmp.readAltitude(1013.25)); // this should be adjusted to your local forcase
  Serial.println(" m");

  Serial.println();
  delay(2000);
}
