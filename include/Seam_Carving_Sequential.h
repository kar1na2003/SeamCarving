#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include <math.h>

unsigned char* read_png_sequential(const char* filename, int* width, int* height, int* channels);

void write_png_sequential(const char* filename, unsigned char* image_data, int width, int height, int channels);

void compute_energy_map_sequential(unsigned char* image_data, int width, int height, int channels, unsigned char* energy_map);

void compute_seam_sequential(unsigned char* energy_map, int width, int height, int* seam);

void highlight_seam_sequential(unsigned char* image_data, int width, int height, int channels, int* seam, const char* output_filename);
 
void remove_and_save_seam_sequential(unsigned char* image_data, int width, int height, int channels, int* seam, const char* output_filename);

