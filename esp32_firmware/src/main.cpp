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

NimBLECharacteristic* commandCharacteristic;
NimBLECharacteristic* sensorCharacteristic;

// --- SENSOR TIMER ---
unsigned long lastSensorRead = 0;
const long sensorInterval = 500; // Read every 500ms

// --- VIRTUAL GEARBOX VARIABLES ---
float currentVelocity = 0.0;
float currentFloatAngle = 90.0;     // Start in the center
unsigned long lastServoUpdate = 0;
const int servoRefreshRate = 20;    // Update servo every 20ms (50Hz)

// --- FAILSAFE TIMER ---
unsigned long lastCommandTime = 0;  // Tracks the last time the phone sent data

// =========================================================
// BLE Callback — receive commands from iOS/Flutter
// =========================================================
class CommandCallback : public NimBLECharacteristicCallbacks {
    void onWrite(NimBLECharacteristic* c) override {
        std::string value = c->getValue();
        if (value.empty()) return;

        try {
            // 1. Read the raw offset from the iPhone (e.g., -0.15)
            float relativeError = std::stof(value);
            
            // 2. Record the exact time we got this command (for the failsafe)
            lastCommandTime = millis();
            
            // 3. Define how aggressively the servo reacts
            // A multiplier of 10.0 means at the edge of the screen (0.5 error),
            // the servo moves 5 degrees every 20ms. 
            float speedMultiplier = 10.0; 
            
            // 4. Calculate target velocity
            // Optional: Deadzone (ignore micro-movements when perfectly centered)
            if (abs(relativeError) < 0.03) {
                currentVelocity = 0.0;
            } else {
                currentVelocity = - relativeError * speedMultiplier;
            }
            
        } catch (const std::exception& e) {
            Serial.printf("BLE Error: Could not parse float. %s\n", e.what());
        }
    }
};

// =========================================================
// SETUP
// =========================================================
void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE SwooshESP32 with Sensor & Gearbox...");

    servo.begin(SERVO_PIN, 90);
    currentFloatAngle = 90.0; // Ensure float angle matches physical start

    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);

    NimBLEDevice::init("SwooshESP32");
    NimBLEServer* server = NimBLEDevice::createServer();
    NimBLEService* service = server->createService(SERVICE_UUID);

    commandCharacteristic = service->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR
    );
    commandCharacteristic->setCallbacks(new CommandCallback());

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
    unsigned long currentMillis = millis();

    // --- 1. THE VIRTUAL GEARBOX (Runs every 20ms) ---
    if (currentMillis - lastServoUpdate >= servoRefreshRate) {
        lastServoUpdate = currentMillis;

        // Failsafe Brake: If the phone hasn't sent a command in 200ms, 
        // assume the ball is lost or connection dropped, and stop the motor.
        if (currentMillis - lastCommandTime > 200) {
            currentVelocity = 0.0;
        }

        // Only move if we have an active velocity
        if (currentVelocity != 0.0) {
            currentFloatAngle += currentVelocity;

            // Clamp to your 180-degree hardware limits!
            if (currentFloatAngle > 180.0) currentFloatAngle = 180.0;
            if (currentFloatAngle < 0.0) currentFloatAngle = 0.0;

            // Write to the physical servo (cast the float back to an int)
            servo.setAngle((int)currentFloatAngle);
        }
    }

    // --- 2. SENSOR READING (Runs every 500ms) ---
    if (currentMillis - lastSensorRead > sensorInterval) {
        lastSensorRead = currentMillis;

        digitalWrite(TRIG_PIN, LOW);
        delayMicroseconds(2);
        digitalWrite(TRIG_PIN, HIGH);
        delayMicroseconds(10);
        digitalWrite(TRIG_PIN, LOW);

        long duration = pulseIn(ECHO_PIN, HIGH, 25000); 
        int distanceCm = duration * 0.034 / 2;

        if (distanceCm > 0 && distanceCm < 400) {
            std::string distStr = std::to_string(distanceCm);
            sensorCharacteristic->setValue(distStr);
            sensorCharacteristic->notify();
        }
    }

    delay(2); // Small stability delay
}