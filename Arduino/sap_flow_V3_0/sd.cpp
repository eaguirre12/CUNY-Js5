#include "sd.h"

#include <Arduino.h>

#include <SD.h>



#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  #define SD_CS 23
  #define SD_CLK 18
  #define SD_MOSI 19
  #define SD_MISO 20
  #define SD_CARD_DETECT 16
#endif

#ifdef ADAFRUIT_FEATHER_M0
  #define SD_CS 4
  #define SD_CARD_DETECT 7
#endif

char filename[32];

bool setupSD(int id)
{

  char name[] = "testNNN.txt";
  name[4] = '0' + (id / 100) % 10;
  name[5] = '0' + (id / 10) % 10;
  name[6] = '0' + id % 10;
  strcpy(filename, name);
  Serial.printf("filename: %s\n", filename);


  pinMode(SD_CARD_DETECT, INPUT_PULLUP);

  if (digitalRead(SD_CARD_DETECT))
  {
    Serial.println("SD card inserted");
  }
  else
  {
    Serial.println("SD card not inserted");
  }
#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  auto result = SD.begin(SD_CS, SPI1);
#endif
#ifdef ADAFRUIT_FEATHER_M0
  auto result = SD.begin(SD_CS);
#endif

  if (!result)
  {
    Serial.println("SD card not started");
    return false;
  }
  else
  {
    Serial.println("SD card started");
    return true;
  }
}

void appendToSD(const char* str)
{

  File f = SD.open(filename, FILE_WRITE);
  f.println(str);
  f.close();
}

void appendToSD(String str)
{
  appendToSD(str.c_str());
}


void writeCsvHeader()
{
  const char* text = "Datetime, Temp0, Temp1, Temp2, Temp3, Temp4, Temp5, Phase, Battery Voltage, Heater Current, Ambient Temp";
  appendToSD(text);
  Serial.println(text);
}

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
)
{
  String line(timestamp);
  line += String(", ");
  line += String(thermistor0, 3) + ", ";
  line += String(thermistor1, 3) + ", ";
  line += String(thermistor2, 3) + ", ";
  line += String(thermistor3, 3) + ", ";
  line += String(thermistor4, 3) + ", ";
  line += String(thermistor5, 3) + ", ";
  line += String(measurementPhase) + ", ";
  line += String(batteryVoltage, 3) + ", ";
  line += String(heaterCurrent, 3) + ", ";
  line += String(ambientTemp, 1);
  appendToSD(line);
  Serial.println(line);
}


