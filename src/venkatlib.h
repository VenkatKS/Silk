/*
 * venkatlib.h
 *
 *  Created on: Feb 22, 2016
 *      Author: Venkat
 */

#ifndef LAB3_VRTOS_EXTERNAL_LIBRARIES_VENKATWARE_VENKATLIB_H_
#define LAB3_VRTOS_EXTERNAL_LIBRARIES_VENKATWARE_VENKATLIB_H_

#include <stdint.h>

char* integerToString(int i, char b[]);
int TimerFrequencyToPeriod50MHZPLL(int Freq); // 50 mhz
#define mhz50timer  20
#define NSINMS      1000000

#define UNITSOF20NSIN1MS 50000

extern long StartCritical (void);    // previous I bit, disable interrupts
extern void EndCritical(long sr);    // restore I bit to previous value
extern void EnableInterrupts(void);
extern void DisableInterrupts(void);

typedef enum tf
{
	FALSE = 0,
	TRUE
} bool, BOOL;


typedef signed int INT;
typedef unsigned int	UINT;

/* These types are assumed as 8-bit integer */
typedef signed char		CHAR;
typedef unsigned char	UCHAR;
typedef unsigned char	BYTE;

/* These types are assumed as 16-bit integer */
typedef signed short	SHORT;
typedef unsigned short	USHORT;
typedef unsigned short	WORD;

/* These types are assumed as 32-bit integer */
typedef signed long		LONG;
typedef unsigned long	ULONG;
typedef unsigned long	DWORD;


#endif /* LAB3_VRTOS_EXTERNAL_LIBRARIES_VENKATWARE_VENKATLIB_H_ */
