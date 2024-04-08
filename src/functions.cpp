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
extern String allowedCards[];

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

    for (int i = 0; i < 2; i++) {
        if (allowedCards[i] == uid) {
            foundIndex = i;
            break;
        }
    }

    lcd.clear();
    if (foundIndex != -1) {
        Serial.print("Card found at index: ");
        Serial.println(foundIndex);
        lcd.print("Card found!");
        lcd.setRGB(0, 255, 0); // Display green
    } else {
        Serial.println("Card not found in the array.");
        lcd.print("Card not allowed");
        lcd.setRGB(255, 0, 0); // Display red
    }
}

void printAndDisplay(String text) {
    lcd.clear();
    lcd.print(text);
    Serial.println(text);
}