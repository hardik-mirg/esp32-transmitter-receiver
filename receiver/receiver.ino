#include <WiFi.h>
#include <esp_now.h>

// UART pins (ESP32 → Nano)
#define UART_TX_PIN 17
#define UART_RX_PIN 16   // Required by ESP32, not used

#define START_BYTE 0xAA

typedef struct __attribute__((packed)) {
  uint16_t ch1;
  uint16_t ch2;
  uint16_t ch3;
  uint16_t ch4;
  uint8_t  btn1;
  uint8_t  btn2;
} espnow_message_t;

HardwareSerial nanoSerial(2);

void onDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(espnow_message_t)) return;

  // Frame-safe transmit
  nanoSerial.write(START_BYTE);
  nanoSerial.write(incomingData, sizeof(espnow_message_t));
}

void setup() {
  nanoSerial.begin(57600, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    return; // silent fail
  }

  esp_now_register_recv_cb(onDataRecv);
}

void loop() {
  // nothing — event driven
}
