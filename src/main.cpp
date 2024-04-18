#include <Arduino.h>
#include <Wire.h>
#include <MFRC522_I2C.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SRAM.h>
#include <Keypad.h>
#include "rgb_lcd.h"
#include "secrets.h"

// Mac, arduino IP and server IP
byte mac[] = {MAC};
IPAddress ip(IP_OCTETS);
IPAddress server(SERVER_OCTETS);

// Ethernet and PubSubClient
SRAM sram(4, SRAM_1024);
EthernetClient ethClient;
PubSubClient client(ethClient);

// LCD and RFID scanner
rgb_lcd lcd;
MFRC522 mfrc522(RFID);

// Keypad row & column pins
byte row_pins[] = {2, 3, 4, 5};
byte column_pins[] = {6, 7, 8, 9};


// Set the keys of the keypad
char hexaKeys[sizeof(row_pins) / sizeof(byte)][sizeof(column_pins) / sizeof(byte)] =
        {
                {'1', '2', '3', 'A'},
                {'4', '5', '6', 'B'},
                {'7', '8', '9', 'C'},
                {'0', 'F', 'E', 'D'}
        };

// The object for the keypad
Keypad kypd = Keypad(makeKeymap(hexaKeys),
                     row_pins,
                     column_pins,
                     sizeof(row_pins) / sizeof(byte),
                     sizeof(column_pins) / sizeof(byte)
);

// Variables to keep track of keypress count, code & last scanned card
int keypresses = 0;
String pressedCode;

// Ethernet & MQTT
void setupEthernet();
void connectClient();
void handleCallback(char *topic, byte *payload, unsigned int length);

// Verification of keypress & card scans
void verify(String code, String type);
void keypad(char keypress);
void scanCard();

// LCD
void resetDisplayToDefault();

void setup() {
    Serial.begin(9600);
    lcd.begin(16, 2);
    lcd.setRGB(255, 255, 255);

    // Initial RFID, MQTT & Ethernet setup
    Wire.begin();
    mfrc522.PCD_Init();
    client.setServer(server, 1883);
    setupEthernet();
    connectClient();
}

void loop() {
    char current_key = kypd.getKey();

    // Keypress or scan card
    if (current_key != 0) {
        keypad(current_key);
    } else {
        scanCard();
    }

    // Refresh MQTT connection
    client.loop();
}

// Connect to our main unit over MQTT
void connectClient() {
    int attempts = 0;
    // Loop until we're reconnected
    while (!client.connected()) {
        if (attempts < 5) {
            Serial.println(F("Attempting MQTT connection..."));
            // Attempt to connect
            if (client.connect("doorClient", MQTT_USER, MQTT_PASS)) {
                // Display were connected via MQTT
                Serial.println(F("MQTT Connected \nSetup done"));
                lcd.print("MQTT Connected");
                delay(500);
                lcd.print("Setup done");

                // Once connected do a little test publish
                client.publish("/mqtt/", "Door arduino connected");
            } else {
                Serial.print(F("failed, rc="));
                // Wait 5 seconds before retrying
                delay(5000);
                attempts++;
            }
        } else {
            Serial.print(F("MQTT failed"));
            break;
        }
    }

    resetDisplayToDefault();
    delay(400);
}

void keypad(char keypress) {
    if (keypresses == 0) {
        lcd.clear();
    }

    if (keypress != 0) {
        keypresses++;

        switch (keypress) {
            case 'E':
                // Reset display, pressed code & keypresses
                lcd.clear();
                lcd.print("Exiting..");
                Serial.println("Exiting..");
                keypresses = 0;
                pressedCode = "";
                delay(700);
                resetDisplayToDefault();
                break;
            case 'D':
                // Display to user we are verifying
                lcd.clear();
                lcd.print("Verifying: " + pressedCode);
                Serial.println("Verifying: " + pressedCode);
                lcd.setCursor(0, 1);
                lcd.print("Abort with E");
                lcd.setRGB(255, 255, 0);
                delay(450);

                // Verify entered pin
                verify(pressedCode, "pin");
                delay(600);

                // Reset back to default after verify pin
                keypresses = 0;
                pressedCode = "";
                break;
            default:
                // Store keypress for later submit
                pressedCode += String(keypress);
                if (keypresses == 17) {
                    keypresses = 0;
                    lcd.clear();
                    lcd.setCursor(0, 0);
                }
                Serial.println("Keypressed: " + String(keypress));
                lcd.print(String(keypress));
                lcd.setCursor(0, 1);
                lcd.print("Send D | Exit E");
                lcd.setCursor(keypresses, 0);
        }
    }
}

void scanCard() {
    // Return if no new card is present or getting read
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        delay(50);
        return;
    }
    keypresses = 0;
    String uidBytes;

    for (byte i = 0; i < mfrc522.uid.size; i++) {
        // Save card uid
        uidBytes += String(mfrc522.uid.uidByte[i], HEX);
    }

    // Display to user were verifying
    Serial.println("Verifying...");
    lcd.clear();
    lcd.print("Verifying..");
    lcd.setCursor(0, 1);
    lcd.print("Abort with E");
    lcd.setRGB(255, 255, 0);
    delay(450);

    // Verify card
    verify(uidBytes, "card");
}

// Initialize ethernet and connect to it
void setupEthernet() {
    Serial.println(F("Connecting to ethernet"));
    Ethernet.init(10);
    Ethernet.begin(mac, ip);

    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
        Serial.println(F("Ethernet shield was not found"));
        return;
    }
    if (Ethernet.linkStatus() == LinkOFF) {
        Serial.println(F("Ethernet cable is not connected."));
    }
}


void handleCallback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Response: ");

    // Since payload is byte we need to for loop through it
    for (int i = 0; i < length; i++) {
        Serial.println((char) payload[i]);

        if ((char) payload[i] == '1') {
            lcd.clear();
            lcd.print("Door opening..");
            Serial.println("Door opening..");
            lcd.setRGB(0, 255, 0);
        } else {
            lcd.clear();
            lcd.print("No entry!");
            Serial.println("No entry!");
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
        Serial.println(F("No type choosen"));
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