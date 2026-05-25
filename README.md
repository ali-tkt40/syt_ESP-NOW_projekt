# ITP-Projekt: Drahtlose Sensor-Erfassung & Visualisierung via ESP-NOW

**Verfasser:** Ali Tanriut, Djordje Stojanovic  
**Datum:** 25.05.2026  

---

## 1. Einführung
In der modernen Automatisierungstechnik und im Internet of Things (IoT) spielt die effiziente, drahtlose Datenübertragung zwischen Kleinstgeräten eine zentrale Rolle. Häufig wird hier dafür auf klassische WLAN-Verbindungen über einen zentralen Router zurückgegriffen, was jedoch zu Latenzen und erhöhtem Energieverbrauch führen kann. Das von Espressif entwickelte ESP-NOW-Protokoll bietet hier eine ressourcenschonende Alternative, da es eine direkte Peer-to-Peer-Kommunikation ohne Accesspoint ermöglicht. Im Rahmen dieses Projekts wird eine Sensor-Aktor-Infrastruktur auf Basis zweier ESP32-Mikrocontroller evaluiert und implementiert.

---

## 2. Projektbeschreibung
Es wurde ein verteiltes IoT-System realisiert, bei dem zwei ESP32-Module drahtlos über ESP-NOW miteinander kommunizieren. Die Sender-Station erfasst kontinuierlich die Pegel eines PIR-Bewegungssensors sowie eines Flammensensors und übermittelt diese im Millisekundenbereich an den Empfänger. Die Empfänger-Station übernimmt die Aggregation, die Steuerung der Alarm-Aktorik (RGB-LED und Buzzer), die lokale Visualisierung über ein OLED-Display und stellt die Messwerte über ein integriertes HTTP-Webinterface in einem eigens aufgespannten Accesspoint zur Verfügung.

---

## 3. Theorie

### ESP-NOW Protokoll
ESP-NOW ist ein von Espressif entwickeltes verbindungsloses Kommunikationsprotokoll, das auf der 2,4-GHz-Frequenz arbeitet. Es ähnelt der MAC-Schicht von klassischen WLAN-Netzwerken, überspringt jedoch den zeit- und energieaufwendigen Handshake-Prozess eines Standard-Wi-Fi-Verbindungsaufbaus. Dadurch können Datenpakete extrem schnell und mit minimalem Strombedarf direkt von einem Mikrocontroller zum anderen übertragen werden.

### Sensorik und Signalverarbeitung
Im System kommen zwei unterschiedliche Sensortypen zum Einsatz. Der PIR-Sensor (Passive Infrared) reagiert auf Temperaturveränderungen in der Umgebung, um Bewegungen von Personen zu detektieren. Der Flammensensor erfasst die Infrarotstrahlung einer offenen Flamme. Beide Sensoren liefern digitale Signale, die vom Sender-ESP32 eingelesen, in eine strukturierte Nachricht verpackt und als Datenpaket für den Versand via ESP-NOW freigegeben werden.

---

## 4. Arbeitsschritte

### 4.1 Hardware-Aufbau und Sensorik
* Sender-Knoten (ESP1): Der Flammensensor (Pin 4) und der PIR-Sensor (Pin 5) wurden an die digitalen GPIO-Pins des ersten ESP32 angeschlossen und als Eingang (INPUT) konfiguriert.
* Empfänger-Knoten (ESP2): Das OLED-Display wurde über den I2C-Bus (Pins 23 und 22) verdrahtet. Die drei Farbkanäle der RGB-LED wurden an die Pins 25 (Rot), 26 (Grün) und 27 (Blau) angeschlossen. Der Buzzer wurde als akustischer Signalgeber an Pin 18 angeschlossen. All diese Pins wurden als Ausgang (OUTPUT) definiert.

### 4.2 Software-Implementierung
Der Code wurde in C++ für die Arduino-IDE entwickelt. Dabei wurde eine identische Datenstruktur (`struct_message`) auf beiden Geräten definiert, um eine fehlerfreie Datenübergabe zu gewährleisten.

