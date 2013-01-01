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

void nbt_save_map(const char * filename,
		  char dimension, char scale,
		  int16_t height, int16_t width,
		  int64_t xCenter, int64_t zCenter,
		  unsigned char * mapdata)
{
  unsigned char databuffer[DATALEN] = {0x0A, 0x00, 0x00, 0x0A, 0x00, 0x04, 0x64, 0x61, 0x74, 0x61, 0x01, 0x00, 0x05, 0x73, 0x63, 0x61, 0x6C, 0x65, 0x03 /*byte*/, 0x01, 0x00, 0x09, 0x64, 0x69, 0x6D, 0x65, 0x6E, 0x73, 0x69, 0x6F, 0x6E, 0x00 /*byte*/, 0x02, 0x00, 0x06, 0x68, 0x65, 0x69, 0x67, 0x68, 0x74, /**/0x00, 0x80/*short*/, 0x07, 0x00, 0x06, 0x63, 0x6F, 0x6C, 0x6F, 0x72, 0x73, 0x00, 0x00, 0x40, 0x00};
  unsigned char ensedatabuffer[ENDDATALEN] = {0x03, 0x00, 0x07, 0x78, 0x43, 0x65, 0x6E, 0x74, 0x65, 0x72, /**/0x13, 0x13, 0x13, 0x83/*int*/, 0x02, 0x00, 0x05, 0x77, 0x69, 0x64, 0x74, 0x68, /**/0x00, 0x80/*short*/, 0x03, 0x00, 0x07, 0x7A, 0x43, 0x65, 0x6E, 0x74, 0x65, 0x72, /**/0x00, 0x00, 0x00, 0x4A/*int*/, 0x00, 0x00};
	
  unsigned char buffer[MAPLEN];
	
  //databuffer[18] = scale;
  //databuffer[31] = dimension;
	
  memcpy(buffer, databuffer, DATALEN);
  memcpy(buffer + DATALEN, mapdata, 0x4000);
  memcpy(buffer + DATALEN + 0x4000, ensedatabuffer, ENDDATALEN);
	
  /*int i;
    for(i = 0; i < 0x38; i++)
    {
    if(buffer[i] < 32 || buffer[i] == 127)
    printf(".");
    else
    printf("%c", buffer[i]);
    }*/
	
  FILE * dest = fopen(filename, "wb");
  deflatenbt(buffer, MAPLEN, dest, 9);
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
	
  /*FILE * source = fopen(filename, "rb");
    int i = 0;
    for(i = 0; i < 56; i++)
    {
    fread(&(colors[i]), sizeof(color_t), 1, source);
    }
    fclose(source);*/
}


