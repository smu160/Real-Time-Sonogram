#include <stdlib.h> // Needed for: srand, rand
#include <iostream>
#include <math.h>
#include <unistd.h> // Needed for: usleep

#include <SDL2/SDL.h>

#include <fstream>
#include <vector>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500
#define TEX_HEIGHT 600
#define TEX_WIDTH 500


struct tuple {
    int x;
    int y;
};


SDL_Point polar_to_cart(double r, double theta) {
    SDL_Point point;
    point.x = r * cos(theta);
    point.y = r * sin(theta);
    return point;
}


SDL_Point cart_to_screen(SDL_Point cart_pt) {
    cart_pt.x = cart_pt.x + SCREEN_WIDTH/2;
    cart_pt.y = SCREEN_HEIGHT/2 - cart_pt.y;
    return cart_pt;
}


/* Draw a line from the start point, `p1` to the end point, `p2`
 * Renders the line by drawing pixels from `p1` to `p2`
 */
void draw_line(SDL_Point p1, SDL_Point p2, std::vector<tuple> &vect,
               std::vector<unsigned char> &pix, int s, int e) {

    int x0 = p1.x;
    int y0 = p1.y;
    int x1 = p2.x;
    int y1 = p2.y;

    int dx = abs(x1-x0);
    int sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0);
    int sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy)/2;
    int e2;

    int color;
    tuple temp;

    while (1) {
        if (s <= e) {
            temp = vect[s++];
            color = temp.y;
        }
        else {
            color = 40;
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


int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file-name>" << std::endl;
        return -1;
    }

    // TODO: Switch to live streaming this data

    // Get the name of the data file to work with
    char* filename = argv[1];

    // Initialize important constants
    double const PI = 3.145926;
    double const X_ORIGIN = SCREEN_WIDTH / 2;
    double const Y_ORIGIN = 0;
    double const LEFT_LIMIT = 7*PI / 6;
    double const RIGHT_LIMIT = 11*PI / 6;

    std::ifstream file(filename);
    std::vector<tuple> vector_of_tuples;

    // Put all file data into the vector
    if (file.is_open()) {
        tuple temp;

        while (file) {
            file >> temp.x >> temp.y;

            if (file) {
                vector_of_tuples.push_back(temp);
            }
            else {
                file.close();
            }
        }
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_UNDEFINED,
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

    SDL_Point start_pt;
    start_pt.x = X_ORIGIN;
    start_pt.y = Y_ORIGIN;

    SDL_Point end_pt;

    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderPresent(renderer);

    tuple temp;
    int prev_tx = 0;
    SDL_Event event;
    std::vector<unsigned char> pixels(TEX_WIDTH * TEX_HEIGHT * 4, 0);

    while (running) {

        // Begin performance timer
        const Uint64 start = SDL_GetPerformanceCounter();

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);

        for (int i = 0; i < vector_of_tuples.size(); i++) {
            temp = vector_of_tuples[i];

            // Tx data point found! Draw a line
            if (temp.x == 0 && i > 0) {
                double theta = random_angle(LEFT_LIMIT, RIGHT_LIMIT);
                int distance = 150;
                end_pt = polar_to_cart(distance, theta);
                end_pt = cart_to_screen(end_pt);
                draw_line(start_pt, end_pt, vector_of_tuples, pixels, prev_tx, i);
                prev_tx = i;
            }

        }

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = SDL_FALSE;
            }
        }

        SDL_UpdateTexture(texture, NULL, &pixels[0], TEX_WIDTH*4);
        SDL_RenderCopy( renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        //              *** Performance Eval ***
        const Uint64 end = SDL_GetPerformanceCounter();
        const static Uint64 freq = SDL_GetPerformanceFrequency();
        const double seconds = ( end - start ) / static_cast< double >( freq );
        std::cout << "Frame time: " << seconds * 1000.0 << "ms" << std::endl;
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
