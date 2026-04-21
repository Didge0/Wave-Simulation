#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <time.h>
#include <Windows.h>
#include "vector.h"
#include <crtdbg.h>
#include "waveSim.h"


#define OPTI_SELECTED 0      //0:Scalaire, 1:SIMD

#define RESOLUTION_WIDTH  3u
#define RESOLUTION_HEIGHT 3u
#define WIDTH 900
#define HEIGHT 600
#define FREQUENCY 24

#define BENCH_FRAMES 5000
#define BENCH_WARMUP 200
#define BENCH_SEED 12345

int width;
int height;

bool init_projet_global_data(){
      width=WIDTH;
      height = HEIGHT;

      if(!init_wave_simulation(width, height)){
            return false;
      }

      return true;
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

            LARGE_INTEGER a, b;
            QueryPerformanceCounter(&a);

#if OPTI_SELECTED == 0
            calculate_buffer(t, RESOLUTION_HEIGHT, RESOLUTION_WIDTH);
#else
            calculate_buffer_SIMD(t, RESOLUTION_HEIGHT, RESOLUTION_WIDTH);
#endif
            update_vector(t);
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