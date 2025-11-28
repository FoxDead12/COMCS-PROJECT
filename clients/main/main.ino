#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "DHT.h"
#include "time.h"

#define WIFI_SSID     "labs"
#define WIFI_PASSWORD "782edcwq#"

#define UDP_ADDRESS "192.168.1.7"
#define UDP_PORT    3001

#define MQTT_SERVER   "d36b6378e3fc4b0ca6725112a64d3d59.s1.eu.hivemq.cloud"
#define MQTT_USERNAME "comcs2425cagg11"
#define MQTT_PASSWORD "COMCSpassword1."
#define MQTT_PORT      8883

#define DHTTYPE DHT11

// ... methods defined ...
void thread_reader(void *pvParameters);
void trhead_sender();
void generate_payload();
void send_udp_message(uint8_t *buffer, int len);

// .. declaration of global variavels ...
DHT dht(21, DHTTYPE);
QueueHandle_t queue;
WiFiUDP udp;

// ... struct of sensor data ...
struct sensor_s {
  float temperature;
  float relativeHumidity;
  char dateObeserved[20];
};

// ... method run on start ...
void setup() {

  Serial.begin(9000);
  delay(500);

  // ... init wifi connection ...
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }

  // ... init sensor ...
  dht.begin();

  // ... create queue of data shared between threads ...
  queue = xQueueCreate(10, sizeof(struct sensor_s));

  // ... create thread ...
  xTaskCreatePinnedToCore(
    thread_reader, "thread_reader", 4096, NULL, 2, NULL, NULL   // core 0
  );

}

void loop() {
  trhead_sender();
}

// ... thread method will run reader ...
void thread_reader (void *pvParameters) {

  // Serial.print("Task 'thread_reader' running in core ");
  // Serial.println(xPortGetCoreID());

  // ... start time of code running and set 1 seccond for sleep ...
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(1000);

  struct sensor_s data;

  // ... make periodic task ...
  while (true) {

    // ... get data of sensor ...
    data.temperature      = dht.readTemperature();
    data.relativeHumidity = dht.readHumidity();

    // ... get time of reading ...
    struct tm timeinfo;
    getLocalTime(&timeinfo);
    strftime(data.dateObeserved, sizeof(data.dateObeserved), "%Y-%m-%dT%H:%M:%S", &timeinfo);

    // ... store data in queue ...
    xQueueSend(queue, &data, portMAX_DELAY);
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

  }

}

// ... thread method will run sender ...
void trhead_sender () {
  struct sensor_s data;

  if ( xQueueReceive(queue, &data, portMAX_DELAY) == pdPASS ) {

    // ... generate message to send ...
    String output;
    generate_payload(&data, &output);

    Serial.print("Recebi dados: ");
    Serial.println(output);

    // ... send message to udp ...
    send_udp_message(&output, output.length());

  }
}

// ... create json string to send to servers ...
void generate_payload ( struct sensor_s *data, String *output ) {
  JsonDocument payload;

  payload["id"] = WiFi.macAddress();
  payload["type"] = "wheatherObservation";
  payload["location"] = "Porto";
  payload["temperature"] = data->temperature;
  payload["relativeHumidity"] = data->relativeHumidity;
  payload["dateObeserved"] = data->dateObeserved;

  serializeJson(payload, *output);

}

// ... method to send data to udp server ...
void send_udp_message ( String* buffer, int len ) {
  udp.beginPacket(UDP_ADDRESS, UDP_PORT);
  udp.write((const uint8_t*)buffer->c_str(), len);
  udp.endPacket();
  // TODO: check if dont return any error try sending message
}

void send_mqtt_message () {

}




