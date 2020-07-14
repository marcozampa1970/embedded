#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define DHTPIN 12 // GPIO12 = D6 nodemcu
#define DHTTYPE DHT22
#define LED_BUILD_IN 2

//const boolean DEBUG = true;
const boolean DEBUG = false;

// ********************************
// ESP_18BE43
// ********************************
const int TEMPERATURE_SENSOR_ID = 1;
const int HUMIDITY_SENSOR_ID = 2;
const int VOLTAGE_SENSOR_ID = 5;

const char *ssid = "ZM1";
const char *password = "42147718";

const String HOST = "192.168.1.101";
const int PORT = 443;
const char fingerprint[] PROGMEM = "25 E2 E5 DB 02 BD BF AE A5 25 C7 5B 76 79 1D 5A FB FB BA 99";

const int ITERATION_DELAY = 2000; // (ms) --> do not modify!!
const int MAX_ITERATION_NUMBER = 9;  // from 0 to 9 --> 10 iteration
const int MAX_SENSOR_SENT = 1;

const unsigned int SLEEP_TIME = 600e6; // 10 min

const short WIFI_CONNECTION_TRY = 10;
const int WIFI_CONNECTION_TRY_DELAY = 500;
const int HTTP_TIMEOUT = 10000;

String oauthToken = "";

String httpResponse;

int sentHIndex = 0;
int sentTIndex = 0;
int sentVIndex = 0;
int iterationNumberIndex = 0;

bool sensorError = false;

DHT dht(DHTPIN, DHTTYPE);

void setup() {

  pinMode(LED_BUILD_IN, OUTPUT);
  digitalWrite(LED_BUILD_IN, HIGH);

  Serial.begin(115200);

  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);

  dht.begin();
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

    int rx = 0;
    while (WiFi.status() != WL_CONNECTED && rx++ <  WIFI_CONNECTION_TRY) {
      delay(WIFI_CONNECTION_TRY_DELAY);
    }

    if (rx >= WIFI_CONNECTION_TRY) {
      iterationNumberIndex = MAX_ITERATION_NUMBER;
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
      // *** sensori begin

      float humidity = dht.readHumidity();
      float temperature = dht.readTemperature();

      if (isnan(humidity) || isnan(temperature)) {
        sensorError = true;
        if (DEBUG) {
          Serial.println("Sensor error!");
        }
      } else {
        sensorError = false;
        float hic = dht.computeHeatIndex(temperature, humidity, false);
      }

      // *** sensori end
      // ***************

      if (sensorError == false) {

        digitalWrite(LED_BUILD_IN, LOW);

        if (!oauthToken.equals("")) {
          if (sentHIndex++ < MAX_SENSOR_SENT) {
            httpsClient.print(makeSensorMessage(HUMIDITY_SENSOR_ID, humidity, "Humidity", oauthToken));
            httpResponse = manageHttpResponse(httpsClient);
          }
          else if (sentTIndex++ < MAX_SENSOR_SENT) {
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
      }
    } else {
      if (DEBUG) {
        Serial.println("HTTPS not connected!");
      }
    }
  }

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
