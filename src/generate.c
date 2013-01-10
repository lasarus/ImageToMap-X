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

#include "color.h"
#include "generate.h"

void generate_palette(unsigned char * data)
{
  int i;
  for(i = 0; i < 128 * 128; i++)
    {
      data[i] = ((i) % 52) + 4;
    }
}

void generate_random_noise(unsigned char * data)
{
  srand(time(NULL));
  int i;
  for(i = 0; i < 128 * 128; i++)
    {
      data[i] = (rand() % 52) + 4;
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
	      data[x + y * 128] = (unsigned char)((n % (56 - 4)) + 4);
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
	      data[x + y * 128] = (unsigned char)((n % (56 - 4)) + 4);
	    }
	}
    }
}

color_t get_pixel_pixbuf(double x, double y, GdkPixbuf * pixbuf, unsigned char * pixels, int * alpha)
{
  color_t color;
  guchar * p;
  p = pixels + ((int)y) * gdk_pixbuf_get_rowstride(pixbuf) + ((int)x) * gdk_pixbuf_get_n_channels(pixbuf);

  color.r = p[0];
  color.g = p[1];
  color.b = p[2];

  if(alpha != NULL)
    *alpha = (p[3] == 0);

  return color;
}

int closest_color(int r, int g, int b, color_t * colors)
{
  int i, closest_id = 0;
  double closest_dist = 0xFFFFFFFF, ndist;
  for(i = 4; i < 56; i++)
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

int nclosest_color(int r, int g, int b, color_t * colors)
{
  int i, closest_id = 0, rc = closest_color(r, g, b, colors);
  double closest_dist = 0xFFFFFFFF, ndist;
  for(i = 4; i < 56; i++)
    {
      if(i == rc)
	continue;
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

color_t * scale_image(GdkPixbuf * image)
{
  double x, y;
  int i;
  double h = gdk_pixbuf_get_height(image), w = gdk_pixbuf_get_width(image);
  double xi = w / 128., yi = h / 128.;
  color_t * scaled_image = malloc(128 * 128 * sizeof(color_t));
  unsigned char * image_pixels = gdk_pixbuf_get_pixels(image);

  for(i = 0; i < 128 * 128; i++)
    {
      x = (double)(i % 128) * xi;
      y = (double)(i / 128) * yi;

      color_t c = get_pixel_pixbuf(x, y, image, image_pixels, NULL);

      scaled_image[i] = c;
    }

  return scaled_image;
}

void generate_image_pixbuf(unsigned char * data, GdkPixbuf * image, color_t * colors)
{
  double h = gdk_pixbuf_get_height(image), w = gdk_pixbuf_get_width(image);
  double xi = w / 128., yi = h / 128.;

  unsigned char * image_pixels = gdk_pixbuf_get_pixels(image);

  double x, y;

  int i = 0, alpha;
  for(i = 0; i < 128 * 128; i++)
    {
      x = (double)(i % 128) * xi;
      y = (double)(i / 128) * yi;

      color_t c = get_pixel_pixbuf(x, y, image, image_pixels, &alpha);
      
      if(alpha)
	data[i] = 0;
      else
	data[i] = closest_color(c.r, c.g, c.b, colors);
    }
}

void generate_image(unsigned char * data, const char * filename, color_t * colors, GError ** error)
{
  GdkPixbuf * image = gdk_pixbuf_new_from_file(filename, error);

  if(*error != NULL)
    return;

  generate_image_pixbuf(data, image, colors);

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

void generate_image_dithered_pixbuf(unsigned char * data, GdkPixbuf * image, color_t * colors)
{
  color_t * image_scaled = scale_image(image);

  int x, y, i;

  for(x = 0; x < 128; x++)
    for(y = 0; y < 128; y++)
      {
	i = x + y * 128;
	double re, ge, be;

	color_t c = image_scaled[i];
	data[i] = closest_color(c.r, c.g, c.b, colors);
	color_t qc = colors[data[i]];
	re = (c.r - qc.r) / 16.;
	ge = (c.g - qc.g) / 16.;
	be = (c.b - qc.b) / 16.;
	
	if(x != 127)
	  {
	    add_without_overflow(&(image_scaled[i + 1].r), (int)(re * 7));
	    add_without_overflow(&(image_scaled[i + 1].g), (int)(ge * 7));
	    add_without_overflow(&(image_scaled[i + 1].b), (int)(be * 7));
	  }

	if(y != 127)
	  {
	    if(x != 0)
	      {
		add_without_overflow(&(image_scaled[i + 127].r), (int)(re * 3));
		add_without_overflow(&(image_scaled[i + 127].g), (int)(ge * 3));
		add_without_overflow(&(image_scaled[i + 127].b), (int)(be * 3));
	      }

	    add_without_overflow(&(image_scaled[i + 128].r), (int)(re * 5));
	    add_without_overflow(&(image_scaled[i + 128].g), (int)(ge * 5));
	    add_without_overflow(&(image_scaled[i + 128].b), (int)(be * 5));

	    if(x != 127)
	      {
		add_without_overflow(&(image_scaled[i + 128].r), (int)(re * 1));
		add_without_overflow(&(image_scaled[i + 128].g), (int)(ge * 1));
		add_without_overflow(&(image_scaled[i + 128].b), (int)(be * 1));
	      }
	  }
      }

  free(image_scaled);
}

void generate_image_dithered(unsigned char * data, const char * filename, color_t * colors, GError ** error)
{
  GdkPixbuf * image = gdk_pixbuf_new_from_file(filename, error);

  if(*error != NULL)
    return;

  generate_image_dithered_pixbuf(data, image, colors);
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
