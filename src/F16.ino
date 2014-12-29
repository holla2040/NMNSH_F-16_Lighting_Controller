#include "lucky7.h"
#include "RC65X.h"
#include "RMYD065.h"
#include "IRremote.h"
#include <EEPROM.h>
#include <avr/wdt.h>


/* 
  output definitions
  O1 wing, intake 
  O2 taxi
  O5 tail illumination
  O6 tail collision
*/

/*
  see F16 Light Dimming Plot.ods
  pwm = a+b*exp(d*t/e)
*/

#define A 305
#define B -50
#define D 1
#define E 500

/*  there's an interrupt collision with the IR routines and the PWM
  1, 5, 6 work 
  2, 3, 7 doesn't work 
*/

Lucky7 hw = Lucky7();

enum {
    LIGHTTHRESHOLDADDRESSH,
    LIGHTTHRESHOLDADDRESSL
};

enum {
    MODE_OVERRIDE   = 'O',
    MODE_DAY        = 'D',
    MODE_EVENING    = 'E',
    MODE_NIGHT      = 'N',
    MODE_BATTERYLOW = 'B'
};

enum {
    COLL_ON1,
    COLL_OFF1,
    COLL_ON2,
    COLL_OFF2
};

uint16_t collisionCount;



#define TIMEOUTSTATUS          1000
#define TIMEOUTEVENING         14400000L
// 4 hours in milliseconds

#define TIMEOUTOVERRIDE        3600000L
// 1 hour

#define TIMEOUTCOLLISIONON     50 
#define TIMEOUTCOLLISIONSHORT  250 
#define TIMEOUTCOLLISIONLONG   2100
#define TIMEOUTTAXI            300000
#define TIMEOUTBATTERYLOW      100
#define BATTERYLOW 11.5
 
uint32_t timeoutStatus;
uint32_t timeoutOverride;
uint32_t timeoutEvening;
uint32_t timeoutCollision;
uint32_t timeoutTaxi;
uint32_t timeoutBatteryLow;
uint16_t lightThreshold;

uint32_t positionUpdateTime;
uint32_t positionNext;

uint8_t mode;

void allOn() {
    hw.o1On();
    hw.o2On();
    hw.o3On();
    hw.o4On();
    hw.o5On();
    hw.o6On();
    hw.o7On();
}

void allOff() {
    hw.o1Off();
    hw.o2Off();
    hw.o3Off();
    hw.o4Off();
    hw.o5Off();
    hw.o6Off();
    hw.o7Off();
}

void setup() {
    Serial.begin(115200);
    Serial.println("NMNSH F-16 Lighting Controller setup");
    hw.setup();
    timeoutStatus = 0;
    lightThreshold = (EEPROM.read(LIGHTTHRESHOLDADDRESSH) << 8) + EEPROM.read(LIGHTTHRESHOLDADDRESSL);

    positionUpdateTime = 0;
    timeoutOverride = 0;
    timeoutCollision = 0;
    collisionCount = 0;
    timeoutTaxi = 0;
    timeoutBatteryLow = 0;
    mode = MODE_DAY;
}

void processKey(uint32_t key) {
    Serial.print("key ");
    Serial.println(key, HEX);
    switch (key) {
      case '0':
      case RC65X_KEY0:
      case RC65X_KEYDOWN:
      case RM_YD065_KEY0:
      case RM_YD065_KEYDOWN:
          allOff();
          break;
      case '1':
      case RC65X_KEY1:
      case RM_YD065_KEY1:
          hw.o1Toggle();
          break;
      case '2':
      case RC65X_KEY2:
      case RM_YD065_KEY2:
          hw.o2Toggle();
          break;
      case '3':
      case RC65X_KEY3:
      case RM_YD065_KEY3:
          hw.o3Toggle();
          break;
      case '4':
      case RC65X_KEY4:
      case RM_YD065_KEY4:
          hw.o4Toggle();
          break;
      case '5':
      case RC65X_KEY5:
      case RM_YD065_KEY5:
          hw.o5Toggle();
          break;
      case '6':
      case RC65X_KEY6:
      case RM_YD065_KEY6:
          hw.o6Toggle();
          break;
      case '7':
      case RC65X_KEY7:
      case RM_YD065_KEY7:
          hw.o7Toggle();
          break;
      case 'R':
      case RC65X_KEYTVINPUT:
      case RM_YD065_KEYINPUT:
          uint16_t val;
          val = hw.photocell2();
          EEPROM.write(LIGHTTHRESHOLDADDRESSH, (val >> 8));
          EEPROM.write(LIGHTTHRESHOLDADDRESSL, (val & 0xFF));
          lightThreshold = (EEPROM.read(LIGHTTHRESHOLDADDRESSH) << 8) + EEPROM.read(LIGHTTHRESHOLDADDRESSL);
          break;
      case '8':
      case RC65X_KEY8:
      case RC65X_KEYUP:
      case RM_YD065_KEY8:
      case RM_YD065_KEYUP:
          allOn();
          break;

      case 'E':
      case RC65X_KEYPLAY:
      case RC65X_KEYSELECT:

      case RM_YD065_KEYPLAY:
      case RM_YD065_KEYOK:
          Serial.println("PLAY");
          timeoutEvening = millis() + TIMEOUTEVENING;

          hw.o2Off();
          timeoutTaxi = 0;

          mode = MODE_EVENING;
          break;
      case 'N':
      case RC65X_KEYSTOP:
      case RM_YD065_KEYSTOP:
        mode = MODE_NIGHT;
        break;
      }
}



