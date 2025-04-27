#include "movement.h"
#include "config.h"
#include "servo_control.h"
#include "utils.h"


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

  long t = hexmillis() % timeperiod;
  long phase = (NUM_TURN_PHASES * t) / timeperiod;

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
    phase = 11 - phase; // go backwards
  }

  switch (dpad) {
    case 'f':
    case 'b':
      // swirl around
      setLeg(ALL_LEGS, HIP_NEUTRAL, NOMOVE, 0); // keep hips stable at 90 degrees
      if (phase < NUM_LEGS) {
        setKnee(phase, KNEE_WAVE);
      } else {
        setKnee(phase - NUM_LEGS, KNEE_STAND);
      }
      break;
    case 'l':
      // teeter totter around font/back legs

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
          ServoTarget[i + KNEE_OFFSET] = ServoPos[i + NUM_LEGS];
          ServoTarget[i] = ServoPos[i];
        }
        break;
      case 'w':  // reset to standard standing position, resets both hips and knees
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
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
          ServoTarget[i + KNEE_OFFSET] = ServoPos[i + NUM_LEGS];
          ServoTarget[i] = ServoPos[i];
        }
        break;
      case 'w':  // reset to standard standing position, resets both hips and knees
        for (int i = 0; i < NUM_LEGS; i++) {
          ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
          ServoTarget[i] = 90;
        }
        break;
      case 'f': // move knees into forward crouch, leave hips alone

        if (ServoPos[8] == KNEE_STAND) { // the back legs are standing, so crouch the front legs
          ServoTarget[6] = ServoTarget[11] = KNEE_CROUCH;
          ServoTarget[7] = ServoTarget[10] = KNEE_HALF_CROUCH;
          ServoTarget[8] = ServoTarget[9] = KNEE_STAND;
        } else { // bring the back legs up first
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
          }
        }
        break;
      case 'b': // move back legs down so robot tips backwards
        if (ServoPos[6] == KNEE_STAND) { // move the back legs down
          ServoTarget[6] = ServoTarget[11] = KNEE_STAND;
          ServoTarget[7] = ServoTarget[10] = KNEE_HALF_CROUCH;
          ServoTarget[8] = ServoTarget[9] = KNEE_CROUCH;
        } else { // front legs are down, return to stand first
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
    return; // we're done, the smoothmove function will take care of reaching the target positions
  }

  // If we get here, we are in either submode 1 or 2
  //
  // submode 1: fight with two front legs, individual movement
  // submode 2: fight with two front legs, in unison

  setLeg(MIDDLE_LEGS, HIP_FORWARD + 10, KNEE_STAND, 0);
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
        setLeg(LEG5, HIP_FISTS_FORWARD + 30, KNEE_RELAX, 0);
      }
      break;

    case 'r':  // right front leg moves right, left stays forward
      if (mode == SUBMODE_1) {
        setLeg(LEG5, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG0, HIP_FISTS_FORWARD, KNEE_RELAX, 0);
      } else { // submode B
        setLeg(LEG5, HIP_NEUTRAL, KNEE_UP, 0);
        setLeg(LEG0, HIP_FISTS_FORWARD + 30, KNEE_RELAX, 0);
      }
      break;

    case 'w':  // automatic ninja motion mode with both legs swinging left/right/up/down furiously!

