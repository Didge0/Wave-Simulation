#include <SDL3/SDL.h>
#include <windows.h>

bool init_SDL_WaveSim(int* width, int* height, unsigned int frequency);

void end_SLD_WaveSim();

void calculate_texture(float* u_buffer, int width, int height, uint32_t (*toRGB_fct)(float));

void calculate_texture_SIMD(float* u_buffer, int width, int height, __m256i (*toRGB_fct)(__m256));

void show_data(SDL_FRect* dataRect, LARGE_INTEGER* end, LARGE_INTEGER* start, LARGE_INTEGER* freq, float t, size_t nb_wave);

void remaningDelay(LARGE_INTEGER* frame_end, LARGE_INTEGER* frame_start, LARGE_INTEGER* freq, unsigned int selected_freq);

void render_WaveSim(float t, size_t nb_wave, unsigned int selected_freq);
