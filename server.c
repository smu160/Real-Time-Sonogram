#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>


/* receive data from a client socket */
void handle(int new_sock) {
    ssize_t bytes_read;
    const int BUFF_SIZE = 4096;
    char buffer[BUFF_SIZE];
    char delim[] = "|";

    while ((bytes_read = recv(new_sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = strtok(buffer, delim);

        while(ptr != NULL) {
            printf("%s\n", ptr);
            ptr = strtok(NULL, delim);
        }
    }
    fprintf(stderr, "Closing connection to client\n");
    close(new_sock);
}


int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server-port>\n", argv[0]);
        exit(1);
    }

    int sock; // Socket file descriptor
    char* port = argv[1]; // Port to listen for clients on
    struct addrinfo hints;
    struct addrinfo* res;
    int reuseaddr = 1; // Set to true

    // Get the address info
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(NULL, port, &hints, &res) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    // Create the socket
    sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1) {
        perror("socket");
        return 1;
    }

    // Enable the socket to reuse the address
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
        perror("setsockopt");
        return 1;
    }

    // Bind to the address
    if (bind(sock, res->ai_addr, res->ai_addrlen) == -1) {
        perror("bind");
        return 1;
    }

    // Listen for clients
    if (listen(sock, 5) == -1) {
        perror("listen");
        return 1;
    }

    freeaddrinfo(res);

    socklen_t size;
    struct sockaddr_in client_addr;

    // Main loop
    while (1) {
        size = sizeof(client_addr);
        int new_sock = accept(sock, (struct sockaddr*) &client_addr, &size);

        if (new_sock == -1) {
            perror("accept");
        }
        else {
            printf("Client connected from %s on port %d\n",
                    inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));
            handle(new_sock);
        }
    }

    close(sock);

    return 0;
}
