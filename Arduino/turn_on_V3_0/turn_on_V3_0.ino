

#define LED_R 10
#define LED_Y 11
#define LED_G 12
#define LED_FEATHER 13
#define LED_POWER 5
#define LED_TIMER 6
#define LED_ERROR 9



void setup() {

  pinMode(LED_R, OUTPUT);
  pinMode(LED_Y, OUTPUT);
  pinMode(LED_G, OUTPUT);
  pinMode(LED_FEATHER, OUTPUT);
  pinMode(LED_POWER, OUTPUT);
  pinMode(LED_TIMER, OUTPUT);
  pinMode(LED_ERROR, OUTPUT);



  

}

void loop() {
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
}













