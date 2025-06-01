
#define V_BAT_SENSE A0
#define HEATER_SENSE A1


#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  // The RP2040 doesn't have A4 and A5, so for now just read A3 for all three
  #define ID_0 A3
  #define ID_1 A3
  #define ID_2 A3
#endif

#ifdef ADAFRUIT_FEATHER_M0
  #define ID_0 A5
  #define ID_1 A4
  #define ID_2 A3
#endif


int analogRange = 1 << 10;

void setupAdc()
{
  analogReadResolution(12);
  analogRange = 1 << 12;

}


float analogReadAverage(int pin, int n)
{
  uint32_t result = 0;
  for (int i = 0; i < n; ++i)
  {
    result += analogRead(pin);
  }
  return result / (float)n;
}



float readBatteryVoltage()
{
  float vbat = analogReadAverage(V_BAT_SENSE, 16);
  vbat /= analogRange;
  vbat *= 3.3;
  vbat /= (10.0 / 110.0);
  return vbat;
}

float readHeaterCurrent()
{
  float sense_resistor = 1;
  float heater_sense = analogReadAverage(HEATER_SENSE, 16);
  float current_mA = heater_sense / analogRange * 3.3 / sense_resistor * 1000.0;
  return current_mA;
}


int analogToDigit(int pin)
{
  pinMode(pin, INPUT);
  auto val = analogRead(pin);
  float increment = analogRange / 10.0;
  float grayArea = analogRange / 80.0;
  for (int i = 0; i <= 9; ++i)
  {
    float threshold = (i + 1) * increment;
    float lowerGrayArea = i * increment + grayArea;
    float upperGrayArea = threshold - grayArea;
    if (val < threshold)
    {
      if (i > 0 && val < lowerGrayArea)
      {
        Serial.printf("Warning: ID pin %i is in the lower gray area. ", pin);
        Serial.printf("val = %i, threshold = %i, lowerGrayArea = %i\n", val, (int)threshold, (int)lowerGrayArea);
      }
      if (i < 9 && val > upperGrayArea)
      {
        Serial.printf("Warning: ID pin %i is in the upper gray area. ", pin);
        Serial.printf("val = %i, threshold = %i, upperGrayArea = %i\n", val, (int)threshold, (int)upperGrayArea);
      }
      return i;
    }
  }
  return 9;
}

int readId()
{
  int id = 0;
  id += analogToDigit(ID_0);
  id += analogToDigit(ID_1) * 10;
  id += analogToDigit(ID_2) * 100;
  return id;
}


