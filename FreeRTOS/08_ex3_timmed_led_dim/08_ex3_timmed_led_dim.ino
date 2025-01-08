//#include timers.h //Required in vanilla FreeRTOS

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Settings 
static const TickType_t dim_delay  = 5000 / portTICK_PERIOD_MS;

//Pins(change of arduino board does not have LED_BUILTIN defined
static const int led_pin = LED_BUILTIN;

//Globals
static TimerHandle_t one_shot_timer = NULL;

//**************************************************************************
//Callbacks

// Turn off LED when timer expires
void autoDimmerCallback(TimerHandle_t xTimer) {
  digitalWrite(led_pin, LOW);
}

//**************************************************************************
//Tasks

// Echo things back to serial port, turn on LED when while entering input
/* The important part of the task is that each time a new character is received, 
 * the LED is turned on and we start the timer with xTimerStart().
 * If the timer is already running, xTimerStart() works like
 * xTimerReset() and resets the counter. This allows the LED to 
 * stay on if we’re still entering characters. */
void doCLI(void *parameters) {
  char c;
  
  // Configure LED pin
  pinMode(led_pin, OUTPUT);

  while(1) {
    //See if there are any things in the input serial buffer
    if (Serial.available() > 0 ) {
      
      // If so, echo everything back to serial port
      c = Serial.read();
      Serial.print(c);

      // Turn on the LED
      digitalWrite(led_pin, HIGH);

      // Start timer (if timer is already runnig this will act as
      // xTimerReset() instead) 
      xTimerStart(one_shot_timer, portMAX_DELAY);
    }
  }
}
//**************************************************************************
//Main
void setup() {
  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Timer Solution---");
  
  // Create a one-shot timer 
  one_shot_timer = xTimerCreate(
                          "One-shot timer",     //Name of timer
                          dim_delay,            //Period of timer (in ticks))
                          pdFALSE,              //Auto-reload,
                          (void *)0,            //Timer ID
                          autoDimmerCallback);  //Callback function
  // Start command line interface (CLI) task
  xTaskCreatePinnedToCore(doCLI,
                          "Do CLI",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
  // Delete "setup and loop" task
  vTaskDelete(NULL);                        
}

void loop() {
  // Code should not enter here
}
