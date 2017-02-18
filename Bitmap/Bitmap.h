/*
 * Vector.h
 *
 *  Created on: Mar 18, 2016
 *      Author: Venkat
 */

#ifndef OS_FILESYS_BITMAP_BITMAP_H_
#define OS_FILESYS_BITMAP_BITMAP_H_
#ifndef LAB3_VRTOS_EXTERNAL_LIBRARIES_VENKATWARE_VENKATLIB_H_
#include "../../Venkatware/venkatlib.h"
#endif
#define WORD_SIZE 32

typedef struct NNODE_BitMap
{
	uint32_t  bitsize;
	uint32_t  wordsize;

	uint32_t* dataarray;
} BitMap;

BitMap* BitMap_Init(uint32_t SizeInWords);
void 	BitMap_SetBit(BitMap* map,  uint32_t BitNum);
void 	BitMap_ClearBit(BitMap* map, uint32_t BitNum);
bool 	BitMap_TestBit(BitMap* map, uint32_t BitNum);
void 	BitMap_DeInit(BitMap* map);
#endif /* OS_FILESYS_BITMAP_BITMAP_H_ */
