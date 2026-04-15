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
#include "menu.h"

#define WIDTH 600
#define HEIGHT 600
#define WINDOWED_WIDTH 900
#define WINDOWED_HEIGHT 600

#define DEBUG_TEXT_SCALE 2.0f
#define WAVE_WORLD_SCALE_X 0.03f
#define WAVE_WORLD_SCALE_Y 0.03f
#define WAVE_HEIGHT_SCALE  6.0f
#define WAVE_COLOR_GAIN    4.0f

#define SIMULATION_SPEED_MIN 0.1f
#define SIMULATION_SPEED_MAX 8.0f

typedef enum{
      DRAW_MODE_POINT = 0,
      DRAW_MODE_MESH = 1,
      DRAW_MODE_TRIANGLES = 2
} Draw_Mode;

const char* drawing_mode_label_fn(int value, void* user_data){
      switch(value){
            case DRAW_MODE_POINT:
                  return "POINT";
            case DRAW_MODE_MESH:
                  return "MESH";
            case DRAW_MODE_TRIANGLES:
                  return "TRIANGLES";
            default:
                  return "UNKNOWN";
      }
}

typedef enum{
      OPTI_SCALAIRE = 0,
      OPTI_SIMD = 1
} Optimization_Mode;

const char* optimization_label_fn(int value, void* user_data){
      switch(value){
            case OPTI_SCALAIRE:
                  return "SCALAIRE";
            case OPTI_SIMD:
                  return "SIMD";
            default:
                  return "UNKNOWN";
      }
}

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

static float clamp_simulation_speed(float value){
      if(value < SIMULATION_SPEED_MIN){
            return SIMULATION_SPEED_MIN;
      }
      if(value > SIMULATION_SPEED_MAX){
            return SIMULATION_SPEED_MAX;
      }
      return value;
}

static void on_menu_fullscreen_change(Menu_Item* item, void* user_data){
      if(item == NULL || item->value_ptr == NULL || user_data == NULL){
            return;
      }

      M3D_set_Fullscreen((M3D_Engine*)user_data, (*(bool*)item->value_ptr) ? 1 : 0);
}

static bool apply_simulation_resize(unsigned int new_width, unsigned int new_height){
      if(!resize_wave_simulation((int)new_width, (int)new_height)){
            return false;
      }

      width = (int)new_width;
      height = (int)new_height;
      return true;
}

static bool is_simulation_resize_key(SDL_Keycode key){
      return key == SDLK_LEFT || key == SDLK_RIGHT || key == SDLK_KP_PLUS || key == SDLK_KP_MINUS;
}

static bool is_simulation_size_item(const Menu_Item* item){
      if(item == NULL || item->label == NULL){
            return false;
      }

      return strcmp(item->label, "Sim width") == 0 || strcmp(item->label, "Sim height") == 0;
}

static void apply_pending_simulation_resize(Menu_Params* params, unsigned int wave_simulation_width, unsigned int wave_simulation_height, bool* pending_resize){
      if(params == NULL || pending_resize == NULL || !(*pending_resize)){
            return;
      }

      if(apply_simulation_resize(wave_simulation_width, wave_simulation_height)){
            *pending_resize = false;
      }
}

static void update_overlay_rect(SDL_Window* window, SDL_FRect* rect, int* window_pixel_width){
      int current_width = WIDTH;
      int current_height = HEIGHT;

      if(window != NULL){
            SDL_GetWindowSizeInPixels(window, &current_width, &current_height);
      }

      rect->x = 0.0f;
      rect->y = 0.0f;
      rect->w = (float)current_width;
      rect->h = 40.0f;

      if(window_pixel_width != NULL){
            *window_pixel_width = current_width;
      }
}

