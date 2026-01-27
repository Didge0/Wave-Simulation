#include <stdint.h>
#include "immintrin.h"
#include <stdbool.h>

typedef struct {
      float start_t;
      int x,y;
}Wave_data;

bool init_wave_simulation(int _width, int _height);

void end_simulation();

float u_r0(float r);
uint32_t valueToRGB(float u);
float u_fct_gene(float r, float t, float inv_r);

__m256i valueToRGB_SIMD(__m256 u);

void add_wave(float x_click, float y_click, float t);

float u_fct_gene_map(float r, float t, float inv_r);

float* calculate_buffer(float t, const unsigned int res_h, const unsigned int res_w);

void update_vector(float t, const float life_time);

size_t getNbWave();