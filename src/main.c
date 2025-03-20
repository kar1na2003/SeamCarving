
#include "../include/Seam_Carving_Sequential.h"
#include "../include/Seam_Carving_Parallel.h"

int main() {
    const char* output_dir = "outputs";
    const char* highlighted_seams_dir = "highlighted_seams";

    // Delete all PNG files in both directories
    delete_png_files_in_directory(output_dir);
    delete_png_files_in_directory(highlighted_seams_dir);
    int width, height, channels, choice;
    
    printf("Choose mode:\n1 - Sequential\n2 - Parallel\n> ");
    scanf("%d", &choice);
     
    if(choice == 1){
      unsigned char* image_data = read_png_sequential("input.png", &width, &height, &channels);
      if (!image_data) {
          printf("Failed to read image.\n");
          return 1;
      }

      int iterations;
      printf("Enter the number of iterations: ");
      scanf("%d", &iterations);

      unsigned char* energy_map = malloc(width * height);
      int* seam = malloc(height * sizeof(int));

      char current_image_filename[256];
      snprintf(current_image_filename, sizeof(current_image_filename), "input.png");

      for (int i = 0; i < iterations; i++) {
          image_data = read_png_sequential(current_image_filename, &width, &height, &channels);
          if (!image_data) {
              printf("Failed to read image: %s\n", current_image_filename);
              return 1;
          }

          compute_energy_map_sequential(image_data, width, height, channels, energy_map);
          compute_seam_sequential(energy_map, width, height, seam);

          char highlighted_filename[256];
          snprintf(highlighted_filename, sizeof(highlighted_filename), "highlighted_seams/seam_%d.png", i);
          highlight_seam_sequential(image_data, width, height, channels, seam, highlighted_filename);

          char output_filename[256];
          snprintf(output_filename, sizeof(output_filename), "outputs/output_%d.png", i);
          remove_and_save_seam_sequential(image_data, width, height, channels, seam, output_filename);

          snprintf(current_image_filename, sizeof(current_image_filename), "outputs/output_%d.png", i);
      }

      free(image_data);
      free(energy_map);
      free(seam);
    }
    else if(choice == 2){
      unsigned char* image_data = read_png_parallel("input.png", &width, &height, &channels);
      if (!image_data) {
          printf("Failed to read image.\n");
          return 1;
      }

      int iterations;
      printf("Enter the number of iterations: ");
      scanf("%d", &iterations);

      unsigned char* energy_map = malloc(width * height);
      int* seam = malloc(height * sizeof(int));

      char current_image_filename[256];
      snprintf(current_image_filename, sizeof(current_image_filename), "input.png");

      for (int i = 0; i < iterations; i++) {
          image_data = read_png_parallel(current_image_filename, &width, &height, &channels);
          if (!image_data) {
              printf("Failed to read image: %s\n", current_image_filename);
              return 1;
          }

          compute_energy_map_parallel(image_data, width, height, channels, energy_map);
          compute_seam_sequential(energy_map, width, height, seam);

          char highlighted_filename[256];
          snprintf(highlighted_filename, sizeof(highlighted_filename), "highlighted_seams/seam_%d.png", i);
          highlight_seam_sequential(image_data, width, height, channels, seam, highlighted_filename);

          char output_filename[256];
          snprintf(output_filename, sizeof(output_filename), "outputs/output_%d.png", i);
          remove_and_save_seam_sequential(image_data, width, height, channels, seam, output_filename);

          snprintf(current_image_filename, sizeof(current_image_filename), "outputs/output_%d.png", i);
      }

      free(image_data);
      free(energy_map);
      free(seam);
    }  

  return 0;
}

