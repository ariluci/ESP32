/**
 * ESP32 Multicore Spinlock demo
 * 
 * Demonstration of critical section and ISR with multicore processors.
 */
// COre definitions
static const BaseType_t pro_cpu = 0;
static const BaseType_t app_cpu = 1;

// Settings
static const TickType_t time_hog = 1;   // Time(ms) hogging the CPU in Task1
static const uint32_t task_0_delay = 30;  // Time(ms) task 0 blocks itself
static const uint32_t task_1_delay = 100; // Time(ms) task 1 blocks itself


// pins
static const int pin_0 = LED_BUILTIN; //LED pin
static const int pin_1 = LED_BUILTIN+1; //LED pin

// Globals
static portMUX_TYPE spinlock = portMUX_INITIALIZER_UNLOCKED;
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
//**************************************************************************
//Tasks
//Task in Core 0
void doTask0(void *parameters) {
  
  // Configure pin
  pinMode(pin_0, OUTPUT);
  
  // Do forever 
  while(1) {

    // Toggle pin
    portENTER_CRITICAL(&spinlock);
    digitalWrite(pin_0, !digitalRead(pin_0));
    portEXIT_CRITICAL(&spinlock);
    // Yield processor for a while
    vTaskDelay(task_0_delay / portTICK_PERIOD_MS);
  }
}

//Task in Core 1
void doTask1(void *parameters) {

  // Configure pin
  pinMode(pin_1, OUTPUT);
  
  // Do forever 
  while(1) {

    // Do some long critical section (this is bad practice)
    portENTER_CRITICAL(&spinlock);
    digitalWrite(pin_1, HIGH);
    hog_delay(time_hog);
    digitalWrite(pin_1, LOW);
    portENTER_CRITICAL(&spinlock);

    // Yield proc for a while
    vTaskDelay(task_1_delay / portTICK_PERIOD_MS);
  }
}
void setup() {  
  
  // Start Task 0 
  xTaskCreatePinnedToCore (doTask0,
                           "Task 0",
                           2048,
                           NULL,
                           1,
                           NULL,
                           pro_cpu);
  // Start Task  1
  xTaskCreatePinnedToCore (doTask1,
                           "Task 1",
                           2048,
                           NULL,
                           1,
                           NULL,
                           app_cpu);
}

void loop() {
  // put your main code here, to run repeatedly:

}
