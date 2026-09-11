#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

Adafruit_SH1106G display(128, 64, &Wire, -1);
MAX30105 particleSensor;

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
uint16_t irBuffer[100];
uint16_t redBuffer[100];
#else
uint32_t irBuffer[100];
uint32_t redBuffer[100];
#endif

int32_t bufferLength;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate;
int8_t validHeartRate;

void setup()
{
  Serial.begin(115200);

  Wire.begin(D2, D1);

  // OLED
  if (!display.begin(0x3C, true))
  {
    Serial.println("OLED not found");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Initializing...");
  display.display();

  // MAX30102
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST))
  {
    display.clearDisplay();
    display.setCursor(0, 20);
    display.println("MAX30102");
    display.println("NOT FOUND!");
    display.display();

    while (1);
  }

  byte ledBrightness = 60;
  byte sampleAverage = 4;
  byte ledMode = 2;
  byte sampleRate = 100;
  int pulseWidth = 411;
  int adcRange = 4096;

  particleSensor.setup(
    ledBrightness,
    sampleAverage,
    ledMode,
    sampleRate,
    pulseWidth,
    adcRange
  );

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Place Finger...");
  display.display();
}

void loop()
{
  bufferLength = 100;

  // Collect first 100 samples
  for (byte i = 0; i < bufferLength; i++)
  {
    while (!particleSensor.available())
      particleSensor.check();

    redBuffer[i] = particleSensor.getRed();
    irBuffer[i] = particleSensor.getIR();

    particleSensor.nextSample();
  }

  maxim_heart_rate_and_oxygen_saturation(
      irBuffer,
      bufferLength,
      redBuffer,
      &spo2,
      &validSPO2,
      &heartRate,
      &validHeartRate);

  while (true)
  {
    // Shift old samples
    for (byte i = 25; i < 100; i++)
    {
      redBuffer[i - 25] = redBuffer[i];
      irBuffer[i - 25] = irBuffer[i];
    }

    // Read 25 new samples
    for (byte i = 75; i < 100; i++)
    {
      while (!particleSensor.available())
        particleSensor.check();

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();

      particleSensor.nextSample();
    }

    maxim_heart_rate_and_oxygen_saturation(
        irBuffer,
        bufferLength,
        redBuffer,
        &spo2,
        &validSPO2,
        &heartRate,
        &validHeartRate);

    // Serial Monitor
    Serial.print("HR=");
    Serial.print(heartRate);
    Serial.print("  SpO2=");
    Serial.print(spo2);
    Serial.print("  IR=");
    Serial.println(irBuffer[99]);

    // OLED
    display.clearDisplay();

    if (irBuffer[99] < 50000)
    {
      display.setTextSize(2);
      display.setCursor(5, 25);
      display.println("NO FINGER!");
    }
    else
    {
      display.setTextSize(1);

      display.setCursor(0, 0);
      display.println("Pulse Oximeter");

      display.setCursor(0, 18);
      display.print("HR: ");
      if (validHeartRate)
      {
        display.print(heartRate);
        display.println(" BPM");
      }
      else
      {
        display.println("--");
      }

      display.setCursor(0, 38);
      display.print("SpO2: ");
      if (validSPO2)
      {
        display.print(spo2);
        display.println("%");
      }
      else
      {
        display.println("--");
      }
    }

    display.display();
  }
}