#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include "mime.h"
#include "file.h"
#include "http_server.h"



int server_socket_fd;

int main(int argc, char *argv[])
{
  int port, new_socket_fd, protocol_type;
  pthread_attr_t pthread_client_attr;
  pthread_t pthread;
  socklen_t client_address_len;
  struct sockaddr_in client_address;
  char *web_root;

  if  (argc != 4)
  {
    printf("Usage: %s [protocol number] [port number] [path to web root]\n", argv[0]);
    exit(1);
  }

  /* Get protocol version from command line arguments or stdin.
  * */
  protocol_type = atoi(argv[1]);
  if (protocol_type != 4 && protocol_type != 6)
  {
    printf("Invalid protocol number.\n");
    exit(1);
  }

  /* Get port from command line arguments or stdin.
   * */
  port = atoi(argv[2]);
  if (port < 0 || port > 65535)
  {
    printf("Invalid port number.\n");
    exit(1);
  }

  /* Get web root from command line arguments or stdin.
  * */
  web_root = argv[3];
  if (web_root == NULL)
  {
    printf("Invalid path to web root.\n");
    exit(1);
  }

  if (if_file_exists(web_root))
  {
    printf("Invalid path to web root.\n");
    exit(1);
  }

  /*Create the server socket */
  if (protocol_type == 4)
  {
    server_socket_fd = CreateServerSocket_ipv4(port);
  }
  else
  {
    server_socket_fd = CreateServerSocket_ipv6(port);
  }

  /*Setup the signal handler*/
  SetupSignalHandler();

  /* Initialise pthread attribute to create detached threads. */
  if (pthread_attr_init(&pthread_client_attr) != 0) {
    perror("pthread_attr_init");
    exit(1);
  }
  if (pthread_attr_setdetachstate(&pthread_client_attr, PTHREAD_CREATE_DETACHED) != 0) {
    perror("pthread_attr_setdetachstate");
    exit(1);
  }

  while (1) {

    /* Accept connection to client. */
    client_address_len = sizeof (client_address);
    new_socket_fd = accept(server_socket_fd, (struct sockaddr *)&client_address, &client_address_len);
    if (new_socket_fd == -1) {
      perror("accept");
      continue;
    }

    printf("Client connected\n");
    unsigned int *thread_arg = (unsigned int *) malloc(sizeof(unsigned int));
    *thread_arg = new_socket_fd;
    /* Create thread to serve connection to client. */
    if (pthread_create(&pthread, &pthread_client_attr, pthread_routine, (void *)thread_arg) != 0) {
      perror("pthread_create");
      continue;
    }
  }

  return 0;
}

int CreateServerSocket_ipv6(int port)
{
  struct sockaddr_in6 address;
  int socket_fd;

  /* Initialise IPv4 address. */
  memset(&address, 0, sizeof (address));
  address.sin6_family = AF_INET6;
  address.sin6_port  = htons(port);
  address.sin6_addr = in6addr_any;

  /* Create TCP socket. */
  if ((socket_fd = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  enable_socket((struct sockaddr*)&address, socket_fd);
  return socket_fd;

}

void enable_socket(struct sockaddr* address, int socket_fd)
{
  /* Bind address to socket. */
  if (bind(socket_fd, (struct sockaddr *) address, sizeof ((*address))) == -1) {
    perror("bind");
    exit(1);
  }

  /* Listen on socket. */
  if (listen(socket_fd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  // Configure server socket
  int enable = 1;
  setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
}

int CreateServerSocket_ipv4(int port)
{
  struct sockaddr_in address;
  int socket_fd;

  /* Initialise IPv4 address. */
  memset(&address, 0, sizeof (address));
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = INADDR_ANY;

  /* Create TCP socket. */
  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    exit(1);
  }

  enable_socket((struct sockaddr*)&address, socket_fd);
  return socket_fd;
}

void SetupSignalHandler() {/* Assign signal handlers to signals. */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    perror("signal");
    exit(1);
  }
  if (signal(SIGTERM, signal_handler) == SIG_ERR) {
    perror("signal");
    exit(1);
  }
  if (signal(SIGINT, signal_handler) == SIG_ERR) {
    perror("signal");
    exit(1);
  }
}


bool is_valid_http_request(char *request) {
  if (strncmp(request, "GET", 3) == 0) {
    return true;
  }
  return false;
}

bool parse_http_request(char *request, char *file_name, char *file_extension) {
  char *token;
  token = strtok(request, " ");
  if (token == NULL) {
    return false;
  }
  strcpy(file_name, token);
  token = strtok(NULL, " ");
  if (token == NULL) {
    return false;
  }
  strcpy(file_extension, token);
  return true;
}

bool if_file_exists(char *file_name) {
  bool does_exist = false;
  if( access( file_name, F_OK ) == 0 ) {
    does_exist = true;
  } else {
    does_exist = false;
  }
}

void create_http_response_success (char *file_name, char *file_extension, char *response)
{
  char *content_type = mime_type_get(file_extension);
  sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);
}

void *pthread_routine(void *arg)
{
  int client_socket = *(int*) arg;
  free(arg);

  char request[MAX_GET_REQUEST_LENGTH] = {0};

  /* Receive request from client. */
  if (recv(client_socket, request, MAX_GET_REQUEST_LENGTH, 0) == -1) {
    perror("recv");
  }

  /* Parse request. */
  char file_name[MAX_FILE_NAME] = {0};
  char file_extension[MAX_FILE_NAME_EXTENSION] = {0};
  if (!is_valid_http_request(request)) {
    send(client_socket, "HTTP/1.1 400 Bad Request\r\n\r\n", strlen("HTTP/1.1 400 Bad Request\r\n\r\n"), 0);
  }

  if (parse_http_request(request, file_name, file_extension)) {
    if (if_file_exists(file_name)) {
      char response[MAX_GET_REQUEST_LENGTH] = {0};
      create_http_response_success(file_name, file_extension, response);
      send(client_socket, response, strlen(response), 0);
    }
  }
  return NULL;
}

void signal_handler(int signal_number)
{
  close(server_socket_fd);
  exit(0);
}