#ifndef COLOR_H
#define COLOR_H

typedef struct color
{
  unsigned char r, g, b;
} color_t;

typedef struct map_data
{
  int xpos, zpos;
  int scale;
  int dimension;
} map_data_t;

typedef struct configvars
{
  GdkInterpType interp;
  double zooms;
  double stdzoom;
	
  int maxzoom;
  int minzoom;
} configvars_t;

#endif
