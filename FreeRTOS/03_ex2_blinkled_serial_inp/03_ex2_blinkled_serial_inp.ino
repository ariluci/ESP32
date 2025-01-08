#include <stdlib.h> //needed for atoi()

//Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNCORE
static const BaseType_t app_cpu = 0;
#else
static const BaseType_t app_cpu = 1;
#endif

//Globals
static int led_delay = 500; //ms
static const uint8_t buf_len = 20;

//Pins
static const int led_pin = LED_BUILTIN;

//***********************************************
//Tasks
//Task:Blink led at rate set by global variable
void toggleLED(void* parameter)
{
  while(1) {
    digitalWrite(led_pin, HIGH);
    vTaskDelay(led_delay / portTICK_PERIOD_MS);
    digitalWrite(led_pin, LOW);
    vTaskDelay(led_delay / portTICK_PERIOD_MS);
  }
}

//Task:Read from serial terminal
void readSerial(void *parameters) {
  char c;
  char buf[buf_len];
  uint8_t idx = 0;

  //Clear whole buffer
  memset(buf, 0, buf_len);

  while(1) {
    //Read chars from serial
    if(Serial.available() > 0) {
      c = Serial.read();
      //Update delay variable and reset buffer if we get a newline character
      if (c == '\n') {
        led_delay  = atoi(buf);
        Serial.print("Updated LED delay to: ");
        Serial.println(led_delay);
        memset(buf, 0, buf_len);
        idx = 0;
      } else {
        //Only happen if index is not over message limit
        if (idx < buf_len - 1) {
          buf[idx] = c;
          idx++;
        }
      }
    }
  }
}

//*****************************************************
//Main
void setup() {
  //Configure oin
  pinMode(led_pin, OUTPUT);

  //Configure serial and wait a second
  Serial.begin(115200);
  vTaskDelay(1000/portTICK_PERIOD_MS);
  Serial.println("Multi-task LED Demo");
  Serial.println("Enter a number in milliseconds to change the LED delay:");

  //Create and start blink task
  xTaskCreatePinnedToCore ( //Use xTaskCreate in vanilla FreeRTOS
       toggleLED,           //Function to be called
       "Toggle LED",        //Name of task   
       1024,                //Stack size
       NULL,                //Parameter to pass
       1,                   //Task priority
       NULL,                //Task handle
       app_cpu);     
  //Create and start read from serial task
  xTaskCreatePinnedToCore ( 
      readSerial,           //Function to be called
      "Read from Serial",   //Name of task
      1024,                 //Stack size
      NULL,                 //Parameter to pass  
      1,                    //Task priority(!must be same to prevent lockup) 
      NULL,                 //Task handle 
      app_cpu);             //Run on one core for demo purposes
}

void loop() {
  // put your main code here, to run repeatedly:

}
