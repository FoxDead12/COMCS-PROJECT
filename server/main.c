/*

 COMCS ALERT SERVER

 Requirements:
    -> UDP Server
    -> Best QoS in UDP communications
    -> Ability responde multi clients

 Client UPD request body:
    -> temperature: { "current": value, "average": value };
    -> humidity:    { "current": value, "average": value };

 gcc main.c -o main -I/opt/homebrew/include -L/opt/homebrew/lib -lpaho-mqtt3c -ljson-c
  gcc main.c -o main -lpaho-mqtt3c -ljson-c


*/

#include "main.h"


int
main(int argc, const char * argv[]) {

  logger(".... starting ...");

  int socket_fd = 0;
  int optval    = 0;
  struct sockaddr_in serveraddr;

  // ... create socket file descriptor ...
  socket_fd = socket(AF_INET, SOCK_DGRAM, 17); // 17 - is protocol UDP (come from MAC mini M4, check linux)
  if ( socket_fd < 0 ) {
    perror("error opening socket");
  }

  logger("socket created");


  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval, sizeof(int));


  // ... set address of server ...
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short) PORT);


  // ... bind socket to port ...
  if( bind(socket_fd, (struct sockaddr*) &serveraddr, sizeof(serveraddr)) < 0 ) {
    perror("error binding socket");
  }

  logger("socket bind to host and port");


  // ... UDP server start here ...
  udp_start(socket_fd);


  return EXIT_SUCCESS;
}


int
udp_start (int socket_fd) {

  // ... create MQTT cliente ...
  MQTTClient client = udp_create_mqtt_client();


  // ... make UDP server ...
  char buffer[BUFFER_SIZE + 1] = {0};
  json_object* datagram;


  while (1) {

    /*
      Datagram message format server accepts:
      { "client_id": "ESP32-01", "temperature": { "current": 24, "average": 20 }, "humidity": { "current": 90, "average": 80 } }
     */

    bzero(&buffer, BUFFER_SIZE + 1);
    size_t n = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, NULL, NULL);

    printf("buffer: %s\n", buffer);

    // ... check if recv return error ...
    if ( n < 0 ) {
      perror("error recvfrom");
      continue;
    }


    // ... parse string to json parser ...
    datagram = json_tokener_parse(buffer);


    // ... json is invalid, parser can't parse json ...
    if ( datagram == NULL ) {
      perror("invalid json format datagram");
      continue;
    }


    // ... validate if datagram has the desired format keys ...
    struct udp_datagram d;

    if ( udp_validate_datagram(datagram, &d) == EXIT_FAILURE ) {
      continue;
    }


    double diferential_temperature = fabs(d.current_t - d.average_t);
    double diferential_humidity    = fabs(d.current_h - d.average_h);

    if ( diferential_temperature > 2 ) {
      printf("Gerar alerta de temperature, valor a %f\n", diferential_temperature);
    }

    if ( diferential_humidity > 5 ) {
      printf("Gerar alerta de humidade, valor a %f\n", diferential_humidity);
    }


    json_object_put(datagram);

  }


  return EXIT_SUCCESS;
}

int
udp_validate_datagram (json_object* datagram, struct udp_datagram* d) {

  /*
   { "client_id": "ESP32-01", "temperature": { "current": 24, "average": 20 }, "humidity": { "current": 90, "average": 80 } }
   */
  json_object* client_id;

  if ( !json_object_object_get_ex(datagram, "client_id", &client_id) ) {
    perror("invalid json message, missing key 'client_id'");
    return EXIT_FAILURE;
  }


  json_object* temperature;
  json_object* temperature_current;
  json_object* temperature_average;

  if ( json_object_object_get_ex(datagram, "temperature", &temperature) ) {

    if ( !json_object_object_get_ex(temperature, "current", &temperature_current) ) {
      perror("invalid json message, missing key 'temperature.current'");
      return EXIT_FAILURE;
    }

    if ( !json_object_object_get_ex(temperature, "average", &temperature_average) ) {
      perror("invalid json message, missing key 'temperature.average'");
      return EXIT_FAILURE;
    }

  } else {

    perror("invalid json message, missing key 'temperature'");
    return EXIT_FAILURE;

  }


  json_object* humidity;
  json_object* humidity_current;
  json_object* humidity_average;

  if ( json_object_object_get_ex(datagram, "humidity", &humidity) ) {

    if ( !json_object_object_get_ex(humidity, "current", &humidity_current) ) {
      perror("invalid json message, missing key 'humidity.current'");
      return EXIT_FAILURE;
    }

    if ( !json_object_object_get_ex(humidity, "average", &humidity_average) ) {
      perror("invalid json message, missing key 'humidity.average'");
      return EXIT_FAILURE;
    }

  } else {

    perror("invalid json message, missing key 'humidity'");
    return EXIT_FAILURE;

  }


  double tcurrent = json_object_get_double(temperature_current);
  double taverage = json_object_get_double(temperature_average);

  double hcurrent = json_object_get_double(humidity_current);
  double haverage = json_object_get_double(humidity_average);


  if ( tcurrent < 0 || tcurrent > 50 ) {
    perror("invalid value 'temperature.current' number between range 0 to 50");
    return EXIT_FAILURE;
  }
  if ( taverage < 0 || taverage > 50 ) {
    perror("invalid value 'temperature.average' number between range 0 to 50");
    return EXIT_FAILURE;
  }


  if ( hcurrent < 20 || hcurrent > 80 ) {
    perror("invalid value 'humidity.current' number between range 20 to 80");
    return EXIT_FAILURE;
  }

  if ( haverage < 20 || haverage > 80 ) {
    perror("invalid value 'humidity.average' number between range 20 to 80");
    return EXIT_FAILURE;
  }

  d->client_id = json_object_get_string(client_id);

  d->current_t = json_object_get_double(temperature_current);
  d->average_t = json_object_get_double(temperature_average);

  d->current_h = json_object_get_double(humidity_current);
  d->average_h = json_object_get_double(humidity_average);

  return EXIT_SUCCESS;
}

