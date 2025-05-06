#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>

const char* ssid = "SLT Repeat";
const char* password = "JT%479#jAB";

const int ledPin = 2;

WebServer server(80);

TaskHandle_t BlinkTaskHandle;


void BlinkTask(void * parameter) {
  while (true) {
    Serial.println("Blinking LED");
    digitalWrite(ledPin, HIGH);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    digitalWrite(ledPin, LOW);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  } 
}

void handleRoot() {
  String html = "<h2>ESP32 Web Server</h2>";
  html += "<p><a href=\"/on\">Turn LED ON</a></p>";
  html += "<p><a href=\"/off\">Turn LED OFF</a></p>";
  html += "<p><a href=\"/blink\">Blink LED</a></p>";
  html += "<p><a href=\"/update\">OTA Update</a></p>";
  server.send(200, "text/html", html);
}

void handleOn() {
  vTaskSuspend(BlinkTaskHandle);
  digitalWrite(ledPin, HIGH);
  Serial.println("LED On");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleOff() {
  vTaskSuspend(BlinkTaskHandle);
  digitalWrite(ledPin, LOW);
  Serial.println("LED Off");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleBlink() {
  vTaskResume(BlinkTaskHandle);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleUpdate() {
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

  xTaskCreate(BlinkTask, "BlinkTask", 1000, NULL, 1, &BlinkTaskHandle);
  vTaskSuspend(BlinkTaskHandle);

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/blink", handleBlink);
  server.on("/update", HTTP_GET, handleUpdate);
  server.on("/update", HTTP_POST, []() {}, handleUpdateUpload);

  server.begin();
  Serial.println("Web server started.");
}

void loop() {
  server.handleClient();
}
