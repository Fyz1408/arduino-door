//
// Created by Daniel Lykke on 04/04/2024.
//

#include <Arduino.h>
#include <SPI.h>
#include <Ethernet.h>
#include "functions.h"
#include "PubSubClient.h"
#include "rgb_lcd.h"

extern byte mac[];
extern IPAddress ip;
extern PubSubClient client;
extern rgb_lcd lcd;
extern String allowedCards[1];

void setupEthernet() {
    Serial.print("Connecting to ethernet ");
    Ethernet.init(10);
    Ethernet.begin(mac, ip);

    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware.");
        return;
    }
    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println("Ethernet cable is not connected.");
    }
}

void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("arduinoClient")) {
            Serial.println("connected");
            // Once connected, TODO Maybe send payload to tell PI were online
            client.publish("outTopic","connected");
            //
            client.subscribe("inTopic");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}

void verifyCard(String uid) {
    int foundIndex = -1;

    // TODO Dont hardcode allowed key cards
    for (int i = 0; i < sizeof(allowedCards); i++) {
        if (allowedCards[i] == uid) {
            foundIndex = i;
            break;
        }
    }

    lcd.clear();
    if (foundIndex != -1) {
        lcd.setRGB(0, 255, 0); // Display green
        printAndDisplay("Card allowed");

        String message = "Card-entered: " + uid;
        client.publish("/mqtt", message.c_str());

    } else {
        lcd.setRGB(255, 0, 0); // Display red
        printAndDisplay("Card not allowed");

        String message = "Card-denied: " + uid;
        client.publish("/mqtt", message.c_str());
    }
}

void printAndDisplay(String text) {
    lcd.clear();
    lcd.print(text);
    Serial.println(text);
}

void resetDisplayToDefault() {
    lcd.clear();
    lcd.setRGB(255, 255, 255);
    lcd.setCursor(0, 0);
    lcd.print("Enter the pin or");
    lcd.setCursor(0, 1);
    lcd.print("scan your card");
}