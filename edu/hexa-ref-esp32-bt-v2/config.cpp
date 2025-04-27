#include "config.h"

byte ServoTrim[NUM_LEGS];
Adafruit_PWMServoDriver servoDriver = Adafruit_PWMServoDriver(SERVO_IIC_ADDR);
int FreqMult = 1;
unsigned short ServoPos[MAX_SERVO];
int Dialmode;
unsigned short ServoTarget[MAX_SERVO];
long ServoTime[MAX_SERVO];
char *Version = "#RV3r1c";
byte HC05_pad = 0;
long startedStanding = 0;
unsigned long LastValidReceiveTime = 0;
long LastReceiveTime = 0;
bool beepOnServoReset = true;
byte SomeLegsUp = 0;
long ScamperTracker = 0;
unsigned long NextScamperPhaseTime = 0;
int ScamperPhase = 0;

unsigned long nextGaitTime = 0;

int curGait = G_STAND;
int curReverse = 0;

byte deferServoSet = 0;
int servoOffset = 0;

byte FrontReverse = 0;
long DebounceFrontReverse = 0;
