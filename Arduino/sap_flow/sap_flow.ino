/* SAP FLOW SENSOR 
    Developped and tested on this system: 
    https://github.com/NextGen-Environmental-Sensor-Lab/NGENS_SapFlowSensor
    Based on J.Belity paper https://www.sciencedirect.com/science/article/pii/S2468067222000967#b0080

    NEXTGEN SENSOR LAB, ASRC, CUNY. RTOLEDO-CROW OCT 2023
 */


/* Stefan's notes

I'm changing the temp calculation to turn any value outside a reasonable range to NaN.
This is just to make the output prettier.

I modified writeSD() so that it composes the full string to be written and writes it
with a single println() call. The same exact string is printed to serial, including the timestamp.

I'm getting all bad values from the thermistors, even with probes hooked up. I don't know why.

I refactored the timestamp printing code. Now it uses a consistent format. 
I added an overload of printDateTime() with no argument -- it prints the current timestamp.

I'm really confused by the sleeping and waking code. Is it really programmed to go to sleep at 2:09 AM
and wake up at 2:10 AM?
Evonne says yeah. Ok, then...

Let's look at the ADC calculations. I see we're setting the gain to "ONE", which means +/-4.096V.
The reference voltage is 2.5V. So at nominal temperature, it should be 1.25V. With the 16-bit signed
format, it should read basically 10000 at nominal voltage. That's interesting.
The code divides 20000 by the reading, subtracts 1, then multiplies by 10000. 
I've verified the calculation. Remember the thermistor is on the top of the voltage divider!
Then there's some calculation to turn that into temperature. I'll just take that on faith.

So why are we reading bad values? Well the multimeter agrees the analog inputs are at 0V.
If the ADC reads 0V, the calculation would give NaN. If it reads 1 we would get 19.58E9 C. 
If it reads -1, we would get something similar negative. And those three values are what we're seeing!
Oh! The ADCs are powered from the uC, but the voltage reference is powered from 6V from the battery!

How much precision can we actually get from the thermistor and ADC?
A quick Excel sheet says if the ADC reading increases from 10000 to 10001, the calculated temperature
will change from 25.25439 to 25.25918. So the third place after the decimal is the last meaningful digit.
This also means that we aren't going to do much better than two places after the decimal that we had before.
We can do a little better by increasing the ADC gain.
If I increase the gain to 2x, our max reading is 2.048V, which corresponds to 51.7 C. That seems
perfectly acceptable. We can't increase the gain to 4x without cutting off at 16.6 C.

So I'll increase the gain to 2x. That will at least increase our resolution to 0.0024 C.
I'm also changing MAX_ADC to 40000 to compensate.


Is it really using the RTC alarms to schedule turning the heat on and off during the measurement cycle?
But the measurement cycle is looping on a 1000ms delay(). This could have bad consequences.
The RTC alarm isn't an interrupt, we're polling it. But we only poll it once per second. But it isn't
*exactly* a second, it's a second plus overhead. So there's bound to be slippage.
This means that the clock time from the RTC (and presumably the alarms too) will advance by 1 second
usually, and 2 seconds occasionally. So the 5 second heat pulse is really a 5 unit heat pulse, and
occasionally it will be a 4 unit heat pulse.
I collected the 100k+ timestamps from the SD card, filtered out the irrelevant ones, and did a frequency
analysis. 112851 1-steps, 13289 2-steps. That's 11.7%! If my calculations are correct, each period is
1.105 seconds long!
So 88.3% of the heat pulses are 5.525 seconds long, and 11.7% are 4.420 seconds long.
Ok. Can I corroborate this with the dataset?
A more direct way to check this. I counted the number of "heat" measurement lines between "pre-heat" and
"post-heat". 1.6% of the time there are only 4 "heat" lines. So not as bad as I thought.
This needs to be fixed.


Ok, let's get back to the issues I came here to look at. 
How about battery level? That should be easy. Done!


Now for dumping the contents. The data rate of Serial will be a problem for dumping the whole file.
There's currently 15MB, which would take 17 minutes even at 115200 (and maybe even longer).
Huh. The definition of Serial.begin() ignores the baud rate! So maybe it's faster than 115200, maybe
much faster
I just measured it as 17.9 kB/s, which is a little faster than 115200.
Evonne says they're running a measurement cycle every 30 minutes. That comes out to 27 seconds per day.
Not too bad for a first pass!
Perhaps at the end of the dump, we should move the existing data to a timestamped file, and start over.
Then each dump will include only the data since the last dump, but all data is preserved.


## 2024-08-04

TODO
  Include voltage in every row of the data
  Low battery cutoff
    (What voltage?)
  Timing issue with the data
    The data shows a bit of a stair-steppy thing every couple of seconds. 
    I think that's because the measurement loop is using delay() for timing.
  Work on data extraction time (lower priority)


I did a bunch of refactoring
  Moved some global variables into inner scopes
  Used a strong enum for heating states

How exactly are the two alarms being used?
  It seems alarm 2 is used for putting the system to sleep
  Alarm 1 is used to time out the three measurement phases

A problem with the dump procedure
  When you plug in the USB to the laptop, you have to wait for a whole measurement cycle before the dump command is read!

What is the purpose of fireAlarm2()?
  I'll bet it's the source of the clock drift Evonne mentioned
  But why is it even there?
  I think I know. If both alarms are cleared, the system will power off in deployment conditions.
  If the system didn't wake due to alarm 2 firing, then clearing alarm 1 might power it off.
    That could be because the device is on USB power, or because someone pushed the button to wake it early.
  Alarm 1 can be set for seconds, but alarm 2 is only set for minutes.
  So here's a better way, that doesn't involve changing the current time
    Just set alarm 2 to go off at the current minute.
    I've implemented that.


I'm modifying the program to work with my ribbon sensor.



*/ 

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
#define H_SECS 2
/// POTSTH is the measurement period after heat
#define POSTH_HRS 0
#define POSTH_MINS 0
#define POSTH_SECS 100
/// TS is the read data period between measuements. The shortest cycle
#define TS_HRS 0
#define TS_MINS 0
#define TS_SECS 1
/// SLEEP time to stop for the day
#define SLEEP_HRS 2
#define SLEEP_MINS 9
/// WAKE time to start the day
#define WAKE_HRS 2
#define WAKE_MINS 10
// ......|______|----|_____________|...................... |____|----|_____________|
//         PREH   H       POSTH
//.......|............................T....................|....................
/// note: T > PREH + H + POSTH > TS
#define DEVICE_NAME "Js5_03_m"
// states (periods) in cycle
enum class HeatingState
{
  PREHEAT,
  HEAT,
  POSTHEAT,
  END,
};
// 
#define HEATER_PIN 6

