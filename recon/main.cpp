#include <stdlib.h> // Needed for: srand, rand
#include <iostream>
#include <math.h>
#include <unistd.h> // Needed for: usleep

#include <SDL2/SDL.h>

#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500


SDL_Point polar_to_cart(double r, double theta) {
    SDL_Point point;
    point.x = r * cos(theta);
    point.y = r * sin(theta);
    return point;
}


/* Draw a line from the start point, `p1` to the end point, `p2`
 * Renders the line by drawing pixels from `p1` to `p2`
 */
void draw_line(SDL_Renderer* renderer, SDL_Point p1, SDL_Point p2) {
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

    while (1) {
        color = rand() % 255;

        SDL_SetRenderDrawColor(renderer, color, color, color, 255);
        SDL_RenderDrawPoint(renderer, x0, y0);

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


SDL_Point cart_to_screen(SDL_Point cart_pt) {
    SDL_Point screen_pt;
    screen_pt.x = cart_pt.x + SCREEN_WIDTH/2;
    screen_pt.y = SCREEN_HEIGHT/2 - cart_pt.y;
    return screen_pt;
    }


double random_angle(double alpha, double beta) {
    double random = ((double) rand()) / (double) RAND_MAX;
    double diff = beta - alpha;
    double r = random * diff;
    return alpha + r;
}


int main(int argc, char** argv) {
    double const PI = 3.145926;
    double const X_ORIGIN = SCREEN_WIDTH / 2;
    double const Y_ORIGIN = 0;
    double const LEFT_LIMIT = 7*PI / 6;
    double const RIGHT_LIMIT = 11*PI / 6;

    if (SDL_Init(SDL_INIT_VIDEO) == 0) {
        SDL_Window* window = NULL;
        SDL_Renderer* renderer = NULL;

        if (SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer) == 0) {
            SDL_bool done = SDL_FALSE;

            SDL_Point start_point;
            start_point.x = X_ORIGIN;
            start_point.y = Y_ORIGIN;

            SDL_Point end_point;

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
            SDL_RenderClear(renderer);

            while (!done) {
                SDL_Event event;

                for (int i = 0; i < 50; i++) {
                    double theta = random_angle(LEFT_LIMIT, RIGHT_LIMIT);
                    SDL_Point cart_point = polar_to_cart(250, theta);
                    end_point = cart_to_screen(cart_point);

                    // std::cout << "Recvd cartesian pt: "<< "(" << cart_point.x << ", " << cart_point.y << ") --> ";
                    // std::cout << "as screen pt: "<< "(" << end_point.x << ", " << end_point.y << ")" << std::endl;
                    draw_line(renderer, start_point, end_point);
                    SDL_RenderPresent(renderer);
                }

                SDL_RenderPresent(renderer);
                usleep(10000);


                while (SDL_PollEvent(&event)) {
                    if (event.type == SDL_QUIT) {
                        done = SDL_TRUE;
                    }
                }
            }
        }

        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }

    }

    SDL_Quit();
    return 0;
}
