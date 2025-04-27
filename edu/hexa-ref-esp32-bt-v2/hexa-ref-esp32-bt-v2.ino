// #include <HID.h>
#include <Arduino.h>
#include <Wire.h>
//#include <Adafruit_PWMServoDriver.h>
// #include <SoftwareSerial.h>
#include <SPI.h>
#include <EEPROM.h>
#define EEPROM_SIZE 64
//#include <Pixy.h>
//#include <Adafruit_PWMServoDriver.h>

#include "config.h"
#include "trim.h"
#include "servo_control.h"
#include "utils.h"
#include "movement.h"
#include "bt.h"
// ------ OR added global variables start: --------------
#define LEDPin 2       // onboard LED pin // OR_OK
#define A0  35         // potentiometer signal, OR_OK
#define A1  32         // potentiometer +5V, OR_OK
#define A2  33         // potentiometer GND, OR_OK

#define A3  34         // analogue GPIO pins for sensors, OR_OK
#define A6  39         // analogue GPIO pins for sensors, OR_OK
#define A7  36         // analogue GPIO pins for sensors, OR_OK

#define SDA_PIN 21
#define SCL_PIN 22

#define ServoTypePin 25        // 5 is used to signal digital vs. analog servo mode // OR_OK 5
#define ServoTypeGroundPin 26  // 6 provides a ground to pull 5 low if digital servos are in use // OR_OK 6


// SoftwareSerial BlueTooth(3, 2); // Bluetooth pins: TX=3=Yellow wire,  RX=2=Green wire



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
  //Serial.println("DETACH");
  for (int i = 0; i < 16; i++) {
    servoDriver.setPin(i, 0, false); // stop pulses which will quickly detach the servo
  }
  ServosDetached = 1;
}

void resetServoDriver() {

  servoDriver.begin();
  servoDriver.setPWMFreq(PWMFREQUENCY);  // Analog servos run at ~60 Hz updates
}

void printTrims() {
  Serial.print("TRIMS: ");
  for (int i = 0; i < NUM_LEGS * 2; i++) {
    Serial.print(ServoTrim[i]); Serial.print(" ");
  }
  Serial.println("");
}
void setup() {
  Serial.begin(115200);
  Serial.println("");
  Serial.println(Version);
  pinMode(BeeperPin, OUTPUT);
  beep(200);

  // Inicialización de I2C con pines específicos
  Wire.begin(SDA_PIN, SCL_PIN);

  EEPROM.begin(EEPROM_SIZE);
  // read in trim values from eeprom if available
  if (EEPROM.read(0) == 'V') {
    // if the first byte in the eeprom is a capital letter V that means there are trim values
    // available. Note that eeprom from the factory is set to all 255 values.
    Serial.print("TRIMS: ");
    for (int i = 0; i < NUM_LEGS * 2; i++) {
      ServoTrim[i] = EEPROM.read(i + 1);
      Serial.print(ServoTrim[i] - TRIM_ZERO); Serial.print(" ");
    }
    Serial.println("");
  } else {
    Serial.println("TRIMS:unset");
    // init trim values to zero, no trim
    for (int i = 0; i < NUM_LEGS * 2; i++) {
      ServoTrim[i] = TRIM_ZERO;   // this is the middle of the trim range and will result in no trim
    }
  }

  // Serial.println("hola 1");

  // make a characteristic flashing pattern to indicate the robot code is loaded (as opposed to the gamepad)
  // There will be a brief flash after hitting the RESET button, then a long flash followed by a short flash.
  // The gamepaid is brief flash on reset, short flash, long flash.
  pinMode(LEDPin, OUTPUT);
  digitalWrite(LEDPin, HIGH);
  delay(300);
  digitalWrite(LEDPin, LOW);
  delay(150);
  digitalWrite(LEDPin, HIGH);
  delay(150);
  digitalWrite(LEDPin, LOW);
  ///////////////////// end of indicator flashing

  Serial.println("hola 2");

  pinMode(A1, OUTPUT);
  pinMode(A2, OUTPUT);
  Serial.println("hola 3");
  pinMode(ServoTypeGroundPin, OUTPUT);    // will provide a ground for shunt on D6 to indicate digital servo mode
  digitalWrite(ServoTypeGroundPin, LOW);
  Serial.println("hola 4");
  pinMode(ServoTypePin, INPUT_PULLUP);    // if high we default to analog servo mode, if pulled to ground
  // (via a shunt to D6) then we'll double the PWM frequency for digital servos
  Serial.println("hola 5");

  digitalWrite(LEDPin, LOW);

  Serial.println("hola 5");
  // A1 and A2 provide power to the potentiometer
  digitalWrite(A1, HIGH);
  digitalWrite(A2, LOW);

  delay(300); // give hardware a chance to come up and stabalize

  /// INICIO BLUETOOTH /////////////////////
  initBLE();



  delay(250);



  if (digitalRead(ServoTypePin) == LOW) { // Analog servo mode
    FreqMult = 3 - FreqMult; // If FreqMult was 1, this makes it 2. If it was 2, this makes it 1.
    // In this way the global default of 1 or 2 will reverse if the shunt
    // is on ServoTypePin
  }

  // Chirp a number of times equal to FreqMult so we confirm what servo mode is in use
  for (int i = 0; i < FreqMult; i++) {
    beep(800, 50);
    delay(100);
  }

  resetServoDriver();
  delay(250);

  Serial.println("hola 4");


  stand();
  setGrip(90, 90);  // neutral grip arm (if installed)

  delay(300);

  //CmuCam5.init();   // we're still working out some issues with CmuCam5

  beep(400); // Signals end of startup sequence

  yield();
}

