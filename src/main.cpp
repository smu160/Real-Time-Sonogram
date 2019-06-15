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
#include <cstring>

#include <SDL2/SDL.h>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500
#define TEX_HEIGHT 500
#define TEX_WIDTH 500
#define PI 3.145926
#define LEFT_LIMIT 7*PI / 6
#define RIGHT_LIMIT 11*PI / 6


struct data_pt {
    int index;
    int intensity;
};


void die(const char* msg) {
    perror(msg);
    exit(1);
}


void* run_server(void* queue_void) {
    const char* socket_path = "./socket";
    struct sockaddr_un addr;
    char buf[4096];
    int fd;
    int client_sock;

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

    if (listen(fd, 1) == -1) {
        die("listen error");
    }

    if ((client_sock = accept(fd, NULL, NULL)) == -1) {
        die("accept error");
    }

    std::string cpp_str;;
    ssize_t bytes_read;
    std::string delimiter = " ";
    SafeQueue<data_pt>& queue = *static_cast<SafeQueue<data_pt>*> (queue_void);

    while ((bytes_read = recv(client_sock, buf, sizeof(buf)-1, 0)) > 0) {
        buf[bytes_read] = '\0';
        char* ptr = strtok(buf, "|");

        while (ptr != NULL) {
            cpp_str = std::string(ptr);
            ptr = strtok(NULL, "|");

            int mod = 0;
            size_t pos = 0;

            std::string token;
            data_pt temp_data_pt;

            while ((pos = cpp_str.find(delimiter)) != std::string::npos) {
                token = cpp_str.substr(0, pos);

                if (!token.empty()) {
                    if (mod == 0) {
                        temp_data_pt.index = stoi(token);
                        mod++;
                    }
                    else if (mod == 1) {
                        temp_data_pt.intensity = stoi(token);
                        queue.push(temp_data_pt);
                        mod++;
                    }
                }

                cpp_str.erase(0, pos + delimiter.length());
            }
        }

        if (bytes_read == -1) {
            die("read");
        }
        else if (bytes_read == 0) {
            std::cerr << "EOF" << std::endl;
            close(client_sock);
            break;
        }

    }

    return NULL;
}


SDL_Point polar_to_cart(double r, double theta) {
    SDL_Point point;
    point.x = r * cos(theta);
    point.y = r * sin(theta);
    return point;
}


SDL_Point cart_to_screen(SDL_Point cart_pt) {
    cart_pt.x += SCREEN_WIDTH/2;
    cart_pt.y = SCREEN_HEIGHT/2 - cart_pt.y;
    return cart_pt;
}


/* Draw a line from the start point, `p1` to the end point, `p2`
 * Renders the line by drawing pixels from `p1` to `p2`
 */
void draw_line(SDL_Point p1, SDL_Point p2, std::vector<data_pt> &vect,
               std::vector<unsigned char> &pix, int s, int e) {

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
    data_pt temp;

    while (1) {
        if (s <= e) {
            temp = vect[s++];
            color = temp.intensity;
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


double random_angle(double alpha, double beta) {
    double random = ((double) rand()) / (double) RAND_MAX;
    double diff = beta - alpha;
    double r = random * diff;
    return alpha + r;
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
                 std::vector<data_pt> &data_vec, SDL_Point &start_pt,
                 SDL_Point &end_pt, int &prev_tx) {

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    double theta = random_angle(LEFT_LIMIT, RIGHT_LIMIT);
    int distance = 150;
    end_pt = polar_to_cart(distance, theta);
    end_pt = cart_to_screen(end_pt);
    draw_line(start_pt, end_pt, data_vec, pixels, 0, data_vec.size()-10);

    SDL_UpdateTexture(texture, NULL, &pixels[0], TEX_WIDTH*4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}


int main(int argc, char** argv) {

    SafeQueue<data_pt> queue;
    std::vector<data_pt> data_vec;

    pthread_t server_thread;
    int result = pthread_create(&server_thread, NULL, run_server, (void*)&queue);

    if (result < 0) {
        std::cerr << "Could not create server thread!" << std::endl;
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

    SDL_Point end_pt;
    SDL_bool running = SDL_TRUE;

    int prev_tx = 0;
    std::vector<unsigned char> pixels(TEX_WIDTH * TEX_HEIGHT * 4, 0);

    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderPresent(renderer);

    bool first_tx = false;
    bool second_tx = false;

    while (running) {

        // Begin performance timer
        // const Uint64 start = SDL_GetPerformanceCounter();

        // Process incoming events
        process_events(running);

        // Put queue items into data vector
        while (1) {
            data_pt temp_data_pt;
            bool result = queue.pop_front(temp_data_pt);

            if (!result) {
                break;
            }

            data_vec.push_back(temp_data_pt);

            if (!first_tx && temp_data_pt.index == 0) {
                first_tx = true;
            }
            else if (first_tx && temp_data_pt.index == 0) {
                second_tx = true;
                break;
            }
        }

        // Draw the screen if both intervals were found
        if (first_tx && second_tx) {
            std::cout << data_vec.size() << std::endl;

            draw_screen(renderer, texture, pixels, data_vec,
                        start_pt, end_pt, prev_tx);

            first_tx = false;
            second_tx = false;
            data_vec.clear();
        }

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
