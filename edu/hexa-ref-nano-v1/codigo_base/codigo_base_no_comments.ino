#include <HID.h>
const char *Version = "#RV3r1c"; 
bool beepOnServoReset = true;
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <EEPROM.h>
int FreqMult = 1;   
byte SomeLegsUp = 0;  
#define HEXSIZE 0 
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
#define SERVO_IIC_ADDR  (0x49)    
Adafruit_PWMServoDriver servoDriver = Adafruit_PWMServoDriver(SERVO_IIC_ADDR);
#define BeeperPin 4           
#define ServoTypePin 5        
#define ServoTypeGroundPin 6  
#define GripElbowCurrentPin A6  
#define GripClawCurrentPin  A7  
#define BF_ERROR  100         
#define BD_MED    50          
#define PWMFREQUENCY (60*FreqMult)
#define SERVOMIN  (190*FreqMult) 
#define SERVOMAX  (540*FreqMult) 
#define NUM_LEGS 6
#define NUM_ACTIVE_SERVO (2*NUM_LEGS+NUM_GRIPSERVOS)
#define MAX_SERVO (2*NUM_LEGS+MAX_GRIPSERVOS)
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
#define HIPSWING 25      
#define HIPSMALLSWING 10  
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
#define NOMOVE (-1)   
#define LEFT_START 3  
#define RIGHT_START 0 
#define KNEE_OFFSET 6 
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
#define MODE_LEG    'L'       
#define MODE_GAIT   'G'       
#define MODE_TRIM   'T'       
#define SUBMODE_1 '1'
#define SUBMODE_2 '2'
#define SUBMODE_3 '3'
#define SUBMODE_4 '4'
#define BATTERYSAVER 5000   
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
unsigned short ServoPos[MAX_SERVO]; 
unsigned short ServoTarget[MAX_SERVO];
long ServoTime[MAX_SERVO]; 
byte ServoTrim[MAX_SERVO];  
long startedStanding = 0;   
long LastReceiveTime = 0;   
unsigned long LastValidReceiveTime = 0;  
byte HC05_pad = 0;  
#define DIALMODE_STAND 0
#define DIALMODE_ADJUST 1
#define DIALMODE_TEST 2
#define DIALMODE_DEMO 3
#define DIALMODE_RC_GRIPARM 4
#define DIALMODE_RC 5
int Dialmode;   
#define NUM_GRIPSERVOS ((Dialmode == DIALMODE_RC_GRIPARM)?2:0)  
unsigned long hexmillis() {  
  unsigned long m = (millis() * TIMEFACTOR) / 10L;
  return m;
}
void beep(int f, int t) {
  if (f > 0 && t > 0) {
    tone(BeeperPin, f, t);
  } else {
    noTone(BeeperPin);
  }
}
void beep(int f) {  
  beep(f, 250);
}
byte TrimInEffect = 1;
byte TrimCurLeg = 0;
byte TrimPose = 0;
#define TRIM_ZERO 127   
void save_trims() {
  Serial.print("SAVE TRIMS:");
  for (int i = 0; i < NUM_LEGS * 2; i++) {
    EEPROM.update(i + 1, ServoTrim[i]);
    Serial.print(ServoTrim[i]); Serial.print(" ");
  }
  Serial.println("");
  EEPROM.update(0, 'V');
}
void erase_trims() {
  Serial.println("ERASE TRIMS");
  for (int i = 0; i < NUM_LEGS * 2; i++) {
    ServoTrim[i] = TRIM_ZERO;
  }
}
void setLeg(int legmask, int hip_pos, int knee_pos, int adj) {
  setLeg(legmask, hip_pos, knee_pos, adj, 0, 0);  
}
void setLeg(int legmask, int hip_pos, int knee_pos, int adj, int raw) {
  setLeg(legmask, hip_pos, knee_pos, adj, raw, 0);
}
void setLeg(int legmask, int hip_pos, int knee_pos, int adj, int raw, int leanangle) {
  for (int i = 0; i < NUM_LEGS; i++) {
    if (legmask & 0b1) {  
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
              pos += abs(leanangle / 2);
              break;
            case 2: case 8: case 3: case 9:
              if (leanangle > 0) pos += leanangle;
              break;
          }
        }
        setKnee(i, pos);
      }
    }
    legmask = (legmask >> 1); 
  }
}
void setHipRaw(int leg, int pos) {
  setServo(leg, pos);
}
void setHip(int leg, int pos) {
  if (leg >= LEFT_START) {
    pos = 180 - pos;
  }
  setHipRaw(leg, pos);
}
void setHip(int leg, int pos, int adj) {
  if (ISFRONTLEG(leg)) {
    pos -= adj;
  } else if (ISBACKLEG(leg)) {
    pos += adj;
  }
  if (leg >= LEFT_START) {
    pos = 180 - pos;
  }
  setHipRaw(leg, pos);
}
void setHipRawAdj(int leg, int pos, int adj) {
  if (leg == 5 || leg == 2) {
    pos += adj;
  } else if (leg == 0 || leg == 3) {
    pos -= adj;
  }
  setHipRaw(leg, pos);
}
void setKnee(int leg, int pos) {
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
  if (ccw) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }
#define NUM_TURN_PHASES 6
#define FBSHIFT_TURN    40   
  long t = hexmillis() % timeperiod;
  long phase = (NUM_TURN_PHASES * t) / timeperiod;
  switch (phase) {
    case 0:
      setLeg(TRIPOD1_LEGS, NOMOVE, kneeup, 0);
      break;
    case 1:
      setLeg(TRIPOD1_LEGS, hipforward, NOMOVE, FBSHIFT_TURN, 1);
      setLeg(TRIPOD2_LEGS, hipbackward, NOMOVE, FBSHIFT_TURN, 1);
      break;
    case 2:
      setLeg(TRIPOD1_LEGS, NOMOVE, kneedown, 0);
      break;
    case 3:
      setLeg(TRIPOD2_LEGS, NOMOVE, kneeup, 0);
      break;
    case 4:
      setLeg(TRIPOD1_LEGS, hipbackward, NOMOVE, FBSHIFT_TURN, 1);
      setLeg(TRIPOD2_LEGS, hipforward, NOMOVE, FBSHIFT_TURN, 1);
      break;
    case 5:
      setLeg(TRIPOD2_LEGS, NOMOVE, kneedown, 0);
      break;
  }
}
void stand() {
  transactServos();
  setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_STAND, 0);
  commitServos();
}
void stand_90_degrees() {  
  transactServos();
  setLeg(ALL_LEGS, 90, 90, 0);
  setGrip(90, 90);
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
  long t = hexmillis() % WAVE_CYCLE_TIME;
  long phase = (NUM_WAVE_PHASES * t) / WAVE_CYCLE_TIME;
  if (dpad == 'b') {
    phase = 11 - phase; 
  }
  switch (dpad) {
    case 'f':
    case 'b':
      setLeg(ALL_LEGS, HIP_NEUTRAL, NOMOVE, 0); 
      if (phase < NUM_LEGS) {
        setKnee(phase, KNEE_WAVE);
      } else {
        setKnee(phase - NUM_LEGS, KNEE_STAND);
      }
      break;
    case 'l':
      if (phase < NUM_WAVE_PHASES / 2) {
        setKnee(0, KNEE_TIPTOES);
        setKnee(5, KNEE_STAND);
        setHipRaw(0, HIP_FORWARD);
        setHipRaw(5, HIP_BACKWARD - 40);
        setKnee(2, KNEE_TIPTOES);
        setKnee(3, KNEE_STAND);
        setHipRaw(2, HIP_BACKWARD);
        setHipRaw(3, HIP_FORWARD + 40);
        setLeg(LEG1, HIP_NEUTRAL, KNEE_TIPTOES, 0);
        setLeg(LEG4, HIP_NEUTRAL, KNEE_NEUTRAL, 0);
      } else {
        setKnee(0, KNEE_STAND);
        setKnee(5, KNEE_TIPTOES);
        setHipRaw(0, HIP_FORWARD + 40);
        setHipRaw(5, HIP_BACKWARD);
        setKnee(2, KNEE_STAND);
        setKnee(3, KNEE_TIPTOES);
        setHipRaw(2, HIP_BACKWARD - 40);
        setHipRaw(3, HIP_FORWARD);
        setLeg(LEG1, HIP_NEUTRAL, KNEE_NEUTRAL, 0);
        setLeg(LEG4, HIP_NEUTRAL, KNEE_TIPTOES, 0);
      }
      break;
    case 'r':
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
      setLeg(ALL_LEGS, HIP_NEUTRAL, NOMOVE, 0);
      int p = phase / 2;
      for (int i = 0; i < NUM_LEGS; i++) {
        if (i == p) {
          setKnee(i, KNEE_UP_MAX);
        } else {
          setKnee(i, KNEE_NEUTRAL);
        }
      }
      return;
      if (phase < NUM_LEGS) {
        setKnee(phase / 2, KNEE_UP);
      } else {
        int p = phase - NUM_LEGS;
        if (p < 0) p += NUM_LEGS;
        setKnee(p / 2, KNEE_NEUTRAL + 10);
      }
      break;
  }
}
void griparm_mode(char dpad) {
  Serial.println();
  switch (dpad) {
    case 's':
      for (int i = 0; i < NUM_ACTIVE_SERVO; i++) {
        ServoTarget[i] = ServoPos[i];
      }
      break;
    case 'w':  
      ServoTarget[GRIPARM_ELBOW_SERVO] = GRIPARM_ELBOW_DEFAULT;
      ServoTarget[GRIPARM_CLAW_SERVO] = GRIPARM_CLAW_DEFAULT;
      break;
    case 'f': 
      ServoTarget[GRIPARM_ELBOW_SERVO] = GRIPARM_ELBOW_MIN;
      break;
    case 'b': 
      ServoTarget[GRIPARM_ELBOW_SERVO] = GRIPARM_ELBOW_MAX;
      break;
    case 'l':  
      ServoTarget[GRIPARM_CLAW_SERVO] = GRIPARM_CLAW_MAX;
      break;
    case 'r': 
      ServoTarget[GRIPARM_CLAW_SERVO] = GRIPARM_CLAW_MIN;
      break;
  }
}
void fight_mode(char dpad, int mode, long timeperiod) {
#define HIP_FISTS_FORWARD 130
  if (Dialmode == DIALMODE_RC_GRIPARM && mode == SUBMODE_2) {
    griparm_mode(dpad);
    return;
  }
  if (mode == SUBMODE_3) {
    switch (dpad) {
      case 's':
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i + KNEE_OFFSET] = ServoPos[i + NUM_LEGS];
          ServoTarget[i] = ServoPos[i];
        }
        break;
      case 'w':  
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
          ServoTarget[i] = 90;
        }
        break;
      case 'f': 
        ServoTarget[5] = ServoTarget[4] = ServoTarget[3] = 125;
        ServoTarget[0] = ServoTarget[1] = ServoTarget[2] = 55;
        break;
      case 'b': 
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
    switch (dpad) {
      case 's':
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i + KNEE_OFFSET] = ServoPos[i + NUM_LEGS];
          ServoTarget[i] = ServoPos[i];
        }
        break;
      case 'w':  
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
          ServoTarget[i] = 90;
        }
        break;
      case 'f': 
        if (ServoPos[8] == KNEE_STAND) { 
          ServoTarget[6] = ServoTarget[11] = KNEE_CROUCH;
          ServoTarget[7] = ServoTarget[10] = KNEE_HALF_CROUCH;
          ServoTarget[8] = ServoTarget[9] = KNEE_STAND;
        } else { 
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
          }
        }
        break;
      case 'b': 
        if (ServoPos[6] == KNEE_STAND) { 
          ServoTarget[6] = ServoTarget[11] = KNEE_STAND;
          ServoTarget[7] = ServoTarget[10] = KNEE_HALF_CROUCH;
          ServoTarget[8] = ServoTarget[9] = KNEE_CROUCH;
        } else { 
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
          }
        }
        break;
      case 'l':
        if (ServoPos[9] == KNEE_STAND) {
          ServoTarget[6] = ServoTarget[8] = KNEE_HALF_CROUCH;
          ServoTarget[7] = KNEE_CROUCH;
          ServoTarget[9] = ServoTarget[10] = ServoTarget[11] = KNEE_STAND;
        } else {
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
          }
        }
        break;
      case 'r':
        if (ServoPos[6] == KNEE_STAND) {
          ServoTarget[6] = ServoTarget[7] = ServoTarget[8] = KNEE_STAND;
          ServoTarget[9] = ServoTarget[11] = KNEE_HALF_CROUCH;
          ServoTarget[10] = KNEE_CROUCH;
        } else {
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
          }
        }
        break;
    }
  }
  if (mode >= SUBMODE_3) {
    return; 
  }
  setLeg(MIDDLE_LEGS, HIP_FORWARD + 10, KNEE_STAND, 0);
  setLeg(BACK_LEGS, HIP_BACKWARD, KNEE_STAND, 0);
  switch (dpad) {
    case 's':  
      setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, KNEE_NEUTRAL, 0);
      break;
    case 'f':  
      setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, KNEE_UP, 0);
      break;
    case 'b':  
      setLeg(FRONT_LEGS, HIP_FORWARD, KNEE_STAND, 0);
      break;
    case 'l':  
      if (mode == SUBMODE_1) {
        setLeg(LEG0, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_RELAX, 0);
      } else {
        setLeg(LEG0, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG5, HIP_FISTS_FORWARD + 30, KNEE_RELAX, 0);
      }
      break;
    case 'r':  
      if (mode == SUBMODE_1) {
        setLeg(LEG5, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_RELAX, 0);
      } else { 
        setLeg(LEG5, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG0, HIP_FISTS_FORWARD + 30, KNEE_RELAX, 0);
      }
      break;
    case 'w':  
