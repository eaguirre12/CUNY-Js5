
#include <Adafruit_NeoPixel.h>
#include <RTClib.h>


#define LED_R 10
#define LED_Y 11
#define LED_G 12
#define LED_FEATHER 13
#define LED_POWER 5
#define LED_TIMER 6
#define LED_ERROR 9

#define V_BAT_SENSE A0


// TX pin is different between these two boards
#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  #define KEEP_ON D0
#define MUX_S0 14
#define MUX_S1 15
#define MUX_S2 8
#endif

#ifdef ARDUINO_ADAFRUIT_FEATHER_M0_ADALOGGER
  #define KEEP_ON D1
#endif


#define ADC_ADDRESS 0b1001'000
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




Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL);
RTC_DS3231 rtc;


void setup() {

  pinMode(KEEP_ON, OUTPUT);
  digitalWrite(KEEP_ON, 1);



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
  

  pixel.begin();
  pixel.clear();
  pixel.show();

  setup_adc();

}

void loop() {
  // Keep itself on once it starts the loop
  digitalWrite(KEEP_ON, 1);

  float vbat = analogRead(V_BAT_SENSE);
  vbat = vbat / 1023 * 3.3 / (10.0 / 110);
  Serial.print("Battery voltage: ");
  Serial.println(vbat);

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

  // Turn itself off
  digitalWrite(KEEP_ON, 0);
  delay(1000);

}













