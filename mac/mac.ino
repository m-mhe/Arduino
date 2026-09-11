#include <Wire.h>

#include <ESP8266WiFi.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

Adafruit_SH1106G display(128, 64, &Wire, -1);

void setup()
{
  Serial.begin(115200);

  Wire.begin(D2, D1);

  WiFi.mode(WIFI_STA);

  if (!display.begin(0x3C, true))
  {
    Serial.println("OLED not found");
    while (1);
  }

  String mac = WiFi.macAddress();

  Serial.println();
  Serial.print("ESP8266 MAC Address: ");
  Serial.println(mac);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("ESP8266");
  display.println("MAC Address:");
  display.println();
  display.println(mac);
  display.display();
}


void loop(){
  
}