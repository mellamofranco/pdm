

// A0 → GPIO36 (ADC1_CH0) - For the mode selection dial/potentiometer
// A1 → GPIO32 (ADC1_CH4) - Used as HIGH output for potentiometer power
// A2 → GPIO33 (ADC1_CH5) - Used as LOW output for potentiometer ground
// A3 → GPIO39 (ADC1_CH3) - Used for sensor data sent over Bluetooth


// BeeperPin (D4) → GPIO25 (good for PWM/tone output)
// ServoTypePin (D5) → GPIO26
// ServoTypeGroundPin (D6) → GPIO27
// ULTRAOUTPUTPIN/TRIG (D7) → GPIO14
// ULTRAINPUTPIN/ECHO (D8) → GPIO12
// I2C SDA → GPIO21 (for servo driver communication)
// I2C SCL → GPIO22 (for servo driver communication)

// A0 Potentiometer signal (white wire)
// A1 Potentiometer Power (red wire)
// A2 Potentiometer Ground (black wire)
// A4 Servo Controller SDA
// A5 Servo Controller SCL


#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <SPI.h>
#include <EEPROM.h>

int FreqMult = 1;   // PWM frequency multiplier, use 1 for analog servos and up to about 3 for digital.
          // The recommended setting for digital is 2 (probably safe for all digital servos)
          // A shunt between Nano D5 and D6 will set this to "1" in setup, this allows you
          // to select digital servo mode (2) or analog servo mode (1) using a shunt 
          // or short jumper wire.
          
byte SomeLegsUp = 0;  // this is a flag to detect situations where a user rapidly switches moves that would
            // cause the robot to try to come up off the ground using fewer than all the legs 
            //(and thus over-stressing the servos).

#define HEXSIZE 0 // set this to 0 for hexapod, 1 for megapod and 2 for gigapod. The megapod works ok at 0 as well, but crawl mode doesn't get anywhere. Don't run the gigapod on anything other than 2!

#if HEXSIZE == 0
#define TIMEFACTOR 10L
#define HEXAPOD
#endif

#if HEXSIZE == 1
#define TIMEFACTOR 9L // slightly slower, 90% speed
#define MEGAPOD
#endif

#if HEXSIZE == 2
#define TIMEFACTOR 7L // much slower
#define GIGAPOD
#endif


#define SERVO_IIC_ADDR  (0x45)    // default servo driver IIC address
Adafruit_PWMServoDriver servoDriver = Adafruit_PWMServoDriver(SERVO_IIC_ADDR); 

#define BeeperPin 4           // digital 4 used for beeper
#define ServoTypePin 5        // 5 is used to signal digital vs. analog servo mode
#define ServoTypeGroundPin 6  // 6 provides a ground to pull 5 low if digital servos are in use
#define BF_ERROR  100         // deep beep for error situations
#define BD_MED    50          // medium long beep duration

#define PWMFREQUENCY (60*FreqMult)

#define SERVOMIN  (190*FreqMult) // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  (540*FreqMult) // this is the 'maximum' pulse length count (out of 4096)

// Basic functions that move legs take a bit pattern
// indicating which legs to move. The legs are numbered
// clockwise starting with the right front leg being
// number zero, going around
// to the left legs, and finishing with the left front leg
// being number 5

#define NUM_LEGS 6
#define NUM_ACTIVE_SERVO (2*NUM_LEGS+NUM_GRIPSERVOS)
#define MAX_SERVO (2*NUM_LEGS+MAX_GRIPSERVOS)

// Bit patterns for different combinations of legs
// bottom six bits. LSB is leg number 0

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

// individual leg bitmasks
#define LEG0 0b1
#define LEG1 0b10
#define LEG2 0b100
#define LEG3 0b1000
#define LEG4 0b10000
#define LEG5 0b100000

#define LEG0BIT  0b1
#define LEG1BIT  0b10
#define LEG2BIT  0b100
#define LEG3BIT  0b1000
#define LEG4BIT  0b10000
#define LEG5BIT  0b100000

#define ISFRONTLEG(LEG) (LEG==0||LEG==5)
#define ISMIDLEG(LEG)   (LEG==1||LEG==4)
#define ISBACKLEG(LEG)  (LEG==2||LEG==3)
#define ISLEFTLEG(LEG)  (LEG==0||LEG==1||LEG==2)
#define ISRIGHTLEG(LEG) (LEG==3||LEG==4||LEG==5)

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

