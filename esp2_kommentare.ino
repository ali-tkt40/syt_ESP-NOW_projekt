#include <WiFi.h>              // Bibliothek für die WLAN-Funktionen des ESP32
#include <esp_now.h>           // Bibliothek für das verbindunglose ESP-NOW Protokoll
#include <WebServer.h>         // Bibliothek für die Bereitstellung des Webinterfaces
#include <Wire.h>              // I2C-Bibliothek für die Kommunikation mit dem Display
#include <Adafruit_GFX.h>      // Grafik-Bibliothek von Adafruit für Text und Formen
#include <Adafruit_SSD1306.h>  // Treiber-Bibliothek für das SSD1306 OLED-Display

// Display Einstellungen
#define SCREEN_WIDTH 128       // Breite des Displays in Pixeln
#define SCREEN_HEIGHT 64       // Höhe des Displays in Pixeln
// Initialisierung des OLED-Displays über den I2C-Bus (-1 bedeutet kein Reset-Pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// RGB LED Pins
#define RED 25
#define GREEN 26
#define BLUE 27

// Buzzer Pin
#define BUZZER_PIN 18

// Variable zum Speichern des aktuellen LED-Zustands (Ein/Aus via Webinterface)
bool rgbEnabled = true;

// Struktur für die zu empfangenden Sensordaten (muss exakt der des Senders entsprechen)
typedef struct struct_message {
  bool flame;
  bool motion;
} struct_message;

struct_message incomingData; // Instanz der Struktur für die empfangenen Daten

// Webserver auf Standard-Port 80 instanziieren
WebServer server(80);

// Aktuelle Werte als String für die Darstellung auf der Webseite
String flameState = "0";
String motionState = "0";

// Funktion zur Aktualisierung des OLED-Displays
void updateDisplay() {
  display.clearDisplay();              // Bildschirminhalt löschen
  display.setTextSize(2);              // Große Schrift für die Überschrift
  display.setTextColor(SSD1306_WHITE); // Textfarbe auf Weiß setzen
  display.setCursor(0,0);              // Cursor oben links positionieren
  display.println("STATUS:");
  
  display.setTextSize(1);              // Kleinere Schrift für die Messwerte
  display.setCursor(0, 30);
  display.print("Flame: ");
  display.println(flameState == "1" ? "ALARM" : "OK"); // Alarm-Text bei Flammenaktivierung
  
  display.setCursor(0, 50);
  display.print("Motion: ");
  display.println(motionState == "1" ? "DETECTED" : "NONE"); // Status der Bewegung
  
  display.display();                   // Aktualisierten Inhalt physisch auf dem Display ausgeben
}

// Callback-Funktion: Wird automatisch aufgerufen, wenn Daten über ESP-NOW empfangen werden
void onReceive(const esp_now_recv_info *info, const uint8_t *data, int len) {
  // Kopiert die empfangenen Rohdaten direkt in unsere Datenstruktur
  memcpy(&incomingData, data, sizeof(incomingData));

  // Konvertiert die Bool-Werte in Strings für die Weiterverarbeitung
  flameState = incomingData.flame ? "1" : "0";
  motionState = incomingData.motion ? "1" : "0";

  // Ausgabe auf dem seriellen Monitor zur Kontrolle
  Serial.print("Flame: ");
  Serial.print(flameState);
  Serial.print(" | Motion: ");
  Serial.println(motionState);

  // Display-Inhalt mit den neuen Werten aktualisieren
  updateDisplay();

  // Logik für die Status-LED (falls im Webinterface eingeschaltet)
  if (rgbEnabled) {
    if (incomingData.flame) {
      digitalWrite(RED, HIGH);   // Rot bei Flammen-Alarm
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, LOW);
    } else if (incomingData.motion) {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, LOW);
      digitalWrite(BLUE, HIGH);  // Blau bei Bewegungserkennung
    } else {
      digitalWrite(RED, LOW);
      digitalWrite(GREEN, HIGH); // Grün, wenn alles im sicheren Bereich ist
      digitalWrite(BLUE, LOW);
    }
  }

  // Akustischer Alarm: Buzzer einschalten, wenn ein Sensor auslöst
  if (incomingData.flame || incomingData.motion) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// HTML-Hauptseite (Root-Verzeichnis des Webservers)
void handleRoot() {
  String html = "<html><head>";
  html += "<meta http-equiv='refresh' content='1'>"; // Seite alle 1 Sekunde automatisch neu laden
  html += "</head><body>";
  html += "<h1>ESP32 Status</h1>";
  html += "<p> Flame: " + flameState + "</p>";
  html += "<p> Motion: " + motionState + "</p>";
  html += "<p> RGB LED: <strong>" + String(rgbEnabled ? "AN" : "AUS") + "</strong></p>";
  
  // HTML-Formular mit einem Button zum Umschalten der LED-Steuerung
  html += "<form action='/toggleRGB' method='GET'>";
  html += "<button type='submit' style='padding:10px 20px;font-size:16px;'>";
  html += rgbEnabled ? "RGB Ausschalten" : "RGB Einschalten";
  html += "</button></form>";
  html += "</body></html>";
  
  server.send(200, "text/html", html); // Antwortet dem Browser mit dem HTML-Inhalt
}

// Funktion zum Umschalten der RGB-LED per HTTP-Request
void handleToggleRGB() {
  rgbEnabled = !rgbEnabled; // Zustand invertieren (True -> False / False -> True)
  
  // Wenn die LED ausgeschaltet wurde, sofort alle Farbkanäle deaktivieren
  if (!rgbEnabled) {
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }
  
  // HTTP-Redirect (Status 303) zurück auf die Hauptseite
  server.sendHeader("Location", "/");
  server.send(303);
}

void setup() {
  Serial.begin(115200); // Seriellen Monitor starten

  // Alle Ausgangspins für Aktorik definieren
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Buzzer initial ausschalten

  // I2C-Bus mit spezifischen Pins (SDA: 23, SCL: 22) initialisieren
  Wire.begin(23, 22);
  // Initialisierung des OLED-Displays mit der I2C-Adresse 0x3C
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 Fehler"));
  }
  display.clearDisplay();
  display.display();

  // Netzwerk-Konfiguration: Accesspoint + Station-Modus aktivieren
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32-ALARM", "12345678"); // Erstellt ein eigenes WLAN-Netzwerk

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP()); // Standardmäßig erreichbar unter 192.168.4.1

  // ESP-NOW aktivieren und Funktion bei Fehlern abbrechen
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Fehler!");
    return;
  }

  // Registriert die oben definierte Funktion als Empfangs-Callback
  esp_now_register_recv_cb(onReceive);

  // Routing für den Webserver festlegen
  server.on("/", handleRoot);
  server.on("/toggleRGB", handleToggleRGB);
  server.begin(); // Webserver starten

  Serial.println("Webserver gestartet");
}

void loop() {
  server.handleClient(); // Fortlaufend eingehende HTTP-Anfragen des Webservers verarbeiten
}