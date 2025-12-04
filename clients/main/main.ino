#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include "SPIFFS.h"
#include "FS.h"
#include "DHT.h"
#include "time.h"

#define WIFI_SSID     "S24 Ultra de David"
#define WIFI_PASSWORD "ola12345"

#define UDP_ADDRESS "10.72.138.100"
#define UDP_PORT    3001

#define MQTT_SERVER   "d36b6378e3fc4b0ca6725112a64d3d59.s1.eu.hivemq.cloud"
#define MQTT_USERNAME "comcs2425cagg11"
#define MQTT_PASSWORD "COMCSpassword1."
#define MQTT_PORT      8883

#define DHTTYPE DHT11

#define TIME_SERVER "pool.ntp.org"
#define TIME_OFFSET 3600

#define FILE_NAME_ERR "/messages-not-send.txt"
#define FILE_ARRAY_SIZE 10

// ... methods defined ...
void thread_reader(void *pvParameters);
void trhead_sender(void *pvParameters);

void generate_payload ( struct sensor_s *data, String *output );

int udp_send_message(String *buffer, int len);

int mqtt_send_message(String* buffer, struct sensor_s *data);
void mqtt_connect();

void spiffs_write_file(struct sensor_s data);
void spiffs_read_file(struct sensor_s *sensor);
void spiffs_open_file();


// .. declaration of global variavels ...
DHT dht(21, DHTTYPE);

QueueHandle_t queue;

File file;
WiFiUDP udp;
WiFiClientSecure esp_client;
PubSubClient mqtt(esp_client);

int err_write_idx = 0;
int err_read_idx = 0;

static const char* certificate = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// ... struct of sensor data ...
struct sensor_s {
  float temperature;
  float relativeHumidity;
  char dateObeserved[20];
};

// ... method run on start ...
void setup() {

  Serial.begin(19200);
  delay(1000);

  // ... init wifi connection ...
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while ( WiFi.status() != WL_CONNECTED ) {
    delay(500);
    Serial.print(".");
  }

  // ... set setting to MQTT server ...
  esp_client.setCACert(certificate);
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);

  // ... init and get time ...
  configTime(TIME_OFFSET, TIME_OFFSET, TIME_SERVER);

  // ... init sensor ...
  dht.begin();

  // ... create queue of data shared between threads ...
  queue = xQueueCreate(10, sizeof(struct sensor_s));

  // ... init SPIFFS (file server) ...
  SPIFFS.begin(true);

  randomSeed(micros());

  // ... create thread ...
  xTaskCreatePinnedToCore(
    thread_reader, "thread_reader", 2048, NULL, 2, NULL, NULL   // core 0
  );

  xTaskCreatePinnedToCore(
    trhead_sender, "trhead_sender", 4096, NULL, 1, NULL, NULL // core 1
  );

}

void loop() {
}

// ... thread method will run reader ...
void thread_reader (void *pvParameters) {

  // ... start time of code running and set 1 seccond for sleep ...
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(1000);

  struct sensor_s data;

  // ... make periodic task ...
  while (true) {

    Serial.println("A LER");

    // ... get data of sensor ...
    data.temperature      = dht.readTemperature();
    data.relativeHumidity = dht.readHumidity();

    // ... get time of reading ...
    struct tm timeinfo;
    getLocalTime(&timeinfo, 200); // timeout is 200ms
    strftime(data.dateObeserved, sizeof(data.dateObeserved), "%Y-%m-%dT%H:%M:%S", &timeinfo);

    // ... store data in queue ...
    xQueueSend(queue, &data, 0); // don't block if queue is full
    vTaskDelayUntil(&xLastWakeTime, xFrequency);

  }

}

