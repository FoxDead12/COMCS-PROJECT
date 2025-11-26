//
//  main.h
//  comcs-alert-server
//
//  Created by David Xavier on 16/11/2025.
//

#ifndef main_h
#define main_h

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <json-c/json.h>
#include "MQTTClient.h"
#include <stdarg.h>
#include <time.h>

#define PORT 3001
#define BUFFER_SIZE 1024

#define MQTT_URL      "ssl://d36b6378e3fc4b0ca6725112a64d3d59.s1.eu.hivemq.cloud:8883"
#define MQTT_CLIENTID "alert_server"
#define MQTT_USERNAME "comcs2425cagg11"
#define MQTT_PASSWORD "COMCSpassword1."
#define MQTT_TOPIC    "/comcs/g11/alerts"

struct udp_datagram {
  const char* client_id;
  double current_t;
  double average_t;
  double current_h;
  double average_h;
};

int udp_start(int socket_fd);
int udp_validate_datagram(json_object* datagram, struct udp_datagram* d);
MQTTClient udp_create_mqtt_client(void);


void logger(const char *format, ...) {
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
