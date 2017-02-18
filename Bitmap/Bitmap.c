/*
 * Vector.c
 *
 *  Created on: Mar 18, 2016
 *      Author: Venkat
 */

#include <stdint.h>
#include "Bitmap.h"
#include "../../OS_Kernel/MemoryManager/OS_Allocation.h"
#include "../../Venkatware/venkatlib.h"

BitMap* BitMap_Init(uint32_t SizeInWords)
{
	BitMap* NewBitMap = (BitMap*) OS_NewAllocation(sizeof(BitMap) * 1);

	// if memory allocation failed, return immediately
	if (NewBitMap == 0)
	{
		return 0;
	}

	NewBitMap->dataarray = (uint32_t*) OS_NewAllocation(sizeof(uint32_t) * SizeInWords);

	// if memory allocation failed, clean up the created allocation and return
	if (NewBitMap->dataarray == 0)
	{
		OS_DestroyAllocation(NewBitMap, sizeof(BitMap));
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
	OS_DestroyAllocation(map->dataarray, map->wordsize * sizeof(uint32_t));
	OS_DestroyAllocation(map, sizeof(BitMap));

	map = 0;
}

