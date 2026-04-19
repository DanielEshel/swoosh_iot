#include <Arduino.h>
#include <NimBLEDevice.h>
#include "servo_control.h"

#define SERVO_PIN 18
ServoControl servo;

// --- HC-SR04 SENSOR PINS ---
#define TRIG_PIN 5
#define ECHO_PIN 17

// BLE UUIDs
#define SERVICE_UUID           "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID_RX "12345678-1234-1234-1234-1234567890ac"
#define CHARACTERISTIC_UUID_TX "12345678-1234-1234-1234-1234567890ad"

// BLE characteristic pointers
NimBLECharacteristic* commandCharacteristic;
NimBLECharacteristic* sensorCharacteristic;

// Timer for sensor reading
unsigned long lastSensorRead = 0;
const long sensorInterval = 500; // Read every 500ms

// =========================================================
// BLE Callback — receive commands from iOS/Flutter
// =========================================================
class CommandCallback : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        std::string value = c->getValue();
        if (value.empty()) return;

        // Print exactly what arrived from the iPhone
        Serial.printf("BLE Received Raw String: '%s'\n", value.c_str());

        try {
            // Convert to integer and move servo
            int targetAngle = std::stoi(value);
            servo.setAngle(targetAngle);
        } catch (const std::exception& e) {
            // This prevents the ESP32 from rebooting if it gets non-numeric data
            Serial.printf("BLE Error: Could not parse integer from string. %s\n", e.what());
        }
    }
};

// =========================================================
// SETUP
// =========================================================
void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE SwooshESP32 with Sensor...");

    // Setup Servo
    servo.begin(SERVO_PIN, 90);

    // Setup HC-SR04
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    // Setup BLE
    NimBLEDevice::init("SwooshESP32");
    NimBLEServer* server = NimBLEDevice::createServer();
    NimBLEService* service = server->createService(SERVICE_UUID);

    // RX Characteristic (Write) - Added WRITE_NR for iOS compatibility
    commandCharacteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    commandCharacteristic->setCallbacks(new CommandCallback());

    // TX Characteristic (Notify) - For Sensor
    sensorCharacteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    service->start();

    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);
    advertising->start();

    Serial.println("BLE advertising started! Ready for connection...");
}

// =========================================================
// LOOP
// =========================================================
void loop() {
    // Non-blocking timer for sensor
    if (millis() - lastSensorRead > sensorInterval) {
        lastSensorRead = millis();

        // 1. Trigger Pulse
        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);

        // 2. Read Echo (Timeout 25ms)
        long duration = pulseIn(ECHO_PIN, HIGH, 25000); 

        // 3. Calculate Distance
        int distanceCm = duration * 0.034 / 2;

        // 4. Send over BLE if value is valid
        if (distanceCm > 0 && distanceCm < 400) {
            std::string distStr = std::to_string(distanceCm);
            sensorCharacteristic->setValue(distStr);
            sensorCharacteristic->notify();
            // Optional: Serial.printf("Distance: %d cm\n", distanceCm);
        }
    }

    delay(10); // Small stability delay
}