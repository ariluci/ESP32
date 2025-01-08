 #if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Settings
static const uint8_t msg_queue_len = 5;

//Globals
static QueueHandle_t msg_queue;

//********************************************************************************
//Tasks

//Task: wait for item on queue and print it
void printMessages(void *parameters) {
  int item;

  while(1) {
/*    msg_queue - handle to the queue; the global var
 *    item      - address of the local variable where the queue item will be copied to
 *   0         - timeout in number of ticks. thaks will go in BLOCK state this many
 *               number of ticks while waiting for something to appear in the queue
 *               if 0 it will return  imediayley, giving us pdTRUE if something was
 *               read from the queue or pdFALSE if not 
 */
    if(xQueueReceive(msg_queue, (void *)&item, 0) == pdTRUE){
//        Serial.println(item);
    }
    Serial.println(item);  
    //Wait before trying again
    // if 100 not 1000 it will read faster than writing
    vTaskDelay(1000/ portTICK_PERIOD_MS);
  }
}
//********************************************************************************
//Main (runs as it's own task with priority 1 in core 1)
void setup() {
  //Configure serial
  Serial.begin(115200);
  vTaskDelay(1000/portTICK_PERIOD_MS); //wait to connect with serial
  Serial.println();
  Serial.println("---FreeRTOS Queue Demo---");
  
/* msg_queue     - set handle to retunr value 
 * msg_queue_len - number of items (5)
 *  sizeof(int) - size of each item
 */
  //Create queue
  msg_queue = xQueueCreate(msg_queue_len, sizeof(int));

  //Start print task
  xTaskCreatePinnedToCore(printMessages,
                          "Print Messages",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
}

void loop() {
  static int num = 0;
  
  /* try to add item to queue for 10 ticksm fail if queue is full
   * give queue handle, address of copunter (explicitly cast as void ptr)
   * timeout after 10 ticks
   */
  if(xQueueSend(msg_queue, (void*)&num, 10) != pdTRUE) {
    Serial.println("Queue full");
  }
  num++;
  /* Wait before trying again
   * if delay  < 1000 we are writing faster than reading =>
   * =>queue will get filled and move to next value 
   */
  //program will drop items it can't addd to the queue
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
