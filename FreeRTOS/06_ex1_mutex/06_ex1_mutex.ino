//#include "semphr.h";
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif
//Globals
static int shared_var = 0;
static SemaphoreHandle_t mutex; 
//********************************************************
//Tasks
//Task:increment share variable (the wrong way)
void incTask(void* parameters) {
  int local_var;
  while(1) {

   //Take mutex prior to critical section
    if (xSemaphoreTake(mutex, 0) == pdTRUE) {
     //Redundant way to do "shared var++" randomly and poorly 
      local_var = shared_var;
      local_var++;
      vTaskDelay(random(100,500) / portTICK_PERIOD_MS);
      shared_var = local_var;
      
      //Giave mutex after critical section
      xSemaphoreGive(mutex);
      
      //print out new shared variable
      Serial.println(shared_var);
    } else { 
      //Do something else if semaphore not available
    }
  }
}
void setup() {
  //Hack to kin of get reandomness
  randomSeed(analogRead(0));
  
  //configure serial
  Serial.begin(115200);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---Freertos Race condition Demo---");

  //Create mutex
  mutex = xSemaphoreCreateMutex();
  //Start task 1
  xTaskCreatePinnedToCore(incTask,
                          "Increment Task 1",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
  //Start task 2
  xTaskCreatePinnedToCore(incTask,
                          "Increment Task 2",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);  
  //V
  vTaskDelete(NULL);
}

void loop() {
  // put your main code here, to run repeatedly:

}
