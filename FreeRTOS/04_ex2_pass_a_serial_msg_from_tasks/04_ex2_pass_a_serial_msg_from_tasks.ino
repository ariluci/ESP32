/*
 * Create 2 task that mimicks a serial echo task
 * Task 1: - listens for input from serial monitor
 *         - on newlinechar ('\n'), stores all chars 
 *           up to that poin in heap memory
 *         - tells task be of new msg
 * Task 2: - waits for notification from Task A        
 *         - pritns message found in heap memory to serial monitor
 *         - frees heap memory
 */
 #if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Settings 
static const uint8_t buf_len = 255;

//Globals
static char *msg_ptr = NULL;
static volatile uint8_t msg_flag = 0;
//*****************************************************
//Tasks

//Task:read message from serial buffer
void readSerial(void *parameters) {
  char c;
  char buf[buf_len];
  uint8_t idx = 0;
  
  //Clar whole buffer
  memset(buf,0,buf_len);
  while(1) {
    //Read characters from serial
    if (Serial.available() > 0) {
      c = Serial.read();
      
      //store received character to buff if not over buff limit
      if (idx < buf_len-1) {
        buf[idx] = c;
        idx++;
      }
      
      if (c == '\n') {
        //The last character in the string is '\n'. 
        //We need to replace it with '\0' to make it null-terminated
        buf[idx-1]='\0';

        //Try to allocate memory and copy over message. If massage buffer
        //is still in use , ignore entire message
        if (msg_flag == 0) {
          msg_ptr = (char*)pvPortMalloc(idx*sizeof(char));
          
          //if malloc returns 0 (out of memort), thorw an error and reset
          configASSERT(msg_ptr);

          //Copy message
          memcpy(msg_ptr,buf,idx);

          //Notify other task that message is ready
          msg_flag  = 1;
        } 

        //Reset receive buffer and index counter
        memset(buf,0,buf_len);
        idx = 0;        
      }
    }  
  }
}
void printMessage(void *parameters) {
  while(1){
    //wait for flag to be set and print msg
    if(msg_flag == 1) {
      Serial.println(msg_ptr);
      
      //Give amount of free heap memory
//      Serial.print("Free heap(bytes): ");
//      Serial.println(xPortGetFreeHeapSize());
      //Freebuffer, set pointer to null and clear flag
      vPortFree(msg_ptr);
      msg_ptr = NULL;
      msg_flag = 0;  
    }
  }
}
void setup() {
  //Configure Serial
  Serial.begin(115200);

  //Wait a moment to start(so we dont miss serial output)
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Heap Demo---");
  Serial.println("Enter a string"); 

  xTaskCreatePinnedToCore(readSerial,
                          "Read Serial",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
  xTaskCreatePinnedToCore(printMessage,
                          "Print msg",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);    
  //Delete "setup and loop" task
  vTaskDelete(NULL);                
}

void loop() {
  // Execution should never get here beacause  vTaskDelete(NULL);  

}
