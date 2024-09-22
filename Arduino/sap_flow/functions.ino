// routine to input date time from Serial
/*

*/
void initializeSD_ADC() {
  if (!SD.begin(SD_CHIP_SELECT)) {
    Serial.println("Card failed or not present!");
    while (1) {};
  }
  Serial.println("Card Initialized");

  ads1.setGain(GAIN_TWO);
  ads2.setGain(GAIN_TWO);
  if (!ads1.begin(0x48)) {
    Serial.println("Failed to initialized ADS1.");
    //while (1) {};
  }
  if (!ads2.begin(0x48)) {
    Serial.println("Failed to initialized ADS2.");
    //while (1) {};
  }
}
/*

*/
DateTime inputDateTime() {
  char datetime[32] = "YYYY/MM/DD hh:mm:ss";
  rtc_ds3231.now().toString(datetime);

  while (Serial.available() > 0)
    Serial.read();

  Serial.println("Enter date-time as YYYY/MM/dd hh:mm:ss");
  static char message[32];
  static unsigned int message_pos = 0;

  while (Serial.available() == 0) {};
  while (Serial.available() > 0) {
    char inByte = Serial.read();
    //Message coming in (check not terminating character) and guard for over message size
    if (inByte != '\n' && (message_pos < 32 - 1)) {
      //Add the incoming byte to our message
      message[message_pos] = inByte;
      message_pos++;
    } else {
      //Add null character to string
      message[message_pos] = '\0';
      //Print the message (or do other things)
      Serial.println(message);
      message_pos = 0;
    }
  }

  if (message) {
    int Year, Month, Day, Hour, Minute, Second;
    sscanf(message, "%d/%d/%d %d:%d:%d", &Year, &Month, &Day, &Hour, &Minute, &Second);
    return DateTime(Year, Month, Day, Hour, Minute, Second);
  } else
    return DateTime(0, 0, 0, 0, 0, 0);
}
/*

*/
void printDateTime(DateTime DT) {
  char dt[32] = "YY/MM/DD hh:mm:ss";
  DT.toString(dt);
  Serial.print(dt);
  Serial.print("->");
}
/*

*/
void printTime(DateTime DT) {
  char dt[32] = "hh:mm:ss";
  DT.toString(dt);
  Serial.print(dt);
  Serial.print(" ");
}
/*

*/
void printTimeSpan(TimeSpan TS) {
  Serial.printf("days:%d - %d:%d:%d ", TS.days(), TS.hours(), TS.minutes(), TS.seconds());
}
/*

*/
void setPREH() {  //
  rtc_ds3231.disableAlarm(1);
  rtc_ds3231.clearAlarm(1);
  DateTime DT = rtc_ds3231.now() + TimeSpan(0, PREH_HRS, PREH_MINS, PREH_SECS);
  rtc_ds3231.setAlarm1(DT, DS3231_A1_Hour);
}
/*
*/
void setH() {  //
  rtc_ds3231.disableAlarm(1);
  rtc_ds3231.clearAlarm(1);
  DateTime DT = rtc_ds3231.now() + TimeSpan(0, H_HRS, H_MINS, H_SECS);
  rtc_ds3231.setAlarm1(DT, DS3231_A1_Hour);
}
/*
*/
void setPOSTH() {
  rtc_ds3231.disableAlarm(1);
  rtc_ds3231.clearAlarm(1);
  DateTime DT = rtc_ds3231.now() + TimeSpan(0, POSTH_HRS, POSTH_MINS, POSTH_SECS);
  rtc_ds3231.setAlarm1(DT, DS3231_A1_Hour);
}
/*
*/
float measureVoltage() {
  pinMode(A7, INPUT);
  return analogRead(A7) * 0.006445;
}
/*
*/
void readThermistor() {
  // int16_t ADCout1 = ads1.readADC_SingleEnded(0);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  // int16_t ADCout2 = ads1.readADC_SingleEnded(1);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  // int16_t ADCout3 = ads1.readADC_SingleEnded(2);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  // int16_t ADCout4 = ads1.readADC_SingleEnded(3);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  // int16_t ADCout5 = ads2.readADC_SingleEnded(0);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  // int16_t ADCout6 = ads2.readADC_SingleEnded(1);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  // int16_t ADCout7 = ads2.readADC_SingleEnded(2);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3
  // int16_t ADCout8 = ads2.readADC_SingleEnded(3);  //A0 input on ADS1115; change to 1=A1, 2=A2, 3=A3

  int16_t ADCout1 = 10000;
  int16_t ADCout2 = 11000;
  int16_t ADCout3 = 12000;
  int16_t ADCout4 = 13000;
  int16_t ADCout5 = 14000;
  int16_t ADCout6 = 15000;
  int16_t ADCout7 = 16000;
  int16_t ADCout8 = 17000;

  // Serial.printf("ADCs: %d, %d, %d, %d, %d, %d, %d, %d \n",
  //               ADCout1, ADCout2, ADCout3, ADCout4, ADCout5, ADCout6, ADCout7, ADCout8);

  float ohms1 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout1) - 1);
  float ohms2 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout2) - 1);
  float ohms3 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout3) - 1);
  float ohms4 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout4) - 1);
  float ohms5 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout5) - 1);
  float ohms6 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout6) - 1);
  float ohms7 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout7) - 1);
  float ohms8 = SERIESRESISTOR * ((MAX_ADC / (float)ADCout8) - 1);

  // this function temp(ohms) is valid 0-50 C
  tempC1 = 62.57 - ohms1 * (0.005314) + 0.0000001827 * ohms1 * ohms1 - 0.000000000002448 * ohms1 * ohms1 * ohms1;
  tempC2 = 62.57 - ohms2 * (0.005314) + 0.0000001827 * ohms2 * ohms2 - 0.000000000002448 * ohms2 * ohms2 * ohms2;
  tempC3 = 62.57 - ohms3 * (0.005314) + 0.0000001827 * ohms3 * ohms3 - 0.000000000002448 * ohms3 * ohms3 * ohms3;
  tempC4 = 62.57 - ohms4 * (0.005314) + 0.0000001827 * ohms4 * ohms4 - 0.000000000002448 * ohms4 * ohms4 * ohms4;
  tempC5 = 62.57 - ohms5 * (0.005314) + 0.0000001827 * ohms5 * ohms5 - 0.000000000002448 * ohms5 * ohms5 * ohms5;
  tempC6 = 62.57 - ohms6 * (0.005314) + 0.0000001827 * ohms6 * ohms6 - 0.000000000002448 * ohms6 * ohms6 * ohms6;
  tempC7 = 62.57 - ohms7 * (0.005314) + 0.0000001827 * ohms7 * ohms7 - 0.000000000002448 * ohms7 * ohms7 * ohms7;
  tempC8 = 62.57 - ohms8 * (0.005314) + 0.0000001827 * ohms8 * ohms8 - 0.000000000002448 * ohms8 * ohms8 * ohms8;
}
/*

*/
void writeSD(int heatingState) {
  File myFile = SD.open("SAPUFLUX.txt", FILE_WRITE);

  char datetime[32] = "YYYY/MM/DD hh:mm:ss";
  rtc_ds3231.now().toString(datetime);
  myFile.print(datetime);
  myFile.print(" ");

  char tempString[128];

  String outString = String(tempC1) + String(", ") + String(tempC2) + String(", ") + String(tempC3) +                //
                     String(", ") + String(tempC4) + String(", ") + String(tempC5) + String(", ") +                  //
                     String(tempC6) + String(", ") + String(tempC7) + String(", ") + String(tempC8) + String(", ");  //

  switch (heatingState) {
    case PREH:
      {
        outString = outString + String("pre-heat");
        break;
      }
    case H:
      {
        outString = outString + String("heat");
        break;
      }
    case POSTH:
      {
        outString = outString + String("post-heat");
        break;
      }
  }

  myFile.println(outString);
  myFile.close();
  Serial.println(outString);
}
/*

*/
void writeTextSD(String message) {
  File myFile = SD.open("SAPUFLUX.txt", FILE_WRITE);

  char datetime[32] = "YYYY/MM/DD hh:mm:ss";
  rtc_ds3231.now().toString(datetime);
  myFile.print("M-");
  myFile.print(datetime);
  myFile.print("-");
  myFile.println(message);
  myFile.close();
}
/*
*/
void writeHeaderSD() {
  File myFile = SD.open("SAPUFLUX.txt", FILE_WRITE);
  myFile.printf("M- Starting Event on Device %s\n", DEVICE_NAME);
  char datetime[32] = "YYYY/MM/DD hh:mm:ss";
  rtc_ds3231.now().toString(datetime);
  myFile.print("M-");
  myFile.print(datetime);
  myFile.printf(" T=%d:%d:%d PREH=%d:%d:%d H=%d:%d:%d POSTH=%d:%d:%d \n",
                T_HRS, T_MINS, T_SECS, PREH_HRS, PREH_MINS, PREH_SECS,
                H_HRS, H_MINS, H_SECS, POSTH_HRS, POSTH_MINS, POSTH_SECS);
  myFile.print("M-");
  myFile.print(datetime);
  myFile.printf("Sleep Time: %d:d%, Wake Time: %d:%d \n", SLEEP_HRS, SLEEP_MINS, WAKE_HRS, WAKE_MINS);
  myFile.close();
}
/*
*/
void heaterOn() {
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, HIGH);
}
/*
*/
void heaterOFF() {
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(HEATER_PIN, LOW);
}
/*
*/
void toggleLedDelay(uint32_t delayms) {
  if (ledToggle) {
    digitalWrite(LED_BUILTIN, HIGH);
    ledToggle = false;
  } else {
    digitalWrite(LED_BUILTIN, LOW);
    ledToggle = true;
  }
  delay(delayms);
}