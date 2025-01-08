/**
 * ESP32 Multicore semaphore demo
 * 
 * How to use semaphores with multicore tasks.
 */
// COre definitions
static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// Settings
static const uint32_t task_0_delay = 500; // Time(ms) task 0 blocks itself

// pins
static const int pin_1 = LED_BUILTIN; //LED pin

// Globals
static SemaphoreHandle_t bin_sem;

//**************************************************************************
//Tasks
//Task in Core 0
void doTask0(void *parameters) {
  
  // Configure pin
  pinMode(pin_1, OUTPUT);
  
  // Do forever 
  while(1) {

    //Notify other task
    xSemaphoreGive(bin_sem);

    // Yield processor for a while
    vTaskDelay(task_0_delay / portTICK_PERIOD_MS);
  }
}

//Task in Core 1
void doTask1(void *parameters) {

  // Do forever 
  while(1) {
    xSemaphoreTake(bin_sem, portMAX_DELAY);

    // Toggle LED
    digitalWrite(pin_1, !digitalRead(pin_1));
  }
}
void setup() {
  // put your setup code here, to run once:
  bin_sem = xSemaphoreCreateBinary();
  
  // Start Task 0 
  xTaskCreatePinnedToCore (doTask0,
                           "Task 0",
                           2048,
                           NULL,
                           1,
                           NULL,
                           pro_cpu);
  // Start Task  1
  xTaskCreatePinnedToCore (doTask1,
                           "Task 1",
                           2048,
                           NULL,
                           1,
                           NULL,
                           app_cpu);
}

void loop() {
  // put your main code here, to run repeatedly:

}
