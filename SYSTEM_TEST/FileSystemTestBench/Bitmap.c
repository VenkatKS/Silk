/*
 * Vector.c
 *
 *  Created on: Mar 18, 2016
 *      Author: Venkat
 */
#include <stdlib.h>
#include <stdint.h>
#include "Bitmap.h"
#include "venkatlib.h"

BitMap* BitMap_Init(uint32_t SizeInWords)
{
	BitMap* NewBitMap = (BitMap*) malloc(sizeof(BitMap) * 1);

	// if memory allocation failed, return immediately
	if (NewBitMap == 0)
	{
		return 0;
	}

	NewBitMap->dataarray = (uint32_t*) malloc(sizeof(uint32_t) * SizeInWords);

	// if memory allocation failed, clean up the created allocation and return
	if (NewBitMap->dataarray == 0)
	{
		free(NewBitMap);
		return 0;
	}

	NewBitMap->bitsize 	= SizeInWords * WORD_SIZE;
	NewBitMap->wordsize 	= SizeInWords;

	return NewBitMap;
}

void BitMap_SetBit(BitMap* map,  uint32_t BitNum)
{
    map->dataarray[BitNum/WORD_SIZE] |= 1 << (BitNum%WORD_SIZE);  // Set the bit at the k-th position in A[i]
}

void BitMap_ClearBit(BitMap* map, uint32_t BitNum)
{
   map->dataarray[BitNum/WORD_SIZE] &= ~(1 << (BitNum%WORD_SIZE));
}

bool BitMap_TestBit(BitMap* map, uint32_t BitNum)
{
   return (bool) ((map->dataarray[BitNum/WORD_SIZE] & (1 << (BitNum%WORD_SIZE) )) != 0 ) ;
}

void BitMap_DeInit(BitMap* map)
{
	free(map->dataarray);
	free(map);

	map = 0;
}

