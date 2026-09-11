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
// PC Connection Tracking
// ==========================================

// Time of the last successful PC update
unsigned long lastPCUpdate = 0;

// Consider PC disconnected after 5 seconds
const unsigned long PC_TIMEOUT = 5000;


// ==========================================
// Extract Parameter From HTTP Request
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
  // Connect to WiFi
  // ==========================================

  WiFi.begin(
    ssid,
    password
  );


  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
  }


  // ==========================================
  // WiFi Connected
  // ==========================================

  Serial.println("WiFi connected");

  Serial.print("ESP8266 IP: ");
  Serial.println(WiFi.localIP());


  // ==========================================
  // Start Web Server
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
  display.println(WiFi.localIP());

  display.display();

  delay(2000);
}


// ==========================================
// Handle PC Request
// ==========================================

void handlePCRequest() {

  WiFiClient client = server.available();

  if (!client) {
    return;
  }

  unsigned long startTime = millis();

  while (!client.available()) {

    if (millis() - startTime > 500) {
      client.stop();
      return;
    }

    delay(1);
  }

  // Read first HTTP request line
  String request = client.readStringUntil('\r');

  // Consume the '\n'
  client.read();

  // Process update request
  if (request.indexOf("GET /update?") >= 0) {

    cpuUsage = getParameter(request, "cpu=");
    cpuTemp  = getParameter(request, "cput=");
    gpuUsage = getParameter(request, "gpu=");
    gpuTemp  = getParameter(request, "gput=");

    lastPCUpdate = millis();

    // HTTP response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Content-Length: 2");
    client.println("Connection: close");
    client.println();
    client.print("OK");

    // Make sure response is transmitted
    client.flush();

    // Give TCP stack a moment
    delay(5);

  } else {

    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/plain");
    client.println("Content-Length: 9");
    client.println("Connection: close");
    client.println();
    client.print("Not Found");

    client.flush();

    delay(5);
  }

  client.stop();
}


// ==========================================
// Display
// ==========================================

void updateDisplay() {

  // ==========================================
  // Clear Display
  // ==========================================

  display.clearDisplay();


  // ==========================================
  // Check PC Connection
  // ==========================================

  bool pcConnected =
    (millis() - lastPCUpdate) < PC_TIMEOUT;


  // ==========================================
  // CPU
  // ==========================================

  display.setTextSize(2);

  display.setCursor(12, 0);

  display.print("CPU");


  display.setTextSize(2);

  display.setCursor(12, 25);

  if (pcConnected) {

    display.print(cpuUsage, 0);

    display.println("%");

    display.setCursor(12, 44);

    display.print(cpuTemp, 0);

    display.println("C");

  } else {

    display.println("Usage: --%");
    display.println("Temp:  --C");
  }


  // ==========================================
  // GPU
  // ==========================================

  display.setTextSize(2);

  display.setCursor(80, 0);

  display.print("GPU");


  display.setTextSize(2);

  display.setCursor(80, 25);

  if (pcConnected) {

    display.print(gpuUsage, 0);

    display.println("%");

    display.setCursor(80, 44);

    display.print(gpuTemp, 0);

    display.println("C");

  } else {

    display.println("Use: --%");
    display.println("Tmp: --C");
  }


  // ==========================================
  // Display Update
  // ==========================================

  display.display();
}


// ==========================================
// Loop
// ==========================================

void loop() {

  // Receive PC data
  handlePCRequest();


  // Update OLED
  updateDisplay();


  // Refresh every 500 ms
  delay(500);
}