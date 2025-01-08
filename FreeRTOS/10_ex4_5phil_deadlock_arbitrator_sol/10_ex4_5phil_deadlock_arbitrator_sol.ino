// Use only 1 for demo purposes 
#if CONFIG_FREERTO_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
enum { NUM_TASKS = 5 };           // Number of tasks (philosophers)
enum { TASK_STACK_SIZE = 2048 };  // Bytes in ESP32, words in vanilla FreeRTOS  

// Globals
static SemaphoreHandle_t bin_sem;  // Wait for parameters to be read
static SemaphoreHandle_t done_sem; // Notifies main task when done 
static SemaphoreHandle_t chopstick[NUM_TASKS] ;
static SemaphoreHandle_t arbitrator_mutex ;

//*****************************************************************************
//Tasks
void eat(void *parameters) {
  int num;
  char buf[50];

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);
  if(xSemaphoreTake(arbitrator_mutex, portMAX_DELAY) == pdTRUE){
    // Take left chopstick
    xSemaphoreTake(chopstick[num], portMAX_DELAY);
    sprintf(buf, "Philosopher %i took chopstick %i", num, num);
    Serial.println(buf);
  
    // Add some delay to force deadlock
    vTaskDelay(1 / portTICK_PERIOD_MS);
  
    // Take Right chopstick
    xSemaphoreTake(chopstick[(num+1)%NUM_TASKS], portMAX_DELAY);
    sprintf(buf, "Philosopher %i took chopstick %i", num, (num+1)%NUM_TASKS);
    Serial.println(buf);
  
    // Do some eating
    sprintf(buf, "Philosopher %i is eating", num);
    Serial.println(buf);
    vTaskDelay(10 / portTICK_PERIOD_MS);
  
    // Put down right chopstick
    xSemaphoreGive(chopstick[(num+1)%NUM_TASKS]);
    sprintf(buf, "Philosopher %i returned chopstick %i", num , (num+1)%NUM_TASKS);
    Serial.println(buf);
  
    // Put down left chopstick
    xSemaphoreGive(chopstick[num]);
    sprintf(buf, "Philosopher %i returned chopstick %i", num, num);
    Serial.println(buf);
    xSemaphoreGive(arbitrator_mutex);
  } else {
    sprintf(buf, "Philosopher %i denied taking first chopstick", num);
    Serial.println(buf);
  }
  // Notify main task and delete self
  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}
//*****************************************************************************
void setup() {
  char task_name[20];

  // Configure Serial
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Dining Philosophers Challange---");
  
  // Create kernel objects befor starting tasks
  bin_sem = xSemaphoreCreateBinary();
  done_sem = xSemaphoreCreateCounting(NUM_TASKS, 0);
  
  for (int i = 0; i < NUM_TASKS; i++) {
    chopstick[i] = xSemaphoreCreateMutex();
  }
  arbitrator_mutex = xSemaphoreCreateMutex();
  // Have the philosophers start eating
  for (int i = 0; i < NUM_TASKS; i++) {
    sprintf(task_name, "Philosopher %i", i);
    xTaskCreatePinnedToCore(eat,
                            task_name,
                            TASK_STACK_SIZE,
                            (void*)&i,
                            1,
                            NULL,
                            app_cpu);
    xSemaphoreTake(bin_sem, portMAX_DELAY);  
  }

  // Wait until all the philosophers are done
  for (int i = 0; i < NUM_TASKS; i++) {
    xSemaphoreTake(done_sem, portMAX_DELAY);
  }

  // Say that we made it through without deadlock
  Serial.println("Done! No deadlock occurred!");
}

void loop() {
  // put your main code here, to run repeatedly:

}