#define KNEE_TRIPOD_UP (KNEE_NEUTRAL-40)
#define KNEE_TRIPOD_ADJ 30
#define KNEE_RIPPLE_UP (KNEE_NEUTRAL-40)
#define KNEE_RIPPLE_DOWN (KNEE_DOWN)
#define TWITCH_ADJ 60

#define HIPSWING 25      // how far to swing hips on gaits like tripod or quadruped
#define HIPSMALLSWING 10  // when in fine adjust mode how far to move hips
#define HIPSWING_RIPPLE 25
#define HIP_FORWARD_MAX 175
#define HIP_FORWARD (HIP_NEUTRAL+HIPSWING)
#define HIP_FORWARD_SMALL (HIP_NEUTRAL+HIPSMALLSWING)
#define HIP_NEUTRAL 90
#define HIP_BACKWARD (HIP_NEUTRAL-HIPSWING)
#define HIP_BACKWARD_SMALL (HIP_NEUTRAL-HIPSMALLSWING)
#define HIP_BACKWARD_MAX 0
#define HIP_FORWARD_RIPPLE (HIP_NEUTRAL+HIPSWING_RIPPLE)
#define HIP_BACKWARD_RIPPLE (HIP_NEUTRAL-HIPSWING_RIPPLE)
#define HIP_FOLD 150

#define NOMOVE (-1)   // fake value meaning this aspect of the leg (knee or hip) shouldn't move

#define LEFT_START 3  // first leg that is on the left side
#define RIGHT_START 0 // first leg that is on the right side
#define KNEE_OFFSET 6 // add this to a leg number to get the knee servo number

// these modes are used to interpret incoming bluetooth commands

#define TRIPOD_CYCLE_TIME 750
#define RIPPLE_CYCLE_TIME 1000
#define FIGHT_CYCLE_TIME 660

#define MODE_WALK   'W'
#define MODE_WALK2  'X'
#define MODE_DANCE  'D'
#define MODE_DANCE2 'Y'
#define MODE_FIGHT  'F'
#define MODE_FIGHT2 'Z'
#define MODE_RECORD 'R'
#define MODE_LEG    'L'       // comes from scratch
#define MODE_GAIT   'G'       // comes from scratch
#define MODE_TRIM   'T'       // gamepad in trim mode

#define SUBMODE_1 '1'
#define SUBMODE_2 '2'
#define SUBMODE_3 '3'
#define SUBMODE_4 '4'

#define BATTERYSAVER 5000   // milliseconds in stand mode before servos all detach to save power and heat buildup

// Definitions for the Grip Arm optional attachment

#define GRIPARM_ELBOW_SERVO 12
#define GRIPARM_CLAW_SERVO  13
#define MAX_GRIPSERVOS 2

#define GRIPARM_ELBOW_DEFAULT 90
#define GRIPARM_CLAW_DEFAULT 90
#define GRIPARM_ELBOW_MIN 30
#define GRIPARM_ELBOW_MAX 180
#define GRIPARM_CLAW_MIN 30
#define GRIPARM_CLAW_MAX 120
#define GRIPARM_CURRENT_DANGER (980)

#define GRIPARM_ELBOW_NEUTRAL 90
#define GRIPARM_CLAW_NEUTRAL 90

// Definitions for the servos

unsigned short ServoPos[MAX_SERVO]; // the last commanded position of each servo
unsigned short ServoTarget[MAX_SERVO];
long ServoTime[MAX_SERVO]; // the time that each servo was last commanded to a new position
byte ServoTrim[MAX_SERVO];  // trim values for fine adjustments to servo horn positions
long startedStanding = 0;   // the last time we started standing, or reset to -1 if we didn't stand recently
long LastReceiveTime = 0;   // last time we got a bluetooth packet
unsigned long LastValidReceiveTime = 0;  // last time we got a completely valid packet including correct checksum

byte HC05_pad = 0;  // if set to 1 we will pad out sensor data with nulls to 230 bytes to support some newer hc05 models that buffer.
                    // this is turned on if we see padding coming from the gamepad.

