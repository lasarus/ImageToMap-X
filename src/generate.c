/* This file is part of ImageToMapX.
   ImageToMapX is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   any later version.
 
   ImageToMapX is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with ImageToMapX. If not, see <http://www.gnu.org/licenses/>. */

#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include <errno.h>

#include "data_structures.h"
#include "generate.h"

extern int old_colors;

void generate_palette(unsigned char * data)
{
  int i;
  for(i = 0; i < 128 * 128; i++)
    {
      data[i] = (i % (((old_colors) ? OLD_NUM_COLORS : NUM_COLORS) - 4)) + 4;
    }
}

void generate_random_noise(unsigned char * data)
{
  srand(time(NULL));
  int i;
  for(i = 0; i < 128 * 128; i++)
    {
      data[i] = (rand() % (((old_colors) ? OLD_NUM_COLORS : NUM_COLORS) - 4)) + 4;
    }
}

void generate_mandelbrot(unsigned char * data)
{
  int i;
  for(i = 0; i < 128 * 128; i++)
    data[i] = 0;

  int height = 128;
  int width = 128;

  double MinRe = -2.0;
  double MaxRe = 2.0;
  double MinIm = -2;
  double MaxIm = 2;
  double Re_factor = (MaxRe - MinRe) / (width - 1);
  double Im_factor = (MaxIm - MinIm) / (height - 1);
  int MaxIterations = 30;

  int x, y;
  for(y = 0; y < height; ++y)
    {
      double c_im = MaxIm - y * Im_factor;
      for(x = 0; x < width; ++x)
	{
	  double c_re = MinRe + x * Re_factor;

	  double Z_re = c_re, Z_im = c_im;
	  int n = 0;
	  int isInside = 1;
	  for (; n < MaxIterations; ++n)
	    {
	      double Z_re2 = Z_re * Z_re, Z_im2 = Z_im * Z_im;
	      if (Z_re2 + Z_im2 > 4)
		{
		  isInside = 0;
		  break;
		}
	      Z_im = 2 * Z_re * Z_im + c_im;
	      Z_re = Z_re2 - Z_im2 + c_re;
	    }
	  if(isInside)
	    {
	      data[x + y * 128] = 0;
	    }
	  else
	    {
	      data[x + y * 128] = (unsigned char)((n % (((old_colors) ? OLD_NUM_COLORS : NUM_COLORS) - 4)) + 4);
	    }
	}
    }
}

void generate_julia(unsigned char * data, double c_im, double c_re)
{
  for (int i = 0; i < 128 * 128; i++)
    data[i] = 0;

  int height = 128;
  int width = 128;

  double MinRe = -2.0;
  double MaxRe = 2.0;
  double MinIm = -2;
  double MaxIm = 2;
  double Re_factor = (MaxRe - MinRe) / (width - 1);
  double Im_factor = (MaxIm - MinIm) / (height - 1);
  int MaxIterations = 30;

  int y, x;
  for (y = 0; y < height; ++y)
    {
      for (x = 0; x < width; ++x)
	{
	  double Z_re = MinRe + x * Re_factor, Z_im = MaxIm - y * Im_factor;
	  int n = 0;
	  int isInside = 1;
	  for (; n < MaxIterations; ++n)
	    {
	      double Z_re2 = Z_re * Z_re, Z_im2 = Z_im * Z_im;
	      if (Z_re2 + Z_im2 > 4)
		{
		  isInside = 0;
		  break;
		}
	      Z_im = 2 * Z_re * Z_im + c_im;
	      Z_re = Z_re2 - Z_im2 + c_re;
	    }
	  if (isInside)
	    {
	      data[x + y * 128] = 0;
	    }
	  else
	    {
	      data[x + y * 128] = (unsigned char)((n % (((old_colors) ? OLD_NUM_COLORS : NUM_COLORS) - 4)) + 4);
	    }
	}
    }
}

