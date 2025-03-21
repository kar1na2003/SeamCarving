#include "../include/Seam_Carving_Parallel.h"

// Function to parallelize reading the PNG image
unsigned char* read_png_parallel(const char* filename, int* width, int* height, int* channels) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("File opening failed");
        return NULL;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) return NULL;

    png_infop info = png_create_info_struct(png);
    if (!info) return NULL;

    if (setjmp(png_jmpbuf(png))) return NULL;

    png_init_io(png, fp);
    png_read_info(png, info);

    *width  = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);
    *channels = png_get_channels(png, info);

    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth  = png_get_bit_depth(png, info);

    if (bit_depth == 16)
        png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);

    png_read_update_info(png, info);

    int rowbytes = png_get_rowbytes(png, info);
    unsigned char* image_data = malloc(rowbytes * (*height));
    if (!image_data) {
        fclose(fp);
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

    png_bytep* row_pointers_temp = malloc(sizeof(png_bytep) * (*height));
    if (!row_pointers_temp) {
        free(image_data);
        fclose(fp);
        png_destroy_read_struct(&png, &info, NULL);
        return NULL;
    }

    for (int y = 0; y < *height; y++)
        row_pointers_temp[y] = malloc(rowbytes);

    png_read_image(png, row_pointers_temp);

    // Parallel copy into contiguous image_data using OMP tasks
    #pragma omp parallel
    {
        #pragma omp single nowait
        for (int y = 0; y < *height; y++) {
            #pragma omp task firstprivate(y)
            {
                memcpy(image_data + y * rowbytes, row_pointers_temp[y], rowbytes);
                free(row_pointers_temp[y]);  // Free temporary row buffer
            }
        }
    }

    free(row_pointers_temp);
    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);

    return image_data;
}

// Parallelize image writing using OpenMP
void write_png_parallel(const char* filename, unsigned char* image_data, int width, int height, int channels) {
    // Lock to ensure only one thread writes to the file at a time
    #pragma omp critical
    {
        FILE* fp = fopen(filename, "wb");
        if (!fp) {
            perror("File opening failed");
            
        }

        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png) {
            perror("png_create_write_struct failed");
            fclose(fp);
           
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            perror("png_create_info_struct failed");
            fclose(fp);
            png_destroy_write_struct(&png, NULL);
          
        }

        if (setjmp(png_jmpbuf(png))) {
            perror("Error during PNG creation");
            fclose(fp);
            png_destroy_write_struct(&png, &info);
        }

        png_init_io(png, fp);
        png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png, info);

        png_bytep row = (png_byte*) malloc(3 * width * sizeof(png_byte));

        #pragma omp parallel for
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                row[x * 3 + 0] = image_data[(y * width + x) * channels];     
                row[x * 3 + 1] = image_data[(y * width + x) * channels + 1]; 
                row[x * 3 + 2] = image_data[(y * width + x) * channels + 2]; 
            }
            png_write_row(png, row);
        }

        free(row);
        png_write_end(png, NULL);
        fclose(fp);
        png_destroy_write_struct(&png, &info);
    }
}

// Parallelize energy map computation using OpenMP
void compute_energy_map_parallel(unsigned char* image_data, int width, int height, int channels, unsigned char* energy_map) {
    #pragma omp parallel for collapse(2)
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            int gx = 0, gy = 0;

            for (int j = -1; j <= 1; j++) {
                for (int i = -1; i <= 1; i++) {
                    int pixel = (y + j) * width + (x + i);
                    gx += image_data[pixel * channels] * (i);   
                    gy += image_data[pixel * channels] * (j);   
                }
            }

            int energy = sqrt(gx * gx + gy * gy);
            energy_map[y * width + x] = energy;
        }
    }
}

// Parallelize seam computation with OpenMP
void compute_seam_parallel(unsigned char* energy_map, int width, int height, int* seam) {
    int* dp = malloc(width * height * sizeof(int));
    int* backtrack = malloc(width * height * sizeof(int));

    #pragma omp parallel for
    for (int x = 0; x < width; x++) {
        dp[x] = energy_map[x];  
        backtrack[x] = -1;
    }

    #pragma omp parallel for collapse(2)
    for (int y = 1; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int min_energy = dp[(y - 1) * width + x];
            int best_x = x;

            if (x > 0 && dp[(y - 1) * width + x - 1] < min_energy) {
                min_energy = dp[(y - 1) * width + x - 1];
                best_x = x - 1;
            }

            if (x < width - 1 && dp[(y - 1) * width + x + 1] < min_energy) {
                min_energy = dp[(y - 1) * width + x + 1];
                best_x = x + 1;
            }

            dp[y * width + x] = energy_map[y * width + x] + min_energy;
            backtrack[y * width + x] = best_x;
        }
    }

    int min_energy = dp[(height - 1) * width];
    int best_x = 0;

    #pragma omp parallel for
    for (int x = 1; x < width; x++) {
        if (dp[(height - 1) * width + x] < min_energy) {
            min_energy = dp[(height - 1) * width + x];
            best_x = x;
        }
    }

    #pragma omp parallel for
    for (int y = height - 1; y >= 0; y--) {
        seam[y] = best_x;
        best_x = backtrack[y * width + best_x];
    }

    free(dp);
    free(backtrack);
}

// Parallelize seam highlighting with OpenMP
void highlight_seam_parallel(unsigned char* image_data, int width, int height, int channels, int* seam, const char* output_filename) {
    unsigned char* highlighted_image = malloc(width * height * channels);
    memcpy(highlighted_image, image_data, width * height * channels);

    #pragma omp parallel for
    for (int y = 0; y < height; ++y) {
        int seam_x = seam[y];
        int idx = (y * width + seam_x) * channels;
        highlighted_image[idx] = 255;       
        highlighted_image[idx + 1] = 0;     
        highlighted_image[idx + 2] = 0;     
    }

    write_png_parallel(output_filename, highlighted_image, width, height, channels);
    free(highlighted_image);
}

// Parallelize seam removal with OpenMP
void remove_and_save_seam_parallel(unsigned char* image_data, int width, int height, int channels, int* seam, const char* output_filename) {
    unsigned char* new_image_data = malloc((width - 1) * height * channels);

    #pragma omp parallel for
    for (int y = 0; y < height; ++y) {
        int seam_x = seam[y];
        for (int x = 0; x < seam_x; ++x) {
            int src_idx = (y * width + x) * channels;
            int dst_idx = (y * (width - 1) + x) * channels;
            memcpy(&new_image_data[dst_idx], &image_data[src_idx], channels);
        }
        for (int x = seam_x + 1; x < width; ++x) {
            int src_idx = (y * width + x) * channels;
            int dst_idx = (y * (width - 1) + (x - 1)) * channels;
            memcpy(&new_image_data[dst_idx], &image_data[src_idx], channels);
        }
    }

    write_png_parallel(output_filename, new_image_data, width - 1, height, channels);
    free(new_image_data);
}