#define DIALMODE_STAND 0
#define DIALMODE_ADJUST 1
#define DIALMODE_TEST 2
#define DIALMODE_DEMO 3
#define DIALMODE_RC_GRIPARM 4
#define DIALMODE_RC 5

int Dialmode;   // What's the robot potentiometer set to?


#define NUM_GRIPSERVOS ((Dialmode == DIALMODE_RC_GRIPARM)?2:0)  // if we're in griparm mode there are 2 griparm servos, else there are none

unsigned long hexmillis() {  // millis that takes into account hexapod size for leg timings
  unsigned long m = (millis() * TIMEFACTOR)/10L;
  return m;
}

void beep(int f, int t) {
  if (f > 0 && t > 0) {
    tone(BeeperPin, f, t);
  } else {
    noTone(BeeperPin);
  }
}

void beep(int f) {  // if no second param is given we'll default to 250 milliseconds for the beep
  beep(f, 250);
}

///////////////////////////////////////////////////////////////
// Trim functions
///////////////////////////////////////////////////////////////
byte TrimInEffect = 1;
byte TrimCurLeg = 0;
byte TrimPose = 0;
#define TRIM_ZERO 127   // this value is the midpoint of the trim range (a byte)

void save_trims() {
  Serial.print("SAVE TRIMS:");
  for (int i = 0; i < NUM_LEGS * 2; i++) {
    if (EEPROM.read(i + 1) != ServoTrim[i]) { // Only write if the value is different
      EEPROM.write(i + 1, ServoTrim[i]);
    }
    Serial.print(ServoTrim[i]);
    Serial.print(" ");
  }
  Serial.println("");
  if (EEPROM.read(0) != 'V') { // Only write if the value is different
    EEPROM.write(0, 'V');
  }
}
void erase_trims() {
  Serial.println("ERASE TRIMS");
  for (int i = 0; i < NUM_LEGS*2; i++) {
    ServoTrim[i] = TRIM_ZERO;
  }
}


// This function sets the positions of both the knee and hip in 
// a single command.  For hip, the left side is reversed so
// forward direction is consistent.

// This function takes a bitmask to specify legs to move, note that
// the basic setHip and setKnee functions take leg numbers, not masks

// if a position is -1 then that means don't change that item

void setLeg(int legmask, int hip_pos, int knee_pos, int adj) {
  setLeg(legmask, hip_pos, knee_pos, adj, 0, 0);  // use the non-raw version with leanangle=0
}

// version with leanangle = 0
void setLeg(int legmask, int hip_pos, int knee_pos, int adj, int raw) {
  setLeg(legmask, hip_pos, knee_pos, adj, raw, 0);
}

// This is the full version of setLeg with all the features
void setLeg(int legmask, int hip_pos, int knee_pos, int adj, int raw, int leanangle) {
  for (int i = 0; i < NUM_LEGS; i++) {
    if (legmask & 0b1) {  // if the lowest bit is ON then we are moving this leg
      if (hip_pos != NOMOVE) {
        if (!raw) {
          setHip(i, hip_pos, adj);
        } else {
          setHipRaw(i, hip_pos);
        }
      }
      if (knee_pos != NOMOVE) {
        int pos = knee_pos;
        if (leanangle != 0) {
          switch (i) {
            case 0: case 6: case 5: case 11:
              if (leanangle < 0) pos -= leanangle;
              break;
            case 1: case 7: case 4: case 10:
              pos += abs(leanangle/2);
              break;
            case 2: case 8: case 3: case 9:
              if (leanangle > 0) pos += leanangle;
              break;
          }
          //Serial.print("Lean:"); Serial.print(leanangle); Serial.print("pos="); Serial.println(pos);
        }
        
        setKnee(i, pos);
      }
    }
    legmask = (legmask>>1);  // shift down one bit position to check the next legmask bit
  }
}

// this version of setHip does no processing at all (for example
// to distinguish left from right sides)
void setHipRaw(int leg, int pos) {
  setServo(leg, pos);
}

// this version of setHip adjusts for left and right legs so
// that 0 degrees moves "forward" i.e. toward legs 5-0 which is
// nominally the front of the robot

void setHip(int leg, int pos) {
  // reverse the left side for consistent forward motion
  if (leg >= LEFT_START) {
    pos = 180 - pos;
  }
  setHipRaw(leg, pos);
}