unsigned int readUltrasonic() {  // returns number of centimeters from ultrasonic rangefinder

  pinMode(ULTRAOUTPUTPIN, OUTPUT);
  digitalWrite(ULTRAOUTPUTPIN, LOW);
  delayMicroseconds(5);
  digitalWrite(ULTRAOUTPUTPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRAOUTPUTPIN, LOW);

  unsigned int duration = pulseIn(ULTRAINPUTPIN, HIGH, 18000);  // maximum 18 milliseconds which would be about 10 feet distance from object

  //Serial.print("ultra cm:"); Serial.println(duration/58);

  if (duration < 100) { // Either 0 means timed out, or less than 2cm is out of range as well
    return 1000;   // we will use this large value to mean out of range, since 400 cm is the manufacturer's max range published
  }
  return (duration) / 58;  // this converts microseconds of sound travel time to centimeters. Remember the sound has to go back and forth
  // so it's traveling twice as far as the object's distance
}

// write out a word, high byte first, and return checksum of two individual bytes
unsigned int
bluewriteword(int w) {
  unsigned int h = highByte(w);
  // BlueTooth.write(h);
  unsigned int l = lowByte(w);
  // BlueTooth.write(l);
  return h + l;
}



// states for processing incoming bluetooth data

// #define P_WAITING_FOR_HEADER      0
// #define P_WAITING_FOR_VERSION     1
// #define P_WAITING_FOR_LENGTH      2
// #define P_READING_DATA            3
// #define P_WAITING_FOR_CHECKSUM    4
// #define P_SIMPLE_WAITING_FOR_DATA 5

// int pulselen = SERVOMIN;

// #define MAXPACKETDATA 48
// unsigned char packetData[MAXPACKETDATA];
// unsigned int packetLength = 0;
// unsigned int packetLengthReceived = 0;
// int packetState = P_WAITING_FOR_HEADER;

// void packetErrorChirp(char c) {
//   beep(70, 8);
//   Serial.print(" BTER:"); Serial.print(packetState); Serial.print(c);
//   //Serial.print("("); Serial.print(c,DEC); Serial.print(")");
//   Serial.print("A");
//   // Serial.println(BlueTooth.available());
//   packetState = P_WAITING_FOR_HEADER; // reset to initial state if any error occurs
// }

byte lastCmd = 's';
byte priorCmd = 0;
byte mode = MODE_WALK; // default
byte submode = SUBMODE_1;     // standard submode.
byte timingfactor = 1;   // default is full speed. If this is greater than 1 it multiplies the cycle time making the robot slower
short priorDialMode = -1;
long NullCount = 0;

