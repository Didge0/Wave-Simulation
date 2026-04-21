#include <math.h>
#include <immintrin.h>
#include "waveSim.h"
#include <stdlib.h>
#include "vector.h"
#include <assert.h>


#define C 4.0f
#define U_R0_RESOLUTION 1024
#define R_MAX 2.5f
#define R_MAX_PAS (R_MAX / (float)U_R0_RESOLUTION)
#define INV_R_MAX_PAS ( (float)U_R0_RESOLUTION / (float)R_MAX)
#define WAVE_IMPACT_THRESHOLD 1e-6f

typedef struct {
      float start_t;
      int x,y;
      float max_impact;
} Wave_data;

typedef struct {
      Vector vect_of_r;
      float* u_buffer;
      float* r_map_all;
      float* inv_r_map;
      float u_r0_table[U_R0_RESOLUTION];
} WaveSim;

static WaveSim dataWaveSim;
static int width;
static int height;

/*** CONSTANTE SIMD ***/
__m256 SIMD_C;
__m256 SIMD_abs_mask;
__m256 SIMD_R_max;
__m256 SIMD_Inv_R_max_pas;
__m256 SIMD_0_5;

static inline float u_r0(float r){
      if(r > R_MAX) return 0.0f;
      return expf(-r*r);
}

static inline void init_data_wave_sim(WaveSim* sim){
      sim->u_buffer = NULL;

      /***** Initialisation de la map de r pour la première onde *****/
      sim->r_map_all = malloc(width * 2 * height * 2 * sizeof(float));
      sim->inv_r_map = malloc(width * 2 * height * 2 * sizeof(float));
      for(unsigned int x=0; x<width*2; x++){
            for(unsigned y=0; y<height*2; y++){
                  float dx = ((float)x - width)/50.0f;
                  float dy = ((float)y - height)/50.0f;
                  sim->r_map_all[y*width*2 + x] = sqrtf(dx*dx + dy*dy);
                  if(sim->r_map_all[y*width*2 + x] < 0.9f)
                        sim->inv_r_map[y*width*2 + x] = 1;
                  else
                        sim->inv_r_map[y*width*2 + x] = 1/sim->r_map_all[y*width*2 + x];
            }
      }

      for(unsigned int i=0; i<U_R0_RESOLUTION; i++){
            sim->u_r0_table[i] = u_r0(R_MAX_PAS*i);
      }

      /***** Initialisation du buffer  *****/
      sim->u_buffer = malloc(width * height * sizeof(float));
      assert(sim->u_buffer);
}

bool init_wave_simulation(int _width, int _height){

      width = _width;
      height = _height;

      dataWaveSim.vect_of_r = vect_init(sizeof(Wave_data));
      dataWaveSim.u_buffer = NULL;
      dataWaveSim.r_map_all = NULL;
      dataWaveSim.inv_r_map = NULL;

      init_data_wave_sim(&dataWaveSim);

      SIMD_C = _mm256_set1_ps(C);
      SIMD_abs_mask = _mm256_set1_ps(-0.0f);
      SIMD_R_max = _mm256_set1_ps(R_MAX);
      SIMD_Inv_R_max_pas = _mm256_set1_ps(INV_R_MAX_PAS);
      SIMD_0_5 = _mm256_set1_ps(0.5f);

      return true;
}

bool resize_wave_simulation(int new_width, int new_height){
      if(new_width <= 0 || new_height <= 0){
            return false;
      }

      width = new_width;
      height = new_height;

      free(dataWaveSim.r_map_all);
      free(dataWaveSim.inv_r_map);
      free(dataWaveSim.u_buffer);

      init_data_wave_sim(&dataWaveSim);

      return true;
}

bool resize_wave_simulation_width(int new_width){
      return resize_wave_simulation(new_width, height);
}

bool resize_wave_simulation_height(int new_height){
      return resize_wave_simulation(width, new_height);
}

void end_simulation(){
      vect_free(&dataWaveSim.vect_of_r);
      free(dataWaveSim.u_buffer);
      free(dataWaveSim.r_map_all);
      free(dataWaveSim.inv_r_map);
}

void add_wave(float x_click, float y_click, float t){
      Wave_data data = {t, (int)x_click, (int)y_click, 0.0f};
      vect_push_back(&dataWaveSim.vect_of_r, &data, sizeof(Wave_data));   
}

static void reset_wave_impacts(void){
      for(unsigned int idx = 0; idx < dataWaveSim.vect_of_r.len; idx++){
            Wave_data* data = vect_get(&dataWaveSim.vect_of_r, idx);
            data->max_impact = 0.0f;
      }
}

