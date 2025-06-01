

#define SW_AUX A2


// TX pin is different between these two boards
#ifdef ARDUINO_ADAFRUIT_FEATHER_RP2040_ADALOGGER
  #define KEEP_ON D0
  #define HEAT D1
#endif

#ifdef ADAFRUIT_FEATHER_M0
  #define KEEP_ON 1
  #define HEAT 0
#endif



void setup_misc()
{
  pinMode(KEEP_ON, OUTPUT);
  pinMode(HEAT, OUTPUT);
  pinMode(SW_AUX, INPUT_PULLUP);
}

void keep_on(bool on)
{
  digitalWrite(KEEP_ON, on);
}

void heater(bool on)
{
  digitalWrite(HEAT, on);
}

bool read_aux_sw()
{
  return !digitalRead(SW_AUX);
}


