
#include "../include/Seam_Carving_Sequential.h"

// Deletes all PNG files in the specified directory
void delete_png_files_in_directory(const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        perror("opendir failed");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char file_path[1024];
            snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, entry->d_name);

            if (strstr(entry->d_name, ".png") != NULL) {
                if (remove(file_path) == 0) {
                    // Successfully deleted PNG file
                } else {
                    perror("remove failed");
                }
            }
        }
    }

    closedir(dir);
}

// Reads a PNG file and returns the image data
unsigned char* read_png_sequential(const char* filename, int* width, int* height, int* channels) {
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
    png_bytep* row_pointers = malloc(sizeof(png_bytep) * (*height));
    for (int y = 0; y < *height; y++)
        row_pointers[y] = image_data + y * rowbytes;

    png_read_image(png, row_pointers);

    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);
    free(row_pointers);

    return image_data;
}

// Writes image data to a PNG file
void write_png_sequential(const char* filename, unsigned char* image_data, int width, int height, int channels) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        perror("File opening failed");
        return;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        perror("png_create_write_struct failed");
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        perror("png_create_info_struct failed");
        fclose(fp);
        png_destroy_write_struct(&png, NULL);
        return;
    }

    if (setjmp(png_jmpbuf(png))) {
        perror("Error during PNG creation");
        fclose(fp);
        png_destroy_write_struct(&png, &info);
        return;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    png_bytep row = (png_byte*) malloc(3 * width * sizeof(png_byte));

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

// Computes the energy map of an image based on pixel gradients
void compute_energy_map_sequential(unsigned char* image_data, int width, int height, int channels, unsigned char* energy_map) {
    int gx, gy;
    for (int y = 1; y < height - 1; y++) {
        for (int x = 1; x < width - 1; x++) {
            gx = 0;
            gy = 0;

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

// Computes the seam (vertical path of minimum energy) for image resizing
void compute_seam_sequential(unsigned char* energy_map, int width, int height, int* seam) {
    int* dp = malloc(width * height * sizeof(int));
    int* backtrack = malloc(width * height * sizeof(int));

    for (int x = 0; x < width; x++) {
        dp[x] = energy_map[x];  
        backtrack[x] = -1;
    }

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

    for (int x = 1; x < width; x++) {
        if (dp[(height - 1) * width + x] < min_energy) {
            min_energy = dp[(height - 1) * width + x];
            best_x = x;
        }
    }

    for (int y = height - 1; y >= 0; y--) {
        seam[y] = best_x;
        best_x = backtrack[y * width + best_x];
    }

    free(dp);
    free(backtrack);
}

// Highlights the seam in the image by changing its color to red and saves the result
void highlight_seam_sequential(unsigned char* image_data, int width, int height, int channels, int* seam, const char* output_filename) {
    unsigned char* highlighted_image = malloc(width * height * channels);
    memcpy(highlighted_image, image_data, width * height * channels);

    for (int y = 0; y < height; ++y) {
        int seam_x = seam[y];
        int idx = (y * width + seam_x) * channels;
        highlighted_image[idx] = 255;       
        highlighted_image[idx + 1] = 0;     
        highlighted_image[idx + 2] = 0;     
    }

    write_png_sequential(output_filename, highlighted_image, width, height, channels);
    free(highlighted_image);
}

// Removes the computed seam from the image and saves the result as a new PNG file
void remove_and_save_seam_sequential(unsigned char* image_data, int width, int height, int channels, int* seam, const char* output_filename) {
    unsigned char* new_image_data = malloc((width - 1) * height * channels);

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

    write_png_sequential(output_filename, new_image_data, width - 1, height, channels);
    free(new_image_data);
}

