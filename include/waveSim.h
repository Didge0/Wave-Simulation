#include <stdint.h>
#include "immintrin.h"
#include <stdbool.h>

typedef struct {
      float start_t;
      int x,y;
}Wave_data;

bool init_wave_simulation(int _width, int _height);

void end_simulation();

void add_wave(float x_click, float y_click, float t);

float* calculate_buffer(float t, const unsigned int res_h, const unsigned int res_w);

float* calculate_buffer_SIMD(float t, const unsigned int res_h, const unsigned int res_w);

void update_vector(float t, const float life_time);

size_t getNbWave();