MQTTClient
udp_create_mqtt_client (void) {

  MQTTClient client;
  MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

  MQTTClient_create(&client, MQTT_URL, MQTT_CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
  conn_opts.keepAliveInterval = 20;
  conn_opts.cleansession = 1;
  conn_opts.username = MQTT_USERNAME;
  conn_opts.password = MQTT_PASSWORD;

  int rc;
  if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS) {
    printf("Failed to connect, return code %d\n", rc);
    exit(-1);
  }

//
//   logger("mqtt client creating");
//
//   MQTTClient client;
//   MQTTClient_create(&client, MQTT_URL, MQTT_CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
//
//   logger(MQTT_USERNAME);
//   logger(MQTT_PASSWORD);
//
//   MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
//   conn_opts.username = MQTT_USERNAME;
//   conn_opts.password = MQTT_PASSWORD;
//   conn_opts.keepAliveInterval = 10;
//   conn_opts.cleansession = 1;
//
//   MQTTClient_SSLOptions ssl_opts = MQTTClient_SSLOptions_initializer;
//   ssl_opts.enableServerCertAuth = 0;
//
//     ssl_opts.verify = 1;
//     ssl_opts.CApath = NULL;
//     ssl_opts.keyStore = NULL;
//     ssl_opts.trustStore = NULL;
//     ssl_opts.privateKey = NULL;
//     ssl_opts.privateKeyPassword = NULL;
//     ssl_opts.enabledCipherSuites = NULL;
//   conn_opts.ssl = &ssl_opts;
//
//   int r = MQTTClient_connect(client, &conn_opts);
//   if (r != MQTTCLIENT_SUCCESS) {
//     printf("MQTT CONNECT FAILED: return code %d\n", r);
//     exit(EXIT_FAILURE);
//   } else {
//     printf("MQTT CONNECT SUCCESS!\n");
//   }
//
//   int qos = 1;
//   int retained = 0;
//
//   MQTTClient_subscribe(client, MQTT_TOPIC, qos);
//   printf("subscribed to %s \n", MQTT_TOPIC);
//
//   char* payload = "<your_payload>";
//   int payloadlen = strlen(payload);
//   MQTTClient_deliveryToken dt;
//   MQTTClient_publish(client, MQTT_TOPIC, payloadlen, payload, qos, retained, &dt);
//   printf("published to %s \n", MQTT_TOPIC);

//   logger(rc);

//   MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
//   opts.username = MQTT_USERNAME;
//   opts.password = MQTT_PASSWORD;
//   opts.keepAliveInterval = 10;
//   opts.cleansession = 1;
//
//
//   if ( MQTTClient_connect(client, &opts) != MQTTCLIENT_SUCCESS ) {
//     perror("error MQTT client connect");
//     exit(EXIT_FAILURE);
//   }


  return client;
}









int
udp_datagram_handler (int socket_fd) {

  /*
  // ... reset reader of file descriptors ...
  fd_set read_fds;
  FD_ZERO(&read_fds);
  FD_SET(socket_fd, &read_fds);


  // ... wait for events in file descriptors ...
  int activity = select(socket_fd + 1, &read_fds, NULL, NULL, NULL);
  if ( activity < 0 ) {
    perror("error select");
    break;
  }


  // ... read event ...
  if ( FD_ISSET(socket_fd, &read_fds) ) {
    udp_datagram_handler(socket_fd);
  }*/

  // ... write events, send data to MQTT server ...

  /*
    Datagram message format server accepts:
    { "client_id": "ESP32-01", "temperature": { "current": 24, "average": 20 }, "humidity": { "current": 90, "average": 80 } }
   */

  char buffer[BUFFER_SIZE + 1] = {0};
  size_t n = recvfrom(socket_fd, buffer, BUFFER_SIZE, 0, NULL, NULL);



  return 1;
}
