#include <stdio.h>
#include <math.h>
#include <Windows.h>
#include <assert.h>
#include "Lib/vector.h"
#include <crtdbg.h>
#include <SDL3/SDL.h>
#include "waveSim.h"
#include "SDLWaveSim.h"


#define LIFE_TIME 4.0f

#define RESOLUTION_WIDTH  3u
#define RESOLUTION_HEIGHT 3u
#define WIDTH 900
#define HEIGHT 600
#define FREQUENCY 24



#define BENCH_MODE 1
#define BENCH_SEED 12345

int width;
int height;


bool init_projet_global_data(){
      width=WIDTH;
      height = HEIGHT;
      
      if(!init_SDL_WaveSim(&width, &height, FREQUENCY)){
            return false;
      }
      if(!init_wave_simulation(width, height)){
            return false;
      }

      return true;
}

void draw_screen(float t){
      float* buffer = calculate_buffer(t, RESOLUTION_HEIGHT, RESOLUTION_WIDTH);
      calculate_texture_SIMD(buffer, width, height, valueToRGB_SIMD);
}

void rand_wave(float t){
      if(rand()%16 == 0){
            int x = rand() % width;
            int y = rand() % height;
            add_wave((float)x, (float)y, t);
      }
}

int main(){
      #if BENCH_MODE
      srand(BENCH_SEED);
      #else
      srand((unsigned)time(NULL));
      #endif

      if(!init_projet_global_data()){
            return -1;
      }
      bool is_opened = true;

       
      float x_click, y_click;

      float t=0;

      while(is_opened){
            SDL_Event event;

            while(SDL_PollEvent(&event)){
                  if (event.type == SDL_EVENT_QUIT) {
                        is_opened = false;
                  }else if( event.type == SDL_EVENT_KEY_DOWN){
                        SDL_Keycode key = event.key.key;
                        if(key == SDLK_ESCAPE){
                              is_opened = false;
                        }
                  }else if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN){
                        if(event.button.button == SDL_BUTTON_LEFT){
                              SDL_GetMouseState(&x_click, &y_click);
                              add_wave(x_click, y_click, t);
                        }
                  }
            }
            draw_screen(t);
            t+=1/(float)FREQUENCY;
            update_vector(t,LIFE_TIME);
            rand_wave(t);
            
            render_WaveSim(t, getNbWave(), FREQUENCY);
      }
      end_SLD_WaveSim();
      end_simulation();
      if (_CrtDumpMemoryLeaks()) {
            printf("Fuites memoire !\n");
      }

      return 0;
}