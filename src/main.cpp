#include <Adafruit_Sensor.h>
#include <creds.h>

#include "DHT.h"

#define DHTPIN 33      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22  // DHT 22  (AM2302), AM2321

DHT dht(DHTPIN, DHTTYPE);

#define TEMPSTATSD_SERVER "http://192.168.0.50:3030"
#define STATSD_TEMPERATURE_ROUTE "temperature"
#define STATSD_LOG_ROUTE "log"
#define JSON_BUFFER_SIZE 256

#define SLEEP_MINS 15
#define MICROSECONDS_TO_MINS 60000000
#define DEEPSLEEP false

#define SENSOR "sensortest"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
WiFiManager wifiManager;

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_MODE_STA);
  Serial.println(F("DHTxx test!"));
  // bool res;
  // res = wifiManager.autoConnect("Tempsensornet", "justinsucks");
  // if (!res) {
  //   Serial.println("not connected to wifi :(");
  // } else {
  //   Serial.println(F("Connected to wifi"));
  // }
  WiFi.begin("The Network", "justinsucks");
  dht.begin();
  // HTTPClient http;
  // http.begin("http://192.0.0.1");
  // int code = http.GET();
  // if (code > 0) {
  //   Serial.println(http.getString());
  // } else {
  //   Serial.println(http.errorToString(code));
  // }
}

void postJSON(const char *json, const char *route) {
  HTTPClient http;
  char url[50];
  sprintf(url, "%s/%s", TEMPSTATSD_SERVER, route);
  http.begin(url);
  int code = http.POST(json);
  if (code > 0) {
    Serial.println(http.getString());
  } else {
    Serial.println(http.errorToString(code));
  }
}

char *buildStatsJSON(float temperature, float humidity, const char *sensor) {
  // DynamicJsonDocument doc(512);
  StaticJsonDocument<JSON_BUFFER_SIZE> doc;
  doc["temp"] = temperature;
  doc["humidity"] = humidity;
  doc["sensor"] = sensor;
  char *output = (char *)malloc(JSON_BUFFER_SIZE);
  serializeJson(doc, output, JSON_BUFFER_SIZE);
  return output;
}

char *buildLogJSON(const char *message, const char *sensor) {
  StaticJsonDocument<JSON_BUFFER_SIZE> doc;
  doc["message"] = message;
  doc["sensor"] = sensor;
  char *output = (char *)malloc(JSON_BUFFER_SIZE);
  serializeJson(doc, output, JSON_BUFFER_SIZE);
  return output;
}

void postStats(float temperature, float humidity, const char *sensor) {
  char *result = buildStatsJSON(temperature, humidity, sensor);
  postJSON(result, STATSD_TEMPERATURE_ROUTE);
  free(result);
}

void postLog(const char *message, const char *sensor) {
  char *result = buildLogJSON(message, sensor);
  postJSON(result, STATSD_LOG_ROUTE);
  free(result);
}

void loop() {
  // Wait a few seconds between measurements.
  delay(500);
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow
  // sensor)
  float h = dht.readHumidity();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(f)) {
    postLog("Failed to read from DHT sensor", SENSOR);
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  postStats(f, h, SENSOR);

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(f);
  Serial.print(F("°F  Heat index: "));
  Serial.print(hif);
  Serial.println(F("°F"));

  if (DEEPSLEEP) {
    esp_sleep_enable_timer_wakeup(SLEEP_MINS * MICROSECONDS_TO_MINS);
    esp_deep_sleep_start();
  } else {
    delay(2000);
  }
}