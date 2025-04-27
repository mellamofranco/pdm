#ifndef TRIM_H
#define TRIM_H

#include <Arduino.h>
#include <EEPROM.h>
// No incluyas config.h aquí para evitar definiciones circulares
#include "config.h"

// Definiciones
//#define TRIM_ZERO 127

// Usa extern para declarar variables definidas en otro lugar
extern byte TrimInEffect;
extern byte TrimCurLeg;
extern byte TrimPose;
//extern byte ServoTrim[];

// Prototipos de funciones
void save_trims();
void erase_trims();

#endif
