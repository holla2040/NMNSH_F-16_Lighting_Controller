#include "lucky7.h"
#include "IRremote.h"

IRrecv irrecv(IR);
decode_results results;


void Lucky7::setup() {
  o1 = 0;
  o2 = 0;
  o3 = 0;
  o4 = 0;
  o5 = 0;
  o6 = 0;
  o7 = 0;
  o8 = 0;
  o13 = 0;
  pinMode(O1,OUTPUT);
  pinMode(O2,OUTPUT);
  pinMode(O3,OUTPUT);
  pinMode(O4,OUTPUT);
  pinMode(O5,OUTPUT);
  pinMode(O6,OUTPUT);
  pinMode(O7,OUTPUT);
  pinMode(O8,OUTPUT);
  pinMode(O13,OUTPUT);
  timeout = 0;
  irrecv.enableIRIn(); // Start the receiver
}


uint32_t Lucky7::loop() {
  uint32_t rv = 0;
  analogWrite(O1,o1);
  analogWrite(O2,o2);
  analogWrite(O3,o3);
  digitalWrite(O4,o4);
  analogWrite(O5,o5);
  analogWrite(O6,o6);
  analogWrite(O7,o7);
  digitalWrite(O8,o8);
  digitalWrite(O13,o13);

  if (millis() > timeout) {
      if (irrecv.decode(&results)) {
        rv = results.value;
        timeout = millis() + 500;
      }
    } else {
      irrecv.resume(); // Receive the next value
    }
  return rv;
}
