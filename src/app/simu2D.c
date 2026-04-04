
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


#define LIFE_TIME 4.0f

#define RESOLUTION_WIDTH  3u
#define RESOLUTION_HEIGHT 3u
#define WIDTH 900
#define HEIGHT 600
#define FREQUENCY 24

#define NB_DATA 2


#define BENCH_FRAMES 5000
#define BENCH_WARMUP 200
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

static inline void progress_print(int current, int total, int bar_width)
{
    if (total <= 0) return;
    if (current < 0) current = 0;
    if (current > total) current = total;

    float ratio = (float)current / (float)total;
    int filled = (int)(ratio * bar_width);

    // \r = retour au début de la ligne (sans newline)
    printf("\r[");

    for (int i = 0; i < bar_width; ++i) {
        putchar(i < filled ? '#' : '-');
    }

    printf("] %3d%% (%d/%d)", (int)(ratio * 100.0f + 0.5f), current, total);
    fflush(stdout);
}

static inline void progress_done(void)
{
    putchar('\n');
    fflush(stdout);
}

#if MODE_SELECTED == 0

int main(void) {
      const float dt = 1.0f / FREQUENCY;
      float t = 0.0f;

      srand(BENCH_SEED);

      if (!init_wave_simulation(WIDTH, HEIGHT)) {
            return 1;
      }

      LARGE_INTEGER fq;
      QueryPerformanceFrequency(&fq);

      double sum = 0.0, min = 1e9, max = 0.0;
      int total = BENCH_FRAMES + BENCH_WARMUP;

      for (int i = 0; i < BENCH_FRAMES + BENCH_WARMUP; i++) {

            // scénario déterministe
            if (rand() % 16 == 0) {
                  add_wave(rand() % WIDTH, rand() % HEIGHT, t);
            }
            update_vector(t, LIFE_TIME);

            LARGE_INTEGER a, b;
            QueryPerformanceCounter(&a);

#if OPTI_SELECTED == 0
            calculate_buffer(t, RESOLUTION_HEIGHT, RESOLUTION_WIDTH);
#else
            calculate_buffer_SIMD(t, RESOLUTION_HEIGHT, RESOLUTION_WIDTH);
#endif
            QueryPerformanceCounter(&b);

            double dt_sec = (double)(b.QuadPart - a.QuadPart) / fq.QuadPart;

            if (i >= BENCH_WARMUP) {
                  sum += dt_sec;
                  if (dt_sec < min) min = dt_sec;
                  if (dt_sec > max) max = dt_sec;
            }

            t += dt;
            if ((i % 50) == 0 || i == total - 1) {
                  progress_print(i + 1, total, 40);
            }
      }
      progress_done();

      double avg = sum / BENCH_FRAMES;
      printf("BENCH calculate_buffer:\n");
      printf("  avg = %.3f ms\n", avg * 1000.0);
      printf("  min = %.3f ms\n", min * 1000.0);
      printf("  max = %.3f ms\n", max * 1000.0);
      printf("  approx FPS = %.1f\n", 1.0 / avg);

      end_simulation();
      if (_CrtDumpMemoryLeaks()) {
            printf("Fuites memoire !\n");
      }
    return 0;
}

#elif MODE_SELECTED == 1
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
            update_vector(t,LIFE_TIME);
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
#endif