// this version of setHip adjusts not only for left and right,
// but also shifts the front legs a little back and the back legs
// forward to make a better balance for certain gaits like tripod or quadruped

void setHip(int leg, int pos, int adj) {
  if (ISFRONTLEG(leg)) {
    pos -= adj;
  } else if (ISBACKLEG(leg)) {
    pos += adj;
  }
  // reverse the left side for consistent forward motion
  if (leg >= LEFT_START) {
    pos = 180 - pos;
  }

  setHipRaw(leg, pos);
}

// this version of setHip doesn't do mirror images like raw, but it
// does honor the adjust parameter to shift the front/back legs

void setHipRawAdj(int leg, int pos, int adj) {
  if (leg == 5 || leg == 2) {
    pos += adj;
  } else if (leg == 0 || leg == 3) {
    pos -= adj;
  }

  setHipRaw(leg, pos);
}

void setKnee(int leg, int pos) {
  // find the knee associated with leg if this is not already a knee
  if (leg < KNEE_OFFSET) {
    leg += KNEE_OFFSET;
  }
  setServo(leg, pos);
}

void setGrip(int elbow, int claw) {
    setServo(GRIPARM_ELBOW_SERVO, elbow);
    setServo(GRIPARM_CLAW_SERVO, claw);
}

void turn(int ccw, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod) {
  turn(ccw, hipforward, hipbackward, kneeup, kneedown, timeperiod, 0);
}

void turn(int ccw, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle) {
  // use tripod groups to turn in place
  if (ccw) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }

#define NUM_TURN_PHASES 6
#define FBSHIFT_TURN    40   // shift front legs back, back legs forward, this much
  
  long t = hexmillis()%timeperiod;
  long phase = (NUM_TURN_PHASES*t)/timeperiod;

  //Serial.print("PHASE: ");
  //Serial.println(phase);

  switch (phase) {
    case 0:
      // in this phase, center-left and noncenter-right legs raise up at
      // the knee
      setLeg(TRIPOD1_LEGS, NOMOVE, kneeup, 0);
      break;

    case 1:
      // in this phase, the center-left and noncenter-right legs move clockwise
      // at the hips, while the rest of the legs move CCW at the hip
      setLeg(TRIPOD1_LEGS, hipforward, NOMOVE, FBSHIFT_TURN, 1);
      setLeg(TRIPOD2_LEGS, hipbackward, NOMOVE, FBSHIFT_TURN, 1);
      break;

    case 2: 
      // now put the first set of legs back down on the ground
      setLeg(TRIPOD1_LEGS, NOMOVE, kneedown, 0);
      break;

    case 3:
      // lift up the other set of legs at the knee
      setLeg(TRIPOD2_LEGS, NOMOVE, kneeup, 0);
      break;
      
    case 4:
      // similar to phase 1, move raised legs CW and lowered legs CCW
      setLeg(TRIPOD1_LEGS, hipbackward, NOMOVE, FBSHIFT_TURN, 1);
      setLeg(TRIPOD2_LEGS, hipforward, NOMOVE, FBSHIFT_TURN, 1);
      break;

    case 5:
      // put the second set of legs down, and the cycle repeats
      setLeg(TRIPOD2_LEGS, NOMOVE, kneedown, 0);
      break;  
  }
  
}


void stand() {
  transactServos();
    setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_STAND, 0);
  commitServos();
}

void stand_90_degrees() {  // used to install servos, sets all servos to 90 degrees
  transactServos();
  setLeg(ALL_LEGS, 90, 90, 0);
  setGrip(90,90);
  commitServos();
}

void laydown() {
  setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_UP, 0);
}

void tiptoes() {
  setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, 0);
}

