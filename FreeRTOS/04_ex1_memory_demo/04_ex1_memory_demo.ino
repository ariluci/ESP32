/**
 * FreeRTOS Stack overflow demo 
 * 
 * Generate a stack overflow on purpose
 * 
 * Date:none
 * Author: https://www.youtube.com/watch?v=Qske3yZRW5I
 * License:
 * 
 * obs1: We might se the output print a few times but the processor resets after a 
 *       few moments indicating a stack overflow buffer
 * obs2: Stack canary watchpoint refers to the fact that the os set some know values
 *       to the last few bytes in stack and chcks them periodically. If they change 
 *       values from thous known values then eror is thrown and processor restes
 *       Safety chck to prvent stack overflow
 * obs3: to fix increase task stack size taking by extra bytes eg1024+(4*100)
 * 
 * obs4: use uTaskGetStackHighWaterMark(NULL) from freeRTOS api to let yoy
 *         know how many bytes you have left in task stack
 *         if aproacjes 0 you'll likely aproach stack overflow
 */
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//Task will store numbers in array and print one of them
//size of int = 4 bytes. 
//we only gave 1kb of stack
//600Bytes are overhead
//array b should take up more stack memory then we have left
void testTask(void *parameter) {
  while(1) {
    int a = 1;
    int b[100];
    for (int i = 0; i < 100; i++) {
      b[i] = a+1;
      
    }
    Serial. println(b[0]);

    //print remaining stack memory (bytes);
    Serial.print("High water mark words: ");
    Serial.println(uxTaskGetStackHighWaterMark(NULL));

    //Print out numbe rof free heap memory bytes before malloc
    Serial.print("Heap before malloc (bytes): ");
    Serial.println(xPortGetFreeHeapSize());

    int *ptr = (int*)pvPortMalloc(1024 * sizeof(int));

    if (ptr == NULL)
      Serial.println("Not enough heap.");
    else { 
      //Do something with mem so that compiler won't optimize it out
      for (int i = 0; i < 1024; i++) {
        ptr[i] = 3;
      }
    }
    //Print out number of free heap memory bytes after amalloc
    Serial.print("Heap after malloc (bytes): ");
    Serial.println(xPortGetFreeHeapSize());

    //Free up our allocated memory to prevent 
    vPortFree(ptr);
    //Wait fo a while
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void setup() {
  //Cobfugyre Serial
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Memory Demo---");

  //Start the only other task
  xTaskCreatePinnedToCore(testTask,
                          "Test task",
                          1024+400,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
   //Delete "setup and loop" task to make sure there;s only 1 task running
   vTaskDelete(NULL);   
}

void loop() {
  // put your main code here, to run repeatedly:

}
