//
//  main.h
//  comcs-alert-server
//
//  Created by David Xavier on 16/11/2025.
//

#ifndef main_h
#define main_h

// Standard Libraries
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// Network & System
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>

// External Libraries
#include "MQTTClient.h"
#include <json-c/json.h>

#define PORT 3001
#define BUFFER_SIZE 1024

#define MQTT_URL                                                               \
  "ssl://d36b6378e3fc4b0ca6725112a64d3d59.s1.eu.hivemq.cloud:8883"
#define MQTT_CLIENTID "Alert Server"
#define MQTT_USERNAME "alert-server"
#define MQTT_PASSWORD "COMCSpassword1."
#define MQTT_TOPIC "comcs/g11/alerts"

struct udp_datagram {
  const char *id;
  const char *type;
  const char *location;
  const char *date_observed;
  double temperature;
  double relative_humidity;
};

int udp_start(int socket_fd);
int udp_validate_datagram(json_object *datagram, struct udp_datagram *d);
MQTTClient udp_create_mqtt_client(void);
void publish_alert(MQTTClient client, const char *msg);
int find_client_index(const char *id);
int find_free_slot(void);
void cleanup_inactive_clients(time_t now);
void process_readings(struct udp_datagram *data, MQTTClient client);
void try_reconnect_mqtt(MQTTClient client);

static inline void logger(const char *format, ...) {
  // Pegar tempo atual
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  // Formatar timestamp
  char timestr[20];
  strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", t);

  // Imprimir timestamp
  printf("[%s][%s] ", timestr, MQTT_CLIENTID);

  // Imprimir mensagem formatada
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);

  printf("\n");
}

#endif /* main_h */
