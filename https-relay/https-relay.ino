// ESP_0A22F2


#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

#define LED_BUILD_IN 2

const int SWITCH_ID = 1;

//const char *ssid = "AndroidAP";
//const char *password = "rdmo6545";
const char *ssid = "ZM1";
const char *password = "42147718";

const String HOST = "zetaemmesoft.com";
const int PORT = 443;

const int ITERATION_DELAY = 1000; // ms
const int MAX_ITERATION_NUMBER = 2;
const unsigned int SLEEP_TIME = 6e7; // 1 min

const char fingerprint[] PROGMEM = "25 E2 E5 DB 02 BD BF AE A5 25 C7 5B 76 79 1D 5A FB FB BA 99";

int iterationIndex = 0;
String token = "";
bool isTokenValid = false;
int msgQueue = 1;
int connectionTry = 10;
boolean error = false;

void setup() {

  pinMode(LED_BUILD_IN, OUTPUT);

  Serial.begin(115200);

  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
}

void loop() {

  Serial.println("..");

  if (WiFi.status() != WL_CONNECTED) {

    WiFi.begin(ssid, password);

    int r = 0;
    while (WiFi.status() != WL_CONNECTED && r < connectionTry) {
      delay(1000);
      r++;
    }

    if (r == connectionTry) {
      // WIFI --> GIALLO ON !!!!!!!!!!!!
      // WIFI --> ROSSO OFF !!!!!!!!!!!!
      Serial.println("WIFI error!");
    } else {
      // WIFI --> GIALLO OFF !!!!!!!!!!!!
      // WIFI --> ROSSO OFF !!!!!!!!!!!!
      Serial.println("WIFI connected!");
    }
  }

  if (WiFi.status() == WL_CONNECTED) {

    error = false;

    if (error == true) {
      // XXXXX --> GIALLO ON !!!!!!!!!!!!
      // XXXXX --> ROSSO ON !!!!!!!!!!!!
      Serial.println("XXXXX error!");
    }

    if (error == false) {
      // XXXXX --> GIALLO OFF !!!!!!!!!!!!
      // XXXXX --> ROSSO OFF !!!!!!!!!!!!
      Serial.println("XXXXX read ok!");

      digitalWrite(LED_BUILD_IN, LOW);

      WiFiClientSecure httpsClient;
      httpsClient.setFingerprint(fingerprint);
      httpsClient.setTimeout(15000);

      int r = 0;
      while ((!httpsClient.connect(HOST, PORT)) && (r < connectionTry)) {
        delay(1000);
        r++;
      }

      if (r == connectionTry) {
        // HTTP --> GIALLO OFF !!!!!!!!!!!!
        // HTTP --> ROSSO ON !!!!!!!!!!!!
        Serial.println("HTTPS error!");
      } else {
        // HTTP --> GIALLO OFF !!!!!!!!!!!!
        // HTTP --> ROSSO ON !!!!!!!!!!!!
        Serial.println("HTTPS connected!");

        if (isTokenValid) {
          if (msgQueue == 1) {
            httpsClient.print(makeGetSwitchMessage(SWITCH_ID));
            msgQueue = 1;
          }
        } else  {
          httpsClient.print(makeOAuthMessage());
        }

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
          // JSON --> GIALLO ON !!!!!!!!!!!!
          // JSON --> ROSSO ON !!!!!!!!!!!!
          Serial.println("JSON error!");
          Serial.println(jsonError.c_str());
        } else {
          // JSON --> GIALLO OFF !!!!!!!!!!!!
          // JSON --> ROSSO OFF !!!!!!!!!!!!
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

          String value = doc["value"];
          Serial.println("value: " + value);
        }

        digitalWrite(LED_BUILD_IN, HIGH);
      }
    }
  }

  iterationIndex = iterationIndex + 1;

  if (iterationIndex == MAX_ITERATION_NUMBER) {
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


String makeGetSwitchMessage(int id) {

  String message = String("GET ") + "/monitor/rest/switch/" + String(id) + "/get" + " HTTP/1.1\r\n" +
                   "Host: " + HOST + "\r\n" +
                   "Content-Type: application/json" + "\r\n" +
                   "Authorization: Bearer " + token + "\r\n" +
                   "User-Agent: ESP8266/1.0" + "\r\n" +
                   "Accept: */*" + "\r\n" +
                   "Cache-Control: no-cache" + "\r\n" +
                   "Accept-Encoding: gzip, deflate" + "\r\n" +
                   "Connection: close\r\n\r\n";

  Serial.println(message);

  return message;
}


String makePostSwitchMessage(int id, int value) {

  String payload = "{\"id\":" + String(id) + ",\"value\":" + String(value) + "}";
  unsigned int payloadLength = payload.length();

  String message = String("POST ") + "/monitor/rest/switch/set" + " HTTP/1.1\r\n" +
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
