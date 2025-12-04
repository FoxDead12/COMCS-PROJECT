/*
 COMCS ALERT SERVER

 Requirements:
    -> UDP Server
    -> Best QoS in UDP communications
    -> Ability responde multi clients

 Client UPD request body:
    -> temperature: { "current": value, "average": value };
    -> humidity:    { "current": value, "average": value };

 gcc main.c -o main -I/opt/homebrew/include -L/opt/homebrew/lib -lpaho-mqtt3cs
-ljson-c

gcc main.c -o main -lpaho-mqtt3cs -ljson-c
*/

#include "main.h"

#define ACK_MESSAGE "ACK_OK"
#define ID_SIZE 18
#define MAX_CLIENTS 2
#define DATA_FRESHNESS_LIMIT 10.0
#define CLIENT_TIMEOUT 60.0

volatile int keep_running = 1;

void handle_sigint(int dummy) { keep_running = 0; }

struct ClientState {
  char id[ID_SIZE];
  double last_temp;
  double last_hum;
  time_t last_seen;
  int active;
};

struct ClientState clients[MAX_CLIENTS];

#define VALIDATE_JSON_STR(jobj, key, dest)                                     \
  {                                                                            \
    json_object *tmp;                                                          \
    if (json_object_object_get_ex(jobj, key, &tmp)) {                          \
      dest = json_object_get_string(tmp);                                      \
    } else {                                                                   \
      logger("Validation Error: Missing '%s'", key);                           \
      return EXIT_FAILURE;                                                     \
    }                                                                          \
  }

#define VALIDATE_JSON_DOUBLE(jobj, key, dest)                                  \
  {                                                                            \
    json_object *tmp;                                                          \
    if (json_object_object_get_ex(jobj, key, &tmp)) {                          \
      dest = json_object_get_double(tmp);                                      \
    } else {                                                                   \
      logger("Validation Error: Missing '%s'", key);                           \
      return EXIT_FAILURE;                                                     \
    }                                                                          \
  }

int main(int argc, const char *argv[]) {
  logger(".... starting Alert Server ...");
  struct sigaction sa;
  sa.sa_handler = handle_sigint;
  sa.sa_flags = 0;
  sigemptyset(&sa.sa_mask);

  if (sigaction(SIGINT, &sa, NULL) == -1) {
    perror("Error setting up signal handler");
    return EXIT_FAILURE;
  }

  struct sockaddr_in serveraddr;

  // ... create socket file descriptor ...
  int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (udp_socket < 0) {
    perror("error opening socket");
  }

  int optval = 1;
  if (setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                 sizeof(int)) < 0) {
    perror("setsockopt failed");
    return EXIT_FAILURE;
  }

  // ... set address of server ...
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)PORT);

  // ... bind socket to port ...
  if (bind(udp_socket, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) <
      0) {
    perror("error binding socket");
  }

  logger("Socket bind to host and port");

  // ... UDP server start here ...
  udp_start(udp_socket);

  logger("Alert server stopped");

  return EXIT_SUCCESS;
}

int udp_start(int udp_socket) {

  // ... create MQTT cliente ...

  MQTTClient client = udp_create_mqtt_client();

  // ... make UDP server ...
  char buffer[BUFFER_SIZE + 1] = {0};
  json_object *datagram;

  struct sockaddr_in client_addr;
  socklen_t client_len = sizeof(client_addr);

  while (keep_running) {
    bzero(&buffer, BUFFER_SIZE + 1);
    size_t n = recvfrom(udp_socket, buffer, BUFFER_SIZE, 0, NULL, NULL);

    // ... check if recv return error ...
    if (n < 0) {
      perror("error recvfrom");
      continue;
    }

    printf("buffer: %s\n", buffer);

    // ... parse string to json parser ...
    datagram = json_tokener_parse(buffer);

    // ... json is invalid, parser can't parse json ...
    if (datagram == NULL) {
      logger("Invalid JSON format from %s", inet_ntoa(client_addr.sin_addr));
      continue;
    }

    // ... validate if datagram has the desired format keys ...
    struct udp_datagram d;
    if (udp_validate_datagram(datagram, &d) == EXIT_FAILURE) {
      logger("Invalid packet from %s", inet_ntoa(client_addr.sin_addr));
      json_object_put(datagram);
      continue;
    }

    process_readings(&d, client);

    sendto(udp_socket, ACK_MESSAGE, strlen(ACK_MESSAGE), 0,
           (struct sockaddr *)&client_addr, client_len);

    json_object_put(datagram);
  }

  MQTTClient_disconnect(client, 10000);
  MQTTClient_destroy(&client);
  return EXIT_SUCCESS;
}