// A battery voltage of 0.9V per cell is a commonly cited cutoff voltage for NiMH cells
#define VOLTAGE_CUTOFF (0.9 * 8)

RTC_DS3231 rtc_ds3231;
Adafruit_ADS1115 ads1, ads2;
bool ledToggle = true;
float tempC1, tempC2, tempC3, tempC4, tempC5, tempC6, tempC7, tempC8, tempC9, tempC10;

#include "ribbon.h"

ThermistorBoard board{0, 0, 0};

/*
*/
void setup() {
  Serial.begin(9600); // Baud rate ignored on this platform
  delay(6000);
  Serial.println(__FILE__);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  initializeSD_ADC(); // this first for debugging info

  if (!rtc_ds3231.begin()) {
    Serial.println("Couldn't find RTC!");
    writeTextSD("Couldn't find RTC!");
    errorBlinkLoop();
  }
  else {
    Serial.println("RTC set");
    writeTextSD("\n");
    writeTextSD(" RTC set");
  }

  // uncomment to adjust time and immediately reupload with comment
  // rtc_ds3231.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(0,0,0,17));

  rtc_ds3231.disable32K();
  rtc_ds3231.writeSqwPinMode(DS3231_OFF);

  fireAlarm2(); // check to see if we are here because of button press

  printDateTime();
  Serial.println(" current RTC date/time");
  //RTC adjust
  if (rtc_ds3231.lostPower()) {
    DateTime dt = inputDateTime();
    if (dt.isValid())
      rtc_ds3231.adjust(dt);
  }

  // If the user requested an SD dump, do so
  checkForDumpCommand();

  writeHeaderSD();

  board = findThermistorBoard();
  if (board.valid())
  {
    board.setup();
    board.setSps(32);
  }

  // float voltage = measureVoltage();
  // if (voltage < VOLTAGE_CUTOFF)
  // {
  //   String message;
  //   message += "Battery pack voltage is ";
  //   message += String(voltage, 3);
  //   message += "V which is below the cutoff of ";
  //   message += String(VOLTAGE_CUTOFF, 3);
  //   message += "V. The measurement cycle will be skipped.\n";

  //   Serial.print(message);
  //   writeTextSD(message);

  //   // Go to sleep until the next wakeup time
  //   putToSleep(rtc_ds3231.now() + TimeSpan(0, T_HRS, T_MINS, T_SECS));
  // }
  // else
  {
    sleepOrMeasure();
  }
}

