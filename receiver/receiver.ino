#include <WiFi.h>
#include <esp_now.h>

// UART pins (ESP32 → Nano)
#define UART_TX_PIN 17
#define UART_RX_PIN 16

#define START_BYTE 0xAA

/* ================= TX → RX CONTROL ================= */

typedef struct __attribute__((packed)) {
  uint16_t ch1;
  uint16_t ch2;
  uint16_t ch3;
  uint16_t ch4;
  uint8_t  btn1;
  uint8_t  btn2;
  uint8_t  throttle;
} espnow_message_t;

/* ================= RX → TX TELEMETRY ================= */

typedef struct __attribute__((packed)) {
  int16_t  altitude_10m;
  uint16_t speed_kmh;
} telemetry_message_t;

telemetry_message_t telemetry;

HardwareSerial nanoSerial(2);

/* ================= PEER ================= */

uint8_t txMAC[6];
bool txPeerAdded = false;

/* ================= ESP-NOW RX ================= */

void onDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(espnow_message_t)) return;

  espnow_message_t msg;
  memcpy(&msg, incomingData, sizeof(msg));

  /* ---- Forward control to Nano ---- */
  nanoSerial.write(START_BYTE);
  nanoSerial.write((uint8_t *)&msg, sizeof(msg));

  /* ---- Add TX as peer (ONCE) ---- */
  if (!txPeerAdded) {
    memcpy(txMAC, info->src_addr, 6);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, txMAC, 6);
    peer.channel = 0;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) == ESP_OK) {
      txPeerAdded = true;
      Serial.println("TX peer added");
    } else {
      Serial.println("Failed to add TX peer");
      return;
    }
  }

  /* ---- Send telemetry back ---- */
  esp_now_send(txMAC,
               (uint8_t *)&telemetry,
               sizeof(telemetry));
}

/* ================= SETUP ================= */

void setup() {
  Serial.begin(115200);
  nanoSerial.begin(57600, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("RX ready (bi-directional)");
}

/* ================= LOOP ================= */

void loop() {
  /* ---- Simulated telemetry ---- */
  static uint16_t t = 0;
  t++;

  telemetry.altitude_10m = 120 + (t % 20);  // 1200–1390 m
  telemetry.speed_kmh   = 60 + (t % 25);   // 60–84 km/h

  delay(150);
}
