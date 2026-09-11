#include <Wire.h>
#include <ESP8266WiFi.h>
#include <avr/pgmspace.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>


// ==========================================
// OLED
// ==========================================

Adafruit_SH1106G display(128, 64, &Wire, -1);


// ==========================================
// WiFi
// ==========================================

const char* ssid = "MAGNETO";
const char* password = "password";


// ==========================================
// Web Server
// ==========================================

WiFiServer server(80);


// ==========================================
// PC Statistics
// ==========================================

// CPU
float cpuUsage = 0.0;
float cpuTemp = 0.0;

// GPU
float gpuUsage = 0.0;
float gpuTemp = 0.0;

// RAM
float ramUsed = 0.0;
float ramTotal = 0.0;

// VRAM
float vramUsed = 0.0;
float vramTotal = 0.0;

// Disk
float diskUsed = 0.0;
float diskTotal = 0.0;
float diskPercent = 0.0;

// aria2 Download
float downloadProgress = 0.0;
float downloadSpeed = 0.0;
float downloadUsed = 0.0;
float downloadTotal = 0.0;
long downloadEta = 0;


// ==========================================
// Current Page
// ==========================================

int currentPage = 1;


// ==========================================
// PC Connection
// ==========================================

unsigned long lastPCUpdate = 0;

const unsigned long PC_TIMEOUT = 5000;


// ==========================================
// Display Timing
// ==========================================

unsigned long lastDisplayUpdate = 0;

const unsigned long DISPLAY_INTERVAL = 15;


// ==========================================
// Infinity Animation
// ==========================================

int infinityFrame = 0;

unsigned long lastInfinityUpdate = 0;

const unsigned long INFINITY_INTERVAL = 15;


// ==========================================
// Extract Parameter
// ==========================================

float getParameter(
  String& request,
  const char* parameter) {

  int start = request.indexOf(parameter);

  if (start < 0) {
    return 0.0;
  }

  start += strlen(parameter);

  int end = request.indexOf("&", start);

  if (end < 0) {
    end = request.indexOf(" ", start);
  }

  if (end < 0) {
    return 0.0;
  }

  return request.substring(start, end).toFloat();
}


// ==========================================
// Draw Horizontal Loading Bar
// ==========================================

void drawLoadingBar(
  int x,
  int y,
  int width,
  int height,
  float percentage) {

  percentage = constrain(
    percentage,
    0.0,
    100.0);


  // Outline
  display.drawRect(
    x,
    y,
    width,
    height,
    SH110X_WHITE);


  // Filled portion
  int fillWidth =
    ((width - 2) * percentage) / 100.0;


  if (fillWidth > 0) {

    display.fillRect(
      x + 1,
      y + 1,
      fillWidth,
      height - 2,
      SH110X_WHITE);
  }
}


// ==========================================
// Precomputed Infinity Path
// ==========================================

// Normalized infinity coordinates.
//
// X range: -100 to +100
// Y range:  -50 to +50
//
// Stored in flash using PROGMEM so it does
// not unnecessarily consume ESP8266 RAM.


const int INFINITY_LOGO_POINTS = 500;

