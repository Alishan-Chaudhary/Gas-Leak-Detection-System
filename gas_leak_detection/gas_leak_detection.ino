/*
  IoT-Based Gas Leak Detection System
  ------------------------------------
  Board: ESP32
  Sensors: MQ-6 (gas), DHT11/DHT22 (temperature & humidity)
  Actuators: Relay module + DC motor (exhaust fan)
  Cloud: Blynk IoT
  Alerting: Twilio WhatsApp API

  IMPORTANT: Replace every placeholder value below (marked YOUR_...) with your
  own credentials before uploading. Never commit real tokens, passwords, or
  phone numbers to a public repository — consider moving these into a
  separate secrets.h file that is excluded via .gitignore.
*/

// ---- Blynk template credentials ----
#define BLYNK_TEMPLATE_ID   "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <HTTPClient.h>
#include <BlynkSimpleEsp32.h>
#include "DHT.h"

// ---- Wi-Fi credentials ----
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// ---- Pin setup ----
#define MQ6_PIN     34
#define DHT_PIN     14
#define DHT_TYPE    DHT22   // change to DHT11 if that's the sensor you're using
#define RELAY_FAN   27
#define LED_PIN     2

DHT dht(DHT_PIN, DHT_TYPE);
BlynkTimer timer;

// Gas concentration threshold (ADC units) — tune to your environment/sensor
int threshold = 1500;

// ---- Twilio credentials ----
String account_sid = "YOUR_TWILIO_ACCOUNT_SID";
String auth_token   = "YOUR_TWILIO_AUTH_TOKEN";

// ---- WhatsApp numbers (E.164 format, no "+") ----
String from = "whatsapp:14155238886";      // Twilio sandbox number
String to   = "whatsapp:YOUR_PHONE_NUMBER"; // your verified WhatsApp number

void readSensors() {
  // Read MQ-6 gas sensor
  int gasValue = analogRead(MQ6_PIN);

  // Read DHT temperature & humidity
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT Error!");
    return;
  }

  // Decision logic
  if (gasValue > threshold) {
    digitalWrite(RELAY_FAN, HIGH); // Turn ON fan
    digitalWrite(LED_PIN, LOW);    // Alert LED off
    Serial.println("GAS DETECTED!");
    sendWhatsAppMessage();
  } else {
    digitalWrite(RELAY_FAN, LOW);  // Fan OFF
    digitalWrite(LED_PIN, HIGH);   // LED on = safe
    Serial.println("Safe");
  }

  // Push readings to Blynk
  Blynk.virtualWrite(V2, gasValue);
  Blynk.virtualWrite(V0, temperature);
  Blynk.virtualWrite(V1, humidity);

  // Debug output
  Serial.print("Gas: ");      Serial.println(gasValue);
  Serial.print("Temp: ");     Serial.println(temperature);
  Serial.print("Humidity: "); Serial.println(humidity);
  Serial.println("----------------------");
}

void sendWhatsAppMessage() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.twilio.com/2010-04-01/Accounts/" + account_sid + "/Messages.json";
    http.begin(url);
    http.setAuthorization(account_sid.c_str(), auth_token.c_str());
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String message = "Body=Alert%20gas%20is%20leaking&From=" + from + "&To=" + to;

    int httpResponseCode = http.POST(message);
    Serial.print("HTTP Response: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println(response);
    http.end();
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  // Default safe state
  digitalWrite(RELAY_FAN, LOW);
  digitalWrite(LED_PIN, HIGH);

  dht.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  Serial.println("System Started...");

  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }
  Serial.println("Connected!");
}

void loop() {
  readSensors();
  delay(2000);
}
