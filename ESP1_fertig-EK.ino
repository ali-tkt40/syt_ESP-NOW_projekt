#include <WiFi.h>
#include <esp_now.h>

#define FLAME_PIN 4
#define PIR_PIN 5

uint8_t receiverMAC[] = {0x00, 0x70, 0x07, 0x19, 0xEF, 0x0C};

typedef struct struct_message {
  bool flame;
  bool motion;
} struct_message;

struct_message data;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(FLAME_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);

  WiFi.mode(WIFI_STA);

  // 🔥 ESP-NOW START CHECK
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FEHLER!");
    return;
  }

  // 🔥 PEER SETUP
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("PEER FEHLER!");
    return;
  }

  Serial.println("ESP1 sendet...");
}

void loop() {
  data.flame = digitalRead(FLAME_PIN);
  data.motion = digitalRead(PIR_PIN);

  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&data, sizeof(data));

  Serial.print("Flame: ");
  Serial.print(data.flame);
  Serial.print(" | Motion: ");
  Serial.print(data.motion);
  Serial.print(" | Send: ");
  Serial.println(result == ESP_OK ? "OK" : "FAIL");

  delay(500);
}