
//Use only 1 core for demo purposese
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

// Pins define a classic led_pin
static const int led_pin = LED_BUILTIN; 

/*vTaskDelay Non blocking wait function. Tells scheduler
 * to run other tasks until specivied delay time is up
 * come back to continue running this task */
 
/* a tick timer is one of the microproce hardware 
 * timers allocated to  interrupt the processor at a
 * specific interval. interrupt period is called tick */

/* scheduller has an opportunity to run each tick to 
 * figure out which task needs to run for that tick */

/*by default freeRtos sets portTick_PERIOD_MS = 1 default tick period */

/* vTaskDelay expects number of tick to delay not num 
 * of ms. We need to divide the number of desired ms
 * by the num of ticks and give that as an argument. */ 

//Our tasks: 
//blink a LED with 500ms rate.
void toggleLED_rate1(void *parameter) {
    while(1) {
      digitalWrite(led_pin, HIGH);
      vTaskDelay(500 / portTICK_PERIOD_MS);                                  
      digitalWrite(led_pin, LOW); 
      vTaskDelay(500 / portTICK_PERIOD_MS);  
    }
}
//blink a LED with 300ms rate.
void toggleLED_rate2(void *parameter) {
    while(1) {
      digitalWrite(led_pin, HIGH);
      vTaskDelay(300 / portTICK_PERIOD_MS);
      digitalWrite(led_pin, LOW);
      vTaskDelay(300 / portTICK_PERIOD_MS);
    }
}

/* In arduino framework, for esp32 , setup and loop functions exist inside their own task 
 * separate from the main progr entry point. Because of this a new task is spawned as soon 
 * as  we call xTaskCreate or xTaskPinnedToCore
 * In other systems you would need to call the vTaskStartScheduler() function to tell scheduler 
 * to start running. Only then would new task begin executing. That has already been  called
 * for us prior to setup function so we don't have to worry about it */

void setup() {
    // configure PIN as output 
    pinMode(led_pin, OUTPUT);

    //Create task that runs forever
    xTaskCreatePinnedToCore(        // Use xTaskCreate() in vanilla FreeRTOS
             toggleLED_rate1,       //Function to be called
             "Toggle LED rate1",    //Name of task as a string
             1024,                  //Stack size (in bytes in ESP32, words in FreeRTOS)
             NULL,                  //Optional param; pointer to some part of the mem as argument to 
             1,                     //Priorty of task (0 to configMAX_PRIORITIES-1 (0 to 24))
             NULL,                  //Taks handle. Manage task from other task to check status or mem usg or end it.
             app_cpu);              //Run on one core for demo purposes (ESP32 only). this param is not
                                    // present in default xTaskCreate()
    xTaskCreatePinnedToCore(
          toggleLED_rate2,
          "Toggle LED rate2",
          1024,
          NULL,
          1,
          NULL,
          app_cpu);
}

void loop() {
  // put your main code here, to run repeatedly:

}
