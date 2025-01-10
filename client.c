#include "included.h"

char *read_text_from_socket(int sockfd)
{
    const int BUF_SIZE = 256;
    char *buffer = malloc(BUF_SIZE);

    char *result = malloc(1);
    result[0] = '\0';

    int n;
    while (1)
    {
        int n = read(sockfd, buffer, BUF_SIZE - 1);
        if (n < 0)
        {
            error("Error reading from socket");
        }
        buffer[n] = '\0';
        char *last_result = result;
        result = strappend(last_result, buffer);
        free(last_result);
        if (n < BUF_SIZE - 1)
        {
            break;
        }
    }

    free(buffer);

    return result;
}

void write_to_socket(int sockfd, const char *message)
{
    if (write(sockfd, message, strlen(message)) == -1)
    {
        error("write_to_socket");
    }
}

int main(int argc, char **argv) {
    char *url = NULL;
    int socketID;
    uint16_t port;
    struct sockaddr_in server_address;
    char *get_str = NULL, *result = NULL;

    if (argc != 2) {
        printf("Syntax: client <url>\n");
        exit(1);
    }

    url = argv[1];
    socketID = socket(AF_INET, SOCK_STREAM, 0);
    port = 8000;

    if (socketID == -1) {
        error("[ERROR]: a problem with socket\n");
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr = htonl(INADDR_LOOPBACK);


    if (connect(socketID, (struct sockaddr_in *)&server_address, \
    sizeof(struct sockaddr_in)) == -1) {
        error("[ERROR]: problem while connection");
    }

    get_str = malloc(128 + strlen(url));

    sprintf(get_str, "GET %s HTTP/1.1", url);
    printf("\n%s", get_str);
    write_to_socket(socketID, get_str);

    result = read_text_from_socket(socketID);
    printf("From socket: %s\n\n", result);
    free(result);

    close(socketID);
    return (0);
}
