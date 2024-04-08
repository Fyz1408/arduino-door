#include <Arduino.h>
#include <Wire.h>
#include <MFRC522_I2C.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include "rgb_lcd.h"
#include "functions.h"
#include <SRAM.h>

// Temp array with allowed cards
String allowedCards[] = {"4be8eff", "928a3dbf"};

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 3);
IPAddress server(192, 168, 1, 139);

SRAM sram(4, SRAM_1024);
EthernetClient ethClient;
PubSubClient client(ethClient);

rgb_lcd lcd;
MFRC522 mfrc522(0x28);

void setup() {
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.setRGB(255, 255, 255);

    // Initial RFID, MQTT & Ethernet setup
    Wire.begin();
    mfrc522.PCD_Init();
    client.setServer(server, 1883);
    //client.setCallback(callback); TODO LATER
    //setupEthernet();

    delay(200);
}

void loop() {
    if (client.connected()) {
        reconnect();
        printAndDisplay("Setup done");
        delay(400);
        printAndDisplay("Scan card:");
    } else {
        if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
            delay(200);
            return;
        }

        String uidBytes = "";
        for (byte i = 0; i < mfrc522.uid.size; i++) {
            // Save card uid
            uidBytes += String(mfrc522.uid.uidByte[i], HEX);
        }

        Serial.println(uidBytes);
        verifyCard(uidBytes);
    }
    client.loop();

    delay(1500);
    lcd.clear();
    lcd.setRGB(255, 255, 255);
    lcd.print("Scan card:");
}