#ifndef NBTSAVE_H
#define NBTSAVE_H

typedef struct block_info
{
  int h, blockid;
} block_info_t;

void nbt_save_map(const char * filename, char dimension, char scale, int16_t height, int16_t width, int64_t xCenter, int64_t zCenter, unsigned char * mapdata);
void nbt_load_map(const char * filename, unsigned char * mapdata);
void save_raw_map(const char * filename, unsigned char * mapdata);
void load_raw_map(const char * filename, unsigned char * mapdata);

void save_colors(color_t * colors, char * filename);
void load_colors(color_t * colors, char * filename);

block_info_t * read_region_files(const char * regionpath, const int x, const int z, const int w, const int h);

#endif
