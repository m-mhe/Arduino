#include <Wire.h>
#include <ESP8266WiFi.h>
#include <time.h>

#include "MAX30105.h"
#include "spo2_algorithm.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

Adafruit_SH1106G display(128, 64, &Wire, -1);
MAX30105 particleSensor;

// WiFi credentials
const char* ssid = "MAGNETO";
const char* password = "password";


void setup() {

  Serial.begin(115200);

  // I2C
  Wire.begin(D2, D1);

  // OLED
  if (!display.begin(0x3C, true)) {
    Serial.println("OLED not found");
    while (1);
  }

  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();

  // Connect to WiFi
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println("WiFi connected");

  // Bangladesh = UTC +6
  configTime(
    6 * 3600,
    0,
    "pool.ntp.org",
    "time.nist.gov"
  );

  // Wait for time synchronization
  Serial.println("Synchronizing time...");

  time_t now = time(nullptr);

  while (now < 100000) {
    delay(500);
    now = time(nullptr);
  }

  Serial.println("Time synchronized");
}


void loop() {

  // Get current time
  time_t now = time(nullptr);

  struct tm* timeinfo = localtime(&now);

  // HH:MM:SS
  char currentTime[9];

  strftime(
    currentTime,
    sizeof(currentTime),
    "%H:%M:%S",
    timeinfo
  );

  // Display
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(17, 25);

  display.println(currentTime);

  display.display();

  delay(1000);
}