int receiveDataHandler() {

  //  while (BlueTooth.available() > 0) {
  //    unsigned int c = BlueTooth.read();

  //    Serial.println(c);
  //    // uncomment the following lines (by making it: #if 1 )if you're doing some serious packet debugging, but be aware this will take up so
  //    // much time you will drop some data. I would suggest slowing the gamepad/scratch sending rate to 4 packets per
  //    // second or slower if you want to use this.
  // #if 0
  //    unsigned long m = millis();
  //    //Serial.print(m);
  //    Serial.print("'"); Serial.write(c); Serial.print("' ("); Serial.print((int)c);
  //    //Serial.print(")S="); Serial.print(packetState); Serial.print(" a="); Serial.print(BlueTooth.available()); Serial.println("");
  //    //Serial.print(m);
  //    Serial.println("");
  // #endif

  //    switch (packetState) {
  //      case P_WAITING_FOR_HEADER:
  //        if (c == 'V') {
  //          packetState = P_WAITING_FOR_VERSION;
  //          //Serial.print("GOT V ");
  //        } else if (c == '@') {  // simplified mode, makes it easier for people to write simple apps to control the robot
  //          packetState = P_SIMPLE_WAITING_FOR_DATA;
  //          packetLengthReceived = 0; // we'll be waiting for exactly 3 bytes like 'D1b' or 'F3s'
  //          //Serial.print("GOT @");
  //        } else {
  //          // may as well flush up to the next header
  //          int flushcount = 0;
  //          while (BlueTooth.available() > 0 && (BlueTooth.peek() != 'V') && (BlueTooth.peek() != '@')) {
  //            BlueTooth.read(); // toss up to next possible header start
  //            flushcount++;
  //          }
  //          Serial.print("F:"); Serial.print(flushcount);
  //          if (c != 0) { // we will not chirp on a null, this allows padding of packet for certain BT modules that don't flush the buffer on their own
  //            packetErrorChirp(c);
  //          } else {
  //            NullCount++;
  //            if (NullCount > 100) {
  //              // if we see this many consecutive nulls we will assume we're in hc05 pad mode
  //              HC05_pad = 1;
  //            }
  //          }
  //        }
  //        break;
  //      case P_WAITING_FOR_VERSION:
  //        if (c == '1') {
  //          packetState = P_WAITING_FOR_LENGTH;
  //          NullCount = 0; // reset null count since we're clearly inside protocol
  //          //Serial.print("GOT 1 ");
  //        } else if (c == 'V') {
  //          // this can actually happen if the checksum was a 'V' and some noise caused a
  //          // flush up to the checksum's V, that V would be consumed by state WAITING FOR HEADER
  //          // leaving the real 'V' header in position 2. To avoid an endless loop of this situation
  //          // we'll simply stay in this state (WAITING FOR VERSION) if we see a 'V' in this state.

  //          // do nothing here
  //        } else {
  //          packetErrorChirp(c);
  //          packetState = P_WAITING_FOR_HEADER; // go back to looking for a 'V' again
  //        }
  //        break;
  //      case P_WAITING_FOR_LENGTH:
  //        { // need scope for local variables
  //          packetLength = c;
  //          if (packetLength > MAXPACKETDATA) {
  //            // this can happen if there's either a bug in the gamepad/scratch code, or if a burst of
  //            // static happened to hit right when the length was being transmitted. In either case, this
  //            // packet is toast so abandon it.
  //            packetErrorChirp(c);
  //            Serial.print("Bad Length="); Serial.println(c);
  //            packetState = P_WAITING_FOR_HEADER;
  //            return 0;
  //          }
  //          packetLengthReceived = 0;
  //          packetState = P_READING_DATA;

  //          //Serial.print("L="); Serial.println(packetLength);
  //        }
  //        break;
  //      case P_READING_DATA:
  //        if (packetLengthReceived >= MAXPACKETDATA) {
  //          // well this should never, ever happen but I'm being paranoid here.
  //          Serial.println("ERROR: PacketDataLen out of bounds!");
  //          packetState = P_WAITING_FOR_HEADER;  // abandon this packet
  //          packetLengthReceived = 0;
  //          return 0;
  //        }
  //        packetData[packetLengthReceived++] = c;
  //        if (packetLengthReceived == packetLength) {
  //          packetState = P_WAITING_FOR_CHECKSUM;
  //        }
  //        //Serial.print("CHAR("); Serial.print(c); Serial.print("/"); Serial.write(c); Serial.println(")");
  //        break;

  //      case P_WAITING_FOR_CHECKSUM:

  //        {
  //          unsigned int sum = packetLength;  // the length byte is part of the checksum
  //          for (unsigned int i = 0; i < packetLength; i++) {
  //            // uncomment the next line if you need to see the packet bytes
  //            //Serial.print(packetData[i]);Serial.print("-");
  //            sum += packetData[i];
  //          }
  //          sum = (sum % 256);

  //          if (sum != c) {
  //            packetErrorChirp(c);
  //            Serial.print("CHECKSUM FAIL "); Serial.print(sum); Serial.print("!="); Serial.print((int)c);
  //            Serial.print(" len="); Serial.println(packetLength);
  //            packetState = P_WAITING_FOR_HEADER;  // giving up on this packet, let's wait for another
  //          } else {
  //            LastValidReceiveTime = millis();  // set the time we received a valid packet
  //            processPacketData();
  //            packetState = P_WAITING_FOR_HEADER;
  //            //dumpPacket();   // comment this line out unless you are debugging packet transmissions
  //            return 1; // new data arrived!
  //          }
  //        }
  //        break;

  //      case P_SIMPLE_WAITING_FOR_DATA:
  //        packetData[packetLengthReceived++] = c;
  //        if (packetLengthReceived == 3) {
  //          // at this point, we're done no matter whether the packet is good or not
  //          // so might as well set the new state right up front
  //          packetState = P_WAITING_FOR_HEADER;

  //          // this simple mode consists of an at-sign followed by three letters that indicate the
  //          // button and mode, such as: @W2f means walk mode two forward. As such, there is no
  //          // checksum, but we can be pretty sure it's valid because there are strong limits on what
  //          // each letter can be. The following large conditional tests these constraints
  //          if ( (packetData[0] != 'W' && packetData[0] != 'D' && packetData[0] != 'F' && packetData[0] != 'X' && packetData[0] != 'Y' && packetData[0] != 'Z') ||
  //               (packetData[1] != '1' && packetData[1] != '2' && packetData[1] != '3' && packetData[1] != '4') ||
  //               (packetData[2] != 'f' && packetData[2] != 'b' && packetData[2] != 'l' && packetData[2] != 'r' &&
  //                packetData[2] != 'w' && packetData[2] != 's')) {

  //            // packet is bad, just toss it.
  //            return 0;
  //          } else {
  //            // we got a good combo of letters in simplified mode
  //            processPacketData();
  //            return 1;
  //          }
  //        }
  //        //Serial.print("CHAR("); Serial.print(c); Serial.print("/"); Serial.write(c); Serial.println(")");
  //        break;
  //    }
  //  }

  // return 0; // no new data arrived
}

