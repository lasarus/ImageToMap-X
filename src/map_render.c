#include <stdint.h>
#include <string.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <math.h>

#include "color.h"
#include "nbtsave.h"

int get_block_baseid(int id)
{
  int baseid;
  switch(id)
    {
    case 2: case 110:
      baseid = 1;
      break;

    case 13: case 88: case 12:
      baseid = 2;
      break;

    case 19: case 26: case 30: case 35:
      baseid = 3;
      break;

    case 10: case 11: case 46:
      baseid = 4;
      break;

    case 79:
      baseid = 5;
      break;

    case 41: case 42: case 57: case 71: case 101: case 117: case 118:
      baseid = 6;
      break;

    case 6: case 18: case 31: case 32: case 37: case 38: case 39:
    case 40: case 59: case 81: case 83: case 86: case 91: case 103:
    case 104: case 105: case 106: case 111: case 115: case 122:
      baseid = 7;
      break;

    case 80: case 78:
      baseid = 8;
      break;

    case 82:   case 97:
      baseid = 9;
      break;
	  
    case 3: case 60:
      baseid = 10;
      break;

    case 1: case 4: 
    case 7:  case 14: case 15: case 16: case 21: case 22: case 23: 
    case 24: case 29: case 33: case 34: case 36: case 43: case 44: 
    case 45: case 48: case 49: case 52: case 56: case 61: case 67: 
    case 70: case 73: case 87: case 113: case 114: case 116: case 121:
      baseid = 11;
      break;

    case 8:
    case 9:
      baseid = 12;
      break;

    case 5: case 17: case 25: case 47: case 53: case 54: case 58: 
    case 63: case 64: case 68: case 72: case 84: case 85: case 95: 
    case 96: case 99: case 100: case 107: 
      baseid = 13;
      break;

    default:
      baseid = 0;
    }
  return baseid;
}

block_info_t get_block_info(block_info_t * blocks, int scale, int i, int j)
{
  return blocks[i + j * (int)pow(2, scale) * 128];
}

unsigned char get_block_id_info(block_info_t * blocks, int scale, int x, int z)
{
  int majority_array[256];
  int i, j, k;
  int rx, rz;

  memset(majority_array, 0, 256 * sizeof(int));

  if(z < 0 || z > pow(2, scale) * 128)
    return -1;

  rx = pow(2, scale) * x;
  rz = pow(2, scale) * z;

  for(i = 0; i < pow(2, scale); i++)
      for(j = 0; j < pow(2, scale); j++)
	{
	  block_info_t info = get_block_info(blocks, scale, rx + i, rz + j);
	  majority_array[info.blockid]++;
	}

  for((i = 0, j = -1); i < 256; i++)
    {
      if(majority_array[i] > j)
	{
	  j = majority_array[i];
	  k = i;
	}
    }
  
  return (unsigned char)k;
}

int get_block_h_info(block_info_t * blocks, int scale, int x, int z)
{
  int majority_array[256];
  int i, j, k;
  int rx, rz;
  memset(majority_array, 0, 256 * sizeof(int));

  if(z < 0 || z > pow(2, scale) * 128)
    return -1;

  rx = pow(2, scale) * x;
  rz = pow(2, scale) * z;

  for(i = 0; i < pow(2, scale); i++)
      for(j = 0; j < pow(2, scale); j++)
	{
	  block_info_t info = get_block_info(blocks, scale, rx + i, rz + j);
	  majority_array[info.h]++;
	}

  for((i = 0, j = -1); i < 256; i++)
    {
      if(majority_array[i] > j)
	{
	  j = majority_array[i];
	  k = i;
	}
    }
  
  return k;
}

void render_map(block_info_t * blocks, unsigned char * data, int scale)
{
  int i, j;
  for(i = 0; i < 128; i++)
    for(j = 0; j < 128; j++)
      {
	int baseid;
	int shadow = 1;

	if(get_block_h_info(blocks, scale, i, j - 1) >
	   get_block_h_info(blocks, scale, i, j))
	  shadow = 0;
        else if(get_block_h_info(blocks, scale, i, j) >
	   get_block_h_info(blocks, scale, i, j))
	   shadow = 2;

	baseid = get_block_baseid(get_block_id_info(blocks, scale, i, j));
	
	data[i + j * 128] = (baseid * 4) + shadow;
      }
}
