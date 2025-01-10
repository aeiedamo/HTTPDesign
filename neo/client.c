#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

typedef struct {
  char *host;
  int port;
  char *filename;
  char *schedalg;
} ThreadArg;

void *client_thread(void *arg) {
  ThreadArg *thread_arg = (ThreadArg *)arg;

  int client_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (client_socket < 0) {
    perror("Error creating socket");
    return NULL;
  }

  struct hostent *server = gethostbyname(thread_arg->host);
  if (server == NULL) {
    fprintf(stderr, "Error: could not resolve hostname\n");
    close(client_socket);
    return NULL;
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  server_addr.sin_port = htons(thread_arg->port);

  if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error connecting to server");
    close(client_socket);
    return NULL;
  }

  char request[1024];
  snprintf(request, sizeof(request),
           "GET /%s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Connection: close\r\n"
           "\r\n",
           thread_arg->filename, thread_arg->host);

  if (write(client_socket, request, strlen(request)) < 0) {
    perror("Error sending request");
    close(client_socket);
    return NULL;
  }

  char buffer[4096];
  ssize_t bytes_read;
  int total_bytes = 0;

  while ((bytes_read = read(client_socket, buffer, sizeof(buffer) - 1)) > 0) {
    buffer[bytes_read] = '\0';
    total_bytes += bytes_read;
    printf("Thread received %zd bytes from %s\n", bytes_read, thread_arg->filename);
  }

  printf("Thread completed request for %s (total: %d bytes)\n", 
         thread_arg->filename, total_bytes);

  close(client_socket);
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < 6) {
    fprintf(stderr, "Usage: %s [host] [portnum] [threads] [schedalg] [filename1] [filename2...]\n", 
            argv[0]);
    exit(1);
  }

  char *host = argv[1];
  int port = atoi(argv[2]);
  int thread_count = atoi(argv[3]);
  char *schedalg = argv[4];
  int file_count = argc - 5;

  ThreadArg *thread_args = malloc(file_count * sizeof(ThreadArg));
  pthread_t *threads = malloc(file_count * sizeof(pthread_t));

  for (int i = 0; i < file_count; i++) {
    thread_args[i].host = host;
    thread_args[i].port = port;
    thread_args[i].filename = argv[i + 5];
    thread_args[i].schedalg = schedalg;

    if (pthread_create(&threads[i], NULL, client_thread, &thread_args[i]) != 0) {
      perror("Error creating thread");
      exit(1);
    }
  }

  for (int i = 0; i < file_count; i++) {
    pthread_join(threads[i], NULL);
  }

  free(thread_args);
  free(threads);

  return 0;
}
