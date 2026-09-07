// Copied from MisBkit_firmware/src/Led.cpp (Etienne Montenegro)
// Adapted: ArduinoLog replaced by Serial.printf; logic unchanged.
#include "Led.h"
#include "Pins.h"

#define DEFAULT_BRIGHTNESS 20

namespace led {
  uint8_t _r = 0;
  uint8_t _g = 0;
  uint8_t _b = 0;
  float _brightness = 0.1;
  float _max_brightness = 0.2;
  bool show = false;

void init() {
  pinMode(38, OUTPUT);
  digitalWrite(38, HIGH);

  setColor(255,255,255,true);
  delay(1000);
}

void off(){
    setColor(0,0,0, true);
    Serial.println("[Led] Off");
}

void error(){
    setColor(255,0,0, true);
    Serial.println("[Led] Error");
}

void setColor( uint8_t r, uint8_t g, uint8_t b, bool force) {
  _r = r;
  _g = g;
  _b = b;
  uint8_t rOut = r*_brightness;
  uint8_t gOut = g*_brightness;
  uint8_t bOut = b*_brightness;

  if(force == true){
    neopixelWrite(pins::led, rOut, gOut, bOut);
    show = false;
  }else{
    show = true;
  }
}

//This function is blocking
void blinkColor( uint8_t r, uint8_t g, uint8_t b, int time, int count) {
  bool state{true};
  unsigned long ln = 0;
  int c = 0;
  while (c < count) {
    unsigned long n = millis();
    if ((n - ln )> time) {
      ln = n;
      c++;
      state = !state;
      if(state){
        setColor(r,g,b, true);
      }else{
        off();
      }
    }
    yield();
  }

  off();
}

void update() {
    static unsigned long ln = 0;
    unsigned long n = millis();

    if(n-ln > 20){
        ln = n;
        int i = (n % 200);
       _brightness = (float(i)/200.0) * _max_brightness;
        setColor(_r,_g,_b,true);
        show = false;
    }
}

//This function is blocking
void blinkDigit(uint8_t digit) {
  Serial.printf("[Led] blinkDigit digit: %d\n", digit);
  if( digit == 0 )
  {
    setColor(255,255,255, true);
    delay(160*3);
    off();
    delay(160);
    return;
  }
  for (uint8_t i = 0; i < digit; i++) {
      setColor(255,255,255, true);
      delay(160);
      off();
      delay(160);
  }
}

//This function is blocking
void blinkNumber(uint16_t number) {
  Serial.printf("[Led] blinkNumber number: %d\n", number);
  if (number > 999) number = 999; // limit to 3 digits

  uint8_t hundreds = number / 100;
  uint8_t tens     = (number / 10) % 10;
  uint8_t units    = number % 10;

  const int pause_between_digits = 800;

  // Hundreds
  if (hundreds > 0) {
      blinkDigit(hundreds);
      delay(pause_between_digits);
  }

  // Tens
  if (tens > 0 || hundreds > 0) {
      blinkDigit(tens);
      delay(pause_between_digits);
  }

  // Units
  blinkDigit(units);
}

//This function is blocking
void blinkIP(IPAddress ip) {
  Serial.printf("[Led] IP: %d.%d.%d.%d\n", ip[0], ip[1], ip[2], ip[3]);
  uint8_t previous[3] = {_r,_g,_b};

  // turn off led once, to help read
  off();
  delay(800);

  int val = ip[3];
  blinkNumber(val);

  delay(2000);
  setColor(previous[0], previous[1], previous[2], true);
}
}  // namespace led
