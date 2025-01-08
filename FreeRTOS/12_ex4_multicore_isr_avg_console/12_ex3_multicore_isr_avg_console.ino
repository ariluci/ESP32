/*
 * ESP32 Sample and Process Solution
 * 
 * Sample ADC in an ISR, process in a task, use multicore
 * 
 * 
 */
// Core definitions 
static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// Settings
static const char command[] = "avg";             // Command
static const uint16_t timer_divider = 8;         // Count at 10MHz
static const uint64_t timer_max_count = 1000000; //Timer counts to this  
static const uint32_t cli_delay = 20;            //(ms)
enum {BUF_LEN = 10 };       //Number of elements in smaple buffer
enum {MSG_LEN = 100};       //Max characters in msg body
enum {MSG_QUEUE_LEN = 5};  //Number of slots in message queue
enum {CMD_BUF_LEN = 255};   //Number of characters in command buffer

// Pins
static const int adc_pin = A0;

// Msg struct to wrap strings for queue
typedef struct Message {
  char body[MSG_LEN];
}Message;

//Globals
static hw_timer_t *timer = NULL;  //hw timer used to gen interrupt
static TaskHandle_t processing_task = NULL; //handler for avg task
static SemaphoreHandle_t sem_done_reading = NULL; //avg task -> timer
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
static QueueHandle_t msg_queue; // avg if buf overun -> doCLI
static volatile uint16_t buf_0[BUF_LEN];      //1st buffer in the pair
static volatile uint16_t buf_1[BUF_LEN];      //2nd buffer in the pair
static volatile uint16_t* write_to  = buf_0;  //Double buffer write pointer
static volatile uint16_t* read_from = buf_1;  //Double buffer read pointer
static volatile uint8_t buf_overrun = 0;      //Double buffer overrun flag
static float adc_avg;
//#include semphr.h

//************************************************************************
// Functions that can be called from anywhere in this file

//Swap the write_to and read_from pointers in the double buffer
//Only ISR calls this at this moment so you need to make it thread-safe
void IRAM_ATTR swap() {
  volatile uint16_t *tmp_ptr = write_to;
  write_to  = read_from;
  read_from = tmp_ptr; 
}

//************************************************************************
// Interrupt Service Routine (ISR)
void IRAM_ATTR onTimer() {

  static uint16_t idx = 0;
  BaseType_t task_woken = pdFALSE;
  
  // If buffer is not overrun, read Adc and write to next buff elem
  // If buffer is overrun, drop the sample
  if ((idx < BUF_LEN) && (buf_overrun == 0)) {
    write_to[idx] = analogRead(adc_pin);
    idx++;
  }
  
  // Check if buffer is full
  if (idx >= BUF_LEN) {
    
    // If reading is not done, set the overrun flag.
    // we don't need to set this as a ciritical section 
    // as nothing can interrupt and change either value
    if (xSemaphoreTakeFromISR(sem_done_reading, &task_woken) == pdFALSE) {
      buf_overrun = 1;
    }  
  }
  
  // Only swap buffers and notify task if overrun flag is cleared
  if (buf_overrun == 0) {
    //Reset index and swap buffer pointers
    idx = 0;
    swap();
    //A task notification works like a binary semaphore but is faster
    //Notify avg task to wake up
    vTaskNotifyGiveFromISR(processing_task, &task_woken);
  }  

  // Exist from ISR
  if (task_woken)
    portYIELD_FROM_ISR();  // ESP-IDF FreeRTOS

//  portYIELD_FROM_ISR(task_woken); //Vanilla FreeRTOS
}
//**************************************************************************
// Tasks
// Task calcAvg: watit for semaphore and calc avg of ADC values
void calcAverage(void *parameters) {
  Message msg;
  float avg;
  
  // %%% We move this here so it runs in core 0
  /* Create and stat timer to run ISR every 100 ms(idxTimer, div, countUP)*/
  timer = timerBegin(0, timer_divider, true);
  /* Provide ISR to timer (time, function, edge) */
  timerAttachInterrupt(timer, &onTimer, true);
  /* At what count should ISR trigger (timer, count, autoreload)*/
  timerAlarmWrite(timer, timer_max_count, true);
  /* Allow ISR to trigger */
  timerAlarmEnable(timer);
  
  while(1) {
    
    // Wait for the notification from ISR(sinilar to binary sem)
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    
    // Calc average
    avg = 0.0;
    for (int i = 0; i < BUF_LEN; i++) {
      avg += (float)read_from[i];
      //vTaskDelay(105/portTICK_PERIOD_MS); // Test buf overrun
    }
    avg /= BUF_LEN; 

    /* Updating the shared float may or may not take multiple instr
     * We protect it with with mutex or critical section */
    portENTER_CRITICAL(&spinlock);
    adc_avg = avg;
    portEXIT_CRITICAL(&spinlock);

    /* If we took too long to proces, writing buf will have overrun
     * So we send a msg to be printed out to the serial terminal */
    if (buf_overrun == 1) {
      strcpy(msg.body, "Error: Buffer overrun. Samples have been dropped.");
      xQueueSend(msg_queue, (void*)&msg, 10);
    }

    /* Clear overrun flag and give the "done reading" semaphore
     * must be done together without being interrupted */
    portENTER_CRITICAL(&spinlock);
    buf_overrun = 0;
    xSemaphoreGive(sem_done_reading);
    portEXIT_CRITICAL(&spinlock);
  }
}

