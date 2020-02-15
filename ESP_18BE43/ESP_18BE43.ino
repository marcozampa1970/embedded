#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define DHTPIN 12 // GPIO12 = D6 nodemcu
#define DHTTYPE DHT22
#define LED_BUILD_IN 2
//#define LED_YELLOW 14
//#define LED_RED 13

// ********************************
// ESP_18BE43
// ********************************
const int TEMPERATURE_SENSOR_ID = 1;
const int HUMIDITY_SENSOR_ID = 2;
const int VOLTAGE_SENSOR_ID = 5;

const char *ssid = "ZM1";
const char *password = "42147718";

const String HOST = "zetaemmesoft.com";
const int PORT = 443;

const int ITERATION_DELAY = 2000; // ms
const int MAX_ITERATION_NUMBER = 15;
const int MAX_SENT_MESSAGES = 9;

const unsigned int SLEEP_TIME = 300e6; // 5 min
const short CONNECTION_TRY = 5;

const char fingerprint[] PROGMEM = "25 E2 E5 DB 02 BD BF AE A5 25 C7 5B 76 79 1D 5A FB FB BA 99";

int sentMessagesIndex = 0;
int iterationNumberIndex = 0;
String token = "";
bool dhtError = false;
bool isTokenValid = false;
int msgQueue = 1;

DHT dht(DHTPIN, DHTTYPE);

void setup() {

//  pinMode(LED_YELLOW, OUTPUT);
//  digitalWrite(LED_YELLOW, LOW);

//  pinMode(LED_RED, OUTPUT);
//  digitalWrite(LED_RED, LOW);

  pinMode(LED_BUILD_IN, OUTPUT);
  digitalWrite(LED_BUILD_IN, HIGH);

  Serial.begin(115200);

  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);

  dht.begin();
}

void loop() {

  int voltage = analogRead(A0);
  Serial.println("read: " + String(voltage));

  // http://www.pcbooster.altervista.org/?artid=232
  // VIN(max) = 6
  // VOUT(max) = 1 (In-->ADC)
  // VOUT / VIN = R2 / (R1 + R2) = 1/6 = K
  // R1 = 470 kohm 
  // R2 = 96 kohm = 47 kohm + 47 kohm
  // VIN = 6 * VOUT
  
  float vin = 6.0 * (voltage / 1024.0);
  Serial.println("vin: " + String(vin, 2));

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("WIFI not connected!");

//    digitalWrite(LED_YELLOW, HIGH);

    WiFi.begin(ssid, password);

    int r = 0;
    while (WiFi.status() != WL_CONNECTED && r <= CONNECTION_TRY) {
      delay(500);
      r++;
    }

//    digitalWrite(LED_YELLOW, LOW);
  }

  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("WIFI connected!");

    WiFiClientSecure httpsClient;
    httpsClient.setFingerprint(fingerprint);
    httpsClient.setTimeout(10000);

//    digitalWrite(LED_RED, HIGH);

    if (httpsClient.connect(HOST, PORT)) {

//      digitalWrite(LED_RED, LOW);

      Serial.println("HTTPS connected!");

      // *** sensori

      float humidity = dht.readHumidity();
      float temperature = dht.readTemperature();

      if (isnan(humidity) || isnan(temperature)) {
        dhtError = true;
        Serial.println("DHT22 error!");
      } else {
        dhtError = false;
        Serial.println("DHT22 read ok!");
        float hic = dht.computeHeatIndex(temperature, humidity, false);
      }

      // *********

      digitalWrite(LED_BUILD_IN, LOW);

      if (isTokenValid) {
        if (msgQueue == 1) {
          if (!dhtError) {
            httpsClient.print(makeSensorMessage(HUMIDITY_SENSOR_ID, humidity, "Humidity"));
            sentMessagesIndex++;
          }
          msgQueue = 2;
        } else if (msgQueue == 2) {
          if (!dhtError) {
            httpsClient.print(makeSensorMessage(TEMPERATURE_SENSOR_ID, temperature, "Temperature"));
            sentMessagesIndex++;            
          }
          msgQueue = 3;
        } else if (msgQueue == 3) {
          httpsClient.print(makeSensorMessage(VOLTAGE_SENSOR_ID, vin, "Voltage"));
          sentMessagesIndex++;          
          msgQueue = 1;
        }
      } else  {
        httpsClient.print(makeOAuthMessage());
      }

      digitalWrite(LED_BUILD_IN, HIGH);

      while (httpsClient.connected()) {
        String line = httpsClient.readStringUntil('\n');
        if (line == "\r") {
          break;
        }
      }

      String response;
      String line;
      while (httpsClient.available()) {
        line = httpsClient.readStringUntil('\n');
        if (line.startsWith("{")) {
          response = line;
        }
      }

      StaticJsonDocument<600> doc;
      DeserializationError jsonError = deserializeJson(doc, response);

      if (jsonError) {
        Serial.println("JSON error!");
        Serial.println(jsonError.c_str());
      } else {
        Serial.println("JSON read ok!");

        String responseError = doc["error"];
        String accessToken = doc["access_token"];

        if (responseError.equals(String("invalid_token"))) {
          isTokenValid = false;
        }

        if (accessToken.length() == 345) {
          isTokenValid = true;
          token = accessToken;
        }
      }
    } // http
  } // wifi

  if (iterationNumberIndex++ >= MAX_ITERATION_NUMBER || sentMessagesIndex >= MAX_SENT_MESSAGES) {
    Serial.println("SLEEP!");
    ESP.deepSleep(SLEEP_TIME);
  }

  delay(ITERATION_DELAY);
}


// ********
String makeOAuthMessage() {

  String payload = "username=admin&password=zaqwsx&grant_type=password";
  unsigned int payloadLength = payload.length();

  String message = String("POST ") + "/auth/oauth/token" + " HTTP/1.1\r\n" +
                   "Host: " + HOST + "\r\n" +
                   "Content-Type: application/x-www-form-urlencoded" + "\r\n" +
                   "Authorization: Bearer " + token + "\r\n" +
                   "User-Agent: ESP8266/1.0" + "\r\n" +
                   "Accept: */*" + "\r\n" +
                   "Cache-Control: no-cache" + "\r\n" +
                   "Accept-Encoding: gzip, deflate" + "\r\n" +
                   "Content-Length: " + String(payloadLength) + "\r\n" +
                   "Connection: close\r\n\r\n" +
                   payload + "\r\n";

  return message;
}

// ********  
String makeSensorMessage(int id, float value, String type) {

  String payload = "{\"id\":" + String(id) + ",\"value\":" + String(value, 2) + ",\"type\":\"" + type + "\"}";
  unsigned int payloadLength = payload.length();

  String message = String("POST ") + "/monitor/rest/sensor/set" + " HTTP/1.1\r\n" +
                   "Host: " + HOST + "\r\n" +
                   "Content-Type: application/json" + "\r\n" +
                   "Authorization: Bearer " + token + "\r\n" +
                   "User-Agent: ESP8266/1.0" + "\r\n" +
                   "Accept: */*" + "\r\n" +
                   "Cache-Control: no-cache" + "\r\n" +
                   "Accept-Encoding: gzip, deflate" + "\r\n" +
                   "Content-Length: " + String(payloadLength) + "\r\n" +
                   "Connection: close\r\n\r\n" +
                   payload + "\r\n";

  return message;
}
