/*
 * OS_IORedirection.h
 *
 *  Created on: Mar 25, 2016
 *      Author: Venkat
 */

#ifndef OS_FILESYS_OS_IOREDIRECTION_H_
#define OS_FILESYS_OS_IOREDIRECTION_H_

void 	OS_SetOutputToFile(char* FileName);
void 	OS_SetOutputToSerial();
int		printf(const char *format, ...);


#endif /* OS_FILESYS_OS_IOREDIRECTION_H_ */
