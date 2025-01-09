#ifndef INCLUDED_H
#define INCLUDED_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

const char *GET = "GET";

/**
 * This section is for server functions
*/

typedef struct request_pair {
    char *path, *query;
} request_pair;

void
write_len_to_socket(int socketID, const char *message);
void write_content_to_socket(int socketID, const char *content);
void http_404_reply(int socketID);
int is_get(char *text);
char *get_path(char *text);
char *read_file(FILE *file);


/**
 * This section is for client functions
*/

#endif
