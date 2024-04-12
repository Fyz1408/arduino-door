#ifndef ARDUINO_DOOR_FUNCTIONS_H
#define ARDUINO_DOOR_FUNCTIONS_H

// Ethernet & MQTT
void setupEthernet();
void reconnect();

// Verify card or pin using MQTT
void verify(String code, String type);

// LCD
void printAndDisplay(String text);
void resetDisplayToDefault();

#endif
