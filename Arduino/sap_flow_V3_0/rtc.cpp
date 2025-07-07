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

void setRtcTimeFromSerial(int timeout)
{
  // Clear any pending data on serial
  while (Serial.available() > 0)
  {
    Serial.read();
  }

  Serial.println("Enter date-time as YYYY/MM/dd hh:mm:ss");
  char message[32];
  memset(message, 0, sizeof(message));
  unsigned int message_pos = 0;
  bool done = false;

  for (int i = 0; i < timeout * 10; ++i)
  {
    while (Serial.available() > 0)
    {
      char inByte = Serial.read();

      if (inByte == '\n')
      {
        done = true;
        break;
      }
      if (message_pos >= sizeof(message))
      {
        done = true;
        break;
      }

      message[message_pos] = inByte;
      message_pos++;
    }


    if (done)
    {
      break;
    }

    delay(100);
  }

  int year, month, day, hour, minute, second;
  sscanf(message, "%d/%d/%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
  setRtcTime(year, month, day, hour, minute, second);
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

  Serial.print("Setting alarm for ");
  Serial.print(minute);
  Serial.println(" after the hour");

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


