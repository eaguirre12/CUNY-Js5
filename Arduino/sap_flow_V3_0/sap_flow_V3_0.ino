
#include "adc.h"
#include "leds.h"
#include "misc.h"
#include "rtc.h"
#include "sd.h"
#include "thermistors.h"

void setup() {

  Serial.begin(9600); // Value ignored

  setup_misc();
  keep_on(true);
  heater(false);

  setupLeds();
  setup_thermistors();
  setupAdc();
  setup_rtc();
}



/// T is the period between measurement events
// This should evenly divide an hour, because we set the alarm to the next multiple.
#define T_MINS 30
/// PREH is the measurement period before heat on
#define PREH_SECS 20
/// H is the period to apply heat
#define H_SECS 2
/// POTSTH is the measurement period after heat
#define POSTH_SECS 120

// ......|______|----|_____________|...................... |____|----|_____________|
//         PREH   H       POSTH
//.......|............................T....................|....................
/// note: T > PREH + H + POSTH > TS


// states (periods) in cycle
enum class HeatingState
{
  PREHEAT,
  HEAT,
  POSTHEAT,
  END,
};


int measure(HeatingState heatingState, char* buffer, int size);

char buffer[16384];

void measurementCycle()
{
  int size = 16383;
  int offset = 0;
  memset(buffer, 0, size);

  offset += writeCsvHeaderToBuffer(buffer + offset, size);

  HeatingState heatingState = HeatingState::PREHEAT;
  
  // Delay execution until the clock rolls over to the next second to align execution.
  waitForNextSecond();

  const int fullCycleSeconds = PREH_SECS + H_SECS + POSTH_SECS;
  for (int i = 0; i < fullCycleSeconds; ++i)
  {
    setTimerLed(1);

    if (i < PREH_SECS)
    {
      // Spend this second in preheat
      heatingState = HeatingState::PREHEAT;
      setRedLed(1);
      setYellowLed(0);
      setGreenLed(0);
    }
    else if (i < PREH_SECS + H_SECS)
    {
      // Spend this second in heat
      heatingState = HeatingState::HEAT;
      setRedLed(0);
      setYellowLed(1);
      setGreenLed(0);
    }
    else
    {
      // Spend this second in postheat
      heatingState = HeatingState::POSTHEAT;
      setRedLed(0);
      setYellowLed(0);
      setGreenLed(1);
    }

    if (heatingState == HeatingState::HEAT)
    {
      heater(1);
    }
    else
    {
      heater(0);
    }

    offset += measure(heatingState, buffer + offset, size);

    setTimerLed(0);

    waitForNextSecond();
  }
  
  setRedLed(0);
  setYellowLed(0);
  setGreenLed(0);

  // Re-initialize the SD and write what we buffered
  setupSD(readId());
  appendToSD(buffer);
  Serial.printf("Writing %i bytes to SD\n", offset);
}


// Measure all the parameters and write to the SD
int measure(HeatingState heatingState, char* buffer, int size)
{
  setup_thermistors();

  String timestamp = getTimestamp();
  float t0 = readThermistorTemp(MuxChannel::NEAR_INNER);
  float t1 = readThermistorTemp(MuxChannel::NEAR_MIDDLE);
  float t2 = readThermistorTemp(MuxChannel::NEAR_OUTER);
  float t3 = readThermistorTemp(MuxChannel::FAR_INNER);
  float t4 = readThermistorTemp(MuxChannel::FAR_MIDDLE);
  float t5 = readThermistorTemp(MuxChannel::FAR_OUTER);

  const char* preheat = "pre-heat";
  const char* heat = "heat";
  const char* postheat = "post-heat";

  const char* phase = nullptr;
  switch (heatingState)
  {
    case HeatingState::PREHEAT: phase = preheat; break;
    case HeatingState::HEAT: phase = heat; break;
    case HeatingState::POSTHEAT: phase = postheat; break;
    default: phase = "INVALID"; break;
  }

  float batteryVoltage = readBatteryVoltage();
  float heaterCurrent = readHeaterCurrent();
  float ambientTemp = rtcTemperature();

  // writeCsvRow(timestamp.c_str(), t0, t1, t2, t3, t4, t5, phase, batteryVoltage, heaterCurrent, ambientTemp);
  return writeCsvRowToBuffer(timestamp.c_str(), t0, t1, t2, t3, t4, t5, phase, batteryVoltage, heaterCurrent, ambientTemp, buffer, size);
}



void loop() {
  // Keep itself on once it starts the loop
  keep_on(true);

  if (rtcLostPower())
  {
    Serial.println("RTC lost power");

    setRtcTimeFromSerial();
  }

  String timestamp = getTimestamp();
  Serial.print("Current time: ");
  Serial.println(timestamp);



  if (setupSD(readId()))
  {
    Serial.println("Writing to SD");
    appendToSD("Hello, world!");
#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
    appendToSD("From RP2040");
#endif
#ifdef ADAFRUIT_FEATHER_M0
    appendToSD("From M0");
#endif

    String timestamp = getTimestamp();
    appendToSD(timestamp);
  }

  clearAlarm1();
  setAlarm1(T_MINS);

  measurementCycle();


  if (read_aux_sw())
  {
    Serial.println("Aux switch pressed, running another cycle immediately");
  }
  else
  {
    Serial.println("Aux switch not pressed, going to sleep");
    
    // Turn itself off
    keep_on(false);
    delay(1000);

    // Wait for alarm
    Serial.print("Waiting for alarm");
    while (!isAlarm1Fired())
    {
      if (read_aux_sw())
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


