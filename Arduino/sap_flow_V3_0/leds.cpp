#include "leds.h"

#include <Arduino.h>

#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
#include <Adafruit_NeoPixel.h>
#endif


#define LED_R 10
#define LED_Y 11
#define LED_G 12
#define LED_FEATHER 13
#define LED_POWER 5
#define LED_TIMER 6
#define LED_ERROR 9


#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL);
#endif

#ifdef ADAFRUIT_FEATHER_M0
#define LED_FEATHER_GREEN 8
#endif


void setupLeds()
{
  pinMode(LED_R, OUTPUT);
  pinMode(LED_Y, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_FEATHER, OUTPUT);
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_TIMER, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);

#ifdef ADAFRUIT_FEATHER_M0
  pinMode(LED_FEATHER_GREEN, OUTPUT);
#endif

#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  pixel.begin();
  pixel.clear();
  pixel.show();
#endif
}


void setRedLed(bool on)
{
  digitalWrite(LED_R, on);
}

void setYellowLed(bool on)
{
  digitalWrite(LED_Y, on);
}

void setGreenLed(bool on)
{
  digitalWrite(LED_G, on);
}

void setFeatherLed(bool on)
{
  digitalWrite(LED_FEATHER, on);
}

void setPowerLed(bool on)
{
  digitalWrite(LED_POWER, on);
}

void setTimerLed(bool on)
{
  digitalWrite(LED_TIMER, on);
}

void setErrorLed(bool on)
{
  digitalWrite(LED_ERROR, on);
}


void setNeopixel(int red, int green, int blue)
{
#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  pixel.setPixelColor(0, pixel.Color(red, green, blue));
  pixel.show();
#else
  // Unused
  (void)red;
  (void)green;
  (void)blue;
#endif
}


void setFeatherGreenLed(bool on)
{
#ifdef ADAFRUIT_FEATHER_M0
  digitalWrite(LED_FEATHER_GREEN, on);
#else
  // Use the Neopixel
  if (on)
  {
    setNeopixel(0, 16, 0);
  }
  else
  {
    setNeopixel(0, 0, 0);
  }
#endif
}

