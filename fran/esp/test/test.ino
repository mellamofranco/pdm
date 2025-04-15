#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Create PWM driver object with address 0x05 (5 in decimal)
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x45);

// Servo configuration
#define SERVO_CHANNEL 15     // Using channel 12 of PCA9685
#define SERVO_FREQ 50        // Standard 50Hz for servos
#define SERVO_MIN_PULSE 150  // Minimum pulse width (adjust for your servo)
#define SERVO_MAX_PULSE 600  // Maximum pulse width (adjust for your servo)

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 Servo Control with PCA9685");
  
  // Initialize I2C and PWM driver
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  
  delay(10);
}

// Function to set servo angle (0-180 degrees)
void setServoAngle(uint8_t channel, int angle) {
  // Map angle to pulse length
  int pulse = map(angle, 0, 180, SERVO_MIN_PULSE, SERVO_MAX_PULSE);
  pwm.setPWM(channel, 0, pulse);
}

void loop() {
  // Move servo from 0 to 180 degrees
  for (int angle = 0; angle <= 180; angle += 5) {
    setServoAngle(SERVO_CHANNEL, angle);
    delay(50);
  }
  
  delay(500); // Pause at maximum position
  
  // Move servo from 180 to 0 degrees
  for (int angle = 180; angle >= 0; angle -= 5) {
    setServoAngle(SERVO_CHANNEL, angle);
    delay(50);
  }
  
  delay(500); // Pause at minimum position
}