int udp_validate_datagram(json_object *datagram, struct udp_datagram *d) {
  d->id = NULL;
  d->type = NULL;
  d->location = NULL;
  d->date_observed = NULL;
  d->temperature = -999.0;
  d->relative_humidity = -999.0;

  VALIDATE_JSON_STR(datagram, "id", d->id);
  VALIDATE_JSON_STR(datagram, "type", d->type);
  VALIDATE_JSON_STR(datagram, "location", d->location);
  VALIDATE_JSON_STR(datagram, "dateObserved", d->date_observed);
  VALIDATE_JSON_DOUBLE(datagram, "temperature", d->temperature);
  VALIDATE_JSON_DOUBLE(datagram, "relativeHumidity", d->relative_humidity);

  if (d->temperature < 0 || d->temperature > 50) {
    logger("Validation Error: Temperature %.2f out of range (0-50)",
           d->temperature);
    return EXIT_FAILURE;
  }

  if (d->relative_humidity < 20 || d->relative_humidity > 80) {
    logger("Validation Error: Humidity %.2f out of range (20-80)",
           d->relative_humidity);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void setup_mqtt_options(MQTTClient_connectOptions *conn_opts,
                        MQTTClient_SSLOptions *ssl_opts) {

  conn_opts->keepAliveInterval = 20;
  conn_opts->cleansession = 1;
  conn_opts->username = MQTT_USERNAME;
  conn_opts->password = MQTT_PASSWORD;
  conn_opts->connectTimeout = 2;

  ssl_opts->enableServerCertAuth = 1;
  ssl_opts->trustStore = "./cert.pem";
  conn_opts->ssl = ssl_opts;
}

MQTTClient udp_create_mqtt_client(void) {

  MQTTClient client;

  MQTTClient_create(&client, MQTT_URL, MQTT_CLIENTID,
                    MQTTCLIENT_PERSISTENCE_NONE, NULL);

  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;

  setup_mqtt_options(&conn_opts, &ssl_opts);

  int rc = MQTTClient_connect(client, &conn_opts);
  if (rc != MQTTCLIENT_SUCCESS) {
    logger("MQTT connection failed: %d\n", rc);
  } else {
    logger("MQTT connected");
  }

  return client;
}

void try_reconnect_mqtt(MQTTClient client) {
  if (MQTTClient_isConnected(client)) {
    return;
  }

  logger("MQTT Disconnected. Attempting to reconnect...");

  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
  MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;

  setup_mqtt_options(&conn_opts, &ssl_opts);

  int rc = MQTTClient_connect(client, &conn_opts);
  if (rc != MQTTCLIENT_SUCCESS) {
    logger("MQTT reconnection failed: %d\n", rc);
  } else {
    logger("MQTT reconnected");
  }
}

void publish_alert(MQTTClient client, const char *msg) {
  if (!client)
    return;

  try_reconnect_mqtt(client);

  if (MQTTClient_isConnected(client)) {
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    pubmsg.payload = (void *)msg;
    pubmsg.payloadlen = (int)strlen(msg);
    pubmsg.qos = 1;
    pubmsg.retained = 0;
    MQTTClient_deliveryToken token;

    int rc = MQTTClient_publishMessage(client, MQTT_TOPIC, &pubmsg, &token);
    if (rc != MQTTCLIENT_SUCCESS) {
      logger("Error publishing message: %d", rc);
    } else {
      logger("MQTT Message published.");
    }
  } else {
    logger("No MQTT connection");
  }
}

void send_diff_alert(MQTTClient client, const char *type, const char *time,
                     const char *dev1, const char *dev2, double diff) {
  char alert_msg[512];

  snprintf(alert_msg, sizeof(alert_msg),
           "{\"alert\": \"SensorMismatch\", \"type\": \"%s\", "
           "\"timestamp\": \"%s\", \"dev1\": \"%s\", "
           "\"dev2\": \"%s\", \"diff\": %.1f}",
           type, time, dev1, dev2, diff);

  logger("SENSOR MISMATCH DETECTED: %s", alert_msg);
  publish_alert(client, alert_msg);
}

void process_readings(struct udp_datagram *data, MQTTClient client) {
  time_t now = time(NULL);
  cleanup_inactive_clients(now);

  int idx = find_client_index(data->id);

  if (idx == -1) {
    idx = find_free_slot();

    if (idx != -1) {
      strncpy(clients[idx].id, data->id, ID_SIZE - 1);
      clients[idx].active = 1;
      logger("New Client Registered at Slot %d: %s", idx, data->id);
    } else {
      logger("Registry full (Max %d). Cannot store state for %s", MAX_CLIENTS,
             data->id);
      return;
    }
  }

  clients[idx].last_temp = data->temperature;
  clients[idx].last_hum = data->relative_humidity;
  clients[idx].last_seen = now;

  char alert_msg[512];

  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (i == idx || !clients[i].active)
      continue;

    double seconds_stale = difftime(now, clients[i].last_seen);
    if (seconds_stale > DATA_FRESHNESS_LIMIT) {
      continue;
    }

    double diff_temp = fabs(clients[idx].last_temp - clients[i].last_temp);
    double diff_hum = fabs(clients[idx].last_hum - clients[i].last_hum);

    if (diff_temp > 2.0) {
      send_diff_alert(client, "temperature", data->date_observed,
                      clients[idx].id, clients[i].id, diff_temp);
    }

    if (diff_hum > 5.0) {
      send_diff_alert(client, "humidity", data->date_observed, clients[idx].id,
                      clients[i].id, diff_hum);
    }
  }
}

int find_client_index(const char *id) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].active && strcmp(clients[i].id, id) == 0) {
      return i;
    }
  }
  return -1;
}

int find_free_slot() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (!clients[i].active) {
      return i;
    }
  }
  return -1;
}

void cleanup_inactive_clients(time_t now) {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clients[i].active) {
      double seconds_inactive = difftime(now, clients[i].last_seen);
      if (seconds_inactive > CLIENT_TIMEOUT) {
        logger("Client '%s' timed out (Inactive %.0fs). Slot %d freed.",
               clients[i].id, seconds_inactive, i);
        clients[i].active = 0;
        memset(clients[i].id, 0, ID_SIZE);
      }
    }
  }
}