color_t get_pixel_pixbuf(double x, double y, GdkPixbuf * pixbuf, unsigned char * pixels, int * alpha)
{
  color_t color;
  guchar * p;
  int n_channels;
  p = pixels + ((int)y) * gdk_pixbuf_get_rowstride(pixbuf) + ((int)x) * gdk_pixbuf_get_n_channels(pixbuf);

  color.r = p[0];
  color.g = p[1];
  color.b = p[2];

  n_channels = gdk_pixbuf_get_n_channels(pixbuf);

  if(alpha != NULL)
    {
      if(n_channels == 4)
	*alpha = (p[3] == 0);
      else
	*alpha = 0;
    }

  return color;
}

/* Transformation Matrix: */
/* Y   |0.299    0.587    0.114  ||R|*/
/* U = |-0.14713 -0.28886 0.436  ||G|*/
/* V   |0.615    -0.51499 -0.0001||B|*/
void RGB_to_YUV(int ri, int gi, int bi, float * y, float * u, float * v)
{
  float r, g, b;
  r = ri / 255.;
  g = gi / 255.;
  b = bi / 255.;
  *y = r * 0.299 + g * 0.587 + b * 0.114;
  *u = r * -0.14713 + g* -0.28886 + b * 0.436;
  *v = r * 0.615 + g * -0.51499 + b * -0.0001;
}


double YUV_to_dist(float y1, float u1, float v1, float y2, float u2, float v2)
{
  float yd, ud, vd;
  yd = y1 - y2;
  ud = u1 - u2;
  vd = v1 - v2;

  return sqrt(yd * yd + ud * ud + vd * vd);
}

int closest_color_YUV(int r, int g, int b, color_t * colors)
{
  int i, closest_id = 0;
  double closest_dist = 0xFFFFFFFF, ndist;
  float y, u, v;
  RGB_to_YUV(r, g, b, &y, &u, &v);

  for(i = 4; i < ((old_colors) ? OLD_NUM_COLORS : NUM_COLORS); i++)
    {
      double testr = colors[i].r, testg = colors[i].g, testb = colors[i].b;
      float testy, testu, testv;
      RGB_to_YUV(testr, testg, testb, &testy, &testu, &testv);

      ndist = YUV_to_dist(y, u, v, testy, testu, testv);

      if(ndist < closest_dist)
	{
	  closest_id = i;
	  closest_dist = ndist;
	}
    }

  return closest_id;
}

int closest_color_RGB(int r, int g, int b, color_t * colors)
{
  int i, closest_id = 0;
  double closest_dist = 0xFFFFFFFF, ndist;
  for(i = 4; i < ((old_colors) ? OLD_NUM_COLORS : NUM_COLORS); i++)
    {
      double testr = colors[i].r, testg = colors[i].g, testb = colors[i].b;
      ndist = sqrt(pow(testr - r, 2)
		   + pow(testg - g, 2)
		   + pow(testb - b, 2));

      if(ndist < closest_dist)
	{
	  closest_id = i;
	  closest_dist = ndist;
	}
    }

  return closest_id;
}

int closest_color(int r, int g, int b, color_t * colors, int yuv)
{
  if(yuv == 0)
    return closest_color_RGB(r, g, b, colors);
  else
    return closest_color_YUV(r, g, b, colors);
}

color_t * scale_image(GdkPixbuf * image, int bw, int bh)
{
  double x, y;
  int i;
  double h = gdk_pixbuf_get_height(image), w = gdk_pixbuf_get_width(image);
  double xi = w / (double)bw, yi = h / (double)bh;
  color_t * scaled_image = malloc(bw * bh * sizeof(color_t));
  unsigned char * image_pixels = gdk_pixbuf_get_pixels(image);

  for(i = 0; i < bw * bh; i++)
    {
      x = (double)(i % bw) * xi;
      y = (double)(i / bw) * yi;

      color_t c = get_pixel_pixbuf(x, y, image, image_pixels, NULL);

      scaled_image[i] = c;
    }

  return scaled_image;
}

