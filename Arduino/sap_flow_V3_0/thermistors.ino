
#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  #define MUX_S0 14
  #define MUX_S1 15
  #define MUX_S2 8
#endif

#ifdef ADAFRUIT_FEATHER_M0
  #define MUX_S0 SCK
  #define MUX_S1 MOSI
  #define MUX_S2 MISO
#endif

// #define ADC_ADDRESS 0b1001'000
#define ADC_ADDRESS 0x48
Adafruit_I2CDevice adc_i2c(ADC_ADDRESS, &Wire);

// The time (ms) to wait after switching the mux before reading the ADC
#define SETTLE_DELAY 5

bool setup_thermistors()
{
  pinMode(MUX_S0, OUTPUT);
  digitalWrite(MUX_S0, 0);
  pinMode(MUX_S1, OUTPUT);
  digitalWrite(MUX_S1, 0);
  pinMode(MUX_S2, OUTPUT);
  digitalWrite(MUX_S2, 0);

  if (!adc_i2c.begin())
  {
    Serial.println("Couldn't find ADS1100");
    return false;
  }

  return true;
}

int16_t read_thermistor(int mux_channel)
{
  digitalWrite(MUX_S0, mux_channel & 1);
  digitalWrite(MUX_S1, mux_channel & 2);
  digitalWrite(MUX_S2, mux_channel & 4);

  delay(SETTLE_DELAY);

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
    delayMicroseconds(1);
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

