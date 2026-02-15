#include <Arduino.h>
#include <SoftwareSerial.h>

#define START_CTRL 0xAA
#define START_TLM  0x55

// SoftwareSerial pins
#define ESP_RX 4   // Nano RX  (from ESP32 TX)
#define ESP_TX 5   // Nano TX  (to ESP32 RX via divider)

SoftwareSerial espSerial(ESP_RX, ESP_TX);

typedef struct __attribute__((packed)) {
  uint16_t ch1;
  uint16_t ch2;
  uint16_t ch3;
  uint16_t ch4;
  uint8_t  btn1;
  uint8_t  btn2;
  uint8_t  throttle;
} control_t;

typedef struct __attribute__((packed)) {
  int16_t  altitude_10m;
  uint16_t speed_kmh;
} telemetry_t;

control_t control;
telemetry_t telemetry;

uint8_t rxIndex = 0;
uint8_t rxBuf[sizeof(control_t)];

void setup() {
  Serial.begin(115200);      // USB debug
  espSerial.begin(57600);    // ESP32 link

  Serial.println("Nano ready");
}

void loop() {

  /* ===== Receive control from ESP32 ===== */
  while (espSerial.available()) {
    uint8_t b = espSerial.read();

    if (rxIndex == 0 && b != START_CTRL) continue;

    if (rxIndex > 0) rxBuf[rxIndex - 1] = b;
    rxIndex++;

    if (rxIndex == sizeof(control_t) + 1) {
      memcpy(&control, rxBuf, sizeof(control));
      rxIndex = 0;

      // ---- Debug print ----
      Serial.print("CH4: "); Serial.print(control.ch4);
      Serial.print("  THR: "); Serial.println(control.throttle);
    }
  }

  /* ===== Generate telemetry ===== */
  static uint16_t t = 0;
  t++;

  telemetry.altitude_10m = 120 + (t % 20);
  telemetry.speed_kmh   = 60 + (t % 25);

  /* ===== Send telemetry to ESP32 ===== */
  espSerial.write(START_TLM);
  espSerial.write((uint8_t *)&telemetry,
                  sizeof(telemetry));

  delay(150);
}