const int8_t infinityLogoPath[500][2] PROGMEM = {
  { 0, 0 }, { 1, 1 }, { 3, 3 }, { 4, 4 }, { 5, 5 }, { 6, 6 }, { 8, 8 }, { 9, 9 }, { 10, 10 }, { 11, 11 }, { 13, 12 }, { 14, 14 }, { 15, 15 }, { 16, 16 }, { 18, 17 }, { 19, 18 }, { 20, 20 }, { 21, 21 }, { 22, 22 }, { 24, 23 }, { 25, 24 }, { 26, 25 }, { 27, 26 }, { 29, 27 }, { 30, 28 }, { 31, 29 }, { 32, 30 }, { 33, 31 }, { 34, 32 }, { 36, 33 }, { 37, 34 }, { 38, 35 }, { 39, 36 }, { 40, 37 }, { 41, 38 }, { 43, 39 }, { 44, 39 }, { 45, 40 }, { 46, 41 }, { 47, 42 }, { 48, 42 }, { 49, 43 }, { 50, 44 }, { 51, 44 }, { 53, 45 }, { 54, 45 }, { 55, 46 }, { 56, 46 }, { 57, 47 }, { 58, 47 }, { 59, 48 }, { 60, 48 }, { 61, 48 }, { 62, 49 }, { 63, 49 }, { 64, 49 }, { 65, 49 }, { 66, 50 }, { 67, 50 }, { 68, 50 }, { 68, 50 }, { 69, 50 }, { 70, 50 }, { 71, 50 }, { 72, 50 }, { 73, 50 }, { 74, 50 }, { 75, 50 }, { 75, 50 }, { 76, 49 }, { 77, 49 }, { 78, 49 }, { 79, 49 }, { 79, 48 }, { 80, 48 }, { 81, 48 }, { 82, 47 }, { 82, 47 }, { 83, 46 }, { 84, 46 }, { 84, 45 }, { 85, 45 }, { 86, 44 }, { 86, 44 }, { 87, 43 }, { 88, 42 }, { 88, 42 }, { 89, 41 }, { 89, 40 }, { 90, 39 }, { 90, 39 }, { 91, 38 }, { 92, 37 }, { 92, 36 }, { 93, 35 }, { 93, 34 }, { 93, 33 }, { 94, 32 }, { 94, 31 }, { 95, 30 }, { 95, 29 }, { 95, 28 }, { 96, 27 }, { 96, 26 }, { 97, 25 }, { 97, 24 }, { 97, 23 }, { 97, 22 }, { 98, 21 }, { 98, 20 }, { 98, 18 }, { 98, 17 }, { 99, 16 }, { 99, 15 }, { 99, 14 }, { 99, 12 }, { 99, 11 }, { 99, 10 }, { 100, 9 }, { 100, 8 }, { 100, 6 }, { 100, 5 }, { 100, 4 }, { 100, 3 }, { 100, 1 }, { 100, 0 }, { 100, -1 }, { 100, -3 }, { 100, -4 }, { 100, -5 }, { 100, -6 }, { 100, -8 }, { 100, -9 }, { 99, -10 }, { 99, -11 }, { 99, -12 }, { 99, -14 }, { 99, -15 }, { 99, -16 }, { 98, -17 }, { 98, -18 }, { 98, -20 }, { 98, -21 }, { 97, -22 }, { 97, -23 }, { 97, -24 }, { 97, -25 }, { 96, -26 }, { 96, -27 }, { 95, -28 }, { 95, -29 }, { 95, -30 }, { 94, -31 }, { 94, -32 }, { 93, -33 }, { 93, -34 }, { 93, -35 }, { 92, -36 }, { 92, -37 }, { 91, -38 }, { 90, -39 }, { 90, -39 }, { 89, -40 }, { 89, -41 }, { 88, -42 }, { 88, -42 }, { 87, -43 }, { 86, -44 }, { 86, -44 }, { 85, -45 }, { 84, -45 }, { 84, -46 }, { 83, -46 }, { 82, -47 }, { 82, -47 }, { 81, -48 }, { 80, -48 }, { 79, -48 }, { 79, -49 }, { 78, -49 }, { 77, -49 }, { 76, -49 }, { 75, -50 }, { 75, -50 }, { 74, -50 }, { 73, -50 }, { 72, -50 }, { 71, -50 }, { 70, -50 }, { 69, -50 }, { 68, -50 }, { 68, -50 }, { 67, -50 }, { 66, -50 }, { 65, -49 }, { 64, -49 }, { 63, -49 }, { 62, -49 }, { 61, -48 }, { 60, -48 }, { 59, -48 }, { 58, -47 }, { 57, -47 }, { 56, -46 }, { 55, -46 }, { 54, -45 }, { 53, -45 }, { 51, -44 }, { 50, -44 }, { 49, -43 }, { 48, -42 }, { 47, -42 }, { 46, -41 }, { 45, -40 }, { 44, -39 }, { 43, -39 }, { 41, -38 }, { 40, -37 }, { 39, -36 }, { 38, -35 }, { 37, -34 }, { 36, -33 }, { 34, -32 }, { 33, -31 }, { 32, -30 }, { 31, -29 }, { 30, -28 }, { 29, -27 }, { 27, -26 }, { 26, -25 }, { 25, -24 }, { 24, -23 }, { 22, -22 }, { 21, -21 }, { 20, -20 }, { 19, -18 }, { 18, -17 }, { 16, -16 }, { 15, -15 }, { 14, -14 }, { 13, -12 }, { 11, -11 }, { 10, -10 }, { 9, -9 }, { 8, -8 }, { 6, -6 }, { 5, -5 }, { 4, -4 }, { 3, -3 }, { 1, -1 },

  { 0, 0 },
  { -1, 1 },
  { -3, 3 },
  { -4, 4 },
  { -5, 5 },
  { -6, 6 },
  { -8, 8 },
  { -9, 9 },
  { -10, 10 },
  { -11, 11 },
  { -13, 12 },
  { -14, 14 },
  { -15, 15 },
  { -16, 16 },
  { -18, 17 },
  { -19, 18 },
  { -20, 20 },
  { -21, 21 },
  { -22, 22 },
  { -24, 23 },
  { -25, 24 },
  { -26, 25 },
  { -27, 26 },
  { -29, 27 },
  { -30, 28 },
  { -31, 29 },
  { -32, 30 },
  { -33, 31 },
  { -34, 32 },
  { -36, 33 },
  { -37, 34 },
  { -38, 35 },
  { -39, 36 },
  { -40, 37 },
  { -41, 38 },
  { -43, 39 },
  { -44, 39 },
  { -45, 40 },
  { -46, 41 },
  { -47, 42 },
  { -48, 42 },
  { -49, 43 },
  { -50, 44 },
  { -51, 44 },
  { -53, 45 },
  { -54, 45 },
  { -55, 46 },
  { -56, 46 },
  { -57, 47 },
  { -58, 47 },
  { -59, 48 },
  { -60, 48 },
  { -61, 48 },
  { -62, 49 },
  { -63, 49 },
  { -64, 49 },
  { -65, 49 },
  { -66, 50 },
  { -67, 50 },
  { -68, 50 },
  { -68, 50 },
  { -69, 50 },
  { -70, 50 },
  { -71, 50 },
  { -72, 50 },
  { -73, 50 },
  { -74, 50 },
  { -75, 50 },
  { -75, 50 },
  { -76, 49 },
  { -77, 49 },
  { -78, 49 },
  { -79, 49 },
  { -79, 48 },
  { -80, 48 },
  { -81, 48 },
  { -82, 47 },
  { -82, 47 },
  { -83, 46 },
  { -84, 46 },
  { -84, 45 },
  { -85, 45 },
  { -86, 44 },
  { -86, 44 },
  { -87, 43 },
  { -88, 42 },
  { -88, 42 },
  { -89, 41 },
  { -89, 40 },
  { -90, 39 },
  { -90, 39 },
  { -91, 38 },
  { -92, 37 },
  { -92, 36 },
  { -93, 35 },
  { -93, 34 },
  { -93, 33 },
  { -94, 32 },
  { -94, 31 },
  { -95, 30 },
  { -95, 29 },
  { -95, 28 },
  { -96, 27 },
  { -96, 26 },
  { -97, 25 },
  { -97, 24 },
  { -97, 23 },
  { -97, 22 },
  { -98, 21 },
  { -98, 20 },
  { -98, 18 },
  { -98, 17 },
  { -99, 16 },
  { -99, 15 },
  { -99, 14 },
  { -99, 12 },
  { -99, 11 },
  { -99, 10 },
  { -100, 9 },
  { -100, 8 },
  { -100, 6 },
  { -100, 5 },
  { -100, 4 },
  { -100, 3 },
  { -100, 1 },
  { -100, 0 },
  { -100, -1 },
  { -100, -3 },
  { -100, -4 },
  { -100, -5 },
  { -100, -6 },
  { -100, -8 },
  { -100, -9 },
  { -99, -10 },
  { -99, -11 },
  { -99, -12 },
  { -99, -14 },
  { -99, -15 },
  { -99, -16 },
  { -98, -17 },
  { -98, -18 },
  { -98, -20 },
  { -98, -21 },
  { -97, -22 },
  { -97, -23 },
  { -97, -24 },
  { -97, -25 },
  { -96, -26 },
  { -96, -27 },
  { -95, -28 },
  { -95, -29 },
  { -95, -30 },
  { -94, -31 },
  { -94, -32 },
  { -93, -33 },
  { -93, -34 },
  { -93, -35 },
  { -92, -36 },
  { -92, -37 },
  { -91, -38 },
  { -90, -39 },
  { -90, -39 },
  { -89, -40 },
  { -89, -41 },
  { -88, -42 },
  { -88, -42 },
  { -87, -43 },
  { -86, -44 },
  { -86, -44 },
  { -85, -45 },
  { -84, -45 },
  { -84, -46 },
  { -83, -46 },
  { -82, -47 },
  { -82, -47 },
  { -81, -48 },
  { -80, -48 },
  { -79, -48 },
  { -79, -49 },
  { -78, -49 },
  { -77, -49 },
  { -76, -49 },
  { -75, -50 },
  { -75, -50 },
  { -74, -50 },
  { -73, -50 },
  { -72, -50 },
  { -71, -50 },
  { -70, -50 },
  { -69, -50 },
  { -68, -50 },
  { -68, -50 },
  { -67, -50 },
  { -66, -50 },
  { -65, -49 },
  { -64, -49 },
  { -63, -49 },
  { -62, -49 },
  { -61, -48 },
  { -60, -48 },
  { -59, -48 },
  { -58, -47 },
  { -57, -47 },
  { -56, -46 },
  { -55, -46 },
  { -54, -45 },
  { -53, -45 },
  { -51, -44 },
  { -50, -44 },
  { -49, -43 },
  { -48, -42 },
  { -47, -42 },
  { -46, -41 },
  { -45, -40 },
  { -44, -39 },
  { -43, -39 },
  { -41, -38 },
  { -40, -37 },
  { -39, -36 },
  { -38, -35 },
  { -37, -34 },
  { -36, -33 },
  { -34, -32 },
  { -33, -31 },
  { -32, -30 },
  { -31, -29 },
  { -30, -28 },
  { -29, -27 },
  { -27, -26 },
  { -26, -25 },
  { -25, -24 },
  { -24, -23 },
  { -22, -22 },
  { -21, -21 },
  { -20, -20 },
  { -19, -18 },
  { -18, -17 },
  { -16, -16 },
  { -15, -15 },
  { -14, -14 },
  { -13, -12 },
  { -11, -11 },
  { -10, -10 },
  { -9, -9 },
  { -8, -8 },
  { -6, -6 },
  { -5, -5 },
  { -4, -4 },
  { -3, -3 },
  { -1, -1 }
};