/* The loop() is only reached when under USB power!

*/
void loop() {
  if (rtc_ds3231.alarmFired(1)) {
    Serial.println("alarm 1 fired in main loop");
    fireAlarm2();
    writeHeaderSD();
    sleepOrMeasure();
  }

  toggleLedDelay(1000);
  printDateTime();
  Serial.println(" current time");
  printDateTime(rtc_ds3231.getAlarm1());
  Serial.println(" alarm 1 time");

  checkForDumpCommand();
}

/* Either perform a measurement cycle or go back to sleep, depending on the time.
   This function will not return under battery power because it will cut power at the end.
   Under USB power, it will return.
*/
void sleepOrMeasure() {
  // decide if time to sleep or work
  DateTime dt = rtc_ds3231.now();
  DateTime sleepTime = DateTime(dt.year(), dt.month(), dt.day(), SLEEP_HRS, SLEEP_MINS, 0);
  DateTime wakeTime = DateTime(dt.year(), dt.month(), dt.day(), WAKE_HRS, WAKE_MINS, 0);
  TimeSpan ts = wakeTime - sleepTime;

  printDateTime(sleepTime);
  Serial.println("sleep time");
  printDateTime(wakeTime);
  Serial.println("wake time");
  printTimeSpan(ts);
  Serial.println("time span");
  Serial.print("time span seconds: ");
  Serial.println(ts.totalseconds());
  printDateTime(dt);
  Serial.println("now");

  if (ts.totalseconds() >= 0) {                  // sleep during the day: sleep < wake
    if ((dt >= sleepTime) && (dt < wakeTime)) {  // somehow woke up during sleep time
      Serial.println("1) S<=W: WOKE DURING SLEEPTIME");
      writeTextSD("1) S<=W: WOKE DURING SLEEPTIME");
      putToSleep(wakeTime);
    } 
    else {  // woke up during work time
      Serial.println("2) S<=W: WOKE DURING WORK TIME");
      writeTextSD("2) S<=W: WOKE DURING WORK TIME");
      measureSetNextT();
      rtc_ds3231.clearAlarm(2);  // POWER OFF
    }
  } 
  else {  // sleep over night
    if ((dt >= wakeTime) && (dt < sleepTime)) {
      Serial.println("3) W<S WOKE DURING WORK TIME");
      writeTextSD("3) W<S WOKE DURING WORK TIME");
      measureSetNextT();
      rtc_ds3231.clearAlarm(2);   // POWER OFF
    } 
    else if (dt > sleepTime) {  // woke past bedtime at night
      Serial.println("4) W<S WOKE DURING SLEEP TIME (past bedtime)");
      writeTextSD("4) W<S WOKE DURING SLEEP TIME (past bedtime)");
      writeTextSD("current waketime ");
      char cdt[32] = "YY/MM/DD hh:mm:ss";
      wakeTime.toString(cdt);
      writeTextSD(String(cdt));
      wakeTime = wakeTime + TimeSpan(1, 0, 0, 0);  //next day
      strcpy(cdt, "YY/MM/DD hh:mm:ss");
      writeTextSD("adding a day waketime ");
      wakeTime.toString(cdt);
      writeTextSD(String(cdt));
      putToSleep(wakeTime);
    } 
    else {  // woke  too early in the morning
      Serial.println("4) W<S WOKE DURING SLEEP TIME (before waketime)");
      writeTextSD("4) W<S WOKE DURING SLEEP TIME (before waketime)");
      putToSleep(wakeTime);
    }
  }
}
/*
*/
void putToSleep(DateTime wakeTime) {

  Serial.println("Going to sleep. Wake at ");
  printDateTime(wakeTime);
  Serial.println();
  
  writeTextSD("Going to sleep. Wake at ");
  char cdt[32] = "YY/MM/DD hh:mm:ss";
  wakeTime.toString(cdt);
  writeTextSD(String(cdt));

  if (!rtc_ds3231.setAlarm2(wakeTime, DS3231_A2_Date)) {
    Serial.println("Error, A2 wasn't set!");
    writeTextSD("putToSleep: Error, A2 wasn't set!");
  }

  writeTextSD("putToSleep: A2 value before sleep");
  strcpy(cdt,"YY/MM/DD hh:mm:ss");
  rtc_ds3231.getAlarm2().toString(cdt);
  writeTextSD(String(cdt));

  rtc_ds3231.disableAlarm(1); // incase it fires ?
  rtc_ds3231.clearAlarm(2);  // POWER OFF
  rtc_ds3231.clearAlarm(1);  // just in case still on ?
}