unsigned int LastGgaittype;
unsigned int LastGreverse;
unsigned int LastGhipforward;
unsigned int LastGhipbackward;
unsigned int LastGkneeup;
unsigned int LastGkneedown;
unsigned int LastGtimeperiod;
int LastGleanangle;   // this can be negative so don't make it unsigned

void gait_command(int gaittype, int reverse, int hipforward, int hipbackward, int kneeup, int kneedown, int leanangle, int timeperiod) {

  if (ServosDetached) { // wake up any sleeping servos
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
      //gait_sidestep(reverse, timeperiod);
      break;
  }

#if 0
  Serial.print("GAIT: style="); Serial.print(gaittype); Serial.print(" dir="); Serial.print(reverse, DEC); Serial.print(" angles="); Serial.print(hipforward);
  Serial.print("/"); Serial.print(hipbackward); Serial.print("/"); Serial.print(kneeup, DEC); Serial.print("/"); Serial.print(kneedown);
  Serial.print("/"); Serial.println(leanangle);
#endif

  mode = MODE_GAIT;   // this stops auto-repeat of gamepad mode commands
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
        // gamepad mode change
        if (i <= packetLengthReceived - 3) {

          if ( mode != packetData[i] || submode != packetData[i + 1] ) {
            // POSITION SAFETY CODE
            // on any mode/submode change go into stand mode for a short time to ensure legs are all on the ground
            // However, we make an exception going into fight2 (Z) submode 2,3,4 and fight 1 (F) modes 3,4
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
          //Serial.print("GP="); Serial.write(mode);Serial.write(submode);Serial.write(lastCmd);Serial.println("");
          i += 3; // length of mode command is 3 bytes
          continue;
        } else {
          // this is an error, we got a command that was not complete
          // so the safest thing to do is toss the entire packet and give an error
          // beep
          beep(BF_ERROR, BD_MED);
          Serial.println("PKERR:M:Short");
          return;  // stop processing because we can't trust this packet anymore
        }
        break;
      case 'B':   // beep
        if (i <= packetLengthReceived - 5) {
          int honkfreq = word(packetData[i + 1], packetData[i + 2]);
          int honkdur = word(packetData[i + 3], packetData[i + 4]);
          // eventually we should queue beeps so scratch can issue multiple tones
          // to be played over time.
          if (honkfreq > 0 && honkdur > 0) {
            Serial.println("#Beep");
            beep(honkfreq, honkdur);
          }
          i += 5; // length of beep command is 5 bytes
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
          Serial.print("PKERR:B:Short:"); Serial.print(i); Serial.print(":"); Serial.println(packetLengthReceived);
          return;  // toss the rest of the packet
        }
        break;

      case 'R': // Raw Servo Move Command (from Scratch most likely)

#define RAWSERVOPOS 0
#define RAWSERVOADD 1
#define RAWSERVOSUB 2
#define RAWSERVONOMOVE 255
#define RAWSERVODETACH 254
        // Raw servo command is 18 bytes, command R, second byte is type of move, next 16 are all the servo ports positions
        // note: this can move more than just leg servos, it can also access the four ports beyond the legs so
        // you could make active attachments with servo motors, or you could control LED light brightness, etc.
        // move types are: 0=set to position, 1=add to position, 2=subtract from position
        // the 16 bytes of movement data is either a number from 1 to 180 meaning a position, or the
        // number 255 meaning "no move, stay at prior value", or 254 meaning "cut power to servo"
        if (i <= packetLengthReceived - 18) {
          //Serial.println("Got Raw Servo with enough bytes left");
          int movetype = packetData[i + 1];
          //Serial.print(" Movetype="); Serial.println(movetype);
          for (int servo = 0; servo < 16; servo++) {
            int pos = packetData[i + 2 + servo];
            if (pos == RAWSERVONOMOVE) {
              //Serial.print("Port "); Serial.print(servo); Serial.println(" NOMOVE");
              continue;
            }
            if (pos == RAWSERVODETACH) {
              servoDriver.setPin(servo, 0, false); // stop pulses which will quickly detach the servo
              //Serial.print("Port "); Serial.print(servo); Serial.println(" detached");
              continue;
            }
            if (movetype == RAWSERVOADD) {
              pos += ServoPos[servo];
            } else if (movetype == RAWSERVOSUB) {
              pos = ServoPos[servo] - pos;
            }
            pos = constrain(pos, 0, 180);
            //Serial.print("Servo "); Serial.print(servo); Serial.print(" pos "); Serial.println(pos);
            ServoPos[servo] = pos;
          }
          checkForCrashingHips();  // make sure the user didn't do something silly
          for (int servo = 0; servo < 12; servo++) {
            setServo(servo, ServoPos[servo]);
          }
          i += 18; // length of raw servo move is 18 bytes
          mode = MODE_LEG;  // suppress auto-repeat of gamepad commands when this is in progress
          startedStanding = -1; // don't allow sleep mode while this is running
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
          Serial.print("PKERR:R:Short:"); Serial.print(i); Serial.print(":"); Serial.println(packetLengthReceived);
          return;  // toss the rest of the packet
        }
        break;

      case 'G': // Gait command (coming from Scratch most likely). This command is always 10 bytes long
        // params: literal 'G',
        //         Gait type: 0=tripod, 1=turn in place CW from top, 2=ripple, 3=sidestep
        //         reverse direction(0 or 1)
        //         hipforward (angle)
        //         hipbackward (angle),
        //         kneeup (angle)
        //         kneedown(angle)
        //         lean (angle)    make the robot body lean forward or backward during gait, adjusts front and back legs
        //         cycle time (2 byte unsigned long)  length of time a complete gait cycle should take, in milliseconds
        if (i <= packetLengthReceived - 10) {
          LastGgaittype = packetData[i + 1];
          LastGreverse = packetData[i + 2];
          LastGhipforward = packetData[i + 3];
          LastGhipbackward = packetData[i + 4];
          LastGkneeup = packetData[i + 5];
          LastGkneedown = packetData[i + 6];
          int lean = packetData[i + 7];
          LastGtimeperiod = word(packetData[i + 8], packetData[i + 9]);

          LastGleanangle = constrain(lean - 70, -70, 70); // lean comes in from 0 to 60, but we need to bring it down to the range -30 to 30

          gait_command(LastGgaittype, LastGreverse, LastGhipforward, LastGhipbackward, LastGkneeup, LastGkneedown, LastGleanangle, LastGtimeperiod);

          i += 10;  // length of command
          startedStanding = -1; // don't sleep the legs during this command
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
          Serial.println("PKERR:G:Short");
          return;  // toss the rest of the packet
        }
        break;

      case 'L': // leg motion command (coming from Scratch most likely). This command is always 5 bytes long
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
          mode = MODE_LEG;   // this stops auto-repeat of gamepad mode commands
          i += 5;  // length of leg command
          startedStanding = -1; // don't sleep the legs when a specific LEG command was received
          if (ServosDetached) { // wake up any sleeping servos
            attach_all_servos();
          }
          break;
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
          Serial.println("PKERR:L:Short");
          return;  // toss the rest of the packet
        }
        break;

      case 'T': // Trim command

        // The trim command is always just a single operator, either a DPAD button (f, b, l, r, s, w) or the
        // special values S (save), E (erase), P (toggle pose), or R (reset temporarily to untrimmed stance).
        // The meanings are:
        // f    Raise current knee 1 microsecond
        // b    Lower current knee 1 microsecond
        // l    Move current hip clockwise
        // r    Move current hip counterclockwise
        // w    Move to next leg, the leg will twitch to indicate
        // s    Do nothing, just hold steady
        // S    Save all the current trim values
        // P    Toggle the pose between standing and adjust mode
        // R    Show untrimmed stance in the current pose
        // E    Erase all the current trim values
        if (i <= packetLengthReceived - 2) {

          unsigned int command = packetData[i + 1];

          Serial.print("Trim Cmd: "); Serial.write(command); Serial.println("");

          i += 2;  // length of trim command
          startedStanding = -1; // don't sleep the legs when a specific LEG command was received
          mode = MODE_LEG;
          if (ServosDetached) { // wake up any sleeping servos
            attach_all_servos();
          }

          TrimInEffect = 1;   // by default we'll show trims in effect

          // Interpret the command received
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
              delay(500);  // twitch the leg up to give the user feedback on what the new leg being trimmed is
              // this delay also naturally debounces this command a bit
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
              TrimPose = 1 - TrimPose;  // toggle between standing (0) and adjust mode (1)
              beep(500, 30);
              break;
            case 'E':
              erase_trims();
              beep(1500, 1000);
              break;

            default:
            case 's':
              // do nothing.
              break;
          }

          // now go ahead and implement the trim settings to display the result
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
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
          Serial.println("PKERR:T:Short");
          return;  // toss the rest of the packet
        }
        break;

      case 'P': // Pose command (from Scratch) sets all 12 robot leg servos in a single command
        // special value of 255 means no change from prior commands, 254 means power down the servo
        // This command is 13 bytes long: "P" then 12 values to set servo positions, in order from servo 0 to 11

        if (ServosDetached) { // wake up any sleeping servos
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

              //Serial.print("POSE:servo="); Serial.print(servo); Serial.print(":pos="); Serial.println(position);
            } else if (position == 254) {
              // power down this servo
              servoDriver.setPin(servo, 0, false); // stop pulses which will quickly detach the servo
              //Serial.print("POSE:servo="); Serial.print(servo); Serial.println(":DETACHED");
            } else {
              //Serial.print("POSE:servo="); Serial.print(servo); Serial.println(":pos=unchanged");
            }
          }
          checkForCrashingHips();
          for (int servo = 0; servo < 12; servo++) {
            setServo(servo, ServoPos[servo]);
          }

          mode = MODE_LEG;   // this stops auto-repeat of gamepad mode commands

          i += 13;  // length of pose command
          startedStanding = -1; // don't sleep the legs when a specific LEG command was received

          break;
        } else {
          // again, we're short on bytes for this command so something is amiss
          beep(BF_ERROR, BD_MED);
          Serial.println("PKERR:P:Short");
          return;  // toss the rest of the packet
        }
        break;  // I don't think we can actually get here.

      case 'S':   // sensor request
        // CMUCam seems to require it's own power supply so for now we're not doing that, will get it
        // figured out by the time KS shipping starts.
        i++;  // right now this is a single byte command, later we will take options for which sensors to send
        // sendSensorData();
        //////////////// TEMPORARY CODE ////////////////////
        // chirp at most once per second if sending sensor data, this is helpful for debugging
        if (0) {
          unsigned long t = hexmillis() % 1000; // do not use hexmillis here
          if (t < 110) {
            beep(2000, 20);
          }
        }
        ////////////////////////////////////////////////////
        break;
      default:
        Serial.print("PKERR:BadSW:"); Serial.print(packetData[i]);
        Serial.print("i="); Serial.print(i); Serial.print(" RCV="); Serial.println(packetLengthReceived);
        beep(BF_ERROR, BD_MED);
        return;  // something is wrong, so toss the rest of the packet
    }
  }
}



