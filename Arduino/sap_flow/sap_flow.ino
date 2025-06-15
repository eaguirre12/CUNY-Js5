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


*/ 

#include <RTClib.h>
#include <SD.h>
#include <Adafruit_ADS1X15.h>
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
#define DEVICE_NAME "SF_03"
#define FILE_NAME DEVICE_NAME ".txt"
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
#define ENABLE_VOLTAGE_CUTOFF (0)
#define VOLTAGE_CUTOFF (0.9 * 8)

RTC_DS3231 rtc_ds3231;
Adafruit_ADS1115 ads1, ads2;
bool ledToggle = true;
float tempC1, tempC2, tempC3, tempC4, tempC5, tempC6, tempC7, tempC8;

// The red LED at the top of the shares a pin with the on-board red LED
#define RED_LED 13
#define YELLOW_LED 12
#define GREEN_LED 11
#define POWER_LED 10
#define TIMER_LED 9
#define ERROR_LED 5


/*
*/
void setup() {
  Serial.begin(9600); // Baud rate ignored on this platform

  // Make sure to drive the heater pin low immediately.
  // Otherwise the gate may float up and turn on the heater!
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);

  // Turn on all LEDs for 1 second on power up
  pinMode(RED_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(POWER_LED, OUTPUT);
  pinMode(TIMER_LED, OUTPUT);
  pinMode(ERROR_LED, OUTPUT);
  digitalWrite(RED_LED, 1);
  digitalWrite(YELLOW_LED, 1);
  digitalWrite(GREEN_LED, 1);
  digitalWrite(POWER_LED, 1);
  digitalWrite(TIMER_LED, 1);
  digitalWrite(ERROR_LED, 1);
  delay(1000);

  digitalWrite(RED_LED, 0);
  digitalWrite(YELLOW_LED, 0);
  digitalWrite(GREEN_LED, 0);
  // Leave power LED on
  digitalWrite(TIMER_LED, 0);
  digitalWrite(ERROR_LED, 0);
  delay(1000);

  digitalWrite(POWER_LED, 1);
  delay(4000);



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
  // rtc_ds3231.adjust(DateTime(F(__DATE__), F(__TIME__)) + TimeSpan(0,0,0,1  ));

  rtc_ds3231.disable32K();
  rtc_ds3231.writeSqwPinMode(DS3231_OFF);

  fireAlarm2(); // check to see if we are here because of button press

  printDateTime();
  Serial.println(" current RTC date/time");
  //RTC adjust
  if (rtc_ds3231.lostPower()) {
    digitalWrite(TIMER_LED, 1);
    DateTime dt = inputDateTime();
    if (dt.isValid())
      rtc_ds3231.adjust(dt);
    digitalWrite(TIMER_LED, 0);
  }

  // If the user requested an SD dump, do so
  checkForDumpCommand();

  writeHeaderSD();

#if ENABLE_VOLTAGE_CUTOFF
  float voltage = measureVoltage();
  if (voltage < VOLTAGE_CUTOFF)
  {
    String message;
    message += "Battery pack voltage is ";
    message += String(voltage, 3);
    message += "V which is below the cutoff of ";
    message += String(VOLTAGE_CUTOFF, 3);
    message += "V. The measurement cycle will be skipped.\n";

    Serial.print(message);
    writeTextSD(message);

    // Go to sleep until the next wakeup time
    putToSleep(rtc_ds3231.now() + TimeSpan(0, T_HRS, T_MINS, T_SECS));
  }
  else
#endif
  {
    // Set the alarm before the measurement cycle. Otherwise you need let it 
    // run a full measurment cycle while programming or it won't wake up!
    setNextAlarm();
    measurementCycle();
  }
  turnOff();
}

/* The loop() is only reached when under USB power!

*/
void loop() {

  Serial.println("Entered loop() -- we must be on USB power");

  checkForDumpCommand();

  delay(1000);
}


