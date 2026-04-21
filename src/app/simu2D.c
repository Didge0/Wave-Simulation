
#define MODE_SELECTED 1       //0:BenchMode, 1:WaveSim2D
#define OPTI_SELECTED 1       //0:Scalaire, 1:SIMD

#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <time.h>
#include <Windows.h>
#include "vector.h"
#include <crtdbg.h>
#include <SDL3/SDL.h>
#include "waveSim.h"
#include "SDLWaveSim.h"

#define RESOLUTION_WIDTH  3u
#define RESOLUTION_HEIGHT 3u
#define WIDTH 900
#define HEIGHT 600
#define FREQUENCY 24

#define NB_DATA 2

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
#if OPTI_SELECTED==1
      float* buffer = calculate_buffer_SIMD(t, RESOLUTION_HEIGHT, RESOLUTION_WIDTH);
      calculate_texture_SIMD(buffer, width, height);
#else
      float* buffer = calculate_buffer(t, RESOLUTION_HEIGHT, RESOLUTION_WIDTH);
      calculate_texture(buffer, width, height);
#endif
}

void rand_wave(float t, unsigned int frequency, unsigned int nb_par_second){
      if(rand()%(frequency/nb_par_second) == 0){
            int x = rand() % width;
            int y = rand() % height;
            add_wave((float)x, (float)y, t);
      }
}

int main(){

      srand((unsigned)time(NULL));


      if(!init_projet_global_data()){
            return -1;
      }
      bool is_opened = true;

       
      float x_click, y_click;

      float t=0;
      float dt = 1/(float)FREQUENCY;
      unsigned int nb_par_second = 2;
      unsigned int data_selected = 0;
      char data[1024];
      unsigned int frequency = FREQUENCY;
      char* data_map[NB_DATA] = {"Freq", "Wave"};

      while(is_opened){
            SDL_Event event;

            while(SDL_PollEvent(&event)){
                  if (event.type == SDL_EVENT_QUIT) {
                        is_opened = false;
                  }else if( event.type == SDL_EVENT_KEY_DOWN){
                        SDL_Keycode key = event.key.key;
                        if(key == SDLK_ESCAPE){
                              is_opened = false;
                        }else if(event.key.key == SDLK_SPACE){
                              data_selected++;
                              data_selected%=NB_DATA;
                        }else if(event.key.key == SDLK_KP_PLUS){
                              switch(data_selected){
                              case 0:
                                    frequency++;
                                    dt = 1/(float)frequency;
                                    update_frequency(frequency);
                                    break;
                              case 1:
                                    nb_par_second++;
                                    break;
                              default:
                                    frequency++;
                                    dt = 1/(float)frequency;
                                    update_frequency(frequency);
                                    break;
                              }
                        }else if(event.key.key == SDLK_KP_MINUS){
                              switch(data_selected){
                              case 0:
                                    frequency = (frequency == 1)?frequency:frequency-1;
                                    dt = 1/(float)frequency;
                                    update_frequency(frequency);
                                    break;
                              case 1:
                                    nb_par_second = (nb_par_second==0)?nb_par_second: nb_par_second-1;
                                    break;
                              default:
                                    frequency = (frequency == 1)?frequency:frequency-1;
                                    update_frequency(frequency);
                                    break;
                              }
                        }
                  }else if(event.type == SDL_EVENT_MOUSE_BUTTON_DOWN){
                        if(event.button.button == SDL_BUTTON_LEFT){
                              SDL_GetMouseState(&x_click, &y_click);
                              add_wave(x_click, y_click, t);
                        }
                  }
            }
            draw_screen(t);
            dt = 1/get_real_frequency();
            t+=dt;
            update_vector(t);
            if(nb_par_second > 0)
                  rand_wave(t, frequency, nb_par_second);
            
            sprintf(data, "%.1fs, %lluWave, %uWave/s\n, %uHz -> %s", t, getNbWave(), nb_par_second, frequency, data_map[data_selected]);
            render_WaveSim(data);
      }
      end_SLD_WaveSim();
      end_simulation();
      if (_CrtDumpMemoryLeaks()) {
            printf("Fuites memoire !\n");
      }

      return 0;
}