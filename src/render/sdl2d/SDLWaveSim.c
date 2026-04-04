#include "SDLWaveSim.h"
#include <Windows.h>

#define MULT_COLOR 255.0f

typedef struct{
      SDL_Window* window;
      SDL_Renderer* renderer;
      SDL_Texture* texture;

      LARGE_INTEGER freq, start, end;
      LARGE_INTEGER frame_start, frame_end;
      SDL_FRect dataRect;
      unsigned int selected_freq;
      double real_freq;
      uint32_t* buffer;
}SDLWaveSim;

SDLWaveSim dataSDL;

/***** Constance SIMD *****/
static __m256 max_val ;
static __m256 min_val ;
static __m256 mult_color ;
static __m256 zero ;
static __m256i mask_255;


bool init_SDL_WaveSim(int* width, int* height, unsigned int frequency){

      /***** Init SIMD global vairable *****/
      max_val = _mm256_set1_ps(1.0f);
      min_val = _mm256_set1_ps(-1.0f);
      mult_color = _mm256_set1_ps(MULT_COLOR);
      zero = _mm256_setzero_ps();
      mask_255 = _mm256_set1_epi32(0xFF);

      /**** Initialisation de SDL ****/
      SDL_Init(SDL_INIT_VIDEO);

      if(!SDL_CreateWindowAndRenderer("Moving Masse", *width, *height, 0, &dataSDL.window, &dataSDL.renderer)){
      //if(!SDL_CreateWindowAndRenderer("Moving Masse", width, height, SDL_WINDOW_FULLSCREEN, &window, &renderer)){
            return false;
      }
      SDL_GetWindowSize(dataSDL.window, width, height);

      dataSDL.texture = SDL_CreateTexture(dataSDL.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, *width, *height);

      if(dataSDL.texture == NULL){
            return false;
      }

      
      QueryPerformanceFrequency(&dataSDL.freq);  
      QueryPerformanceCounter(&dataSDL.start); 
      QueryPerformanceCounter(&dataSDL.frame_start);
      dataSDL.dataRect = (SDL_FRect){0,0,*width,40};
      dataSDL.selected_freq = frequency;
      dataSDL.real_freq = frequency;
      dataSDL.buffer = malloc((*width)*(*height)*sizeof(uint32_t));

      return true;
}

void end_SLD_WaveSim(){
      SDL_DestroyTexture(dataSDL.texture);
      SDL_DestroyRenderer(dataSDL.renderer);
      SDL_DestroyWindow(dataSDL.window);
      SDL_Quit();
      free(dataSDL.buffer);
}

static uint32_t valueToRGB(float u){
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

void calculate_texture(float* u_buffer, int width, int height){

      int x, y;
      for (y = 0; y < height; y++) {
            for(x=0; x<width; x++){
                  dataSDL.buffer[y*width+x] = valueToRGB(u_buffer[y*width+x]);
            } 
      }
      SDL_UpdateTexture(dataSDL.texture, NULL, dataSDL.buffer, width * sizeof(uint32_t));
}

static __m256i valueToRGB_SIMD(__m256 u){
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

void calculate_texture_SIMD(float* u_buffer, int width, int height){
      uint32_t* pixels;
      int pitch, x, y;

      SDL_LockTexture(dataSDL.texture, NULL, (void**)&pixels, &pitch);
      for (y = 0; y < height; y++) {
            uint32_t* row = (uint32_t*)((uint8_t*)pixels + y * pitch);
            x=0;
            for (; x <= width-8; x+=8) {
                  __m256 u = _mm256_loadu_ps(&u_buffer[y*width+x]);
                  _mm256_storeu_si256((__m256i*)&row[x], valueToRGB_SIMD(u));
            }

            for(; x<width; x++){
                  row[x] = valueToRGB(u_buffer[y*width+x]);
            } 
      }
      SDL_UnlockTexture(dataSDL.texture);
}

static void show_data(char* data){
      QueryPerformanceCounter(&dataSDL.end);
      dataSDL.real_freq = (double)dataSDL.freq.QuadPart/(double)(dataSDL.end.QuadPart - dataSDL.start.QuadPart);
      SDL_SetRenderDrawColor(dataSDL.renderer, 255,255,255,255);
      SDL_RenderFillRect(dataSDL.renderer, &dataSDL.dataRect);

      SDL_SetRenderDrawColor(dataSDL.renderer, 50,150,50,255);
      SDL_SetRenderScale(dataSDL.renderer, 2.f, 2.f);
      SDL_RenderDebugTextFormat(dataSDL.renderer, 5, 5, "%0.2lfHz, %s", dataSDL.real_freq, data);
      SDL_SetRenderScale(dataSDL.renderer, 1.0f, 1.0f);
}

static void remaningDelay(){
      QueryPerformanceCounter(&dataSDL.frame_end);
      double elapsed = (double)(dataSDL.frame_end.QuadPart - dataSDL.frame_start.QuadPart) / (double)dataSDL.freq.QuadPart;
      double remaining = (1.0 / dataSDL.selected_freq) - elapsed;

      if (remaining > 0.0) {
            SDL_Delay((Uint32)(remaining * 1000.0));
      }

      QueryPerformanceCounter(&dataSDL.frame_start);
}

void render_WaveSim(char* data){
      SDL_RenderClear(dataSDL.renderer);
      SDL_RenderTexture(dataSDL.renderer, dataSDL.texture, NULL, NULL);

      show_data(data);
      SDL_RenderPresent(dataSDL.renderer);
      QueryPerformanceCounter(&dataSDL.start);
      remaningDelay();
}

void update_frequency(unsigned int frequency){
      dataSDL.selected_freq = frequency;
}

double get_real_frequency(){
      return dataSDL.real_freq;
}

