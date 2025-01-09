#include "included.h"

/**
 * This function is used to write length of a content message to the socket
*/

void write_len_to_socket(int socketID, const char* message) {
    if (!message) {
        printf("[ERROR]: No message was found!\n");
        return;
    }

    write(socketID, message, strlen(message));
    write(socketID, "\r\n", 2);
}

/**
 * This function is used to write the content itself to the socket
*/

void write_content_to_socket(int socketID, const char *content) {
    int length_str[100];
    char *content_length_str = NULL;

    if (!content)
    {
        printf("[ERROR]: No content was found!\n");
        return;
    }

    sprintf(length_str, "%d", (int)(strlen(content)));

    content_length_str = concat("Content-Length: ", length_str);

    write_len_to_socket(socketID, "Server: HTTP Server / Linux\n");
    write_len_to_socket(socketID, "Content-Type: text/html");
    write_len_to_socket(socketID, content_length_str);
    write_len_to_socket(socketID, "");
    write_len_to_socket(socketID, content);

    free(content_length_str);
}

/**
 * This function is used for the case of unavailable reply (error code 404)
*/

void http_404_reply(int socketID) {
    static const char *content = NULL;

    write_len_to_socket(socketID, "HTTP/1.1: 404 Not Found");
    content = "<html><body><h1>Not found</h1></body></html>\r\n";
    write_content_to_socket(socketID, content);
}

int is_get(char *text) {
    if (!text) {
        printf("[ERROR]: No text was found\n");
        return (-1);
    }
    return starts_with(text, GET);
}

/**
 * This function is supposed is get the path of the cgi (common gateway
 * interface) binary
 */

char *get_path(char *text) {
    int start_position, end_position, path_length;
    char *end_path = NULL, *path = NULL;

    if (!text) {
        printf("[ERROR]: No text was found\n");
        return (NULL);
    }

    start_position = strlen(GET) + 1;
    end_path = strchr(text + start_position, " ");
    end_position = end_path - text;

    path_length = end_position - start_position;

    path = malloc(path_length + 1);
    substr(text, start_position, path_length, path);
    path[path_length] = 0;

    return (path);
}

int is_cgibin_request(const char *path) {
    if (!path || !(contains(contains(path, '/cgi-bin/'))))
        return 0;

    return 1;
}

char *read_file(FILE *file){
    int size = 10, i = 0, c;
    char *buffer = NULL, *new = NULL;

    if (!file) {
        printf("[ERROR]: File not found!\n");
        return (NULL);
    }

    buffer = malloc(size);

    for (; (c = fgetc(file) != EOF);) {
        assert(i < size);
        buffer[i++] = c;

        if (i == size) {
            new = malloc(size * 2);
            memcpy(new, buffer, size);
            free(buffer);
            buffer = new;
            size *= 2;
        }
    }

    buffer[i] = 0;
    return (buffer);
}

request_pair extract_query(const char *cgipath) {
    request_pair rtn;
    char *query = NULL;
    int path_len, query_len;
    const char *query_start = NULL;

    if (!cgipath) {
        printf("[ERROR: CGI path not found\n");
        return;
    }

    query = strchr(cgipath, '?');

    if (!query) {
        rtn.path = strdup(cgipath);
        rtn.query = NULL;
    }
    else {
        path_len = query - cgipath;
        rtn.path = malloc(path_len + 1);
        strncpy(rtn.path, cgipath, path_len);
        rtn.path[path_len] = 0;

        query_len = strlen(cgipath) - path_len - 1;
        rtn.query = malloc(query_len + 1);
        query_start = cgipath + path_len - 1;

        strncpy(rtn.query, query_start, query_len);
        rtn.query[query_len] = 0;
    }

    return rtn;
}



int main() {
    return (0);
}
