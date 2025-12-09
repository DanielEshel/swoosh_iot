#include <Arduino.h>
#include <NimBLEDevice.h>
#include "servo_control.h"

#define SERVO_PIN 18
ServoControl servo;

// BLE UUIDs (use your own in production)
#define SERVICE_UUID           "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID_RX "12345678-1234-1234-1234-1234567890ac"

// BLE characteristic pointer
NimBLECharacteristic* commandCharacteristic;

// How much the servo moves per button press
#define STEP_SIZE 50   // degrees per command



// =========================================================
// BLE Callback — receive commands from Flutter
// =========================================================
class CommandCallback : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {

        std::string value = c->getValue();
        if (value.empty()) return;

        char cmd = value[0];
        Serial.printf("Received BLE command: %c\n", cmd);

        int current = servo.getCurrentAngle();
        int newAngle = current;

        // LEFT = 0 → decrease angle
        if (cmd == '0') {
            newAngle = current - STEP_SIZE;
            if (newAngle < 0) newAngle = 0;
        }

        // RIGHT = 1 → increase angle
        else if (cmd == '1') {
            newAngle = current + STEP_SIZE;
            if (newAngle > 180) newAngle = 180;
        }

        // Only move if angle changed
        if (newAngle != current) {
            Serial.printf("Servo moving: %d → %d\n", current, newAngle);
            servo.setAngle(newAngle);
        }
    }
};

// =========================================================
// SETUP
// =========================================================
void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE SwooshESP32...");

    // Initialize servo at center 90°
    servo.begin(SERVO_PIN, 90);

    // Setup BLE
    NimBLEDevice::init("SwooshESP32");

    NimBLEServer* server = NimBLEDevice::createServer();
    NimBLEService* service = server->createService(SERVICE_UUID);

    commandCharacteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE
    );

    commandCharacteristic->setCallbacks(new CommandCallback());

    service->start();

    // Start BLE advertising
    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->start();

    Serial.println("BLE advertising started!");
}


// =========================================================
// LOOP
// =========================================================
void loop() {
    delay(50); // Light loop, BLE events handled internally
}
