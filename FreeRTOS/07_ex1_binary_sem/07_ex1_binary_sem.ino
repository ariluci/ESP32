//#include "semphr.h" in vanilla FreeRTOS
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

//pins
static int led_pin = LED_BUILTIN;
//Global data
static int shared_var = 0;
//static SemaphoreHandle_t mutex;
static SemaphoreHandle_t bin_sem;

//**********************************
//Tasks 
//Task: increment shared variable(the wrong way)
void blinkLED(void* parameters) {
  
  //Copy the parameter into a local variable
  int num = *(int *)parameters;
  
/* //Release the mutex so that the creating gunction can finish  
*  xSemaphoreGive(mutex); */

  //Release the binary semaphore so that the creating function can finish
  xSemaphoreGive(bin_sem);
  
  //Print the parameter 
  Serial.print("Received:");
  Serial.println(num);
  
  //Configure tje LED pin
  pinMode(led_pin, OUTPUT);
  
  while(1) {
    digitalWrite(led_pin,HIGH);
    vTaskDelay(num / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(num / portTICK_PERIOD_MS);
  }
}

void setup() {
   long int delay_arg; 

  //Configure Serial
  Serial.begin(115200);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Semaphore solution---");
  Serial.println("Enter a number(in ms) for delay: ");

  //Wait for input from serial
  while(Serial.available() <= 0);

  //Read integer value
  delay_arg = Serial.parseInt();
  Serial.print("Sending:");
  Serial.println(delay_arg);
   
/* //Crate mutex 
 * mutex = xSemaphoreCreateMutex(); 
 * // Take the mutex
 *  xSemaphoreTake(bin_sem, portMAX_DELAY); */

  //Create binary semaphore
  bin_sem = xSemaphoreCreateBinary();
/* bin_sem is initialized to 0 so No need to take the 
 * semaphore before creating task */ 

  //Create task
  xTaskCreatePinnedToCore(blinkLED,
                          "Blink LED",
                          1024,
                          (void*)&delay_arg,
                          1,
                          NULL,
                          app_cpu); 
  //Block setup function until exiting parameter has been read                          
/*  xSemaphoreTake(mutex, portMAX_DELAY); */                 
  xSemaphoreTake(bin_sem, portMAX_DELAY);
  
  //Show that we accomplished our task of passing the stack base argument;
  Serial.println("Done!");
} 

void loop() {
   // put your main code here, to run repeatedly:
    vTaskDelay(1000/portTICK_PERIOD_MS);
}
