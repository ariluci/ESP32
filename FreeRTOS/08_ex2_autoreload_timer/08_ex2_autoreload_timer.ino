//#include timers.h //Required in vanilla FreeRTOS

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Globals
/* Will call an arbitrary func after a time period 
 * but only once */
static TimerHandle_t one_shot_timer = NULL;
static TimerHandle_t auto_reload_timer = NULL;
//****************************************************************
// Callbacks

//Called when one of the timers expires
/* This can be used to identyfy which timer called the function 
 * if multiple timers have the same func as callback*/
void myTimerCallback(TimerHandle_t xTimer) {

  // Print message if timer 0 expired 
  if ((uint32_t)pvTimerGetTimerID(xTimer) == 0)
    Serial.println("One-shot timer expired");

  // Print message if timer 1 expired
  if ((uint32_t)pvTimerGetTimerID(xTimer) == 1)
    Serial.println("Auto-reload timer expired");
}

//****************************************************************
//Main

void setup() {
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS timer demo---");

  // Create a one shot timer 
  /* pdFALSE - auto reload; alows timmer to continually expire and execute the cb
                (if timer is periodic or not; false = not) */     
  one_shot_timer = xTimerCreate(
                          "One-shot timer",          //Name of timer
                          2000/ portTICK_PERIOD_MS,  //Period of timer (in ticks)
                          pdFALSE,                   //Auto-reload
                          (void *)0,                 //Timer ID
                          myTimerCallback);          //Callback function 

  auto_reload_timer = xTimerCreate(
                        "Auto-reload timer",          //Name of timer
                        1000/ portTICK_PERIOD_MS,  //Period of timer (in ticks)
                        pdTRUE,                   //Auto-reload
                        (void *)1,                 //Timer ID
                        myTimerCallback);          //Callback function
                         
  // Check to make sure the timers were created
  if (one_shot_timer == NULL || auto_reload_timer == NULL) {
    Serial.println("could not create one of the timers");
  } else {

      // Wait and the print out a message that we're starting the timer
      Serial.println("starting timers...");

      // Start timer (max block time if command queue is full)
      xTimerStart(one_shot_timer, portMAX_DELAY); 
      xTimerStart(auto_reload_timer, portMAX_DELAY); 
  }
  // Delete self task to show that timers will work with no user tasks
}

void loop() {
  // put your main code here, to run repeatedly:

}
