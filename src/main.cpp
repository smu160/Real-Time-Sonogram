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

#include "server.cpp"
#include "safe_queue.h"

#include <SDL2/SDL.h>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500
#define TEX_HEIGHT 500
#define TEX_WIDTH 500


struct dl_args {
    std::vector<unsigned char>* pixels;
    SafeQueue<tx_interval>* queue;
    std::mutex* m;
};


static inline SDL_Point polar_to_cart(double r, double theta) {
    SDL_Point point;
    point.x = r * cos(theta);
    point.y = r * sin(theta);
    return point;
}


static inline SDL_Point cart_to_screen(SDL_Point cart_pt) {
    cart_pt.x += SCREEN_WIDTH * 0.5;
    cart_pt.y = SCREEN_HEIGHT*0.5 - cart_pt.y;
    return cart_pt;
}


/*
 * Draw a line from the start point, `p1` to the end point, `p2`
 * Renders the line by drawing pixels from `p1` to `p2`
 *
 * Colors each voxel according to the value that was mapped to it.
 * For high values close to 255, the color of the voxel would black and opaque.
 * For low values close to 0, the voxel would be white and transparent.
 * Voxels representing values in between 0 and 255 are in greyscale and
 * have transparency corresponding to its value. (The higher the value, the more
 * opaque rhe voxel.) This allows one to see through the cube of voxels to
 * view the surface.
 */
static inline void draw_line(SDL_Point p1, SDL_Point p2, tx_interval& tx_itval,
                      std::vector<unsigned char>& pix, std::mutex& m) {

    int x0 = p1.x;
    int y0 = p1.y;
    int x1 = p2.x;
    int y1 = p2.y;

    int dx = abs(x1 - x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) * 0.5;
    int e2;

    int color;
    int alpha;
    int value;
    int i = 0;

    m.lock();

    while (1) {
        if (i < tx_itval.intensities.size()) {
            value = tx_itval.intensities[i++];
        }
        else {
            value = 0;
        }

        unsigned int offset = (TEX_WIDTH * 4 * y0) + x0 * 4;

        if (value <= 25) {
            color = 0;
            alpha = SDL_ALPHA_TRANSPARENT;
        }
        else if (value > 199) {
            color = 255;
            alpha = SDL_ALPHA_OPAQUE;
        }
        else {
            color = value;
            alpha = value;
        }

        pix[offset] = color;    // b
        pix[offset+1] = color;  // g
        pix[offset+2] = color;  // r
        pix[offset+3] = alpha;

        if (x0==x1 && y0==y1) {
            m.unlock();
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


// Helper function to draw as many lines as possible
void* draw_lines(void* dl_args_void) {
    dl_args* draw_lines_args = (struct dl_args*) dl_args_void;
    std::vector<unsigned char>* pixels = draw_lines_args->pixels;
    SafeQueue<tx_interval>* queue = draw_lines_args->queue;
    std::mutex* m = draw_lines_args->m;

    double const X_ORIGIN = SCREEN_WIDTH / 2;
    double const Y_ORIGIN = 0;
    SDL_Point start_pt;
    start_pt.x = X_ORIGIN;
    start_pt.y = Y_ORIGIN;

    bool got_interval;
    int distance = 150;
    tx_interval temp_tx_interval;

    while (1) {

        // Put queue items into data vector
        got_interval = (*queue).pop_front(temp_tx_interval);
        if (!got_interval) {
            continue;
        }

        if (temp_tx_interval.angle < 3.67 || temp_tx_interval.angle > 5.76) {
            continue;
        }

        // TODO: distance should be based on length of interval
        SDL_Point end_pt = polar_to_cart(distance, temp_tx_interval.angle);

        end_pt = cart_to_screen(end_pt);

        draw_line(start_pt, end_pt, temp_tx_interval, *pixels, *m);
    }

    return NULL;
}


void draw_screen(SDL_Renderer* renderer, SDL_Texture* texture,
                 std::vector<unsigned char>& pixels, std::mutex& m) {

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
