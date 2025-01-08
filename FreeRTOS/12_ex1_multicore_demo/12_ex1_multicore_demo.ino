/**
 * ESP32 Multicore Demo
 * 
 * How to rin two tasks on different cores
 */

// Core definitions 
static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// Settings
static const TickType_t time_hog = 200; //Time (ms) hogging the CPU

//********************************************************************
// Functions

// Hogs the processor. Accurat to about 1 second (no promises).
static void hog_delay(uint32_t ms) {
  for (uint32_t i = 0; i < ms; i++) {
    for (uint32_t j = 0; j < 40000; j++) {
      asm("nop");  
    }  
  }
}
//********************************************************************
// Tasks

// Task L (low priority)
void doTaskL(void *parameters) {
  TickType_t timestamp;
  char str[20];

  // Do forever 
  while (1) {

    // Say something
    sprintf(str, "Task L, core %i\r\n", xPortGetCoreID());
    Serial.print(str);

    // Hog the processor for a while doing nothing (this is a bad idea)
    hog_delay(time_hog);
  }
}

// Task H (high priority)
void doTaskH(void *) {

  TickType_t timestamp;
  char str[20];
  
  while(1) {
    // Say something
    sprintf(str, "Task H, Core %i\r\n", xPortGetCoreID());
    Serial.print(str);

    // Hog the processor for a while while doing nothing (this is a bad idea)
    hog_delay(time_hog);
  }
}
//********************************************************************
// Main
void setup() {
  // Config serial
  Serial.begin(115200);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Multicore Demo---");
   Serial.println(LED_BUILTIN);
  /**
   * obs1 : if pro_cpu there is a watchdog timmer that activates every
   * few seconds , app_cpu should solve this
   * tskNO_AFFINITY tells task it can run on either core
  */
  // Start Task L (low priority)
  xTaskCreatePinnedToCore (doTaskL,
                           "Task L",
                           2048,
                           NULL,
                           1,
                           NULL,
                           tskNO_AFFINITY);
  // Start Task H (High priority)
  xTaskCreatePinnedToCore (doTaskH,
                           "Task H",
                           2048,
                           NULL,
                           2,
                           NULL,
                           tskNO_AFFINITY);
                         
                           
}

void loop() {
  // put your main code here, to run repeatedly:

}
