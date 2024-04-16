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

// Serial and LCD print text
void printAndDisplay(String text) {
    lcd.clear();
    lcd.print(text);
    Serial.println(text);
}

// Reconnect to our main unit over MQTT
void reconnect() {
    int attempts = 0;
    // Loop until we're reconnected
    while (!client.connected()) {
        if (attempts < 5) {
            Serial.println("Attempting MQTT connection...");
            // Attempt to connect
            if (client.connect("doorClient", MQTT_USER, MQTT_PASS)) {
                // Tell user were connected
                Serial.println("MQTT Connected");
                lcd.print("MQTT Connected");
                delay(500);
                Serial.println("Setup done");
                lcd.print("Setup done");
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

    // Since payload is byte we need to for loop through it
    for (int i = 0; i < length; i++) {
        Serial.println((char) payload[i]);

        if ((char) payload[i] == '1') {
            printAndDisplay("Door opening..");
            Serial.println("\n");
            lcd.setRGB(0, 255, 0);
        } else {
            printAndDisplay("No entry!");
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

    // Setup topic either for pin code or keycard
    if (type == "pin") {
        strcat(topic, "/mqtt/room/doors/3/pin");
    } else if (type == "card") {
        strcat(topic, "/mqtt/room/doors/2/keycard");
    } else {
        Serial.println("No type choosen");
    }

    // Create return address from topic
    strcat(returnAddress, topic);
    strcat(returnAddress, "/return");

    // Create message to publish
    strcat(message, "!");
    strcat(message, code.c_str());
    strcat(message, "+");
    strcat(message, returnAddress);

    // Publish message and setup callback to handle response from server
    client.publish(topic, message);
    client.subscribe(returnAddress);
    client.setCallback(handleCallback);
}

// Reset LCD display to default ready to receive pin or card
void resetDisplayToDefault() {
    lcd.clear();
    lcd.setRGB(255, 255, 255);
    lcd.setCursor(0, 0);
    lcd.print("Enter the pin or");
    lcd.setCursor(0, 1);
    lcd.print("scan your card");
    Serial.println("Enter the pin or scan your card");
}