void wave(int dpad) {
  
#define NUM_WAVE_PHASES 12
#define WAVE_CYCLE_TIME 900
#define KNEE_WAVE  60
  long t = hexmillis()%WAVE_CYCLE_TIME;
  long phase = (NUM_WAVE_PHASES*t)/WAVE_CYCLE_TIME;

  if (dpad == 'b') {
    phase = 11-phase;  // go backwards
  }

  switch (dpad) {
    case 'f':
    case 'b':
      // swirl around
      setLeg(ALL_LEGS, HIP_NEUTRAL, NOMOVE, 0); // keep hips stable at 90 degrees
      if (phase < NUM_LEGS) {
        setKnee(phase, KNEE_WAVE);
      } else {
        setKnee(phase-NUM_LEGS, KNEE_STAND);
      }
      break;
    case 'l':
      // teeter totter around font/back legs
   
      if (phase < NUM_WAVE_PHASES/2) {
        setKnee(0, KNEE_TIPTOES);
        setKnee(5, KNEE_STAND);
        setHipRaw(0, HIP_FORWARD);
        setHipRaw(5, HIP_BACKWARD-40);
        setKnee(2, KNEE_TIPTOES);
        setKnee(3, KNEE_STAND);
        setHipRaw(2, HIP_BACKWARD);
        setHipRaw(3, HIP_FORWARD+40);
                
        setLeg(LEG1, HIP_NEUTRAL, KNEE_TIPTOES, 0);
        setLeg(LEG4, HIP_NEUTRAL, KNEE_NEUTRAL, 0);
      } else {
        setKnee(0, KNEE_STAND);
        setKnee(5, KNEE_TIPTOES);
        setHipRaw(0, HIP_FORWARD+40);
        setHipRaw(5, HIP_BACKWARD);
        setKnee(2, KNEE_STAND);
        setKnee(3, KNEE_TIPTOES);
        setHipRaw(2, HIP_BACKWARD-40);
        setHipRaw(3, HIP_FORWARD);
           
        setLeg(LEG1, HIP_NEUTRAL, KNEE_NEUTRAL, 0);
        setLeg(LEG4, HIP_NEUTRAL, KNEE_TIPTOES, 0);
      }
      break;
    case 'r':
      // teeter totter around middle legs
      setLeg(MIDDLE_LEGS, HIP_NEUTRAL, KNEE_STAND, 0);
      if (phase < NUM_LEGS) {
        setLeg(FRONT_LEGS, HIP_NEUTRAL, KNEE_NEUTRAL, 0);
        setLeg(BACK_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, 0);
      } else {
        setLeg(FRONT_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, 0);
        setLeg(BACK_LEGS, HIP_NEUTRAL, KNEE_NEUTRAL, 0);       
      }
      break;
    case 'w':
      // lay on ground and make legs go around in a wave
      setLeg(ALL_LEGS, HIP_NEUTRAL, NOMOVE, 0);
      int p = phase/2;
      for (int i = 0; i < NUM_LEGS; i++) {
        if (i == p) {
          setKnee(i, KNEE_UP_MAX);
        } else {
          setKnee(i, KNEE_NEUTRAL);
        }
      }
      return;
      if (phase < NUM_LEGS) {
        setKnee(phase/2, KNEE_UP);
      } else {
        int p = phase-NUM_LEGS;
        if (p < 0) p+=NUM_LEGS;
        setKnee(p/2, KNEE_NEUTRAL+10);
      }
      break;
  }
}

void griparm_mode(char dpad) {
    // this mode retains state and moves slowly

    //Serial.print("Grip:"); Serial.print(dpad); 

    Serial.println();
      switch (dpad) {
      case 's': 
        // do nothing in stop mode, just hold current position
        for (int i = 0; i < NUM_ACTIVE_SERVO; i++) {
          ServoTarget[i] = ServoPos[i];
        }
        // ServoTarget[GRIPARM_ELBOW_SERVO] = ServoPos[GRIPARM_ELBOW_SERVO];
        // ServoTarget[GRIPARM_CLAW_SERVO] = ServoPos[GRIPARM_CLAW_SERVO];
        break;
      case 'w':  // reset to standard grip arm position, arm raised to mid-level and grip open a medium amount
        ServoTarget[GRIPARM_ELBOW_SERVO] = GRIPARM_ELBOW_DEFAULT;
        ServoTarget[GRIPARM_CLAW_SERVO] = GRIPARM_CLAW_DEFAULT;
        break;
      case 'f': // Elbow up
        ServoTarget[GRIPARM_ELBOW_SERVO] = GRIPARM_ELBOW_MIN;
        break;
      case 'b': // Elbow down
        ServoTarget[GRIPARM_ELBOW_SERVO] = GRIPARM_ELBOW_MAX;
        break;
      case 'l':  // Claw closed
        ServoTarget[GRIPARM_CLAW_SERVO] = GRIPARM_CLAW_MAX;
        break;
      case 'r': // Claw open
        ServoTarget[GRIPARM_CLAW_SERVO] = GRIPARM_CLAW_MIN;
        break;
    }

    // The Smoothmove function will take care of moving the grip arm servos to targets
    
}


