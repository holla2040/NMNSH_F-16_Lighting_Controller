#include "lucky7.h"
#include "RC65X.h"
#include "IRremote.h"

Lucky7 hw = Lucky7();

void setup()
{
  Serial.begin(115200);
  Serial.println("NMNSH F-16 Lighting Controller setup");
  hw.setup();
}

void processKey(uint32_t key) {
  Serial.print(millis());
  Serial.print(" ");
  Serial.println(key,HEX);
  switch(key) {
    case RC65X_KEY1:
      if (hw.o13 == 0) {
        hw.o13On();
      } else {
        hw.o13Off();
      }
  }
}

void modeLoop() {
  uint32_t now = millis()/1000;

  if ((now % 2) == 0) {
    hw.o3Set(10);
  } else {
    hw.o3Off();
  }
}
 

void loop() {
  unsigned long irKey;
  irKey = hw.loop();

  if (irKey) {
    processKey(irKey);
  }
  modeLoop();
}

