
// http://www.esp8266learning.com/esp8266-lm75-temperature-sensor-example.php
// https://github.com/jlz3008/lm75


// I set device address to 0 soldering these pins (see above picture):
// 1. A0 and GND together.
// 2. A1 and GND together.
// 3. A2 and GND together.
// After soldering all three pins - A0 , A1 and A2 to one of GND or VCC the board is ready for use.

// 1 0 0 1 A2 A1 A0

#include <lm75.h>
#include <inttypes.h>
#include <Wire.h>

// address LM75    1001000 
// address BMP280  1110111  


TempI2C_LM75 termo = TempI2C_LM75(0x48, TempI2C_LM75::nine_bits);

void setup()
{
  Serial.begin(115200);
  Serial.println("Start");
  Serial.print("Actual temp ");
  Serial.print(termo.getTemp());
  Serial.println(" oC");
  delay(2000);
}

void loop()
{
  Serial.print(termo.getTemp());
  Serial.println(" oC");
  delay(5000);
}
