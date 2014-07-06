#include <OneWire.h>
#include <DallasEPROM.h>

OneWire onew(4);  // on pin 4
DallasEPROM de(&onew);

void setup() {
  Serial.begin(9600);
}

void loop() {
  byte buffer[32];  // Holds one page of data
  int status;

  // Search for the first compatible EPROM/EEPROM on the bus.
  // If you have multiple devices you can use de.setAddress()
  de.search();

  // Print out the 1-wire device's 64-bit address
  Serial.print("Address=");
  for(int i = 0; i < 8; i++) {
    Serial.print(de.getAddress()[i], HEX);
    Serial.print(" ");
  }
  Serial.println("");

  if (de.getAddress()[0] == 0x00) {
    Serial.println("No device was found!");
  } else {
    if (de.validAddress(de.getAddress())) {
      Serial.println("Address CRC is correct.");

      // Uncomment to write to the first page of memory
      //strcpy((char*)buffer, "allthingsgeek.com");
      //if ((status = de.writePage(buffer, 0)) != 0) {
        //sprintf((char*)buffer, "Error writing page! Code: %d", status);
        //Serial.println((char*)buffer);
      //}

      // Read the first page of memory into buffer
      if ((status = de.readPage(buffer, 0)) == 0) {
        Serial.println((char*)buffer);
      } else {
        sprintf((char*)buffer, "Error reading page! Code: %d", status);
        Serial.println((char*)buffer);
      }
    } else {
      Serial.println("Address CRC is wrong.");
    }
  }
  Serial.println("");

  delay(30000);
}