void generate_image_pixbuf(unsigned char * data, int bw, int bh, int yuv, GdkPixbuf * image, color_t * colors)
{
  double h = gdk_pixbuf_get_height(image), w = gdk_pixbuf_get_width(image);
  double xi = w / (double)bw, yi = h / (double)bh;

  unsigned char * image_pixels = gdk_pixbuf_get_pixels(image);
  double x, y;

  int i = 0, alpha;
  for(i = 0; i < bw * bh; i++)
    {
      x = (double)(i % bw) * xi;
      y = (double)(i / bw) * yi;

      color_t c = get_pixel_pixbuf(x, y, image, image_pixels, &alpha);
      
      if(alpha)
	data[i] = 0;
      else
	data[i] = closest_color(c.r, c.g, c.b, colors, yuv);
    }
}

void generate_image(unsigned char * data, int w, int h, int yuv, const char * filename, color_t * colors, GError ** error)
{
  GdkPixbuf * image = gdk_pixbuf_new_from_file(filename, error);

  if(*error != NULL)
    return;

  generate_image_pixbuf(data, w, h, yuv, image, colors);

  g_object_unref(image);
}

void add_without_overflow(unsigned char * i, int j)
{
  if(*i + j < 256 && *i + j >= 0)
    *i += j;
  else if(*i + j >= 256)
    *i = 0xFF;
  else if(*i + j < 0)
    *i = 0;
}

void generate_image_dithered_pixbuf(unsigned char * data, int w, int h, int yuv, GdkPixbuf * image, color_t * colors)
{
  color_t * image_scaled = scale_image(image, w, h);

  int x, y, i;

  printf("dither!\n");
  for(x = 0; x < w; x++)
    for(y = 0; y < h; y++)
      {
	i = x + y * w;
	double re, ge, be;

	color_t c = image_scaled[i];
	data[i] = closest_color(c.r, c.g, c.b, colors, yuv);
	color_t qc = colors[data[i]];
	re = (c.r - qc.r) / 16.;
	ge = (c.g - qc.g) / 16.;
	be = (c.b - qc.b) / 16.;
	
	if(x != w - 1)
	  {
	    add_without_overflow(&(image_scaled[i + 1].r), (int)(re * 7));
	    add_without_overflow(&(image_scaled[i + 1].g), (int)(ge * 7));
	    add_without_overflow(&(image_scaled[i + 1].b), (int)(be * 7));
	  }

	if(y != h - 1)
	  {
	    if(x != 0)
	      {
		add_without_overflow(&(image_scaled[i + w - 1].r), (int)(re * 3));
		add_without_overflow(&(image_scaled[i + w - 1].g), (int)(ge * 3));
		add_without_overflow(&(image_scaled[i + w - 1].b), (int)(be * 3));
	      }

	    add_without_overflow(&(image_scaled[i + w].r), (int)(re * 5));
	    add_without_overflow(&(image_scaled[i + w].g), (int)(ge * 5));
	    add_without_overflow(&(image_scaled[i + w].b), (int)(be * 5));

	    if(x != w - 1)
	      {
		add_without_overflow(&(image_scaled[i + w].r), (int)(re * 1));
		add_without_overflow(&(image_scaled[i + w].g), (int)(ge * 1));
		add_without_overflow(&(image_scaled[i + w].b), (int)(be * 1));
	      }
	  }
      }

  free(image_scaled);
}

void generate_image_dithered(unsigned char * data, int w, int h, int yuv, const char * filename, color_t * colors, GError ** error)
{
  GdkPixbuf * image = gdk_pixbuf_new_from_file(filename, error);

  if(*error != NULL)
    return;

  generate_image_dithered_pixbuf(data, w, h, yuv, image, colors);
  g_object_unref(image);
}

void merge_buffers(unsigned char * data1, unsigned char * data2)
{
  int i;

  for(i = 0; i < 128 * 128; i++)
    {
      if(data1[i] < 4)
	data1[i] = data2[i];
    }
}
