#include "errors.h"

#include "leds.h"
#include "misc.h"

#include <Arduino.h>

void setLedsForError(Error error, bool blink)
{
  bool timer = false;
  bool red = false;
  bool yellow = false;
  bool green = false;
  bool featherRed = false;
  bool featherGreen = false;

  switch (error)
  {
  case Error::ADC_MISSING:
    yellow = true;
    break;
  case Error::ADC_TIMED_OUT:
    yellow = true;
    break;
  
  case Error::RTC_MISSING:
    timer = true;
    red = true;
    break;
  case Error::RTC_LOST_POWER:
    timer = true;
    yellow = true;
    break;
  case Error::RTC_NOT_SET:
    timer = true;
    green = true;
    break;

  case Error::SD_CARD_NOT_INSERTED:
    featherGreen = true;
    break;
  case Error::SD_CARD_ERROR:
    featherGreen = true;
    red = true;
    break;
  default:
    break;
  }

  setTimerLed(timer);
  setRedLed(red);
  setYellowLed(yellow);
  setGreenLed(green);
  setFeatherLed(featherRed);
  setFeatherGreenLed(featherGreen);
}



void signalError(Error error, int timeout)
{
  for (int i = 0; i < timeout; ++i)
  {
    setLedsForError(error, true);
    setErrorLed(true);
    delay(250);

    setErrorLed(false);
    delay(250);

    setLedsForError(error, false);
    setErrorLed(true);
    delay(250);

    setErrorLed(false);
    delay(250);
  }
}

void signalErrorAndPowerOff(Error error, int timeout)
{
  signalError(error, timeout);
  keep_on(false);
}