//Task doCli: serial terminal task
void doCLI (void *paramters) {

  Message rcv_msg;
  char c;
  char cmd_buf[CMD_BUF_LEN];
  uint8_t idx = 0;
  uint8_t cmd_len = strlen(command);
  
  /* Clear whole buffer */
  memset(cmd_buf, 0, CMD_BUF_LEN);

  /* Loop forever */
  while(1) {
    /* Look for any error messages that need to be printed */
    if (xQueueReceive(msg_queue, (void*)&rcv_msg, 0) == pdTRUE){
      Serial.println(rcv_msg.body);
    }

    /* Read characters from serial */
    if (Serial.available() > 0 ) {
      c = Serial.read();
      
      /* Store received character to buffer if not over buffer limit */
      if (idx < CMD_BUF_LEN - 1) {
        cmd_buf[idx] = c;
        idx++;
      }

      /* print newline and check input for 'enter' */
      if ((c == '\n') || (c == '\r')) {
        
        /* print newline to terminal */
        Serial.print("\r\n");

        /* Print average value if command given is "avg" */
        cmd_buf[idx - 1] = '\0';
        if (strcmp(cmd_buf, command) == 0) {
          Serial.print("Average:");
          Serial.println(adc_avg);
        }

        /* Reset receive buffer and index counter */
        memset(cmd_buf, 0, CMD_BUF_LEN);
        idx = 0;      
      /* otherwise , echo character back to serial terminal */
      } else {
        Serial.print(c);
      }
    }

    /* Don't hog the CPU. Yield to other tasks or a while */
    vTaskDelay(cli_delay / portTICK_PERIOD_MS);
  }
} 

//**********************************************************************
// Main
void setup() {
  /* Config serial */
  Serial.begin(115200);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Sample and Process Multicore Demo---");

  /* Create done_read semaphore before it is used in task or ISR */
  sem_done_reading = xSemaphoreCreateBinary();

  /* Core reboot if we can't create the semaphore */
  if (sem_done_reading == NULL) {
    Serial.println("Could not create semaphore.");
    ESP.restart();
  }

  /* We want the done reading to initialize to 1 */
  xSemaphoreGive(sem_done_reading);

  /* Create msgqueue before it is used */
  msg_queue = xQueueCreate(MSG_QUEUE_LEN, sizeof(Message));

  /* Start task to handle command line interface events */
  /* Let's set it at a higher priority but only run it once every 20ms */
  xTaskCreatePinnedToCore(doCLI,
                          "Do CLI",
                          1024,
                          NULL,
                          2,
                          NULL,
                          app_cpu);
  /* Start task to calculate average. 
   * Save the handle to use with notification */                        
  xTaskCreatePinnedToCore(calcAverage,
                          "Calculate average",
                          1024,
                          NULL, 
                          1,
                          &processing_task,
                          pro_cpu);


  /* Delete setup and loop task */
  vTaskDelete(NULL);
}

void loop() {
  /* Execution should never get here */
}
