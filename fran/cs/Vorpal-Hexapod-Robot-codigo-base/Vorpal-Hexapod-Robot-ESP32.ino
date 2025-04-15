////////////////////////////////////////////////////////////////////////////////
//           Vorpal Hexapod Control Program - ESP32 Version
//
// Copyright (C) 2017-2021 Vorpal Robotics, LLC.
// Modified for ESP32 by [Your Name]
//
// This is a port of the original Vorpal Hexapod code to ESP32
// Removed Bluetooth functionality and updated for ESP32 compatibility

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <SPI.h>
#include <EEPROM.h>

// Version information
const char *Version = "#ESP32-1.0"; // ESP32 version

// PWM frequency multiplier, use 1 for analog servos and up to about 3 for digital
int FreqMult = 1;

// Flag for leg position safety
byte SomeLegsUp = 0;

// Robot size configuration
#define HEXSIZE 0 // 0 for hexapod, 1 for megapod, 2 for gigapod

#if HEXSIZE == 0
#define TIMEFACTOR 10L
#define HEXAPOD
#endif

#if HEXSIZE == 1
#define TIMEFACTOR 9L
#define MEGAPOD
#endif

#if HEXSIZE == 2
#define TIMEFACTOR 7L
#define GIGAPOD
#endif

// Servo driver configuration
#define SERVO_IIC_ADDR  (0x45)
Adafruit_PWMServoDriver servoDriver = Adafruit_PWMServoDriver(SERVO_IIC_ADDR);

// ESP32 pin definitions
#define BeeperPin 4
#define ServoTypePin 5
#define ServoTypeGroundPin 6
#define GripElbowCurrentPin 34  // Changed to ESP32 ADC pin
#define GripClawCurrentPin 35   // Changed to ESP32 ADC pin

// PWM configuration
#define PWMFREQUENCY (60*FreqMult)
#define SERVOMIN  (190*FreqMult)
#define SERVOMAX  (540*FreqMult)

// Leg configuration
#define NUM_LEGS 6
#define NUM_ACTIVE_SERVO (2*NUM_LEGS)
#define MAX_SERVO (2*NUM_LEGS)

// Leg bit patterns
#define ALL_LEGS      0b111111
#define LEFT_LEGS     0b000111
#define RIGHT_LEGS    0b111000
#define TRIPOD1_LEGS  0b010101
#define TRIPOD2_LEGS  0b101010
#define QUAD1_LEGS    0b001001
#define QUAD2_LEGS    0b100100
#define FRONT_LEGS    0b100001
#define MIDDLE_LEGS   0b010010
#define BACK_LEGS     0b001100
#define NO_LEGS       0b0

// Individual leg bitmasks
#define LEG0 0b1
#define LEG1 0b10
#define LEG2 0b100
#define LEG3 0b1000
#define LEG4 0b10000
#define LEG5 0b100000

// Leg position macros
#define ISFRONTLEG(LEG) (LEG==0||LEG==5)
#define ISMIDLEG(LEG)   (LEG==1||LEG==4)
#define ISBACKLEG(LEG)  (LEG==2||LEG==3)
#define ISLEFTLEG(LEG)  (LEG==0||LEG==1||LEG==2)
#define ISRIGHTLEG(LEG) (LEG==3||LEG==4||LEG==5)

// Servo angle definitions
#define KNEE_UP_MAX 180
#define KNEE_UP    150
#define KNEE_RELAX  120  
#define KNEE_NEUTRAL 90 
#define KNEE_CROUCH 110
#define KNEE_HALF_CROUCH 80
#define KNEE_STAND 30
#define KNEE_DOWN  30   
#define KNEE_TIPTOES 5
#define KNEE_FOLD 170
#define KNEE_SCAMPER (KNEE_NEUTRAL-20)

// Function declarations
void setup();
void loop();
void initializeServos();
void moveLegs(byte legs, int kneeAngle, int hipAngle);
void setKnee(int leg, int angle);
void setHip(int leg, int angle);
void beep(int duration);
void updateServos();
void stand();
void crouch();
void walkForward();
void walkBackward();
void turnLeft();
void turnRight();
void dance1();
void dance2();
void processCommand(char cmd);

// Global variables
int servoPositions[MAX_SERVO];
int targetServoPositions[MAX_SERVO];

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize servo driver
  servoDriver.begin();
  servoDriver.setPWMFreq(PWMFREQUENCY);
  
  // Initialize pins
  pinMode(BeeperPin, OUTPUT);
  pinMode(ServoTypePin, INPUT_PULLUP);
  pinMode(ServoTypeGroundPin, OUTPUT);
  digitalWrite(ServoTypeGroundPin, LOW);
  
  // Initialize EEPROM
  EEPROM.begin(512);
  
  // Initialize servos
  initializeServos();
  
  // Welcome beep
  beep(100);
}

void loop() {
  // Main control loop
  updateServos();
  
  // Check for serial commands
  if(Serial.available()) {
    char cmd = Serial.read();
    processCommand(cmd);
  }
  
  delay(20); // Small delay to prevent overwhelming the ESP32
}

