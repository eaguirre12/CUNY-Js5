/* SAP FLOW SENSOR */
#include <RTClib.h>
#include <SD.h>
#include <Adafruit_ADS1X15.h>

/// T is the period between measurement events
#define T_HRS 0
#define T_MINS 3
#define T_SECS 0
/// PREH is the measurement period before heat on
#define PREH_HRS 0
#define PREH_MINS 0
#define PREH_SECS 20
/// H is the period to apply heat
#define H_HRS 0
#define H_MINS 0
#define H_SECS 10
/// POTSTH is the measurement period after heat
#define POSTH_HRS 0
#define POSTH_MINS 0
#define POSTH_SECS 30
/// TS is the read data period between measuements. The shortest cycle
#define TS_HRS 0
#define TS_MINS 0
#define TS_SECS 2
/// SLEEP time to stop for the day
#define SLEEP_HRS 23
#define SLEEP_MINS 01
/// WAKE time to start the day
#define WAKE_HRS 6
#define WAKE_MINS 30
// ......|______|----|_____________|...................... |____|----|_____________|
//         PREH   H       POSTH
//.......|............................T....................|....................
/// note: T > PREH + H + POSTH > TS

#define DEVICE_NAME "SFS02"
// states (periods) in cycle
#define PREH 0
#define H 1
#define POSTH 2

#define R2 10000.00
#define R1 100000.00
#define MAX_ADC 19999
#define MAX_ADCV 2.048
#define SERIESRESISTOR 10000.000

#define SD_CHIP_SELECT 4
#define HEATER_PIN 6

RTC_DS3231 rtc_ds3231;
Adafruit_ADS1115 ads1, ads2;
File myFile;

bool ledToggle = true;
float vin, tempC1, tempC2, tempC3, tempC4, tempC5, tempC6, tempC7, tempC8;
int heatingState = PREH;
int readDataPeriod;
uint32_t acc = 0;
DateTime dt, sleepTime, wakeTime;
TimeSpan ts;

/*
*/
void setup() {
  Serial.begin(9600);
  delay(3000);
  Serial.println(__FILE__);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  initializeSD_ADC();  // this first for debugging info

  if (!rtc_ds3231.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1) {};
  }

  rtc_ds3231.disable32K();
  rtc_ds3231.writeSqwPinMode(DS3231_OFF);

  fireAlarm2();  // check to see if we are here because of button press

  readDataPeriod = TimeSpan(0, TS_HRS, TS_MINS, TS_SECS).totalseconds() * 1000;

  //RTC adjust
  if (rtc_ds3231.lostPower()) {
    dt = inputDateTime();
    if (dt.isValid())
      rtc_ds3231.adjust(dt);
  }

  writeHeaderSD();
  sleepOrMeasure();
}
/*

*/
void loop() {
  if (rtc_ds3231.alarmFired(1)) {
    Serial.println("alarm 1 fired in main loop");
    fireAlarm2();
    writeHeaderSD();
    sleepOrMeasure();
  }
  toggleLedDelay(1000);
}
/*

*/
void sleepOrMeasure() {
  // decide if time to sleep or work
  dt = rtc_ds3231.now();
  sleepTime = DateTime(dt.year(), dt.month(), dt.day(), SLEEP_HRS, SLEEP_MINS, 0);
  wakeTime = DateTime(dt.year(), dt.month(), dt.day(), WAKE_HRS, WAKE_MINS, 0);
  ts = wakeTime - sleepTime;

  if (ts.totalseconds() >= 0) {                  // sleep during the day: sleep < wake
    if ((dt >= sleepTime) && (dt < wakeTime)) {  // somehow woke up during sleep time
      putToSleep();
    } else {  // woke up during work time
      measureSetNextT();
      rtc_ds3231.clearAlarm(2);  // POWER OFF
    }
  } else {  // sleep over night
    if ((dt >= wakeTime) && (dt < sleepTime)) {
      measureSetNextT();
      rtc_ds3231.clearAlarm(2);                    // POWER OFF
    } else if (dt > sleepTime) {                   // woke past bedtime at night
      wakeTime = wakeTime + TimeSpan(1, 0, 0, 0);  //next day
      putToSleep();
    } else {  // woke  too early in the morning
      putToSleep();
    }
  }
}
/*
*/
void putToSleep() {
  rtc_ds3231.setAlarm2(wakeTime, DS3231_A2_Date);
  rtc_ds3231.disableAlarm(1);  // incase it fires ?
  rtc_ds3231.clearAlarm(2);    // POWER OFF
  rtc_ds3231.clearAlarm(1);    // just in case still on ?
}
/*
*/
void measureSetNextT() {
  heatingState = PREH;
  setPREH();

  while (true) {
    readThermistor();
    vin = measureVoltage();
    writeSD(heatingState);

    if (rtc_ds3231.alarmFired(1)) {
      switch (heatingState) {
        case PREH:
          {  // turn heater ON
            heaterOn();
            heatingState = H;
            setH();
            break;
          }
        case H:
          {  // turn heater OFF
            heaterOFF();
            heatingState = POSTH;
            setPOSTH();
            break;
          }
        case POSTH:
          {  // done cycle so set alarm 1 to T cycle and get out
            heatingState = PREH;
            break;
          }
      }  // end switch
      if (heatingState == PREH)
        break;  // get out of the while(true)
    }           // end if A1 fired
    //    delay(readDataPeriod);
    toggleLedDelay(readDataPeriod);
  }  // end while(true)

  ts = TimeSpan(0, T_HRS, T_MINS, T_SECS) - TimeSpan(0, PREH_HRS, PREH_MINS, PREH_SECS)  //
       - TimeSpan(0, H_HRS, H_MINS, H_SECS) - TimeSpan(0, POSTH_HRS, POSTH_MINS, POSTH_SECS);
  dt = rtc_ds3231.now() + ts;

  rtc_ds3231.setAlarm2(dt, DS3231_A2_Day);

  rtc_ds3231.disableAlarm(1);
  rtc_ds3231.clearAlarm(2);  // power off
  rtc_ds3231.clearAlarm(1);  // just in case
}
/*
*/
void fireAlarm2() {
  if (!rtc_ds3231.alarmFired(2)) {
    DateTime dt = DateTime(0, 0, 0, 0, 0, 0);
    rtc_ds3231.setAlarm2(dt, DS3231_A2_Minute);
    uint32_t strt = micros();
    DateTime nw = rtc_ds3231.now();
    rtc_ds3231.adjust(dt);          // set to 0000 to fire
    rtc_ds3231.adjust(nw);          // restore now time
    acc = acc + (micros() - strt);  // us delay accumulator
    Serial.printf("acc (us): %d\n", acc);
    if (acc > 610000) {                                   // adjust this number to tune
      rtc_ds3231.adjust(rtc_ds3231.now() + TimeSpan(1));  // advance 1s
      acc = 0;
    }
  }
}
