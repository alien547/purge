#ifndef BLUR_H
#define BLUR_H

#include <vector>
#include <SDL.h>

std::vector<float> gaussian_kernel(float sigma, int radius);
void blur_pass(SDL_Surface* surf, const std::vector<float>& kernel, bool vertical);
SDL_Texture* create_blurred_texture(SDL_Surface* src, float blur_strength=1.0f);

#endif

