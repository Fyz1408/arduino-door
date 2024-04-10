#ifndef ARDUINO_DOOR_FUNCTIONS_H
#define ARDUINO_DOOR_FUNCTIONS_H

// Ethernet & MQTT
void setupEthernet();
void reconnect();

// RFID Cards
void verifyCard(String uid);

// LCD
void printAndDisplay(String text);
void resetDisplayToDefault();

#endif