void measurementCycle()
{
  HeatingState heatingState = HeatingState::PREHEAT;
  
  // Delay execution until the clock rolls over to the next second to align execution.
  waitForNextSecond();

  const int fullCycleSeconds = PREH_SECS + H_SECS + POSTH_SECS;
  for (int i = 0; i < fullCycleSeconds; ++i)
  {
    if (i < PREH_SECS)
    {
      // Spend this second in preheat
      heatingState = HeatingState::PREHEAT;
    }
    else if (i < PREH_SECS + H_SECS)
    {
      // Spend this second in heat
      heatingState = HeatingState::HEAT;

    }
    else
    {
      // Spend this second in postheat
      heatingState = HeatingState::POSTHEAT;

    }

    if (heatingState == HeatingState::HEAT)
    {
      heaterOn();
    }
    else
    {
      heaterOFF();
    }

    readThermistor();
    writeSD(heatingState);

    // Toggle the LED, but don't delay 1 second. Instead poll the RTC time until we reach the next second.
    toggleLedDelay(0);
    digitalWrite(TIMER_LED, !digitalRead(TIMER_LED));
    waitForNextSecond();
  }
  
  digitalWrite(GREEN_LED, LOW);
}




// This will sleep until the next multiple of T_MINS.
// If T_MINS is 30, it will sleep until the minute hand says 0 or 30.
// If T_MINS is 2, it will sleep until the minute hand says 0, 2, ... 58.
// Important: If the measurement cycle time is more than T_MINS, it will not take a measurement cycle every T_MINS.
//            For examplle, if T_MINS = 2 but the measurement cycle takes 3 minutes, it will actually measure every 4 minutes!

// This will set the alarm for the next measurement
void setNextAlarm()
{
  
  DateTime currentTime = rtc_ds3231.now();
  int nextMinute = currentTime.minute() + 1;
  while (nextMinute % T_MINS)
  {
    nextMinute++;
  }
  if (nextMinute >= 60)
  {
    nextMinute -= 60;
  }
  Serial.print("Next measurement will start at ");
  Serial.print(nextMinute);
  Serial.println(" minutes past the hour");

  // We're ignoring the year through hour, only alarming on the minute and second fields.s
  DateTime wakeTime = DateTime(0, 0, 0, 0, nextMinute, 0);

  rtc_ds3231.disableAlarm(1);
  rtc_ds3231.clearAlarm(1);
  digitalWrite(LED_BUILTIN, LOW);

  // Sleep until the minutes match.
  if (!rtc_ds3231.setAlarm2(wakeTime, DS3231_A2_Minute))
    Serial.println("Error, A2 T wasn't set!");

}

void turnOff()
{
  rtc_ds3231.clearAlarm(2);  // POWER OFF
}


// This will sleep until the next multiple of T_MINS.
// If T_MINS is 30, it will sleep until the minute hand says 0 or 30.
// If T_MINS is 2, it will sleep until the minute hand says 0, 2, ... 58.
// Important: If the measurement cycle time is more than T_MINS, it will not take a measurement cycle every T_MINS.
//            For examplle, if T_MINS = 2 but the measurement cycle takes 3 minutes, it will actually measure every 4 minutes!
void sleepUntilNextMeasurement()
{
  
  DateTime currentTime = rtc_ds3231.now();
  int nextMinute = currentTime.minute() + 1;
  while (nextMinute % T_MINS)
  {
    nextMinute++;
  }
  if (nextMinute >= 60)
  {
    nextMinute -= 60;
  }
  Serial.print("Next measurement will start at ");
  Serial.print(nextMinute);
  Serial.println(" minutes past the hour");

  // We're ignoring the year through hour, only alarming on the minute and second fields.s
  DateTime wakeTime = DateTime(0, 0, 0, 0, nextMinute, 0);

  rtc_ds3231.disableAlarm(1);
  rtc_ds3231.clearAlarm(1);
  digitalWrite(LED_BUILTIN, LOW);

  // Sleep until the minutes match.
  if (!rtc_ds3231.setAlarm2(wakeTime, DS3231_A2_Minute))
    Serial.println("Error, A2 T wasn't set!");

  rtc_ds3231.clearAlarm(2);  // POWER OFF
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