int flash(unsigned long t) {
  // the following code will return HIGH for t milliseconds
  // followed by LOW for t milliseconds. Useful for flashing
  // the LED on pin 13 without blocking
  return (millis() % (2 * t)) > t; // do NOT use hexmillis here, we want real time
}

//
// Short power dips can cause the servo driver to put itself to sleep
// the checkForServoSleep() function uses IIC protocol to ask the servo
// driver if it's asleep. If it is, this function wakes it back up.
// You'll see the robot stutter step for about half a second and a chirp
// is output to indicate what happened.
// This happens more often on low battery conditions. When the battery gets low
// enough, however, this code will not be able to wake it up again.
// If your robot constantly resets even though the battery is fully charged, you
// may have too much friction on the leg hinges, or you may have a bad servo that's
// drawing more power than usual. A bad BEC can also cause the issue.
//
unsigned long freqWatchDog = 0;
unsigned long SuppressScamperUntil = 0;  // if we had to wake up the servos, suppress the power hunger scamper mode for a while

void checkForServoSleep() {

  if (millis() > freqWatchDog) { // do NOT use hexmillis here, we want real time

    // See if the servo driver module went to sleep, probably due to a short power dip
    Wire.beginTransmission(SERVO_IIC_ADDR);
    Wire.write(0);  // address 0 is the MODE1 location of the servo driver, see documentation on the PCA9685 chip for more info
    Wire.endTransmission();
    Wire.requestFrom((uint8_t)SERVO_IIC_ADDR, (uint8_t)1);
    int mode1 = Wire.read();
    if (mode1 & 16) { // the fifth bit up from the bottom is 1 if controller was asleep
      // wake it up!
      resetServoDriver();
      if (beepOnServoReset) {
        beep(1200, 50); // chirp to warn user of brown out on servo controller
        beep(800, 50);
      }
      SuppressScamperUntil = millis() + 10000;  // no scamper for you! (for 10 seconds because we ran out of power, give the battery
      // a bit of time for charge migration and let the servos cool down). Don't use hexmillis here.
    }
    freqWatchDog = millis() + 100; // do NOT use hexmillis here
  }
}

