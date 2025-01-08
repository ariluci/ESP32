// Use only 1 for demo purposes 
#if CONFIG_FREERTO_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint16_t timer_divider = 8; // Clock ticks at 10MHz now
static const uint64_t timer_max_count = 1000000;
/* Delay for task wich allows the isr a few times before the task runs again */
static const TickType_t task_delay = 2000 / portTICK_PERIOD_MS;

// Pins
static const int led_pin = LED_BUILTIN;

// Globals
static hw_timer_t *timer = NULL;
/* Volatile tells compiler not to optimize because
 * valued might change ouside of scope of crt executing task (inisde the isr)*/
static volatile int isr_counter;
/* A special MUTEX, to prevent tasks in the other core to enter
 * a creitical sections. Like mutex, but cause task atempting to take them to  
 * loop forever to be available */
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
//************************************************************************************
//Interrupt Service Routines (ISRs)

//This function executes when timer reaches max
void IRAM_ATTR onTimer() {
  
  //ESP-IDF version of a critical section (in a ISR)
  /* in adition to preventing other tasks from the other core to 
   * take the spinlock and enter crit_sec; it also disables all other
   * interupts in the curent core */
  /* thus this prevents nested interupts but may*have the efect of delaying or missing
   *  other interupts while in crit_section. Best practice: keep isr short */ 
  /* Best practice: don't call other freeRTOS api func from inside crit_sec */
  portENTER_CRITICAL_ISR(&spinlock);
  isr_counter++;
  portEXIT_CRITICAL_ISR(&spinlock);  

  // Vanilla FreeRTOS version of a critical section (in an ISR)
  //UBaseType_t save_int_status;
  //saved_int_status = taskENTER_CRITICAL_FROM_ISR();
  //isr_counter++;
  //taskEXIST_CRITICAL_FROM_ISR(saved_int_status);
}

//************************************************************************************
//Task

// Wait for semaphore and print value when received
void printValues(void *parameters) {
  
  while(1) {

    // Count down and print out counter value
    while (isr_counter > 0) {
      
      // Print value of counter
      Serial.println(isr_counter);

       /* A mutex can't block an interupt and do not want tasks to spin*/ 
       /* we use a no-ISR critcal section functions to disable interupts and
        * prevent context switching while crt core executes code between them */
       /* We still need spinlock to prevent other core to access this variable */
       /* vanilla freeRtos version is easyer as it does not include the spinlock */
       /* if a hw interrupt accures douring execution of this critical section
        * it will not be dropped; the isr will trigger as soon as the crt exec 
        * leaves the crt sect and interupts are reenabled*/ 
       //ESP-IDF version of a critical section (in a task)
       portENTER_CRITICAL(&spinlock);
       isr_counter--;
       portEXIT_CRITICAL(&spinlock);
        
       //Vanilla FreeRTOS version of a critical section (in a task)
       //taskENTER_CRITICAL();
       //isr_counter--;
       //taskEXIST_CRITICAL();
    }
    // Wait 2 sec while ISR increments counter a few times
    vTaskDelay(task_delay); 
  }
}
//************************************************************************************
//Main

void setup() {
  //Configure Serial
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS ISR Crtitical section Demo---");
  
  // Start task to print results
  xTaskCreatePinnedToCore(printValues,
                          "Print values",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);  
  
  // Create and start timer (num, divider, countUP)
  timer = timerBegin(0, timer_divider, true);

  // Provide ISR to timer (timer, cb_function, edge)
  timerAttachInterrupt(timer, &onTimer, true);

  // At what count should isr trigger (timer, timer_max_count, autoreload)
  timerAlarmWrite (timer, timer_max_count, true);

  // Allow ISR to trigger
  timerAlarmEnable(timer);

  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // put your main code here, to run repeatedly:

}