static float u_fct_gene_map(float r, float t, float inv_r){
      float r1 = (fabsf(r-C*t) < R_MAX)?fabsf(r-C*t):R_MAX;
      float r2 = ((r+C*t) < R_MAX)?(r+C*t):R_MAX;
      unsigned int r1_idx = (unsigned int)(r1*INV_R_MAX_PAS);
      unsigned int r2_idx = (unsigned int)(r2*INV_R_MAX_PAS);
      float u1 = dataWaveSim.u_r0_table[r1_idx];
      float u2 = dataWaveSim.u_r0_table[r2_idx];
      return 0.5 * inv_r * ((r-C*t)*u1 + (r+C*t)*u2);
}

static __m256 u_fct_gene_map_SIMD(__m256 r, __m256 t, __m256 inv_r){
      __m256 C_t = _mm256_mul_ps(SIMD_C, t);
      __m256 r_plus_C_t = _mm256_add_ps(r,C_t);
      __m256 r_moin_C_t = _mm256_sub_ps(r,C_t);
      __m256 abs_r_moin_C_t = _mm256_andnot_ps(SIMD_abs_mask,r_moin_C_t);

      r_plus_C_t = _mm256_min_ps(r_plus_C_t, SIMD_R_max);
      abs_r_moin_C_t = _mm256_min_ps(abs_r_moin_C_t, SIMD_R_max);

      // r_plus_C_t = _mm256_blendv_ps(r_plus_C_t, SIMD_R_max, _mm256_cmp_ps(SIMD_R_max, r_plus_C_t, _CMP_LT_OQ));
      // r_moin_C_t = _mm256_blendv_ps(r_moin_C_t, SIMD_R_max, _mm256_cmp_ps(SIMD_R_max, r_moin_C_t, _CMP_LT_OQ));
      
      __m256 rplus_idx_float = _mm256_mul_ps(r_plus_C_t, SIMD_Inv_R_max_pas);
      __m256 rmoin_idx_float = _mm256_mul_ps(abs_r_moin_C_t, SIMD_Inv_R_max_pas);

      __m256i rplus_idx_int = _mm256_cvttps_epi32(rplus_idx_float);
      __m256i rmoin_idx_int = _mm256_cvttps_epi32(rmoin_idx_float);
      
      __m256 uplus = _mm256_i32gather_ps(dataWaveSim.u_r0_table, rplus_idx_int, 4);
      __m256 umoin = _mm256_i32gather_ps(dataWaveSim.u_r0_table, rmoin_idx_int, 4);
      
      __m256 result = _mm256_add_ps(_mm256_mul_ps(r_moin_C_t, umoin), _mm256_mul_ps(r_plus_C_t, uplus));
      result = _mm256_mul_ps(inv_r, result);
      result = _mm256_mul_ps(SIMD_0_5, result);

      return result;
}

void fill_hole_in_buffer(const unsigned int res_h, const unsigned int res_w){
      int x,y;
      for (y = 0; y < height; y += res_h) {
            for (x = 0; x < width; x += res_w) {

                  float a = dataWaveSim.u_buffer[y * width + x];

                  bool has_b = (x + res_w < width);
                  bool has_c = (y + res_h < height);
                  bool has_d = has_b && has_c;

                  float sum = a;
                  int count = 1;

                  if (has_b) {
                        sum += dataWaveSim.u_buffer[y * width + x + res_w];
                        count++;
                  }
                  if (has_c) {
                        sum += dataWaveSim.u_buffer[(y + res_h) * width + x];
                        count++;
                  }
                  if (has_d) {
                        sum += dataWaveSim.u_buffer[(y + res_h) * width + x + res_w];
                        count++;
                  }

                  float interp = sum / count;

                  for (int dy = 0; dy < res_h; dy++) {
                        for (int dx = 0; dx < res_w; dx++) {
                              int px = x + dx;
                              int py = y + dy;
                              if (px < width && py < height) {
                                    dataWaveSim.u_buffer[py * width + px] = interp;
                              }
                        }
                  }
            }
      }
}

float* calculate_buffer(float t, const unsigned int res_h, const unsigned int res_w){
      
      float r;
      float u;
      float inv_r;
      int x,y;
 
      reset_wave_impacts();
      for (y = 0; y < height; y += res_h) {
            for (x = 0; x < width; x += res_w) {
                  u = 0.0f;
                  for (unsigned int w = 0; w < dataWaveSim.vect_of_r.len; w++) {
                        Wave_data* data = vect_get(&dataWaveSim.vect_of_r, w);
                        r     = dataWaveSim.r_map_all[(height + y-data->y) * width * 2 + (width + x-data->x)];
                        inv_r = dataWaveSim.inv_r_map[(height + y-data->y) * width * 2 + (width + x-data->x)];
                        float contribution = u_fct_gene_map(r, t - data->start_t, inv_r);
                        u += contribution;
                        float abs_contribution = fabsf(contribution);
                        if(abs_contribution > data->max_impact){
                              data->max_impact = abs_contribution;
                        }
                  }

                  dataWaveSim.u_buffer[y * width + x] = u;
            }
      }
      if(res_h != 1 || res_w != 1){
            fill_hole_in_buffer(res_h, res_w);
      }
      return dataWaveSim.u_buffer;
}