#define NUM_PUGIL_PHASES 8
      { 
        long t = hexmillis() % timeperiod;
        long phase = (NUM_PUGIL_PHASES * t) / timeperiod;
        switch (phase) {
          case 0:
            setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, (mode == SUBMODE_2) ? KNEE_DOWN : KNEE_RELAX, 0);
            break;
          case 1:
            setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, KNEE_UP, 0);
            break;
          case 2:
            setLeg(FRONT_LEGS, HIP_BACKWARD, KNEE_NEUTRAL, 0);
            break;
          case 3:
            setLeg(FRONT_LEGS, HIP_BACKWARD, KNEE_UP, 0);
            break;
          case 4:
            setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_UP, 0);
            setLeg(LEG5, HIP_FISTS_FORWARD, (mode == SUBMODE_2) ? KNEE_DOWN : KNEE_STAND, 0);
            break;
          case 5:
            setLeg(LEG0, HIP_FISTS_FORWARD, (mode == SUBMODE_2) ? KNEE_DOWN : KNEE_STAND, 0);
            setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_UP, 0);
            break;
          case 6:
            setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_UP, 0);
            setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_DOWN, 0);
            break;
          case 7:
            setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_DOWN, 0);
            setLeg(LEG5, HIP_FISTS_FORWARD, KNEE_UP, 0);
            break;
        }
      }
  }
}
void gait_tripod(int reverse, int hipforward, int hipbackward,
                 int kneeup, int kneedown, long timeperiod) {
  gait_tripod(reverse, hipforward, hipbackward,
              kneeup, kneedown, timeperiod, 0);
}
void gait_tripod(int reverse, int hipforward, int hipbackward,
                 int kneeup, int kneedown, long timeperiod, int leanangle) {
  if (reverse) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }
#define NUM_TRIPOD_PHASES 6
#define FBSHIFT    15   
  long t = hexmillis() % timeperiod;
  long phase = (NUM_TRIPOD_PHASES * t) / timeperiod;
  transactServos(); 
  switch (phase) {
    case 0:
      setLeg(TRIPOD1_LEGS, NOMOVE, kneeup, 0, 0, leanangle);
      break;
    case 1:
      setLeg(TRIPOD1_LEGS, hipforward, NOMOVE, FBSHIFT);
      setLeg(TRIPOD2_LEGS, hipbackward, NOMOVE, FBSHIFT);
      break;
    case 2:
      setLeg(TRIPOD1_LEGS, NOMOVE, kneedown, 0, 0, leanangle);
      break;
    case 3:
      setLeg(TRIPOD2_LEGS, NOMOVE, kneeup, 0, 0, leanangle);
      break;
    case 4:
      setLeg(TRIPOD1_LEGS, hipbackward, NOMOVE, FBSHIFT);
      setLeg(TRIPOD2_LEGS, hipforward, NOMOVE, FBSHIFT);
      break;
    case 5:
      setLeg(TRIPOD2_LEGS, NOMOVE, kneedown, 0, 0, leanangle);
      break;
  }
  commitServos(); 
}
int ScamperPhase = 0;
unsigned long NextScamperPhaseTime = 0;
long ScamperTracker = 0;
void gait_tripod_scamper(int reverse, int turn) {
  ScamperTracker += 2;  
  int hipforward, hipbackward;
  if (reverse) {
    hipforward = HIP_BACKWARD;
    hipbackward = HIP_FORWARD;
  } else {
    hipforward = HIP_FORWARD;
    hipbackward = HIP_BACKWARD;
  }
#define FBSHIFT    15   
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
      case 0: NextScamperPhaseTime = millis() + KNEEDELAY; break;
      case 1: NextScamperPhaseTime = millis() + HIPDELAY; break;
      case 2: NextScamperPhaseTime = millis() + KNEEDELAY; break;
      case 3: NextScamperPhaseTime = millis() + KNEEDELAY; break;
      case 4: NextScamperPhaseTime = millis() + HIPDELAY; break;
      case 5: NextScamperPhaseTime = millis() + KNEEDELAY; break;
    }
  }
  transactServos();
  switch (ScamperPhase) {
    case 0:
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_SCAMPER, 0);
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_DOWN, 0);
      break;
    case 1:
      setLeg(TRIPOD1_LEGS, hipforward, NOMOVE, FBSHIFT, turn);
      setLeg(TRIPOD2_LEGS, hipbackward, NOMOVE, FBSHIFT, turn);
      break;
    case 2:
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_DOWN, 0);
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_DOWN, 0);
      break;
    case 3:
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_SCAMPER, 0, turn);
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_DOWN, 0, turn);
      break;
    case 4:
      setLeg(TRIPOD1_LEGS, hipbackward, NOMOVE, FBSHIFT, turn);
      setLeg(TRIPOD2_LEGS, hipforward, NOMOVE, FBSHIFT, turn);
      break;
    case 5:
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_DOWN, 0);
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_DOWN, 0);
      break;
  }
  commitServos();
}
void gait_ripple(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod) {
  gait_ripple(turn, reverse, hipforward, hipbackward, kneeup, kneedown, timeperiod, 0);
}
void gait_ripple(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle) {
  if (turn) {
    reverse = 1 - reverse; 
  }
  if (reverse) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }
#define NUM_RIPPLE_PHASES 19
  long t = hexmillis() % timeperiod;
  long phase = (NUM_RIPPLE_PHASES * t) / timeperiod;
  transactServos();
  if (phase == 18) {
    setLeg(ALL_LEGS, hipbackward, NOMOVE, FBSHIFT, turn);
  } else {
    int leg = phase / 3; 
    leg = 1 << leg;
    int subphase = phase % 3;
    switch (subphase) {
      case 0:
        setLeg(leg, NOMOVE, kneeup, 0);
        break;
      case 1:
        setLeg(leg, hipforward, NOMOVE, FBSHIFT, turn);  
        break;
      case 2:
        setLeg(leg, NOMOVE, kneedown, 0);
        break;
    }
  }
  commitServos();
}
#define FBSHIFT_QUAD 25
#define HIP_FORWARD_QUAD (HIP_FORWARD)
#define HIP_BACKWARD_QUAD (HIP_BACKWARD)
#define KNEE_QUAD_UP (KNEE_DOWN+30)
#define KNEE_QUAD_DOWN (KNEE_DOWN)
#define QUAD_CYCLE_TIME 600
void gait_quad(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle) {
  if (turn) {
    reverse = 1 - reverse; 
  }
  if (reverse) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }
#define NUM_QUAD_PHASES 6
  long t = hexmillis() % timeperiod;
  long phase = (NUM_QUAD_PHASES * t) / timeperiod;
  transactServos();
  setLeg(MIDDLE_LEGS, HIP_NEUTRAL, KNEE_UP_MAX, FBSHIFT_QUAD, 0);
  switch (phase) {
    case 0:
      setLeg(QUAD1_LEGS, NOMOVE, kneeup, FBSHIFT_QUAD, turn);
      if (kneeup != kneedown) { 
        setLeg(MIDDLE_LEGS, reverse ? HIP_BACKWARD_MAX : HIP_FORWARD_MAX, NOMOVE, 0, 1);
      }
      break;
    case 1:
      setLeg(QUAD1_LEGS, hipforward, NOMOVE, FBSHIFT_QUAD, turn);
      setLeg(QUAD2_LEGS, hipbackward, NOMOVE, FBSHIFT_QUAD, turn);
      break;
    case 2:
      setLeg(QUAD1_LEGS, NOMOVE, kneedown, 0, turn);
      break;
    case 3:
      setLeg(QUAD2_LEGS, NOMOVE, kneeup, 0, turn);
      if (kneeup != kneedown) {
        setLeg(MIDDLE_LEGS, reverse ? HIP_FORWARD_MAX : HIP_BACKWARD_MAX, NOMOVE, 0, 1);
      }
      break;
    case 4:
      setLeg(QUAD1_LEGS, hipbackward, NOMOVE, FBSHIFT_QUAD, turn);
      setLeg(QUAD2_LEGS, hipforward, NOMOVE, FBSHIFT_QUAD, turn);
      break;
    case 5:
      setLeg(QUAD2_LEGS, NOMOVE, kneedown, 0, turn);
      break;
  }
  commitServos();
}
#define FBSHIFT_BELLY 55
#define HIP_FORWARD_BELLY (HIP_FORWARD+10)
#define HIP_BACKWARD_BELLY (HIP_BACKWARD-10)
#define KNEE_BELLY_UP (KNEE_NEUTRAL-30)
#define KNEE_BELLY_DOWN (KNEE_NEUTRAL+30)
#define BELLY_CYCLE_TIME 600
void gait_belly(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle) {
  if (turn) {
    reverse = 1 - reverse; 
  }
  if (reverse) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }
