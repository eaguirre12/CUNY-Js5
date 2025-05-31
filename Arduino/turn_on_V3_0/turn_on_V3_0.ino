
#include <RTClib.h>

#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
#include <Adafruit_NeoPixel.h>
#endif


#define LED_R 10
#define LED_Y 11
#define LED_G 12
#define LED_FEATHER 13
#define LED_POWER 5
#define LED_TIMER 6
#define LED_ERROR 9

#define V_BAT_SENSE A0
#define HEATER_SENSE A1

// TX pin is different between these two boards
#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  #define KEEP_ON D0
  #define HEAT D1

  #define MUX_S0 14
  #define MUX_S1 15
  #define MUX_S2 8
#endif

#ifdef ADAFRUIT_FEATHER_M0
  #define KEEP_ON 1
  #define HEAT 0

  #define MUX_S0 SCK
  #define MUX_S1 MOSI
  #define MUX_S2 MISO
#endif


// #define ADC_ADDRESS 0b1001'000
#define ADC_ADDRESS 0x48
Adafruit_I2CDevice adc_i2c(ADC_ADDRESS, &Wire);

bool setup_adc()
{
  if (!adc_i2c.begin())
  {
    Serial.println("Couldn't find ADS1100");
    return false;
  }

  return true;
}

int16_t read_adc(int mux_channel, int settle_delay)
{
  digitalWrite(MUX_S0, mux_channel & 1);
  digitalWrite(MUX_S1, mux_channel & 2);
  digitalWrite(MUX_S2, mux_channel & 4);

  if (settle_delay > 0)
  {
    delay(settle_delay);
  }

  // Data rate also affects full-scale value.
  // uint8_t dr = 0b00; // 128 SPS, +/- 2048
  // uint8_t dr = 0b01; // 32 SPS,  +/- 8192
  // uint8_t dr = 0b10; // 16 SPS,  +/- 16384
  uint8_t dr = 0b11; // 8 SPS,    +/- 32768

  // uint8_t pga = 0b00; // Gain = 1
  // uint8_t pga = 0b01; // Gain = 2
  uint8_t pga = 0b10; // Gain = 4
  // uint8_t pga = 0b11; // Gain = 8
  
  uint8_t config =
      (1 << 7) // Start conversion
    | (1 << 4) // Single conversion
    | (dr << 2)
    | (pga);

  adc_i2c.write(&config, 1);

  if (dr == 0b00) delayMicroseconds(8);
  if (dr == 0b01) delayMicroseconds(32);
  if (dr == 0b10) delayMicroseconds(63);
  if (dr == 0b11) delayMicroseconds(125);

  uint8_t output[3];
  bool done = false;
  for (int i = 0; i < 1000; ++i)
  {
    adc_i2c.read(output, 3);

    done = !(output[2] & (1 << 7));

    if (done)
    {
      break;
    }
    delay(1);
  };
  if (!done)
  {
    Serial.println("ADC never finished");
    return 0;
  }

  int16_t result = (output[0] << 8) | (output[1]);

  // The ADS1100 gives a proportionally smaller value at higher data rates.
  // Scale it back up to full scale.
  if (dr == 0b00) result *= 16;
  if (dr == 0b01) result *= 4;
  if (dr == 0b10) result *= 2;

  return result;
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


int analogRange;

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
  float sense_resistor = 20; // Bodged on 20 Ohm in place of 0.1. Next version will have 1 Ohm.
  float heater_sense = analogReadAverage(HEATER_SENSE, 16);
  float current_mA = heater_sense / 4095.0 * 3.3 / sense_resistor * 1000.0;
  return current_mA;
}


RTC_DS3231 rtc;

#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL);
#endif


void setup() {

  pinMode(KEEP_ON, OUTPUT);
  digitalWrite(KEEP_ON, 1);

  pinMode(HEAT, OUTPUT);
  digitalWrite(HEAT, 0);


  pinMode(LED_R, OUTPUT);
  pinMode(LED_Y, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_FEATHER, OUTPUT);
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_TIMER, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);

  
  pinMode(MUX_S0, OUTPUT);
  digitalWrite(MUX_S0, 0);
  pinMode(MUX_S1, OUTPUT);
  digitalWrite(MUX_S1, 0);
  pinMode(MUX_S2, OUTPUT);
  digitalWrite(MUX_S2, 0);


  Serial.begin(9600); // Value ignored
  

#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  pixel.begin();
  pixel.clear();
  pixel.show();
#endif

  setup_adc();


  analogReadResolution(12);
  analogRange = 1 << 12;

}

void loop() {
  // Keep itself on once it starts the loop
  digitalWrite(KEEP_ON, 1);
  // Delay after enabling KEEP_ON is necessary, or our battery voltage reading is low.
  // Don't know why yet.
  delay(10);

  Serial.print("Battery voltage: ");
  Serial.println(readBatteryVoltage());

  digitalWrite(LED_R, 1);
  delay(500);
  digitalWrite(LED_R, 0);
  delay(500);
  digitalWrite(LED_Y, 1);
  delay(500);
  digitalWrite(LED_Y, 0);
  delay(500);
  digitalWrite(LED_G, 1);
  delay(500);
  digitalWrite(LED_G, 0);
  delay(500);
  digitalWrite(LED_FEATHER, 1);
  delay(500);
  digitalWrite(LED_FEATHER, 0);
  delay(500);
  digitalWrite(LED_POWER, 1);
  delay(500);
  digitalWrite(LED_POWER, 0);
  delay(500);
  digitalWrite(LED_TIMER, 1);
  delay(500);
  digitalWrite(LED_TIMER, 0);
  delay(500);
  digitalWrite(LED_ERROR, 1);
  delay(500);
  digitalWrite(LED_ERROR, 0);
  delay(500);

#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  pixel.setPixelColor(0, pixel.Color(16, 0, 0));
  pixel.show();
  delay(1000);
  pixel.setPixelColor(0, pixel.Color(0, 16, 0));
  pixel.show();
  delay(1000);
  pixel.setPixelColor(0, pixel.Color(0, 0, 16));
  pixel.show();
  delay(1000);
  pixel.clear();
  pixel.show();
#endif



  Serial.println("Thermistors: ");
  for (int channel = 0; channel < 6; ++channel)
  {
    Serial.print(read_adc(channel, 1));
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
  delay(1);


#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  Serial.print("Internal temp: ");
  Serial.print(analogReadTemp());
  Serial.println(" C");
#endif

  // Turn itself off
  digitalWrite(KEEP_ON, 0);
  delay(1000);

  // Wait for alarm
  Serial.print("Waiting for alarm");
  while (!rtc.alarmFired(1))
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();

}