float* calculate_buffer_SIMD(float t, const unsigned int res_h, const unsigned int res_w){ 
      reset_wave_impacts();
      int x,y;
      float r,inv_r,u;
      __m256 SIMD_u;
      __m256i idx_map;
      __m256 SIMD_r;
      __m256 SIMD_inv_r;
      __m256 SIMD_t = _mm256_set1_ps(t);
      __m256 SIMD_t_start;
      __m256i lane_offsets = _mm256_setr_epi32(0*res_w, 1*res_w, 2*res_w, 3*res_w, 4*res_w, 5*res_w, 6*res_w, 7*res_w);
 
      for (y = 0; y < height; y += res_h) {
            //SIMD
            x = 0;
            for (; x < width-(res_w*8); x += res_w*8) {
                  SIMD_u = _mm256_setzero_ps();
                  for (unsigned int w = 0; w < dataWaveSim.vect_of_r.len; w++) {
                        Wave_data* data = vect_get(&dataWaveSim.vect_of_r, w);
                        idx_map = _mm256_add_epi32(_mm256_set1_epi32((height + y-data->y) * width * 2 + width + x-data->x),lane_offsets);
                        SIMD_r = _mm256_i32gather_ps(dataWaveSim.r_map_all, idx_map, 4);
                        SIMD_inv_r = _mm256_i32gather_ps(dataWaveSim.inv_r_map, idx_map, 4);
                        SIMD_t_start = _mm256_set1_ps(data->start_t);
                        __m256 SIMD_contribution = u_fct_gene_map_SIMD(SIMD_r, _mm256_sub_ps(SIMD_t, SIMD_t_start), SIMD_inv_r);
                        SIMD_u = _mm256_add_ps(SIMD_u, SIMD_contribution);

                        __m256 abs_contribution = _mm256_andnot_ps(SIMD_abs_mask, SIMD_contribution);
                        float lane_max[8];
                        _mm256_storeu_ps(lane_max, abs_contribution);
                        for (int lane = 0; lane < 8; lane++) {
                              if (lane_max[lane] > data->max_impact) {
                                    data->max_impact = lane_max[lane];
                              }
                        }
                  }

                  if(res_w == 1){
                        _mm256_storeu_ps(&dataWaveSim.u_buffer[y * width + x], SIMD_u);
                  }else{
                        float tmp[8];
                        _mm256_storeu_ps(tmp, SIMD_u);
                        for (int i=0; i<8; ++i) {
                              dataWaveSim.u_buffer[y*width + x + i*res_w] = tmp[i];
                        }
                  }
            }
            //RESET
            for (; x < width; x += res_w) {
                  u = 0.0f;
                  for (unsigned int w = 0; w < dataWaveSim.vect_of_r.len; w++) {
                        Wave_data* data = vect_get(&dataWaveSim.vect_of_r, w);
                        r     = dataWaveSim.r_map_all[(height + y-data->y) * width * 2 + (width + x-data->x)];
                        inv_r = dataWaveSim.inv_r_map[(height + y-data->y) * width * 2 + (width + x-data->x)];
                        float contribution = u_fct_gene_map(r, t - data->start_t, inv_r);
                        u += contribution;
                        float abs_contribution = fabsf(contribution);
                        if(abs_contribution > data->max_impact){
                              data->max_impact = abs_contribution;
                        }
                  }
                  dataWaveSim.u_buffer[y * width + x] = u;
            }

      }
      if(res_h != 1 || res_w != 1){
            fill_hole_in_buffer(res_h, res_w);
      }
      return dataWaveSim.u_buffer;
}


void update_vector(float t){
      Wave_data* data;
      for(unsigned int idx=0; idx<dataWaveSim.vect_of_r.len;){
            data = vect_get(&dataWaveSim.vect_of_r, idx);
            if(data->max_impact < WAVE_IMPACT_THRESHOLD){
                  vect_pop_idx(&dataWaveSim.vect_of_r, idx);
            }else{
                  idx++;
            }
      }                     
}

size_t getNbWave(){
      return dataWaveSim.vect_of_r.len;
}