Code vom ESP1(Sender)
```cpp
#include <WiFi.h>      // Bibliothek für die WLAN-Funktionen des ESP32
#include <esp_now.h>   // Bibliothek für das verbindunglose ESP-NOW Protokoll

// Definition der GPIO-Pins für die angeschlossenen Sensoren
#define FLAME_PIN 4
#define PIR_PIN 5

// Eindeutige MAC-Adresse des Empfänger-ESP32 (muss exakt übereinstimmen)
uint8_t receiverMAC[] = {0x00, 0x70, 0x07, 0x19, 0xEF, 0x0C};

// Definition einer Struktur, um mehrere Datenfelder kompakt in einem Paket zu bündeln
typedef struct struct_message {
  bool flame;   // Zustand des Flammensensors (true/false)
  bool motion;  // Zustand des Bewegungssensors (true/false)
} struct_message;

// Erstellen einer Instanz der Struktur mit dem Namen "data"
struct_message data;

void setup() {
  // Seriellen Monitor für Debug-Ausgaben mit 115200 Baud starten
  Serial.begin(115200);
  delay(1000); // Kurze Pause, damit die serielle Verbindung stabil steht

  // Sensor-Pins als digitale Eingänge konfigurieren
  pinMode(FLAME_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  // WLAN in den Station-Modus versetzen (wird zwingend für ESP-NOW benötigt)
  WiFi.mode(WIFI_STA);

  //  ESP-NOW START CHECK
  // Initialisiert das ESP-NOW Protokoll und bricht bei Fehlern ab
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FEHLER!");
    return;
  }

  //  PEER SETUP
  // Informationen über das Empfängergerät (Peer) konfigurieren
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6); // Kopiert die MAC-Adresse in die Peer-Struktur
  peerInfo.channel = 0;                        // Kanal 0 nutzt den aktuellen Wi-Fi Kanal
  peerInfo.encrypt = false;                    // Übertragung wird nicht verschlüsselt

  // Registriert den Empfänger-ESP32 im System des Senders
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("PEER FEHLER!");
    return;
  }

  Serial.println("ESP1 sendet...");
}

void loop() {
  // Sensordaten aktuell einlesen und in der Datenstruktur speichern
  data.flame = digitalRead(FLAME_PIN);
  data.motion = digitalRead(PIR_PIN);

  // Sendet das gesamte Datenpaket an die definierte MAC-Adresse
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&data, sizeof(data));

  // Ausgabe der gelesenen Werte und des Sendestatus auf dem seriellen Monitor
  Serial.print("Flame: ");
  Serial.print(data.flame);
  Serial.print(" | Motion: ");
  Serial.print(data.motion);
  Serial.print(" | Send: ");
  // Überprüfung, ob die Übertragung erfolgreich war (ESP_OK)
  Serial.println(result == ESP_OK ? "OK" : "FAIL");

  // Wartezeit von 500 Millisekunden bis zur nächsten Messung/Übertragung
  delay(500);
}
```

*Beschreibung des Sender-Codes:* Die Struktur sorgt dafür, dass die Zustände von Flamme und Bewegung kompakt gebündelt übertragen werden. Im setup() wird der WLAN-Stack im Modus WIFI_STA initialisiert und der Empfänger über seine eindeutige MAC-Adresse registriert. Die loop() liest die Sensoren zyklisch ein und stößt alle 500 Millisekunden die Übertragung an.

---

Code vom ESP2(Empfänger)
```cpp
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
```

*Beschreibung des Empfänger-Codes:* Nach dem Start initialisiert der Empfänger das OLED-Display und startet einen eigenen WLAN-Accesspoint mit dem Namen "ESP32-ALARM". Sobald Daten über die registrierte Callback-Funktion `onReceive` eingehen, wird die Alarm-Logik abgearbeitet, das Display aktualisiert und der Webserver bedient.

### 4.3 Integration des Webinterfaces und Accesspoints
Der Empfänger-ESP32 öffnet ein eigenes drahtloses Netzwerk. Sobald sich ein Endgerät mit diesem Accesspoint verbindet, kann das Webinterface über die statische IP-Adresse **192.168.4.1** aufgerufen werden. Die HTML-Seite aktualisiert sich im Sekundentakt automatisch selbst, stellt die Zustände der Sensoren dar und bietet ein Formular, über das die RGB-LED remote deaktiviert oder aktiviert werden kann.

---

## 5. Systemfunktionen im Betrieb

Das System reagiert im laufenden Betrieb dynamisch auf die empfangenen Sensorwerte des Senders:

* **Zustand "Alles OK":** Es wird weder eine Flamme noch eine Bewegung registriert. Die RGB-LED leuchtet dauerhaft **Grün**, der Buzzer ist stumm und das Display zeigt bei beiden Sensoren den Status "OK" bzw. "NONE".
* **Zustand "Bewegung erkannt":** Der PIR-Sensor schlägt an. Die RGB-LED wechselt sofort auf die Farbe **Blau**, der Buzzer schaltet sich ein und das Display signalisiert "DETECTED".
* **Zustand "Flammen-Alarm":** Der Flammensensor registriert ein Feuer. Die RGB-LED wechselt sofort auf die Farbe **Rot**, der Buzzer gibt einen Alarmton aus und das Display signalisiert "ALARM".

---

## 6. Bilder und Schaltungen

### Komponentenliste
* 2x ESP32 NodeMCU Modul
* 1x PIR-Bewegungssensor
* 1x Flammensensor (Flame B05)
* 1x I2C OLED-Display (SSD1306)
* 1x RGB-LED
* 1x Aktiv-Buzzer
* Breadboards und Jumper-Kabel

<img width="897" height="531" alt="ESP1_Schaltplan" src="https://github.com/user-attachments/assets/634afbec-115a-439a-8f95-3f3aac4b1a37" />
<img width="1317" height="801" alt="ESP2_Schaltplan" src="https://github.com/user-attachments/assets/d4a95685-24f9-4213-9bd7-a09ffc32eec4" />


