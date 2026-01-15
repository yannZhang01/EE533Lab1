#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static void handle_client(int client_fd)
{
    char buffer[256];

    memset(buffer, 0, sizeof(buffer));
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        perror("ERROR reading from socket");
        return;
    }

    printf("Here is the message: %s\n", buffer);

    const char *reply = "I got your message";
    n = write(client_fd, reply, (int)strlen(reply));
    if (n < 0) {
        perror("ERROR writing to socket");
        return;
    }
}

int main(int argc, char *argv[])
{
    int sockfd;
    int portno;

    struct sockaddr_in serv_addr;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    portno = atoi(argv[1]);
    if (portno <= 0 || portno > 65535) {
        fprintf(stderr, "ERROR: invalid port: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) die("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons((uint16_t)portno);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        die("ERROR on binding");

    if (listen(sockfd, 5) < 0)
        die("ERROR on listen");

    printf("Server listening on port %d...\n", portno);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = (socklen_t)sizeof(cli_addr);

        int client_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (client_fd < 0) {
            perror("ERROR on accept");
            continue;
        }

        handle_client(client_fd);
        close(client_fd);
    }

    close(sockfd);
    return 0;
}
