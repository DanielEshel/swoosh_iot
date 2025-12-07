#include "servo_control.h"
#include <Arduino.h> // For basic Arduino functions
#include <ESP32Servo.h> // The servo library for ESP32

// Global servo object (used within this file)
Servo myServo; 

// Constructor (optional, but good practice)
ServoControl::ServoControl() : currentAngle(90) { 
    // Initialize current angle to a default of 90 degrees
}

// Method to initialize the servo
void ServoControl::begin(int pin, int initialAngle) {
    // Attach the servo object to the specified pin
    myServo.attach(pin); 
    
    // Ensure the initial angle is within the 0-180 degree range
    if (initialAngle >= 0 && initialAngle <= 180) {
        currentAngle = initialAngle;
    } else {
        currentAngle = 90; // Default to center if initial angle is invalid
    }
    
    // Move the physical servo to the initial position
    myServo.write(currentAngle); 
    
    Serial.print("Servo initialized on Pin ");
    Serial.print(pin);
    Serial.print(" at ");
    Serial.print(currentAngle);
    Serial.println(" degrees.");
}

// Method to set the servo angle
void ServoControl::setAngle(int angle) {
    // Basic clamping to ensure the angle is valid (0 to 180 degrees)
    if (angle < 0) {
        angle = 0;
    } else if (angle > 180) {
        angle = 180;
    }

    if (angle != currentAngle) {
        myServo.write(angle);
        currentAngle = angle;
        
        Serial.print("Servo commanded to: ");
        Serial.println(currentAngle);
    }
}

// Method to return the current angle
int ServoControl::getCurrentAngle() const {
    return currentAngle;
}