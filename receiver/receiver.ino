#include <WiFi.h>
#include <esp_now.h>

#define UART_TX_PIN 17
#define UART_RX_PIN 16

#define START_CTRL 0xAA
#define START_TLM  0x55

typedef struct __attribute__((packed)) {
  uint16_t ch1;
  uint16_t ch2;
  uint16_t ch3;
  uint16_t ch4;
  uint8_t  btn1;
  uint8_t  btn2;
  uint8_t  throttle;
} espnow_message_t;

typedef struct __attribute__((packed)) {
  int16_t  altitude_10m;
  uint16_t speed_kmh;
} telemetry_message_t;

telemetry_message_t telemetry;
HardwareSerial nanoSerial(2);

uint8_t txMAC[6];
bool txPeerAdded = false;

/* ================= ESP-NOW RX ================= */

void onDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *incomingData,
                int len) {

                  
  if (len != sizeof(espnow_message_t)) return;


  // ---- Forward control to Nano ----
  nanoSerial.write(START_CTRL);
  nanoSerial.write(incomingData, sizeof(espnow_message_t));

  // ---- Add peer once ----
  if (!txPeerAdded) {
    memcpy(txMAC, info->src_addr, 6);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, txMAC, 6);
    peer.channel = 0;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) == ESP_OK) {
      txPeerAdded = true;
    }
  }

  // ---- Send telemetry back ----
  if (txPeerAdded) {
    esp_now_send(txMAC,
                 (uint8_t *)&telemetry,
                 sizeof(telemetry));
  }
}

/* ================= SETUP ================= */

void setup() {
  Serial.begin(115200);              // USB debug
  nanoSerial.begin(57600, SERIAL_8N1,
                   UART_RX_PIN, UART_TX_PIN);

  WiFi.mode(WIFI_STA);
  esp_now_init();
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("ESP32 RX ready");
}

/* ================= LOOP ================= */

void loop() {





  static uint8_t idx = 0;
  static uint8_t buf[sizeof(telemetry_message_t)];

  while (nanoSerial.available()) {
    uint8_t b = nanoSerial.read();

    if (idx == 0 && b != START_TLM) continue;

    if (idx > 0) buf[idx - 1] = b;
    idx++;

    if (idx == sizeof(telemetry_message_t) + 1) {
      memcpy(&telemetry, buf, sizeof(telemetry));
      idx = 0;
    }
  }
}
