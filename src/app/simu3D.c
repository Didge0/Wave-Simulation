#define _USE_MATH_DEFINES
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <Windows.h>
#include <SDL3/SDL.h>
#include <crtdbg.h>

#include "waveSim.h"
#include "Moteur3D/Moteur3D.h"

#define DRAWING_MODE 0   //0:Point, 1:Mesh, 2:Triangles
#define FULL_SCREEN true
#define OPTI_SELECTED 1       //0:Scalaire, 1:SIMD
#define LIFE_TIME 4.0f
#define RESOLUTION_WIDTH  3u
#define RESOLUTION_HEIGHT 3u
#define WIDTH 600
#define HEIGHT 600
#define FREQUENCY 24

#define DEBUG_TEXT_SCALE 1.0f
#define WAVE_WORLD_SCALE_X 0.03f
#define WAVE_WORLD_SCALE_Y 0.03f
#define WAVE_HEIGHT_SCALE  6.0f
#define WAVE_RENDER_STEP   5

static int width;
static int height;

static float clamp_percentage(float value){
      if(value < 0.0f){
            return 0.0f;
      }
      if(value > 100.0f){
            return 100.0f;
      }
      return value;
}

static void rand_wave(float t, unsigned int frequency, unsigned int nb_par_second){
      if(nb_par_second == 0){
            return;
      }
      if(rand() % (frequency / nb_par_second) == 0){
            int x = rand() % width;
            int y = rand() % height;
            add_wave((float)x, (float)y, t);
      }
}

static Vect3 wave_point_to_world(int sample_x, int sample_y, float amplitude){
      return (Vect3){
            ((float)sample_x - ((float)width * 0.5f)) * WAVE_WORLD_SCALE_X,
            ((float)sample_y - ((float)height * 0.5f)) * WAVE_WORLD_SCALE_Y,
            amplitude * WAVE_HEIGHT_SCALE
      };
}

static uint32_t wave_color(float amplitude){
      if(amplitude > 1.0f){
            amplitude = 1.0f;
      }else if(amplitude < -1.0f){
            amplitude = -1.0f;
      }

      if(amplitude == 0.0f){
            return 0x000000FF;
      }

      uint32_t intensity = (uint32_t)(fabsf(amplitude) * 255.0f);

      if(amplitude > 0.0f){
            return (intensity << 24) | 0x000000FF;
      }

      return (intensity << 8) | 0x000000FF;
}

static void draw_wave_triangles(M3D_Engine* engine, const float* buffer, int step){
      for(int y = 0; y < height - step; y += step){
            for(int x = 0; x < width - step; x += step){
                  int idx = y * width + x;
                  int idx_right = y * width + (x + step);
                  int idx_down = (y + step) * width + x;
                  int idx_diag = (y + step) * width + (x + step);

                  Vect3 p00 = wave_point_to_world(x, y, buffer[idx]);
                  Vect3 p10 = wave_point_to_world(x + step, y, buffer[idx_right]);
                  Vect3 p01 = wave_point_to_world(x, y + step, buffer[idx_down]);
                  Vect3 p11 = wave_point_to_world(x + step, y + step, buffer[idx_diag]);

                  float amp_t1 = (buffer[idx] + buffer[idx_right] + buffer[idx_diag]) / 3.0f;
                  float amp_t2 = (buffer[idx] + buffer[idx_diag] + buffer[idx_down]) / 3.0f;

                  M3D_draw_triangle(engine, &p00, &p10, &p11, wave_color(amp_t1));
                  M3D_draw_triangle(engine, &p00, &p11, &p01, wave_color(amp_t2));
            }
      }
}

static void draw_wave_mesh(M3D_Engine* engine, const float* buffer, int step){
      for(int y = 0; y < height - step; y += step){
            for(int x = 0; x < width - step; x += step){
                  int idx = y * width + x;
                  int idx_right = y * width + (x + step);
                  int idx_down = (y + step) * width + x;

                  Vect3 current = wave_point_to_world(x, y, buffer[idx]);
                  Vect3 right = wave_point_to_world(x + step, y, buffer[idx_right]);
                  Vect3 down = wave_point_to_world(x, y + step, buffer[idx_down]);

                  M3D_draw_line(engine, &current, &right, wave_color((buffer[idx] + buffer[idx_right]) * 0.5f));
                  M3D_draw_line(engine, &current, &down, wave_color((buffer[idx] + buffer[idx_down]) * 0.5f));
            }
      }
}

