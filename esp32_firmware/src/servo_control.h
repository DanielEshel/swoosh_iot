#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

// The ServoControl class encapsulates all servo-related functionality.
class ServoControl {
public:
    // Constructor
    ServoControl();

    // Initializes the servo on the specified pin and sets the initial angle.
    void begin(int pin, int initialAngle = 90); 
    
    // Sets the servo angle immediately.
    void setAngle(int angle);

    // Returns the current commanded angle.
    int getCurrentAngle() const;

private:
    // Internal variable to store the current angle.
    int currentAngle; 
};

#endif // SERVO_CONTROL_H