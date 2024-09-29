/*
This file contains implementation details for the ribbon-in-needle sensor design.

The sensor is contained in a separate box, connected with a CAT5 cable.
The CAT5 cable carries 3.3V/GND, I2C, and heater power.

In the sensor, attached to the I2C bus, are the following
- ADS1115 ADC for sensing the voltage across the thermistors,
  as well as across the 0.1 Ohm shunt resistor for heater
  voltage measurement.
- M24C01 EEPROM for storing calibration data and ID.
- TCA9534 GPIO for controlling the analog mux.

The analog mux CD74HC4067 controls which thermistor
will be measured.


ADS1115 details:
- AIN0 is connected to the common of the analog mux.
- AIN1 is connected to analog ground.
- AIN2 is connected to the positive side of the shunt resistor.
- AIN3 is connected to the negative side of the shunt resistor.
- To measure a thermistor, perform a differential measurement
  across AIN0 to AIN1.
- To measure the shunt resistor, perform a differential measurement
  across AIN2 to AIN3.
- There is a 47n C0G capacitor between AIN0 and GND, 
  as suggested in the ADS1115 datasheet.
- The ADDR pin is connected to some solder jumpers on the
  underside of the board. By default it should be connected
  to GND (marked 0 on the board), for an address of 72.


Thermistor details:
- The 10k thermistors form a voltage divider with 10k fixed
  resistors. The fixed resistor is the top of the voltage divider.
- The voltage divider is driven by a 2.5V reference.


Analog mux details:
- The five thermistors in ribbon 0 are connected to I0-4
  of the analog mux. Ribbon 1 is connected to I8-12.
- The remaining 6 channels on the analog mux are exposed
  on test points on the underside of the board.
- The TCA9534 drives the analog mux. P0 on the TCA9534 goes to
  S0 on the analog mux, and so on through P3->S3.
- The remaining 4 pins of the TCA9534 are exposed on test points
  on the underside of the board.
- The A0-2 pins of the TCA9534 are exposed to solder jumpers
  on the underside of the board. By default it should be 
  connected all 0's, for an address of 32.


EEPROM details:
- The address pins are exposed to solder jumpers on the
  underside of the board. By default they should all be
  soldered to 0, for an address of 80.

*/

#include <Wire.h>
#include <I2C_8Bit.h>
#include <I2C_16Bit.h>

class ThermistorBoard
{
public:
    ThermistorBoard(uint8_t ads_addr, uint8_t tca_addr, uint8_t eeprom_addr)
        : ADS1115_addr(ads_addr), TCA9534_addr(tca_addr), M24C01_addr(eeprom_addr)
    {}

    bool valid() { return ADS1115_addr != 0 && TCA9534_addr != 0 && M24C01_addr != 0; }

    // uint16_t readAdc(int sps, int mux)

    bool setup()
    {
        if (!valid())
        {
            Serial.println("ThermistorBoard is not valid");
            return false;
        }

        // Set all TCA9534 pins to low, output
        I2C_8Bit_writeToModule(TCA9534_addr, 1, 0);
        I2C_8Bit_writeToModule(TCA9534_addr, 3, 0);

        return true;
    }

    void setSps(int sps)
    {
        switch (sps)
        {
        case 8:
            ADS1115_spsBits = 0b000;
            break;
        case 16:
            ADS1115_spsBits = 0b001;
            break;
        case 32:
            ADS1115_spsBits = 0b010;
            break;
        case 64:
            ADS1115_spsBits = 0b011;
            break;
        case 128:
            ADS1115_spsBits = 0b100;
            break;
        case 250:
            ADS1115_spsBits = 0b101;
            break;
        case 475:
            ADS1115_spsBits = 0b110;
            break;
        case 860:
            ADS1115_spsBits = 0b111;
            break;
        default:
            Serial.println("Invalid sps");
            return;
        }
        // Only update if we didn't hit default
        ADS1115_sps = sps;
    }

    float readThermistor(int index)
    {
        if (!valid())
        {
            Serial.println("ThermistorBoard is not valid");
            return false;
        }
        
        if (index < 0 || index > 10)
        {
            Serial.println("Invalid index");
            return 0;
        }
        int probe = index / 5;
        int channel = index % 5;

        selectThermistor(probe, channel);


        uint16_t val = readAdc();

        return adcToTemp(val, 2.048);
    }

    float readShunt()
    {
        if (!valid())
        {
            Serial.println("ThermistorBoard is not valid");
            return false;
        }
        
        // The shunt resistor is 0.1 Ohms. The heater probe is 10 Ohms.
        // If the battery is 12V (probably won't be that high ever), 
        // there would be 1.2A, so the shunt would read 0.12V.
        // We will use FSR 0b101, which is +/- 0.256V.
        float adcFSR = 0.256;

        int32_t val = readAdc(0b011, 0b101);
        if (val & 0x8000)
        {
            val = -0x10000 + val;
        }

        float adcVoltage = val / 32768.0 * adcFSR;
        float current = adcVoltage / 0.1;
        float voltageAcrossHeater = current * 10;
        return voltageAcrossHeater;
    }

private:
    uint8_t ADS1115_addr;
    uint8_t TCA9534_addr;
    uint8_t M24C01_addr;

    int ADS1115_sps;
    int ADS1115_spsBits;

