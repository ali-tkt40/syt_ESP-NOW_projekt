# ITP-Projekt: Drahtlose Sensor-Erfassung & Visualisierung via ESP-NOW

**Verfasser:** Dein Name / Gruppennamen  
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

[Hier eigenen Programmcode für den SENDER einfügen]

*Beschreibung des Sender-Codes:* Die Struktur sorgt dafür, dass die Zustände von Flamme und Bewegung kompakt gebündelt übertragen werden. Im setup() wird der WLAN-Stack im Modus WIFI_STA initialisiert und der Empfänger über seine eindeutige MAC-Adresse registriert. Die loop() liest die Sensoren zyklisch ein und stößt alle 500 Millisekunden die Übertragung an.

---

[Hier eigenen Programmcode für den EMPFÄNGER einfügen]

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

*(Hinweis: Die physischen Fotos des Aufbaus, die Fritzing-Schaltpläne sowie die interaktiven Simulations-Links zu Tinkercad/Wokwi sind vollständig im Projekt-Repository unter /schaltplaene und /fotos hinterlegt.)*
