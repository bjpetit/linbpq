#include <EEPROM.h>

void setup() {
  // put your setup code here, to run once:

for (int i=0; i < 16; i++)
  EEPROM.write(i, 255);

}

void loop() {
  // put your main code here, to run repeatedly:

}
