//Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

//Some string to print form one task
const char msg[] = "Barkadeer bring Arr booty rum.";

//Use handles for both tasks so we can control their states for a third task
static TaskHandle_t task_1 = NULL;
static TaskHandle_t task_2 = NULL;

//*************************************************
//Tasks
//Task:count no of char in str and print to serial terminal 1 by 1
 /* if Serial.print(msg) (the whole msg) it wouls jost copy 
  * the whole string to arduino  serial buff which would prevent 
  * us from seeing any interruption by the second task
  * That's why we loop the whole string 1by1 */
void startTask1(void *parameter) {
  
  //Count number of characters in string 
  int msg_len = strlen(msg);

  //Print string to terminal
  while(1) {
    Serial.println();
    for (int i = 0;  i < msg_len; i++) {
      Serial.print(msg[i]);
    }
    Serial.println();
    vTaskDelay(1000 / portTICK_PERIOD_MS); //Put in BLOCK state for 1 second.
                                    //After second is up it will be in READy state       
  }
}

//Task: print to serial terminal with higher priority
void startTask2(void *parameter) {
  while(1) {
    Serial.print('*');
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

//*****************************************************
//Main (runs as it;s own task with priority 1 on core 1)
void setup() {
  //Configure Serial (go slow so we can see preemption)
  Serial.begin(300); //Very slow baude rate
  //Wait a moment to start (to give esp32 a chance to connect with the serial)
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("--Freertos Task Demo--");
  //Print self priority
  Serial.print("Setup and loop task running on core");
  Serial.print(xPortGetCoreID());
  Serial.print(" with priority ");
  Serial.println(uxTaskPriorityGet(NULL));

  //Tasks to run forever
  xTaskCreatePinnedToCore(startTask1,
                          "Task 1",
                          1024,
                          NULL,
                          1,
                          &task_1,
                          app_cpu);
  xTaskCreatePinnedToCore(startTask2,
                          "Task 2",
                          1024,
                          NULL,
                          2,
                          &task_2,
                          app_cpu);
}

void loop() {
  //Note: we're running a 3rd task to controll the other 2 tasks
  //Suspend the higher priority task 2 for some intervals
  //Suspend task2 every 2 seconds
  for(int i = 0; i < 3; i++) {
    vTaskSuspend(task_2);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    vTaskResume(task_2);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
  //After this use vTaskDelete to completely remove lower priority task1
  if(task_1 != NULL) {
    vTaskDelete(task_1);
    task_1 = NULL;
  }
}
