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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
#include <assert.h>
#include <gtk/gtk.h>

#include "data_structures.h"
#include "nbtsave.h"

#define DEBUG_MESSAGE printf("Debug Message line %d file %s function %s\n", __LINE__, __FILE__, __FUNCTION__)

void nbt_jump_raw_list(unsigned char * data, int * offset);

#define CHUNK 8192
#define MAPLEN 0x4060
#define DATALEN 0x38
#define ENDDATALEN 40

typedef enum nbttag
  {
    NBT_END = 0,
    NBT_BYTE = 1,
    NBT_SHORT = 2,
    NBT_INT = 3,
    NBT_LONG = 4,
    NBT_FLOAT = 5,
    NBT_DOUBLE = 6,
    NBT_BYTEARRAY = 7,
    NBT_STRING = 8,
    NBT_LIST = 9,
    NBT_COMPOUND = 10,
    NBT_INTARRAY = 11
  } nbttag_t;

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

unsigned char * inflatenbt(FILE * source, long * rsize, int compression)
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
  if(compression == 1)
    ret = inflateInit2(&strm, 16 + MAX_WBITS);
  else if(compression == 2)
    ret = inflateInit(&strm);
  else
    return NULL;
  if (ret != Z_OK)
    return NULL;
  
  do
    {
      long in_size = CHUNK;
      if(*rsize != -1 && size > *rsize)
	in_size = *rsize - size;
      strm.avail_in = fread(in, 1, in_size, source);
      if(ferror(source))
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

void nbt_write_raw_string(unsigned char * data, const char * str, int * offset)
{
  int i;
  short len = (str == NULL) ? 0 : strlen(str);

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

void nbt_write_raw_tag(unsigned char * data, char type, const char * name, int * offset, ...)
{
  va_list arguments;
  va_start(arguments, offset);

  nbt_write_raw_tagstart(data, type, name, offset);

  int i;
  unsigned char * barray;
  switch(type)
    {
    case 01:
      data[*offset] = va_arg(arguments, int) & 0xFF;
      *offset += 1;
      break;

    case 02:
      i = va_arg(arguments, unsigned int);
      data[*offset + 0] = (i >> 8) & 0xFF;
      data[*offset + 1] = (i >> 0) & 0xFF;
      *offset += 2;
      break;

    case 03:
      i = va_arg(arguments, unsigned int);
      data[*offset + 0] = (i >> 24) & 0xFF;
      data[*offset + 1] = (i >> 16) & 0xFF;
      data[*offset + 2] = (i >> 8) & 0xFF;
      data[*offset + 3] = (i >> 0) & 0xFF;
      *offset += 4;
      break;

    case 07:
      i = va_arg(arguments, unsigned int);
      barray = va_arg(arguments, unsigned char *);
      
      data[*offset + 0] = (i >> 24) & 0xFF;
      data[*offset + 1] = (i >> 16) & 0xFF;
      data[*offset + 2] = (i >> 8) & 0xFF;
      data[*offset + 3] = (i >> 0) & 0xFF;
      *offset += 4;
      memcpy(data + *offset, barray, i);
      *offset += i;
      break;
      
    default:
      break;
    }

  va_end (arguments);
}

void nbt_save_map(const char * filename, char dimension, char scale, int16_t height, int16_t width,
		  int64_t xCenter, int64_t zCenter, unsigned char * mapdata)
{
  int offset = 0;
  unsigned char data[MAPLEN];
  
  nbt_write_raw_tag(data, 0x0A, NULL, &offset);
  nbt_write_raw_tag(data, 0x0A, "data", &offset);
  nbt_write_raw_tag(data, 0x01, "scale", &offset, scale);
  nbt_write_raw_tag(data, 0x01, "dimension", &offset, dimension);
  nbt_write_raw_tag(data, 0x02, "height", &offset, (int)height);
  nbt_write_raw_tag(data, 0x02, "width", &offset, (int)width);
  nbt_write_raw_tag(data, 0x03, "xCenter", &offset, (int)xCenter);
  nbt_write_raw_tag(data, 0x03, "zCenter", &offset, (int)zCenter);
  nbt_write_raw_tag(data, 0x07, "colors", &offset, 0x4000, mapdata);
  /* data and root end tag */
  data[offset + 0] = 0x00;
  data[offset + 1] = 0x00;
  
  FILE * dest = fopen(filename, "wb");
  deflatenbt(data, MAPLEN, dest, 9);
  fclose(dest);
}

void nbt_jump_raw_string(unsigned char * data, int * offset)
{
  int len = 0;

  len |= (data[*offset + 0] & 0xFF) << 8;
  len |= (data[*offset + 1] & 0xFF) << 0;

  *offset += 2;
  *offset += len;
}

char * nbt_read_raw_string(unsigned char * data, int * offset)
{
  char * str;
  int len = 0;

  len |= (data[*offset + 0] & 0xFF) << 8;
  len |= (data[*offset + 1] & 0xFF) << 0;
  *offset += 2;

  str = malloc(len + 1);
  memcpy(str, &(data[*offset]), len);
  str[len] = 0;
  *offset += len;

  return str;
}

void nbt_jump_raw_compound(unsigned char * data, int * offset)
{
  int r = 1, arraylen = 0;
  
  while(r)
    {
      *offset += 1;
      switch(data[*offset - 1])
	{
	case 0:
	  r = 0;
	  break;

	case 1:
	  nbt_jump_raw_string(data, offset);
	  *offset += 1;
	  break;

	case 2:
	  nbt_jump_raw_string(data, offset);
	  *offset += 2;
	  break;

	case 3:
	  nbt_jump_raw_string(data, offset);
	  *offset += 4;
	  break;

	case 4:
	  nbt_jump_raw_string(data, offset);
	  *offset += 8;
	  break;

	case 5:
	  nbt_jump_raw_string(data, offset);
	  *offset += 4;
	  break;

	case 6:
	  nbt_jump_raw_string(data, offset);
	  *offset += 8;
	  break;

	case 7:
	  arraylen = 0;
	  nbt_jump_raw_string(data, offset);
	  arraylen |= (data[*offset + 0] & 0xFF) << 24;
	  arraylen |= (data[*offset + 1] & 0xFF) << 16;
	  arraylen |= (data[*offset + 2] & 0xFF) << 8;
	  arraylen |= (data[*offset + 3] & 0xFF) << 0;
	  *offset += 4;
	  *offset += arraylen;
	  break;

	case 8:
	  nbt_jump_raw_string(data, offset);
	  nbt_jump_raw_string(data, offset);
	  break;

	case 9:
	  nbt_jump_raw_string(data, offset);
	  nbt_jump_raw_list(data, offset);
	  break;

	case 10:
	  nbt_jump_raw_string(data, offset);
	  nbt_jump_raw_compound(data, offset);
	  break;

	case 11:
	  arraylen = 0;
	  nbt_jump_raw_string(data, offset);
	  arraylen |= (data[*offset + 0] & 0xFF) << 24;
	  arraylen |= (data[*offset + 1] & 0xFF) << 16;
	  arraylen |= (data[*offset + 2] & 0xFF) << 8;
	  arraylen |= (data[*offset + 3] & 0xFF) << 0;
	  *offset += 4;
	  *offset += 4 * arraylen;
	  break;
	}
    }
}

void nbt_jump_raw_list(unsigned char * data, int * offset)
{
  int len = 0, arraylen = 0;
  int id;
  int i;

  id = data[*offset];
  len |= (data[*offset + 1] & 0xFF) << 24;
  len |= (data[*offset + 2] & 0xFF) << 16;
  len |= (data[*offset + 3] & 0xFF) << 8;
  len |= (data[*offset + 4] & 0xFF) << 0;
  *offset += 5;

  switch(id)
    {
    case 1:
      *offset += 1 * len;
      break;

    case 2:
      *offset += 2 * len;
      break;

    case 3:
      *offset += 4 * len;
      break;

    case 4:
      *offset += 8 * len;
      break;

    case 5:
      *offset += 4 * len;
      break;

    case 6:
      *offset += 8 * len;
      break;

    case 7:
      for(i = 0; i < len; i++)
	{
	  arraylen |= (data[*offset + 0] & 0xFF) << 24;
	  arraylen |= (data[*offset + 1] & 0xFF) << 16;
	  arraylen |= (data[*offset + 2] & 0xFF) << 8;
	  arraylen |= (data[*offset + 3] & 0xFF) << 0;
	  *offset += 4;
	  *offset += arraylen;
	}
      break;

    case 8:
      for(i = 0; i < len; i++)
	nbt_jump_raw_string(data, offset);
      break;

    case 9:
      for(i = 0; i < len; i++)
	nbt_jump_raw_list(data, offset);
      break;

    case 10:
      for(i = 0; i < len; i++)
        nbt_jump_raw_compound(data, offset);
      break;

    case 11:
      for(i = 0; i < len; i++)
	{
	  arraylen |= (data[*offset + 0] & 0xFF) << 24;
	  arraylen |= (data[*offset + 1] & 0xFF) << 16;
	  arraylen |= (data[*offset + 2] & 0xFF) << 8;
	  arraylen |= (data[*offset + 3] & 0xFF) << 0;
	  *offset += 4;
	  *offset += 4 * arraylen;
	}
      break;
    }
}

void nbt_load_map(const char * filename, unsigned char * mapdata)
{
  int r = 1;
  unsigned char * data;
  long size = -1;
  int offset;
  FILE * dest = fopen(filename, "rb");
  data = inflatenbt(dest, &size, 1);
  fclose(dest);

  offset = 0;

  while(offset < size && r)
    {
      switch(data[offset])
	{
	case 0x0A:
	  offset += 1;
	  nbt_jump_raw_string(data, &offset);
	  break;

	case 0x01:
	  offset += 1;
	  nbt_jump_raw_string(data, &offset);
	  offset += 1;
	  break;

	case 0x02:
	  offset += 1;
	  nbt_jump_raw_string(data, &offset);
	  offset += 2;
	  break;

	case 0x03:
	  offset += 1;
	  nbt_jump_raw_string(data, &offset);
	  offset += 4;
	  break;

	case 0x07:
	  offset += 1;
	  nbt_jump_raw_string(data, &offset);
	  offset += 4;
	  memcpy(mapdata, &(data[offset]), 0x4000);
	  break;

	case 0x00:
	  r = 0;
	  break;

	default:
	  r = 0;
	  break;
	}
      offset += 0;
    }

  free(data);
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
  long size = -1;
  unsigned char * tbuffer = inflatenbt(source, &size, 1);
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
  long size = -1;
  unsigned char * tbuffer = inflatenbt(source, &size, 1);
  if(size == 56 * sizeof(color_t))
    memcpy(colors, tbuffer, 56 * sizeof(color_t));
  free(tbuffer);
  fclose(source);
}

void get_chunk_info(unsigned char ** blocks, int * y, unsigned char * data, int * offset)
{
  int r = 1, arraylen;
  char * name;
  while(r)
    {
      switch(data[*offset])
	{
	case 0x00:
	  r = 0;
	  *offset += 1;
	  break;

	case 0x01:
	  *offset += 1;
	  nbt_jump_raw_string(data, offset);
	  *y = data[*offset];
	  *offset += 1;
	  break;

	case 0x07:
	  *offset += 1;
	  arraylen = 0;
	  name = nbt_read_raw_string(data, offset);
	  arraylen |= (data[*offset + 0] & 0xFF) << 24;
	  arraylen |= (data[*offset + 1] & 0xFF) << 16;
	  arraylen |= (data[*offset + 2] & 0xFF) << 8;
	  arraylen |= (data[*offset + 3] & 0xFF) << 0;
	  *offset += 4;
	  if(strcmp(name, "Blocks") == 0)
	    {
	      *blocks = &(data[*offset]);
	    }
	  free(name);
	  *offset += arraylen;
	  break;
	}
    }
}

void get_chunk_row_info(int * h, int * id, unsigned char ** blocks, int * ylist, int count, int localx, int localz)
{
  int i, j, r = 1, water = 0;

  *h = 0;
  *id = 0;

  for(i = 0; i < count && r; i++)
    {
      for(j = 15; j >= 0; j--)
	{
	  if(blocks[i][j * 16 * 16 + localz * 16 + localx] != 0)
	    {
	      if(!water)
		*id = blocks[i][j * 16 * 16 + localz * 16 + localx];
	      *h = j + ylist[i] * 16;
	      if(blocks[i][j * 16 * 16 + localz * 16 + localx] != 8 &&
		 blocks[i][j * 16 * 16 + localz * 16 + localx] != 9)
		{
		  r = 0;
		  break;
		}
	      water = 1;
	    }
	} 
    }
}

block_info_t * read_region_files(const char * regionpath, const int x, const int z, const int w, const int h)
{
  int startrx, startrz, endrx, endrz;
  int startcx, startcz, endcx, endcz;
  volatile int ri /*region x*/, rj /*reigon z*/;
  volatile int ci /*chunk  x*/, cj /*chunk  y*/;
  volatile int i  /*block  x*/, j  /*block  z*/;
  char pathbuffer[256];
  FILE * regionfile;
  block_info_t * rmap = malloc(w * h * sizeof(block_info_t));
  memset(rmap, 0, w * h * sizeof(block_info_t));
  
  startrx = x >> 9;
  startrz = z >> 9;
  endrx = (x + w) >> 9;
  endrz = (z + h) >> 9;

  startcx = x >> 4;
  startcz = z >> 4;
  endcx = (x + w) >> 4;
  endcz = (z + h) >> 4;
  
  for(ri = startrx; ri <= endrx; ri++)
    for(rj = startrz; rj <= endrz; rj++)
      {
        sprintf(pathbuffer, "%s/r.%i.%i.mca", regionpath, ri, rj);
        regionfile = fopen(pathbuffer, "rb");
	if(regionfile == NULL)
	    continue;
	
	for(ci = 0; ci < 32; ci++)
	  for(cj = 0; cj < 32; cj++)
	    {
	      uint32_t header;
	      uint32_t lenght;
	      long lenghtv;
	      //unsigned char usedsectors;
	      unsigned char compression;
	      unsigned char * data = NULL;
	      int offset;
	      int bufferoffset = 0;
	      int r = 1, arraylen = 0;

	      if(!((ci + ri * 32 >= startcx) && (ci + ri * 32 < endcx) && (cj + rj * 32 >= startcz) && (cj + rj * 32 < endcz)))
		continue;
	      
	      fseek(regionfile, (ci + cj * 32) * sizeof(uint32_t), SEEK_SET);
	      
	      if (fread(&header, sizeof(uint32_t), 1, regionfile)) {}
	      header = ((header >> 24) & 0xFF)
		| ((header >> 8) & 0xFF00)
		| ((header << 8) & 0xFF0000)
		| ((header << 24) & 0xFF000000);
	      
	      //usedsectors = header & 0xFF;
	      offset = header >> 8;
	      
	      if(offset == 0)
		continue;
	      
	      fseek(regionfile, offset * 4096, SEEK_SET);
	      if (fread(&lenght, sizeof(uint32_t), 1, regionfile)) {}
	      lenght = ((lenght >> 24) & 0xFF)
		| ((lenght >> 8) & 0xFF00)
		| ((lenght << 8) & 0xFF0000)
		| ((lenght << 24) & 0xFF000000);
	      if (fread(&compression, 1, 1, regionfile)) {}
	      
	      lenghtv = lenght;
	      data = inflatenbt(regionfile, &lenghtv, compression);
	      
	      
	      bufferoffset += 4;
	      nbt_jump_raw_string(data, &bufferoffset);
	      
	      while(bufferoffset < lenghtv && r)
		{
		  char * name;
		  nbttag_t tagid = (nbttag_t)data[bufferoffset];
		  bufferoffset += 1;
		  if(tagid != NBT_END)
		    name = nbt_read_raw_string(data, &bufferoffset);
		  switch(tagid)
		    {
		    case NBT_BYTE:
		      bufferoffset += 1;
		      break;
		      
		    case NBT_SHORT:
		      bufferoffset += 2;
		      break;
		      
		    case NBT_INT:
		      bufferoffset += 4;
		      break;
		      
		    case NBT_LONG:
		      bufferoffset += 8;
		      break;
		      
		    case NBT_FLOAT:
		      bufferoffset += 4;
		      break;
		      
		    case NBT_DOUBLE:
		      bufferoffset += 8;
		      break;
		      
		    case NBT_BYTEARRAY:
		      arraylen = 0;
		      arraylen |= (data[bufferoffset + 0] & 0xFF) << 24;
		      arraylen |= (data[bufferoffset + 1] & 0xFF) << 16;
		      arraylen |= (data[bufferoffset + 2] & 0xFF) << 8;
		      arraylen |= (data[bufferoffset + 3] & 0xFF) << 0;
		      bufferoffset += 4;
		      bufferoffset += arraylen;
		      break;
		      
		    case NBT_STRING:
		      nbt_jump_raw_string(data, &bufferoffset);
		      break;
		      
		    case NBT_LIST:
		      {
		        int count = 0;
			if(strcmp(name, "Sections") == 0)
			  {
			    bufferoffset += 1;
			    count |= (data[bufferoffset + 0] & 0xFF) << 24;
			    count |= (data[bufferoffset + 1] & 0xFF) << 16;
			    count |= (data[bufferoffset + 2] & 0xFF) << 8;
			    count |= (data[bufferoffset + 3] & 0xFF) << 0;
			    bufferoffset += 4;
			    
			    int ylist[count];
			    unsigned char * blocks[count];
			    for(i = 0; i < count; i++)
			      {
			        get_chunk_info(&(blocks[i]), &(ylist[i]), data, &bufferoffset);
			      }

			    for(i = 0; i < count; i++)
			      for(j = 0; j < count - 1; j++)
				{
				  int temp;
				  unsigned char * tempd;
				  if(ylist[j] < ylist[j + 1])
				    {
				      temp = ylist[j];
				      ylist[j] = ylist[j + 1];
				      ylist[j + 1] = temp;

				      tempd = blocks[j];
				      blocks[j] = blocks[j + 1];
				      blocks[j + 1] = tempd;
				    }
				}

			    if(count != 0)
			      for(i = 0; i < 16; i++)
				for(j = 0; j < 16; j++)
				  {
				    int globalx, globalz;
				    int id, bh;
				    
				    globalx = i + ci * 16 + ri * 512;
				    globalz = j + cj * 16 + rj * 512;
				    
				    if((globalx >= x) && (globalx < x + w) && (globalz >= z) && (globalz < z + h))
				    {
				      get_chunk_row_info(&bh, &id, blocks, ylist, count, i, j);
				      rmap[(globalx - x) + (globalz - z) * w].h = bh;
				      rmap[(globalx - x) + (globalz - z) * w].blockid = id;
				    }
				  }
			    r = 0;
			  }
			else
			  nbt_jump_raw_list(data, &bufferoffset);
		      }
		      break;
		      
		    case NBT_COMPOUND:
		      nbt_jump_raw_compound(data, &bufferoffset);
		      break;
		      
		    case NBT_INTARRAY:
		      arraylen = 0;
		      arraylen |= (data[bufferoffset + 0] & 0xFF) << 24;
		      arraylen |= (data[bufferoffset + 1] & 0xFF) << 16;
		      arraylen |= (data[bufferoffset + 2] & 0xFF) << 8;
		      arraylen |= (data[bufferoffset + 3] & 0xFF) << 0;
		      bufferoffset += 4;
		      bufferoffset += 4 * arraylen;
		      break;
		      
		    case NBT_END:
		      r = 0;
		      break;
		    }
		}
	      free(data);
	    }
	fclose(regionfile);
      }
  return rmap;
}
