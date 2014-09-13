#ifndef LUCKY7_H
#define LUCKY7_H
#include "Arduino.h"

#define OFF 0
#define ON  255
#define O1  11     // PWM
#define O2  10     // PWM
#define O3  9      // PWM
#define O4  7      
#define O5  6      // PWM
#define O6  5      // PWM
#define O7  3      // PWM
#define O8  8
#define O13 13

#define IR  2


class Lucky7
{
private:
  uint32_t timeout;
public:
  uint8_t o1,o2,o3,o4,o5,o6,o7,o8,o13;

  void setup();

  uint32_t loop();

  void o1On()             {o1 = 255;}
  void o1Set(uint8_t v)   {o1 = v;}
  void o1Off()            {o1 = 0;}
  
  void o2On()             {o2 = 255;}
  void o2Set(uint8_t v)   {o2 = v;}
  void o2Off()            {o2 = 0;}

  void o3On()             {o3 = 255;}
  void o3Set(uint8_t v)   {o3 = v;}
  void o3Off()            {o3 = 0;}
  

  void o13On()            {o13 = 255;}
  void o13Set(uint8_t v)  {o13 = v;}
  void o13Off()           {o13 = 0;}

};
#endif


