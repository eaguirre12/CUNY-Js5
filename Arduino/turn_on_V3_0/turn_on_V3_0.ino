


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
#endif

#ifdef ARDUINO_ADAFRUIT_FEATHER_M0_ADALOGGER
  #define KEEP_ON D1
#endif



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


  Serial.begin(9600); // Value ignored
  

}

void loop() {
  // Keep itself on once it starts the loop
  digitalWrite(KEEP_ON, 1);

  Serial.println("Hello");

  float vbat = analogRead(V_BAT_SENSE);
  vbat = vbat / 1023 * 3.3 / (10.0 / 110);
  Serial.println(vbat);

  // Serial.println(analogRead(V_BAT_SENSE));

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

  // Turn itself off
  digitalWrite(KEEP_ON, 0);
  delay(1000);

}













