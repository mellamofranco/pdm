#include "servo_control.h"
#include "config.h"

// Global variable definitions
// unsigned short ServoPos[MAX_SERVO];
// unsigned short ServoTarget[MAX_SERVO];
// long ServoTime[MAX_SERVO];
// byte ServoTrim[MAX_SERVO];

// This function sets the positions of both the knee and hip in
// a single command. For hip, the left side is reversed so
// forward direction is consistent.
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
    legmask = (legmask >> 1); // shift down one bit position to check the next legmask bit
  }
}

// this version of setHip does no processing at all
void setHipRaw(int leg, int pos) {
  setServo(leg, pos);
}

// this version of setHip adjusts for left and right legs so
// that 0 degrees moves "forward"
void setHip(int leg, int pos) {
  // reverse the left side for consistent forward motion
  if (leg >= LEFT_START) {
    pos = 180 - pos;
  }
  setHipRaw(leg, pos);
}

// this version of setHip adjusts not only for left and right,
// but also shifts the front legs a little back and the back legs
// forward to make a better balance for certain gaits
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

void setServo(int servo, int pos) {
  // Add trim value to the position
  // pos += ServoTrim[servo] - TRIM_ZERO;
  // pos += ServoTrim[servo] - TRIM_ZERO;


  // Constrain the position to valid range
  pos = constrain(pos, 0, 180);

  // Convert position to pulse length
  int pulse = map(pos, 0, 180, SERVOMIN, SERVOMAX);

  // Update the servo position
  servoDriver.setPWM(servo, 0, pulse);

  // Store the position and time
  ServoPos[servo] = pos;
  ServoTarget[servo] = pos;
  ServoTime[servo] = millis();
}