// ... thread method will run sender ...
void trhead_sender (void *pvParameters) {

  struct sensor_s data;

  while (true) {
    if ( xQueueReceive(queue, &data, portMAX_DELAY) == pdPASS ) {

      spiffs_read_file(&data);

      Serial.print("read: ");
      Serial.print(data.temperature);
      Serial.print("");
      Serial.print(data.relativeHumidity);
      Serial.print("");
      Serial.println(data.dateObeserved);

//
//       // ... logic to dont write same message when errors append ...
//       int error = 0;
//
//       // ... generate message to send ...
//       String output;
//       generate_payload(&data, &output);
//
//
//       // ... send message to udp ...
//       if ( udp_send_message(&output, output.length()) == 1 ) {
//         // ... error send udp message ...
//         spiffs_write_file(data);
//         error = 1;
//       }
//
//       // ... send message to mqtt ...
//       if ( mqtt_send_message(&output, &data) == 1 && error == 0) {
//         spiffs_write_file(data);
//       }

    }
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
  payload["dateObserved"] = data->dateObeserved;

  serializeJson(payload, *output);

}

// ... method to send data to udp server ...
int udp_send_message ( String* buffer, int len ) {

  Serial.print("SENDING JSON IN UDP: ");
  Serial.printf("%s", buffer->c_str());
  Serial.println("");

  if ( !udp.beginPacket(UDP_ADDRESS, UDP_PORT) ) {
    Serial.println("[UDP] ERRO: beginPacket falhou");
    return 1;
  }

  int bytes = udp.write((const uint8_t*) buffer->c_str(), len);
  if (bytes != len) {
    Serial.printf("[UDP] ERRO: udp.write escreveu %d bytes, mas esperado %d\n", bytes, len);
    return 1;
  }

  if ( udp.endPacket() == 0 ) {
    Serial.println("[UDP] ERRO: endPacket falhou! Pacote não enviado.");
    return 1;
  }

  return 0;

}

// ... method to send message to mqtt server ...
int mqtt_send_message (String* buffer, struct sensor_s *data) {

  Serial.print("SENDING JSON IN MQTT: ");
  Serial.printf("%s", buffer->c_str());
  Serial.println("");

  if ( !mqtt.connected() ) {
    mqtt_connect();
  }

  if ( !mqtt.publish("/messages", buffer->c_str(), true) ) {
    // ... error sendind message ...
    return 1;
  }

  return 0;

}

// ... method to connect to mqtt server ...
void mqtt_connect () {
  while ( !mqtt.connected() ) {
    // ... use mac address to mqtt ...
    String client_id = WiFi.macAddress();
    client_id += String(random(0xffff), HEX);

    // ... connect to mqtt server ...
    if ( mqtt.connect(client_id.c_str(), MQTT_USERNAME, MQTT_PASSWORD) ) {
      // ... set chanel to mqtt server using QoS 1 ...
      mqtt.subscribe("/messages", 1);
    } else {
      // ... error connection, try again ...
      Serial.print("failed connect to mqtt, rc=");
      Serial.println(mqtt.state());
    }
  }
}

// ... write struct in file ...
void spiffs_write_file (struct sensor_s data) {

  // ... open file ...
  spiffs_open_file();

  // ... write in file in specific position ...
  file.seek(err_write_idx * sizeof(data), SeekSet);
  file.write((uint8_t*) &data, sizeof(data));

  // ... increase value ...
  err_write_idx += 1;

  // ... rolback ...
  if (err_write_idx >= FILE_ARRAY_SIZE) {
    err_write_idx = 0;
  }

  file.close();

}

// ... read struct from file ...
void spiffs_read_file (struct sensor_s *sensor) {

  // ... open file ...
  spiffs_open_file();

  // ... finding struct in file ...
  int finding_struct = 1;
  int idx = err_write_idx + 1;
  int i = 0;
  while ( finding_struct == 1 ) {

    Serial.println("A procura de dados");

    file.seek(idx * sizeof(struct sensor_s), SeekSet);
    file.read((uint8_t*) sensor, sizeof(struct sensor_s));

    struct sensor_s e = {0};
    bool is_empty = (memcmp(sensor, &e, sizeof(struct sensor_s)) == 0);

    if ( is_empty == false ) {
      Serial.println("Estrutura de dados encontrada");
      file.seek(err_read_idx * sizeof(struct sensor_s), SeekSet);
      file.write((uint8_t*)&empty, sizeof(struct sensor_s));
      finding_struct = 0;
      break;
    } else {
      Serial.println("Estrutura de dados FUCKEDDDDD");
    }

    i++;
    idx += 1;

    if (idx >= FILE_ARRAY_SIZE) {
      idx = 0;
    }

    if (i >= FILE_ARRAY_SIZE) {
      break;
    }


  }


  // ... read struct from position ...
  file.seek(err_read_idx * sizeof(struct sensor_s), SeekSet);
  file.read((uint8_t*) sensor, sizeof(struct sensor_s));

  // ... create emepty struct ...
  struct sensor_s empty;
  memset(&empty, 0, sizeof(struct sensor_s));

  // ... reset struct readed ...
  file.seek(err_read_idx * sizeof(struct sensor_s), SeekSet);
  file.write((uint8_t*)&empty, sizeof(struct sensor_s));

  err_read_idx += 1;

  // ... rolback ...
  if (err_read_idx >= FILE_ARRAY_SIZE) {
    err_read_idx = 0;
  }

  file.close();

}

void spiffs_open_file () {

  // ... allocate file ...
  if ( !SPIFFS.exists(FILE_NAME_ERR) ) {
    file = SPIFFS.open(FILE_NAME_ERR, "w+");
    for (int i = 0; i < FILE_ARRAY_SIZE; i++) {
      struct sensor_s sensor;
      memset(&sensor, 0, sizeof(struct sensor_s));
      file.write((const uint8_t*) &sensor, sizeof(struct sensor_s));
    }
    file.close();
  }

  file = SPIFFS.open(FILE_NAME_ERR, "r+");

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
