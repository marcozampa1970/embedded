#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <lm75.h>
#include <inttypes.h>
#include <Wire.h>
#include <ArduinoJson.h>

#define LED_BUILD_IN 2

#define SDA_PIN 4
#define SCL_PIN 5
const int16_t I2C_SLAVE = 0x48; // 0 1001 000

TempI2C_LM75 termo = TempI2C_LM75(I2C_SLAVE, TempI2C_LM75::nine_bits);

const boolean DEBUG = false;

// ********************************
// ESP_296715
// ********************************
const int TEMPERATURE_SENSOR_ID = 10;
const int VOLTAGE_SENSOR_ID = 11;

const char *ssid = "ZM1";
const char *password = "42147718";

const String HOST = "zetaemmesoft.com";
const int PORT = 443;

const int ITERATION_DELAY = 2000; // (ms) --> non modificare!!
const int MAX_ITERATION_NUMBER = 10;
const int MAX_SENSOR_SENT = 2;

const unsigned int SLEEP_TIME = 600e6; // 10 min --> 600 (s) --> 600.000.000 (microseconds)

const short WIFI_CONNECTION_TRY = 5;
const int WIFI_CONNECTION_TRY_DELAY = 500;
const int HTTP_TIMEOUT = 10000;

const char fingerprint[] PROGMEM = "25 E2 E5 DB 02 BD BF AE A5 25 C7 5B 76 79 1D 5A FB FB BA 99";
String oauthToken = "";

String httpResponse;

int sentHIndex = 0;
int sentTIndex = 0;
int sentVIndex = 0;
int iterationNumberIndex = 0;

bool sensorError = false;

void setup() {

  pinMode(LED_BUILD_IN, OUTPUT);
  digitalWrite(LED_BUILD_IN, HIGH);

  Serial.begin(115200);

  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);

  Wire.begin(SDA_PIN, SCL_PIN);
}

void loop() {

  int voltage = analogRead(A0);

  if (DEBUG) {
    Serial.println("read: " + String(voltage));
  }

  // VIN(max) = 6
  // VOUT(max) = 1 (In-->ADC)
  // VOUT / VIN = R2/(R1+R2) = 1/6
  // R2/(R1+R2)=1/6
  // (6*R2)/(R1+R2)=1
  // 6*R2=R1+R2
  // 5*R2=R1
  // 5*94K=470K
  // R1 = 470K
  // R2 = 94K = (circa) 47K+47K
  // VIN = 6 * VOUT

  float vin = 6.0 * (voltage / 1024.0);

  if (DEBUG) {
    Serial.println("vin: " + String(vin, 2));
  }

  if (WiFi.status() != WL_CONNECTED) {

    if (DEBUG) {
      Serial.println("WIFI not connected!");
    }

    WiFi.begin(ssid, password);

    int r = 0;
    while (WiFi.status() != WL_CONNECTED && r <=  WIFI_CONNECTION_TRY) {
      delay(WIFI_CONNECTION_TRY_DELAY);
      r++;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {

    if (DEBUG) {
      Serial.println("WIFI connected!");
    }

    WiFiClientSecure httpsClient;
    httpsClient.setFingerprint(fingerprint);
    httpsClient.setTimeout(HTTP_TIMEOUT);

    if (httpsClient.connect(HOST, PORT)) {

      if (DEBUG) {
        Serial.println("HTTPS connected!");
      }

      // *****************
      // *** SENSOR BEGIN

      float temperature = termo.getTemp();

      if (isnan(temperature)) {
        sensorError = true;
        if (DEBUG) {
          Serial.println("Sensor error!");
        }
      } else {
        sensorError = false;
        float hic = 0.0;
      }

      // *** SENSOR END
      // ***************

      if (sensorError == false) {

        digitalWrite(LED_BUILD_IN, LOW);

        if (!oauthToken.equals("")) {

          if (sentTIndex++ < MAX_SENSOR_SENT) {
            httpsClient.print(makeSensorMessage(TEMPERATURE_SENSOR_ID, temperature, "Temperature", oauthToken));
            httpResponse = manageHttpResponse(httpsClient);
          }
          else if (sentVIndex++ < MAX_SENSOR_SENT) {
            httpsClient.print(makeSensorMessage(VOLTAGE_SENSOR_ID, vin, "Voltage", oauthToken));
            httpResponse = manageHttpResponse(httpsClient);
          }

        } else  {
          httpsClient.print(makeOAuthMessage());
          httpResponse = manageHttpResponse(httpsClient);
          oauthToken = manageOauthJsonResponse(httpResponse);
        }

        digitalWrite(LED_BUILD_IN, HIGH);

      } // sensor ok
    } // http connected
  } // wifi connected

  if (iterationNumberIndex++ >= MAX_ITERATION_NUMBER || (sentHIndex >= MAX_SENSOR_SENT && sentTIndex >= MAX_SENSOR_SENT && sentVIndex >= MAX_SENSOR_SENT)) {
    if (DEBUG) {
      Serial.println("SLEEP!");
    }
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
String makeSensorMessage(int id, float value, String type, String token) {

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

String manageOauthJsonResponse(String response) {

  String token;

  StaticJsonDocument<600> doc;
  DeserializationError jsonError = deserializeJson(doc, response);

  if (jsonError) {
    if (DEBUG) {
      Serial.println("JSON error!");
      Serial.println(jsonError.c_str());
    }
  } else {
    String responseError = doc["error"];
    String accessToken = doc["access_token"];

    if (responseError.equals(String("invalid_token"))) {
      token = "";
    }

    if (accessToken.length() == 345) {
      token = accessToken;
    }
  }

  return token;
}

String manageHttpResponse(WiFiClientSecure httpsClient) {

  String headerLine;
  String bodyLine;
  String response;

  // http response header
  while (httpsClient.connected()) {

    headerLine = httpsClient.readStringUntil('\n');

    if (DEBUG) {
      Serial.println(headerLine);
    }

    if (headerLine == "\r") {
      break;
    }
  }

  // http response body
  while (httpsClient.available()) {

    bodyLine = httpsClient.readStringUntil('\n');

    if (DEBUG) {
      Serial.println(bodyLine);
    }

    if (bodyLine.startsWith("{")) {
      response = bodyLine;
    }
  }

  return response;
}
