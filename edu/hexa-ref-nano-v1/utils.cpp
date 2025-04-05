#include "utils.h"
#include "config.h"

unsigned long hexmillis() {  // millis that takes into account hexapod size for leg timings
  unsigned long m = (millis() * TIMEFACTOR) / 10L;
  return m;
}

void beep(int f, int t) {
  if (f > 0 && t > 0) {
    tone(BeeperPin, f, t);
  } else {
    noTone(BeeperPin);
  }
//  Serial.println(f);
}

void beep(int f) {  // if no second param is given we'll default to 250 milliseconds for the beep
  beep(f, 250);
}

// void beep(int f, int t) {
//   if (f > 0 && t > 0) {
//     tone(BeeperPin, f, t);
//   } else {
//     noTone(BeeperPin);
//   }
// }

// void beep(int f) {  // if no second param is given we'll default to 250 milliseconds for the beep
//   beep(f, 250);
// } 