void initializeServos() {
  // Initialize all servos to neutral position
  for(int i = 0; i < MAX_SERVO; i++) {
    servoPositions[i] = 90;
    targetServoPositions[i] = 90;
  }
  updateServos();
}

void moveLegs(byte legs, int kneeAngle, int hipAngle) {
  for(int leg = 0; leg < NUM_LEGS; leg++) {
    if(legs & (1 << leg)) {
      setKnee(leg, kneeAngle);
      setHip(leg, hipAngle);
    }
  }
}

void setKnee(int leg, int angle) {
  if(angle < 0) angle = 0;
  if(angle > 180) angle = 180;
  targetServoPositions[leg * 2] = angle;
}

void setHip(int leg, int angle) {
  if(angle < 0) angle = 0;
  if(angle > 180) angle = 180;
  // Reverse angle for left legs
  if(ISLEFTLEG(leg)) {
    angle = 180 - angle;
  }
  targetServoPositions[leg * 2 + 1] = angle;
}

void beep(int duration) {
  digitalWrite(BeeperPin, HIGH);
  delay(duration);
  digitalWrite(BeeperPin, LOW);
}

void updateServos() {
  for(int i = 0; i < MAX_SERVO; i++) {
    if(servoPositions[i] != targetServoPositions[i]) {
      int pulse = map(targetServoPositions[i], 0, 180, SERVOMIN, SERVOMAX);
      servoDriver.setPWM(i, 0, pulse);
      servoPositions[i] = targetServoPositions[i];
    }
  }
}

// Movement patterns and control functions
void stand() {
  moveLegs(ALL_LEGS, KNEE_STAND, 90);
}

void crouch() {
  moveLegs(ALL_LEGS, KNEE_CROUCH, 90);
}

void walkForward() {
  // Tripod gait forward
  moveLegs(TRIPOD1_LEGS, KNEE_UP, 45);
  delay(200);
  moveLegs(TRIPOD1_LEGS, KNEE_DOWN, 45);
  moveLegs(TRIPOD2_LEGS, KNEE_UP, 135);
  delay(200);
  moveLegs(TRIPOD2_LEGS, KNEE_DOWN, 135);
}

void walkBackward() {
  // Tripod gait backward
  moveLegs(TRIPOD1_LEGS, KNEE_UP, 135);
  delay(200);
  moveLegs(TRIPOD1_LEGS, KNEE_DOWN, 135);
  moveLegs(TRIPOD2_LEGS, KNEE_UP, 45);
  delay(200);
  moveLegs(TRIPOD2_LEGS, KNEE_DOWN, 45);
}

void turnLeft() {
  // Tripod gait turn left
  moveLegs(TRIPOD1_LEGS, KNEE_UP, 90);
  delay(200);
  moveLegs(TRIPOD1_LEGS, KNEE_DOWN, 90);
  moveLegs(TRIPOD2_LEGS, KNEE_UP, 90);
  delay(200);
  moveLegs(TRIPOD2_LEGS, KNEE_DOWN, 90);
}

void turnRight() {
  // Tripod gait turn right
  moveLegs(TRIPOD1_LEGS, KNEE_UP, 90);
  delay(200);
  moveLegs(TRIPOD1_LEGS, KNEE_DOWN, 90);
  moveLegs(TRIPOD2_LEGS, KNEE_UP, 90);
  delay(200);
  moveLegs(TRIPOD2_LEGS, KNEE_DOWN, 90);
}

void dance1() {
  // Simple dance pattern
  for(int i = 0; i < 3; i++) {
    moveLegs(LEFT_LEGS, KNEE_UP, 90);
    delay(200);
    moveLegs(LEFT_LEGS, KNEE_DOWN, 90);
    moveLegs(RIGHT_LEGS, KNEE_UP, 90);
    delay(200);
    moveLegs(RIGHT_LEGS, KNEE_DOWN, 90);
  }
}

void dance2() {
  // Another dance pattern
  for(int i = 0; i < 3; i++) {
    moveLegs(FRONT_LEGS, KNEE_UP, 90);
    delay(200);
    moveLegs(FRONT_LEGS, KNEE_DOWN, 90);
    moveLegs(BACK_LEGS, KNEE_UP, 90);
    delay(200);
    moveLegs(BACK_LEGS, KNEE_DOWN, 90);
  }
}

// Command processing
void processCommand(char cmd) {
  switch(cmd) {
    case 'S': // Stand
      stand();
      break;
    case 'C': // Crouch
      crouch();
      break;
    case 'F': // Forward
      walkForward();
      break;
    case 'B': // Backward
      walkBackward();
      break;
    case 'L': // Left
      turnLeft();
      break;
    case 'R': // Right
      turnRight();
      break;
    case 'D': // Dance 1
      dance1();
      break;
    case 'E': // Dance 2
      dance2();
      break;
  }
} 