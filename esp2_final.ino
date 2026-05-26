#include <WiFi.h>              // Bibliothek für die WLAN-Funktionen des ESP32
#include <esp_now.h>           // Bibliothek für das verbindunglose ESP-NOW Protokoll
#include <WebServer.h>         // Bibliothek für die Bereitstellung des Webinterfaces
#include <Wire.h>              // I2C-Bibliothek für die Kommunikation mit dem Display
#include <Adafruit_GFX.h>      // Grafik-Bibliothek von Adafruit für Text und Formen
#include <Adafruit_SSD1306.h>  // Treiber-Bibliothek für das SSD1306 OLED-Display
#include <time.h>              // Zeit-Bibliothek für die Berechnung des Zeitstempels

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

// Zeitvariablen für den Zeitstempel
unsigned long initialEpoch = 0; // Speichert die vom Browser übergebene UTC-Zeit
unsigned long syncMillis = 0;   // Interner CPU-Zeitpunkt zum Synchronisations-Zeitpunkt
bool timeSynced = false;        // Status, ob die Zeit bereits synchronisiert wurde

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
String lastTimestamp = "Noch kein Event"; // Speichert den Zeitpunkt der letzten Änderung

// Funktion zur Generierung des aktuellen Zeitstempels mit Zeitzonen-Korrektur
String getTimestamp() {
  if (!timeSynced) {
    // Wenn noch nicht synchronisiert, gib die Laufzeit in Sekunden aus
    return "Systemzeit: " + String(millis() / 1000) + "s";
  }
  // 7020 Sekunden entsprechen 2 Stunden minus 3 Minuten (7200 - 180), 
  // um die Abweichung der Browser-UTC-Zeit zur lokalen Echtzeit exakt auszugleichen.
  unsigned long currentEpoch = initialEpoch + 7020 + ((millis() - syncMillis) / 1000);
  time_t rawtime = (time_t)currentEpoch;
  struct tm *ti;
  ti = localtime(&rawtime);
  
  // Puffer für die Formatierung (YYYY-MM-DD HH:MM:SS)
  char buffer[25];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", 
          ti->tm_year + 1900, ti->tm_mon + 1, ti->tm_mday, 
          ti->tm_hour, ti->tm_min, ti->tm_sec);
          
  return String(buffer);
}

// Funktion zur Aktualisierung des OLED-Displays
void updateDisplay() {
  display.clearDisplay();              // Bildschirminhalt löschen
  display.setTextSize(1);              // Schriftgröße setzen
  display.setTextColor(SSD1306_WHITE); // Textfarbe auf Weiß setzen
  display.setCursor(0,0);              // Cursor oben links positionieren
  display.println("SYS-STATUS:");
  
  display.setCursor(0, 15);
  display.print("Flame: ");
  display.println(flameState == "1" ? "ALARM" : "OK"); // Alarm-Text bei Flammenaktivierung
  
  display.setCursor(0, 30);
  display.print("Motion: ");
  display.println(motionState == "1" ? "DETECTED" : "NONE"); // Status der Bewegung

  display.setCursor(0, 48);
  display.print("Zeit:");
  display.setCursor(0, 56);
  display.println(getTimestamp()); // Zeigt die korrigierte Uhrzeit unten im Display an
  
  display.display();                   // Inhalt physisch auf dem Display ausgeben
}