// ==========================================
// Read Infinity Coordinate
// ==========================================


void getInfinityLogoPoint(
  int index,
  int scaleX,
  int scaleY,
  int& x,
  int& y) {
  int8_t rawX =
    (int8_t)pgm_read_byte(&infinityLogoPath[index][0]);

  int8_t rawY =
    (int8_t)pgm_read_byte(&infinityLogoPath[index][1]);

  x = (rawX * scaleX) / 100;
  y = (rawY * scaleY) / 50;
}
// ==========================================
// Draw Small Infinity Loader
// ==========================================

void drawInfinityLoader() {

  const int CENTER_X = 106;
  const int CENTER_Y = 5;

  const int SCALE_X = 14;
  const int SCALE_Y = 5;

  // Same ~90% trail length as before
  const int TRAIL = 450;

  const float SPEED = 0.2;

  int position =
    (int)(millis() * SPEED) % INFINITY_LOGO_POINTS;

  for (int i = 0; i < TRAIL; i++) {

    int index =
      (position + i) % INFINITY_LOGO_POINTS;

    int x, y;

    getInfinityLogoPoint(
      index,
      SCALE_X,
      SCALE_Y,
      x,
      y);

    int px = CENTER_X + x;
    int py = CENTER_Y + y;

    // Main stroke
    display.drawPixel(
      px,
      py,
      SH110X_WHITE);

    

    // Thick middle section
    if (i >= 50 && i <= 400) {
      display.drawPixel(CENTER_X + round(x), CENTER_Y + round(y) + 1, SH110X_WHITE);
    }
  }
}


