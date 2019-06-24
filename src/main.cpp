#include <stdlib.h> // srand, rand
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


void die(const char* msg) {
    perror(msg);
    exit(1);
}


// TODO: Write up this function and test it
/*
 * Initialize a server that listens for a single client on a Unix Domain Socket
 */
int start_server() {
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

    return fd;
}


// TODO: Implement the handler function and test it
/*
 * Handles the incoming data stream
 */
void* handle_data_stream(void* client_sock_void) {
    return NULL;
}


// TODO: Implement the listener function and test it
/*
 * Listens for a client and starts a handler thread
 */
void* listener(void* fd_void) {
    std::cerr << "listener thread started..." << std::endl;

    const int backlog = 1;
    int fd = *(int*) fd_void;

    if (listen(fd, backlog) == -1) {
        die("listen failed");
    }

    while (1) {
        int client_sock = accept(fd, NULL, NULL);

        if (client_sock == -1) {
            die("accept error");
        }

        pthread_t client_thread;
        int result = pthread_create(&client_thread, NULL, handle_data_stream,
                                    (void*)&client_sock);

        if (result < 0) {
            die("Could not create thread");
        }
    }

    return NULL;
}


void* run_server(void* queue_void) {
    int fd = start_server();

    if (listen(fd, 1) == -1) {
        die("listen error");
    }

    int client_sock;
    if ((client_sock = accept(fd, NULL, NULL)) == -1) {
        die("accept error");
    }

    SafeQueue<tx_interval>& queue = *static_cast<SafeQueue<tx_interval>*> (queue_void);

    char buffer[65536];
    const char* interval_delimeter = "|";

    tx_interval temp_tx_interval;
    ssize_t bytes_read;

    while ((bytes_read = recv(client_sock, buffer, sizeof(buffer)-1, 0)) > 0) {
        buffer[bytes_read] = '\0';

        char* token;
        char* rest = buffer;

        bool is_angle = false;

        while ((token = strtok_r(rest, ",", &rest))) {
            if (strcmp(token, interval_delimeter) == 0) {
                is_angle = true;
                queue.push(temp_tx_interval);
                // std::cerr << queue.size() << std::endl;
                temp_tx_interval.intensities.clear();
                SDL_Delay(1);
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
                      std::vector<unsigned char> &pix) {

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
}


void process_events(SDL_bool &running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            running = SDL_FALSE;
        }
    }
}


void draw_screen(SDL_Renderer* renderer, SDL_Texture* texture,
                 std::vector<unsigned char> &pixels,
                 tx_interval& tx_itval, SDL_Point &start_pt) {

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // double theta = random_angle(7*M_PI/6, 11*M_PI/6);
    int distance = 150;
    SDL_Point end_pt = polar_to_cart(distance, tx_itval.angle);
    end_pt = cart_to_screen(end_pt);
    draw_line(start_pt, end_pt, tx_itval, pixels);

    SDL_UpdateTexture(texture, NULL, &pixels[0], TEX_WIDTH*4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


int main(int argc, char** argv) {
    SafeQueue<tx_interval> queue;

    pthread_t server_thread;
    int result = pthread_create(&server_thread, NULL, run_server, (void*)&queue);

    if (result < 0) {
        std::cerr << "Could not create server thread!" << std::endl;
        return 1;
    }

    // Initialize important constants
    double const X_ORIGIN = SCREEN_WIDTH / 2;
    double const Y_ORIGIN = 0;


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

    SDL_Point start_pt;
    start_pt.x = X_ORIGIN;
    start_pt.y = Y_ORIGIN;

    SDL_bool running = SDL_TRUE;

    std::vector<unsigned char> pixels(TEX_WIDTH * TEX_HEIGHT * 4, 0);

    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderPresent(renderer);

    while (running) {

        // Begin performance timer
        // const Uint64 start = SDL_GetPerformanceCounter();

        // Process incoming events
        process_events(running);

        tx_interval temp_tx_itval;

        // Put queue items into data vector
        bool result = queue.pop_front(temp_tx_itval);
        if (!result) {
            continue;
        }

        // Draw the screen if both intervals were found
        draw_screen(renderer, texture, pixels, temp_tx_itval, start_pt);


        /*           *** Performance Eval ***
        const Uint64 end = SDL_GetPerformanceCounter();
        const static Uint64 freq = SDL_GetPerformanceFrequency();
        const double seconds = ( end - start ) / static_cast< double >( freq );
        std::cout << "Frame time: " << seconds * 1000.0 << "ms" << std::endl;
        */
    }

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
