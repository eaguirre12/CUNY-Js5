#pragma once

enum class Error
{
  ADC_MISSING,
  ADC_TIMED_OUT,
  
  RTC_MISSING,
  RTC_LOST_POWER,
  RTC_NOT_SET,

  SD_CARD_NOT_INSERTED,
  SD_CARD_ERROR,

};

void signalError(Error error, int timeout = 30);
void signalErrorAndPowerOff(Error error, int timeout = 30);

