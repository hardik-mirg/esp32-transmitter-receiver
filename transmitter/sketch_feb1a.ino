#include <WiFi.h>
#include <esp_now.h>

// REPLACE WITH RECEIVER MAC ADDRESS
uint8_t receiverMAC[] = {0xEC, 0xE3, 0x34, 0x19, 0xCE, 0xC4};

// Structure to send data
typedef struct struct_message {
  uint16_t ch1; // right joystick y axis - D32
  uint16_t ch2; // right joystick x axis - D33
  uint16_t ch3; // left joystick y axis - D34
  uint16_t ch4; // left joystick x axis - D35
  uint8_t btn1; // D12
  uint8_t btn2; // D13

} struct_message;

struct_message outgoingData;

// Callback when data is sent
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS) Serial.println("Fail");
}


void setup() {
  Serial.begin(115200);

  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);

  // Set device as Wi-Fi station
  WiFi.mode(WIFI_STA);
  Serial.print("ESP32 MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(onDataSent);

  // Register peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESP-NOW Transmitter Ready");
}

void loop() {

  outgoingData.ch1 = analogRead(32);
  outgoingData.ch2 = analogRead(33);
  outgoingData.ch3 = analogRead(34);
  outgoingData.ch4 = analogRead(35);
  outgoingData.btn1 = !digitalRead(12);
  outgoingData.btn2 = !digitalRead(13);

  esp_err_t result = esp_now_send(receiverMAC,
                                  (uint8_t *) &outgoingData,
                                  sizeof(outgoingData));

  if (result != ESP_OK) {
    Serial.println("Error sending packet");
  }

  delay(30);
}
