
#include <omp.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

// Function to parallelize reading the PNG image
unsigned char* read_png_parallel(const char* filename, int* width, int* height, int* channels);

void write_png_parallel(const char* filename, unsigned char* image_data, int width, int height, int channels);

void compute_energy_map_parallel(unsigned char* image_data, int width, int height, int channels, unsigned char* energy_map);

void compute_seam_parallel(unsigned char* energy_map, int width, int height, int* seam);

void highlight_seam_parallel(unsigned char* image_data, int width, int height, int channels, int* seam, const char* output_filename);

void remove_and_save_seam_parallel(unsigned char* image_data, int width, int height, int channels, int* seam, const char* output_filename);

