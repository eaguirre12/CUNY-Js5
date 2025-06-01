#include "rtc.h"

#include <Arduino.h>

#include <RTClib.h>

RTC_DS3231 rtc;

bool setup_rtc()
{
  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    return false;
  }
  
  rtc.disable32K();
  rtc.writeSqwPinMode(DS3231_OFF);
  
  rtc.clearAlarm(2);

  return true;
}

bool rtcLostPower()
{
  return rtc.lostPower();
}

void setRtcTime(int year, int month, int day, int hour, int minute, int second)
{
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
}

String getTimestamp()
{
  char datetime[32] = "YYYY-MM-DD hh:mm:ss";
  rtc.now().toString(datetime);
  return String(datetime);
}

// If minuteMultiple is 30, it will set an alarm to go off at 0 or 30 minutes past the hour, whichever is next.
// If it's 1, it will set the alarm for the next minute.
// If it's 60, it will set the alarm for the top of the hour.
void setAlarm1(int minuteMultiple)
{
  DateTime now = rtc.now();

  int minute = now.minute();
  minute++;
  while (minute % minuteMultiple != 0)
  {
    minute++;
  }
  minute %= 60;

  rtc.setAlarm1(DateTime(0, 0, 0, 0, minute, 0), DS3231_A1_Minute);
}


bool isAlarm1Fired()
{
  return rtc.alarmFired(1);
}

void clearAlarm1()
{
  rtc.clearAlarm(1);
}

void waitForNextSecond()
{
  DateTime startTime = rtc.now();
  while (rtc.now() == startTime) 
  {
    delay(1);
  }
}

float rtcTemperature()
{
  return rtc.getTemperature();
}


