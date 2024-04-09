#include <Arduino.h>
#include <Wire.h>
#include <MFRC522_I2C.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SRAM.h>
#include <Keypad.h>
#include "functions.h"
#include "rgb_lcd.h"

// Temp array with allowed cards
String allowedCards[] = {"4be8eff", "928a3dbf"};

// Mac, arduino IP and server IP
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 3);
IPAddress server(192, 168, 1, 139);

// Ethernet and PubSubClient
SRAM sram(4, SRAM_1024);
EthernetClient ethClient;
PubSubClient client(ethClient);

// LCD and RFID scanner
rgb_lcd lcd;
MFRC522 mfrc522(0x28);

byte row_pins[] = {2, 3, 4, 5}; // Row pins of the keypad
byte column_pins[] = {6, 7, 8, 9}; // Column pins of the keypad

// Declaration of the keys of the keypad
char hexaKeys[sizeof(row_pins) / sizeof(byte)][sizeof(column_pins) / sizeof(byte)] =
        {
                {'1', '2', '3', 'A'},
                {'4', '5', '6', 'B'},
                {'7', '8', '9', 'C'},
                {'0', 'F', 'E', 'D'}
        };

Keypad kypd = Keypad(makeKeymap(hexaKeys),
                     row_pins,
                     column_pins,
                     sizeof(row_pins) / sizeof(byte),
                     sizeof(column_pins) / sizeof(byte)
                     ); //define object for the keypad

int keypresses = 0;


void setup() {
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.setRGB(255, 255, 255);

    // Initial RFID, MQTT & Ethernet setup
    Wire.begin();
    mfrc522.PCD_Init();
    client.setServer(server, 1883);
    //setupEthernet();

    printAndDisplay("Setup done");
    delay(500);
    lcd.clear();
}

void loop() {
    char current_key = kypd.getKey();

    if (current_key != 0) {
        keypresses++;
        if (keypresses == 17) {
            lcd.setCursor(0, 1);
        } else if (keypresses == 33) {
            keypresses = 0;
            lcd.clear();
            lcd.setCursor(0, 0);
        }
        lcd.print(current_key);
    }
}


void scanCard() {
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