void fight_mode(char dpad, int mode, long timeperiod) {

#define HIP_FISTS_FORWARD 130

  if (Dialmode == DIALMODE_RC_GRIPARM && mode == SUBMODE_2) {
    // we're really not fighting, we're controlling the grip arm if GRIPARM is nonzero
    griparm_mode(dpad);
    return;
  }
 
  if (mode == SUBMODE_3) {
    // in this mode the robot leans forward, left, or right by adjusting hips only

    // this mode retains state and moves slowly, it's for getting somethign like the joust or 
    // capture the flag accessories in position

      switch (dpad) {
      case 's': 
        // do nothing in stop mode, just hold current position
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i+KNEE_OFFSET] = ServoPos[i+NUM_LEGS];
          ServoTarget[i] = ServoPos[i];
        }
        break;
      case 'w':  // reset to standard standing position, resets both hips and knees
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i+KNEE_OFFSET] = KNEE_STAND;
          ServoTarget[i] = 90;
        }
        break;
      case 'f': // swing hips forward, mirrored
        ServoTarget[5] = ServoTarget[4] = ServoTarget[3] = 125;
        ServoTarget[0] = ServoTarget[1] = ServoTarget[2] = 55;
        break;
      case 'b': // move the knees back up to standing position, leave hips alone
        ServoTarget[5] = ServoTarget[4] = ServoTarget[3] = 55;
        ServoTarget[0] = ServoTarget[1] = ServoTarget[2] = 125;
        break;
      case 'l':
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i] = 170;
        }
        break;
      case 'r':
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i] = 10;
        }
        break;
    }
    
  } else if (mode == SUBMODE_4) {
    // in this mode the entire robot leans in the direction of the pushbuttons
    // and the weapon button makes the robot return to standing position.

    // Only knees are altered by this, not hips (other than the reset action for
    // the special D-PAD button)

    // this mode does not immediately set servos to final positions, instead it
    // moves them toward targets slowly.
    
    switch (dpad) {
      case 's': 
        // do nothing in stop mode, just hold current position
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i+KNEE_OFFSET] = ServoPos[i+NUM_LEGS];
          ServoTarget[i] = ServoPos[i];
        }
        break;
      case 'w':  // reset to standard standing position, resets both hips and knees
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i+KNEE_OFFSET] = KNEE_STAND;
          ServoTarget[i] = 90;
        }
        break;
      case 'f': // move knees into forward crouch, leave hips alone

        if (ServoPos[8] == KNEE_STAND) { // the back legs are standing, so crouch the front legs
          ServoTarget[6]=ServoTarget[11]=KNEE_CROUCH;
          ServoTarget[7]=ServoTarget[10]=KNEE_HALF_CROUCH;
          ServoTarget[8]=ServoTarget[9]=KNEE_STAND;
        } else { // bring the back legs up first
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i+KNEE_OFFSET] = KNEE_STAND;
          }
        }
        break;
      case 'b': // move back legs down so robot tips backwards
        if (ServoPos[6] == KNEE_STAND) { // move the back legs down
          ServoTarget[6]=ServoTarget[11]=KNEE_STAND;
          ServoTarget[7]=ServoTarget[10]=KNEE_HALF_CROUCH;
          ServoTarget[8]=ServoTarget[9]=KNEE_CROUCH;
        } else { // front legs are down, return to stand first
            for (int i = 0; i < NUM_LEGS; i++) {
              ServoTarget[i+KNEE_OFFSET] = KNEE_STAND;
            }
        }
        break;
     case 'l':
        if (ServoPos[9] == KNEE_STAND) {
          ServoTarget[6]=ServoTarget[8] = KNEE_HALF_CROUCH;
          ServoTarget[7]=KNEE_CROUCH;
          ServoTarget[9]=ServoTarget[10]=ServoTarget[11]=KNEE_STAND;
        } else {
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i+KNEE_OFFSET] = KNEE_STAND;
          }
        }
        break;
      case 'r':
        if (ServoPos[6] == KNEE_STAND) {
          ServoTarget[6]=ServoTarget[7]=ServoTarget[8] = KNEE_STAND;
          ServoTarget[9]=ServoTarget[11]=KNEE_HALF_CROUCH;
          ServoTarget[10]=KNEE_CROUCH;
        } else {
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i+KNEE_OFFSET] = KNEE_STAND;
          }
        }
        break;

    }
  }

  if (mode >= SUBMODE_3) {
    return; // we're done, the smoothmove function will take care of reaching the target positions
  }

  // If we get here, we are in either submode 1 or 2
  //
  // submode 1: fight with two front legs, individual movement
  // submode 2: fight with two front legs, in unison
  
  setLeg(MIDDLE_LEGS, HIP_FORWARD+10, KNEE_STAND, 0);
  setLeg(BACK_LEGS, HIP_BACKWARD, KNEE_STAND, 0);
  
  switch (dpad) {
    case 's':  // stop mode: both legs straight out forward
      setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, KNEE_NEUTRAL, 0);

      break;
      
    case 'f':  // both front legs move up in unison
      setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, KNEE_UP, 0);
      break;
    
    case 'b':  // both front legs move down in unison
      setLeg(FRONT_LEGS, HIP_FORWARD, KNEE_STAND, 0);
      break;
    
    case 'l':  // left front leg moves left, right stays forward
      if (mode == SUBMODE_1) {
        setLeg(LEG0, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_RELAX, 0);
      } else {
        // both legs move in unison in submode B
        setLeg(LEG0, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG5, HIP_FISTS_FORWARD+30, KNEE_RELAX, 0);
      }
      break;
    
    case 'r':  // right front leg moves right, left stays forward
      if (mode == SUBMODE_1) {
        setLeg(LEG5, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_RELAX, 0);
      } else { // submode B
        setLeg(LEG5, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG0, HIP_FISTS_FORWARD+30, KNEE_RELAX, 0);
      }
      break;
    
    case 'w':  // automatic ninja motion mode with both legs swinging left/right/up/down furiously!

