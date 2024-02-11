
#include <FlexCAN_T4.h>
#include "USBHost_t36.h"
#include "arduino_freertos.h"
#include "semphr.h"
SemaphoreHandle_t xBinarySemaphore;


#include <EEPROM.h>
 #include <SD.h>
#include <SPI.h>
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> Can0;
#define USBBAUD 115200
uint32_t baud = USBBAUD;
uint32_t format = USBHOST_SERIAL_8N1;
USBHost myusb;
USBSerial userial(myusb);  // works only for those Serial devices who transfer <=64 bytes (like T3.x, FTDI...)
USBDriver *drivers[] = {&userial};
#define CNT_DEVICES (sizeof(drivers)/sizeof(drivers[0]))
const char * driver_names[CNT_DEVICES] = {"USERIAL1" };
unsigned int counter, filecounter;
unsigned long currentTimestamp;
int previous_millis, current_millis;
unsigned int SUS1;
unsigned int SUS2;
float t;
int variablecount;
String Data;
static  bool counterUpdated = false; 
char filename[20];
char filename2[20];
char filename3[20];
const int chipSelect = BUILTIN_SDCARD;
File vectornav;
File CanFile;
void setup() {
  //Serial3.begin(11520);
  Serial.begin(11520);
  pinMode(13,OUTPUT);
  Can0.begin();
  Can0.setBaudRate(500000);
  Can0.setMaxMB(16);
  Can0.enableFIFO();
  Can0.enableFIFOInterrupt();
  Can0.onReceive(canSniff);
  Can0.mailboxStatus();
  pinMode(chipSelect,OUTPUT);
  myusb.begin();
  getUniqueFilename(filename, "Vectornav_%lu.csv");
  getUniqueFilename(filename2, "Front_%lu.csv");
  getUniqueFilename(filename3, "Rear_%lu.csv");
   if (!SD.begin(chipSelect)) {
    //Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  xBinarySemaphore = xSemaphoreCreateBinary();
  xTaskCreate(vectornavdata, "Vectornavdata",100,NULL,1,NULL);
  xTaskCreate(candata, "Candata", 100,NULL,2,NULL);
  xSemaphoreGive(xBinarySemaphore);
}
  
void loop(){}
void vectornavdata(void *pvParameters)//start of main task
{
  
  while(1)
  {
     Can0.events();
     String tokens[16];
    userial.begin(baud, 8);
  while (userial.available()) {
  Can0.events();
  String Data = userial.readStringUntil('\n');
  if (Data.startsWith("$VNISE")) {
    int variablecount = 0;
    int commacount = 0;
    for (int i = 0; i < Data.length(); i++) {
      if (Data.charAt(i) == ',') {
        tokens[variablecount] = Data.substring(commacount + 0, i);
        variablecount++;
        commacount = i;
      }
    }

}
t = millis();
vTaskSuspendAll();
File vectornav = SD.open(filename, FILE_WRITE);
            if (vectornav) {
               vectornav.print(Data);
               vectornav.print(",");
               vectornav.print(t);
               vectornav.println();
            }
            vectornav.close();
  }
  Serial.print(Data);
  xTaskResumeAll();
  }
}//end of main task

void candata(void *pvParameters)//start of deferred task
{
    xSemaphoreTake(xBinarySemaphore,portMAX_DELAY);
    float sec= millis();
    File CanFile = SD.open(filename2, FILE_WRITE);
    //unsigned int SUS1 = (msg.buf[1] << 8) | msg.buf[0];
    //unsigned int SUS2 = (msg.buf[5] << 8) | msg.buf[4];
    if (CanFile) {

    float susvoltage1 = (3.3/4096)*SUS1;
    float susvoltage2 = (3.3/4096)*SUS2;
    CanFile.print("SUS1,"); CanFile.print(susvoltage1);CanFile.print(",SUS2,");
    CanFile.print(susvoltage2);
    CanFile.print(",");
    CanFile.print(sec);
    CanFile.println();
    CanFile.close();
    }
    }


void canSniff(const CAN_message_t &msg) {
  BaseType_t xHigherPriorityTaskWoken;
  xHigherPriorityTaskWoken = pdFALSE;
  digitalWrite(13, !digitalRead(13));
   SUS1 = (msg.buf[1] << 8) | msg.buf[0];
   SUS2 = (msg.buf[5] << 8) | msg.buf[4];
   xSemaphoreGiveFromISR(xBinarySemaphore,&xHigherPriorityTaskWoken);

}
void getUniqueFilename(char* filename, const char* form) {
  
  if(counterUpdated == false){
     unsigned int counter = EEPROM.read(0);// Retrieve the counter from EEPROM
    //Serial.println(counter);
    EEPROM.write(0,  counter++); 
    }
  filecounter = millis();
    snprintf(filename, 20, form, counter);
  
}