// ==========================================
// Draw Main Infinity Logo
// ==========================================

void drawInfinityLogo() {

  const int CENTER_X = 63;
  const int CENTER_Y = 31;

  const int SCALE_X = 40;
  const int SCALE_Y = 14;

  // 450/500 points visible.
  // This leaves a small dark gap.
  const int TRAIL = 450;

  const float SPEED = 0.2;

  int position =
    (int)(millis() * SPEED) % INFINITY_LOGO_POINTS;

  for (int i = 0; i < TRAIL; i++) {

    int index =
      (position + i) % INFINITY_LOGO_POINTS;

    int x, y;

    getInfinityLogoPoint(
      index,
      SCALE_X,
      SCALE_Y,
      x,
      y);

    int px = CENTER_X + x;
    int py = CENTER_Y + y;

    // Main stroke
    display.drawPixel(
      px,
      py,
      SH110X_WHITE);

    

    // Thick middle section
    if (i >= 50 && i <= 400) {
      display.drawPixel(CENTER_X + round(x), CENTER_Y + round(y) + 1, SH110X_WHITE);
      display.drawPixel(CENTER_X + round(x), CENTER_Y + round(y) + 2, SH110X_WHITE);
      if (i >= 150 && i <= 300) {
        display.drawPixel(CENTER_X + round(x), CENTER_Y + round(y) + 3, SH110X_WHITE);
        display.drawPixel(CENTER_X + round(x), CENTER_Y + round(y) + 4, SH110X_WHITE);
        if (i >= 187 && i <= 225) { display.drawPixel(CENTER_X + round(x), CENTER_Y + round(y) + 5, SH110X_WHITE); }
      }
    }
  }
}


