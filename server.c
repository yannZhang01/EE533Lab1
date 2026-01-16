#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int active_connections = 0;
static int total_connections = 0;

static void die(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static void handle_sigchld(int sig)
{
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

static void handle_client(int client_fd)
{
    char buffer[256];

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
        if (n < 0) {
            perror("ERROR reading from socket");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
        if (n == 0) {
            printf("Client closed connection\n");
            break;
        }

        printf("Here is the message: %s\n", buffer);

        const char *reply = "I got your message";
        n = write(client_fd, reply, (int)strlen(reply));
        if (n < 0) {
            perror("ERROR writing to socket");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
    }

    close(client_fd);
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

    // Handle zombie processes
    signal(SIGCHLD, handle_sigchld);

    while (1) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = (socklen_t)sizeof(cli_addr);

        int client_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (client_fd < 0) {
            perror("ERROR on accept");
            continue;
        }

        active_connections++;
        total_connections++;
        printf("[Connection %d] Client connected - Socket FD: %d, Client IP: %s, Client Port: %d | Active: %d\n", 
               total_connections, client_fd, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), active_connections);

        pid_t pid = fork();
        if (pid < 0) {
            perror("ERROR on fork");
            close(client_fd);
            active_connections--;
            continue;
        }

        if (pid == 0) {
            // Child process
            close(sockfd);  // Child doesn't need the listening socket
            handle_client(client_fd);
            exit(EXIT_SUCCESS);
        } else {
            // Parent process
            close(client_fd);  // Parent doesn't need this connection
            active_connections--;
        }
    }

    close(sockfd);
    return 0;
}