/* The main measurement loop function.
*/
void measureSetNextT() {
  // Store the time before we start the measurement loop, to later calculate how long to sleep.
  DateTime startTime = rtc_ds3231.now();


  printDateTime();
  Serial.print("measureSetNext: entering measurements: PREH starting\n");
  writeTextSD("measureSetNext: entering measurements: PREH starting");

  // Delay execution until the clock rolls over to the next second to align execution.
  waitForNextSecond();

  HeatingState heatingState = HeatingState::PREHEAT;
  setPreheatAlarm();

  while (true) {
    readThermistor();
    writeSD(heatingState);

    // If the alarm has fired, advance to the next phase of the measurement cycle
    if (rtc_ds3231.alarmFired(1)) {
      switch (heatingState) {
        case HeatingState::PREHEAT:
          {  // turn heater ON
            printDateTime();
            Serial.print("turning heater on \n");
            heaterOn();
            heatingState = HeatingState::HEAT;
            setHeatAlarm();
            break;
          }
        case HeatingState::HEAT:
          {  // turn heater OFF
            printDateTime();
            Serial.print("turning heater off");
            Serial.println();
            heaterOFF();
            heatingState = HeatingState::POSTHEAT;
            setPostheatAlarm();
            break;
          }
        case HeatingState::POSTHEAT:
          {  // done cycle so set alarm 1 to T cycle and get out
            printDateTime();
            Serial.print("leaving measurements and going to standby");
            Serial.println();
            heatingState = HeatingState::END;
            break;
          }
      }  // end switch
      if (heatingState == HeatingState::END)
        break;  // get out of the while(true)
    }           // end if A1 fired

    // Toggle the LED, but don't delay 1 second. Instead poll the RTC time until we reach the next second.
    toggleLedDelay(0);
    waitForNextSecond();
  }  // end while(true)

  DateTime wakeTime = startTime + TimeSpan(0, T_HRS, T_MINS, T_SECS);

  printDateTime();
  Serial.print("T will start at ");
  printTime(wakeTime);
  Serial.println();

  rtc_ds3231.disableAlarm(1);
  rtc_ds3231.clearAlarm(1);
  digitalWrite(LED_BUILTIN, LOW);

  if (!rtc_ds3231.setAlarm2(wakeTime, DS3231_A2_Day))
    Serial.println("Error, A2 T wasn't set!");
}


/* Manually fire alarm 2, if the system didn't power on because alarm 2 fired.
   Otherwise we might power off when we clear alarm 1.
*/
void fireAlarm2() {
  // If alarm 2 is already fired, no need to do the following
  if (rtc_ds3231.alarmFired(2))
  {
    return;
  }

  // Store the current alarm 2 settings so we can restore them after
  DateTime currentAlarm2 = rtc_ds3231.getAlarm2();
  auto currentAlarm2Mode = rtc_ds3231.getAlarm2Mode();

  // Set alarm 2 to go off when the minutes match the current minute
  rtc_ds3231.setAlarm2(rtc_ds3231.now(), DS3231_A2_Minute);

  // It's remotely possible that the minute will roll over between getting the current time
  // and setting the alarm. If alarm 2 still isn't fired, try again.
  if (!rtc_ds3231.alarmFired(2))
  {
    rtc_ds3231.setAlarm2(rtc_ds3231.now(), DS3231_A2_Minute);
  }

  // If we still didn't get it, print an error message. Could be a misunderstanding.
  if (!rtc_ds3231.alarmFired(2))
  {
    Serial.println("Unable to fire alarm 2 manually!");
  }

  // Restore the previous alarm 2 settings
  rtc_ds3231.setAlarm2(currentAlarm2, currentAlarm2Mode);
}
