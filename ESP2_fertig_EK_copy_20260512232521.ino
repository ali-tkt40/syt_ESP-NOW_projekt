#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// 📺 Display Einstellungen
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// 🔌 RGB Pins
#define RED 25
#define GREEN 26
#define BLUE 27

// ✅ Buzzer Pin
#define BUZZER_PIN 18

// ✅ RGB Toggle Status
bool rgbEnabled = true;

// 📦 Daten
typedef struct struct_message {
  bool flame;
  bool motion;
} struct_message;

struct_message incomingData;

// 🌐 Webserver
WebServer server(80);

// 📊 aktuelle Werte
String flameState = "0";
String motionState = "0";

void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("STATUS:");
  
  display.setTextSize(1);
  display.setCursor(0, 30);
  display.print("Flame: ");
  display.println(flameState == "1" ? "ALARM" : "OK");
  
  display.setCursor(0, 50);
  display.print("Motion: ");
  display.println(motionState == "1" ? "DETECTED" : "NONE");
  
  display.display();
}

void onReceive(const esp_now_recv_info *info, const uint8_t *data, int len) {
  memcpy(&incomingData, data, sizeof(incomingData));

  flameState = incomingData.flame ? "1" : "0";
  motionState = incomingData.motion ? "1" : "0";

  Serial.print("Flame: ");
  Serial.print(flameState);
  Serial.print(" | Motion: ");
  Serial.println(motionState);

  updateDisplay();

  if (rgbEnabled) {
    if (incomingData.flame) {
      digitalWrite(RED, HIGH);
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, LOW);
    } else if (incomingData.motion) {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, HIGH);
    } else {
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, HIGH); // ✅ Grün wenn alles OK
  digitalWrite(BLUE, LOW);
}
  }

  if (incomingData.flame || incomingData.motion) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void handleRoot() {
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='1'>";
  html += "</head><body>";
  html += "<h1>ESP32 Status</h1>";
  html += "<p> Flame: " + flameState + "</p>";
  html += "<p> Motion: " + motionState + "</p>";
  html += "<p> RGB LED: <strong>" + String(rgbEnabled ? "AN" : "AUS") + "</strong></p>";
  html += "<form action='/toggleRGB' method='GET'>";
  html += "<button type='submit' style='padding:10px 20px;font-size:16px;'>";
  html += rgbEnabled ? "RGB Ausschalten" : "RGB Einschalten";
  html += "</button></form>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleToggleRGB() {
  rgbEnabled = !rgbEnabled;
  if (!rgbEnabled) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200);

  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Wire.begin(23, 22);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 Fehler"));
  }
  display.clearDisplay();
  display.display();

  // 📡 Accesspoint + ESP-NOW
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32-ALARM", "12345678");

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP()); // immer 192.168.4.1

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Fehler!");
    return;
  }

  esp_now_register_recv_cb(onReceive);

  server.on("/", handleRoot);
  server.on("/toggleRGB", handleToggleRGB);
  server.begin();

  Serial.println("Webserver gestartet");
}

void loop() {
  server.handleClient();
}