static void rand_wave(float current_time, float simulation_delta_seconds, unsigned int waves_per_second){
      if(waves_per_second == 0 || simulation_delta_seconds <= 0.0f){
            return;
      }

      float expected_wave_count = simulation_delta_seconds * (float)waves_per_second;
      unsigned int generated_wave_count = (unsigned int)expected_wave_count;
      float fractional_wave = expected_wave_count - (float)generated_wave_count;

      if(((float)rand() / ((float)RAND_MAX + 1.0f)) < fractional_wave){
            generated_wave_count++;
      }

      for(unsigned int wave_index = 0; wave_index < generated_wave_count; ++wave_index){
            int x = rand() % width;
            int y = rand() % height;
            float spawn_offset = ((float)rand() / ((float)RAND_MAX + 1.0f)) * simulation_delta_seconds;
            add_wave((float)x, (float)y, current_time - spawn_offset);
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
      const uint32_t neutral = 90u;
      float intensity_scale;
      uint32_t intensity;
      uint32_t red;
      uint32_t green;
      uint32_t blue;

      if(amplitude > 1.0f){
            amplitude = 1.0f;
      }else if(amplitude < -1.0f){
            amplitude = -1.0f;
      }

      intensity_scale = fabsf(amplitude) * WAVE_COLOR_GAIN;
      if(intensity_scale > 1.0f){
            intensity_scale = 1.0f;
      }

      intensity = (uint32_t)(intensity_scale * 255.0f);

      if(amplitude > 0.0f){
            red = neutral + ((255u - neutral) * intensity) / 255u;
            green = (neutral * (255u - intensity)) / 255u;
            blue = (neutral * (255u - intensity)) / 255u;
      }else if(amplitude < 0.0f){
            red = (neutral * (255u - intensity)) / 255u;
            green = (neutral * (255u - intensity)) / 255u;
            blue = neutral + ((255u - neutral) * intensity) / 255u;
      }else{
            red = neutral;
            green = neutral;
            blue = neutral;
      }

      return (red << 24) | (green << 16) | (blue << 8) | 0x000000FF;
}

static void draw_wave_triangles(M3D_Engine* engine, const float* buffer, int step){
      for(int y = 0; y < height - step; y += step){
            for(int x = 0; x < width - step; x += step){
                  int idx = y * width + x;
                  int idx_right = y * width + (x + step);
                  int idx_down = (y + step) * width + x;
                  int idx_diag = (y + step) * width + (x + step);
                  uint32_t cell_color;

                  Vect3 p00 = wave_point_to_world(x, y, buffer[idx]);
                  Vect3 p10 = wave_point_to_world(x + step, y, buffer[idx_right]);
                  Vect3 p01 = wave_point_to_world(x, y + step, buffer[idx_down]);
                  Vect3 p11 = wave_point_to_world(x + step, y + step, buffer[idx_diag]);

                  cell_color = wave_color((buffer[idx] + buffer[idx_right] + buffer[idx_down] + buffer[idx_diag]) * 0.25f);

                  M3D_draw_triangle(engine, &p00, &p10, &p11, cell_color, false);
                  M3D_draw_triangle(engine, &p00, &p11, &p01, cell_color, false);
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
      float simulation_time = 0.0f;
      
      
      Uint64 perf_frequency = SDL_GetPerformanceFrequency();

      srand((unsigned)time(NULL));
      M3D_fill_default_init_data(&initData, WINDOWED_WIDTH, WINDOWED_HEIGHT);

      bool is_opened = true;
      if(!M3D_init_custom(&engine, &initData, "Moving Masse", 1)){
            return false;
      }
      width = WIDTH;
      height = HEIGHT;
      if(!init_wave_simulation(width, height)){
            M3D_shutdown(&engine);
            return false;
      }
      add_wave((float)(width / 2), (float)(height / 2), simulation_time);

      M3D_InputState input = {0};
      
      Uint64 previous_counter = SDL_GetPerformanceCounter();
      float smoothed_hz = 0.0f;
      float smoothed_simulation_pct = 0.0f;
      float smoothed_render_pct = 0.0f;
      int window_pixel_width = WIDTH;

      char data[1024];
      char hz_text[64];
      SDL_FRect dataRect = {0};

      Uint64 simulation_start;
      Uint64 simulation_end;
      Uint64 render_start;
      Uint64 render_end;
      Uint64 current_counter;
      SDL_Event event;
      float* buffer;
      float delta_seconds;
      float input_delta_seconds;
      
      

      /**** Menu data ****/
      bool show_menu = false;
      bool is_fullscreen = 1;
      bool pause = false;
      float simulation_speed = 1.0f;
      Draw_Mode drawing_mode = DRAW_MODE_POINT;
      unsigned int resolution_simulation = 3u;
      Optimization_Mode optimization_selected = OPTI_SIMD;
      unsigned int resolution_daffichage = 5u;
      unsigned int wave_simulation_width = WIDTH;
      unsigned int wave_simulation_height = HEIGHT;
      bool pending_simulation_resize = false;
      unsigned int nb_par_second = 1;

      Menu_Params params;
      menu_init(&params);
      menu_add_bool(&params, "Fullscreen", &is_fullscreen, on_menu_fullscreen_change, &engine);
      menu_add_bool(&params, "Pause", &pause, NULL, NULL);
      menu_add_uint(&params, "Sim width", &wave_simulation_width, 1, 10000, 10, NULL, NULL);
      menu_add_uint(&params, "Sim height", &wave_simulation_height, 1, 10000, 10, NULL, NULL);
      menu_add_float(&params, "Sim Speed", &simulation_speed, SIMULATION_SPEED_MIN, SIMULATION_SPEED_MAX, 0.1f, NULL, NULL);
      menu_add_uint(&params, "Waves/s", &nb_par_second, 0, 20, 1, NULL, NULL);
      menu_add_int(&params, "Drawing Res", (int*)&resolution_daffichage, 1, 20, 1, NULL, NULL);
      menu_add_int(&params, "Simulation Res", (int*)&resolution_simulation, 1, 20, 1, NULL, NULL);
      menu_add_enum(&params, "Optimization", (int*)&optimization_selected, 0, 1, 1, optimization_label_fn, NULL, NULL);
      menu_add_enum(&params, "Drawing Mode", (int*)&drawing_mode, 0, 2, 1, drawing_mode_label_fn, NULL, NULL);

      buffer = calculate_buffer(simulation_time, resolution_simulation, resolution_simulation);

      update_overlay_rect(engine.window, &dataRect, &window_pixel_width);

      while(is_opened){
            current_counter = SDL_GetPerformanceCounter();
            if(perf_frequency != 0){
                  delta_seconds = (float)(current_counter - previous_counter) / (float)perf_frequency;
            }else{
                  delta_seconds = 0.0f;
            }

            previous_counter = current_counter;
            if(delta_seconds < 0.0f){
                  delta_seconds = 0.0f;
            }

            input_delta_seconds = delta_seconds;
            if(input_delta_seconds > 0.100f){
                  input_delta_seconds = 0.100f;
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
                  }else if(event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event.type == SDL_EVENT_WINDOW_RESIZED){
                        update_overlay_rect(engine.window, &dataRect, &window_pixel_width);
                  }else if( event.type == SDL_EVENT_KEY_DOWN){
                        if(show_menu && pending_simulation_resize && !is_simulation_resize_key(event.key.key)){
                              apply_pending_simulation_resize(&params, wave_simulation_width, wave_simulation_height, &pending_simulation_resize);
                        }

                        if(event.key.key == SDLK_P && !event.key.repeat){
                              drawing_mode = (drawing_mode + 1) % 3;
                        }else if(event.key.key == SDLK_RETURN && !event.key.repeat && !show_menu){
                              pause = !pause;
                        }else if(event.key.key == SDLK_KP_ENTER && !event.key.repeat){
                              show_menu = !show_menu;
                        }
                        if(show_menu){
                              Menu_Item* selected_item = menu_get_selected(&params);

                              if(is_simulation_size_item(selected_item) && is_simulation_resize_key(event.key.key) && !event.key.repeat){
                                    pending_simulation_resize = true;
                              }

                              menu_bind_default_key_down(&params, (int)event.key.key, event.key.repeat ? 1 : 0);
                        }

                        M3D_bind_default_key_down_camera(&engine, &input, (int)event.key.key, event.key.repeat ? 1 : 0);
                        if(M3D_bind_default_key_down_fullscreen(&engine, &input, (int)event.key.key, event.key.repeat ? 1 : 0)){
                              is_fullscreen = !is_fullscreen;
                        }
                        M3D_bind_default_key_down_show_mouse(&engine, &input, (int)event.key.key, event.key.repeat ? 1 : 0);
                        if(M3D_bind_default_key_down_quit(&engine, &input, (int)event.key.key, event.key.repeat ? 1 : 0)){
                              if(show_menu){
                                    show_menu = false;
                              }else{
                                    is_opened = false;
                              }
                              
                        }
                        
                  }else if(event.type == SDL_EVENT_KEY_UP){
                        if(show_menu && pending_simulation_resize && is_simulation_resize_key(event.key.key)){
                              apply_pending_simulation_resize(&params, wave_simulation_width, wave_simulation_height, &pending_simulation_resize);
                        }
                        M3D_bind_default_key_up(&input, (int)event.key.key);
                  }else if(event.type == SDL_EVENT_MOUSE_MOTION){
                        M3D_bind_default_mouse_motion(&input, (float)event.motion.xrel, (float)event.motion.yrel);
                  }else if(event.type == SDL_EVENT_MOUSE_WHEEL){
                        M3D_bind_default_mouse_wheel(&engine, event.wheel.y * 10.0f);
                  }
            }
            M3D_apply_input_state(&engine.camera, &input, input_delta_seconds);

            simulation_start = SDL_GetPerformanceCounter();
            if(!pause){
                  float simulation_dt = delta_seconds * simulation_speed;

                  simulation_time += simulation_dt;
                  rand_wave(simulation_time, simulation_dt, nb_par_second);

                  if(optimization_selected == OPTI_SIMD){
                        buffer = calculate_buffer_SIMD(simulation_time, resolution_simulation, resolution_simulation);
                  }else{
                        buffer = calculate_buffer(simulation_time, resolution_simulation, resolution_simulation);
                  }
                  update_vector(simulation_time);
            }
            simulation_end = SDL_GetPerformanceCounter();

            render_start = SDL_GetPerformanceCounter();
            M3D_clear_frame(&engine, 0xFFFFFFFF);
            if(drawing_mode == DRAW_MODE_POINT){
                  draw_wave_point(&engine, buffer, resolution_daffichage);
            }else if(drawing_mode == DRAW_MODE_MESH){
                  draw_wave_mesh(&engine, buffer, resolution_daffichage);
            }else{
                  draw_wave_triangles(&engine, buffer, resolution_daffichage);
            }
            

            snprintf(
                  data,
                  sizeof(data),
                  "mode=%s Rot=%s win=%s speed=x%.2f t=%.2fs waves=%zu cam=(%.2f,%.2f,%.2f) sim=%.0f%% render=%.0f%%",
                  engine.camera.mode == CAM_MODE_ORBIT ? "ORBIT" : "FPS",
                  drawing_mode == DRAW_MODE_POINT ? "POINT" : drawing_mode == DRAW_MODE_MESH ? "MESH" : "TRIANGLES",
                  is_fullscreen ? "FULL" : "900x600",
                  simulation_speed,
                  simulation_time,
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
                  int logical_width = (int)((float)window_pixel_width / DEBUG_TEXT_SCALE);
                  int text_x = logical_width - 5 - (int)strlen(hz_text) * 8;
                  if(text_x < 5){
                        text_x = 5;
                  }
                  SDL_RenderDebugTextFormat(engine.renderer, text_x, 5, "%s", hz_text);
            }

            SDL_SetRenderScale(engine.renderer, 1.0f, 1.0f);
            if(show_menu){
                  menu_show(engine.window, &params);
            }
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

      menu_free(&params);
      end_simulation();
      M3D_shutdown(&engine);

#if defined(_MSC_VER) && defined(_DEBUG)
      if(_CrtDumpMemoryLeaks()){
            printf("Fuites memoire !\n");
      }
#endif

      return 0;
}
