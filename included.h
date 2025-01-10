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
#include <ctype.h>

#include "threadpool/threadpool.h"

const char *CGI_BIN_PATH = "./cgi-bin/";
const char *GET = "GET";
const int MAX_CWD = 100;

/**
 * This section is for server functions
*/

typedef struct request_pair {
    char *path, *query;
} request_pair;

void write_len_to_socket(int socketID, const char *message);
void write_content_to_socket(int socketID, const char *content);
void http_404_reply(int socketID);
void http_get_reply(int socketID, const char *content);
int is_get(char *text);
char *get_path(char *text);
char *read_file(FILE *file);
request_pair extract_query(const char *cgipath);
void run_cgi(int socketID, const char *currentDIR, const char *cgipath);
void *handle_socket_thread(void *socketID_arg);
int create_listen_socket();

/**
 * This section is for client functions
*/
char *read_text_from_socket(int sockfd);
void write_to_socket(int sockfd, const char *message);


/**
 * these functions are used in strings
*/

int starts_with(char *s, const char *with)
{
    return strncmp(s, with, strlen(with)) == 0;
}

int ends_with(const char *s, const char *with)
{
    int len_s = strlen(s);
    int len_with = strlen(with);

    if (len_with <= len_s)
    {
        return strncmp(s + len_s - len_with, with, len_with) == 0;
    }
    else
    {
        return 0;
    }
}

int contains(const char *s1, const char *s2)
{
    return strstr(s1, s2) != NULL;
}

char *substr(const char *input, int offset, int len, char *dest)
{
    int input_len = strlen(input);

    if (offset + len > input_len)
    {
        return NULL;
    }

    strncpy(dest, input + offset, len);
    return dest;
}

int ends_with_extension(const char *inp)
{
    int end_pos = strlen(inp);

    while (--end_pos >= 0)
    {
        if (inp[end_pos] == '.')
            return 1;
        if (!isalpha(inp[end_pos]))
            return 0;
    }

    return 0;
}

char *concat(const char *s1, const char *s2)
{
    char *r = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(r, s1);
    strcat(r, s2);
    return r;
}

char *concat3(const char *s1, const char *s2, const char *s3)
{
    char *r = malloc(strlen(s1) + strlen(s2) + strlen(s3) + 1);
    strcpy(r, s1);
    strcat(r, s2);
    strcat(r, s3);
    return r;
}

char *concat4(const char *s1, const char *s2, const char *s3, const char *s4)
{
    char *r = malloc(strlen(s1) + strlen(s2) + strlen(s3) + strlen(s4) + 1);
    strcpy(r, s1);
    strcat(r, s2);
    strcat(r, s3);
    strcat(r, s4);
    return r;
}

char *strappend(const char *s1, const char *s2)
{
    char *r = malloc(strlen(s1) + strlen(s2) + 1);
    strcpy(r, s1);
    strcat(r, s2);
    assert(strlen(r) == strlen(s1) + strlen(s2));
    return r;
}

#endif
