#include <WiFi.h>
#include <esp_now.h>

#define buzzer 4
#define red 5
#define blue 15
#define green 2

bool connected = false;

/* ================= ESP-NOW ================= */

uint8_t receiverMAC[] = {0xEC, 0xE3, 0x34, 0x19, 0xCE, 0xC4};

/* ================= DISPLAY BUFFER ================= */

int value[4] = {10, 10, 10, 10};

/* ---- TX → RX control packet ---- */
typedef struct __attribute__((packed)) {
  uint16_t ch1;
  uint16_t ch2;
  uint16_t ch3;
  uint16_t ch4;
  uint8_t  btn1;
  uint8_t  btn2;
  uint8_t  throttle;
} struct_message;

/* ---- RX → TX telemetry packet ---- */
typedef struct __attribute__((packed)) {
  int16_t  altitude_10m;
  uint16_t speed_kmh;
} telemetry_message_t;

struct_message outgoingData;
telemetry_message_t telemetry;

void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  if (status != ESP_NOW_SEND_SUCCESS){
    Serial.println("ESP-NOW Send Fail");
    connected = false;
    analogWrite(green, 255);   
  } else {
    analogWrite(red, 255);
    analogWrite(green, 230);
    connected = true;
    value[1] = 10;
    value[2] = 10;
    value[3] = 10;
  }
}

/* ---- TELEMETRY RECEIVE CALLBACK ---- */
void onDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *incomingData,
                int len) {

  if (len != sizeof(telemetry_message_t)) return;

  memcpy(&telemetry, incomingData, sizeof(telemetry));

  // Serial.print("ALT: ");
  // Serial.print(telemetry.altitude_10m * 10);
  // Serial.print(" m   SPEED: ");
  // Serial.print(telemetry.speed_kmh);
  // Serial.println(" km/h");

  value[1] = (telemetry.speed_kmh % 99) / 10;
  value[2] = (telemetry.speed_kmh % 99) - value[1]*10;
  value[3] = telemetry.altitude_10m % 9;
}

/* ================= 7-SEG DISPLAY ================= */

// Segments: A B C D E F G (active LOW, common anode)
int segPins[7]   = {16, 17, 18, 19, 21, 22, 23};
int digitPins[4] = {25, 26, 27, 14};

//      A B C D E F G
byte digits[11][7] = {
  {0,0,0,0,0,0,1},
  {1,0,0,1,1,1,1},
  {0,0,1,0,0,1,0},
  {0,0,0,0,1,1,0},
  {1,0,0,1,1,0,0},
  {0,1,0,0,1,0,0},
  {0,1,0,0,0,0,0},
  {0,0,0,1,1,1,1},
  {0,0,0,0,0,0,0},
  {0,0,0,0,1,0,0},
  {1,1,1,1,1,1,1}
};

void showDigit(int digit, int number) {
  for (int i = 0; i < 7; i++) digitalWrite(segPins[i], HIGH);
  for (int i = 0; i < 4; i++) digitalWrite(digitPins[i], LOW);

  for (int i = 0; i < 7; i++)
    digitalWrite(segPins[i], digits[number][i]);

  digitalWrite(digitPins[digit], HIGH);
  delayMicroseconds(1000);
}

/* ================= THROTTLE CONTROL ================= */

int throttle = 0;

const int JOY_UP   = 2400;
const int JOY_DOWN = 1800;

unsigned long lastThrottleUpdate = 0;
const unsigned long throttleInterval = 225;


/* ================= SETUP ================= */

void setup() {

  pinMode(red, OUTPUT);
  pinMode(blue, OUTPUT);
  pinMode(green, OUTPUT);

  analogWrite(red, 255);
  analogWrite(blue, 255);
  analogWrite(green, 255);

  // ======== START UP TONE =======
  float f = 1000;
  tone(buzzer, 2*f);
  delay(500);
  tone(buzzer, f);
  delay(175);
  tone(buzzer, f*pow(2, 7.0/12));
  delay(400);
  tone(buzzer, f*pow(2, 5.0/12));
  delay(400);
  tone(buzzer, 2*f);
  delay(300);
  tone(buzzer, f*pow(2, 7.0/12));
  delay(800);
  noTone(buzzer);

  analogWrite(red, 200);
  delay(1000);
  analogWrite(red, 255);
  delay(500);
  analogWrite(blue, 200);
  delay(1000);
  analogWrite(blue, 255);
  delay(500);
  analogWrite(green, 200);
  delay(1000);
  analogWrite(green, 255);



  Serial.begin(115200);

  pinMode(12, INPUT_PULLUP);
  pinMode(13, INPUT_PULLUP);

  for (int i = 0; i < 7; i++) {
    pinMode(segPins[i], OUTPUT);
    digitalWrite(segPins[i], HIGH);
  }

  for (int i = 0; i < 4; i++) {
    pinMode(digitPins[i], OUTPUT);
    digitalWrite(digitPins[i], LOW);
  }

  WiFi.mode(WIFI_STA);

  esp_now_init();
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);   // 👈 TELEMETRY RX

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("TX ready (control + telemetry)");
}

/* ================= LOOP ================= */

void loop() {

  /* --- BULB --- */
  if (!connected){
    analogWrite(red, 200);
    delay(250);
    analogWrite(red, 255);
    tone(buzzer, 1000);
    delay(1000);
    noTone(buzzer);

  }

  /* ---- DISPLAY ---- */
  value[0] = throttle;
  showDigit(0, value[0]);
  showDigit(1, value[1]);
  showDigit(2, value[2]);
  showDigit(3, value[3]);

  /* ---- THROTTLE UPDATE ---- */
  int joy3 = analogRead(35);

  if (millis() - lastThrottleUpdate >= throttleInterval) {
    lastThrottleUpdate = millis();

    if (joy3 > JOY_UP && throttle < 9) throttle++;
    else if (joy3 < JOY_DOWN && throttle > 0) throttle--;
  }

  /* ---- ESP-NOW SEND ---- */
  static unsigned long lastSend = 0;
  if (millis() - lastSend >= 20) {
    lastSend = millis();

    outgoingData.ch1 = analogRead(32);
    outgoingData.ch2 = analogRead(33);
    outgoingData.ch3 = joy3;
    outgoingData.ch4 = analogRead(34);
    outgoingData.btn1 = !digitalRead(12);
    outgoingData.btn2 = !digitalRead(13);
    outgoingData.throttle = throttle;

    esp_now_send(receiverMAC,
                 (uint8_t *)&outgoingData,
                 sizeof(outgoingData));
  }
}