// ==========================================
// Setup
// ==========================================

void setup() {

  Serial.begin(115200);


  // ==========================================
  // I2C
  // ==========================================

  Wire.begin(D2, D1);


  // ==========================================
  // OLED
  // ==========================================

  if (!display.begin(0x3C, true)) {

    Serial.println("OLED not found");

    while (true) {
      delay(1000);
    }
  }


  display.clearDisplay();

  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("Connecting WiFi...");

  display.display();


  // ==========================================
  // Connect WiFi
  // ==========================================

  WiFi.begin(
    ssid,
    password);


  while (
    WiFi.status() != WL_CONNECTED) {

    delay(500);
  }


  // ==========================================
  // WiFi Connected
  // ==========================================

  Serial.println("WiFi connected");

  Serial.print("ESP8266 IP: ");

  Serial.println(
    WiFi.localIP());


  // ==========================================
  // Start Server
  // ==========================================

  server.begin();

  Serial.println("Server started");


  // ==========================================
  // Ready Screen
  // ==========================================

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("Smart Monitor");

  display.setCursor(0, 20);

  display.println("WiFi Connected");

  display.setCursor(0, 35);

  display.print("IP: ");

  display.println(
    WiFi.localIP());

  display.display();

  delay(2000);
}


// ==========================================
// Handle PC Request
// ==========================================

void handlePCRequest() {

  WiFiClient client =
    server.available();


  if (!client) {
    return;
  }


  // ==========================================
  // Wait for request
  // ==========================================

  unsigned long startTime =
    millis();


  while (!client.available()) {

    if (
      millis() - startTime > 500) {

      client.stop();

      return;
    }

    delay(1);
  }


  // ==========================================
  // Read request
  // ==========================================

  String request =
    client.readStringUntil('\r');

  client.read();


  // ==========================================
  // Process update
  // ==========================================

  if (
    request.indexOf("GET /update?") >= 0) {

    // CPU
    cpuUsage =
      getParameter(request, "?cpu=");

    cpuTemp =
      getParameter(request, "&cput=");


    // GPU
    gpuUsage =
      getParameter(request, "&gpu=");

    gpuTemp =
      getParameter(request, "&gput=");


    // RAM
    ramUsed =
      getParameter(request, "&ramu=");

    ramTotal =
      getParameter(request, "&ramt=");


    // VRAM
    vramUsed =
      getParameter(request, "&vramu=");

    vramTotal =
      getParameter(request, "&vramt=");


    // Disk
    diskUsed =
      getParameter(request, "&disku=");

    diskTotal =
      getParameter(request, "&diskt=");

    diskPercent =
      ((diskUsed / diskTotal) * 100);


    // aria2 Download
    downloadProgress =
      getParameter(request, "&dprog=");

    downloadSpeed =
      getParameter(request, "&dspeed=");

    downloadUsed =
      getParameter(request, "&duse=");

    downloadTotal =
      getParameter(request, "&dtot=");

    downloadEta =
      (long)getParameter(request, "&deta=");


    // Page
    currentPage =
      (int)getParameter(
        request,
        "&page=");


    if (
      currentPage < 0 || currentPage > 4) {

      currentPage = 0;
    }


    // Record update
    lastPCUpdate =
      millis();


    // ========================================
    // HTTP response
    // ========================================

    client.println(
      "HTTP/1.1 200 OK");

    client.println(
      "Content-Type: text/plain");

    client.println(
      "Content-Length: 2");

    client.println(
      "Connection: close");

    client.println();

    client.print("OK");

    client.flush();

  } else {

    client.println(
      "HTTP/1.1 404 Not Found");

    client.println(
      "Content-Type: text/plain");

    client.println(
      "Content-Length: 9");

    client.println(
      "Connection: close");

    client.println();

    client.print(
      "Not Found");

    client.flush();
  }


  client.stop();
}


