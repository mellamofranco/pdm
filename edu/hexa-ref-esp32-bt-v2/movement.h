#ifndef MOVEMENT_H
#define MOVEMENT_H

#include "config.h"


// Funciones de movimiento
void setGrip(int elbow, int claw) ;
void turn(int ccw, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod);
void turn(int ccw, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle);
void stand();
void stand_90_degrees() ;

void laydown();
void tiptoes() ;
void wave(int dpad);
void griparm_mode(char dpad);
void fight_mode(char dpad, int mode, long timeperiod) ;


void gait_tripod(int reverse, int hipforward, int hipbackward,
                 int kneeup, int kneedown, long timeperiod);
void gait_tripod(int reverse, int hipforward, int hipbackward,
                 int kneeup, int kneedown, long timeperiod, int leanangle);
void gait_tripod_scamper(int reverse, int turn);
void gait_ripple(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod);
void gait_ripple(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle);
void gait_quad(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle);
void gait_belly(int turn, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, long timeperiod, int leanangle);
void random_gait(int timingfactor);
void foldup();
void dance_dab(int timingfactor);
void flutter();
void dance_ballet(int dpad) ;
void dance_hands(int dpad) ;
void dance(int legs_up, int submode, int timingfactor);
void boogie_woogie(int legs_flat, int submode, int timingfactor);


/// otros:
void transactServos();
void commitServos();

void checkForCrashingHips();

///////////////

void walk2(int cmd, int submode);
void dance_twitch(int cmd, int hipforward, int hipbackward, int kneeup, int kneedown, int twitchmode);
void randomizeLeg(int legnum);
void dance_brownian(int cmd);
void dance_star(int cmd);
void dance2(int cmd, int submode);
void corgi(int begmode) ;
void wave_hello(int cmd) ;
void incremental_move();
void fight2(int cmd, int submode);


#endif // MOVEMENT_H 