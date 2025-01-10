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

void http_get_reply(int socketID, const char *content) {
    if (!content) {
        printf("[ERROR]: content wasn't found\n");
        return;
    }

    write_len_to_socket(socketID, "HTTP/1.1 200 OK");
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

/**
 * This function extracts the query information before sending them to cgi bin
 */

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


void run_cgi(int socketID, const char *currentDIR, const char *cgipath) {
    char *full_path = NULL, *param = NULL, *result = NULL;
    request_pair req;
    FILE *file;

    if (!currentDIR || !cgipath) {
        printf("[ERROR]: Problem with the paths\n");
        return;
    }

    req = extract_query(cgipath);

    if (req.query) {
        param = malloc(strlen(req.query) + 100);
        sprintf(param, "QUERY_STRING=%s\n", req.query);
    }
    else {
        param = strdup(" ");
    }

    if (ends_with(req.path, ".py")) {
        full_path = concat4(param, "python ", currentDIR, req.path);
    } else {
        full_path = concat3(param, currentDIR, req.path);
    }

    free(param);
    free(req.path);
    free(req.query);

    printf("Executing: [%s]...\n", full_path);

    file = popen(full_path, "r");
    free(full_path);

    if (!file) {
        perror("[ERROR]: popen can't function\n");
        http_404_reply(socketID);
    }
    else {
        result = read_file(file);
        http_get_reply(socketID, result);
        free(result);
    }
}

void output_static_file(int socketID, const char *currentDIR, const char *path){
    char *full_path = NULL, *result = NULL;
    FILE *file;

    if (!currentDIR || !path) {
        printf("[ERROR: problem with path\n");
        return;
    }

    full_path = malloc(strlen(currentDIR) + strlen(path) + 1);
    strcpy(full_path, currentDIR);
    strcat(full_path, path);

    printf("Opening static file: [%s]\n", full_path);

    file = fopen(full_path, "r");
    if (!file) {
        printf("[ERROR]: error with file\n");
        http_404_reply(socketID);
    }
    else {
        result = read_file(file);
        http_get_reply(socketID, result);
        free(result);
    }
}

void *handle_socket_thread(void *socketID_arg) {
    int socketID = *((int *)socketID_arg);
    char *text = NULL;
    char currentDIR[MAX_CWD];
    char *path = NULL;

    printf("Handling socket: %d\n", socketID);

    text = read_text_from_socket(socketID);
    printf("From socket %s\n\n", text);

    if (is_get(text)) {
        if (!getcwd(currentDIR, MAX_CWD)) {
            printf("[ERROR]: can't access current directory");
            return (NULL);
        }

        path = get_path(text);

        if (is_cgibin_request(path)) {
            run_cgi(socketID, currentDIR, path);
        } else {
            printf("cwd[%s]\n", currentDIR);
            printf("path[%s]\n", path);
            output_static_file(socketID, currentDIR, path);
        }

        free(path);
    } else {
        http_404_reply(socketID);
    }

    free(text);
    close(socketID);
    free(socketID_arg);

    return (NULL);
}

int create_listen_socket() {
    int socketID = socket(AF_INET, SOCK_STREAM, 0);
    int setopt = 1;
    struct sockaddr_in server_address;
    uint16_t port = 8000;

    if (socketID < 0) {
        printf("[ERROR]: problem with opening socket\n");
        return (-1);
    }

    if (-1 == setsockopt(socketID, SOL_SOCKET, SO_REUSEADDR, \
    (char *)&setopt, sizeof(setopt))) {
        printf("[ERROR]: problem while socket options\n");
        return (-1);
    }

    while (1) {
        bzero(&server_address, sizeof(server_address));
        server_address.sin_family = AF_INET;
        server_address.sin_port = htons(port);
        server_address.sin_addr.s_addr = htonl(INADDR_ANY);


        if (bind(socketID, (struct sockaddr_in *)&server_address, sizeof(server_address)) < 0) {
            port++;
        } else {
            break;
        }
    }

    if (listen(socketID, SOMAXCONN) < 0)
        error("Couldn't listen");

    printf("Running on port %d\n", port);

    return (socketID);
}

int main() {
    int socketID = create_listen_socket();
    struct sockaddr_in client_address;
    int cli_len = sizeof(client_address);
    struct thread_pool *pool = pool_init(4);
    int newsocketID;
    int *arg;

    while (1) {
        newsocketID = accept(socketID, (struct sockaddr_in *)&client_address,\
        (socklen_t *)&cli_len);

        if (newsocketID < 0)
            error("Error on accept\n");

        printf("New socket: %d\n", newsocketID);

        arg = malloc(sizeof(int));
        arg = newsocketID;
        pool_add_task(pool, handle_socket_thread, arg);
    }

    close(socketID);
    return (0);
}
