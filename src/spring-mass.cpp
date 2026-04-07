#include <SDL3/SDL.h>
#include <emscripten.h>

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

void mainloop(){
    //background
    SDL_SetRenderDrawColor(renderer, 30,30,30,255);
    SDL_RenderClear(renderer);


    SDL_SetRenderDrawColor(renderer, 255,80,80,255);
    SDL_FRect rect = {400,300,100,100};
    SDL_RenderFillRect(renderer, &rect);

    SDL_RenderPresent(renderer);
}

int main() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer("sketch",800,600,0,&window,&renderer);

    emscripten_set_main_loop(mainloop,0,1);

}
