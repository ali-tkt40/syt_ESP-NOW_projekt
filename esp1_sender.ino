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
