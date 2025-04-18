#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define RST_PIN 4
#define SS_PIN 5

const char* ssid = "Mike";
const char* password = "lucky009";
const char* ubidotsToken = "BBUS-gsCDjpaE6w6x2wpWCCUgK39w4Xk2sU";
const char* deviceLabel = "smart_shopping_cart";
const char* variableId = "67fa7f50fd88d23c76ca2061";

MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 20, 4); // 0x27 is the default I2C address, change if needed

void setup() {
  Serial.begin(115200);
  while (!Serial);

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  lcd.setCursor(0, 1);
  lcd.print("WiFi Connected");

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID reader ready");
  lcd.setCursor(0, 2);
  lcd.print("RFID Ready");
  delay(2000);
  lcd.clear();
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;

  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toLowerCase();
  Serial.println("Scanned UID: " + uid);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UID:");
  lcd.setCursor(5, 0);
  lcd.print(uid);

  long mappedId = mapUidToNumber(uid);
  if (mappedId != 0) {
    lcd.setCursor(0, 1);
    lcd.print("Mapped ID:");
    lcd.setCursor(11, 1);
    lcd.print(mappedId);

    sendToUbidots(mappedId);

    lcd.setCursor(0, 2);
    lcd.print("Sent to Ubidots");
  } else {
    lcd.setCursor(0, 1);
    lcd.print("Unknown UID");
  }

  mfrc522.PICC_HaltA();
  delay(2000);
}

long mapUidToNumber(String uid) {
  if (uid == "1364c43") return 13640403;
  if (uid == "d1a72843") return 21872843;
  if (uid == "416e3d43") return 41634343;
  if (uid == "d1f53543") return 21553543;
  if (uid == "b2993f2") return 29939002;
  return 0;
}

void sendToUbidots(long value) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected");
    lcd.setCursor(0, 3);
    lcd.print("WiFi Disconnected");
    return;
  }

  HTTPClient http;
  String url = "https://industrial.api.ubidots.com/api/v1.6/variables/" + String(variableId) + "/values";
  String payload = "{\"value\":" + String(value) + "}";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("X-Auth-Token", ubidotsToken);

  Serial.println("Sending to Ubidots:");
  Serial.println("URL: " + url);
  Serial.println("Payload: " + payload);

  int httpCode = http.POST(payload);

  if (httpCode > 0) {
    Serial.printf("HTTP Response: %d\n", httpCode);
    if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.println("Error response: " + http.getString());
    }
  } else {
    Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
    lcd.setCursor(0, 3);
    lcd.print("Ubidots Error");
  }

  http.end();
}
