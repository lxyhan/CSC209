#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>

#ifndef PORT
#define PORT 30000
#endif
#define BUF_SIZE 128

int main(void)
{
    // Create socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("client: socket");
        exit(1);
    }

    // Set up server address
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) < 1)
    {
        perror("client: inet_pton");
        close(sock_fd);
        exit(1);
    }

    // Connect to server
    if (connect(sock_fd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("client: connect");
        close(sock_fd);
        exit(1);
    }

    // Send username to server
    char buf[2 * BUF_SIZE + 2]; // Extra space for username prefixes
    printf("Please enter a username: ");
    fflush(stdout);

    int num_read = read(STDIN_FILENO, buf, BUF_SIZE);
    if (num_read == 0)
    {
        close(sock_fd);
        exit(0);
    }

    if (write(sock_fd, buf, num_read) != num_read)
    {
        perror("client: write");
        close(sock_fd);
        exit(1);
    }

    // Set up for select() - monitor both stdin and socket
    fd_set all_fds;
    FD_ZERO(&all_fds);
    FD_SET(STDIN_FILENO, &all_fds);
    FD_SET(sock_fd, &all_fds);

    int max_fd = (sock_fd > STDIN_FILENO) ? sock_fd : STDIN_FILENO;

    while (1)
    {
        fd_set read_fds = all_fds;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1)
        {
            perror("client: select");
            exit(1);
        }

        // Handle user input
        if (FD_ISSET(STDIN_FILENO, &read_fds))
        {
            num_read = read(STDIN_FILENO, buf, BUF_SIZE);
            if (num_read == 0)
            {
                break; // EOF on stdin
            }

            if (write(sock_fd, buf, num_read) != num_read)
            {
                perror("client: write");
                close(sock_fd);
                exit(1);
            }
        }

        // Handle server messages
        if (FD_ISSET(sock_fd, &read_fds))
        {
            num_read = read(sock_fd, buf, sizeof(buf) - 1);
            if (num_read == 0)
            {
                printf("Server closed connection\n");
                break;
            }

            buf[num_read] = '\0';
            printf("[Server] %s", buf);
        }
    }

    close(sock_fd);
    return 0;
}