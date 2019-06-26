//
//  Author: Saveliy Yusufov
//  Date: 9 June 2019
//
//  Copyright © 2019 Creative Machines Lab. All rights reserved.
//

#include <iostream> // cout, cerr
#include <math.h>

#include <pthread.h>
#include <vector>
#include <mutex>
#include "safe_queue.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500
#define TEX_HEIGHT 500
#define TEX_WIDTH 500


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


// TODO: Move all this server crap to a separate file

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

        while ((token = strtok_r(rest, ",", &rest))) {
            if (strcmp(token, interval_delimeter) == 0) {
                is_angle = true;
                queue.push(temp_tx_interval);
                temp_tx_interval.intensities.clear();
                // SDL_Delay(1);
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

    if (bytes_read == -1) {
        die("read");
    }
    else if (bytes_read == 0) {
        std::cerr << "EOF" << std::endl;
        close(client_sock);
    }

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

    if (listen(fd, backlog) == -1) {
        die("listen failed");
    }

    while (1) {
        int client_sock = accept(fd, NULL, NULL);

        if (client_sock == -1) {
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
 * Initialize a server that listens for a single client on a Unix Domain Socket
 */
void* start_server(void* queue_void) {
    const char* socket_path = "./socket";
    struct sockaddr_un addr;
    int fd;

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        die("socket error");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    unlink(socket_path);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        die("bind error");
    }

    SafeQueue<tx_interval>& queue = *static_cast<SafeQueue<tx_interval>*> (queue_void);

    handle_st my_handle_st;
    my_handle_st.sock = &fd;
    my_handle_st.safe_q = &queue;

    pthread_t t1;
    int result = pthread_create(&t1, NULL, listener, (void*)&my_handle_st);

    if (result < 0) {
        die("Thread could not be created...");
    }

    pthread_join(t1, NULL);

    return NULL;
}


inline SDL_Point polar_to_cart(double r, double theta) {
    SDL_Point point;
    point.x = r * cos(theta);
    point.y = r * sin(theta);
    return point;
}


inline SDL_Point cart_to_screen(SDL_Point cart_pt) {
    cart_pt.x += SCREEN_WIDTH/2;
    cart_pt.y = SCREEN_HEIGHT/2 - cart_pt.y;
    return cart_pt;
}


/* Draw a line from the start point, `p1` to the end point, `p2`
 * Renders the line by drawing pixels from `p1` to `p2`
 */
inline void draw_line(SDL_Point p1, SDL_Point p2, tx_interval& tx_itval,
                      std::vector<unsigned char> &pix, std::mutex& m) {

    int x0 = p1.x;
    int y0 = p1.y;
    int x1 = p2.x;
    int y1 = p2.y;

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy)/2;
    int e2;

    int color;
    int i = 0;

    m.lock();
    while (1) {
        if (i < tx_itval.intensities.size()) {
            color = tx_itval.intensities[i++];
        }
        else {
            color = 70;
        }

        unsigned int offset = (TEX_WIDTH * 4 * y0) + x0 * 4;

        pix[offset] = color; // b
        pix[offset+1] = color; // g
        pix[offset+2] = color; // r
        pix[offset+3] = SDL_ALPHA_OPAQUE;

        if (x0==x1 && y0==y1) {
            break;
        }

        e2 = err;

        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
    m.unlock();
}


void process_events(SDL_bool &running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = SDL_FALSE;
        }
    }
}


struct dl_args {
    std::vector<unsigned char>* pixels;
    SafeQueue<tx_interval>* queue;
    std::mutex* m;
};


// Helper function to draw as many lines as possible
void* draw_lines(void* dl_args_void) {
    dl_args* draw_lines_args = (struct dl_args*) dl_args_void;
    std::vector<unsigned char>& pixels = *draw_lines_args->pixels;
    SafeQueue<tx_interval>& queue = *draw_lines_args->queue;
    std::mutex& m = *draw_lines_args->m;

    double const X_ORIGIN = SCREEN_WIDTH / 2;
    double const Y_ORIGIN = 0;
    SDL_Point start_pt;
    start_pt.x = X_ORIGIN;
    start_pt.y = Y_ORIGIN;

    bool got_interval;

    while (1) {
        tx_interval temp_tx_interval;

        // Put queue items into data vector
        got_interval = queue.pop_front(temp_tx_interval);
        if (!got_interval) {
            continue;
        }

        // TODO: distance should be based on length of interval
        int distance = 150;

        SDL_Point end_pt = polar_to_cart(distance, temp_tx_interval.angle);
        end_pt = cart_to_screen(end_pt);

        draw_line(start_pt, end_pt, temp_tx_interval, pixels, m);
    }

    return NULL;
}

void draw_screen(SDL_Renderer* renderer, SDL_Texture* texture,
                 std::vector<unsigned char> &pixels, std::mutex& m) {

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    m.lock();
    SDL_UpdateTexture(texture, NULL, &pixels[0], TEX_WIDTH*4);
    m.unlock();

    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


int main(int argc, char** argv) {
    SafeQueue<tx_interval> queue;

    pthread_t t0;
    int result = pthread_create(&t0, NULL, start_server, (void*)&queue);

    if (result < 0) {
        std::cerr << "Could not create server thread!" << std::endl;
        return 1;
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Columbia Open-Source UltraSound",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          600,
                                          600,
                                          SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window,
                                                -1,
                                                SDL_RENDERER_ACCELERATED);

    SDL_Texture* texture = SDL_CreateTexture(renderer,
                                             SDL_PIXELFORMAT_ARGB8888,
                                             SDL_TEXTUREACCESS_STREAMING,
                                             TEX_WIDTH,
                                             TEX_HEIGHT);

    SDL_bool running = SDL_TRUE;

    std::mutex m;
    std::vector<unsigned char> pixels(TEX_WIDTH * TEX_HEIGHT * 4, 0);

    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderPresent(renderer);

    dl_args dl_args_void;
    dl_args_void.pixels = &pixels;
    dl_args_void.queue = &queue;
    dl_args_void.m = &m;

    pthread_t t3;
    result = pthread_create(&t3, NULL, draw_lines, (void*)&dl_args_void);

    while (running) {

        // Begin performance timer
        // const Uint64 start = SDL_GetPerformanceCounter();

        // Process incoming events
        process_events(running);

        draw_screen(renderer, texture, pixels, m);

        /*           *** Performance Eval ***
        const Uint64 end = SDL_GetPerformanceCounter();
        const static Uint64 freq = SDL_GetPerformanceFrequency();
        const double seconds = ( end - start ) / static_cast< double >( freq );
        std::cout << "Frame time: " << seconds * 1000.0 << "ms" << std::endl;
        */
    }

    // TODO: server threads need to be cleaned up here

    if (texture) {
        SDL_DestroyTexture(texture);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }

    SDL_Quit();

    return 0;
}
