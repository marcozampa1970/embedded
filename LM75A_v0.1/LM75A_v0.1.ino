
// http://www.esp8266learning.com/esp8266-lm75-temperature-sensor-example.php
// https://github.com/jlz3008/lm75

#include <lm75.h>
#include <inttypes.h>
#include <Wire.h>

// address   0100100 

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
