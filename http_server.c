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

#define BACKLOG 10
#define PORT_NUM 1313


/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

void SetupSignalHandler();

int CreateServerSocket(int port);

int server_socket_fd;

int main(int argc, char *argv[])
{
  int port, new_socket_fd;
  pthread_attr_t pthread_client_attr;
  pthread_t pthread;
  socklen_t client_address_len;
  struct sockaddr_in client_address;

  /* Get port from command line arguments or stdin.
   * For this server, this is fixed to 1113*/
  port = PORT_NUM;

  /*Create the server socket */
  server_socket_fd = CreateServerSocket(port);

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


int CreateServerSocket(int port)
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

  /* Bind address to socket. */
  if (bind(socket_fd, (struct sockaddr *)&address, sizeof (address)) == -1) {
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

void create_http_response_success
(char *file_name, char *file_extension, char *response) {
  char *content_type = mime_type_get(file_extension);
  sprintf(response, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n", content_type);

}

void *pthread_routine(void *arg)
{
  int client_socket = *(int*) arg;
  free(arg);




  write(client_socket, "Hello! This is HTTP Server", strlen("Hello! This is HTTP Server"));

  return NULL;
}

void signal_handler(int signal_number)
{
  close(server_socket_fd);
  exit(0);
}