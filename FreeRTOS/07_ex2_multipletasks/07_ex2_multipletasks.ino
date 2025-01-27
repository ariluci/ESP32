//#include "semphr.h" in vanilla FreeRTOS
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Settings 
const int num_tasks = 5;  //Number of task to create

//Example struct for massing a string as parameter 
typedef struct Message {
  char body[20];
  uint8_t len;
}Message;

//Globals
static SemaphoreHandle_t sem_params; //Counts down when parameters read
static SemaphoreHandle_t mutex; //protct serial shared resource

//************************************************************************
//Tasks
//Task:

void myTask(void *parameters) {
  //Copy the message struct fron the parameter to a local variable
  Message msg = *(Message *)parameters;

  //Increment semaphore to indicate that the parameter has been read
  xSemaphoreGive(sem_params);

  xSemaphoreTake(mutex, portMAX_DELAY);
  //Print out message contents
  Serial.print("Received: ");
  Serial.print(msg.body);
  Serial.print(" | len: ");
  Serial.println(msg.len);
  xSemaphoreGive(mutex);
  
  //Wait for a wile and delete self
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  vTaskDelete(NULL);
} 

//**************************************************************
//Main (runs as its own task with priority 1 on core 1)
void setup() {
  char task_name[12];
  Message msg;
  char text[20] = "All your base";
  
  Serial.begin(115200);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Semaphore Count demo---");

  //Create mutex  (initialize at 0)
  mutex = xSemaphoreCreateMutex();
  xSemaphoreGive(mutex);
  
  //Create semaphores (initializes at 0)
  sem_params = xSemaphoreCreateCounting(num_tasks, 0);
  
  //Create message to ause as argument common to all tasks
  strcpy(msg.body, text);
  msg.len = strlen(text);
  
  //Start tasks
  for(int i = 0; i < num_tasks; i++) {
    //Generate unique name string for task
    sprintf(task_name, "Task %i", i);

    //Start task and pass argument (common message struct)
    xTaskCreatePinnedToCore(myTask,
                            task_name,
                            1024,
                            (void *)&msg,
                            1,
                            NULL,
                            app_cpu);
    
  }
  for (int i = 0; i < num_tasks; i++) {
    xSemaphoreTake(sem_params, portMAX_DELAY);
  }
  //Notify that all tasks have been created
  
  xSemaphoreTake(mutex, portMAX_DELAY);
  Serial.println("All tasks created");
  xSemaphoreGive(mutex);
}

void loop() {
 // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
