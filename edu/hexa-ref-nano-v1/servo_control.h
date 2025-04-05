#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

#include "config.h"

// Variables globales
// extern unsigned short ServoPos[];
// extern unsigned short ServoTarget[];
// extern long ServoTime[];
// extern byte ServoTrim[];

// Funciones
void setLeg(int legmask, int hip_pos, int knee_pos, int adj);
void setLeg(int legmask, int hip_pos, int knee_pos, int adj, int raw);
void setLeg(int legmask, int hip_pos, int knee_pos, int adj, int raw, int leanangle);
void setHipRaw(int leg, int pos);
void setHip(int leg, int pos);
void setHip(int leg, int pos, int adj);
void setHipRawAdj(int leg, int pos, int adj);
void setKnee(int leg, int pos);
void setServo(int servo, int pos);



#endif
