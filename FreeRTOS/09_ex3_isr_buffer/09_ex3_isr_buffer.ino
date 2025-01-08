/*
 * ESP32 ISR Semaphore
 * 
 * Read ADC values in ISR at 1Hz and defer printing them in a task
 */

//#include semphr.h // required in vanilla FreeRTOS

// Use only 1 for demo purposes 
#if CONFIG_FREERTO_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
static const uint16_t timer_divider = 80; // Count at 1 MHz
static const uint64_t timer_max_count = 1000000;

// Pins
static const int adc_pin = A0;

// Globals
static hw_timer_t *timer = NULL;
static volatile uint16_t val;
static SemaphoreHandle_t bin_sem = NULL;
//***********************************************************************************
// Interrupt Service Routine (ISRs)

// This funcion executes when timer raches max and resets
void IRAM_ATTR onTimer() {

  BaseType_t task_woken = pdFALSE;

  // Perform action (read from ADC)
  val = analogRead(adc_pin);

  // Give semaphore to tell task that new value is ready
  /* give sem from isr will never bloack as ISR not a task 
   * 
   *  if you're trying to take a mutex or as semaphore that is not available
   * you want to deal with that diferently rather than wait 
   * 
   *task_woken parameter - a feature is to determine if a new task is to be
   * un locked if a mutex or sem has been given. if a higher priority task
   * was unblocked because of this action the scheduler should be called to run  
   * that task imediately as soon as the isr is done. we record that fact in this
   * boolean value which gets passed to the portYIELD_FROM_ISR function
   */
  xSemaphoreGiveFromISR(bin_sem, &task_woken);

  // Exit from ISR (ESP-IDF)
  if (task_woken) {
    portYIELD_FROM_ISR();
  }
  // Exit from ISR (Vanilla FreeRTOS)
  //portYIELD_FROM_ISR(task_woken);
}

//***********************************************************************************
//Tasks

// Wait for semaphore  and print out ADC value when received
void printValues(void *parameters) {

  // Loop forever, wait for semaphore, and print value
  while (1) {
    xSemaphoreTake(bin_sem, portMAX_DELAY);
    Serial.println(val);
  }
}

//***********************************************************************************
//Main
void setup() {
  // Config serial
  Serial.begin(115200);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS ISR buffer Demo---");

  // Create binary semahore 
  bin_sem = xSemaphoreCreateBinary();

  //Fore reboot if we can't create semaphore
  if (bin_sem == NULL) {
    Serial.println("Could not create semaphore");
    ESP.restart();
  }

  //Start task to print out results (higher priority thant main)
  xTaskCreatePinnedToCore(printValues,
                          "Print Values",
                          1024,
                          NULL,
                          2,
                          NULL,
                          app_cpu);
                          
  // Create and start timer (num, divider, countUp)
  timer = timerBegin(0, timer_divider, true);

  // Provide ISR to timer (timer, function, edge)
  timerAttachInterrupt(timer, &onTimer, true);

  // At what count should ISR trigger (timer, count, autoreload)
  timerAlarmWrite(timer, timer_max_count, true);

  // Allow ISR to trigger
  timerAlarmEnable(timer);

  vTaskDelete(NULL);
}
void loop() {
  // Do nothing, forever, only is interuped by isr and let other task prin values   
}