void GeneralCheckSmoothMoves() {
  static long lastSmoothTime = 0;

#define SMOOTHTIME 20 // milliseconds resolution

  long now = millis(); // do NOT use hexmillis here, we need realtime
  if (now >= lastSmoothTime + SMOOTHTIME) {
    lastSmoothTime = now;
  } else {
    return; // not time yet
  }
  for (int servo = 0; servo < NUM_ACTIVE_SERVO; servo++) {
    SmoothMove(servo);
  }
}

void SmoothMove(int servo) {
#define SMOOTHINC  3  // degrees per resolution time
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
//short GlobalState = -1;
void loop() {

  checkForServoSleep();
  checkForCrashingHips();

  ////////////////////
  int p = analogRead(A0);
  int factor = 1;

  /// mapear pote para esp32:
  p /= 3;

  if (p < RANGE_STAND) {
    Dialmode = DIALMODE_STAND;
  } else if (p < RANGE_ADJUST) {
    Dialmode = DIALMODE_ADJUST;
  } else if (p < RANGE_TEST) {
    Dialmode = DIALMODE_TEST;
  } else if (p < RANGE_DEMO) {
    Dialmode = DIALMODE_DEMO;
  } else if (p < RANGE_RC_GRIPARM) {
    Dialmode = DIALMODE_RC_GRIPARM;
  } else {
    Dialmode = DIALMODE_RC;
    //    Serial.println(p);
  }
  //  Serial.println(p);

  //  if (GlobalState != Dialmode) {
  //    GlobalState = Dialmode;
  //    Serial.print("GlobalState: ");
  //    Serial.println(GlobalState);
  //  }

  if (Dialmode != priorDialMode && priorDialMode != -1) {
    Serial.print("MODO: ");
    Serial.print(Dialmode);
    Serial.print(". POT:");
    Serial.println(p);

    if (Dialmode == DIALMODE_TEST) {
      printTrims();
    }

    for (int i = 0; i < Dialmode + 1; i++) {
      beep(100 + 100 * Dialmode * (i + 1), 60); // audio feedback that a new mode has been entered
      delay(100);
    }
    SuppressModesUntil = millis() + 1000; // do NOT use hexmillis here
  }
  priorDialMode = Dialmode;

  if (millis() < SuppressModesUntil) { // do NOT use hexmillis here
    return;
  }

  //Serial.print("Analog0="); Serial.println(p);

  if (Dialmode == DIALMODE_STAND) { // STAND STILL MODE

    digitalWrite(LEDPin, LOW);  // turn off LED13 in stand mode
    //resetServoDriver();
    delay(250);
    stand();
    setGrip(90, 90);  // in stand mode set the grip arms to neutral positions
    // in Stand mode we will also dump out all sensor values once per second to aid in debugging hardware issues
    if (millis() > ReportTime) { // do not use hexmillis here
      ReportTime = millis() + 1000;
      Serial.println("Stand Mode, Sensors:");
      //      Serial.print(" A3="); Serial.print(analogRead(A3));
      //      Serial.print(" A6="); Serial.print(analogRead(A6));
      //      Serial.print(" A7="); Serial.print(analogRead(A7));
      //      Serial.print(" Dist=");
      //      Serial.print(readUltrasonic());
      Serial.println("");
    }

  } else if (Dialmode == DIALMODE_ADJUST) {  // Servo adjust mode, put all servos at 90 degrees

    digitalWrite(LEDPin, flash(100));  // Flash LED13 rapidly in adjust mode
    stand_90_degrees();

    if (millis() > ReportTime) { // do not use hexmillis here
      ReportTime = millis() + 1000;
      Serial.println("AdjustMode");
    }

  } else if (Dialmode == DIALMODE_TEST) {   // Test each servo one by one
    pinMode(LEDPin, flash(500));      // flash LED13 moderately fast in servo test mode
    // Serial.println("Hola 1");

    for (int i = 0; i < 2 * NUM_LEGS + NUM_GRIPSERVOS; i++) {
      // Serial.println("Hola 2");

      p = analogRead(A0);
      p /= 3;

      if (p > RANGE_TEST || p < RANGE_ADJUST) {
        break;
      }
      // Serial.println("Hola 3");

      setServo(i, 140);
      delay(500);
      if (p > RANGE_TEST || p < RANGE_ADJUST) {
        break;
      }
      setServo(i, 40);
      delay(500);
      setServo(i, 90);
      delay(100);
      Serial.print("SERVO: "); Serial.println(i);
    }

  } else if (Dialmode == DIALMODE_DEMO) {  // demo mode

    digitalWrite(LEDPin, flash(2000));  // flash LED13 very slowly in demo mode
    random_gait(timingfactor);
    if (millis() > ReportTime) {  // do not use hexmillis here
      ReportTime = millis() + 1000;
      Serial.println("Demo Mode");
    }
    return;

  } else { // bluetooth mode (regardless of whether it's with or without the grip arm)

    digitalWrite(LEDPin, HIGH);   // LED13 is set to steady on in bluetooth mode
    if (millis() > ReportTime) { // do not use hexmillis here or on following line
      ReportTime = millis() + 2000;
      Serial.print("RC Mode:");
      Serial.print(ServosDetached);
      Serial.write(lastCmd);
      Serial.write(mode);
      Serial.write(submode);
      Serial.println("");
    }
    int gotnewdata = receiveDataHandler();  // handle any new incoming data first
    //Serial.print(gotnewdata); Serial.print(" ");

    // if its been more than 1 second since we got a valid bluetooth command
    // then for safety just stand still.

    if (millis() > LastValidReceiveTime + 1000) { // don't use hexmillis in any of this code
      if (millis() > LastValidReceiveTime + 15000) {
        // after 15 full seconds of not receiving a valid command, reset the bluetooth connection
        //Serial.println("Loss of Signal: resetting bluetooth");
        // Make a three tone chirp to indicate reset
        beep(200, 40); // loss of connection test
        delay(100);
        beep(400, 40);
        delay(100);
        beep(600, 40);
        //        BlueTooth.begin(9600);
        LastReceiveTime = LastValidReceiveTime = millis();
        lastCmd = -1;  // for safety put it in stop mode
      }
      long losstime = millis() - LastValidReceiveTime;
      //      Serial.print("LOS "); Serial.println(losstime);  // LOS stands for "Loss of Signal"
      return;  // don't repeat commands if we haven't seen valid data in a while
    }

    if (gotnewdata == 0) {
      // we didn't receive any new instructions so repeat the last command unless it was binary
      // or unless we're in fight adjust mode
      if (lastCmd == -1) {
        //Serial.println("REP");
        return;
      }


      // The following three mode combinations must retain their state because they
      // use an incremental move strategy. They also all use the smooth moves system
      // to incrementally move the servos toward a target in a controlled fashion that
      // allows the user to stop right where they want to stop. So, trigger the smooth move
      // check and then just return, we don't want to auto-repeat the command.
      if (
        (mode == MODE_FIGHT && (submode == SUBMODE_3 || submode == SUBMODE_4)) ||
        (mode == MODE_FIGHT2 && (submode == SUBMODE_2 || submode == SUBMODE_3 || submode == SUBMODE_4))  ||
        (Dialmode == DIALMODE_RC_GRIPARM && mode == MODE_FIGHT && (submode == SUBMODE_2))
      ) {
        GeneralCheckSmoothMoves();
        return;
      }

    } else {
      LastReceiveTime = millis();    // don't use hexmillis() here, we want real time
    }

    // Leg set mode should also not be repeated

    if (mode == MODE_LEG) {
      //Serial.print("l");
      return;
    } else if (mode == MODE_GAIT) {
      // repeat the last Gait command (from scratch typically)
      gait_command(LastGgaittype, LastGreverse, LastGhipforward, LastGhipbackward,
                   LastGkneeup, LastGkneedown, LastGleanangle, LastGtimeperiod);
      return;
    }
    //
    // Now we're either repeating the last command, or reading the new bluetooth command
    //

    ScamperTracker -= 1;
    if (ScamperTracker < 0) {
      ScamperTracker = 0;
    } else {
      //Serial.println(ScamperTracker);
    }

    if (mode != MODE_WALK2 || submode != SUBMODE_4) {
      FrontReverse = 0; // clear the reverse front feature if we're not in W4W4 mode (double click on W4 walks by redefining the front of the robot for turning)
    }

    switch (lastCmd) {
      case '?':
        //        BlueTooth.println("#Vorpal Hexapod");
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
      case 'w':  // special mode, special depending on major mode. ("w" used to stand for "weapon" but that is outdated)
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
              // stomp in place while beeping horn
              if (submode == SUBMODE_2) { // high step
                factor = 2;
              }
              int cyc = TRIPOD_CYCLE_TIME * factor;
              if (submode == SUBMODE_4) {
                cyc = TRIPOD_CYCLE_TIME / 2; // faster stomp in scamper mode
              }
              gait_tripod(1, 90, 90,
                          KNEE_TRIPOD_UP + factor * KNEE_TRIPOD_ADJ, KNEE_DOWN,
                          cyc);
            }
            break;
          case MODE_WALK2: walk2(lastCmd, submode); break;
          case MODE_DANCE2: dance2(lastCmd, submode); break;
          case MODE_FIGHT2: fight2(lastCmd, submode); break;
          default:     // for any other mode implement a "horn"
            beep(400);
            break;
        }
        break;

      case 'f':  // forward
        startedStanding = -1;
        switch (mode) {
          case MODE_WALK:
            if (submode == SUBMODE_4 && SuppressScamperUntil < millis()) {
              gait_tripod_scamper(0, 0);
            } else {
              if (submode == SUBMODE_2) { // high step
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

      case 'b':  // backward
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

      case 'l': // left
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

      case 'r':  // right
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

      case 's':  // stop and just stand there
        if (startedStanding == -1) {
          startedStanding = millis();
        }
        if (mode == MODE_FIGHT) {
          startedStanding = millis();  // reset in fight mode, never sleep the legs
          fight_mode(lastCmd, submode, 660 * timingfactor);
        } else if (mode == MODE_DANCE && submode == SUBMODE_2) { // ballet
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
            //Serial.print("DET LC=");Serial.write(lastCmd); Serial.println("");
            detach_all_servos();
            return;
          }
          stand();
        }


        break;

      case 'a': // adjust mode
        stand_90_degrees();
        break;

      default:
        Serial.print("BADCHR:"); Serial.write(lastCmd); Serial.println("");
        beep(100, 20);
    }  // end of switch


  }  // end of main if statement



  //   }
}
