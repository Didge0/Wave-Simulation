#include "SDLWaveSim3D.h"
#include <Windows.h>

#define MULT_COLOR 255.0f
#define SPEED_LEFT_RIGHT 5
#define SPEED_FRONT_BACK 5
#define SPEED_UP_DOWN 5

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


typedef struct{
      int x,y,z;
}Vect3;
typedef struct{
      int x,y;
}Vect2;

typedef struct{
      Vect3 camera3D;
      Vect2 camera2D;
      Vect3 screen;
}Env3D;

static Env3D env_3D;

static Vect3 Env[7][2];


/***** Constance SIMD *****/
static __m256 max_val ;
static __m256 min_val ;
static __m256 mult_color ;
static __m256 zero ;
static __m256i mask_255;


bool init_SDL_WaveSim3D(int* width, int* height, unsigned int frequency){
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

      Env[0][0] = (Vect3){0,(*height)/4,0};
      Env[0][1] = (Vect3){(*width),(*height)/4,0};
      Env[1][0] = (Vect3){0,(*height)/2,0};
      Env[1][1] = (Vect3){(*width),(*height)/2,0};
      Env[2][0] = (Vect3){0,3*(*height)/4,0};
      Env[2][1] = (Vect3){(*width),3*(*height)/4,0};
      Env[3][0] = (Vect3){(*width)/4,0,0};
      Env[3][1] = (Vect3){(*width)/4,(*height),0};
      Env[4][0] = (Vect3){(*width)/2,0,0};
      Env[4][1] = (Vect3){(*width)/2,(*height),0};
      Env[5][0] = (Vect3){3*(*width)/4,0,0};
      Env[5][1] = (Vect3){3*(*width)/4,(*height),0};
      Env[6][0] = (Vect3){0,0,100};
      Env[6][0] = (Vect3){0,0,-100};

      env_3D.screen = (Vect3){0,(*height) + 400, 0};
      env_3D.camera3D = (Vect3){(*width)/2,  env_3D.screen.y + 400, 200};
      env_3D.camera2D = (Vect2){(*width)/2, ((*height)/2) - env_3D.camera3D.z};

      return true;
}

void end_SLD_WaveSim3D(){
      SDL_DestroyTexture(dataSDL.texture);
      SDL_DestroyRenderer(dataSDL.renderer);
      SDL_DestroyWindow(dataSDL.window);
      SDL_Quit();
      free(dataSDL.buffer);
}

void Convert_3D_to_2D(Vect3* point3D, Vect2* point2D){
      point2D->y = env_3D.camera2D.y + (((env_3D.camera3D.y - env_3D.screen.y)*(env_3D.camera3D.z - point3D->z))/(env_3D.camera3D.y-point3D->y));
      point2D->x = env_3D.camera2D.x - (((env_3D.camera3D.y - env_3D.screen.y)*(env_3D.camera3D.x - point3D->x))/(env_3D.camera3D.y-point3D->y));
}

static void draw_line( uint32_t* pixels, int width, int height, Vect2 pt1, Vect2 pt2, uint32_t color){
    int dx = abs(pt2.x - pt1.x);
    int dy = -abs(pt2.y - pt1.y);
    int sx = (pt1.x < pt2.x) ? 1 : -1;
    int sy = (pt1.y < pt2.y) ? 1 : -1;
    int err = dx + dy;

    while (1) {
        if (pt1.x >= 0 && pt1.x < width && pt1.y >= 0 && pt1.y < height) {
            pixels[pt1.y * width + pt1.x] = color;
        }

        if (pt1.x == pt2.x && pt1.y == pt2.y) break;

        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; pt1.x += sx; }
        if (e2 <= dx) { err += dx; pt1.y += sy; }
    }
}

static void draw_Env(int width, int height){
      int x,y;
      Vect2 new_pt1;
      Vect2 new_pt2;
      
      for(x=0; x<6; x++){
            Convert_3D_to_2D(&Env[x][0], &new_pt1);
            Convert_3D_to_2D(&Env[x][1], &new_pt2);
            draw_line(dataSDL.buffer, width, height, new_pt1,new_pt2, 0xFFFFFFFF);
            //draw_line(dataSDL.buffer, width, height, Env[0][0].x, Env[0][0].y, Env[0][1].x, Env[0][1].x, 0xFFFFFFFF);
      }
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

void calculate_texture3D(float* u_buffer, int width, int height){
      int x, y;
      /*for (y = 0; y < height; y++) {
            for(x=0; x<width; x++){
                  dataSDL.buffer[y*width+x] = valueToRGB(u_buffer[y*width+x]);
            } 
      } */
      for (y = 0; y < height; y++) {
            for(x=0; x<width; x++){
                  dataSDL.buffer[y*width+x] = 0x000000FF;
            } 
      }
      draw_Env(width, height);
      //draw_line(dataSDL.buffer,width, height,100, 100, 400, 300,0xFFFFFFFF);
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

void calculate_texture_SIMD3D(float* u_buffer, int width, int height){
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

void render_WaveSim3D(char* data){
      SDL_RenderClear(dataSDL.renderer);
      SDL_RenderTexture(dataSDL.renderer, dataSDL.texture, NULL, NULL);

      show_data(data);
      SDL_RenderPresent(dataSDL.renderer);
      QueryPerformanceCounter(&dataSDL.start);
      remaningDelay();
}

void update_frequency3D(unsigned int frequency){
      dataSDL.selected_freq = frequency;
}

double get_real_frequency3D(){
      return dataSDL.real_freq;
}

void move_camera_forward(){
      if(env_3D.camera3D.y - 1 > env_3D.screen.y )
            env_3D.camera3D.y -= SPEED_FRONT_BACK;
}
void move_camera_backward(){
      env_3D.camera3D.y += SPEED_FRONT_BACK;
}
void move_camera_right(){
      env_3D.camera3D.x += SPEED_LEFT_RIGHT;
      env_3D.camera2D.x += SPEED_LEFT_RIGHT;
}
void move_camera_left(){
      env_3D.camera3D.x -= SPEED_LEFT_RIGHT;
      env_3D.camera2D.x -= SPEED_LEFT_RIGHT;
}

void get_camera_coord(int* x, int* y, int* z){
      *x = env_3D.camera3D.x;
      *y = env_3D.camera3D.y;
      *z = env_3D.camera3D.z;
}

void move_camera_up(){
      env_3D.camera3D.z += SPEED_UP_DOWN;
      env_3D.camera2D.y -= SPEED_UP_DOWN;
}
void move_camera_down(){
      env_3D.camera3D.z -= SPEED_UP_DOWN;
      env_3D.camera2D.y += SPEED_UP_DOWN;
}



