#include <SDL3/SDL.h>

bool init_SDL_WaveSim3D(int* width, int* height, unsigned int frequency);

void end_SLD_WaveSim3D();

void calculate_texture3D(float* u_buffer, int width, int height);

void calculate_texture_SIMD3D(float* u_buffer, int width, int height);

void render_WaveSim3D(char* data);

void update_frequency3D(unsigned int frequency);

double get_real_frequency3D();

void move_camera_forward();
void move_camera_backward();
void move_camera_right();
void move_camera_left();
void move_camera_up();
void move_camera_down();

void get_camera_coord(int* x, int* y, int* z);