void updateLights() {
    uint32_t now10ms = millis() / 100;
    uint8_t p;

    // if override fast blink status led, otherwise slow blink
    if ((now10ms % ((mode == MODE_OVERRIDE)?3:20)) == 0) {
        hw.o8On();
    } else {
        hw.o8Off();
    }

    if (mode == MODE_EVENING) {
      if ((now10ms % 20) == 0) {
        hw.o13On();
      } else {
        hw.o13Off();
      }
    } else {
      hw.o13Off();
    }

    
    // no need to hammer this
    if (millis() > positionUpdateTime) {
      uint32_t millis1000 = millis() % 1100;

      positionUpdateTime = millis() + 10;
      if (millis1000 > 900) {
          p = 0;
      } else {
        p = A + B * exp(D * float (millis1000) / E);
      }
      switch (mode) {
        case MODE_EVENING:
          hw.o1Set(p); // winginlent
          hw.o5On();   // tail illum

          if (millis() > timeoutCollision) {
            collisionCount++;
            switch (collisionCount % 4) {
                case COLL_ON1:
                    hw.o6On();
                    timeoutCollision = millis() + TIMEOUTCOLLISIONON;
                    break;
                case COLL_OFF1:
                    hw.o6Off();
                    timeoutCollision = millis() + TIMEOUTCOLLISIONSHORT;
                    break;
                case COLL_ON2:
                    hw.o6On();
                    timeoutCollision = millis() + TIMEOUTCOLLISIONON;
                    break;
                case COLL_OFF2:
                    hw.o6Off();
                    timeoutCollision = millis() + TIMEOUTCOLLISIONLONG;
                    break;
            }
          }
          if (millis() > timeoutTaxi) {
            hw.o2Toggle();
            timeoutTaxi = millis() + TIMEOUTTAXI;
          }
          break;
        case MODE_DAY:
          hw.o1Set(p); // winginlent
          hw.o2Off();  // taxi
          hw.o5Off();  // tail illum
          break;
        case MODE_NIGHT:
          allOff();
          break;
        case MODE_BATTERYLOW:
          allOff();
          if (millis() > timeoutBatteryLow) {
            timeoutBatteryLow = millis() + TIMEOUTBATTERYLOW;
            hw.o13Toggle();
          }
      }
    }
}

void statemap() {
  if (hw.batteryVoltage() < BATTERYLOW) {
    mode = MODE_BATTERYLOW;
    return;
  }
  switch (mode) {
    case MODE_OVERRIDE:
      if (millis() > timeoutOverride) {
        mode = MODE_DAY;
      }
      break;
    case MODE_DAY:
        if (hw.photocell2() < lightThreshold) {
            timeoutEvening = millis() + TIMEOUTEVENING;
            mode = MODE_EVENING;

            hw.o2Off();
            timeoutTaxi = 0;
        }
      break;
    case MODE_EVENING:
        if (hw.photocell2() > lightThreshold) {
            mode = MODE_DAY;
            return;
        }
      if (millis() > timeoutEvening) {
          timeoutEvening = 0;
          mode = MODE_NIGHT;
      }
      break;
    case MODE_NIGHT:
        if (hw.photocell2() > lightThreshold) {
            mode = MODE_DAY;
        }
      break;
    case MODE_BATTERYLOW:
      if (hw.batteryVoltage() > BATTERYLOW) {
        mode = MODE_DAY;
      }
      break;
  }
}

void status() {
    char buffer[40];
    if (millis() > timeoutStatus) {
        // Serial.print("\x1B[0;0f\x1B[K"); // home
        // Serial.print("\x1B[0J"); // clear

        Serial.print(millis());

        timeoutStatus = millis() + TIMEOUTSTATUS;
        Serial.print(" lightLevel:");
        Serial.print(hw.photocell2());
        Serial.print(" ");
        Serial.print("lightThreshold:");
        Serial.print(lightThreshold);
        Serial.print(" ");
        Serial.print("mode:");
        Serial.print((char)mode);

        sprintf(buffer," lights:%3d %3d %3d %3d %3d %3d %3d",hw.o1,hw.o2,hw.o3,hw.o4,hw.o5,hw.o6,hw.o7);
        Serial.print(buffer);

        Serial.print(" ");
        Serial.print("volt:");
        Serial.print(hw.batteryVoltage(),2);


        if (mode == MODE_EVENING) {
            Serial.print(" ");
            Serial.print("timeoutEvening:");
            Serial.print(int((timeoutEvening - millis())/1000));
        }
        if (mode == MODE_OVERRIDE) {
            Serial.print(" ");
            Serial.print("timeoutOverride:");
            Serial.print(int((timeoutOverride - millis())/1000));
        }

        Serial.println();
    }

  if (millis() > 2592000000L) {
    wdt_disable();
    wdt_enable(WDTO_15MS);
    while (1) {}
  }
}

void input() {
    unsigned long irKey;
    irKey = hw.loop();
    if (irKey) {
        mode = MODE_OVERRIDE;
        timeoutOverride = millis() + TIMEOUTOVERRIDE;
        processKey(irKey);
    }

    if (Serial.available()) {
        mode = MODE_OVERRIDE;
        timeoutOverride = millis() + TIMEOUTOVERRIDE;
        processKey(Serial.read()); 
    }
}

void loop() {
    input();
    statemap();
    updateLights();
    status();
}
