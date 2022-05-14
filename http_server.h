#ifndef UBIQUITOUS_OCTO_CARNIVAL_HTTP__HTTP_SERVER_H_
#define UBIQUITOUS_OCTO_CARNIVAL_HTTP__HTTP_SERVER_H_


#define MAX_GET_REQUEST_LENGTH (10 * 1024) // 10KB
#define MAX_FILE_NAME 1024
#define MAX_FILE_NAME_EXTENSION 32
#define BACKLOG 10


/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

void SetupSignalHandler();

void create_http_response_success (char *file_name, char *file_extension, char *response);
bool if_file_exists(char *file_name);

bool parse_http_request(char *request, char *file_name, char *file_extension);

bool is_valid_http_request(char *request);

int CreateServerSocket_ipv4(int port);

int CreateServerSocket_ipv6(int port);

void enable_socket(struct sockaddr* address, int socket_fd);

#endif //UBIQUITOUS_OCTO_CARNIVAL_HTTP__HTTP_SERVER_H_
