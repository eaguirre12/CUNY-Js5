
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


bool setupSD()
{
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

void appendToSD(const char* str, int id)
{
  char filename[] = "testNNN.txt";
  filename[4] = '0' + (id / 100) % 10;
  filename[5] = '0' + (id / 10) % 10;
  filename[6] = '0' + id % 10;
  Serial.printf("filename: %s\n", filename);

  File f = SD.open(filename, FILE_WRITE);
  f.println(str);
  f.close();
}



