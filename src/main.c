#include <stdio.h>
#include <SDL3/SDL.h>
#include <math.h>
#include <Windows.h>
#include <assert.h>
#include "Lib/vector.h"
#include <crtdbg.h>

#define C 4
#define A 5.0f
#define MULT_COLOR 51.0f
#define LIFE_TIME 4.0f
#define NB_FACT 9

#define RESOLUTION_WIDTH  3
#define RESOLUTION_HEIGHT 3


SDL_Window* window;
SDL_Renderer* renderer;
SDL_Texture* texture;
int width = 900;
int height = 600;
unsigned int selected_freq = 24;

Vector vect_of_r;

float* u_buffer = NULL;
float fact[NB_FACT+1];
float* r_map_all;
float* inv_r_map;

typedef struct {
      float start_t;
      int x,y;
}Wave_data;

inline float exp_approxim(float x){
      return (1 +
      x/fact[1] +
      (x*x)/fact[2] +
      (x*x*x)/fact[3] +
      (x*x*x*x)/fact[4] +
      (x*x*x*x*x)/fact[5] +
      (x*x*x*x*x*x)/fact[6] +
      (x*x*x*x*x*x*x)/fact[7] +
      (x*x*x*x*x*x*x*x)/fact[8] +
      (x*x*x*x*x*x*x*x*x)/fact[9]);
}

bool init_projet_global_data(){
      /**** Initialisation de SDL ****/
      SDL_Init(SDL_INIT_VIDEO);

      if(!SDL_CreateWindowAndRenderer("Moving Masse", width, height, 0, &window, &renderer)){
      //if(!SDL_CreateWindowAndRenderer("Moving Masse", width, height, SDL_WINDOW_FULLSCREEN, &window, &renderer)){
            return false;
      }
      SDL_GetWindowSize(window, &width, &height);

      texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, width, height);

      if(texture == NULL){
            return false;
      }

      /***** Initialisation de la map de r pour la première onde *****/
      vect_of_r = vect_init(sizeof(Wave_data));

      Wave_data d0;
      d0.start_t = 0;
      d0.x = width/2;
      d0.y = height/2;
      vect_push_back(&vect_of_r, &d0, sizeof(Wave_data));

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


      /***** Initialisation du buffer  *****/
      u_buffer = malloc(width * height * sizeof(float));
      assert(u_buffer);

      /**** Initialisation du tableau factorielle n: n! ****/
      unsigned int fact_somme = 1;
      fact[0] = 1.0f;
      for(unsigned int n=1; n<NB_FACT+1; n++){
            fact_somme *= n;
            fact[n] = (float)fact_somme; 
      }

      return true;
}

uint32_t valueToRGB(float u){
      if(u > A){
            return 0xff0000ff;
      }else if( u < -A){
            return 0x0000ffff;
      }
      /* u = u*u*u*u*u;
      u /= A*A*A*A; */
      if(u < 0.0f){
            return ((uint8_t)(-u * MULT_COLOR)<<24) |0x0000ff;
      }else{
            return 0x0000 | ((uint8_t)(u * MULT_COLOR) << 8) | 0xff; 
      }
}

void update_r_map(float x_click, float y_click, float t){
      Wave_data data = {t, (int)x_click, (int)y_click};
      vect_push_back(&vect_of_r, &data, sizeof(Wave_data));   
}

float value;
float u_r0(float r){
      if(r > 2.5f) return 0.0f;
      //float value = A * expf(-(r*r));
      value = A * exp_approxim(-r*r);
      if(value < 0.0f) return 0.0f;
      return value;
}

float u_fct_gene(float r, float t, float inv_r){
      if( r < C*t){
            return 0.5 * inv_r * ((r-C*t)*u_r0(-(r-C*t)) + (r+C*t)*u_r0(r+C*t));
      }else{
            return 0.5 * inv_r * ((r-C*t)*u_r0(r-C*t) + (r+C*t)*u_r0(r+C*t));
      }
}

