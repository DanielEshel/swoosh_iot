// NOTE: Arduino IDE requires the main sketch file to be an .ino file.
// All C++ files (.h and .cpp) must be in a subfolder named 'src' 
// or placed directly next to this .ino file.
#include "servo_control.h"
#include "BluetoothSerial.h"

// --- Hardware and Logic Objects ---
#define SERVO_PIN 18
#define BT_LED_PIN 4  // For Bluetooth connection indicator

BluetoothSerial SerialBT;
ServoControl servo; 

// --- Setup ---
void setup() {
    Serial.begin(115200);
    
    // LED setup
    pinMode(BT_LED_PIN, OUTPUT);
    digitalWrite(BT_LED_PIN, LOW);

    // Servo initialization
    servo.begin(SERVO_PIN, 90); // Start the servo at 90 degrees

    // Bluetooth initialization
    SerialBT.begin("Swoosh_ESP32"); 
    Serial.println("Bluetooth device 'Swoosh_ESP32' is ready to pair!");
}

// --- Main Loop ---
void loop() {
    // 1. Handle Bluetooth Command Input
    if (SerialBT.available()) {
        char incomingChar = SerialBT.read();
        int newAngle = -1; // Flag for a new angle

        Serial.print("Received Command: ");
        Serial.println(incomingChar);

        // Command Mapping: '0', '1', '2' map to discrete angles
        if (incomingChar == '1') {
            newAngle = 180; // Max Angle
        } else if (incomingChar == '0') {
            newAngle = 0;  // Min Angle
        } else if (incomingChar == '2') {
            newAngle = 90; // Center Angle
        }
        
        // If a valid command was received, update the servo
        if (newAngle != -1) {
            servo.setAngle(newAngle);
        }
    }

    // 2. Bluetooth Connection Indicator LED
    if (SerialBT.connected()) {
        digitalWrite(BT_LED_PIN, HIGH); // BT LED ON when connected
    } else {
        digitalWrite(BT_LED_PIN, LOW); // BT LED OFF when disconnected
    }

    // Small delay to keep the loop from running too fast
    delay(20); 
}