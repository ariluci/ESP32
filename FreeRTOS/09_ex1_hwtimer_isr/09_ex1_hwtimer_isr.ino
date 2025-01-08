//Settings
/* Default esp32 clock rate is 80MHz. 
 * We use prescale to divide by 80 and obtain 
 * Clock now ticks at 1MHZ */
static const uint16_t timer_divider = 80;         //Prescaler 
static const uint64_t timer_max_count = 1000000; /*Count to 1 milion*/

// Pins
static const int led_pin = LED_BUILTIN;

// Globals
static hw_timer_t *timer = NULL;

//***********************************************************************************
//Interrupt Service Routines (ISRs)

//This function executes when timer reaches max (and resets)
void IRAM_ATTR onTimer() {
  
  //Toggle LED
  int pin_state = digitalRead(led_pin);
  digitalWrite(led_pin, !pin_state);
}

//***********************************************************************************
//Main

void setup() {
  // Configure LED pin
  pinMode(led_pin, OUTPUT);

  // Create and start timer (num, divider, countUp)
  timer = timerBegin(0, timer_divider, true);

  // Provide ISR to timer (timer, functuion, edge)
  /* config on timer function as the callback function 
   * for when the timer reaches the specified time */
  timerAttachInterrupt(timer, &onTimer, true);

  // At what count should ISR trigger (timer, timer_max_count, autoreload)
  timerAlarmWrite(timer, timer_max_count, true);

  // Allow ISR to trigger
  timerAlarmEnable(timer);

}

void loop() {
  // put your main code here, to run repeatedly:

}
