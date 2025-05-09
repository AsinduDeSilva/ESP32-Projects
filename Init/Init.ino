#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

const char* ssid = "Redmi 13C";
const char* password = "19901990";

const char* http_username = "admin";
const char* http_password = "12345";

const int ledPin = 2;

WebServer server(80);

unsigned long startTime;


bool isAuthenticated() {
  if (!server.authenticate(http_username, http_password)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void handleRoot() {
  if (!isAuthenticated()) return; 

  String html = "<h2>ESP32 Web Server</h2>";
  html += "<p><a href=\"/on\">Turn LED ON</a></p>";
  html += "<p><a href=\"/off\">Turn LED OFF</a></p>";
  html += "<p><a href=\"/update\">OTA Update</a></p>";
  server.send(200, "text/html", html);
}

void handleOn() {
  if (!isAuthenticated()) return; 

  digitalWrite(ledPin, HIGH);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleOff() {
  if (!isAuthenticated()) return; 

  digitalWrite(ledPin, LOW);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleMetrics() {
  if (!isAuthenticated()) return; 

  unsigned long uptime = (millis() - startTime) / 1000;

  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t totalHeap = ESP.getHeapSize();
  float heapUsagePercent = 100.0 * (totalHeap - freeHeap) / totalHeap;

  String json = "{";
  json += "\"uptime\":" + String(uptime) + ",";
  json += "\"freeHeapBytes\":" + String(freeHeap) + ",";
  json += "\"totalHeapBytes\":" + String(totalHeap) + ",";
  json += "\"heapUsagePercent\":" + String(100.0 * (totalHeap - freeHeap) / totalHeap, 2) + ",";
  json += "\"chipTemperatureCelcius\":" + String(temperatureRead()) + ",";
  json += "\"cpuFreqMHz\":" + String(ESP.getCpuFreqMHz()) + ",";
  json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  json += "\"sdkVersion\":\"" + String(ESP.getSdkVersion()) + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

void handleUpdate() {
  if (!isAuthenticated()) return; 

  String html = R"rawliteral(
    <h2>OTA Firmware Update</h2>
    <form method='POST' action='/update' enctype='multipart/form-data' onsubmit='showMessage()'>
      <input type='file' name='update'>
      <input type='submit' value='Upload'>
    </form>
    <script>
      function showMessage() {
        alert("Update uploaded! Device will reboot shortly.");
      }
    </script>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleUpdateUpload() {
  if (!isAuthenticated()) return; 
  
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    if (!Update.begin()) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.println("Update Success: Rebooting...");
      server.send(200, "text/plain", "OK");
      delay(100); 
      ESP.restart();
    } else {
      Update.printError(Serial);
      server.send(500, "text/plain", "Update failed");
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/metrics", handleMetrics);
  server.on("/update", HTTP_GET, handleUpdate);
  server.on("/update", HTTP_POST, []() {}, handleUpdateUpload);

  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}