// ==========================================
// CPU & GPU Page
// ==========================================

void drawCPUGPUPage(
  bool pcConnected) {
  if (pcConnected) {

    // ==========================================
    // Title
    // ==========================================

    display.setTextSize(1);

    display.setCursor(3, 1);

    display.print("CPU & GPU");


    // ==========================================
    // Infinity loader
    // ==========================================

    drawInfinityLoader();


    // ==========================================
    // CPU + GPU loading bars
    // ==========================================


    drawLoadingBar(
      4,
      17,
      120,
      10,
      cpuUsage);


    drawLoadingBar(
      4,
      34,
      120,
      10,
      gpuUsage);


    // CPU temperature
    display.setTextSize(1);

    display.setCursor(4, 50);

    display.print("CPU ");

    display.print(cpuTemp, 0);

    display.print("C");


    // GPU temperature
    display.setCursor(82, 50);

    display.print("GPU ");

    display.print(gpuTemp, 0);

    display.print("C");

  } else {
    display.clearDisplay();
    int x = random(0, 128);
    int y = random(0, 64);
    display.drawPixel(
      x, y, SH110X_WHITE);
  }
}


// ==========================================
// RAM & VRAM Page
// ==========================================

void drawRAMVRAMPage(
  bool pcConnected) {
  if (pcConnected) {

    // ==========================================
    // Title
    // ==========================================

    display.setTextSize(1);

    display.setCursor(3, 1);

    display.print("RAM & VRAM");


    // ==========================================
    // Infinity loader
    // ==========================================

    drawInfinityLoader();



    // RAM percentage
    float ramPercent = 0.0;

    if (ramTotal > 0) {

      ramPercent =
        (ramUsed / ramTotal) * 100.0;
    }


    // VRAM percentage
    float vramPercent = 0.0;

    if (vramTotal > 0) {

      vramPercent =
        (vramUsed / vramTotal) * 100.0;
    }


    // ========================================
    // RAM bar
    // ========================================

    drawLoadingBar(
      4,
      17,
      120,
      10,
      ramPercent);


    // ========================================
    // VRAM bar
    // ========================================

    drawLoadingBar(
      4,
      32,
      120,
      10,
      vramPercent);


    // ========================================
    // RAM text
    // ========================================

    display.setTextSize(1);

    display.setCursor(4, 47);

    display.print("RAM ");

    display.print(ramUsed, 1);

    display.print("/");

    display.print(ramTotal, 1);

    display.print("GB");


    // ========================================
    // VRAM text
    // ========================================

    display.setCursor(4, 55);

    display.print("VRAM ");

    display.print(vramUsed, 1);

    display.print("/");

    display.print(vramTotal, 1);

    display.print("GB");

  } else {
    display.clearDisplay();
    int x = random(0, 128);
    int y = random(0, 64);
    display.drawPixel(
      x, y, SH110X_WHITE);
  }
}


// ==========================================
// Disk Page
// ==========================================

