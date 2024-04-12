#include <Arduino.h>
#include <Wire.h>
#include <MFRC522_I2C.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <SRAM.h>
#include <Keypad.h>
#include "functions.h"
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
MFRC522 mfrc522(LCD);

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

// Variables to keep track of keypresses and pressed code
int keypresses = 0;
String pressedCode;

void connectClient();
void keypad(char keypress);
void scanCard();

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

    if (current_key != 0) {
        keypad(current_key);
    } else {
        scanCard();
    }

    // Refresh MQTT connection
    client.loop();
}

void connectClient() {
    // Connect to MQTT
    reconnect();
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
                printAndDisplay("Exiting..");
                keypresses = 0;
                pressedCode = "";
                delay(700);
                resetDisplayToDefault();
                break;
            case 'D':
                // Display to user we are verifying
                printAndDisplay("Verifying: " + pressedCode);
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
    printAndDisplay("Verifying..");
    lcd.setCursor(0, 1);
    lcd.print("Abort with E");
    lcd.setRGB(255, 255, 0);
    delay(450);

    // Verify card
    verify(uidBytes, "card");
}