#define NUM_PUGIL_PHASES 8
        {  // we need a new scope for this because there are local variables
        
        long t = hexmillis()%timeperiod;
        long phase = (NUM_PUGIL_PHASES*t)/timeperiod;
      
        //Serial.print("PHASE: ");
        //Serial.println(phase);
    
        switch (phase) {
          case 0:
            // Knees down, hips forward
            setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, (mode==SUBMODE_2)?KNEE_DOWN:KNEE_RELAX, 0);
            break;
      
          case 1:
            // Knees up, hips forward
            setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, KNEE_UP, 0);
            break;
      
          case 2:
            // Knees neutral, hips neutral
            setLeg(FRONT_LEGS, HIP_BACKWARD, KNEE_NEUTRAL, 0);
            break;
      
          case 3:
            // Knees up, hips neutral
            setLeg(FRONT_LEGS, HIP_BACKWARD, KNEE_UP, 0);
            break;
      
          case 4:
             // hips forward, kick
             setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_UP, 0);
             setLeg(LEG5, HIP_FISTS_FORWARD, (mode==SUBMODE_2)?KNEE_DOWN:KNEE_STAND, 0);
             break;
      
          case 5:
              // kick phase 2
              // hips forward, kick
             setLeg(LEG0, HIP_FISTS_FORWARD, (mode==SUBMODE_2)?KNEE_DOWN:KNEE_STAND, 0);
             setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_UP, 0);
             break;
      
          case 6:
             // hips forward, kick
             setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_UP, 0);
             setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_DOWN, 0);
             break;
      
          case 7:
              // kick phase 2
              // hips forward, kick
             setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_DOWN, 0);
             setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_UP, 0);
             break;
        }
      }
  }

}

void gait_tripod(int reverse, int hipforward, int hipbackward, 
          int kneeup, int kneedown, long timeperiod) {

    // this version makes leanangle zero
    gait_tripod(reverse, hipforward, hipbackward, 
          kneeup, kneedown, timeperiod, 0);      
}

