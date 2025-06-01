#pragma once

#include <Arduino.h>

bool setup_rtc();

bool rtcLostPower();

void setRtcTime(int year, int month, int day, int hour, int minute, int second);

String getTimestamp();

// If minuteMultiple is 30, it will set an alarm to go off at 0 or 30 minutes past the hour, whichever is next.
// If it's 1, it will set the alarm for the next minute.
// If it's 60, it will set the alarm for the top of the hour.
void setAlarm1(int minuteMultiple);

bool isAlarm1Fired();

void clearAlarm1();

void waitForNextSecond();

float rtcTemperature();
