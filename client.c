#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int sockfd;
    int portno;
    char buffer[256];

    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <hostname_or_ip> <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    portno = atoi(argv[2]);
    if (portno <= 0 || portno > 65535) {
        fprintf(stderr, "ERROR: invalid port: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) die("ERROR opening socket");

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host: %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;

    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], (size_t)server->h_length);
    serv_addr.sin_port = htons((uint16_t)portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        die("ERROR connecting");

    printf("Please enter the message: ");
    memset(buffer, 0, sizeof(buffer));
    if (fgets(buffer, (int)sizeof(buffer), stdin) == NULL) {
        fprintf(stderr, "ERROR reading input\n");
        close(sockfd);
        return EXIT_FAILURE;
    }

    ssize_t n = write(sockfd, buffer, (int)strlen(buffer));
    if (n < 0) die("ERROR writing to socket");

    memset(buffer, 0, sizeof(buffer));
    n = read(sockfd, buffer, sizeof(buffer) - 1);
    if (n < 0) die("ERROR reading from socket");

    printf("%s\n", buffer);

    close(sockfd);
    return 0;
}