#define NUM_PUGIL_PHASES 8
      { // we need a new scope for this because there are local variables

        long t = hexmillis() % timeperiod;
        long phase = (NUM_PUGIL_PHASES * t) / timeperiod;

        //Serial.print("PHASE: ");
        //Serial.println(phase);

        switch (phase) {
          case 0:
            // Knees down, hips forward
            setLeg(FRONT_LEGS, HIP_FISTS_FORWARD, (mode == SUBMODE_2) ? KNEE_DOWN : KNEE_RELAX, 0);
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
            setLeg(LEG5, HIP_FISTS_FORWARD, (mode == SUBMODE_2) ? KNEE_DOWN : KNEE_STAND, 0);
            break;

          case 5:
            // kick phase 2
            // hips forward, kick
            setLeg(LEG0, HIP_FISTS_FORWARD, (mode == SUBMODE_2) ? KNEE_DOWN : KNEE_STAND, 0);
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

  long t = hexmillis() % timeperiod;
  long phase = (NUM_TRIPOD_PHASES * t) / timeperiod;

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
      case 0: NextScamperPhaseTime = millis() + KNEEDELAY; break;
      case 1: NextScamperPhaseTime = millis() + HIPDELAY; break;
      case 2: NextScamperPhaseTime = millis() + KNEEDELAY; break;
      case 3: NextScamperPhaseTime = millis() + KNEEDELAY; break;
      case 4: NextScamperPhaseTime = millis() + HIPDELAY; break;
      case 5: NextScamperPhaseTime = millis() + KNEEDELAY; break;
    }

  }

  //Serial.print("ScamperPhase: "); Serial.println(ScamperPhase);

  transactServos();
  switch (ScamperPhase) {
    case 0:
      // in this phase, center-left and noncenter-right legs raise up at
      // the knee
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_SCAMPER, 0);
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_DOWN, 0);
      break;

    case 1:
      // in this phase, the center-left and noncenter-right legs move forward
      // at the hips, while the rest of the legs move backward at the hip
      setLeg(TRIPOD1_LEGS, hipforward, NOMOVE, FBSHIFT, turn);
      setLeg(TRIPOD2_LEGS, hipbackward, NOMOVE, FBSHIFT, turn);
      break;

    case 2:
      // now put the first set of legs back down on the ground
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_DOWN, 0);
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_DOWN, 0);
      break;

    case 3:
      // lift up the other set of legs at the knee
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_SCAMPER, 0, turn);
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_DOWN, 0, turn);
      break;

    case 4:
      // similar to phase 1, move raised legs forward and lowered legs backward
      setLeg(TRIPOD1_LEGS, hipbackward, NOMOVE, FBSHIFT, turn);
      setLeg(TRIPOD2_LEGS, hipforward, NOMOVE, FBSHIFT, turn);
      break;

    case 5:
      // put the second set of legs down, and the cycle repeats
      setLeg(TRIPOD2_LEGS, NOMOVE, KNEE_DOWN, 0);
      setLeg(TRIPOD1_LEGS, NOMOVE, KNEE_DOWN, 0);
      break;
  }
  commitServos();
}

// call gait_ripple with leanangle = 0
void gait_ripple(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod) {
  gait_ripple(turn, reverse, hipforward, hipbackward, kneeup, kneedown, timeperiod, 0);
}

void gait_ripple(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle) {
  // the gait consists of 19 phases. This code determines what phase
  // we are currently in by using the millis clock modulo the
  // desired time period that all phases should consume.
  // Right now each phase is an equal amount of time but this may not be optimal

  if (turn) {
    reverse = 1 - reverse; // yeah this is weird but if you're turning you need to reverse the sense of reverse to make left and right turns come out correctly
  }
  if (reverse) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }

#define NUM_RIPPLE_PHASES 19

  long t = hexmillis() % timeperiod;
  long phase = (NUM_RIPPLE_PHASES * t) / timeperiod;

  //Serial.print("PHASE: ");
  //Serial.println(phase);

  transactServos();

  if (phase == 18) {
    setLeg(ALL_LEGS, hipbackward, NOMOVE, FBSHIFT, turn);
  } else {
    int leg = phase / 3; // this will be a number between 0 and 5 because phase==18 is handled above
    leg = 1 << leg;
    int subphase = phase % 3;

    switch (subphase) {
      case 0:
        setLeg(leg, NOMOVE, kneeup, 0);
        break;
      case 1:
        setLeg(leg, hipforward, NOMOVE, FBSHIFT, turn);  // move in "raw" mode if turn is engaged, this makes all legs ripple in the same direction
        break;
      case 2:
        setLeg(leg, NOMOVE, kneedown, 0);
        break;
    }
  }
  commitServos();
}

void gait_quad(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle) {
  // the gait walks using a quadruped gait with middle legs raised up. This code determines what phase
  // we are currently in by using the millis clock modulo the
  // desired time period that all phases should consume.
  // Right now each phase is an equal amount of time but this may not be optimal

  if (turn) {
    reverse = 1 - reverse; // yeah this is weird but if you're turning you need to reverse the sense of reverse to make left and right turns come out correctly
  }
  if (reverse) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }

