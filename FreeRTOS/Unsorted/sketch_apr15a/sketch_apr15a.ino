// Use only 1 for demo purposes 
#if CONFIG_FREERTO_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

#define BUF_LEN 10
#define MSG_LEN 100
#define MSG_QUEUE_LEN 5
#define CMD_BUF_LEN 255

//Settings
static const char command[] = "avg";
static const uint16_t timer_divider  = 8;  // 10MHz
static const uint64_t timer_max_count = 1000000;
static const uint32_t cli_delay = 20; //ms delay

//Pins
static const int adc_pin = A0;

//Message struct to wrap strings for queue
typedef struct Message {
  char body[MSG_LEN];
}Message;

// Globals
static hw_timer_t *timer = NULL;                  //hw timer used to gen interrupt   
static TaskHandle_t processing_task = NULL;       //Used by isrto notify task
static SemaphoreHandle_t sem_done_reading = NULL; //used to signalt that avgTask done with buf
static portMUX_TYPE spinlock  = portMUX_INITIALIZER_UNLOCKED; 
static QueueHandle_t msg_queue;
static volatile uint16_t buf_0[BUF_LEN];
static volatile uint16_t buf_1[BUF_LEN];
static volatile uint16_t* write_to = buf_0;
static volatile uint16_t* read_from = buf_1;
static volatile uint8_t buf_overrun = 0;
static float adc_avg;

//*****************************************************************
//Functions
void IRAM_ATTR swap() {
  volatile uint16_t *tmp_ptr = write_to;
  write_to = read_from;
  read_from = tmp_ptr;
}
//*****************************************************************
//ISRs
void IRAM_ATTR onTimer() {
  
  static uint16_t idx = 0;
  BaseType_t task_woken = pdFALSE;

  if ((idx < BUF_LEN)&&(buf_overrun == 0)) {
    write_to[idx] = analogRead(adc_pin);
    idx++;
  }

  if (idx >= BUF_LEN) {
    if (xSemaphoreTakeFromISR(sem_done_reading, &task_woken)==pdFALSE) {
      buf_overrun = 1;
    } 

    if (buf_overrun == 0) {
      idx = 0;
      swap();
      vTaskNotifyGiveFromISR(processing_task, &task_woken);
    } 
  }

  //Exit form ISR
  if (task_woken)
    portYIELD_FROM_ISR();
  
}
//*****************************************************************
//Tasks

void calcAverage(void *parameters) {
   Message msg;
   float avg;

  while(1) {
    // Wait form notification from isr ; similar to bin sem, blocking
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    avg = 0.0;
    for (int i = 0; i<BUF_LEN; i++) {
      avg += (float)read_from[i];
      //vTaskDelay(105/portTICK_PERIOD_MS);
    }
    avg  /= BUF_LEN;
    
    // Updating the shared float may or may not take multiple instructions
    // so we protect it with a mutex or critical section. The ESP-IDF critical
    // section is the easiest for this aplication.
    portENTER_CRITICAL(&spinlock);
    adc_avg  = avg;
    portEXIT_CRITICAL(&spinlock);

    // if we take too long we might have overrun 
    //we print a msg 
    if (buf_overrun == 1) {
      strcpy(msg.body, "Error:Buffer ovverun. Samples have been dropped.");
      xQueueSend(msg_queue, (void *)&msg, 10);  
    }

    //Clearing the buf overun and giving the done semaphore must
    // be done wihtout being interupted
    portENTER_CRITICAL(&spinlock);
    buf_overrun = 0;
    xSemaphoreGive(sem_done_reading);
    portEXIT_CRITICAL(&spinlock);
  }
}
//serial termina
void doCLI (void *parameters) {
  Message rcv_msg;
  char c;
  char cmd_buf[CMD_BUF_LEN];
  uint8_t idx = 0;
  uint8_t cmd_len = strlen(command);

  //Clear whole buffer
  memset(cmd_buf,0,CMD_BUF_LEN);

  while(1) {
    if (xQueueReceive(msg_queue, (void*)&rcv_msg, 0)==pdTRUE){
      Serial.println(rcv_msg.body);
    }

    if (Serial.available() >  0) {
      c = Serial.read();
      //Store rcv char ot buf if nod over buf limit
      if (idx < CMD_BUF_LEN -1) {
        cmd_buf[idx] = c;
        idx++;
      }

      if ((c=='\n')||(c=='\r')) {
        Serial.print("\r\n");

        //print avg
        cmd_buf[idx-1]='\0';
        if(strcmp(cmd_buf, command) == 0) {
          Serial.print("Average:");
          Serial.println(adc_avg);
        }

        //Reset receive buffer and index counter
        memset(cmd_buf,0,CMD_BUF_LEN);
        idx = 0;
      } else {
        Serial.print(c);
      }
    }
      //Don't hog the CPU. Yield to other tasks for a while
    vTaskDelay(cli_delay / portTICK_PERIOD_MS);
  }
}
//*****************************************************************
//Main
void setup() {
      // Config serial
  Serial.begin(115200);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Sample and Process Demo---");

    // Create semaphore before it is used (in task or ISR)
  sem_done_reading = xSemaphoreCreateBinary();
  
  // Core reboot if we can't create semaphore
  if (sem_done_reading == NULL) {
    Serial.println("Could not create semaphore");
    ESP.restart();
  } 
  
  // We want the done reading semaphore to initialize to 1
  xSemaphoreGive(sem_done_reading);

  // Create msg queue before it is used
  msg_queue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message)); 
  // Start task to handle command line interface events
  // Let's set it at a higher priority but only run it once every 20ms
  xTaskCreatePinnedToCore(doCLI,
                          "Do CLI",
                          1024,
                          NULL,
                          2,
                          NULL,
                          app_cpu);
                          
  // Start task to calculate average. Save the handle to use with notifications.
  xTaskCreatePinnedToCore(calcAverage,
                          "Calculate average",
                          1024,
                          NULL,
                          1,
                          &processing_task,
                          app_cpu);
   
  // Create and start timer to run ISR every 100 ms(num, divider, countUp)
  timer = timerBegin(0, timer_divider, true);

  // Provide ISR to timer (timer, function, edge)
  timerAttachInterrupt(timer, &onTimer, true);

  // At what count should ISR trigger (timer, count, autoreload)
  timerAlarmWrite(timer, timer_max_count, true);

  // Allow ISR to trigger
  timerAlarmEnable(timer);

  
  //Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // put your main code here, to run repeatedly:

}
