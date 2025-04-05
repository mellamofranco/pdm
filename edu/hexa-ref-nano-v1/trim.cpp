//#define NUM_LEGS 6
#include "trim.h"
//#include "config.h"
// No incluyas config.h aquí

// Definiciones de variables
byte TrimInEffect = 1;
byte TrimCurLeg = 0;
byte TrimPose = 0;

// Implementación de funciones
void save_trims() {
  //  extern int NUM_LEGS;       // Referencia a la variable definida en otro lugar
  //   extern byte ServoTrim[];   // Referencia a la variable definida en otro lugar

  Serial.print("SAVE TRIMS:");
  for (int i = 0; i < NUM_LEGS * 2; i++) {
    EEPROM.update(i + 1, ServoTrim[i]);
    Serial.print(ServoTrim[i]); Serial.print(" ");
  }
  Serial.println("");
  EEPROM.update(0, 'V');
}

void erase_trims() {
  //  extern int NUM_LEGS;       // Referencia a la variable definida en otro lugar
  //   extern byte ServoTrim[];   // Referencia a la variable definida en otro lugar

  Serial.println("ERASE TRIMS");
  for (int i = 0; i < NUM_LEGS * 2; i++) {
    ServoTrim[i] = TRIM_ZERO;
  }
}
