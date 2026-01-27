#include "SDLWaveSim.h"

SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;

LARGE_INTEGER freq, start, end;
LARGE_INTEGER frame_start, frame_end;
SDL_FRect dataRect;
unsigned int selected_freq;

bool init_SDL_WaveSim(int* width, int* height, unsigned int frequency){

      /**** Initialisation de SDL ****/
      SDL_Init(SDL_INIT_VIDEO);

      if(!SDL_CreateWindowAndRenderer("Moving Masse", *width, *height, 0, &window, &renderer)){
      //if(!SDL_CreateWindowAndRenderer("Moving Masse", width, height, SDL_WINDOW_FULLSCREEN, &window, &renderer)){
            return false;
      }
      SDL_GetWindowSize(window, width, height);

      texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, *width, *height);

      if(texture == NULL){
            return false;
      }

      
      QueryPerformanceFrequency(&freq);  
      QueryPerformanceCounter(&start); 
      QueryPerformanceCounter(&frame_start);
      dataRect = (SDL_FRect){0,0,*width,40};
      selected_freq = frequency;

      return true;
}

void end_SLD_WaveSim(){
      SDL_DestroyTexture(texture);
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
      SDL_Quit();
}

void calculate_texture(float* u_buffer, int width, int height, uint32_t (*toRGB_fct)(float)){
      uint32_t* pixels;
      int pitch, x, y;

      SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
      for (y = 0; y < height; y++) {
            uint32_t* row = (uint32_t*)((uint8_t*)pixels + y * pitch);
            for(x=0; x<width; x++){
                  row[x] = valueToRGB(u_buffer[y*width+x]);
            } 
      }
      SDL_UnlockTexture(texture);
}

void calculate_texture_SIMD(float* u_buffer, int width, int height, __m256i (*toRGB_fct)(__m256)){
      uint32_t* pixels;
      int pitch, x, y;

      SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);
      for (y = 0; y < height; y++) {
            uint32_t* row = (uint32_t*)((uint8_t*)pixels + y * pitch);
            x=0;
            for (; x <= width-8; x+=8) {
                  __m256 u = _mm256_loadu_ps(&u_buffer[y*width+x]);
                  _mm256_storeu_si256((__m256i*)&row[x], toRGB_fct(u));
            }

            for(; x<width; x++){
                  row[x] = valueToRGB(u_buffer[y*width+x]);
            } 
      }
      SDL_UnlockTexture(texture);
}

void show_data(SDL_FRect* dataRect, LARGE_INTEGER* end, LARGE_INTEGER* start, LARGE_INTEGER* freq, float t, size_t nb_wave){
      QueryPerformanceCounter(end);
      double frequency = (double)freq->QuadPart/(double)(end->QuadPart - start->QuadPart);
      SDL_SetRenderDrawColor(renderer, 255,255,255,255);
      SDL_RenderFillRect(renderer, dataRect);

      SDL_SetRenderDrawColor(renderer, 50,150,50,255);
      SDL_SetRenderScale(renderer, 2.f, 2.f);
      SDL_RenderDebugTextFormat(renderer, 5, 5, "%uHz, %.1fs, %u\n", (unsigned int)frequency, t, nb_wave);
      SDL_SetRenderScale(renderer, 1.0f, 1.0f);
}

void remaningDelay(LARGE_INTEGER* frame_end, LARGE_INTEGER* frame_start, LARGE_INTEGER* freq, unsigned int selected_freq){
      QueryPerformanceCounter(frame_end);
      double elapsed = (double)(frame_end->QuadPart - frame_start->QuadPart) / (double)freq->QuadPart;
      double remaining = (1.0 / selected_freq) - elapsed;

      if (remaining > 0.0) {
            SDL_Delay((Uint32)(remaining * 1000.0));
      }

      QueryPerformanceCounter(frame_start);
}

void render_WaveSim(float t, size_t nb_wave){
      SDL_RenderClear(renderer);
      SDL_RenderTexture(renderer, texture, NULL, NULL);

      show_data(&dataRect, &end, &start, &freq, t, nb_wave);
      SDL_RenderPresent(renderer);
      QueryPerformanceCounter(&start);
      remaningDelay(&frame_end, &frame_start, &freq, selected_freq);
}



