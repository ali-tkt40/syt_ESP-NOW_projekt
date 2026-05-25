# SYT_ESP-NOW_Projekt
# ITP-Projekt: Drahtlose Sensor-Erfassung & Visualisierung via ESP-NOW

**Verfasser:** Ali Ahmet Tanrikut, Djordje Stojanovic
**Datum:** 25.05.2026  

---

## 1. Einführung
In der modernen Automatisierungstechnik und im Internet of Things (IoT) spielt die effiziente, drahtlose Datenübertragung zwischen Kleinstgeräten eine zentrale Rolle. Häufig wird hier dafür auf klassische WLAN-Verbindungen über einen zentralen Router zurückgegriffen, was jedoch zu Latenzen und erhöhtem Energieverbrauch führen kann. Das von Espressif entwickelte ESP-NOW-Protokoll bietet hier eine ressourcenschonende Alternative, da es eine direkte Peer-to-Peer-Kommunikation ohne Accesspoint ermöglicht. Im Rahmen dieses Projekts wird eine energieeffiziente Sensor-Aktor-Infrastruktur auf Basis zweier ESP32-Mikrocontroller evaluiert und implementiert.

---

## 2. Projektbeschreibung
Es wurde ein verteiltes IoT-System realisiert, bei dem zwei ESP32-Module drahtlos über ESP-NOW miteinander kommunizieren. Die Sender-Station erfasst kontinuierlich die Pegel eines PIR-Bewegungssensors sowie eines Flame-B05-Flammensensors, bereinigt diese Daten softwareseitig und übermittelt sie in Echtzeit. Die Empfänger-Station übernimmt die Aggregation, die lokale Visualisierung über ein Display sowie eine RGB-Status-LED und stellt die bereinigten Messwerte inklusive NTP-Zeitstempel über ein integriertes HTTP-Webinterface im lokalen Netzwerk zur Verfügung.

---

## 3. Theorie

### ESP-NOW Protokoll
ESP-NOW ist ein von Espressif entwickeltes verbindungsloses Kommunikationsprotokoll, das auf der 2,4-GHz-Frequenz arbeitet. Es ähnelt der MAC-Schicht von klassischen WLAN-Netzwerken, überspringt jedoch den zeit- und energieaufwendigen Handshake-Prozess eines Standard-Wi-Fi-Verbindungsaufbaus. Dadurch können Datenpakete extrem schnell (im Millisekundenbereich) und mit minimalem Strombedarf übertragen werden.

### Datenbereinigung & Signalverarbeitung
Rohdaten von Sensoren sind in der Praxis oft von Rauschen oder sporadischen Fehlmessungen betroffen. Zur Stabilisierung der Werte wird ein Algorithmus zur softwareseitigen Signalglättung eingesetzt. Dabei wird kontinuierlich der gleitende Mittelwert über eine feste Anzahl der letzten Messungen gebildet. Zusätzlich filtert eine programmierte Schwellenwert-Logik physikalisch unmögliche Sprünge (Ausreißer) automatisch heraus, bevor das Datenpaket für den Versand freigegeben wird.

---

## 4. Arbeitsschritte

### 4.1 Hardware-Aufbau und Sensorik
* Sender-Knoten: Der PIR-Sensor und der Flame-B05-Sensor wurden an die digitalen GPIO-Pins des ersten ESP32 angeschlossen.
* Empfänger-Knoten: Das OLED-Display wurde über den I2C-Bus (SDA/SCL) verdrahtet. Die RGB-LED und der optionale Buzzer (Erweiterungskomponente) wurden an PWM-fähige GPIOs angeschlossen.

### 4.2 Software-Implementierung
Der Code wurde modular in C++ für die Arduino-IDE entwickelt. Der Fokus lag auf der sauberen Trennung von Sender- und Empfängerlogik.

[Hier eigenen Programmcode einfügen]

*Beschreibung des Codes:* Die Struktur sorgt dafür, dass alle Datentypen kompakt gebündelt übertragen werden. Im setup() wird der WLAN-Stack im Modus WIFI_STA initialisiert und der Empfänger über seine eindeutige MAC-Adresse als Peer registriert. Die loop() steuert die zyklische Übertragung der bereinigten Datenpakete.

### 4.3 Integration des WiFi-Managers und Webservers (Empfänger)
Der Empfänger nutzt die WiFiManager-Bibliothek. Konnte sich das Gerät nicht mit dem bekannten WLAN verbinden, öffnet der ESP32 selbstständig einen Accesspoint (AP). Nach erfolgreicher Verbindung synchronisiert das System die Uhrzeit via NTP und startet den asynchronen Webserver, um die Daten auf dem Webinterface darzustellen.

---

## 5. Bilder und Schaltungen

### Komponentenliste
* 2x ESP32 NodeMCU Modul
* 1x PIR-Bewegungssensor
* 1x Flame B05 Sensor
* 1x I2C OLED-Display (z.B. SSD1306)
* 1x RGB-LED (Common Cathode)
* 1x Aktiv-Buzzer (Erweiterungskomponente)
* Breadboards und Jumper-Kabel

*(Hinweis: Die physischen Fotos des Aufbaus, die Fritzing-Schaltpläne sowie die interaktiven Simulations-Links zu Tinkercad/Wokwi sind vollständig im Projekt-Repository unter /schaltplaene und /fotos hinterlegt.)*
