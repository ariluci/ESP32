/**
 * ESP32 Deadlock demo
 * 
 * Demonstrate deadlock with 2 tasks
 */
// Use only 1 for demo purposes 
#if CONFIG_FREERTO_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//#include semphr.h -needed in vanilla FreeRTOS

//Globals
SemaphoreHandle_t mutex_1;
SemaphoreHandle_t mutex_2;

//*****************************************************************************
// Tasks
void doTaskA(void *parameters) {

  // loop forever
  while(1) {
    
    // Take mutex_1 (introduce wait to force deadlock)
    xSemaphoreTake(mutex_1, portMAX_DELAY);
    Serial.println("Task A took mutex 1");
    vTaskDelay(1 / portTICK_PERIOD_MS);
  
    // Take mutex_2
    xSemaphoreTake(mutex_2, portMAX_DELAY);
    Serial.println("Task B took mutex 2");
    
    // Critical section protected by 2 mutexes
    Serial.println("Task A entered crit section protected by 2 mutexes and is doing some work"); 
    vTaskDelay(500 / portTICK_PERIOD_MS);
  
    // Give back mutexes
    xSemaphoreGive(mutex_2);
    xSemaphoreGive(mutex_1); 

    // Wait to let other task execute
    Serial.println("Task A going to sleep");
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Task B (low priority)
void doTaskB(void *parameters) {

  // Loop forever 
  while(1) {

      // Take mutex 2 (introduce wait to force deadlock)
      xSemaphoreTake(mutex_2, portMAX_DELAY);
      Serial.println("Task B took mutex 2");
      vTaskDelay(1 / portTICK_PERIOD_MS);

      // Take mutex 1
      xSemaphoreTake(mutex_1, portMAX_DELAY);
      Serial.println("Task B took mutex 1");

      //Critical section protected by 2 mutexes
      Serial.println("Task B entered crit section protected by 2 mutexes and is doing some work");
      vTaskDelay(500 / portTICK_PERIOD_MS);

      // Give back mutexes
      /* Good practice: give back mutexes in reverse order of obtaining */
      xSemaphoreGive(mutex_1);
      xSemaphoreGive(mutex_2);
  }
}
//*****************************************************************************
// Main
void setup() {
  // Config serial
  Serial.begin(115200);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Deadlock Demo---");

  // Create mutexes before starting tasks
  mutex_1 = xSemaphoreCreateMutex();
  mutex_2 = xSemaphoreCreateMutex();

  //Start task A (high prioriy)
  xTaskCreatePinnedToCore(doTaskA,
                          "Task A",
                          1024,
                          NULL,
                          2,
                          NULL,
                          app_cpu);
  xTaskCreatePinnedToCore(doTaskB,
                          "Task B",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);   

}

void loop() {
  // put your main code here, to run repeatedly:

}