void drawDiskPage(
  bool pcConnected) {
  if (pcConnected) {

    // ==========================================
    // Title
    // ==========================================

    display.setTextSize(1);

    display.setCursor(3, 1);

    display.print("DISK");


    // ==========================================
    // Infinity loader
    // ==========================================

    drawInfinityLoader();



    // ========================================
    // Disk usage bar
    // ========================================

    drawLoadingBar(
      4,
      18,
      120,
      12,
      diskPercent);


    // ========================================
    // Occupied percentage
    // ========================================

    display.setTextSize(1);

    display.setCursor(4, 36);

    display.print("Occupied: ");

    display.print(
      diskPercent,
      1);

    display.print("%");


    // ========================================
    // Used / Total
    // ========================================

    display.setCursor(4, 49);

    display.print("Space: ");

    display.print(
      diskUsed,
      1);

    display.print("/");

    display.print(
      diskTotal,
      1);

    display.print("GB");

  } else {
    display.clearDisplay();
    int x = random(0, 128);
    int y = random(0, 64);
    display.drawPixel(
      x, y, SH110X_WHITE);
  }
}


// ==========================================
// aria2 Download Page
// ==========================================

void drawAria2Page(
  bool pcConnected) {
  if (pcConnected) {

    // ==========================================
    // Title
    // ==========================================

    display.setTextSize(1);

    display.setCursor(3, 1);

    display.print("DOWNLOAD");


    // ==========================================
    // Infinity loader
    // ==========================================

    drawInfinityLoader();


    // ========================================
    // Download progress bar
    // ========================================

    drawLoadingBar(
      4,
      17,
      120,
      12,
      downloadProgress);


    // ========================================
    // Progress
    // ========================================

    display.setCursor(4, 34);

    display.print(downloadProgress, 0);

    display.print("%");


    // ========================================
    // Speed
    // ========================================

    display.setCursor(58, 34);

    display.print(downloadSpeed, 1);

    display.print(" MB/s");


    // ========================================
    // Downloaded / Total
    // ========================================

    display.setCursor(4, 46);

    display.print(downloadUsed, 1);

    display.print("/");

    display.print(downloadTotal, 1);

    display.print("GB");


    // ========================================
    // ETA
    // ========================================

    display.setCursor(4, 56);

    display.print("ETA ");

    if (downloadEta > 0) {

      long hours = downloadEta / 3600;
      long minutes = (downloadEta % 3600) / 60;
      long seconds = downloadEta % 60;

      if (hours > 0) {
        if (hours < 10) display.print("0");
        display.print(hours);
        display.print(":");
      }

      if (minutes < 10) display.print("0");
      display.print(minutes);
      display.print(":");

      if (seconds < 10) display.print("0");
      display.print(seconds);

    } else {

      display.print("--:--");
    }

  } else {
    display.clearDisplay();
    int x = random(0, 128);
    int y = random(0, 64);
    display.drawPixel(
      x, y, SH110X_WHITE);
  }
}


void drawFrontPage() {

  drawInfinityLogo();
}


// ==========================================
// Display
// ==========================================

void updateDisplay() {

  display.clearDisplay();


  // ==========================================
  // PC connection
  // ==========================================

  bool pcConnected =
    (millis() - lastPCUpdate) < PC_TIMEOUT;


  // ==========================================
  // Animate infinity
  // ==========================================

  if (
    millis() - lastInfinityUpdate
    >= INFINITY_INTERVAL) {

    lastInfinityUpdate =
      millis();

    infinityFrame++;
  }


  // ==========================================
  // Select page
  // ==========================================

  if (currentPage == 1) {

    drawCPUGPUPage(
      pcConnected);

  } else if (currentPage == 2) {

    drawRAMVRAMPage(
      pcConnected);

  } else if (currentPage == 3) {

    drawDiskPage(
      pcConnected);
  } else if (currentPage == 4) {

    drawAria2Page(
      pcConnected);
  } else {

    drawFrontPage();
  }


  // ==========================================
  // Update OLED
  // ==========================================

  display.display();
}


// ==========================================
// Main Loop
// ==========================================

void loop() {

  handlePCRequest();


  if (
    millis() - lastDisplayUpdate
    >= DISPLAY_INTERVAL) {

    lastDisplayUpdate =
      millis();

    updateDisplay();
  }
}