/*
 * if you pass a local variable, there is a very good chance 
 * that the calling function will end  and variable will go out of scope
 * before the newly created task gets a chance to copy it into its own local 
 * variable 
*/
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif
//Globals
static const int led_pin = LED_BUILTIN;
static SemaphoreHandle_t mutex;
//*********************************************************
//Tasks

//Task blink led task on rate passed by parameter
void blinkLED(void *parameters) {
  int num = *(int*)parameters;
  xSemaphoreGive(mutex);
  //Print the parameter
  Serial.print("Received from console:");
  Serial.println(num);
  
  //Configure LED pin
  pinMode(led_pin, OUTPUT);
  while(1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(num / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(num / portTICK_PERIOD_MS);
  }
}
void setup() {
  long int delay_arg;

  //Configure serial
  Serial.begin(115200);
  //Wait a moment to connect to console
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println();
  Serial.println("---FreeRTOS Mutex Soulution---");
  Serial.println("Enter a number for delay in ms:");

  //Wait for input from serial
  while (Serial.available() <= 0);

  //Read integer value

  delay_arg = Serial.parseInt();
  Serial.print("Sending: ");
  Serial.println(delay_arg);
  
  //Create mutex
  mutex = xSemaphoreCreateMutex();
  
  // Take the mutex
  xSemaphoreTake(mutex, portMAX_DELAY);
  
  //Start Task 1
  xTaskCreatePinnedToCore(blinkLED,
                          "Blink LED",
                          1024,
                          (void *)&delay_arg,
                          1,
                          NULL,
                          app_cpu);
                          
   // Do nothing until mutex has been returned (maximum delay)
   xSemaphoreTake(mutex, portMAX_DELAY);
    
  //Show that we accomplished our task of passing the stack-based argument
  Serial.println("Done");
}

void loop() {
  // put your main code here, to run repeatedly:
    vTaskDelay(1000/portTICK_PERIOD_MS);
}
