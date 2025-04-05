#ifndef HEXAPOD_CONFIG
#define HEXAPOD_CONFIG
//#include <EEPROM.h>

//const char *Version = "#RV3r1c"; // This version supports padding out radio packets to 230 bytes to support newer HC05 firmware versions (starting with 3 and 4)
extern char *Version;

#include <Adafruit_PWMServoDriver.h>


extern bool beepOnServoReset;



//int FreqMult = 1;   // PWM frequency multiplier, use 1 for analog servos and up to about 3 for digital.
extern int FreqMult;
extern byte SomeLegsUp;  // this is a flag to detect situations where a user rapidly switches moves that would

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


#define SERVO_IIC_ADDR  (0x49)    // default servo driver IIC address
//Adafruit_PWMServoDriver servoDriver = Adafruit_PWMServoDriver(SERVO_IIC_ADDR);
extern Adafruit_PWMServoDriver servoDriver;

#define BeeperPin 4           // digital 4 used for beeper
#define ServoTypePin 5        // 5 is used to signal digital vs. analog servo mode
#define ServoTypeGroundPin 6  // 6 provides a ground to pull 5 low if digital servos are in use
#define GripElbowCurrentPin A6  // current sensor for grip arm elbow servo, only used if GRIPARM mode
#define GripClawCurrentPin  A7  // current sensor for grip claw servo, only used if GRIPARM mode
#define BF_ERROR  100         // deep beep for error situations
#define BD_MED    50          // medium long beep duration

#define PWMFREQUENCY (60*FreqMult)

#define SERVOMIN  (190*FreqMult) // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  (540*FreqMult) // this is the 'maximum' pulse length count (out of 4096)

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

// default positions for knee and hip. Note that hip position is
// automatically reversed for the left side by the setHip function
// These are in degrees

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

//unsigned short ServoPos[MAX_SERVO]; // the last commanded position of each servo
extern unsigned short ServoPos[];
//unsigned short ServoTarget[MAX_SERVO];
extern unsigned short ServoTarget[];
//long ServoTime[MAX_SERVO]; // the time that each servo was last commanded to a new position
extern long ServoTime[];
extern byte ServoTrim[];
//byte ServoTrim[MAX_SERVO];  // trim values for fine adjustments to servo horn positions
//long startedStanding = 0;   // the last time we started standing, or reset to -1 if we didn't stand recently
extern long startedStanding;
//long LastReceiveTime = 0;   // last time we got a bluetooth packet
extern long LastReceiveTime;
//unsigned long LastValidReceiveTime = 0;  // last time we got a completely valid packet including correct checksum
extern unsigned long LastValidReceiveTime;
//byte HC05_pad = 0;  // if set to 1 we will pad out sensor data with nulls to 230 bytes to support some newer hc05 models that buffer.
extern byte HC05_pad;
// this is turned on if we see padding coming from the gamepad.

#define DIALMODE_STAND 0
#define DIALMODE_ADJUST 1
#define DIALMODE_TEST 2
#define DIALMODE_DEMO 3
#define DIALMODE_RC_GRIPARM 4
#define DIALMODE_RC 5




extern int Dialmode;   // What's the robot potentiometer set to?

#define NUM_GRIPSERVOS ((Dialmode == DIALMODE_RC_GRIPARM)?2:0)  // if we're in griparm mode there are 2 griparm servos, else there are none

#define TRIM_ZERO 127

// byte TrimInEffect = 1;
// byte TrimCurLeg = 0;
// byte TrimPose = 0;

extern long ScamperTracker;
extern unsigned long NextScamperPhaseTime;
extern int ScamperPhase;

#define FBSHIFT_QUAD 25
#define HIP_FORWARD_QUAD (HIP_FORWARD)
#define HIP_BACKWARD_QUAD (HIP_BACKWARD)
#define KNEE_QUAD_UP (KNEE_DOWN+30)
#define KNEE_QUAD_DOWN (KNEE_DOWN)
#define QUAD_CYCLE_TIME 600

#define FBSHIFT_BELLY 55
#define HIP_FORWARD_BELLY (HIP_FORWARD+10)
#define HIP_BACKWARD_BELLY (HIP_BACKWARD-10)
#define KNEE_BELLY_UP (KNEE_NEUTRAL-30)
#define KNEE_BELLY_DOWN (KNEE_NEUTRAL+30)
#define BELLY_CYCLE_TIME 600

extern unsigned long nextGaitTime;
extern int curGait;
extern int curReverse;

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

#define ULTRAOUTPUTPIN 7      // TRIG
#define ULTRAINPUTPIN  8

extern byte deferServoSet;
extern int servoOffset;

#endif
