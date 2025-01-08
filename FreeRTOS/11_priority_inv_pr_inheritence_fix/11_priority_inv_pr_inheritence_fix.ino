/**
 * ESP32 Deadlock demo
 * 
 * Demonstrate priorty inversion fix demo
 * changed binary sem to mutex to use native FreeRTOS priority inheritance
 */
//#include semphr.h 
// Use only 1 for demo purposes 
#if CONFIG_FREERTO_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
TickType_t cs_wait = 250;    // Time spent in critical section(ms)
TickType_t med_wait = 5000;  // Time medium task sends working (ms)

// Globals
static SemaphoreHandle_t lock;

//************************************************************************
// Tasks

// Task L (low_priority)
void doTaskL(void *parameters) {

  TickType_t timestamp;

  // Do forever
  while(1) {

    // Take lock
    Serial.println("Task L trying to take lock...");
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    xSemaphoreTake(lock, portMAX_DELAY);

    // Say how long we spent waitning for a lock;
    Serial.print("Task L got lock. Spent ");
    Serial.print((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp);
    Serial.println(" ms waiting for lock. Doing some work...");

    // Hog the processor for a while doing nothing;
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while ((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp < cs_wait);

    // Release lock
    Serial.println("Task L releasing lock.");
    xSemaphoreGive(lock);

    // Go to sleep
    vTaskDelay(500/portTICK_PERIOD_MS);
  }
}

// Task M (medium priority)
void doTaskM(void *parameters) {

  TickType_t timestamp;

  // Do forever 
  while(1) {
    
    // Hog the processor for a while doing nothing
    Serial.println("Task M doing some work...");
    timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp < med_wait);

    // Go to sleep
    Serial.println("Task M done!");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Task H (high priority)
void doTaskH(void *parameters) {

  TickType_t timestamp;

  // Do forever
  while (1) {

      // Take lock
      Serial.println("Task H trying to take lock...");
      timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
      xSemaphoreTake(lock, portMAX_DELAY);

      // Say how long we spend waiting for a lock
      Serial.print("Task H got lock. Spent ");
      Serial.print((xTaskGetTickCount() * portTICK_PERIOD_MS)- timestamp);
      Serial.println(" ms waiting for lock. doing some work...");

      // Hog te processor for a while doing nothing
      timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
      while ((xTaskGetTickCount() * portTICK_PERIOD_MS) - timestamp < cs_wait);

      // Release lock
      Serial.println("Task H releasing lock.");
      xSemaphoreGive(lock);

     // Go to sleep
     vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}
//************************************************************************
// Main
void setup() {

     // Config serial
  Serial.begin(115200);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Priority inversion Demo---");

  // Create semaphores mutexes before starting tasks
  

  // !!!!!!!!!!!!!!!!!!!!
  // Replace bin semaphore with mutex to use builtin FreeRTOS
  // support for pr inheritence via mutex

//  lock = xSemaphoreCreateBinary();
  lock = xSemaphoreCreateMutex();

  //Remove line as mutex already starts at 1
  //xSemaphoreGive(lock); //make sure the binary sem starts at 1

  // The order of the task matters to force priority inversion
   
  // Start task L (low prioriy)
  xTaskCreatePinnedToCore(doTaskL,
                          "Task L",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
                          
  // Introduce a delay to forece priority inversion
  vTaskDelay(1 / portTICK_PERIOD_MS);
    
  // Start task H (HIGH prioriy)
  xTaskCreatePinnedToCore(doTaskH,
                          "Task H",
                          1024,
                          NULL,
                          3,
                          NULL,
                          app_cpu);
                          
  // Start task M (MEDIUM prioriy)
  xTaskCreatePinnedToCore(doTaskM,
                          "Task M",
                          1024,
                          NULL,
                          2,
                          NULL,
                          app_cpu);
  vTaskDelete(NULL);
}

void loop() {
  // put your main code here, to run repeatedly:

}
