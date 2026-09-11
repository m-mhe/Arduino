#include <Wire.h>
#include <ESP8266WiFi.h>

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

float cpuUsage = 0.0;
float cpuTemp = 0.0;

float gpuUsage = 0.0;
float gpuTemp = 0.0;


// ==========================================
// PC Connection
// ==========================================

unsigned long lastPCUpdate = 0;

const unsigned long PC_TIMEOUT = 5000;


// ==========================================
// Display Timing
// ==========================================

unsigned long lastDisplayUpdate = 0;

const unsigned long DISPLAY_INTERVAL = 100;


// ==========================================
// Extract Parameter
// ==========================================

float getParameter(
  String& request,
  const char* parameter
) {

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
// Draw Circular Gauge
// ==========================================

void drawCircularGauge(
  int centerX,
  int centerY,
  int radius,
  float percentage
) {

  percentage = constrain(
    percentage,
    0.0,
    100.0
  );


  // ==========================================
  // Gauge settings
  // ==========================================

  const int THICKNESS = 4;

  const float START_ANGLE = -90.0;


  // ==========================================
  // Calculate used angle
  // ==========================================

  float usedAngle =
    360.0 * percentage / 100.0;


  // ==========================================
  // Draw used portion
  // ==========================================

  for (
    float angle = 0.0;
    angle < usedAngle;
    angle += 0.5
  ) {

    float currentAngle =
      START_ANGLE + angle;

    float radians =
      currentAngle * PI / 180.0;


    // ========================================
    // Draw thick arc
    // ========================================

    for (
      int t = 0;
      t < THICKNESS;
      t++
    ) {

      int r = radius - t;

      int x =
        centerX +
        cos(radians) * r;

      int y =
        centerY +
        sin(radians) * r;


      display.drawPixel(
        x,
        y,
        SH110X_WHITE
      );
    }
  }
}


// ==========================================
// Draw Centered Text
// ==========================================

void drawCenteredText(
  const char* text,
  int centerX,
  int y
) {

  int16_t x1;
  int16_t y1;

  uint16_t width;
  uint16_t height;


  display.getTextBounds(
    text,
    0,
    y,
    &x1,
    &y1,
    &width,
    &height
  );


  int x =
    centerX -
    (width / 2);


  display.setCursor(
    x,
    y
  );

  display.print(text);
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

  display.setTextColor(
    SH110X_WHITE
  );

  display.setTextSize(1);

  display.setCursor(
    0,
    0
  );

  display.println(
    "Connecting WiFi..."
  );

  display.display();


  // ==========================================
  // Connect WiFi
  // ==========================================

  WiFi.begin(
    ssid,
    password
  );


  while (
    WiFi.status() != WL_CONNECTED
  ) {

    delay(500);
  }


  // ==========================================
  // WiFi Connected
  // ==========================================

  Serial.println(
    "WiFi connected"
  );

  Serial.print(
    "ESP8266 IP: "
  );

  Serial.println(
    WiFi.localIP()
  );


  // ==========================================
  // Start Server
  // ==========================================

  server.begin();

  Serial.println(
    "Server started"
  );


  // ==========================================
  // Ready Screen
  // ==========================================

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(
    0,
    0
  );

  display.println(
    "Smart Monitor"
  );

  display.setCursor(
    0,
    20
  );

  display.println(
    "WiFi Connected"
  );

  display.setCursor(
    0,
    35
  );

  display.print(
    "IP: "
  );

  display.println(
    WiFi.localIP()
  );

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
      millis() - startTime > 500
    ) {

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
    request.indexOf(
      "GET /update?"
    ) >= 0
  ) {

    cpuUsage = getParameter(
      request,
      "?cpu="
    );

    cpuTemp = getParameter(
      request,
      "&cput="
    );

    gpuUsage = getParameter(
      request,
      "&gpu="
    );

    gpuTemp = getParameter(
      request,
      "&gput="
    );


    // Record update

    lastPCUpdate =
      millis();


    // ========================================
    // HTTP response
    // ========================================

    client.println(
      "HTTP/1.1 200 OK"
    );

    client.println(
      "Content-Type: text/plain"
    );

    client.println(
      "Content-Length: 2"
    );

    client.println(
      "Connection: close"
    );

    client.println();

    client.print("OK");

    client.flush();

  } else {

    client.println(
      "HTTP/1.1 404 Not Found"
    );

    client.println(
      "Content-Type: text/plain"
    );

    client.println(
      "Content-Length: 9"
    );

    client.println(
      "Connection: close"
    );

    client.println();

    client.print(
      "Not Found"
    );

    client.flush();
  }


  client.stop();
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
    (
      millis() - lastPCUpdate
    ) < PC_TIMEOUT;


  // ==========================================
  // Gauge positions
  // ==========================================

  const int CPU_X = 32;
  const int GPU_X = 96;

  const int CENTER_Y = 32;

  const int RADIUS = 28;


  // ==========================================
  // Draw Gauges
  // ==========================================

  if (pcConnected) {

    drawCircularGauge(
      CPU_X,
      CENTER_Y,
      RADIUS,
      cpuUsage
    );

    drawCircularGauge(
      GPU_X,
      CENTER_Y,
      RADIUS,
      gpuUsage
    );
  }


  // ==========================================
  // Text
  // ==========================================

  display.setTextSize(1);


  // ==========================================
  // CPU values
  // ==========================================

  if (pcConnected) {

    char cpuUsageText[8];
    char cpuTempText[8];


    snprintf(
      cpuUsageText,
      sizeof(cpuUsageText),
      "%.0f%%",
      cpuUsage
    );


    snprintf(
      cpuTempText,
      sizeof(cpuTempText),
      "%.0fC",
      cpuTemp
    );


    // Usage centered at CPU gauge center

    drawCenteredText(
      cpuUsageText,
      CPU_X,
      27
    );


    // Temperature centered at CPU gauge center

    drawCenteredText(
      cpuTempText,
      CPU_X,
      38
    );

  } else {

    drawCenteredText(
      "--%",
      CPU_X,
      27
    );

    drawCenteredText(
      "--C",
      CPU_X,
      38
    );
  }


  // ==========================================
  // GPU values
  // ==========================================

  if (pcConnected) {

    char gpuUsageText[8];
    char gpuTempText[8];


    snprintf(
      gpuUsageText,
      sizeof(gpuUsageText),
      "%.0f%%",
      gpuUsage
    );


    snprintf(
      gpuTempText,
      sizeof(gpuTempText),
      "%.0fC",
      gpuTemp
    );


    // Usage centered at GPU gauge center

    drawCenteredText(
      gpuUsageText,
      GPU_X,
      27
    );


    // Temperature centered at GPU gauge center

    drawCenteredText(
      gpuTempText,
      GPU_X,
      38
    );

  } else {

    drawCenteredText(
      "--%",
      GPU_X,
      27
    );

    drawCenteredText(
      "--C",
      GPU_X,
      38
    );
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

  // Receive PC data
  handlePCRequest();


  // Update display
  if (
    millis() - lastDisplayUpdate
    >= DISPLAY_INTERVAL
  ) {

    lastDisplayUpdate =
      millis();

    updateDisplay();
  }
}