// #include <WiFi.h>
// #include <WiFiClientSecure.h>
// #include <PubSubClient.h>
// #include <ArduinoJson.h>
// #include "DHT.h"
// #include "time.h"
//
//
// // ... wifi connections props ...
// #define WIFI_SSID     "labs"
// #define WIFI_PASSWORD "782edcwq#"
//
// // ... mqtt server connection props ...
// #define MQTT_SERVER   "d36b6378e3fc4b0ca6725112a64d3d59.s1.eu.hivemq.cloud"
// #define MQTT_USERNAME "comcs2425cagg11"
// #define MQTT_PASSWORD "COMCSpassword1."
// #define MQTT_PORT      8883
//
// // ... define the type of sensor using ...
// #define DHTTYPE DHT11
//
//
// DHT dht(21, DHTTYPE);
// WiFiClientSecure esp_client;
// PubSubClient mqtt_client(esp_client);
//
//
//
// void setup() {
//   // put your setup code here, to run once:
//   delay(2000);
//   Serial.begin(115200);
//
//   // ... create thread to communciate with the world ...
//
//
//   //
//   //
//   //   // ... connect to WIFI ...
//   //   WiFi.mode(WIFI_STA);
//   //   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//   //
//   //   while ( WiFi.status() != WL_CONNECTED ) {
//   //     delay(500);
//   //     Serial.print(".");
//   //   }
//   //
//   //   // ... don´t know what this make, (this is for ESP32 can generate random numbers in feature) ...
//   //   // randomSeed(micros());
//   //
//   //   Serial.println("\nWiFi connected\nIP address: ");
//   //   Serial.println(WiFi.localIP());
//   //
//   //   while (!Serial) delay(1);
//   //
//   //   // ... init connection to MQTT server ...
//   //   esp_client.setInsecure();
//   //   mqtt_client.setServer(MQTT_SERVER, MQTT_PORT);
//   //   mqtt_client.setCallback(callback);
//   //
//   //   // ... init sensor ...
//   //   dht.begin();
// }
//
// void loop() {
//     // o loop corre no Core 1 por defeito
//   Serial.println("Loop Arduino (Core 1)");
//   delay(1000);
//
// //
// //   // ... prepar payload to send ...
// //   JsonDocument payload;
// //   // payload["id"] = "CLIENT_1_PORTO";
// //   payload["type"] = "wheatherObservation";
// //   payload["location"] = "Porto";
// //   payload["temperature"] = dht.readTemperature();
// //   payload["relativeHumidity"] = dht.readHumidity();
// //
// //   // ... get current time of message ...
// //   struct tm timeinfo;
// //   getLocalTime(&timeinfo);
// //   char data_iso[25];
// //   strftime(data_iso, sizeof(data_iso), "%Y-%m-%dT%H:%M:%S", &timeinfo);
// //   payload["dateObeserved"] = data_iso;
// //   payload["id"] = String(payload["location"]) + "-" + String(payload["type"]) + "-" + String(data_iso);
// //
// //   // ... send data ...
// //   String output;
// //   serializeJson(payload, output);
// //   publishMessage("/comcs/g11/temperature", output, true);
// //
// //
// //   if ( !mqtt_client.connected() ) {
// //     reconnect();
// //   }
// //   mqtt_client.loop();
//
//   // Serial.println("LUL");
//
//   // ... SEND DATA TO SERVER , IF SOME ERROR STORE INFORMATION IN FILE ...
//   // delay(1000);
// }
//
//
//
// //
// //
// // void reconnect() {
// //   // Loop until we’re reconnected
// //
// //   while ( !mqtt_client.connected() ) {
// //     Serial.print("Attempting MQTT connection…");
// //     String clientId = "ESP32-G99-"; // change the groupID
// //     clientId += String(random(0xffff), HEX);
// //
// //     // Attempt to connect
// //     if (mqtt_client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
// //       Serial.println("connected");
// //       mqtt_client.subscribe("/comcs/g11/temperature");// change topic for your group
// //     } else {
// //       Serial.print("failed, rc=");
// //       Serial.print(mqtt_client.state());
// //       Serial.println(" try again in 5 seconds"); // Wait 5 seconds before retrying
// //       delay(5000);
// //     }
// //   }
// // }
// //
// // void callback (char* topic, byte* payload, unsigned int length) {
// //   Serial.print("Callback - ");
// //   Serial.print("Message:");
// //   for (int i = 0; i < length; i++) {
// //     Serial.print((char)payload[i]);
// //   }
// //   Serial.println("");
// // }
// //
// // void publishMessage (const char* topic, String payload, boolean retained) {
// //   if (mqtt_client.publish(topic, payload.c_str(), true)) {
// //     Serial.println("Message published ["+String(topic)+"]: "+payload);
// //   } else {
// //     Serial.println("Entrei no IF");
// //   }
// // }