#define NUM_BELLY_PHASES 4
  long t = hexmillis() % timeperiod;
  long phase = (NUM_BELLY_PHASES * t) / timeperiod;
  transactServos();
  switch (phase) {
    case 0: 
      setLeg(ALL_LEGS, NOMOVE, kneeup, FBSHIFT_BELLY, turn);
      break;
    case 1:
      if (turn) {
        for (int i = 0; i < NUM_LEGS; i++) {
          setHipRawAdj(i, hipbackward, FBSHIFT_BELLY);
        }
      } else {
        setLeg(ALL_LEGS, hipbackward, NOMOVE, FBSHIFT_BELLY, turn);
      }
      break;
    case 2:
      setLeg(ALL_LEGS, NOMOVE, kneedown, FBSHIFT_BELLY, turn);
      break;
    case 3:
      if (turn) {
        for (int i = 0; i < NUM_LEGS; i++) {
          setHipRawAdj(i, hipforward, FBSHIFT_BELLY);
        }
      } else {
        setLeg(ALL_LEGS, hipforward, NOMOVE, FBSHIFT_BELLY, turn);
      }
      break;
  }
  commitServos();
}
#define G_STAND 0
#define G_TURN  1
#define G_TRIPOD 2
#define G_SCAMPER 3
#define G_DANCE 4
#define G_BOOGIE 5
#define G_FIGHT 6
#define G_TEETER 7
#define G_BALLET 8
#define G_NUMGATES 9
int curGait = G_STAND;
int curReverse = 0;
unsigned long nextGaitTime = 0;
void random_gait(int timingfactor) {
#define GATETIME 3500  
  if (millis() > nextGaitTime) {
    curGait++;
    if (curGait >= G_NUMGATES) {
      curGait = 0;
    }
    nextGaitTime = millis() + GATETIME;
    setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_STAND, 0);
    delay(600);
  }
  switch (curGait) {
    case G_STAND:
      stand();
      break;
    case G_TURN:
      turn(1, HIP_FORWARD, HIP_BACKWARD, KNEE_NEUTRAL, KNEE_DOWN, TRIPOD_CYCLE_TIME); 
      break;
    case G_TRIPOD:
      gait_tripod(1, HIP_FORWARD, HIP_BACKWARD, KNEE_NEUTRAL, KNEE_DOWN, TRIPOD_CYCLE_TIME); 
      break;
    case G_SCAMPER:
      gait_tripod_scamper((nextGaitTime - (millis()) < GATETIME / 2), 0); 
      break;
    case G_DANCE:
      stand();
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 145);
      delay(350);
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 35);
      delay(350);
      break;
    case G_BOOGIE:
      boogie_woogie(NO_LEGS, SUBMODE_1, 2);
      break;
    case G_FIGHT:
      fight_mode('w', SUBMODE_1, FIGHT_CYCLE_TIME);
      break;
    case G_TEETER:
      wave('r');
      break;
    case G_BALLET:
      flutter();
      break;
  }
}
void foldup() {
  setLeg(ALL_LEGS, NOMOVE, KNEE_FOLD, 0);
  for (int i = 0; i < NUM_LEGS; i++)
    setHipRaw(i, HIP_FOLD);
}
void dance_dab(int timingfactor) {
#define NUM_DAB_PHASES 3
  long t = hexmillis() % (1100 * timingfactor);
  long phase = (NUM_DAB_PHASES * t) / (1100 * timingfactor);
  switch (phase) {
    case 0:
      stand(); break;
    case 1:
      setKnee(6, KNEE_UP); break;
    case 2:
      for (int i = 0; i < NUM_LEGS; i++)
        if (i != 0) setHipRaw(i, 40);
      setHipRaw(0, 140);
      break;
  }
}
void flutter() {   
#define NUM_FLUTTER_PHASES 4
#define FLUTTER_TIME 200
#define KNEE_FLUTTER (KNEE_TIPTOES+20)
  long t = hexmillis() % (FLUTTER_TIME);
  long phase = (NUM_FLUTTER_PHASES * t) / (FLUTTER_TIME);
  setLeg(ALL_LEGS, HIP_NEUTRAL, NOMOVE, 0, 0);
  switch (phase) {
    case 0:
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_FLUTTER, 0, 0);
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      break;
    case 1:
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      break;
    case 2:
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_FLUTTER, 0, 0);
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      break;
    case 3:
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_TIPTOES, 0, 0);
      break;
  }
}
void dance_ballet(int dpad) {   
#define BALLET_TIME 250
  switch (dpad) {
    default:
    case 's': tiptoes(); return;
    case 'w': flutter(); return;
    case 'l':
      turn(1, HIP_FORWARD_SMALL, HIP_BACKWARD_SMALL, KNEE_FLUTTER, KNEE_TIPTOES, BALLET_TIME);
      break;
    case 'r':
      turn(0, HIP_FORWARD_SMALL, HIP_BACKWARD_SMALL, KNEE_FLUTTER, KNEE_TIPTOES, BALLET_TIME);
      break;
    case 'f':
      gait_tripod(0, HIP_FORWARD_SMALL, HIP_BACKWARD_SMALL, KNEE_FLUTTER, KNEE_TIPTOES, BALLET_TIME);
      break;
    case 'b':
      gait_tripod(1, HIP_FORWARD_SMALL, HIP_BACKWARD_SMALL, KNEE_FLUTTER, KNEE_TIPTOES, BALLET_TIME);
      break;
  }
}
void dance_hands(int dpad) {
  setLeg(FRONT_LEGS, HIP_NEUTRAL, KNEE_STAND, 0, 0);
  setLeg(BACK_LEGS, HIP_NEUTRAL, KNEE_STAND, 0, 0);
  switch (dpad) {
    case 's':
      setLeg(MIDDLE_LEGS, HIP_NEUTRAL, KNEE_UP, 0, 0);
      break;
    case 'f':
      setLeg(MIDDLE_LEGS, HIP_FORWARD_MAX, KNEE_UP_MAX, 0, 0);
      break;
    case 'b':
      setLeg(MIDDLE_LEGS, HIP_BACKWARD_MAX, KNEE_UP_MAX, 0, 0);
      break;
    case 'l':
      setLeg(MIDDLE_LEGS, HIP_NEUTRAL, NOMOVE, 0, 0);
      setLeg(LEG1, NOMOVE, KNEE_NEUTRAL, 0, 0);
      setLeg(LEG4, NOMOVE, KNEE_UP_MAX, 0, 0);
      break;
    case 'r':
      setLeg(MIDDLE_LEGS, HIP_NEUTRAL, NOMOVE, 0, 0);
      setLeg(LEG1, NOMOVE, KNEE_UP_MAX, 0, 0);
      setLeg(LEG4, NOMOVE, KNEE_NEUTRAL, 0, 0);
      break;
    case 'w':
#define NUM_HANDS_PHASES 2
#define HANDS_TIME_PERIOD 400
      { 
        long t = hexmillis() % HANDS_TIME_PERIOD;
        long phase = (NUM_HANDS_PHASES * t) / HANDS_TIME_PERIOD;
        switch (phase) {
          case 0:
            setLeg(MIDDLE_LEGS, HIP_NEUTRAL, NOMOVE, 0, 0);
            setLeg(LEG1, NOMOVE, KNEE_NEUTRAL, 0, 0);
            setLeg(LEG4, NOMOVE, KNEE_UP_MAX, 0, 0);
            break;
          case 1:
            setLeg(MIDDLE_LEGS, HIP_NEUTRAL, NOMOVE, 0, 0);
            setLeg(LEG1, NOMOVE, KNEE_UP_MAX, 0, 0);
            setLeg(LEG4, NOMOVE, KNEE_NEUTRAL, 0, 0);
            break;
        }
      }
      break;
  }
}
void dance(int legs_up, int submode, int timingfactor) {
  setLeg(legs_up, NOMOVE, KNEE_UP, 0, 0);
  setLeg((legs_up ^ 0b111111), NOMOVE, ((submode == SUBMODE_1) ? KNEE_STAND : KNEE_TIPTOES), 0, 0);
#define NUM_DANCE_PHASES 2
  long t = hexmillis() % (600 * timingfactor);
  long phase = (NUM_DANCE_PHASES * t) / (600 * timingfactor);
  switch (phase) {
    case 0:
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 140);
      break;
    case 1:
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 40);
      break;
  }
}
void boogie_woogie(int legs_flat, int submode, int timingfactor) {
  setLeg(ALL_LEGS, NOMOVE, KNEE_UP, 0);
#define NUM_BOOGIE_PHASES 2
  long t = hexmillis() % (400 * timingfactor);
  long phase = (NUM_BOOGIE_PHASES * t) / (400 * timingfactor);
  switch (phase) {
    case 0:
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 140);
      break;
    case 1:
      for (int i = 0; i < NUM_LEGS; i++)
        setHipRaw(i, 40);
      break;
  }
}
SoftwareSerial BlueTooth(3, 2); 
int ServosDetached = 0;
void attach_all_servos() {
  Serial.print("ATTACH");
  for (int i = 0; i < 2 * NUM_LEGS + NUM_GRIPSERVOS; i++) {
    setServo(i, ServoPos[i]);
    Serial.print(ServoPos[i]); Serial.print(":");
  }
  Serial.println("");
  ServosDetached = 0;
  return;
}
void detach_all_servos() {
  for (int i = 0; i < 16; i++) {
    servoDriver.setPin(i, 0, false); 
  }
  ServosDetached = 1;
}
void resetServoDriver() {
  servoDriver.begin();
  servoDriver.setPWMFreq(PWMFREQUENCY);  
}
void setup() {
  Serial.begin(9600);
  Serial.println("");
  Serial.println(Version);
  pinMode(BeeperPin, OUTPUT);
  beep(200);
  if (EEPROM.read(0) == 'V') {
    Serial.print("TRIMS: ");
    for (int i = 0; i < NUM_LEGS * 2; i++) {
      ServoTrim[i] = EEPROM.read(i + 1);
      Serial.print(ServoTrim[i] - TRIM_ZERO); Serial.print(" ");
    }
    Serial.println("");
  } else {
    Serial.println("TRIMS:unset");
    for (int i = 0; i < NUM_LEGS * 2; i++) {
      ServoTrim[i] = TRIM_ZERO;   
    }
  }
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  delay(300);
  digitalWrite(13, LOW);
  delay(150);
  digitalWrite(13, HIGH);
  delay(150);
  digitalWrite(13, LOW);
  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(ServoTypeGroundPin, OUTPUT);    
  digitalWrite(ServoTypeGroundPin, LOW);
  pinMode(ServoTypePin, INPUT_PULLUP);    
  digitalWrite(13, LOW);
  digitalWrite(A1, HIGH);
  digitalWrite(A2, LOW);
  delay(300); 
  BlueTooth.begin(38400);
  BlueTooth.println("");
  delay(250);
  BlueTooth.println(Version);
  delay(250);
  if (digitalRead(ServoTypePin) == LOW) { 
    FreqMult = 3 - FreqMult; 
  }
  for (int i = 0; i < FreqMult; i++) {
    beep(800, 50);
    delay(100);
  }
  resetServoDriver();
  delay(250);
  stand();
  setGrip(90, 90);  
  delay(300);
  beep(400); 
  yield();
}
byte deferServoSet = 0;
int servoOffset = 0; 
void setServo(int servonum, unsigned int position) {
  servonum = constrain(servonum, 0, 15);
  position = constrain(position, 0, 180);
  if (servonum < 12 && servoOffset != 0) { 
    int tmp = ((servonum + servoOffset) % 6);
    if (servonum > 5) { 
      tmp += 6;
    }
    servonum = constrain(tmp, 0, 11);
  }
  if (position != ServoPos[servonum]) {
    ServoTime[servonum] = millis();
  }
  ServoPos[servonum] = position;  
  int p = map(position, 0, 180, SERVOMIN, SERVOMAX);
  if (TrimInEffect && servonum < 12) {
    p += ServoTrim[servonum] - TRIM_ZERO;   
  }
  if (!deferServoSet) {
    servoDriver.setPWM(servonum, 0, p);
  }
}
void transactServos() {
  deferServoSet = 1;
}
void commitServos() {
  checkForCrashingHips();
  deferServoSet = 0;
  int tmp = servoOffset;
  servoOffset = 0;  
  for (int servo = 0; servo < 2 * NUM_LEGS + NUM_GRIPSERVOS; servo++) {
    setServo(servo, ServoPos[servo]);
  }
  servoOffset = tmp; 
}
void checkForCrashingHips() {
  int tmp = servoOffset;
  servoOffset = 0; 
  for (int leg = 0; leg < NUM_LEGS; leg++) {
    if (ServoPos[leg] > 85) {
      continue; 
    }
    int nextleg = ((leg + 1) % NUM_LEGS);
    if (ServoPos[nextleg] < 100) {
      continue;   
    }
    int diff = ServoPos[nextleg] - ServoPos[leg];
    if (diff <= 85) {
      continue;
    }
    int adjust = (diff - 85) / 2 + 1; 
#if 1
    Serial.print("#CRASH:");
    Serial.print(leg); Serial.print("="); Serial.print(ServoPos[leg]);
    Serial.print("/"); Serial.print(nextleg); Serial.print("="); Serial.print(ServoPos[nextleg]);
    Serial.print(" Diff="); Serial.print(diff); Serial.print(" ADJ="); Serial.println(adjust);
#endif
    setServo(leg, ServoPos[leg] + adjust);
    setServo(nextleg, ServoPos[nextleg] - adjust);
  }
  servoOffset = tmp;
}
#define ULTRAOUTPUTPIN 7      
#define ULTRAINPUTPIN  8      
unsigned int readUltrasonic() {  
  pinMode(ULTRAOUTPUTPIN, OUTPUT);
  digitalWrite(ULTRAOUTPUTPIN, LOW);
  delayMicroseconds(5);
  digitalWrite(ULTRAOUTPUTPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRAOUTPUTPIN, LOW);
  unsigned int duration = pulseIn(ULTRAINPUTPIN, HIGH, 18000);  
  if (duration < 100) { 
    return 1000;   
  }
  return (duration) / 58;  
}
unsigned int
bluewriteword(int w) {
  unsigned int h = highByte(w);
  BlueTooth.write(h);
  unsigned int l = lowByte(w);
  BlueTooth.write(l);
  return h + l;
}
void padwrite(int len) {
  if (HC05_pad == 0) {
    return;  
  }
  int zero = 0;
  for (int i = len + 4; i < 230; i++) {
    BlueTooth.write(zero);
  }
}
void
sendSensorData() {
  unsigned int ultra = readUltrasonic(); 
  int blocks = 0; 
  BlueTooth.print("V");
  BlueTooth.print("1");
  int length = 8;  
  unsigned int checksum = length;
  BlueTooth.write(length);
  checksum += bluewriteword(analogRead(A3));
  checksum += bluewriteword(analogRead(A6));
  checksum += bluewriteword(analogRead(A7));
  checksum += bluewriteword(ultra);
  if (blocks > 0) {
  }
  checksum = (checksum % 256);
  BlueTooth.write(checksum); 
  padwrite(length);
  startedStanding = millis(); 
}
#define P_WAITING_FOR_HEADER      0
#define P_WAITING_FOR_VERSION     1
#define P_WAITING_FOR_LENGTH      2
#define P_READING_DATA            3
#define P_WAITING_FOR_CHECKSUM    4
#define P_SIMPLE_WAITING_FOR_DATA 5
int pulselen = SERVOMIN;
#define MAXPACKETDATA 48
unsigned char packetData[MAXPACKETDATA];
unsigned int packetLength = 0;
unsigned int packetLengthReceived = 0;
int packetState = P_WAITING_FOR_HEADER;
void packetErrorChirp(char c) {
  beep(70, 8);
  Serial.print(" BTER:"); Serial.print(packetState); Serial.print(c);
  Serial.print("A"); Serial.println(BlueTooth.available());
  packetState = P_WAITING_FOR_HEADER; 
}
byte lastCmd = 's';
byte priorCmd = 0;
byte mode = MODE_WALK; 
byte submode = SUBMODE_1;     
byte timingfactor = 1;   
short priorDialMode = -1;
long NullCount = 0;
int receiveDataHandler() {
  while (BlueTooth.available() > 0) {
    unsigned int c = BlueTooth.read();
#if 0
    unsigned long m = millis();
    Serial.print("'"); Serial.write(c); Serial.print("' ("); Serial.print((int)c);
    Serial.println("");
#endif
    switch (packetState) {
      case P_WAITING_FOR_HEADER:
        if (c == 'V') {
          packetState = P_WAITING_FOR_VERSION;
        } else if (c == '@') {  
          packetState = P_SIMPLE_WAITING_FOR_DATA;
          packetLengthReceived = 0; 
        } else {
          int flushcount = 0;
          while (BlueTooth.available() > 0 && (BlueTooth.peek() != 'V') && (BlueTooth.peek() != '@')) {
            BlueTooth.read(); 
            flushcount++;
          }
          Serial.print("F:"); Serial.print(flushcount);
          if (c != 0) { 
            packetErrorChirp(c);
          } else {
            NullCount++;
            if (NullCount > 100) {
              HC05_pad = 1;
            }
          }
        }
        break;
      case P_WAITING_FOR_VERSION:
        if (c == '1') {
          packetState = P_WAITING_FOR_LENGTH;
          NullCount = 0; 
        } else if (c == 'V') {
        } else {
          packetErrorChirp(c);
          packetState = P_WAITING_FOR_HEADER; 
        }
        break;
      case P_WAITING_FOR_LENGTH:
        { 
          packetLength = c;
          if (packetLength > MAXPACKETDATA) {
            packetErrorChirp(c);
            Serial.print("Bad Length="); Serial.println(c);
            packetState = P_WAITING_FOR_HEADER;
            return 0;
          }
          packetLengthReceived = 0;
          packetState = P_READING_DATA;
        }
        break;
      case P_READING_DATA:
        if (packetLengthReceived >= MAXPACKETDATA) {
          Serial.println("ERROR: PacketDataLen out of bounds!");
          packetState = P_WAITING_FOR_HEADER;  
          packetLengthReceived = 0;
          return 0;
        }
        packetData[packetLengthReceived++] = c;
        if (packetLengthReceived == packetLength) {
          packetState = P_WAITING_FOR_CHECKSUM;
        }
        break;
      case P_WAITING_FOR_CHECKSUM:
        {
          unsigned int sum = packetLength;  
          for (unsigned int i = 0; i < packetLength; i++) {
            sum += packetData[i];
          }
          sum = (sum % 256);
          if (sum != c) {
            packetErrorChirp(c);
            Serial.print("CHECKSUM FAIL "); Serial.print(sum); Serial.print("!="); Serial.print((int)c);
            Serial.print(" len="); Serial.println(packetLength);
            packetState = P_WAITING_FOR_HEADER;  
          } else {
            LastValidReceiveTime = millis();  
            processPacketData();
            packetState = P_WAITING_FOR_HEADER;
            return 1; 
          }
        }
        break;
      case P_SIMPLE_WAITING_FOR_DATA:
        packetData[packetLengthReceived++] = c;
        if (packetLengthReceived == 3) {
          packetState = P_WAITING_FOR_HEADER;
          if ( (packetData[0] != 'W' && packetData[0] != 'D' && packetData[0] != 'F' && packetData[0] != 'X' && packetData[0] != 'Y' && packetData[0] != 'Z') ||
               (packetData[1] != '1' && packetData[1] != '2' && packetData[1] != '3' && packetData[1] != '4') ||
               (packetData[2] != 'f' && packetData[2] != 'b' && packetData[2] != 'l' && packetData[2] != 'r' &&
                packetData[2] != 'w' && packetData[2] != 's')) {
            return 0;
          } else {
            processPacketData();
            return 1;
          }
        }
        break;
    }
  }
  return 0; 
}
unsigned int LastGgaittype;
unsigned int LastGreverse;
unsigned int LastGhipforward;
unsigned int LastGhipbackward;
unsigned int LastGkneeup;
unsigned int LastGkneedown;
unsigned int LastGtimeperiod;
int LastGleanangle;   
void gait_command(int gaittype, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, int leanangle, int timeperiod) {
  if (ServosDetached) { 
    attach_all_servos();
  }
  switch (gaittype) {
    case 0:
    default:
      gait_tripod(reverse, hipforward, hipbackward, kneeup, kneedown, timeperiod, leanangle);
      break;
    case 1:
      turn(reverse, hipforward, hipbackward, kneeup, kneedown, timeperiod, leanangle);
      break;
    case 2:
      gait_ripple(0, reverse, hipforward, hipbackward, kneeup, kneedown, timeperiod, leanangle);
      break;
    case 3:
      break;
  }
#if 0
  Serial.print("GAIT: style="); Serial.print(gaittype); Serial.print(" dir="); Serial.print(reverse, DEC); Serial.print(" angles="); Serial.print(hipforward);
  Serial.print("/"); Serial.print(hipbackward); Serial.print("/"); Serial.print(kneeup, DEC); Serial.print("/"); Serial.print(kneedown);
  Serial.print("/"); Serial.println(leanangle);
#endif
  mode = MODE_GAIT;   
}
void dumpPacket() { 
  Serial.print("DMP:");
  for (unsigned int i = 0; i < packetLengthReceived; i++) {
    Serial.write(packetData[i]); Serial.print("("); Serial.print(packetData[i]); Serial.print(")");
  }
  Serial.println("");
}
void processPacketData() {
  unsigned int i = 0;
  while (i < packetLengthReceived) {
    switch (packetData[i]) {
      case 'W':
      case 'F':
      case 'D':
      case 'X':
      case 'Y':
      case 'Z':
        if (i <= packetLengthReceived - 3) {
          if ( mode != packetData[i] || submode != packetData[i + 1] ) {
            if ( (packetData[i] != 'Z' && (packetData[i] != 'F')) ||
                 (packetData[i] == 'Z' && mode != 'Z' && mode != 'F') ||
                 (packetData[i] == 'F' && mode != 'Z' && mode != 'F')) {
              stand();
#ifdef GIGAPOD
              delay(250);
#else
              delay(120);
#endif
            }
          }
          mode = packetData[i];
          submode = packetData[i + 1];
          lastCmd = packetData[i + 2];
          i += 3; 
          continue;
        } else {
          beep(BF_ERROR, BD_MED);
          Serial.println("PKERR:M:Short");
          return;  
        }
        break;
      case 'B':   
        if (i <= packetLengthReceived - 5) {
          int honkfreq = word(packetData[i + 1], packetData[i + 2]);
          int honkdur = word(packetData[i + 3], packetData[i + 4]);
          if (honkfreq > 0 && honkdur > 0) {
            Serial.println("#Beep");
            beep(honkfreq, honkdur);
          }
          i += 5; 
        } else {
          beep(BF_ERROR, BD_MED);
          Serial.print("PKERR:B:Short:"); Serial.print(i); Serial.print(":"); Serial.println(packetLengthReceived);
          return;  
        }
        break;
      case 'R': 
#define RAWSERVOPOS 0
#define RAWSERVOADD 1
#define RAWSERVOSUB 2
#define RAWSERVONOMOVE 255
#define RAWSERVODETACH 254
        if (i <= packetLengthReceived - 18) {
          int movetype = packetData[i + 1];
          for (int servo = 0; servo < 16; servo++) {
            int pos = packetData[i + 2 + servo];
            if (pos == RAWSERVONOMOVE) {
              continue;
            }
            if (pos == RAWSERVODETACH) {
              servoDriver.setPin(servo, 0, false); 
              continue;
            }
            if (movetype == RAWSERVOADD) {
              pos += ServoPos[servo];
            } else if (movetype == RAWSERVOSUB) {
              pos = ServoPos[servo] - pos;
            }
            pos = constrain(pos, 0, 180);
            ServoPos[servo] = pos;
          }
          checkForCrashingHips();  
          for (int servo = 0; servo < 12; servo++) {
            setServo(servo, ServoPos[servo]);
          }
          i += 18; 
          mode = MODE_LEG;  
          startedStanding = -1; 
        } else {
          beep(BF_ERROR, BD_MED);
          Serial.print("PKERR:R:Short:"); Serial.print(i); Serial.print(":"); Serial.println(packetLengthReceived);
          return;  
        }
        break;
      case 'G': 
        if (i <= packetLengthReceived - 10) {
          LastGgaittype = packetData[i + 1];
          LastGreverse = packetData[i + 2];
          LastGhipforward = packetData[i + 3];
          LastGhipbackward = packetData[i + 4];
          LastGkneeup = packetData[i + 5];
          LastGkneedown = packetData[i + 6];
          int lean = packetData[i + 7];
          LastGtimeperiod = word(packetData[i + 8], packetData[i + 9]);
          LastGleanangle = constrain(lean - 70, -70, 70); 
          gait_command(LastGgaittype, LastGreverse, LastGhipforward, LastGhipbackward, LastGkneeup, LastGkneedown, LastGleanangle, LastGtimeperiod);
          i += 10;  
          startedStanding = -1; 
        } else {
          beep(BF_ERROR, BD_MED);
          Serial.println("PKERR:G:Short");
          return;  
        }
        break;
      case 'L': 
        if (i <= packetLengthReceived - 5) {
          unsigned int knee = packetData[i + 2];
          unsigned int hip = packetData[i + 3];
          if (knee == 255) {
            knee = NOMOVE;
            Serial.println("KNEE NOMOVE");
          }
          if (hip == 255) {
            hip = NOMOVE;
            Serial.println("HIP NOMOVE");
          }
          unsigned int legmask = packetData[i + 1];
          int raw = packetData[i + 4];
          Serial.print("SETLEG:"); Serial.print(legmask, DEC); Serial.print("/"); Serial.print(knee);
          Serial.print("/"); Serial.print(hip); Serial.print("/"); Serial.println(raw, DEC);
          setLeg(legmask, knee, hip, 0, raw);
          mode = MODE_LEG;   
          i += 5;  
          startedStanding = -1; 
          if (ServosDetached) { 
            attach_all_servos();
          }
          break;
        } else {
          beep(BF_ERROR, BD_MED);
          Serial.println("PKERR:L:Short");
          return;  
        }
        break;
      case 'T': 
        if (i <= packetLengthReceived - 2) {
          unsigned int command = packetData[i + 1];
          Serial.print("Trim Cmd: "); Serial.write(command); Serial.println("");
          i += 2;  
          startedStanding = -1; 
          mode = MODE_LEG;
          if (ServosDetached) { 
            attach_all_servos();
          }
          TrimInEffect = 1;   
          switch (command) {
            case 'f':
            case 'b':
              ServoTrim[TrimCurLeg + NUM_LEGS] = constrain(ServoTrim[TrimCurLeg + NUM_LEGS] + ((command == 'b') ? -1 : 1), 0, 255);
              beep(300, 30);
              break;
            case 'l':
            case 'r':
              ServoTrim[TrimCurLeg] = constrain(ServoTrim[TrimCurLeg] + ((command == 'r') ? -1 : 1), 0, 255);
              beep(500, 30);
              break;
            case 'w':
              TrimCurLeg = (TrimCurLeg + 1) % NUM_LEGS;
              setKnee(TrimCurLeg, 120);
              beep(100, 30);
              delay(500);  
              break;
            case 'R':
              TrimInEffect = 0;
              beep(100, 30);
              break;
            case 'S':
              save_trims();
              beep(800, 1000);
              delay(500);
              break;
            case 'P':
              TrimPose = 1 - TrimPose;  
              beep(500, 30);
              break;
            case 'E':
              erase_trims();
              beep(1500, 1000);
              break;
            default:
            case 's':
              break;
          }
          for (int i = 0; i < NUM_LEGS; i++) {
            setHip(i, HIP_NEUTRAL);
            if (TrimPose == 0) {
              setKnee(i, KNEE_STAND);
            } else {
              setKnee(i, KNEE_NEUTRAL);
            }
          }
          break;
        } else {
          beep(BF_ERROR, BD_MED);
          Serial.println("PKERR:T:Short");
          return;  
        }
        break;
      case 'P': 
        if (ServosDetached) { 
          attach_all_servos();
        }
        if (i <= packetLengthReceived - 13) {
          for (int servo = 0; servo < 12; servo++) {
            unsigned int position = packetData[i + 1 + servo];
            if (position < 0) {
              position = 0;
            } else if (position > 180 && position < 254) {
              position = 180;
            }
            if (position < 254) {
              ServoPos[servo] = position;
            } else if (position == 254) {
              servoDriver.setPin(servo, 0, false); 
            } else {
            }
          }
          checkForCrashingHips();
          for (int servo = 0; servo < 12; servo++) {
            setServo(servo, ServoPos[servo]);
          }
          mode = MODE_LEG;   
          i += 13;  
          startedStanding = -1; 
          break;
        } else {
          beep(BF_ERROR, BD_MED);
          Serial.println("PKERR:P:Short");
          return;  
        }
        break;  
      case 'S':   
        i++;  
        sendSensorData();
        if (0) {
          unsigned long t = hexmillis() % 1000; 
          if (t < 110) {
            beep(2000, 20);
          }
        }
        break;
      default:
        Serial.print("PKERR:BadSW:"); Serial.print(packetData[i]);
        Serial.print("i="); Serial.print(i); Serial.print(" RCV="); Serial.println(packetLengthReceived);
        beep(BF_ERROR, BD_MED);
        return;  
    }
  }
}
byte FrontReverse = 0;
long DebounceFrontReverse = 0;
void walk2(int cmd, int submode) {
  switch (submode) {
    case '1': 
      switch (cmd) {
        case 'f':
          gait_ripple(0, 0, HIP_FORWARD_RIPPLE, HIP_BACKWARD_RIPPLE, KNEE_RIPPLE_UP, KNEE_RIPPLE_DOWN, RIPPLE_CYCLE_TIME, 0);
          break;
        case 'b':
          gait_ripple(0, 1, HIP_FORWARD_RIPPLE, HIP_BACKWARD_RIPPLE, KNEE_RIPPLE_UP, KNEE_RIPPLE_DOWN, RIPPLE_CYCLE_TIME, 0);
          break;
        case 'l':
          gait_ripple(1, 0, HIP_FORWARD_RIPPLE, HIP_BACKWARD_RIPPLE, KNEE_RIPPLE_UP, KNEE_RIPPLE_DOWN, RIPPLE_CYCLE_TIME, 0);
          break;
        case 'r':
          gait_ripple(1, 1, HIP_FORWARD_RIPPLE, HIP_BACKWARD_RIPPLE, KNEE_RIPPLE_UP, KNEE_RIPPLE_DOWN, RIPPLE_CYCLE_TIME, 0);
          break;
        case 's':
          stand();
          break;
        case 'w':
          beep(400);
          gait_ripple(0, 0, HIP_NEUTRAL, HIP_NEUTRAL, KNEE_RIPPLE_UP, KNEE_RIPPLE_DOWN, RIPPLE_CYCLE_TIME, 0);
          break;
      }
      break;
    case '2': 
      switch (cmd) {
        case 'f':
          gait_belly(0, 0, HIP_FORWARD_BELLY, HIP_BACKWARD_BELLY, KNEE_BELLY_UP, KNEE_BELLY_DOWN, BELLY_CYCLE_TIME, 0);
          break;
        case 'b':
          gait_belly(0, 1, HIP_FORWARD_BELLY, HIP_BACKWARD_BELLY, KNEE_BELLY_UP, KNEE_BELLY_DOWN, BELLY_CYCLE_TIME, 0);
          break;
        case 'l':
          gait_belly(1, 0, HIP_FORWARD_BELLY, HIP_BACKWARD_BELLY, KNEE_BELLY_UP, KNEE_BELLY_DOWN, BELLY_CYCLE_TIME, 0);
          break;
        case 'r':
          gait_belly(1, 1, HIP_FORWARD_BELLY, HIP_BACKWARD_BELLY, KNEE_BELLY_UP, KNEE_BELLY_DOWN, BELLY_CYCLE_TIME, 0);
          break;
        case 'w':
          beep(300);
          gait_belly(0, 0, HIP_NEUTRAL, HIP_NEUTRAL, KNEE_BELLY_UP, KNEE_BELLY_DOWN, BELLY_CYCLE_TIME, 0);
          break;
        case 's':
          setLeg(ALL_LEGS, HIP_NEUTRAL, 85, FBSHIFT_BELLY, 0);
          break;
      }
      break;
    case '3':  
      switch (cmd) {
        case 'f':
          gait_quad(0, 0, HIP_FORWARD_QUAD, HIP_BACKWARD_QUAD, KNEE_QUAD_UP, KNEE_QUAD_DOWN, QUAD_CYCLE_TIME, 0);
          break;
        case 'b':
          gait_quad(0, 1, HIP_FORWARD_QUAD, HIP_BACKWARD_QUAD, KNEE_QUAD_UP, KNEE_QUAD_DOWN, QUAD_CYCLE_TIME, 0);
          break;
        case 'l':
          gait_quad(1, 0, HIP_FORWARD_QUAD, HIP_BACKWARD_QUAD, KNEE_QUAD_UP, KNEE_QUAD_DOWN, QUAD_CYCLE_TIME, 0);
          break;
        case 'r':
          gait_quad(1, 1, HIP_FORWARD_QUAD, HIP_BACKWARD_QUAD, KNEE_QUAD_UP, KNEE_QUAD_DOWN, QUAD_CYCLE_TIME, 0);
          break;
        case 'w': 
          beep(500);
          gait_quad(1, 1, HIP_NEUTRAL, HIP_NEUTRAL, KNEE_QUAD_UP, KNEE_QUAD_DOWN, QUAD_CYCLE_TIME, 0);
          break;
        case 's':
          gait_quad(1, 1, HIP_NEUTRAL, HIP_NEUTRAL, KNEE_QUAD_DOWN, KNEE_QUAD_DOWN, QUAD_CYCLE_TIME, 0);
          break;
      }
      break;
    case '4': 
      switch (cmd) {
        case 'f':
          servoOffset = FrontReverse;
          gait_tripod_scamper(0, 0);
          servoOffset = 0;
          break;
        case 'b':
          servoOffset = FrontReverse;
          gait_tripod_scamper(1, 0);
          servoOffset = 0;
          break;
        case 'l':
          servoOffset = 1 + FrontReverse;
          gait_tripod_scamper(0, 0);
          servoOffset = 0;
          break;
        case 'r':
          servoOffset = 5 + FrontReverse;
          gait_tripod_scamper(0, 0);
          servoOffset = 0;
          break;
        case 'w': 
          {
            long now = millis(); 
            if (now > DebounceFrontReverse) {
              FrontReverse = 3 - FrontReverse; 
              beep(500 + 300 * FrontReverse, 50); 
              DebounceFrontReverse = now + 300;
            }
          }
          break;
        case 's':
          stand();
          break;
      }
      break;
  }
}
#define MODETWITCH 0
#define MODESWAY 1
void dance_twitch(int cmd, int hipforward, int hipbackward, int kneeup, int kneedown, int twitchmode) {
#define NUM_TWITCH_PHASES 4
#define TWITCH_TIME 300
  int ttime = TWITCH_TIME;
  if (twitchmode == MODESWAY) {
    ttime = 750;
  }
  long t = hexmillis() % ttime;
  long phase = (NUM_TWITCH_PHASES * t) / ttime;
  int legs = ALL_LEGS;
  int revlr = 0;
  switch (cmd) {
    case 'f':
      legs = ALL_LEGS; break;
    case 'l':
      legs = LEFT_LEGS; break;
    case 'b':
      legs = ALL_LEGS;
      revlr = 1;
      break;
    case 'r':
      legs = RIGHT_LEGS; break;
  }
  transactServos();
  if (twitchmode == MODETWITCH) {
    setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, TWITCH_ADJ);
  } else {
    setLeg(ALL_LEGS, HIP_NEUTRAL, (KNEE_TIPTOES + KNEE_DOWN) / 2, TWITCH_ADJ);
    switch (cmd) {
      case 'f':
        setLeg(MIDDLE_LEGS, NOMOVE, KNEE_NEUTRAL, TWITCH_ADJ); break;
      case 'b':
        break;
      case 'l':
        setLeg(MIDDLE_LEGS, NOMOVE, KNEE_UP, TWITCH_ADJ);
        break;
      case 'r':
        setLeg(MIDDLE_LEGS, NOMOVE, KNEE_UP_MAX, TWITCH_ADJ, 1);
        break;
      case 'w':
        break;
    }
  }
  switch (phase) {
    case 0:
      if (twitchmode == MODETWITCH) {
        setLeg(legs, hipforward, kneeup, TWITCH_ADJ);
        if (revlr) {
          setLeg(LEFT_LEGS, hipbackward, kneeup, TWITCH_ADJ);
        }
      } else { 
        setLeg(LEFT_LEGS, HIP_NEUTRAL, kneeup, TWITCH_ADJ);
        setLeg(RIGHT_LEGS, HIP_NEUTRAL, kneedown, TWITCH_ADJ);
        switch (cmd) {
          case 'f':
            setLeg(MIDDLE_LEGS, NOMOVE, KNEE_UP_MAX, TWITCH_ADJ);
            break;
          case 'b':
            break;
          case 'l':
            setLeg(LEG1, NOMOVE, KNEE_NEUTRAL, TWITCH_ADJ);
            setLeg(LEG4, NOMOVE, KNEE_UP_MAX, TWITCH_ADJ);
            break;
          case 'r':
            setLeg(MIDDLE_LEGS, HIP_FORWARD, KNEE_UP_MAX, TWITCH_ADJ);
            break;
          case 'w':
            setLeg(RIGHT_LEGS, HIP_FORWARD_MAX, kneedown, TWITCH_ADJ);
            break;
        }
      }
      break;
    case 1: break;
    case 2:
      if (twitchmode == MODETWITCH) {
        setLeg(legs, hipbackward, kneedown, TWITCH_ADJ);
        if (revlr) {
          setLeg(LEFT_LEGS, hipforward, kneeup, TWITCH_ADJ);
        }
      } else { 
        setLeg(LEFT_LEGS, hipforward, kneedown, TWITCH_ADJ);
        setLeg(RIGHT_LEGS, hipforward, kneeup, TWITCH_ADJ);
        switch (cmd) {
          case 'f':
            setLeg(MIDDLE_LEGS, NOMOVE, KNEE_UP, TWITCH_ADJ); break;
          case 'b':
            break;
          case 'l':
            setLeg(LEG1, NOMOVE, KNEE_UP_MAX, TWITCH_ADJ);
            setLeg(LEG4, NOMOVE, KNEE_NEUTRAL, TWITCH_ADJ);
            break;
          case 'r':
            setLeg(MIDDLE_LEGS, HIP_BACKWARD, KNEE_UP_MAX, TWITCH_ADJ, 1);
            break;
          case 'w':
            setLeg(LEFT_LEGS, HIP_FORWARD_MAX, kneedown, TWITCH_ADJ);
            break;
        }
      }
      break;
    case 3: break;
  }
  commitServos();
}
void randomizeLeg(int legnum) {
  int hip = ServoPos[legnum];
#define RANDMAX 20
  hip += random(-RANDMAX, RANDMAX);
  hip = constrain(hip, HIP_BACKWARD, HIP_FORWARD);
  setHipRaw(legnum, hip);
  int knee = ServoPos[legnum + KNEE_OFFSET];
  knee += random(-RANDMAX, RANDMAX);
  knee = constrain(knee, KNEE_NEUTRAL - 10, KNEE_UP_MAX);
  setKnee(legnum, knee);
}
void dance_brownian(int cmd) {
  switch (cmd) {
    case 'f':
      randomizeLeg(0);
      randomizeLeg(5);
      break;
    case 'b':
      randomizeLeg(2);
      randomizeLeg(3);
      break;
    case 'l':
      for (int i = 0; i < 3; i++) {
        randomizeLeg(i);
      }
      break;
    case 'r':
      for (int i = 3; i < 6; i++) {
        randomizeLeg(i);
      }
      break;
    case 'w':
      for (int i = 0; i < 6; i++) {
        randomizeLeg(i);
      }
      break;
    case 's':
      setLeg(ALL_LEGS, NOMOVE, KNEE_NEUTRAL, 0, 1);
      return;
  }
}
void dance_star(int cmd) {
#define NUM_STAR_PHASES 4
#define STAR_TIME 700
  long t = hexmillis() % STAR_TIME;
  long phase = (NUM_STAR_PHASES * t) / STAR_TIME;
  int kneeposleft = KNEE_STAND;
  int kneeposright = KNEE_STAND;
  byte sidebyside = 0;
  switch (cmd) {
    case 'f':
      kneeposleft = kneeposright = KNEE_DOWN;
      break;
    case 'b':
      kneeposleft = kneeposright = KNEE_UP_MAX;
      break;
    case 'l':
      kneeposleft = KNEE_DOWN;
      kneeposright = KNEE_NEUTRAL;
      break;
    case 'r':
      kneeposleft = KNEE_NEUTRAL;
      kneeposright = KNEE_DOWN;
      break;
    case 'w':
      kneeposleft = KNEE_UP;
      kneeposright = KNEE_DOWN;
      break;
    case 's':
      setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_DOWN, 0, 1);
      return;
  }
  transactServos();
  setLeg(ALL_LEGS, HIP_NEUTRAL, (kneeposleft + kneeposright) / 2, 0, 1);
  switch (phase) {
    case 0:
      setLeg(TRIPOD1_LEGS, HIP_FORWARD + 5, kneeposleft + 20, 0, 1);
      setLeg(TRIPOD2_LEGS, HIP_BACKWARD - 5, kneeposright - 10, 0, 1);
      if (sidebyside) {
        setLeg(LEFT_LEGS, NOMOVE, KNEE_UP, 0, 1);
      }
      break;
    case 1: break;
    case 2:
      setLeg(TRIPOD2_LEGS, HIP_FORWARD + 5, kneeposright + 20, 0, 1);
      setLeg(TRIPOD1_LEGS, HIP_BACKWARD - 5, kneeposleft - 10, 0, 1);
      if (sidebyside) {
        setLeg(RIGHT_LEGS, NOMOVE, KNEE_UP, 0, 1);
      }
      break;
    case 3: break;
  }
  commitServos();
}
void dance2(int cmd, int submode) {
  switch (submode) {
    case SUBMODE_1: 
      switch (cmd) {
        case 'f':
          dance_twitch(cmd, HIP_FORWARD_RIPPLE, HIP_BACKWARD_RIPPLE, NOMOVE, NOMOVE, MODETWITCH);
          break;
        case 'b':
          dance_twitch(cmd, HIP_FORWARD_RIPPLE, HIP_BACKWARD_RIPPLE, NOMOVE, NOMOVE, MODETWITCH);
          break;
        case 'l':
          dance_twitch(cmd, NOMOVE, NOMOVE, KNEE_TIPTOES, KNEE_DOWN, MODETWITCH);
          break;
        case 'r':
          dance_twitch(cmd, NOMOVE, NOMOVE, KNEE_TIPTOES, KNEE_DOWN, MODETWITCH);
          break;
        case 'w': 
          dance_twitch(cmd, NOMOVE, NOMOVE, KNEE_TIPTOES, KNEE_DOWN, MODETWITCH);
          break;
        case 's': 
          setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, TWITCH_ADJ);
          break;
      }
      break;
    case SUBMODE_2: 
      switch (cmd) {
        case 'f':
          dance_twitch(cmd, NOMOVE, NOMOVE, KNEE_TIPTOES, KNEE_NEUTRAL, MODESWAY);
          break;
        case 'b':
          dance_twitch(cmd, NOMOVE, NOMOVE, KNEE_TIPTOES, KNEE_NEUTRAL, MODESWAY);
          break;
        case 'l':
          dance_twitch(cmd, NOMOVE, NOMOVE, KNEE_TIPTOES, KNEE_NEUTRAL, MODESWAY);
          break;
        case 'r':
          dance_twitch(cmd, NOMOVE, NOMOVE, KNEE_TIPTOES, KNEE_NEUTRAL, MODESWAY);
          break;
        case 'w': 
          dance_twitch(cmd, NOMOVE, NOMOVE, KNEE_TIPTOES, KNEE_NEUTRAL, MODESWAY);
          break;
        case 's': 
          setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, TWITCH_ADJ);
          break;
      }
      break;
    case SUBMODE_3:  
      dance_star(cmd);
      break;
    case SUBMODE_4:
      dance_brownian(cmd);
      break;
  }
}
void corgi(int begmode) {
#define NUM_DOGBEG_PHASES 4
#define DOGBEG_TIME 350
  long t = hexmillis() % DOGBEG_TIME;
  long phase = (NUM_DOGBEG_PHASES * t) / DOGBEG_TIME;
  transactServos();
  setLeg(BACK_LEGS, HIP_BACKWARD, KNEE_UP, 0);
  setLeg(MIDDLE_LEGS, HIP_FORWARD, KNEE_TIPTOES, 0);
  if (begmode == 's') { 
    commitServos();
    return;
  }
  switch (phase) {
    case 0:
      switch (begmode) {
        case 'f': 
          setLeg(LEG0, HIP_FORWARD, KNEE_DOWN, 0);
          setLeg(LEG5, HIP_FORWARD, KNEE_UP, 0);
          break;
        case 'b': 
          setLeg(LEG0, HIP_FORWARD + 30, KNEE_UP, 0);
          setLeg(LEG5, HIP_FORWARD + 30, KNEE_UP, 0);
          break;
        case 'w': 
          setLeg(LEG0, HIP_FORWARD + 30, KNEE_UP, 0);
          setLeg(LEG5, HIP_FORWARD - 30, KNEE_UP, 0);
          break;
        case 'l':
          setLeg(LEG0, HIP_FORWARD, KNEE_UP_MAX, 0); 
          setLeg(LEG5, HIP_FORWARD, KNEE_DOWN, 0);
          break;
        case 'r':
          setLeg(LEG5, HIP_FORWARD, KNEE_UP_MAX, 0);  
          setLeg(LEG0, HIP_FORWARD, KNEE_DOWN, 0);
          break;
      }
    case 1: break;
    case 2:
      switch (begmode) {
        case 'f':
          setLeg(LEG0, HIP_FORWARD, KNEE_UP, 0);
          setLeg(LEG5, HIP_FORWARD, KNEE_DOWN, 0);
          break;
        case 'b':
        case 'w':
          setLeg(LEG0, HIP_FORWARD, KNEE_UP, 0);
          setLeg(LEG5, HIP_FORWARD, KNEE_UP, 0);
          break;
        case 'r':
          setLeg(LEG5, HIP_FORWARD, KNEE_UP, 0);
          break;
        case 'l':
          setLeg(LEG0, HIP_NEUTRAL, KNEE_UP_MAX, 0);
          break;
      }
      break;
    case 3: break;
  }
  commitServos();
}
void wave_hello(int cmd) {
#define NUM_HELLO_PHASES 4
#define HELLO_TIME 300
  long t = hexmillis() % HELLO_TIME;
  long phase = (NUM_HELLO_PHASES * t) / HELLO_TIME;
  transactServos();
  setLeg(ALL_LEGS & (~LEG5), HIP_NEUTRAL, KNEE_STAND, 0); 
  switch (phase) {
    case 0:
      if (cmd == 'r') {
        setLeg(LEG5, HIP_FORWARD, KNEE_UP_MAX, 0);  
      } else {
        setLeg(LEG5, HIP_FORWARD, KNEE_UP_MAX, 0); 
      }
    case 1: break;
    case 2:
      if (cmd == 'r') {
        setLeg(LEG5, HIP_FORWARD, KNEE_UP, 0);
      } else {
        setLeg(LEG5, HIP_NEUTRAL, KNEE_UP_MAX, 0);
      }
      break;
    case 3: break;
  }
  commitServos();
}
void incremental_move() {
  return;
#define MOVEINCREMENT 10  
  for (int i = 0; i < NUM_LEGS; i++) {
    int h, k;
    h = ServoPos[i];
    k = ServoPos[i + KNEE_OFFSET];
    int diff = ServoTarget[i + KNEE_OFFSET] - k;
    if (diff <= -MOVEINCREMENT) {
      k -= MOVEINCREMENT;
    } else if (diff >= MOVEINCREMENT) {
      k += MOVEINCREMENT;
    } else {
      k = ServoTarget[i + KNEE_OFFSET];
    }
    setKnee(i, k);
    diff = ServoTarget[i] - h;
    if (diff <= -MOVEINCREMENT) {
      h -= MOVEINCREMENT;
    } else if (diff >= MOVEINCREMENT) {
      h += MOVEINCREMENT;
    } else {
      h = ServoTarget[i];
    }
    setHipRaw(i, h);
  }
  return;  
}
void fight2(int cmd, int submode) {
  switch (submode) {
    case SUBMODE_1: 
      corgi(cmd);
      break;
    case SUBMODE_2: 
      switch (cmd) {
        case 's':
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = ServoPos[i + NUM_LEGS];
            ServoTarget[i] = ServoPos[i];
          }
          break;
        case 'w':  
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
            ServoTarget[i] = 90;
          }
          break;
        case 'f':
          ServoTarget[5 + KNEE_OFFSET] = 180;
          break;
        case 'b':
          ServoTarget[5 + KNEE_OFFSET] = 0;
          break;
        case 'l':
          ServoTarget[5] = 0;
          break;
        case 'r':
          ServoTarget[5] = 180;
          break;
      }
      break; 
    case SUBMODE_3:  
      switch (cmd) {
        case 's':
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = ServoPos[i + NUM_LEGS];
            ServoTarget[i] = ServoPos[i];
          }
          break;
        case 'w':  
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
            ServoTarget[i] = 90;
          }
          break;
        case 'f': 
          ServoTarget[0 + KNEE_OFFSET] = 180;
          break;
        case 'b': 
          ServoTarget[0 + KNEE_OFFSET] = 0;
          break;
        case 'l': 
          ServoTarget[0] = 0;
          break;
        case 'r': 
          ServoTarget[0] = 180;
          break;
      }
      break; 
    case SUBMODE_4: 
      switch (cmd) {
        case 's':
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = ServoPos[i + NUM_LEGS];
            ServoTarget[i] = ServoPos[i];
          }
          break;
        case 'w':  
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
            ServoTarget[i] = 90;
          }
          break;
        case 'f': 
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i] = 90;
            ServoTarget[i + KNEE_OFFSET] = 0;
          }
          break;
        case 'b': 
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i] = 90;
            ServoTarget[i + KNEE_OFFSET] = 180;
          }
          break;
        case 'l': 
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i] = 0;
            ServoTarget[i + KNEE_OFFSET] = 0;
          }
          break;
        case 'r': 
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i] = 180;
            ServoTarget[i + KNEE_OFFSET] = 180;
          }
          break;
      }
      break; 
  }
}
int flash(unsigned long t) {
  return (millis() % (2 * t)) > t; 
}
unsigned long freqWatchDog = 0;
unsigned long SuppressScamperUntil = 0;  
void checkForServoSleep() {
  if (millis() > freqWatchDog) { 
    Wire.beginTransmission(SERVO_IIC_ADDR);
    Wire.write(0);  
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)SERVO_IIC_ADDR, (uint8_t)1);
    int mode1 = Wire.read();
    if (mode1 & 16) { 
      resetServoDriver();
      if (beepOnServoReset) {
        beep(1200, 50); 
        beep(800, 50);
      }
      SuppressScamperUntil = millis() + 10000;  
    }
    freqWatchDog = millis() + 100; 
  }
}
void GeneralCheckSmoothMoves() {
  static long lastSmoothTime = 0;
#define SMOOTHTIME 20 
  long now = millis(); 
  if (now >= lastSmoothTime + SMOOTHTIME) {
    lastSmoothTime = now;
  } else {
    return; 
  }
  for (int servo = 0; servo < NUM_ACTIVE_SERVO; servo++) {
    SmoothMove(servo);
  }
}
void SmoothMove(int servo) {
#define SMOOTHINC  3  
  int tmp = deferServoSet;
  if (abs(ServoPos[servo] - ServoTarget[servo]) <= SMOOTHINC) {
    deferServoSet = 0;
    setServo(servo, ServoTarget[servo]);
    deferServoSet = tmp;
    return;
  }
  deferServoSet = 0;
  setServo(servo, ServoPos[servo] + (ServoPos[servo] > ServoTarget[servo] ? (0 - SMOOTHINC) : SMOOTHINC));
  deferServoSet = tmp;
}
unsigned long ReportTime = 0;
unsigned long SuppressModesUntil = 0;
void loop() {
  checkForServoSleep();
  checkForCrashingHips();
  int p = analogRead(A0);
  int factor = 1;
  if (p < 50) {
    Dialmode = DIALMODE_STAND;
  } else if (p < 150) {
    Dialmode = DIALMODE_ADJUST;
  } else if (p < 300) {
    Dialmode = DIALMODE_TEST;
  } else if (p < 750) {
    Dialmode = DIALMODE_DEMO;
  } else if (p < 950) {
    Dialmode = DIALMODE_RC_GRIPARM;
  } else {
    Dialmode = DIALMODE_RC;
  }
  if (Dialmode != priorDialMode && priorDialMode != -1) {
    beep(100 + 100 * Dialmode, 60); 
    SuppressModesUntil = millis() + 1000; 
  }
  priorDialMode = Dialmode;
  if (millis() < SuppressModesUntil) { 
    return;
  }
  if (Dialmode == DIALMODE_STAND) { 
    digitalWrite(13, LOW);  
    delay(250);
    stand();
    setGrip(90, 90);  
    if (millis() > ReportTime) { 
      ReportTime = millis() + 1000;
      Serial.println("Stand Mode, Sensors:");
      Serial.print(" A3="); Serial.print(analogRead(A3));
      Serial.print(" A6="); Serial.print(analogRead(A6));
      Serial.print(" A7="); Serial.print(analogRead(A7));
      Serial.print(" Dist="); Serial.print(readUltrasonic());
      Serial.println("");
    }
  } else if (Dialmode == DIALMODE_ADJUST) {  
    digitalWrite(13, flash(100));  
    stand_90_degrees();
    if (millis() > ReportTime) { 
      ReportTime = millis() + 1000;
      Serial.println("AdjustMode");
    }
  } else if (Dialmode == DIALMODE_TEST) {   
    pinMode(13, flash(500));      
    for (int i = 0; i < 2 * NUM_LEGS + NUM_GRIPSERVOS; i++) {
      p = analogRead(A0);
      if (p > 300 || p < 150) {
        break;
      }
      setServo(i, 140);
      delay(500);
      if (p > 300 || p < 150) {
        break;
      }
      setServo(i, 40);
      delay(500);
      setServo(i, 90);
      delay(100);
      Serial.print("SERVO: "); Serial.println(i);
    }
  } else if (Dialmode == DIALMODE_DEMO) {  
    digitalWrite(13, flash(2000));  
    random_gait(timingfactor);
    if (millis() > ReportTime) {  
      ReportTime = millis() + 1000;
      Serial.println("Demo Mode");
    }
    return;
  } else { 
    digitalWrite(13, HIGH);   
    if (millis() > ReportTime) { 
      ReportTime = millis() + 2000;
      Serial.print("RC Mode:"); Serial.print(ServosDetached); Serial.write(lastCmd); Serial.write(mode); Serial.write(submode); Serial.println("");
    }
    int gotnewdata = receiveDataHandler();  
    if (millis() > LastValidReceiveTime + 1000) { 
      if (millis() > LastValidReceiveTime + 15000) {
        beep(200, 40); 
        delay(100);
        beep(400, 40);
        delay(100);
        beep(600, 40);
        BlueTooth.begin(38400);
        LastReceiveTime = LastValidReceiveTime = millis();
        lastCmd = -1;  
      }
      long losstime = millis() - LastValidReceiveTime;
      return;  
    }
    if (gotnewdata == 0) {
      if (lastCmd == -1) {
        return;
      }
      if (
        (mode == MODE_FIGHT && (submode == SUBMODE_3 || submode == SUBMODE_4)) ||
        (mode == MODE_FIGHT2 && (submode == SUBMODE_2 || submode == SUBMODE_3 || submode == SUBMODE_4))  ||
        (Dialmode == DIALMODE_RC_GRIPARM && mode == MODE_FIGHT && (submode == SUBMODE_2))
      ) {
        GeneralCheckSmoothMoves();
        return;
      }
    } else {
      LastReceiveTime = millis();    
    }
    if (mode == MODE_LEG) {
      return;
    } else if (mode == MODE_GAIT) {
      gait_command(LastGgaittype, LastGreverse, LastGhipforward, LastGhipbackward,
                   LastGkneeup, LastGkneedown, LastGleanangle, LastGtimeperiod);
      return;
    }
    ScamperTracker -= 1;
    if (ScamperTracker < 0) {
      ScamperTracker = 0;
    } else {
    }
    if (mode != MODE_WALK2 || submode != SUBMODE_4) {
      FrontReverse = 0; 
    }
    switch (lastCmd) {
      case '?': BlueTooth.println("#Vorpal Hexapod");
        break;
      case 'W':
        mode = MODE_WALK;
        break;
      case 'X':
        mode = MODE_WALK2;
        break;
      case 'F':
        mode = MODE_FIGHT; startedStanding = -1;
        break;
      case 'Y':
        mode = MODE_FIGHT2; startedStanding = -1;
        break;
      case 'D':
        mode = MODE_DANCE; startedStanding = -1;
        break;
      case 'Z':
        mode = MODE_DANCE2;
        break;
      case '1':
      case '2':
      case '3':
      case '4':
        submode = lastCmd;
        break;
      case 'w':  
        startedStanding = -1;
        switch (mode) {
          case MODE_FIGHT:
            fight_mode(lastCmd, submode, 660 * timingfactor);
            break;
          case MODE_DANCE:
            if (submode == SUBMODE_1) {
              dance_dab(timingfactor);
            } else if (submode == SUBMODE_2) {
              dance_ballet(lastCmd);
            } else if (submode == SUBMODE_3) {
              wave(lastCmd);
            } else if (submode == SUBMODE_4) {
              dance_hands(lastCmd);
            }
            break;
          case MODE_WALK: {
              beep(400);
              if (submode == SUBMODE_2) { 
                factor = 2;
              }
              int cyc = TRIPOD_CYCLE_TIME * factor;
              if (submode == SUBMODE_4) {
                cyc = TRIPOD_CYCLE_TIME / 2; 
              }
              gait_tripod(1, 90, 90,
                          KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ, KNEE_DOWN,
                          cyc);
            }
            break;
          case MODE_WALK2: walk2(lastCmd, submode); break;
          case MODE_DANCE2: dance2(lastCmd, submode); break;
          case MODE_FIGHT2: fight2(lastCmd, submode); break;
          default:     
            beep(400);
            break;
        }
        break;
      case 'f':  
        startedStanding = -1;
        switch (mode) {
          case MODE_WALK:
            if (submode == SUBMODE_4 && SuppressScamperUntil < millis()) {
              gait_tripod_scamper(0, 0);
            } else {
              if (submode == SUBMODE_2) { 
                factor = 2;
              }
              gait_tripod(1, (submode == SUBMODE_3) ? HIP_BACKWARD_SMALL : HIP_BACKWARD,
                          (submode == SUBMODE_3) ? HIP_FORWARD_SMALL : HIP_FORWARD,
                          KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ, KNEE_DOWN,
                          TRIPOD_CYCLE_TIME * factor);
            }
            break;
          case MODE_DANCE:
            if (submode == SUBMODE_1) {
              dance(NO_LEGS, submode, timingfactor);
            } else if (submode == SUBMODE_2) {
              dance_ballet(lastCmd);
            } else if (submode == SUBMODE_3) {
              wave(lastCmd);
            } else if (submode == SUBMODE_4) {
              dance_hands(lastCmd);
            }
            break;
          case MODE_FIGHT:
            fight_mode(lastCmd, submode, FIGHT_CYCLE_TIME * timingfactor);
            break;
          case MODE_WALK2: walk2(lastCmd, submode); break;
          case MODE_DANCE2: dance2(lastCmd, submode); break;
          case MODE_FIGHT2: fight2(lastCmd, submode); break;
        }
        break;
      case 'b':  
        startedStanding = -1;
        switch (mode) {
          case MODE_WALK:
            if (submode == SUBMODE_4 && SuppressScamperUntil < millis()) {
              gait_tripod_scamper(1, 0);
            } else {
              if (submode == SUBMODE_2) {
                factor = 2;
              }
              gait_tripod(0, (submode == SUBMODE_3) ? HIP_BACKWARD_SMALL : HIP_BACKWARD,
                          (submode == SUBMODE_3) ? HIP_FORWARD_SMALL : HIP_FORWARD,
                          KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ, KNEE_DOWN, TRIPOD_CYCLE_TIME * factor);
            }
            break;
          case MODE_DANCE:
            if (submode == SUBMODE_1) {
              boogie_woogie(NO_LEGS, submode, timingfactor);
            } else if (submode == SUBMODE_2) {
              dance_ballet(lastCmd);
            } else if (submode == SUBMODE_3) {
              wave(lastCmd);
            } else if (submode == SUBMODE_4) {
              dance_hands(lastCmd);
            }
            break;
          case MODE_FIGHT:
            fight_mode(lastCmd, submode, FIGHT_CYCLE_TIME * timingfactor);
            break;
          case MODE_WALK2: walk2(lastCmd, submode); break;
          case MODE_DANCE2: dance2(lastCmd, submode); break;
          case MODE_FIGHT2: fight2(lastCmd, submode); break;
        }
        break;
      case 'l': 
        startedStanding = -1;
        switch (mode) {
          case MODE_WALK:
            if (submode == SUBMODE_2) {
              factor = 2;
            }
            if (submode == SUBMODE_4 && SuppressScamperUntil < millis()) {
              gait_tripod_scamper(1, 1);
            } else {
              turn(0, (submode == SUBMODE_3) ? HIP_BACKWARD_SMALL : HIP_BACKWARD,
                   (submode == SUBMODE_3) ? HIP_FORWARD_SMALL : HIP_FORWARD,
                   KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ, KNEE_DOWN, TRIPOD_CYCLE_TIME * factor);
            }
            break;
          case MODE_DANCE:
            if (submode == SUBMODE_1) {
              dance(TRIPOD1_LEGS, submode, timingfactor);
            } else if (submode == SUBMODE_2) {
              dance_ballet(lastCmd);
            } else if (submode == SUBMODE_3) {
              wave(lastCmd);
            } else if (submode == SUBMODE_4) {
              dance_hands(lastCmd);
            }
            break;
          case MODE_FIGHT:
            fight_mode(lastCmd, submode, FIGHT_CYCLE_TIME * timingfactor);
            break;
          case MODE_WALK2: walk2(lastCmd, submode); break;
          case MODE_DANCE2: dance2(lastCmd, submode); break;
          case MODE_FIGHT2: fight2(lastCmd, submode); break;
        }
        break;
      case 'r':  
        startedStanding = -1;
        switch (mode) {
          case MODE_WALK:
            if (submode == SUBMODE_2) {
              factor = 2;
            }
            if (submode == SUBMODE_4 && SuppressScamperUntil < millis()) {
              gait_tripod_scamper(0, 1);
            } else {
              turn(1, (submode == SUBMODE_3) ? HIP_BACKWARD_SMALL : HIP_BACKWARD,
                   (submode == SUBMODE_3) ? HIP_FORWARD_SMALL : HIP_FORWARD,
                   KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ,
                   KNEE_DOWN, TRIPOD_CYCLE_TIME * factor);
            }
            break;
          case MODE_DANCE:
            if (submode == SUBMODE_1) {
              dance(TRIPOD2_LEGS, submode, timingfactor);
            } else if (submode == SUBMODE_2) {
              dance_ballet(lastCmd);
            } else if (submode == SUBMODE_3) {
              wave(lastCmd);
            } else if (submode == SUBMODE_4) {
              dance_hands(lastCmd);
            }
            break;
          case MODE_FIGHT:
            fight_mode(lastCmd, submode, FIGHT_CYCLE_TIME * timingfactor);
            break;
          case MODE_WALK2: walk2(lastCmd, submode); break;
          case MODE_DANCE2: dance2(lastCmd, submode); break;
          case MODE_FIGHT2: fight2(lastCmd, submode); break;
        }
        break;
      case 's':  
        if (startedStanding == -1) {
          startedStanding = millis();
        }
        if (mode == MODE_FIGHT) {
          startedStanding = millis();  
          fight_mode(lastCmd, submode, 660 * timingfactor);
        } else if (mode == MODE_DANCE && submode == SUBMODE_2) { 
          tiptoes();
        } else if (mode == MODE_DANCE && submode == SUBMODE_4) {
          dance_hands(lastCmd);
        } else if (mode == MODE_DANCE2) {
          dance2(lastCmd, submode);
        } else if (mode == MODE_WALK2) {
          walk2(lastCmd, submode);
        } else if (mode == MODE_FIGHT2) {
          fight2(lastCmd, submode);
        } else {
          if (millis() - startedStanding > BATTERYSAVER) {
            detach_all_servos();
            return;
          }
          stand();
        }
        break;
      case 'a': 
        stand_90_degrees();
        break;
      default:
        Serial.print("BADCHR:"); Serial.write(lastCmd); Serial.println("");
        beep(100, 20);
    }  
  }  
}