    uint16_t readAdc(int mux = 0, int fsr = 0b010)
    {
        uint16_t configReg = 0;
        configReg |= 1 << 15; // Start a conversion
        configReg |= mux << 12;
        configReg |= fsr << 9; // FSR 0b010 = +/- 2.048V
        configReg |= 1 << 8; // Single-shot
        configReg |= ADS1115_spsBits << 5;
        // Ignore comparator bits

        // Start the conversion
        I2C_16Bit_writeToModule(ADS1115_addr, 1, configReg);
        
        // Wait for completion
        delay(1000 / ADS1115_sps + 1);

        // Poll config register for completion
        bool stillRunning = true;
        while (stillRunning)
        {
            stillRunning = !I2C_16Bit_readFlag(ADS1115_addr, 1, 15);
            if (stillRunning)
            {
                Serial.println("Waiting for conversion");
                delay(1);
            }
        }

        // Read the result
        uint16_t val = I2C_16Bit_readFromModule(ADS1115_addr, 0);
        return val;
    }

    bool selectThermistor(int probe, int channel)
    {
        if (!valid())
        {
            Serial.println("ThermistorBoard is not valid");
            return false;
        }

        if (probe != 0 && probe != 1)
        {
            Serial.println("Invalid probe number");
            return false;
        }
        if (channel < 0 || channel > 4)
        {
            Serial.println("Invalid channel number");
            return false;
        }

        int channel_byte = (probe << 3) | channel;

        I2C_8Bit_writeToModule(TCA9534_addr, 1, channel_byte);

        return true;
    }

    float adcToTemp(uint16_t adcValue, float adcFSR)
    {
        float Vref = 2.5;

        float val;

        if (adcValue & 0x8000)
        {
            val = -0x10000 + (float)adcValue;
        }
        else
        {
            val = adcValue;
        }

        // From the datasheet
        const float resistanceTable[] = {
            20.52,
            15.48,
            11.79,
            9.069,
            9.465,
            5.507,
            4.344,
            3.453,
            2.764,
            2.227,
            1.806,
            1.474,
            1.211,
            1.0,
            0.8309,
            0.6941,
            0.5828,
            0.4916,
            0.4165,
            0.3543,
            0.3027,
            0.2595,
            0.2233,
            0.1929,
            0.1672,
            0.1451,
            0.1261,
            0.1097,
            0.09563,
            0.08357,
            0.07317,
            0.06421,
            0.05650,
            0.04986,
        };
        
        const static int tempTable[] = {
            -40,
            -35,
            -30,
            -25,
            -20,
            -15,
            -10,
             -5,
              0,
              5,
             10,
             15,
             20,
             25,
             30,
             35,
             40,
             45,
             50,
             55,
             60,
             65,
             70,
             75,
             80,
             85,
             90,
             95,
            100,
            105,
            110,
            115,
            120,
            125,
        };

        const int tableSize = sizeof(tempTable) / sizeof(tempTable[0]);

        float adcVoltage = val / 32768.0 * adcFSR;
        // R_T = (V * 10k) / (V_ref - V)
        float R_t = (adcVoltage * 10E3) / (Vref - adcVoltage);
        // Normalize R_t to the scale of the table
        R_t /= 10E3;


        int tableUpperIndex = -1;
        for (int i = 0; i < tableSize; ++i)
        {
            if (resistanceTable[i] < R_t)
            {
                tableUpperIndex = i;
                break;
            }
        }

        if (tableUpperIndex == -1)
        {
            Serial.println("Couldn't find temp in table");
            return 0;
        }

        if (tableUpperIndex == 0)
        {
            return tempTable[0];
        }

        float lowTemp = tempTable[tableUpperIndex - 1];
        float highTemp = tempTable[tableUpperIndex];
        float lowRes = resistanceTable[tableUpperIndex - 1];
        float highRes = resistanceTable[tableUpperIndex];

        return (R_t - lowRes) / (highRes - lowRes) * (highTemp - lowTemp) + lowTemp;
    }
};

bool checkI2cDevice(uint8_t addr)
{
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0)
    {
        return true;
    }
    return false;
}

ThermistorBoard findThermistorBoard()
{
    I2C_8Bit_begin();

    uint8_t ads1115_addr = 0;
    for (uint8_t i = 72; i < 74; ++i)
    {
        if (checkI2cDevice(i))
        {
            ads1115_addr = i;
            Serial.print("Found ADS1115 at address ");
            Serial.println(i);
            break;
        }
    }
    
    uint8_t tca9534_addr = 0;
    for (uint8_t i = 32; i < 40; ++i)
    {
        if (checkI2cDevice(i))
        {
            tca9534_addr = i;
            Serial.print("Found TCA9534 at address ");
            Serial.println(i);
            break;
        }
    }
    
    uint8_t eeprom_addr = 0;
    for (uint8_t i = 80; i < 88; ++i)
    {
        if (checkI2cDevice(i))
        {
            eeprom_addr = i;
            Serial.print("Found EEPROM at address ");
            Serial.println(i);
            break;
        }
    }

    if (ads1115_addr == 0 || tca9534_addr == 0 || eeprom_addr == 0)
    {
        Serial.println("No thermistor board found");
        return ThermistorBoard(0, 0, 0);
    }

    Serial.println("Thermistor board found");
    return ThermistorBoard(ads1115_addr, tca9534_addr, eeprom_addr);
}