#include <SDL3/SDL.h>

bool init_SDL_WaveSim(int* width, int* height, unsigned int frequency);

void end_SLD_WaveSim();

void calculate_texture(float* u_buffer, int width, int height);

void calculate_texture_SIMD(float* u_buffer, int width, int height);

void render_WaveSim(char* data);

void update_frequency(unsigned int frequency);

double get_real_frequency();
