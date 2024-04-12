//
// Created by Daniel Lykke on 04/04/2024.
//

#include <Arduino.h>
#include <Ethernet.h>
#include <string.h>
#include "functions.h"
#include "PubSubClient.h"
#include "rgb_lcd.h"
#include "secrets.h"

extern byte mac[];
extern IPAddress ip;
extern PubSubClient client;
extern rgb_lcd lcd;

// Connect to the ethernet
void setupEthernet() {
    Serial.println("Connecting to ethernet ");
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

// Reconnect to our main unit over MQTT
void reconnect() {
    int attempts = 0;
    // Loop until we're reconnected
    while (!client.connected()) {
        if (attempts < 5) {
            Serial.println("Attempting MQTT connection...");
            // Attempt to connect
            if (client.connect("doorClient")) {
                printAndDisplay("MQTT Connected");
                printAndDisplay("Setup done\n");
                delay(500);
                resetDisplayToDefault();
                // Once connected, publish an announcement...
                client.publish("/mqtt/", "Door arduino connected");
            } else {
                Serial.print("failed, rc=");
                Serial.print(client.state());
                Serial.println(" try again in 5 seconds");
                // Wait 5 seconds before retrying
                delay(5000);
                attempts++;
            }
        } else {
            Serial.println("MQTT Connection failed after 5 attempts exiting...");
            break;
        }
    }
}

void handleCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Handling response | Topic: ");
    Serial.print(topic);
    Serial.print("| Payload: ");

    for (int i = 0; i < length; i++) {
        Serial.println((char) payload[i]);

        if ((char) payload[i] == '1') {
            printAndDisplay("Door opening");
            Serial.println("\n");
            lcd.setRGB(0, 255, 0);
        } else {
            printAndDisplay("No entry");
            Serial.println("\n");
            lcd.setRGB(255, 0, 0);
        }
    }

    delay(1000);
    resetDisplayToDefault();
}

// Verify a card or entered pin over MQTT
void verify(String code, String type) {
    char topic[30] = {0};
    char message[50] = {0};
    char returnAddress[50] = {0};

    if (type == "pin") {
        strcat(topic, "/mqtt/room/doors/4/pin");
    } else if (type == "card") {
        strcat(topic, "/mqtt/room/doors/3/keycard");
    } else {
        Serial.println("No type choosen");
    }

    strcat(returnAddress, topic);
    strcat(returnAddress, "/return");

    strcat(message, code.c_str());
    strcat(message, "+");
    strcat(message, returnAddress);

    client.publish(topic, message);
    client.subscribe(returnAddress);
    client.setCallback(handleCallback);
}

// Serial and LCD print text
void printAndDisplay(String text) {
    lcd.clear();
    lcd.print(text);
    Serial.println(text);
}

// Reset LCD display to default ready to receive pin or card
void resetDisplayToDefault() {
    lcd.clear();
    lcd.setRGB(255, 255, 255);
    lcd.setCursor(0, 0);
    lcd.print("Enter the pin or");
    lcd.setCursor(0, 1);
    lcd.print("scan your card");
}