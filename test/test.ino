#include <Wire.h>
#include "MAX30105.h"
#include "spo2_algorithm.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

Adafruit_SH1106G display(128, 64, &Wire, -1);
MAX30105 particleSensor;



void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  Wire.begin(D2, D1);

  // OLED
  if (!display.begin(0x3C, true))
  {
    Serial.println("OLED not found");
    while (1);
  }

}

void loop() {

  for(int i = 0; i <38; i++){

    display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(18, i);
  display.println("MMHE");
  display.display();
  delay(2);

  }

  for(int i = 0; i <33; i++){

    display.clearDisplay();
  display.setTextSize(4);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(i, 22);
  display.println("MMHE");
  display.display();
  delay(2);

  }

}
