#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <png.h>

// Function to read a PNG file
int read_png(const uint8_t *data, size_t size) {
    FILE *fp = fmemopen((void *)data, size, "rb");
    if (!fp) {
        // Input is not valid as a file; skip further processing
        return -1;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        return -1;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return -1;
    }

    if (setjmp(png_jmpbuf(png))) {
        // libpng encountered an error; cleanup
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return -1;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    // Access metadata
    png_uint_32 width = png_get_image_width(png, info);
    png_uint_32 height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    printf("Read PNG: width=%u, height=%u, color_type=%u, bit_depth=%u\n", width, height, color_type, bit_depth);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    return 0; // Indicate success
}

// Function to write a PNG file
void write_png(const uint8_t *data, size_t size) {
    FILE *fp = fopen("fuzzed_output.png", "wb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file for writing\n");
        return;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fclose(fp);
        return;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, NULL);
        fclose(fp);
        return;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return;
    }

    png_init_io(png, fp);

    // Use fuzzed data to determine dimensions and color type
    uint32_t width = (data[0] << 8) | data[1];
    uint32_t height = (data[2] << 8) | data[3];
    png_byte bit_depth = data[4] % 2 ? 8 : 16;
    png_byte color_type = data[5] % 4;

    if (width == 0 || height == 0 || width > 1024 || height > 1024) {
        // Invalid dimensions; skip writing
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return;
    }

    png_set_IHDR(
        png, info,
        width, height,
        bit_depth, color_type,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT
    );

    png_write_info(png, info);

    // Generate dummy data for each row
    png_bytep row = (png_bytep)malloc(png_get_rowbytes(png, info));
    if (!row) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        return;
    }

    for (uint32_t y = 0; y < height; y++) {
        memset(row, (y + 1) % 256, png_get_rowbytes(png, info)); // Simple gradient fill
        png_write_row(png, row);
    }

    free(row);

    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(fp);

    printf("Wrote PNG: width=%u, height=%u\n", width, height);
}

int main(int argc, char **argv) {
    // Read input from AFL or other fuzzing tools
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "Error: Could not open input file %s\n", argv[1]);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = (uint8_t *)malloc(size);
    if (!data) {
        fclose(f);
        return 1;
    }

    fread(data, 1, size, f);
    fclose(f);

    // Attempt to read the PNG
    if (read_png(data, size) == 0) {
        // If reading succeeded, attempt to write a PNG
        write_png(data, size);
    } else {
        fprintf(stderr, "Read failed, skipping write operation.\n");
    }

    free(data);
    return 0;
}
