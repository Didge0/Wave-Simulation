#include <math.h>
#include "waveSim.h"
#include <stdlib.h>
#include "Lib/vector.h"
#include <assert.h>

#define MULT_COLOR 255.0f
#define C 4

#define U_R0_RESOLUTION 1024
#define R_MAX 2.5f
#define R_MAX_PAS (R_MAX / (float)U_R0_RESOLUTION)

/***** Constance SIMD *****/
__m256 max_val ;
__m256 min_val ;
__m256 mult_color ;
__m256 zero ;
__m256i mask_255;

Vector vect_of_r;

float* u_buffer = NULL;
float* r_map_all;
float* inv_r_map;

float u_r0_table[U_R0_RESOLUTION];

int width;
int height;

bool init_wave_simulation(int _width, int _height){

      width = _width;
      height = _height;

      /***** Init SIMD global vairable *****/
      max_val = _mm256_set1_ps(1.0f);
      min_val = _mm256_set1_ps(-1.0f);
      mult_color = _mm256_set1_ps(MULT_COLOR);
      zero = _mm256_setzero_ps();
      mask_255 = _mm256_set1_epi32(0xFF);

      /***** Initialisation de la map de r pour la première onde *****/
      vect_of_r = vect_init(sizeof(Wave_data));

      r_map_all = malloc(width*2 * height * 2 * sizeof(float));
      inv_r_map = malloc(width * 2 * height * 2 * sizeof(float));
      for(unsigned int x=0; x<width*2; x++){
            for(unsigned y=0; y<height*2; y++){
                  float dx = ((float)x - width)/50.0f;
                  float dy = ((float)y - height)/50.0f;
                  r_map_all[y*width*2 + x] = sqrtf(dx*dx + dy*dy);
                  if(r_map_all[y*width*2 + x] < 0.9f)
                        inv_r_map[y*width*2 + x] = 1;
                  else
                        inv_r_map[y*width*2 + x] = 1/r_map_all[y*width*2 + x];
            }
      }

      for(unsigned int i=0; i<U_R0_RESOLUTION; i++){
            u_r0_table[i] = u_r0(R_MAX_PAS*i);
      }


      /***** Initialisation du buffer  *****/
      u_buffer = malloc(width * height * sizeof(float));
      assert(u_buffer);

      return true;
}

void end_simulation(){
      vect_free(&vect_of_r);
      free(u_buffer);
      free(r_map_all);
      free(inv_r_map);
}

float u_r0(float r){
      if(r > 2.5f) return 0.0f;
      return expf(-r*r);
}

uint32_t valueToRGB(float u){
      if(u > 1.0f){
            return 0xff0000ff;
      }else if( u < -1.0f){
            return 0x0000ffff;
      }
      if(u < 0.0f){
            return ((uint8_t)(-u * MULT_COLOR)<<24) |0x0000ff;
      }else{
            return 0x0000 | ((uint8_t)(u * MULT_COLOR) << 8) | 0xff; 
      }
}

float u_fct_gene(float r, float t, float inv_r){
      return 0.5 * inv_r * ((r-C*t)*u_r0(fabsf(r-C*t)) + (r+C*t)*u_r0(r+C*t));
}

__m256i valueToRGB_SIMD(__m256 u){
      u = _mm256_max_ps(u, min_val);
      u = _mm256_min_ps(u, max_val);
      __m256 mask_neg = _mm256_cmp_ps(u, zero, _CMP_LT_OQ);

      // Calcul des valeurs RGB en fonction du signe de u
      __m256 val = _mm256_blendv_ps(
            _mm256_mul_ps(mult_color, u),
            _mm256_mul_ps(mult_color, _mm256_sub_ps(zero, u)),
            mask_neg
      );

      //Converti en entier
      __m256i val_uint32 = _mm256_cvtps_epi32(val);

      //renvoie la valeur RGB final en fonction du signe de u (mask_neg)
      return _mm256_or_si256(
            _mm256_blendv_epi8(
                  _mm256_slli_epi32(val_uint32, 8),
                  _mm256_slli_epi32(val_uint32, 24),
                  _mm256_castps_si256(mask_neg)
            ),
            mask_255
      );
}


void add_wave(float x_click, float y_click, float t){
      Wave_data data = {t, (int)x_click, (int)y_click};
      vect_push_back(&vect_of_r, &data, sizeof(Wave_data));   
}

float u_fct_gene_map(float r, float t, float inv_r){
      float r1 = (fabsf(r-C*t) < 2.5f)?fabsf(r-C*t):2.5f;
      float r2 = ((r+C*t) < 2.5f)?(r+C*t):2.5f;
      unsigned int r1_idx = (unsigned int)(r1/R_MAX_PAS);
      unsigned int r2_idx = (unsigned int)(r2/R_MAX_PAS);
      float u1 = u_r0_table[r1_idx];
      float u2 = u_r0_table[r2_idx];
      return 0.5 * inv_r * ((r-C*t)*u1 + (r+C*t)*u2);
}

float* calculate_buffer(float t, const unsigned int res_h, const unsigned int res_w){
      
      float r;
      float u;
      float inv_r;
      int x,y;
 
      for (y = 0; y < height; y += res_h) {
            for (x = 0; x < width; x += res_w) {
                  float u = 0.0f;
                  for (unsigned int w = 0; w < vect_of_r.len; w++) {
                        Wave_data* data = vect_get(&vect_of_r, w);
                        r     = r_map_all[(height + y-data->y) * width * 2 + (width + x-data->x)];
                        inv_r = inv_r_map[(height + y-data->y) * width * 2 + (width + x-data->x)];
                        u += u_fct_gene_map(r, t - data->start_t, inv_r);
                  }

                  u_buffer[y * width + x] = u;
            }
      }
      for (y = 0; y < height; y += res_h) {
            for (x = 0; x < width; x += res_w) {

                  float a = u_buffer[y * width + x];

                  bool has_b = (x + res_w < width);
                  bool has_c = (y + res_h < height);
                  bool has_d = has_b && has_c;

                  float sum = a;
                  int count = 1;

                  if (has_b) {
                        sum += u_buffer[y * width + x + res_w];
                        count++;
                  }
                  if (has_c) {
                        sum += u_buffer[(y + res_h) * width + x];
                        count++;
                  }
                  if (has_d) {
                        sum += u_buffer[(y + res_h) * width + x + res_w];
                        count++;
                  }

                  float interp = sum / count;

                  for (int dy = 0; dy < res_h; dy++) {
                        for (int dx = 0; dx < res_w; dx++) {
                              int px = x + dx;
                              int py = y + dy;
                              if (px < width && py < height) {
                                    u_buffer[py * width + px] = interp;
                              }
                        }
                  }
            }
      }
      return u_buffer;
}


void update_vector(float t, const float life_time){
      Wave_data* data;
      for(unsigned int idx=0; idx<vect_of_r.len;){
            data = vect_get(&vect_of_r, idx);
            if(t-data->start_t > life_time){
                  vect_pop_idx(&vect_of_r, idx);
            }else{
                  idx++;
            }
      }                     
}

size_t getNbWave(){
      return vect_of_r.len;
}













