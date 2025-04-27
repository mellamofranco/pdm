# Vorpal Hexapod Robot - ESP32 Version

## Overview
This project is a port of the original Vorpal Hexapod control software to the ESP32 platform. The port removes Bluetooth functionality found in the original and updates the code for ESP32 compatibility.

## Features
- Control a hexapod robot with 6 legs (12 servos total)
- Multiple movement patterns: walking, turning, dancing
- Control via Serial commands
- Configurable for different robot sizes (hexapod, megapod, gigapod)
- Servo control via I2C PWM driver

## Hardware Requirements
- ESP32 development board
- Adafruit PWM Servo Driver (I2C address: 0x45)
- 12 servo motors (2 per leg - hip and knee)
- Piezo buzzer for sound effects
- Optional: Grip mechanism with current sensing

## Wiring Guide
| Component | Connection |
|-----------|------------|
| Piezo Buzzer | Pin 4 |
| Servo Type Detection | Pin 5 (input with pull-up) |
| Servo Type Ground | Pin 6 |
| Grip Elbow Current Sensor | Pin 34 (Analog input) |
| Grip Claw Current Sensor | Pin 35 (Analog input) |
| Servo Driver | I2C (default ESP32 SDA/SCL pins) |

## Servo Connections
The code expects servos to be connected to the PWM driver as follows:
- Channels 0, 2, 4, 6, 8, 10: Knee servos for legs 0-5
- Channels 1, 3, 5, 7, 9, 11: Hip servos for legs 0-5

## Robot Configuration
- `HEXSIZE`: Set to 0 for hexapod (default), 1 for megapod, or 2 for gigapod
- `FreqMult`: PWM frequency multiplier (1 for analog servos, up to 3 for digital servos)

## Leg Configuration
```
     [Front]
  Leg0    Leg5
  Leg1    Leg4
  Leg2    Leg3
     [Back]
```

- Legs 0-2: Left side
- Legs 3-5: Right side
- Legs 0,5: Front legs
- Legs 1,4: Middle legs
- Legs 2,3: Back legs

## Control Commands
Send the following ASCII characters over serial (115200 baud) to control the robot:

| Command | Action | Description |
|---------|--------|-------------|
| S | Stand | Sets all legs to standing position |
| C | Crouch | Sets all legs to crouching position |
| F | Forward | Walks forward using tripod gait |
| B | Backward | Walks backward using tripod gait |
| L | Left | Turns left |
| R | Right | Turns right |
| D | Dance 1 | Performs alternating left-right dance |
| E | Dance 2 | Performs alternating front-back dance |

## Function Reference

### Core Functions
- `setup()`: Initializes hardware, communications, and servo positions
- `loop()`: Main program loop, processes incoming serial commands
- `initializeServos()`: Sets all servos to neutral position (90°)
- `updateServos()`: Updates physical servo positions based on target positions

### Movement Control
- `moveLegs(byte legs, int kneeAngle, int hipAngle)`: Moves specified legs to given angles
- `setKnee(int leg, int angle)`: Sets a specific knee servo angle
- `setHip(int leg, int angle)`: Sets a specific hip servo angle (automatically reverses for left legs)
- `stand()`: Places robot in standing position
- `crouch()`: Places robot in crouched position

### Gait Patterns
- `walkForward()`: Implements forward walking tripod gait
- `walkBackward()`: Implements backward walking tripod gait
- `turnLeft()`: Implements left turning movement
- `turnRight()`: Implements right turning movement
- `dance1()`, `dance2()`: Implements dance patterns

### Utility Functions
- `beep(int duration)`: Produces a beep of specified duration
- `processCommand(char cmd)`: Processes incoming serial commands

## Extending the Code
To add new movements:
1. Create a new function implementing the movement pattern
2. Add the command character and function call to the `processCommand()` switch statement
3. Send the corresponding character over serial

## Troubleshooting
- If servos aren't responsive, check power supply and I2C connections
- If movements seem incorrect, verify servo connections match the expected order
- For erratic behavior, ensure `FreqMult` is appropriate for your servo type
- Use the serial monitor at 115200 baud to send commands and debug

## Credits
- Original Vorpal Hexapod by Vorpal Robotics, LLC.
- ESP32 port modifications