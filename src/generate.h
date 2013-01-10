#ifndef GENERATE_H
#define GENERATE_H

void generate_palette(unsigned char * data);
void generate_random_noise(unsigned char * data);
void generate_mandelbrot(unsigned char * data);
void generate_julia(unsigned char * data, double x, double y);
void generate_image(unsigned char * data, const char * filename, color_t * colors, GError ** error);
void generate_image_dithered(unsigned char * data, const char * filename, color_t * colors, GError ** error);

void merge_buffers(unsigned char * data1, unsigned char * data2);

#endif