void gait_tripod(int reverse, int hipforward, int hipbackward, 
          int kneeup, int kneedown, long timeperiod, int leanangle) {

  // the gait consists of 6 phases. This code determines what phase
  // we are currently in by using the millis clock modulo the 
  // desired time period that all six  phases should consume.
  // Right now each phase is an equal amount of time but this may not be optimal

  if (reverse) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }

#define NUM_TRIPOD_PHASES 6
#define FBSHIFT    15   // shift front legs back, back legs forward, this much
  
  long t = hexmillis()%timeperiod;
  long phase = (NUM_TRIPOD_PHASES*t)/timeperiod;

  //Serial.print("PHASE: ");
  //Serial.println(phase);

transactServos(); // defer leg motions until after checking for crashes
  switch (phase) {
    case 0:
      // in this phase, center-left and noncenter-right legs raise up at
      // the knee
      setLeg(TRIPOD1_LEGS, NOMOVE, kneeup, 0, 0, leanangle);
      break;

    case 1:
      // in this phase, the center-left and noncenter-right legs move forward
      // at the hips, while the rest of the legs move backward at the hip
      setLeg(TRIPOD1_LEGS, hipforward, NOMOVE, FBSHIFT);
      setLeg(TRIPOD2_LEGS, hipbackward, NOMOVE, FBSHIFT);
      break;

    case 2: 
      // now put the first set of legs back down on the ground
      setLeg(TRIPOD1_LEGS, NOMOVE, kneedown, 0, 0, leanangle);
      break;

    case 3:
      // lift up the other set of legs at the knee
      setLeg(TRIPOD2_LEGS, NOMOVE, kneeup, 0, 0, leanangle);
      break;
      
    case 4:
      // similar to phase 1, move raised legs forward and lowered legs backward
      setLeg(TRIPOD1_LEGS, hipbackward, NOMOVE, FBSHIFT);
      setLeg(TRIPOD2_LEGS, hipforward, NOMOVE, FBSHIFT);
      break;

    case 5:
      // put the second set of legs down, and the cycle repeats
      setLeg(TRIPOD2_LEGS, NOMOVE, kneedown, 0, 0, leanangle);
      break;  
  }
  commitServos(); // implement all leg motions
}

int ScamperPhase = 0;
unsigned long NextScamperPhaseTime = 0;

long ScamperTracker = 0;

void gait_tripod_scamper(int reverse, int turn) {

  ScamperTracker += 2;  // for tracking if the user is over-doing it with scamper

  // this is a tripod gait that tries to go as fast as possible by not waiting
  // for knee motions to complete before beginning the next hip motion

  // this was experimentally determined and assumes the battery is maintaining
  // +5v to the servos and they are MG90S or equivalent speed. There is very
  // little room left for slower servo motion. If the battery voltage drops below
  // 6.5V then the BEC may not be able to maintain 5.0V to the servos and they may
  // not complete motions fast enough for this to work.

  int hipforward, hipbackward;
  
  if (reverse) {
    hipforward = HIP_BACKWARD;
    hipbackward = HIP_FORWARD;
  } else {
    hipforward = HIP_FORWARD;
    hipbackward = HIP_BACKWARD;
  }


#define FBSHIFT    15   // shift front legs back, back legs forward, this much
#define SCAMPERPHASES 6

#ifdef HEXAPOD
#define KNEEDELAY 35
#define HIPDELAY 100
#endif

#ifdef MEGAPOD
#define KNEEDELAY 45
#define HIPDELAY 120
#endif

#ifdef GIGAPOD
#define KNEEDELAY 60
#define HIPDELAY 170
#endif

  if (millis() >= NextScamperPhaseTime) {
    ScamperPhase++;
    if (ScamperPhase >= SCAMPERPHASES) {
      ScamperPhase = 0;
    }
    switch (ScamperPhase) {
      case 0: NextScamperPhaseTime = millis()+KNEEDELAY; break;
      case 1: NextScamperPhaseTime = millis()+HIPDELAY; break;
      case 2: NextScamperPhaseTime = millis()+KNEEDELAY; break;
      case 3: NextScamperPhaseTime = millis()+KNEEDELAY; break;
      case 4: NextScamperPhaseTime = millis()+HIPDELAY; break;
      case 5: NextScamperPhaseTime = millis()+KNEEDELAY; break;
    }

  }


void setup() {
}
void loop() {
}