static void draw_wave_point(M3D_Engine* engine, const float* buffer, int step){
      for(int y = 0; y < height - step; y += step){
            for(int x = 0; x < width - step; x += step){
                  int idx = y * width + x;
                  int idx_right = y * width + (x + step);
                  int idx_down = (y + step) * width + x;

                  Vect3 current = wave_point_to_world(x, y, buffer[idx]);

                  M3D_draw_point(engine, &current, wave_color(buffer[idx]));
            }
      }
}

int main(){
      M3D_Engine engine;
      Moteur3D_InitData initData;
      float t = 0.0f;
      float dt = 1.0f / (float)FREQUENCY;
      unsigned int nb_par_second = 1;
      Uint64 perf_frequency = SDL_GetPerformanceFrequency();

      srand((unsigned)time(NULL));
      M3D_fill_default_init_data(&initData, WIDTH, HEIGHT);

      bool is_opened = true;
      if(!M3D_init_custom(&engine, &initData, "Moving Masse", FULL_SCREEN)){
            return false;
      }
      width = WIDTH;
      height = HEIGHT;
      if(!init_wave_simulation(width, height)){
            M3D_shutdown(&engine);
            return false;
      }
      add_wave((float)(width / 2), (float)(height / 2), t);

      M3D_InputState input = {0};
      int drawing_mode = DRAWING_MODE;
      Uint64 prev_ticks = SDL_GetTicks();
      float smoothed_hz = 0.0f;
      float smoothed_simulation_pct = 0.0f;
      float smoothed_render_pct = 0.0f;

      char data[1024];
      char hz_text[64];
      SDL_FRect dataRect = (SDL_FRect){0,0,(float)WIDTH,40};

      Uint64 simulation_start;
      Uint64 simulation_end;
      Uint64 render_start;
      Uint64 render_end;
      Uint64 now_ticks;
      SDL_Event event;
      float* buffer;
      float delta_seconds;
      bool pause = false;

      while(is_opened){
            now_ticks = SDL_GetTicks();
            delta_seconds = (float)(now_ticks - prev_ticks) / 1000.0f;
            
            prev_ticks = now_ticks;
            if(delta_seconds < 0.0f){
                  delta_seconds = 0.0f;
            }
            if(delta_seconds > 0.100f){
                  delta_seconds = 0.100f;
            }

            if(delta_seconds > 0.00001f){
                  float instant_hz = 1.0f / delta_seconds;
                  if(smoothed_hz <= 0.0f){
                        smoothed_hz = instant_hz;
                  }else{
                        smoothed_hz = smoothed_hz * 0.90f + instant_hz * 0.10f;
                  }
            }

            while(SDL_PollEvent(&event)){
                  if (event.type == SDL_EVENT_QUIT) {
                        is_opened = false;
                  }else if( event.type == SDL_EVENT_KEY_DOWN){
                        if(event.key.key == SDLK_P && !event.key.repeat){
                              drawing_mode = (drawing_mode + 1) % 3;
                        }else if(event.key.key == SDLK_SPACE && !event.key.repeat){
                              pause = !pause;
                        }

                        int should_quit = 0;
                        M3D_bind_default_key_down(&engine.camera, &input, (int)event.key.key, event.key.repeat ? 1 : 0, &should_quit);
                        if(should_quit){
                              is_opened = false;
                        }
                  }else if(event.type == SDL_EVENT_KEY_UP){
                        M3D_bind_default_key_up(&input, (int)event.key.key);
                  }else if(event.type == SDL_EVENT_MOUSE_MOTION){
                        M3D_bind_default_mouse_motion(&input, (float)event.motion.xrel, (float)event.motion.yrel);
                  }else if(event.type == SDL_EVENT_MOUSE_WHEEL){
                        M3D_bind_default_mouse_wheel(&engine.camera, event.wheel.y * 10.0f);
                  }
            }
            M3D_apply_input_state(&engine.camera, &input, delta_seconds);

            simulation_start = SDL_GetPerformanceCounter();
            if(!pause){
#if OPTI_SELECTED == 1
                  buffer = calculate_buffer_SIMD(t, RESOLUTION_HEIGHT, RESOLUTION_WIDTH);
#else
                  buffer = calculate_buffer(t, RESOLUTION_HEIGHT, RESOLUTION_WIDTH);
#endif
            
                  update_vector(t, LIFE_TIME);
                  rand_wave(t, FREQUENCY, nb_par_second);
                  t += dt;
            }
            simulation_end = SDL_GetPerformanceCounter();

            render_start = SDL_GetPerformanceCounter();
            M3D_clear_frame(&engine, 0xFFFFFFFF);
            if(drawing_mode == 0){
                  draw_wave_point(&engine, buffer, WAVE_RENDER_STEP);
            }else if(drawing_mode == 1){
                  draw_wave_mesh(&engine, buffer, WAVE_RENDER_STEP);
            }else{
                  draw_wave_triangles(&engine, buffer, WAVE_RENDER_STEP);
            }
            

            snprintf(
                  data,
                  sizeof(data),
                  "mode=%s Rot=%s t=%.2fs waves=%zu cam=(%.2f,%.2f,%.2f) sim=%.0f%% render=%.0f%%",
                  engine.camera.mode == CAM_MODE_ORBIT ? "ORBIT" : "FPS",
                  drawing_mode == 0 ? "POINT" : drawing_mode == 1 ? "MESH" : "TRIANGLES",
                  t,
                  getNbWave(),
                  engine.camera.pos.x,
                  engine.camera.pos.y,
                  engine.camera.pos.z,
                  smoothed_simulation_pct,
                  smoothed_render_pct
            );

            M3D_end_frame(&engine);

            SDL_SetRenderDrawColor(engine.renderer, 255,255,255,255);
            SDL_RenderFillRect(engine.renderer, &dataRect);

            SDL_SetRenderDrawColor(engine.renderer, 50,150,50,255);
            SDL_SetRenderScale(engine.renderer, DEBUG_TEXT_SCALE, DEBUG_TEXT_SCALE);
            SDL_RenderDebugTextFormat(engine.renderer, 5, 5, "%s", data);

            snprintf(hz_text, sizeof(hz_text), "Hz: %.1f", smoothed_hz);
            {
                  int logical_width = (int)((float)engine.camera.width / DEBUG_TEXT_SCALE);
                  int text_x = logical_width - 5 - (int)strlen(hz_text) * 8;
                  if(text_x < 5){
                        text_x = 5;
                  }
                  SDL_RenderDebugTextFormat(engine.renderer, text_x, 5, "%s", hz_text);
            }

            SDL_SetRenderScale(engine.renderer, 1.0f, 1.0f);
            M3D_present_frame(&engine);
            render_end = SDL_GetPerformanceCounter();

            if(perf_frequency != 0){
                  double simulation_seconds = (double)(simulation_end - simulation_start) / (double)perf_frequency;
                  double render_seconds = (double)(render_end - render_start) / (double)perf_frequency;
                  double measured_total = simulation_seconds + render_seconds;

                  if(measured_total > 0.0){
                        float simulation_pct = clamp_percentage((float)((simulation_seconds / measured_total) * 100.0));
                        float render_pct = clamp_percentage(100.0f - simulation_pct);

                        if(smoothed_simulation_pct <= 0.0f && smoothed_render_pct <= 0.0f){
                              smoothed_simulation_pct = simulation_pct;
                              smoothed_render_pct = render_pct;
                        }else{
                              smoothed_simulation_pct = smoothed_simulation_pct * 0.90f + simulation_pct * 0.10f;
                              smoothed_render_pct = smoothed_render_pct * 0.90f + render_pct * 0.10f;
                        }
                  }
            }
      }

      end_simulation();
      M3D_shutdown(&engine);

#if defined(_MSC_VER) && defined(_DEBUG)
      if(_CrtDumpMemoryLeaks()){
            printf("Fuites memoire !\n");
      }
#endif

      return 0;
}
