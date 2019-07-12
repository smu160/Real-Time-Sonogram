//
//  Server to receive live stream of ADC and piezo angle data
//
//  Copyright © 2019 Saveliy Yusufov <sy2685@columbia.edu>
//
//

#include <iostream>
#include <vector>

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "safe_queue.h"


struct tx_interval {
    double angle;
    std::vector<int> intensities;
};


struct handle_st {
    int* sock;
    SafeQueue<tx_interval>* safe_q;
};


void die(const char* msg) {
    perror(msg);
    exit(1);
}


/*
 * Handles the incoming data stream
 */
void* stream_handler(void* handle_st_void) {
    const int BUFF_SIZE = 65536; // 2^16
    char buffer[BUFF_SIZE];
    const char* interval_delimeter = "|";

    handle_st my_handle_st = *(struct handle_st*) handle_st_void;
    int& client_sock = *my_handle_st.sock;
    SafeQueue<tx_interval>& queue = *my_handle_st.safe_q;

    tx_interval temp_tx_interval;
    ssize_t bytes_read;
    std::cerr << "Receiving stream on client socket: " << client_sock << std::endl;

    while ((bytes_read = recv(client_sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes_read] = '\0';

        char* token;
        char* rest = buffer;
        bool is_angle = false;

        // TODO: figure out the weird angle issue here
        while ((token = strtok_r(rest, ",", &rest))) {
            if (strcmp(token, interval_delimeter) == 0) {
                is_angle = true;
                queue.push(temp_tx_interval);
                temp_tx_interval.intensities.clear();
                continue;
            }

            if (is_angle) {
                temp_tx_interval.angle = std::stod(token);
                is_angle = false;
            }
            else {
                // TODO: figure out why std::stoi was causing errors
                temp_tx_interval.intensities.push_back(std::stod(token));
            }

        }
    }

    std::cerr << "Closing connection to client" << std::endl;
    close(client_sock);

    return NULL;
}


/*
 * Listens for a client and starts a handler thread
 */
void* listener(void* handle_st_void) {
    std::cerr << "listener thread started..." << std::endl;
    const int backlog = 1;

    handle_st handle_st_1 = *(struct handle_st*) handle_st_void;
    int fd = *handle_st_1.sock;
    SafeQueue<tx_interval>& queue = *handle_st_1.safe_q;

    std::cerr << fd << std::endl;

    if (listen(fd, backlog) < 0) {
        die("listen failed");
    }

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (1) {
        int client_sock = accept(fd, (struct sockaddr*) &client_addr, &client_len);

        if (client_sock < 0) {
            die("accept error");
        }

        handle_st my_handle_st;
        my_handle_st.sock = &client_sock;
        my_handle_st.safe_q = &queue;

        pthread_t t2;
        int result = pthread_create(&t2, NULL, stream_handler, (void*)&my_handle_st);

        if (result < 0) {
            die("Thread could not be created...");
        }
    }

    return NULL;
}


/*
 * Initializes a server and spawn a listener thread
 */
void* start_server(void* queue_void) {
    int server_sock;
    int server_port = 11112; // port for medical imaging IAW DICOM
    struct sockaddr_in serv_addr;

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Any network interface
    serv_addr.sin_port = htons(server_port);

    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        die("socket failed");
    }

    int enable = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0) {
        die("setsockopt(SO_REUSEADDR) failed");
    }

    if (bind(server_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        die("bind error");
    }

    SafeQueue<tx_interval>& queue = *static_cast<SafeQueue<tx_interval>*> (queue_void);

    handle_st my_handle_st;
    my_handle_st.sock = &server_sock;
    my_handle_st.safe_q = &queue;

    pthread_t t1;
    int result = pthread_create(&t1, NULL, listener, (void*)&my_handle_st);

    if (result < 0) {
        die("Thread could not be created...");
    }

    pthread_join(t1, NULL);

    return NULL;
}
