
#include "thermistors.h"

#include <Arduino.h>
#include <Adafruit_I2CDevice.h>

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

int adc_gain = 4;

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

  uint8_t pga;
  switch (adc_gain)
  {
    case 1: pga = 0b00; break;
    case 2: pga = 0b01; break;
    case 4: pga = 0b10; break;
    case 8: pga = 0b11; break;
    default: Serial.println("Invalid adc_gain value"); return 0;
  }
  
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

float readThermistorTemp(MuxChannel channel)
{
  int16_t rawValue = read_thermistor((int) channel);
  // Serial.println(rawValue);

  // The raw value is a differential measurement against a reference voltage divider.
  // When the raw value is 0, the thermistor is at its nominal temperature.
  // The thermistors in the East 30 sensor on at the top of the voltage divider.
  // These are NTC thermistors, so when the temperature goes up, the resistance goes down.
  // Therefore, a positive raw value indicates an increase in temperature above nominal.

  // // This is the voltage reference, although I think it should cancel out in the end
  // float refVoltage = 2.7;

  // // This is the voltage of a full-scale positive reading of 32767.
  // // It depends on the adc_gain which determines the pga bit field in read_thermistor().
  // // It would also depend on the data rate, but read_thermistor() multiplies the value to compensate.
  // float voltsFullScale = refVoltage / adc_gain;

  // float measuredVoltage = rawValue * voltsFullScale / 32767 + refVoltage / 2;
  // float thermistorVoltage = refVoltage - measuredVoltage;

  // // float thermistorVoltage = rawValue * voltsFullScale / 32767 + refVoltage / 2;
  // Serial.println(thermistorVoltage);

  // int fixedResistance = 10000;
  // float thermistorResistance = fixedResistance / thermistorVoltage - fixedResistance;
  // float R_T = thermistorResistance;
  // Serial.println(R_T);

  // float logR_T = log(R_T);
  // float c1 = 1.009249522e-03, c2 = 2.378405444e-04, c3 = 2.019202697e-07;
  // float temp = 1.0 / (c1 + c2 * logR_T + c3 * logR_T * logR_T * logR_T);

  // // float temp = 62.57 - R_T * (0.005314) + 0.0000001827 * R_T * R_T - 0.000000000002448 * R_T * R_T * R_T;

  
  float R = 10000;
  float fs = 32767;
  float G = adc_gain;

  float R_T = R * ( 1 / (rawValue / (fs * G) + 0.5) - 1);
  
  float logR_T = log(R_T / R);
  float B = 3977;
  float T0 = 25 + 273.15;
  float temp = 1 / (logR_T / B + 1.0 / T0) - 273.15;

  return temp;
}






