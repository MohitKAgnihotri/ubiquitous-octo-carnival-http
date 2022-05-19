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
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "mime.h"
#include "http_server.h"


char *web_root = NULL;
int server_socket_fd;

int main(int argc, char *argv[])
{
  int port, new_socket_fd, protocol_type;
  pthread_attr_t pthread_client_attr;
  pthread_t pthread;
  socklen_t client_address_len;
  struct sockaddr_in client_address;

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

  /* Check if the web root exist */
  if (!if_file_exists(web_root))
  {
    printf("Invalid path to web root.\n");
    exit(1);
  }

  /*Create the server socket */
  if (protocol_type == 4)
  {
    /* IPV4 type */
    server_socket_fd = CreateServerSocket_ipv4(port);
  }
  else
  {
    /* IPV6 type */
    server_socket_fd = CreateServerSocket_ipv6(port);
  }

  /*Setup the signal handler*/
  SetupSignalHandler();

  /* Initialise pthread attribute to create detached threads. */
  if (pthread_attr_init(&pthread_client_attr) != 0) {
    perror("pthread_attr_init");
    exit(1);
  }
  /**
   * Set the detach state of the thread attribute object to PTHREAD_CREATE_DETACHED.
   * */
  if (pthread_attr_setdetachstate(&pthread_client_attr, PTHREAD_CREATE_DETACHED) != 0) {
    perror("pthread_attr_setdetachstate");
    exit(1);
  }

  while (1) {

    /* Accept connection to client. */
    client_address_len = sizeof (client_address);

    /* Accept connection to client. */
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

/* Create IPv6 server socket. */
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

/* Create IPv6 server socket. */
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

/** Setup signal handler. */

void SetupSignalHandler() {

  /* Assign signal handlers to signals. */
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

/* Check if it's a valid GET Req*/
bool is_valid_http_request(char *request) {
  if (strncmp(request, "GET", 3) == 0) {
    return true;
  }
  return false;
}

/*Parse HTTP Request:
 * Request of the following type are supported GET /index.llll HTTP/1.0 */
bool parse_http_request(char *request, char *file_name, char *file_extension) {
    char *pointer_start_filename = strchr(request, '/');
    if (pointer_start_filename == NULL) {
        return false;
    }
    char *pointer_start_HTTP = strchr(request, 'H');
    if (pointer_start_HTTP == NULL) {
        return false;
    }
    pointer_start_HTTP--;

    strncpy(file_name, pointer_start_filename + 1, pointer_start_HTTP - pointer_start_filename - 1);
    file_name[pointer_start_HTTP - pointer_start_filename - 1] = '\0';

    char *pointer_start_extension = strchr(file_name, '.');
    if (pointer_start_extension == NULL) 
    {
        // case : No filename extension
        memset(file_extension, 0x00, MAX_FILE_NAME_EXTENSION);
    }
    else
    {
        strncpy(file_extension, pointer_start_extension + 1, strlen(file_name) - (pointer_start_extension - file_name));
        file_extension[strlen(file_name) - (pointer_start_extension - file_name)] = '\0';
    }
    return true;

}

/* Verify if the file exist */
bool if_file_exists(char *file_name) {
  bool does_exist = false;
  if( access( file_name, F_OK ) == 0 ) {
    does_exist = true;
  } else {
    does_exist = false;
  }
  return does_exist;
}

/* Determine the size of the file */
unsigned int get_file_size(char *file_name) {
  struct stat st;
  stat(file_name, &st);
  return st.st_size;
}

/* Create and send the HTTP Success Response */
void create_n_send_http_response_success (int client_socket, char *file_name, char *response)
{
  unsigned int file_size = get_file_size(file_name);

  char *content_type = mime_type_get(file_name);
  sprintf(response, "HTTP/1.0 200 OK \r\n\"\n"
                    "Server: My Server \r\n\"\n"
                    "MIME-version: 1.0\r\n"
                    "X-Powered-By: ME \r\n\"\n"
                    "Content-Language: nl \r\n\"\n"
                    "X-Cache: MISS \r\n\"\n"
                    "Content-Type: %s \r\n\"\n"
                    "Content-Length: %d \r\n\r\n\"", content_type, file_size);

  char *buffer = (char *) malloc(file_size + strlen(response));
  if (buffer == NULL) {
    perror("malloc");
    exit(1);
  }

  memcpy(buffer, response, strlen(response));

  // map file to memory
  int fd = open(file_name, O_RDONLY);
  char *file_content = mmap(NULL, file_size, PROT_READ, MAP_PRIVATE, fd, 0);
  close(fd);

  memcpy(buffer + strlen(response), file_content, file_size);

  // send file content
  size_t sent_bytes = send(client_socket, buffer, file_size + strlen(response) - 1, 0);
  if (sent_bytes < (file_size + strlen(response) -1)) {
        perror("send");
        exit(1);
  }
  munmap(file_content, file_size);
}

void create_http_failure_response (char *response)
{
  sprintf(response, "HTTP/1.1 404 Not Found \r\n"
                    "Server: MY Server \r\n"
                    "Date: Mon, 02 Jan 2012 02:33:17 GMT \r\n"
                    "Content-Type: text/html \r\n"
                    "Content-Length: 174 \r\n"
                    "Connection: close \r\n\r\n"
                    "<html> \r\n"
                    "<head><title>404 Not Found</title></head> \r\n"
                    "<body bgcolor=\"white\"> \r\n"
                     "<center><h1>404 Not Found</h1></center> \r\n"
                     "<hr><center>nginx/0.8.54</center> \r\n"
                     "</body>\r\n"
                     "</html>\r\n");
}

void *pthread_routine(void *arg)
{
  int client_socket = *(int*) arg;
  free(arg);

  char request[MAX_GET_REQUEST_LENGTH] = {0};
  char response[MAX_GET_REQUEST_LENGTH] = {0};

  /* Receive request from client. */
  if (recv(client_socket, request, MAX_GET_REQUEST_LENGTH, 0) == -1) {
    perror("recv");
  }

  /* Parse request. */
  char file_name[MAX_FILE_NAME] = {0};
  char file_extension[MAX_FILE_NAME_EXTENSION] = {0};
  if (!is_valid_http_request(request)) {
    memset(response, 0, sizeof(response));
    create_http_failure_response(response);
    send(client_socket, response, strlen(response), 0);
  }

  if (parse_http_request(request, file_name, file_extension))
  {
    /* Check if file exists. */
    char fully_qualified_file_name[MAX_FILE_NAME*2] = {0};
    sprintf(fully_qualified_file_name, "%s/%s", web_root, file_name);
    bool does_exist = if_file_exists(fully_qualified_file_name);
    if (does_exist) {
      memset(response, 0, sizeof(response));
      create_n_send_http_response_success(client_socket, fully_qualified_file_name, response);
    }
    else
    {
      memset(response, 0, sizeof(response));
      create_http_failure_response(response);
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
