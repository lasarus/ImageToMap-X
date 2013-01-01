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

color_t get_pixel_pixbuf(double x, double y, GdkPixbuf * pixbuf, unsigned char * pixels)
{
  color_t color;
  guchar * p;
  p = pixels + ((int)y) * gdk_pixbuf_get_rowstride(pixbuf) + ((int)x) * gdk_pixbuf_get_n_channels(pixbuf);

  color.r = p[0];
  color.g = p[1];
  color.b = p[2];

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

void generate_image(unsigned char * data, const char * filename, color_t * colors, GError ** error)
{
  GdkPixbuf * image = gdk_pixbuf_new_from_file(filename, error);
	
  if(*error != NULL)
    return;
	
  double h = gdk_pixbuf_get_height(image), w = gdk_pixbuf_get_width(image);
  double xi = w / 128., yi = h / 128.;
	
  unsigned char * image_pixels = gdk_pixbuf_get_pixels(image);
	
  double x, y;
	
  int i = 0;
  for(i = 0; i < 128 * 128; i++)
    {
      x = (double)(i % 128) * xi;
      y = (double)(i / 128) * yi;
		
      color_t c = get_pixel_pixbuf(x, y, image, image_pixels);
		
      data[i] = closest_color(c.r, c.g, c.b, colors);
    }
	
  g_object_unref(image);
}

