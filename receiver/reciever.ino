#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

#define ESC_PIN_1 18
#define ESC_PIN_2 5

#define ESC_MIN_US 1000
#define ESC_MAX_US 2000
#define DEADZONE 100

Servo esc1;
Servo esc2;

int current_pwm = ESC_MIN_US;

// Must match the transmitter's struct exactly
typedef struct {
  // uint32_t counter;
  // int voltage;
  uint16_t ch1; //right joystick y-axis - D32 - A
  uint16_t ch2; //right joystick x-axis - D33 - E
  uint16_t ch3; //left joystick x-axis - D34 - T
  uint16_t ch4; //left joystick y-axis - D35 - R
  uint8_t btn1; //D12 - right joystick switch
  uint8_t btn2; //D13 - left joystick switch
  // bool state;
} espnow_message_t;

espnow_message_t rxData;

// Receive callback (ESP32 core 3.x)
void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&rxData, incomingData, sizeof(rxData));

  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", info->src_addr[0], info->src_addr[1], info->src_addr[2],info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  Serial.print("From: ");
  Serial.print(macStr);
  Serial.print(" | CH-1: ");
  Serial.print(rxData.ch1);
  Serial.print(" V | CH-2: ");
  Serial.print(rxData.ch2);
  Serial.print(" V | CH-3: ");
  Serial.print(rxData.ch3);
  Serial.print(" V | CH-4: ");
  Serial.print(rxData.ch4);
  Serial.print(" V | Button-1: ");
  Serial.println(rxData.btn1 ? "ON" : "OFF");
  Serial.print(" | Button-2: ");
  Serial.println(rxData.btn2 ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);
  esc1.attach(ESC_PIN_1, ESC_MIN_US, ESC_MAX_US);
  esc2.attach(ESC_PIN_2, ESC_MIN_US, ESC_MAX_US);

  // Arm esc
  esc1.writeMicroseconds(ESC_MIN_US);
  // delay(3000);
  esc2.writeMicroseconds(ESC_MIN_US);
  delay(3000);
  // Serial.prinln("ESC armed");

  pinMode(19, OUTPUT);
  digitalWrite(19, LOW);

  // ESP-NOW requires station mode
  WiFi.mode(WIFI_STA);
  Serial.print("Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  // Register receive callback
  esp_now_register_recv_cb(onDataRecv);

  Serial.println("ESP-NOW receiver ready");
}

void loop() {
  int targetPWM = map(rxData.ch3, 0, 4095, ESC_MIN_US, ESC_MAX_US);
  targetPWM = constrain(targetPWM, ESC_MIN_US, ESC_MAX_US);

  // Smooth ramp
  // if (current_pwm < targetPWM) current_pwm++;
  // else if (current_pwm > targetPWM) current_pwm--;
  esc1.writeMicroseconds(targetPWM);
  esc2.writeMicroseconds(targetPWM);
  if (rxData.btn1==1) digitalWrite(19, HIGH);
  else digitalWrite(19, LOW);
  // delay(5);
}