void draw_screen(float t){
      uint32_t* pixels;
      int pitch;
      float r;
      float u;
      float inv_r;
      /**** Ancienne version ou tous les pixels était traité pas optimal car on peux baisser la résolution et interpoler les pixels pour "retrouver" sa couleur ****/
      /*Wave_data* data;

      SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

      // pitch = nombre d’octets par ligne
      // pitch / 4 = nombre de pixels par ligne

      for (int y = 0; y < height; y+=RESOLUTION_HEIGHT) {
            uint32_t* row = (uint32_t*)((uint8_t*)pixels + y * pitch);
            for (int x = 0; x < width; x+=RESOLUTION_WIDTH) {
                  u=0.0f;
                  for(unsigned int w=0; w<vect_of_r.len; w++){
                        data = (Wave_data*)vect_get(&vect_of_r, w);
                        r = data->r_map[y*width + x];
                        u += u_fct(r, t-data->start_t);
                  }
                  

                  row[x] = valueToRGB(u);
            }
      }

      SDL_UnlockTexture(texture); */
      for (int y = 0; y < height; y += RESOLUTION_HEIGHT) {
            for (int x = 0; x < width; x += RESOLUTION_WIDTH) {
                  float u = 0.0f;
                  for (unsigned int w = 0; w < vect_of_r.len; w++) {
                        Wave_data* data = vect_get(&vect_of_r, w);
                        r     = r_map_all[(height + y-data->y) * width * 2 + (width + x-data->x)];
                        inv_r = inv_r_map[(height + y-data->y) * width * 2 + (width + x-data->x)];
                        u += u_fct_gene(r, t - data->start_t, inv_r);
                  }

                  u_buffer[y * width + x] = u;
            }
      }
      for (int y = 0; y < height - RESOLUTION_HEIGHT; y += RESOLUTION_HEIGHT) {
            for (int x = 0; x < width - RESOLUTION_WIDTH; x += RESOLUTION_WIDTH) {

                  float a = u_buffer[y * width + x];
                  float b = u_buffer[y * width + x + RESOLUTION_WIDTH];
                  float c = u_buffer[(y + RESOLUTION_HEIGHT) * width + x];
                  float d = u_buffer[(y + RESOLUTION_HEIGHT) * width + x + RESOLUTION_WIDTH];

                  for (int dy = 0; dy < RESOLUTION_HEIGHT; dy++) {
                        for (int dx = 0; dx < RESOLUTION_WIDTH; dx++) {

                        int px = x + dx;
                        int py = y + dy;

                        // interpolation simple (rapide)
                        u_buffer[py * width + px] =
                              0.25f * (a + b + c + d);
                        }
                  }
            }
      }
      SDL_LockTexture(texture, NULL, (void**)&pixels, &pitch);

      for (int y = 0; y < height; y++) {
            uint32_t* row = (uint32_t*)((uint8_t*)pixels + y * pitch);
            for (int x = 0; x < width; x++) {
                  row[x] = valueToRGB(u_buffer[y * width + x]);
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

void remaningDelay(LARGE_INTEGER* frame_end, LARGE_INTEGER* frame_start, LARGE_INTEGER* freq){
      QueryPerformanceCounter(frame_end);
            double elapsed = (double)(frame_end->QuadPart - frame_start->QuadPart) / (double)freq->QuadPart;
            double remaining = (1.0 / selected_freq) - elapsed;

            if (remaining > 0.0) {
                  SDL_Delay((Uint32)(remaining * 1000.0));
            }

            QueryPerformanceCounter(frame_start);
}

void update_vector(float t){
      Wave_data* data;
      for(unsigned int idx=0; idx<vect_of_r.len;){
            data = vect_get(&vect_of_r, idx);
            if(t-data->start_t > LIFE_TIME){
                  vect_pop_idx(&vect_of_r, idx);
            }else{
                  idx++;
            }
      }                     
}

void free_all_data(){
      vect_free(&vect_of_r);
      free(u_buffer);
      free(r_map_all);
      free(inv_r_map);
}

void rand_wave(float t){
      if(rand()%20 == 0){
            int x = rand() % width;
            int y = rand() % height;
            update_r_map((float)x, (float)y, t);
      }

}

int main(){
      assert(255/A == MULT_COLOR);
      srand( time( NULL ) );

      if(!init_projet_global_data()){
            return -1;
      }
      bool is_opened = true;

      LARGE_INTEGER freq, start, end;
      LARGE_INTEGER frame_start, frame_end;
      QueryPerformanceFrequency(&freq);  
      QueryPerformanceCounter(&start); 
      QueryPerformanceCounter(&frame_start);
      SDL_FRect dataRect = {0,0,width,40}; 
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
                              update_r_map(x_click, y_click, t);
                        }
                  }
            }
            draw_screen(t);
            t+=1/(float)selected_freq;
            update_vector(t);
            rand_wave(t);
            

            SDL_RenderClear(renderer);
            SDL_RenderTexture(renderer, texture, NULL, NULL);

            show_data(&dataRect, &end, &start, &freq, t, vect_of_r.len);
            SDL_RenderPresent(renderer);
            QueryPerformanceCounter(&start);
            remaningDelay(&frame_end, &frame_start, &freq);
      }
      SDL_DestroyTexture(texture);
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
      SDL_Quit();
      free_all_data();
      if (_CrtDumpMemoryLeaks()) {
            printf("Fuites memoire !\n");
      }

      return 0;
}