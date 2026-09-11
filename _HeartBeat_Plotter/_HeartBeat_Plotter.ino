/*
  Heart beat plotting!
  By: Nathan Seidle @ SparkFun Electronics
  Date: October 20th, 2016
  https://github.com/sparkfun/MAX30105_Breakout

  Shows the user's heart beat on Arduino's serial plotter

  Instructions:
  1) Load code onto Redboard
  2) Attach sensor to your finger with a rubber band (see below)
  3) Open Tools->'Serial Plotter'
  4) Make sure the drop down is set to 115200 baud
  5) Checkout the blips!
  6) Feel the pulse on your neck and watch it mimic the blips

  It is best to attach the sensor to your finger using a rubber band or other tightening
  device. Humans are generally bad at applying constant pressure to a thing. When you
  press your finger against the sensor it varies enough to cause the blood in your
  finger to flow differently which causes the sensor readings to go wonky.

  Hardware Connections (Breakoutboard to Arduino):
  -5V = 5V (3.3V is allowed)
  -GND = GND
  -SDA = A4 (or SDA)
  -SCL = A5 (or SCL)
  -INT = Not connected

  The MAX30105 Breakout can handle 5V or 3.3V I2C logic. We recommend powering the board with 5V
  but it will also run at 3.3V.
*/

#include <Wire.h>
#include "MAX30105.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

Adafruit_SH1106G display(128, 64, &Wire, -1);


MAX30105 particleSensor;



void setup()
{
  Serial.begin(115200);
  Serial.println("Initializing...");

  Wire.begin(D2, D1);

display.begin(0x3C, true);
display.clearDisplay();
display.display();

  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }

  //Setup to sense a nice looking saw tooth on the plotter
  byte ledBrightness = 0x1F; //Options: 0=Off to 255=50mA
  byte sampleAverage = 8; //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 3; //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  int sampleRate = 100; //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411; //Options: 69, 118, 215, 411
  int adcRange = 4096; //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings

  //Arduino plotter auto-scales annoyingly. To get around this, pre-populate
  //the plotter with 500 of an average reading from the sensor

  //Take an average of IR readings at power up
  const byte avgAmount = 64;
  long baseValue = 0;
  for (byte x = 0 ; x < avgAmount ; x++)
  {
    baseValue += particleSensor.getIR(); //Read the IR value
  }
  baseValue /= avgAmount;

  //Pre-populate the plotter so that the Y scale is close to IR values
  for (int x = 0 ; x < 500 ; x++)
    Serial.println(baseValue);
}

void loop()
{
  static int graph[128];
  long irValue = particleSensor.getIR();

  // Detect no finger
  if (irValue < 50000)
  {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(10, 25);
    display.println("NO FINGER");
    display.display();
    return;
  }

  // Shift graph left
  for (int i = 0; i < 127; i++)
    graph[i] = graph[i + 1];

  static long minIR = irValue;
static long maxIR = irValue;

minIR = min(minIR, irValue);
maxIR = max(maxIR, irValue);

// Slowly adapt to changing conditions
minIR += (irValue - minIR) / 100;
maxIR -= (maxIR - irValue) / 100;

if (maxIR - minIR < 1000)
    maxIR = minIR + 1000;

graph[127] = map(irValue, minIR, maxIR, 58, 6);
graph[127] = constrain(graph[127], 6, 58);

  display.clearDisplay();

  // Draw waveform
  for (int i = 0; i < 127; i++)
  {
    display.drawLine(i, graph[i], i + 1, graph[i + 1], SH110X_WHITE);
  }

  display.display();

  delay(5);
}
