#include "lucky7.h"
#include "RC65X.h"
#include "RMYD065.h"
#include "IRremote.h"
#include <EEPROM.h>

/*
  see F16 Light Dimming Plot.ods
  pwm = a+b*exp(d*t/e)
*/

#define A 275
#define B -40
#define D 1
#define E 500

/*
// dim settings
#define A 150
#define B -40
#define D 1
#define E 800
*/

/*  there's an interrupt collision with the IR routines and the PWM
  1, 2, 5 work 
  3, 6, 7 doesn't work 
*/

enum {
    LIGHTTHRESHOLDADDRESSH,
    LIGHTTHRESHOLDADDRESSL
};

enum {
    MODE_OVERRIDE = 'O',
    MODE_DAY = 'D',
    MODE_EVENING = 'E',
    MODE_NIGHT = 'N'
};

Lucky7 hw = Lucky7();

#define TIMEOUTSTATUS    500
#define TIMEOUTEVENING   240000L
#define TIMEOUTOVERRIDE  30000
uint32_t timeoutStatus;
uint32_t timeoutOverride;
uint32_t timeoutEvening;
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
    mode = MODE_DAY;
}

void processKey(uint32_t key) {
    Serial.print("key ");
    Serial.println(key, HEX);
    switch (key) {
      case '0':
      case RC65X_KEY0:
      case RM_YD065_KEY0:
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
      case RM_YD065_KEY8:
          allOn();
          break;
      case 'E':
      case RC65X_KEYPLAY:
      case RM_YD065_KEYPLAY:
          Serial.println("PLAY");
          timeoutEvening = millis() + TIMEOUTEVENING;
          mode = MODE_EVENING;
          break;
      }
}



void updateLights() {
    uint32_t now = millis() / 100;
    uint8_t p;

    if ((now % 20) == 0) {
        hw.o8On();
    } else {
        hw.o8Off();
    }

    if (mode == MODE_EVENING) {
        if ((now % 20) == 0) {
            hw.o13On();
        } else {
            hw.o13Off();
        }
    } else {
        hw.o13Off();
    }

    // no need to hammer this
    if (millis() > positionUpdateTime) {
      switch (mode) {
        case MODE_DAY:
        case MODE_EVENING:
          p = A + B * exp(D * float ((millis() % 1000)) / E);
          hw.o1Set(p);
          positionUpdateTime = millis() + 10;
          break;
        case MODE_NIGHT:
          allOff();
          break;
      }
    }
}

void statemap() {
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
  }
}

void status() {
    char buffer[40];
    if (millis() > timeoutStatus) {
        // Serial.print("\x1B[0;0f\x1B[K"); // home
        // Serial.print("\x1B[0J"); // clear

        timeoutStatus = millis() + TIMEOUTSTATUS;
        Serial.print("lightLevel:");
        Serial.print(hw.photocell2());
        Serial.print(" ");
        Serial.print("lightThreshold:");
        Serial.print(lightThreshold);
        Serial.print(" ");
        Serial.print("mode:");
        Serial.print((char)mode);

        sprintf(buffer," lights:%3d %3d %3d %3d %3d %3d %3d",hw.o1,hw.o2,hw.o3,hw.o4,hw.o5,hw.o6,hw.o7);
        Serial.print(buffer);


        if (mode == MODE_EVENING) {
            Serial.print(" ");
            Serial.print("timeoutEvening:");
            Serial.print(timeoutEvening - millis());
        }
        if (mode == MODE_OVERRIDE) {
            Serial.print(" ");
            Serial.print("timeoutOverride:");
            Serial.print(timeoutOverride - millis());
        }

        Serial.println();
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
    updateLights();
    statemap();
    status();

/*
    hw.o1MoveTo(255,10);
    hw.o1MoveTo(0,50);
    delay(100);

    hw.o5MoveTo(255,100);
    hw.o5MoveTo(0,2000);
    delay(100);
*/

}

/*
*/
