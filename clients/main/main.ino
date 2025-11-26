#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "DHT.h"
#include "time.h"

// ... wifi connections props ...
#define WIFI_SSID     "labs"
#define WIFI_PASSWORD "782edcwq#"

// ... mqtt server connection props ...
#define MQTT_SERVER   "a7b92dbc05a74883bd73c30dc0d4b9af.s1.eu.hivemq.cloud"
#define MQTT_USERNAME "comcs2425cagg11"
#define MQTT_PASSWORD "COMCSpassword1."
#define MQTT_PORT      8883

// ... define the type of sensor using ...
#define DHTTYPE DHT11


DHT dht(21, DHTTYPE);
WiFiClientSecure esp_client;
PubSubClient     mqtt_client(esp_client);


void setup() {
  // put your setup code here, to run once:

  delay(2000);
  Serial.begin(9600);

  Serial.println("init client");

  // ... connect to WIFI ...
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }

  // ... don´t know what this make, (this is for ESP32 can generate random numbers in feature) ...
  // randomSeed(micros());

  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());

  while (!Serial) delay(1);

  // ... init connection to MQTT server ...
  esp_client.setInsecure();
  mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt_client.setCallback(callback);

  // ... init sensor ...
  dht.begin();
}

void loop() {

  // ... prepar payload to send ...
  JsonDocument payload;
  // payload["id"] = "CLIENT_1_PORTO";
  payload["type"] = "wheatherObservation";
  payload["location"] = "Porto";
  payload["temperature"] = dht.readTemperature();
  payload["relativeHumidity"] = dht.readHumidity();

  // ... get current time of message ...
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char data_iso[25];
  strftime(data_iso, sizeof(data_iso), "%Y-%m-%dT%H:%M:%S", &timeinfo);
  payload["dateObeserved"] = data_iso;
  payload["id"] = String(payload["location"]) + "-" + String(payload["type"]) + "-" + String(data_iso);

  // ... send data ...
  String output;
  serializeJson(payload, output);
  publishMessage("/comcs/g11/temperature", output, true);


  if ( !mqtt_client.connected() ) {
    reconnect();
  }
  mqtt_client.loop();

  Serial.println("LUL");

  // ... SEND DATA TO SERVER , IF SOME ERROR STORE INFORMATION IN FILE ...
  delay(1000);
}





void reconnect() {
  // Loop until we’re reconnected

  while ( !mqtt_client.connected() ) {
    Serial.print("Attempting MQTT connection…");
    String clientId = "ESP32-G99-"; // change the groupID
    clientId += String(random(0xffff), HEX);

    // Attempt to connect
    if (mqtt_client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
      Serial.println("connected");
      mqtt_client.subscribe("/comcs/g11/temperature");// change topic for your group
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds"); // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void callback (char* topic, byte* payload, unsigned int length) {
  Serial.print("Callback - ");
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println("");
}

void publishMessage (const char* topic, String payload, boolean retained) {
  if (mqtt_client.publish(topic, payload.c_str(), true)) {
    Serial.println("Message published ["+String(topic)+"]: "+payload);
  } else {
    Serial.println("Entrei no IF");
  }
}
