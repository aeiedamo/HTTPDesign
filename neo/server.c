#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>

typedef struct {
  int req_arrival_count;
  double req_arrival_time;
  int req_dispatch_count;
  double req_dispatch_time;
  int req_complete_count;
  double req_complete_time;
  double req_age;
  pthread_t thread_id;
  int thread_count;
  int thread_static;
  int thread_dynamic;
} ServerStats;

typedef struct {
  int client_socket;
  struct timeval arrival_time;
  char *filename;
} Request;

typedef struct {
  pthread_t *threads;
  int thread_count;
  Request **buffer;
  int buffer_size;
  int buffer_count;
  int front;
  int rear;
  pthread_mutex_t buffer_mutex;
  pthread_cond_t buffer_not_full;
  pthread_cond_t buffer_not_empty;
  char *scheduling_alg;
  ServerStats stats;
} ThreadPool;

ThreadPool *pool = NULL;

void *worker_thread(void *arg);
void handle_request(Request *req);
void update_statistics(Request *req);
void initialize_thread_pool(int thread_count, int buffer_size, char *schedalg);
void destroy_thread_pool();
double get_time_diff(struct timeval start, struct timeval end);

void initialize_thread_pool(int thread_count, int buffer_size, char *schedalg) {
  pool = (ThreadPool *)malloc(sizeof(ThreadPool));
  pool->thread_count = thread_count;
  pool->buffer_size = buffer_size;
  pool->buffer_count = 0;
  pool->front = 0;
  pool->rear = 0;
  pool->scheduling_alg = schedalg;
  pthread_mutex_init(&pool->buffer_mutex, NULL);
  pthread_cond_init(&pool->buffer_not_full, NULL);
  pthread_cond_init(&pool->buffer_not_empty, NULL);
  pool->buffer = (Request **)malloc(buffer_size * sizeof(Request *));
  pool->threads = (pthread_t *)malloc(thread_count * sizeof(pthread_t));
  for (int i = 0; i < thread_count; i++) {
    pthread_create(&pool->threads[i], NULL, worker_thread, (void *)(long)i);
  }

  memset(&pool->stats, 0, sizeof(ServerStats));
  pool->stats.thread_count = thread_count;
}

void *worker_thread(void *arg) {
  int thread_num = (int)(long)arg;
  pool->stats.thread_id = pthread_self();

  while (1) {
    pthread_mutex_lock(&pool->buffer_mutex);

    while (pool->buffer_count == 0) {
      pthread_cond_wait(&pool->buffer_not_empty, &pool->buffer_mutex);
    }

    Request *req = pool->buffer[pool->front];
    pool->front = (pool->front + 1) % pool->buffer_size;
    pool->buffer_count--;

    pthread_cond_signal(&pool->buffer_not_full);
    pthread_mutex_unlock(&pool->buffer_mutex);

    if (req != NULL) {
      handle_request(req);
      update_statistics(req);
      free(req->filename);
      free(req);
    }
  }
  return NULL;
}

void handle_request(Request *req) {
  char response[1024];
  FILE *file = fopen(req->filename, "r");

  if (file == NULL) {
    snprintf(response, sizeof(response),
             "HTTP/1.1 404 Not Found\r\n"
             "Content-Length: 13\r\n"
             "\r\n"
             "File not found");
  } else {
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = malloc(file_size + 1);
    fread(file_content, 1, file_size, file);
    file_content[file_size] = '\0';

    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Length: %ld\r\n"
             "\r\n"
             "%s", file_size, file_content);

    free(file_content);
    fclose(file);
  }

  write(req->client_socket, response, strlen(response));
  close(req->client_socket);
}

void update_statistics(Request *req) {
  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  pthread_mutex_lock(&pool->buffer_mutex);

  pool->stats.req_arrival_count++;
  pool->stats.req_arrival_time += get_time_diff(req->arrival_time, current_time);
  pool->stats.req_complete_count++;
  pool->stats.req_age += get_time_diff(req->arrival_time, current_time);

  pthread_mutex_unlock(&pool->buffer_mutex);
}

double get_time_diff(struct timeval start, struct timeval end) {
  return (end.tv_sec - start.tv_sec) + 
  (end.tv_usec - start.tv_usec) / 1000000.0;
}

int main(int argc, char *argv[]) {
  if (argc != 5) {
    fprintf(stderr, "Usage: %s [portnum] [threads] [buffers] [schedalg]\n", argv[0]);
    exit(1);
  }

  int port = atoi(argv[1]);
  int threads = atoi(argv[2]);
  int buffers = atoi(argv[3]);
  char *schedalg = argv[4];

  initialize_thread_pool(threads, buffers, schedalg);

  int server_socket = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket < 0) {
    perror("Error creating socket");
    exit(1);
  }

  int opt = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("Error setting socket options");
    exit(1);
  }

  struct sockaddr_in server_addr;
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Error binding socket");
    exit(1);
  }

  if (listen(server_socket, 1024) < 0) {
    perror("Error listening on socket");
    exit(1);
  }

  printf("Server listening on port %d...\n", port);

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
    if (client_socket < 0) {
      perror("Error accepting connection");
      continue;
    }

    Request *req = (Request *)malloc(sizeof(Request));
    req->client_socket = client_socket;
    gettimeofday(&req->arrival_time, NULL);

    char buffer[1024];
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
      buffer[bytes_read] = '\0';

      char *filename_start = strstr(buffer, "GET /") + 5;
      char *filename_end = strstr(filename_start, " ");
      int filename_len = filename_end - filename_start;

      req->filename = (char *)malloc(filename_len + 1);
      strncpy(req->filename, filename_start, filename_len);
      req->filename[filename_len] = '\0';

      pthread_mutex_lock(&pool->buffer_mutex);

      while (pool->buffer_count == pool->buffer_size) {
        pthread_cond_wait(&pool->buffer_not_full, &pool->buffer_mutex);
      }

      pool->buffer[pool->rear] = req;
      pool->rear = (pool->rear + 1) % pool->buffer_size;
      pool->buffer_count++;

      pthread_cond_signal(&pool->buffer_not_empty);
      pthread_mutex_unlock(&pool->buffer_mutex);
    }
  }

  return 0;
}
