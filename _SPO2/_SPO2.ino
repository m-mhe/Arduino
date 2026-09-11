#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>




#define BLYNK_TEMPLATE_ID "TMPL6WQk9xdAx"
#define BLYNK_TEMPLATE_NAME "SmartCadio"
#define BLYNK_AUTH_TOKEN "1zGWeqf2bbQ8H1CLmbRESE0gHR7OmrUr"
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
char ssid[] = "MAGNETO";
char pass[] = "password";
int age = 25;

BLYNK_WRITE(V2)
{
    age = param.asInt();
}


Adafruit_SH1106G display(128, 64, &Wire, -1);

MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100]; //infrared LED sensor data
uint16_t redBuffer[100];  //red LED sensor data
#else
uint32_t irBuffer[100]; //infrared LED sensor data
uint32_t redBuffer[100];  //red LED sensor data
#endif

int32_t bufferLength; //data length
int32_t spo2; //SPO2 value
int8_t validSPO2; //indicator to show if the SPO2 calculation is valid
int32_t heartRate; //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid


void setup()
{


  Serial.begin(115200); // initialize serial communication at 115200 bits per second:




  Wire.begin(D2, D1);

if (!display.begin(0x3C, true)) {
  Serial.println("OLED not found");
  while (1);
}

display.clearDisplay();
display.setTextSize(2);
display.setTextColor(SH110X_WHITE);
display.setCursor(0, 0);
display.println("Initializing...");
display.display();




  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30102 NOT FOUND");

display.clearDisplay();
display.setCursor(5, 25);
display.println("--Pulse Oximeter--\n");
display.println("!!!!Not Found!!!!");
display.display();

while (1);
  }



  byte ledBrightness = 100; //Options: 0=Off to 255=50mA
  byte sampleAverage = 1; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 16384; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
  particleSensor.clearFIFO();






display.clearDisplay();
display.setCursor(0, 0);
display.setTextSize(2);
display.println("Syncing...");
display.display();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);



}

void loop()
{
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps

  //read the first 100 samples, and determine the signal range
  for (byte i = 0 ; i < bufferLength ; i++)
  {
    while (particleSensor.available() == false) //do we have new data?
      particleSensor.check(); //Check the sensor for new data

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();
    particleSensor.nextSample(); //We're finished with this sample so move to next sample


  }

  //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
  maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  while (1)
  {


    Blynk.run();



    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    //take 25 sets of samples before calculating the heart rate.
    for (byte i = 75; i < 100; i++)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check(); //Check the sensor for new data


      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      //send samples and calculation result to terminal program through UART
    }

    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);


/*if (heartRate > 120){
    randomSeed(heartRate);
    heartRate = random(70, 81);
    }*/


    if (irBuffer[99] < 50000)
{
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(5, 26);
  display.println("NO FINGER!");
  display.display();
  continue;
}


    display.clearDisplay();

display.setTextSize(2);
display.setCursor(0, 0);
display.println("SmartCadio");

display.setCursor(0, 20);

display.setTextSize(1);
if (validHeartRate)
{
  display.print("HR: ");
  display.print(heartRate);
  display.println(" BPM");
  Blynk.virtualWrite(V0, heartRate);
}
else
{
  display.println("HR: --");
}

display.setCursor(0, 30);

if (validSPO2)
{
  display.print("SpO2: ");
  display.print(spo2);
  display.println("%");
  Blynk.virtualWrite(V1, spo2);
}
else
{
  display.println("SpO2: --");
}


display.setCursor(0, 45);
display.setTextSize(1);
    if (heartRate < 40 && validHeartRate)
{
    display.println("CRITICAL!");
}
else if (heartRate > (220 - age) && validHeartRate)
{
    
    display.println("CRITICAL!");
}
else if (spo2 < 90 && validSPO2)
{
    
    display.println("CRITICAL!");
}else{

display.setTextSize(2);
    display.println("GOOD");
}





    if (heartRate < 40 && validHeartRate)
{
    Blynk.logEvent(
        "low_heart_rate_detected"
    );
}
if (heartRate > (220 - age) && validHeartRate)
{
    Blynk.logEvent(
        "high_heart_rate"
    );
}
if (spo2 < 90 && validSPO2)
{
    Blynk.logEvent(
        "low_blood_oxygen"
    );
}




// Plot area: x = 64 to 127, y = 20 to 63

uint32_t minVal = irBuffer[36];
uint32_t maxVal = irBuffer[36];

// Find min and max of the last 64 samples
for (int i = 36; i < 100; i++)
{
    if (irBuffer[i] < minVal) minVal = irBuffer[i];
    if (irBuffer[i] > maxVal) maxVal = irBuffer[i];
}

// Avoid divide-by-zero
if (maxVal == minVal) maxVal = minVal + 1;

// Draw waveform
for (int x = 0; x < 63; x++)
{
    int i1 = 36 + x;
    int i2 = 37 + x;

    int y1 = map(irBuffer[i1], minVal, maxVal, 63, 20);
    int y2 = map(irBuffer[i2], minVal, maxVal, 63, 20);

    display.drawLine(64 + x, y1, 64 + x + 1, y2, SH110X_WHITE);
}

display.display();
  }
}

