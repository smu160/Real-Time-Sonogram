#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


void die(const char* str) {
    perror(str);
    exit(1);
}


/* receive data from a client socket */
void* handle(void* new_sock) {
    ssize_t bytes_read;
    const int BUFF_SIZE = 65536; // 2^16
    char buffer[BUFF_SIZE];
    char delim[] = "|";

    printf("New client handler thread started\n");

    while ((bytes_read = recv(*(int*)new_sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        char *ptr = strtok(buffer, delim);

        while(ptr != NULL) {
            printf("%s\n", ptr);
            ptr = strtok(NULL, delim);
        }
    }

    fprintf(stderr, "Closing connection to client\n");
    close(*(int*)new_sock);

    return NULL;
}


void* listener(void* server_sock) {
    fprintf(stderr, "Listener thread started!\n");

    // Amount of pending connections to queue
    const short backlog = 10;

    // Start listening for incoming connections
    if (listen(*(int*) server_sock, backlog) < 0) {
        die("listen failed");
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        int client_sock = accept(*(int*)server_sock, (struct sockaddr*) &client_addr, &client_len);

        if (client_sock < 0) {
            die("accept failed");
        }

        printf("Connection From: %s:%d (%d)\n",
        inet_ntoa(client_addr.sin_addr), // address as dotted quad
        ntohs(client_addr.sin_port),     // the port in host order
        client_sock);

        pthread_t client_thread;
        int result = pthread_create(&client_thread, NULL, handle, (void*)&client_sock);

        if (result < 0) {
            die("Could not create thread");
        }

    }

    return NULL;
}


int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <server-port>\n", argv[0]);
        exit(1);
    }

    int server_port = atoi(argv[1]);
    struct sockaddr_in serv_addr; // socket interent address of server

    // Construct local address structure
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // any network interface
    serv_addr.sin_port = htons(server_port);

    int server_sock;
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        die("socket failed");
    }

    int enable = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        die("setsockopt(SO_REUSEADDR) failed");
    }

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        die("bind failed");
    }

    // struct arg_struct args;
    pthread_t listener_thread;
    int result = pthread_create(&listener_thread, NULL, listener, (void*) &server_sock);

    if (result < 0) {
        die("Could not create thread");
    }

    pthread_join(listener_thread, NULL);
    close(server_sock);

    return 0;
}
