
#include <RTClib.h>
#include <SD.h>


#define V_BAT_SENSE A0
#define HEATER_SENSE A1
#define SW_AUX A2


// TX pin is different between these two boards
#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  #define KEEP_ON D0
  #define HEAT D1
  
  #define SD_CS 23
  #define SD_CLK 18
  #define SD_MOSI 19
  #define SD_MISO 20
  #define SD_CARD_DETECT 16
  
  // The RP2040 doesn't have A4 and A5, so for now just read A3 for all three
  #define ID_0 A3
  #define ID_1 A3
  #define ID_2 A3
#endif

#ifdef ADAFRUIT_FEATHER_M0
  #define KEEP_ON 1
  #define HEAT 0
  
  #define SD_CS 4
  #define SD_CARD_DETECT 7
  
  #define ID_0 A5
  #define ID_1 A4
  #define ID_2 A3
#endif


bool setupSD()
{
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


float analogReadAverage(int pin, int n)
{
  uint32_t result = 0;
  for (int i = 0; i < n; ++i)
  {
    result += analogRead(pin);
  }
  return result / (float)n;
}


int analogRange = 1 << 10;

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
  float current_mA = heater_sense / 4095.0 * 3.3 / sense_resistor * 1000.0;
  return current_mA;
}



RTC_DS3231 rtc;



int id = 0;

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

void readId()
{
  id = 0;
  id += analogToDigit(ID_0);
  id += analogToDigit(ID_1) * 10;
  id += analogToDigit(ID_2) * 100;
  Serial.print("Read ID as ");
  Serial.println(id);
}


void setup() {

  pinMode(KEEP_ON, OUTPUT);
  digitalWrite(KEEP_ON, 1);

  pinMode(HEAT, OUTPUT);
  digitalWrite(HEAT, 0);

  setupLeds();

  Serial.begin(9600); // Value ignored

  setup_thermistors();


  analogReadResolution(12);
  analogRange = 1 << 12;

  pinMode(SW_AUX, INPUT_PULLUP);

  pinMode(SD_CARD_DETECT, INPUT_PULLUP);

  readId();
}

void loop() {
  // Keep itself on once it starts the loop
  digitalWrite(KEEP_ON, 1);
  // Delay after enabling KEEP_ON is necessary, or our battery voltage reading is low.
  // Don't know why yet.
  delay(10);

  Serial.print("Battery voltage: ");
  Serial.println(readBatteryVoltage());

  setRedLed(1);
  delay(500);
  setRedLed(0);
  delay(500);
  setYellowLed(1);
  delay(500);
  setYellowLed(0);
  delay(500);
  setGreenLed(1);
  delay(500);
  setGreenLed(0);
  delay(500);
  setFeatherLed(1);
  delay(500);
  setFeatherLed(0);
  delay(500);
  setPowerLed(1);
  delay(500);
  setPowerLed(0);
  delay(500);
  setTimerLed(1);
  delay(500);
  setTimerLed(0);
  delay(500);
  setErrorLed(1);
  delay(500);
  setErrorLed(0);
  delay(500);

  setFeatherGreenLed(1);
  delay(500);
  setFeatherGreenLed(0);
  delay(500);


  Serial.println("Thermistors: ");
  for (int channel = 0; channel < 6; ++channel)
  {
    Serial.print(read_thermistor(channel));
    Serial.print("\t");
  }
  Serial.println();

  if (rtc.begin())
  {
    // Serial.println("Found RTC");

    if (rtc.lostPower())
    {
      Serial.println("RTC lost power");
      rtc.adjust(DateTime(2025, 5, 28, 14, 15, 0));
    }

    char datetime[32] = "YYYY-MM-DD hh:mm:ss";
    rtc.now().toString(datetime);

    Serial.print("Current time: ");
    Serial.println(datetime);

    rtc.disable32K();
    rtc.writeSqwPinMode(DS3231_OFF);

    rtc.clearAlarm(1);
    rtc.clearAlarm(2);

    rtc.setAlarm1(DateTime(0, 0, 0, 0, 0, 0), DS3231_A1_Second);
  }
  else
  {
    Serial.println("Couldn't find RTC");
  }



  Serial.print("Heater current while off: ");
  Serial.print(readHeaterCurrent());
  Serial.println(" mA");

  int heatSeconds = 5;
  Serial.print("Turning heater on for ");
  Serial.print(heatSeconds);
  Serial.println(" seconds");
  digitalWrite(HEAT, 1);
  delay(heatSeconds * 1000);
  float heaterCurrent = readHeaterCurrent();
  float batteryVoltage = readBatteryVoltage();
  digitalWrite(HEAT, 0);
  Serial.println("Heater off");
  Serial.print("Heater current: ");
  Serial.print(heaterCurrent);
  Serial.println(" mA");
  Serial.print("Battery voltage while heater is on: ");
  Serial.println(batteryVoltage);


#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  delay(1);
  Serial.print("Internal temp: ");
  Serial.print(analogReadTemp());
  Serial.println(" C");
#endif

  readId();

  if (setupSD())
  {
    Serial.println("Writing to SD");
    appendToSD("Hello, world!", id);
#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
    appendToSD("From RP2040", id);
#endif
#ifdef ADAFRUIT_FEATHER_M0
    appendToSD("From M0", id);
#endif
    
    char datetime[32] = "YYYY-MM-DD hh:mm:ss";
    rtc.now().toString(datetime);
    appendToSD(datetime, id);
  }


  if (!digitalRead(SW_AUX))
  {
    Serial.println("Aux switch pressed, running another cycle immediately");
  }
  else
  {
    Serial.println("Aux switch not pressed, going to sleep");
    
    // Turn itself off
    digitalWrite(KEEP_ON, 0);
    delay(1000);

    // Wait for alarm
    Serial.print("Waiting for alarm");
    while (!rtc.alarmFired(1))
    {
      if (!digitalRead(SW_AUX))
      {
        Serial.println("Aux switch pressed, running another cycle immediately");
        break;
      }

      delay(1000);
      Serial.print(".");
    }
    Serial.println();
  }

}











