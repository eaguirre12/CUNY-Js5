#pragma once

#include <Arduino.h>

bool setupSD(int id);

void appendToSD(const char* str);

void appendToSD(String str);


void writeCsvHeader();
int writeCsvHeaderToBuffer(char* buffer, int size);

void writeCsvRow(
  const char* timestamp,
  float thermistor0,
  float thermistor1,
  float thermistor2,
  float thermistor3,
  float thermistor4,
  float thermistor5,
  const char* measurementPhase,
  float batteryVoltage,
  float heaterCurrent,
  float ambientTemp
);

int writeCsvRowToBuffer(
  const char* timestamp,
  float thermistor0,
  float thermistor1,
  float thermistor2,
  float thermistor3,
  float thermistor4,
  float thermistor5,
  const char* measurementPhase,
  float batteryVoltage,
  float heaterCurrent,
  float ambientTemp,
  char* buffer,
  int size
);

