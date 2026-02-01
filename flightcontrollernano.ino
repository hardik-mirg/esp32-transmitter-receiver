#include <SoftwareSerial.h>

#define START_BYTE 0xAA

SoftwareSerial espSerial(2, -1); // RX only (D2)

typedef struct __attribute__((packed)) {
  uint16_t ch1;
  uint16_t ch2;
  uint16_t ch3;
  uint16_t ch4;
  uint8_t  btn1;
  uint8_t  btn2;
} espnow_message_t;

espnow_message_t rxData;

uint8_t buffer[sizeof(espnow_message_t)];
uint8_t index = 0;
bool syncing = true;

void setup() {
  Serial.begin(57600);     // USB debug
  espSerial.begin(57600); // ESP32 → Nano
}

void loop() {
  while (espSerial.available()) {
    uint8_t b = espSerial.read();

    // Wait for frame start
    if (syncing) {
      if (b == START_BYTE) {
        syncing = false;
        index = 0;
      }
      continue;
    }

    buffer[index++] = b;

    // Full packet received
    if (index >= sizeof(espnow_message_t)) {
      memcpy(&rxData, buffer, sizeof(rxData));
      syncing = true;

      // Debug (values now stable)
      Serial.print("CH1: "); Serial.print(rxData.ch1);
      Serial.print(" | CH2: "); Serial.print(rxData.ch2);
      Serial.print(" | CH3: "); Serial.print(rxData.ch3);
      Serial.print(" | CH4: "); Serial.print(rxData.ch4);
      Serial.print(" | BTN1: "); Serial.print(rxData.btn1);
      Serial.print(" | BTN2: "); Serial.println(rxData.btn2);
    }
  }
}