// Callback-Funktion: Wird automatisch aufgerufen, wenn Daten über ESP-NOW empfangen werden
void onReceive(const esp_now_recv_info *info, const uint8_t *data, int len) {
  // Kopiert die empfangenen Rohdaten direkt in unsere Datenstruktur
  memcpy(&incomingData, data, sizeof(incomingData));

  // Konvertiert die Bool-Werte in Strings für die Weiterverarbeitung
  flameState = incomingData.flame ? "1" : "0";
  motionState = incomingData.motion ? "1" : "0";
  
  // Zeitstempel für dieses spezifische Empfangs-Event festhalten
  lastTimestamp = getTimestamp();

  // Ausgabe auf dem seriellen Monitor zur Kontrolle
  Serial.print("[" + lastTimestamp + "] Flame: " + flameState + " | Motion: " + motionState + "\n");

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

// JSON-Endpunkt: Liefert aktuelle Live-Werte im Hintergrund ohne Seiten-Reload
void handleData() {
  String json = "{";
  json += "\"time\":\"" + getTimestamp() + "\",";
  json += "\"flame\":" + flameState + ",";
  json += "\"motion\":" + motionState + ",";
  json += "\"lastEvent\":\"" + lastTimestamp + "\"";
  json += "}";
  server.send(200, "application/json", json); // Sende Daten als standardisiertes JSON-Objekt
}

// HTML-Hauptseite (Wird beim Aufruf der IP-Adresse geladen)
void handleRoot() {
  // Fängt beim ersten Laden die Handy-/PC-Zeit ab, um die ESP-Uhr zu synchronisieren
  if (server.hasArg("time")) {
    initialEpoch = server.arg("time").toInt();
    syncMillis = millis();
    timeSynced = true;
  }

  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 Dashboard</title>";
  
  // JavaScript für die clientseitige Logik und das Zeichnen der Graphen
  html += "<script>";
  html += "window.onload = function() {";
  html += "  const urlParams = new URLSearchParams(window.location.search);";
  // Falls die URL noch keinen Zeitparameter hat, hänge ihn an und lade einmalig neu
  html += "  if (!urlParams.has('time') && " + String(timeSynced ? "false" : "true") + ") {";
  html += "    window.location.href = '/?time=' + Math.floor(Date.now() / 1000);";
  html += "  }";
  html += "  setInterval(updateData, 1000);"; // Startet die AJAX-Abfrage im 1-Sekunden-Takt
  html += "};";
  
  // Datenpuffer-Arrays (halten die letzten 30 Sekunden im Browser-Speicher)
  html += "let flameHistory = Array(30).fill(0);";
  html += "let motionHistory = Array(30).fill(0);";
  
  // High-Speed Zeichenfunktion über HTML5 Canvas (arbeitet 100% lokal und offline)
  html += "function drawGraph(canvasId, data, color) {";
  html += "  const canvas = document.getElementById(canvasId);";
  html += "  const ctx = canvas.getContext('2d');";
  html += "  ctx.clearRect(0, 0, canvas.width, canvas.height);"; // Alten Inhalt verwerfen
  
  // Gitter-Hintergrundlinien (0-Linie unten, 1-Linie oben) zeichnen
  html += "  ctx.strokeStyle = '#e0e0e0'; ctx.lineWidth = 1;";
  html += "  ctx.beginPath(); ctx.moveTo(0, 20); ctx.lineTo(canvas.width, 20); ctx.moveTo(0, 130); ctx.lineTo(canvas.width, 130); ctx.stroke();";
  
  // Sensorkurve zeichnen
  html += "  ctx.strokeStyle = color; ctx.lineWidth = 3; ctx.beginPath();";
  html += "  for(let i=0; i<data.length; i++) {";
  html += "    let x = (canvas.width / (data.length - 1)) * i;"; // Verteilt 30 Punkte über die Breite
  html += "    let y = data[i] === 1 ? 20 : 130;";               // Status 1 zeichnet oben, 0 zeichnet unten
  html += "    if(i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);";
  html += "  }";
  html += "  ctx.stroke();";
  html += "}";
  
  // AJAX-Funktion: Holt im Hintergrund die JSON-Daten ab und aktualisiert Text + Graphen
  html += "function updateData() {";
  html += "  fetch('/data').then(r => r.json()).then(data => {";
  html += "    document.getElementById('currTime').innerText = data.time;";
  html += "    document.getElementById('currFlame').innerText = data.flame == 1 ? 'ALARM' : 'OK';";
  html += "    document.getElementById('currMotion').innerText = data.motion == 1 ? 'BEWEGUNG' : 'KEINE';";
  html += "    document.getElementById('lastEvt').innerText = data.lastEvent;";
  
  // Schiebt den ältesten Punkt raus und fügt den neuesten hinzu (auch wenn der Wert 0 bleibt)
  html += "    flameHistory.shift(); flameHistory.push(data.flame);";
  html += "    motionHistory.shift(); motionHistory.push(data.motion);";
  
  // Beide Graphen mit den aktualisierten Arrays neu zeichnen
  html += "    drawGraph('flameCanvas', flameHistory, '#d9534f');";
  html += "    drawGraph('motionCanvas', motionHistory, '#0275d8');";
  html += "  }).catch(e => console.log('Fehler:', e));";
  html += "}";
  html += "</script>";
  
  // CSS Design-Spezifikationen für das Dashboard
  html += "<style>";
  html += "body { font-family: sans-serif; margin: 20px; background: #f4f6f9; color: #333; }";
  html += ".card { background: white; padding: 15px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); margin-bottom: 15px; }";
  html += "canvas { width: 100%; height: 150px; background: #fff; border: 1px solid #ccc; border-radius: 4px; display: block; }";
  html += ".grid { display: flex; flex-wrap: wrap; gap: 15px; } .col { flex: 1; min-width: 280px; }";
  html += "button { padding: 10px 20px; font-size: 16px; border: none; background: #222; color: #fff; border-radius: 5px; cursor: pointer; }";
  html += "span.time-info { font-size: 12px; color: #666; font-weight: normal; }"; // CSS für den Zeitraum-Text
  html += "</style>";
  
  html += "</head><body>";
  html += "<h1>ESP32 IoT Überwachung</h1>";
  html += "<p><strong>ESP-Zeit:</strong> <span id='currTime'>" + getTimestamp() + "</span></p>";
  
  // Sensorstatus-Sektion
  html += "<div class='card'>";
  html += "  <p>🔥 Flame: <strong id='currFlame'>" + String(flameState == "1" ? "ALARM" : "OK") + "</strong></p>";
  html += "  <p>🏃 Motion: <strong id='currMotion'>" + String(motionState == "1" ? "BEWEGUNG" : "KEINE") + "</strong></p>";
  html += "  <p>Letztes Event: <span id='lastEvt'>" + lastTimestamp + "</span></p>";
  html += "  <p>RGB-LED: " + String(rgbEnabled ? "AN" : "AUS") + "</p>";
  html += "  <form action='/toggleRGB' method='GET'><button type='submit'>LED Umschalten</button></form>";
  html += "</div>";
  
  // Sektion für die beiden Verlaufs-Graphen mit expliziter Zeitraum-Anzeige (Letzte 30 Sekunden)
  html += "<div class='grid'>";
  html += "  <div class='col'><h3>Flammenverlauf <span class='time-info'>(Letzte 30 Sekunden)</span></h3><canvas id='flameCanvas' width='400' height='150'></canvas></div>";
  html += "  <div class='col'><h3>Bewegungsverlauf <span class='time-info'>(Letzte 30 Sekunden)</span></h3><canvas id='motionCanvas' width='400' height='150'></canvas></div>";
  html += "</div>";
  
  html += "</body></html>";
  
  server.send(200, "text/html", html); // Übermittelt das fertige Dokument an den Client
}

// Funktion zum Umschalten der RGB-LED via Webserver-Request
void handleToggleRGB() {
  rgbEnabled = !rgbEnabled; // Zustand invertieren
  if (!rgbEnabled) {
    // Schaltet alle Farbkanäle ab, wenn die LED deaktiviert wurde
    digitalWrite(RED, LOW);
    digitalWrite(GREEN, LOW);
    digitalWrite(BLUE, LOW);
  }
  server.sendHeader("Location", "/"); // Weiterleitung zurück zur Hauptseite
  server.send(303);
}

void setup() {
  Serial.begin(115200); // Seriellen Monitor aktivieren

  // Initialisierung der Peripherie-Pins
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(BLUE, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Buzzer ausstellen

  // I2C-Bus & OLED-Display initialisieren
  Wire.begin(23, 22);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println("SSD1306 Fehler");
  }
  display.clearDisplay();
  display.display();

  // WiFi Access-Point starten
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("ESP32-ALARM", "12345678");

  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP()); // Standard-IP: 192.168.4.1

  // ESP-NOW Protokoll initialisieren
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Fehler!");
    return;
  }

  // Datenempfangs-Funktion registrieren
  esp_now_register_recv_cb(onReceive);

  // Routen-Pfade für den Webserver konfigurieren
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/toggleRGB", handleToggleRGB);
  server.begin(); // Server hochfahren

  Serial.println("Webserver gestartet");
  updateDisplay();
}

void loop() {
  server.handleClient(); // Behandelt ankommende HTTP-Webanfragen
  
  // Aktualisiert das physische OLED-Display autark alle 5 Sekunden (Uhrzeit-Update)
  static unsigned long lastDisplayUpdate = 0;
  if (millis() - lastDisplayUpdate > 5000) {
    lastDisplayUpdate = millis();
    updateDisplay();
  }
}