#define NUM_QUAD_PHASES 6

  long t = hexmillis() % timeperiod;
  long phase = (NUM_QUAD_PHASES * t) / timeperiod;

  //Serial.print("PHASE: ");
  //Serial.println(phase);

  transactServos();
  setLeg(MIDDLE_LEGS, HIP_NEUTRAL, KNEE_UP_MAX, FBSHIFT_QUAD, 0);

  switch (phase) {
    case 0:
      // in this phase, center-left and noncenter-right legs raise up at
      // the knee
      setLeg(QUAD1_LEGS, NOMOVE, kneeup, FBSHIFT_QUAD, turn);
      // use the middle legs to try to counter balance
      if (kneeup != kneedown) { // if not standing still
        setLeg(MIDDLE_LEGS, reverse ? HIP_BACKWARD_MAX : HIP_FORWARD_MAX, NOMOVE, 0, 1);
      }
      break;

    case 1:
      // in this phase, the center-left and noncenter-right legs move forward
      // at the hips, while the rest of the legs move backward at the hip
      setLeg(QUAD1_LEGS, hipforward, NOMOVE, FBSHIFT_QUAD, turn);
      setLeg(QUAD2_LEGS, hipbackward, NOMOVE, FBSHIFT_QUAD, turn);
      break;

    case 2:
      // now put the first set of legs back down on the ground
      setLeg(QUAD1_LEGS, NOMOVE, kneedown, 0, turn);
      break;

    case 3:
      // lift up the other set of legs at the knee
      setLeg(QUAD2_LEGS, NOMOVE, kneeup, 0, turn);
      if (kneeup != kneedown) {
        setLeg(MIDDLE_LEGS, reverse ? HIP_FORWARD_MAX : HIP_BACKWARD_MAX, NOMOVE, 0, 1);
      }
      break;

    case 4:
      // similar to phase 1, move raised legs forward and lowered legs backward
      setLeg(QUAD1_LEGS, hipbackward, NOMOVE, FBSHIFT_QUAD, turn);
      setLeg(QUAD2_LEGS, hipforward, NOMOVE, FBSHIFT_QUAD, turn);
      break;

    case 5:
      // put the second set of legs down, and the cycle repeats
      setLeg(QUAD2_LEGS, NOMOVE, kneedown, 0, turn);
      break;
  }
  commitServos();
}
void gait_belly(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle) {
  // the gait walks using a rowboat motion while the robot rests on its belly between steps. This code determines what phase
  // we are currently in by using the millis clock modulo the
  // desired time period that all phases should consume.
  // Right now each phase is an equal amount of time but this may not be optimal

  if (turn) {
    reverse = 1 - reverse; // yeah this is weird but if you're turning you need to reverse the sense of reverse to make left and right turns come out correctly
  }
  if (reverse) {
    int tmp = hipforward;
    hipforward = hipbackward;
    hipbackward = tmp;
  }

#define NUM_BELLY_PHASES 4

  long t = hexmillis() % timeperiod;
  long phase = (NUM_BELLY_PHASES * t) / timeperiod;

  //Serial.print("PHASE: ");
  //Serial.println(phase);

  transactServos();

  switch (phase) {
    case 0: // lie down with legs out to the sides
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

void random_gait(int timingfactor) {

#define GATETIME 3500  // number of milliseconds for each demo

  if (millis() > nextGaitTime) {
    curGait++;
    if (curGait >= G_NUMGATES) {
      curGait = 0;
    }
    nextGaitTime = millis() + GATETIME;

    // when switching demo modes, briefly go into a standing position so
    // we're starting at the same position every time.
    setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_STAND, 0);
    delay(600);
  }
  //  Serial.print("curgait: ");
  //  Serial.println(curGait);

  switch (curGait) {
    case G_STAND:
      stand();
      break;
    case G_TURN:
      turn(1, HIP_FORWARD, HIP_BACKWARD, KNEE_NEUTRAL, KNEE_DOWN, TRIPOD_CYCLE_TIME); // 700
      break;
    case G_TRIPOD:
      gait_tripod(1, HIP_FORWARD, HIP_BACKWARD, KNEE_NEUTRAL, KNEE_DOWN, TRIPOD_CYCLE_TIME); // 900
      break;
    case G_SCAMPER:
      gait_tripod_scamper((nextGaitTime - (millis()) < GATETIME / 2), 0); // reverse direction halfway through
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

void flutter() {   // ballet flutter legs on pointe
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

void dance_ballet(int dpad) {   // ballet flutter legs on pointe

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
      // AUTOMATIC MODE
#define NUM_HANDS_PHASES 2
#define HANDS_TIME_PERIOD 400
      { // we need a new scope for this because there are local variables

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
  //setLeg(legs_flat, NOMOVE, KNEE_RELAX, 0, 0);

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

///////////////////////
void transactServos() {
  deferServoSet = 1;
}

void commitServos() {
  checkForCrashingHips();
  deferServoSet = 0;
  int tmp = servoOffset;
  servoOffset = 0;  // want true servo positions not virtual
  for (int servo = 0; servo < 2 * NUM_LEGS + NUM_GRIPSERVOS; servo++) {
    setServo(servo, ServoPos[servo]);
  }
  servoOffset = tmp; // put it back
}


void checkForCrashingHips() {

  int tmp = servoOffset;
  servoOffset = 0; // we want this to operate on actaul servo numbers not remapped numbers
  for (int leg = 0; leg < NUM_LEGS; leg++) {
    if (ServoPos[leg] > 85) {
      continue; // it's not possible to crash into the next leg in line unless the angle is 85 or less
    }
    int nextleg = ((leg + 1) % NUM_LEGS);
    if (ServoPos[nextleg] < 100) {
      continue;   // it's not possible for there to be a crash if the next leg is less than 100 degrees
      // there is a slight assymmetry due to the way the servo shafts are positioned, that's why
      // this number does not match the 85 number above
    }
    int diff = ServoPos[nextleg] - ServoPos[leg];
    // There's a fairly linear relationship
    if (diff <= 85) {
      // if the difference between the two leg positions is less than about 85 then there
      // is not going to be a crash (or maybe just a slight touch that won't really cause issues)
      continue;
    }
    // if we get here then the legs are touching, we will adjust them so that the difference is less than 85
    int adjust = (diff - 85) / 2 + 1; // each leg will get adjusted half the amount needed to avoid the crash

    // to debug crash detection, make the following line #if 1, else make it #if 0
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


///////////////////////////////


// byte FrontReverse = 0;
// long DebounceFrontReverse = 0;

void walk2(int cmd, int submode) {

  switch (submode) {
    case '1': // RIPPLE GAIT

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

    case '2': //BELLY CRAWL GAIT
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
          // stomp in place
          beep(300);
          gait_belly(0, 0, HIP_NEUTRAL, HIP_NEUTRAL, KNEE_BELLY_UP, KNEE_BELLY_DOWN, BELLY_CYCLE_TIME, 0);
          break;
        case 's':
          setLeg(ALL_LEGS, HIP_NEUTRAL, 85, FBSHIFT_BELLY, 0);
          break;
      }
      break;

    case '3':  // Quadruped Gait

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
        case 'w': // stomp and honk
          beep(500);
          gait_quad(1, 1, HIP_NEUTRAL, HIP_NEUTRAL, KNEE_QUAD_UP, KNEE_QUAD_DOWN, QUAD_CYCLE_TIME, 0);
          break;
        case 's':
          gait_quad(1, 1, HIP_NEUTRAL, HIP_NEUTRAL, KNEE_QUAD_DOWN, KNEE_QUAD_DOWN, QUAD_CYCLE_TIME, 0);
          break;
      }
      break;

    case '4': // redefine front gait while scampering

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
        case 'w': // reverse entire sense of the robot legs 180 degrees
          {
            long now = millis(); // do NOT use hexmillis here, we want realtime for debounce type operations
            if (now > DebounceFrontReverse) {
              FrontReverse = 3 - FrontReverse; // it will toggle between 3 and 0
              beep(500 + 300 * FrontReverse, 50); //feedback to user: high pitch means reversed, low pitch means back to normal
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
  // Dance by twitching legs. This code determines what phase
  // we are currently in by using the millis clock modulo the
  // desired time period that all phases should consume.

#define NUM_TWITCH_PHASES 4
#define TWITCH_TIME 300
  int ttime = TWITCH_TIME;

  if (twitchmode == MODESWAY) {
    ttime = 750;
  }
  long t = hexmillis() % ttime;
  long phase = (NUM_TWITCH_PHASES * t) / ttime;

  //Serial.print("PHASE: ");
  //Serial.println(phase);
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
        // hands alternate from knee_neutral to knee_up_max at same time
        setLeg(MIDDLE_LEGS, NOMOVE, KNEE_NEUTRAL, TWITCH_ADJ); break;
      case 'b':
        // no hands at all, all legs down
        break;
      case 'l':
        // hands sway in opposite direction of  grounded legs
        setLeg(MIDDLE_LEGS, NOMOVE, KNEE_UP, TWITCH_ADJ);
        break;
      case 'r':
        // hands pivot at the hips
        setLeg(MIDDLE_LEGS, NOMOVE, KNEE_UP_MAX, TWITCH_ADJ, 1);
        break;
      case 'w':
        // sway both back and forth and with hip swings
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
      } else { //sway
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
      } else { //sway
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
  //randomSeed(millis());

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
  // Dance by making star patterns with legs

#define NUM_STAR_PHASES 4
#define STAR_TIME 700

  long t = hexmillis() % STAR_TIME;
  long phase = (NUM_STAR_PHASES * t) / STAR_TIME;

  //Serial.print("PHASE: ");  Serial.println(phase);
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

    case SUBMODE_1: // TWITCH DANCE

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
        case 'w': // special
          dance_twitch(cmd, NOMOVE, NOMOVE, KNEE_TIPTOES, KNEE_DOWN, MODETWITCH);
          break;
        case 's': // stop mode, just stand tall in twitch configuration
          setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, TWITCH_ADJ);
          break;
      }
      break;

    case SUBMODE_2: // SWAY DANCE

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
        case 'w': // special
          dance_twitch(cmd, NOMOVE, NOMOVE, KNEE_TIPTOES, KNEE_NEUTRAL, MODESWAY);
          break;
        case 's': // stop mode, just stand tall in twitch configuration
          setLeg(ALL_LEGS, HIP_NEUTRAL, KNEE_TIPTOES, TWITCH_ADJ);
          break;
      }
      break;


    case SUBMODE_3:  // STAR DANCE
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

  //Serial.print("PHASE: ");
  //Serial.println(phase);

  transactServos();
  setLeg(BACK_LEGS, HIP_BACKWARD, KNEE_UP, 0);
  setLeg(MIDDLE_LEGS, HIP_FORWARD, KNEE_TIPTOES, 0);

  if (begmode == 's') { // just standing here, nothing to do
    commitServos();
    return;
  }

  switch (phase) {
    case 0:
      switch (begmode) {
        case 'f': // beg for food
          setLeg(LEG0, HIP_FORWARD, KNEE_DOWN, 0);
          setLeg(LEG5, HIP_FORWARD, KNEE_UP, 0);
          break;
        case 'b': // clap
          setLeg(LEG0, HIP_FORWARD + 30, KNEE_UP, 0);
          setLeg(LEG5, HIP_FORWARD + 30, KNEE_UP, 0);
          break;
        case 'w': // sway
          setLeg(LEG0, HIP_FORWARD + 30, KNEE_UP, 0);
          setLeg(LEG5, HIP_FORWARD - 30, KNEE_UP, 0);
          break;
        case 'l':
          setLeg(LEG0, HIP_FORWARD, KNEE_UP_MAX, 0); // queen's wave, side to side
          setLeg(LEG5, HIP_FORWARD, KNEE_DOWN, 0);
          break;
        case 'r':
          setLeg(LEG5, HIP_FORWARD, KNEE_UP_MAX, 0);  // normal wave, up and down
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

  //Serial.print("PHASE: ");
  //Serial.println(phase);

  transactServos();
  setLeg(ALL_LEGS & (~LEG5), HIP_NEUTRAL, KNEE_STAND, 0); // every leg except leg 5

  switch (phase) {
    case 0:
      if (cmd == 'r') {
        setLeg(LEG5, HIP_FORWARD, KNEE_UP_MAX, 0);  // normal wave, up and down
      } else {
        setLeg(LEG5, HIP_FORWARD, KNEE_UP_MAX, 0); // queen's wave, side to side
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
  // move servos toward their targets
#define MOVEINCREMENT 10  // degrees per transmission time delay

  for (int i = 0; i < NUM_LEGS; i++) {
    int h, k;
    h = ServoPos[i];
    k = ServoPos[i + KNEE_OFFSET];
    int diff = ServoTarget[i + KNEE_OFFSET] - k;

    if (diff <= -MOVEINCREMENT) {
      // the knee has a greater value than the target
      k -= MOVEINCREMENT;
    } else if (diff >= MOVEINCREMENT) {
      // the knee has a smaller value than the target
      k += MOVEINCREMENT;
    } else {
      // the knee is within MOVEINCREMENT of the target so just go to target
      k = ServoTarget[i + KNEE_OFFSET];
    }

    setKnee(i, k);

    diff = ServoTarget[i] - h;

    if (diff <= -MOVEINCREMENT) {
      // the hip has a greater value than the target
      h -= MOVEINCREMENT;
    } else if (diff >= MOVEINCREMENT) {
      // the hip has a smaller value than the target
      h += MOVEINCREMENT;
    } else {
      // the knee is within MOVEINCREMENT of the target so just go to target
      h = ServoTarget[i];
    }

    setHipRaw(i, h);
    //Serial.print("RAW "); Serial.print(i); Serial.print(" "); Serial.println(h);

  }
  return;  // /this mode does not execute the rest of the actions
}

void fight2(int cmd, int submode) {
  //Serial.print("F2="); Serial.write(cmd); Serial.write(submode); Serial.println("");

  switch (submode) {

    case SUBMODE_1: // Corgi Mode
      corgi(cmd);
      break;

    case SUBMODE_2: // single arm manipulator mode, right front arm
      switch (cmd) {
        case 's':
          // do nothing in stop mode, just hold current position
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = ServoPos[i + NUM_LEGS];
            ServoTarget[i] = ServoPos[i];
          }
          break;
        case 'w':  // reset to standard standing position, resets both hips and knees
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
      break; // end of submode 2


    case SUBMODE_3:  // single arm manipulator mode, left front arm
      switch (cmd) {
        case 's':
          // do nothing in stop mode, just hold current position
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = ServoPos[i + NUM_LEGS];
            ServoTarget[i] = ServoPos[i];
          }
          break;
        case 'w':  // reset to standard standing position, resets both hips and knees
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
            ServoTarget[i] = 90;
          }
          break;
        case 'f': // raise leg 0
          ServoTarget[0 + KNEE_OFFSET] = 180;
          break;
        case 'b': // lower leg 0
          ServoTarget[0 + KNEE_OFFSET] = 0;
          break;
        case 'l': //move left leg 0
          ServoTarget[0] = 0;
          break;
        case 'r': //twist down
          ServoTarget[0] = 180;
          break;
      }
      break; // end of submode 3

    case SUBMODE_4: // rise/fall mode
      // this mode retains state and moves slowly, it's for getting somethign like the joust or
      // capture the flag accessories in position

      switch (cmd) {
        case 's':
          // do nothing in stop mode, just hold current position
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = ServoPos[i + NUM_LEGS];
            ServoTarget[i] = ServoPos[i];
          }
          break;
        case 'w':  // reset to standard standing position, resets both hips and knees
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i + KNEE_OFFSET] = KNEE_STAND;
            ServoTarget[i] = 90;
          }
          break;
        case 'f': // rise straight up to tiptoes
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i] = 90;
            ServoTarget[i + KNEE_OFFSET] = 0;
          }
          break;
        case 'b': // fall straight down
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i] = 90;
            ServoTarget[i + KNEE_OFFSET] = 180;
          }
          break;
        case 'l': //twist up
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i] = 0;
            ServoTarget[i + KNEE_OFFSET] = 0;
          }
          break;
        case 'r': //twist down
          for (int i = 0; i < NUM_LEGS; i++) {
            ServoTarget[i] = 180;
            ServoTarget[i + KNEE_OFFSET] = 180;
          }
          break;
      }
      break; // end of submode 4
  }
}