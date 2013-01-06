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

/* This file is a little bit messy, and should be changed into a more dynamic
   way of saving maps as soon as possible */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
#include <assert.h>
#include <gtk/gtk.h>

#include "color.h"

#define CHUNK 1024
#define MAPLEN 0x4060
#define DATALEN 0x38
#define ENDDATALEN 40

int deflatenbt(unsigned char * source, long src_len, FILE * dest, int level)
{
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char out[CHUNK];
	
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
	
  int windowbits = 15 + 16;
	
  ret = deflateInit2(&strm,
		     Z_DEFAULT_COMPRESSION,
		     Z_DEFLATED,
		     windowbits,
		     level,
		     Z_DEFAULT_STRATEGY);
  //deflateInit2(z_streamp strm, int level, int method, int windowBits, int memLevel, int strategy);
  if (ret != Z_OK)
    return ret;
	
  strm.avail_in = src_len;
  strm.next_in = source;
	
  do
    {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = deflate(&strm, Z_FINISH);
      assert(ret != Z_STREAM_ERROR);
      have = CHUNK - strm.avail_out;
      if(fwrite(out, 1, have, dest) != have || ferror(dest))
	{
	  (void)deflateEnd(&strm);
	  return Z_ERRNO;
	}
    } while (strm.avail_out == 0);
	
  (void)deflateEnd(&strm);
  return Z_OK;
}

void nbt_write_raw_string(unsigned char * data, const char * str, int * offset)
{
  int i;
  short len = strlen(str);

  data[*offset + 0] = (len >> 8) & 0xFF;
  data[*offset + 1] = (len >> 0) & 0xFF;
  *offset += 2;
  for(i = 0; i < len; i++)
    {
      data[*offset + i] = str[i];
    }
  *offset += i;
}

void nbt_write_raw_tagstart(unsigned char * data, char type, const char * name, int * offset)
{
  data[*offset] = type;
  *offset += 1;
  nbt_write_raw_string(data, name, offset);
}

void nbt_save_map(const char * filename,
		  char dimension, char scale,
		  int16_t height, int16_t width,
		  int64_t xCenter, int64_t zCenter,
		  unsigned char * mapdata)
{
  int offset = 0;
  unsigned char data[MAPLEN];
  
  /* root compound */
  data[offset] = 0x0A;
  data[offset + 1] = 0x00;
  data[offset + 2] = 0x00;
  offset += 3;
  /* data compound */
  nbt_write_raw_tagstart(data, 0x0A, "data", &offset);//10
  /* scale byte */
  nbt_write_raw_tagstart(data, 0x01, "scale", &offset);//19
  data[offset] = scale;
  offset += 1;
  /* dimension byte */
  nbt_write_raw_tagstart(data, 0x01, "dimension", &offset);//32
  data[offset] = dimension;
  offset += 1;
  /* height short */
  nbt_write_raw_tagstart(data, 0x02, "height", &offset); //43
  data[offset + 0] = (height >> 8) & 0xFF;
  data[offset + 1] = (height >> 0) & 0xFF;
  offset += 2;
  /* width short */
  nbt_write_raw_tagstart(data, 0x02, "width", &offset); //53
  data[offset + 0] = (width >> 8) & 0xFF;
  data[offset + 1] = (width >> 0) & 0xFF;
  offset += 2;
  /* xCenter int */
  nbt_write_raw_tagstart(data, 0x03, "xCenter", &offset); //67
  data[offset + 0] = (xCenter >> 24) & 0xFF;
  data[offset + 1] = (xCenter >> 16) & 0xFF;
  data[offset + 2] = (xCenter >> 8) & 0xFF;
  data[offset + 3] = (xCenter >> 0) & 0xFF;
  offset += 4;
  /* zCenter int */
  nbt_write_raw_tagstart(data, 0x03, "zCenter", &offset); //81
  data[offset + 0] = (zCenter >> 24) & 0xFF;
  data[offset + 1] = (zCenter >> 16) & 0xFF;
  data[offset + 2] = (zCenter >> 8) & 0xFF;
  data[offset + 3] = (zCenter >> 0) & 0xFF;
  offset += 4;
  /* colors byte array */
  nbt_write_raw_tagstart(data, 0x07, "colors", &offset); // 94
  data[offset + 0] = 0x00;
  data[offset + 1] = 0x00;
  data[offset + 2] = 0x40;
  data[offset + 3] = 0x00;
  offset += 4;
  memcpy(data + offset, mapdata, 0x4000);
  offset += 0x4000;
  /* data and root end tag */
  data[offset + 0] = 0x00;
  data[offset + 1] = 0x00;
  printf("offset %X\n", offset);
  
  FILE * dest = fopen(filename, "wb");
  deflatenbt(data, MAPLEN, dest, 9);
  fclose(dest);
}

unsigned char * inflatenbt(FILE * source, long * rsize)
{
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];
  
  unsigned char * inflated = NULL;
  long size = 0;
  
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit2(&strm, 16 + MAX_WBITS);
  if (ret != Z_OK)
    return NULL;
  
  do
    {
      strm.avail_in = fread(in, 1, CHUNK, source);
      if (ferror(source))
	{
	  (void)inflateEnd(&strm);
	  return NULL;
	}
      if (strm.avail_in == 0)
	break;
      strm.next_in = in;
		
      do
	{
	  strm.avail_out = CHUNK;
	  strm.next_out = out;
	  ret = inflate(&strm, Z_NO_FLUSH);
	  assert(ret != Z_STREAM_ERROR);
	  switch (ret)
	    {
	    case Z_NEED_DICT:
	    case Z_DATA_ERROR:
	    case Z_MEM_ERROR:
	      (void)inflateEnd(&strm);
	      return NULL;
	    }
	  have = CHUNK - strm.avail_out;
	  inflated = realloc(inflated, have + size);
	  memcpy(inflated + size, out, have);
	  size += have;
	} while (strm.avail_out == 0);
    } while (ret != Z_STREAM_END);

  (void)inflateEnd(&strm);
  if(Z_STREAM_END)
    {
      *rsize = size;
      return inflated;
    }
  else
    return NULL;
}

void save_raw_map(const char * filename, unsigned char * mapdata)
{
  FILE * dest = fopen(filename, "wb");
  deflatenbt(mapdata, 128 * 128, dest, 9);
  fclose(dest);
}

void load_raw_map(const char * filename, unsigned char * mapdata)
{
  FILE * source = fopen(filename, "rb");
  long size = 0;
  unsigned char * tbuffer = inflatenbt(source, &size);
  memcpy(mapdata, tbuffer, 128 * 128);
  free(tbuffer);
  fclose(source);
}

void save_colors(color_t * colors, char * filename)
{
  FILE * dest = fopen(filename, "wb");
  deflatenbt((unsigned char *)colors, 56 * sizeof(color_t), dest, 9);
  fclose(dest);
  /*FILE * dest = fopen(filename, "wb");
    int i = 0;
    for(i = 0; i < 56; i++)
    {
    fwrite(&(colors[i]), sizeof(color_t), 1, dest);
    }
    fclose(dest);*/
}

void load_colors(color_t * colors, char * filename)
{
  FILE * source = fopen(filename, "rb");
  long size = 0;
  unsigned char * tbuffer = inflatenbt(source, &size);
  if(size == 56 * sizeof(color_t))
    memcpy(colors, tbuffer, 56 * sizeof(color_t));
  free(tbuffer);
  fclose(source);
}


