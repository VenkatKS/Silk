/*
 * OS_FileSystem.h
 *
 *  Created on: Mar 15, 2016
 *      Author: Venkat
 */

#ifndef OS_FILESYS_OS_FILESYSTEM_H_
#define OS_FILESYS_OS_FILESYSTEM_H_
#ifndef LAB3_VRTOS_EXTERNAL_LIBRARIES_VENKATWARE_VENKATLIB_H_
#include "../Venkatware/venkatlib.h"
#endif


// Initialization Routine For the File System
void OSFS_Init();

//***************************************** File System Definitions *****************************************//

#define DRIVENUM 0
#define SUPER_BLOCK_SECTOR_NUM 0										// The location on sector where the super block resides
#define INODE_BITMAP_SECTOR_NUM 1
#define DATA_BITMAP_SECTOR_NUM 2
#define INODE_BLOCK_START_NUM 3										// Where the first inode resides

#define MAX_INODE_COUNT	3950											// Maximum number of nnodes created, and thus max number of files
#define MAX_BLOCKS_TRACKED 3950										// Maximum number of blocks begin tracked

#define DATA_BITMAP_SIZE_IN_WORDS ((MAX_BLOCKS_TRACKED/(sizeof(uint32_t)*8)) + 1)	// We're tracking N blocks.
																	// We need 1 bit per block and there's 32 bits per int.
#define INODE_BITMAP_SIZE_IN_WORDS ((MAX_INODE_COUNT/(sizeof(uint32_t)*8)) + 1)

#define SECTOR_SIZE	512
#define SIZE_OF_FLASH_BLOCK SECTOR_SIZE									// Make sure the block is the same size as the sector

// inode properties
#define MAX_FILE_SECTORS	 	22											// Max number of sectors a file can use (25 * 512 = 12.8 kB)
#define MAX_FILE_NAME_CHARS	10											// Max size of file names
#define INODE_SIZE	114
#define INODES_PER_SECTOR 4
#define TOTAL_INODE_SECTORS (MAX_INODE_COUNT/INODES_PER_SECTOR) + 1		// The plus one compensates for the .5 that might occur

// Justifications:
// We want both the inode bitmap and the data bitmap to fit into individual blocks, so the max size they can be is 508 bytes.
// This is due to the fact that we're using the FLASH_BLOCK struct below and that has an overhead of 4 bytes (space per physical block
// is 512 bytes). Thus, we can index 508 * 8 = 4064. For posterity, we will index 3800 blocks. This will allow us to acquire space for
// 2022400 bytes on disk (2.0 MB). This will give us enough space for the bitmap's overhead as well (8 bytes).

typedef struct nRTOS_FileNode
{											 // Total: 114 bytes per inode
	uint32_t INODE_NUM;						 // 4 bytes
	uint32_t FILE_BYTES;						 // 4 bytes; Increments of SECTOR_SIZE. Directly indicates how many blocks are used by file.
	uint32_t BYTES_USED;						 // 4 bytes; The amount of bytes used by data stored within the file. Once it hits n*SECTOR_SIZE, need to expand file
	uint32_t LATEST_CURSOR;					 // 4 bytes; The last written area of file (so that append can start appending from there
	char		 FILE_NAME[MAX_FILE_NAME_CHARS];	 // 10 bytes
	uint32_t BLOCKS_USED[MAX_FILE_SECTORS];   // 88 bytes
} INODE;

typedef struct nRTOS_FileInfo
{
	INODE*		FileInode;						// Inode associated with the file. Needs to be resident in memory.
} FILE;

// Total inode space: 3950 * 114 = 450300 bytes (450 KB of inodes).

// Superblock definition. Contains properties about the file system.
// NOTE: If this block is updated, be sure to update the read and write systems as well
struct nRTOS_SuperBlock
{
	uint32_t NumInodes;
	uint32_t NumDataBlocks;

	uint32_t InodeStartBlock;


};

// Allows us to read and write individual structures into nonvolatile storage
#define DATA_STORAGE SIZE_OF_FLASH_BLOCK - sizeof(uint32_t)
typedef struct nRTOS_Block
{
	uint32_t SizeInBytes;					// Size of valid data in this block

	uint8_t	 Data[DATA_STORAGE];	// Remaining size of the block

} FLASH_BLOCK;

typedef enum nRTOS_File_Errors
{
	FILE_ALREADY_EXISTS = 1,
	FILE_DOES_NOT_EXIST,
	FILE_INIT_FAILED,
	FILE_NAME_TOO_LONG,
	FILE_OK
} FileError;
#endif /* OS_FILESYS_OS_FILESYSTEM_H_ */

//***************************************** File System Function Calls ****************************************//

bool 		OSFS_Format();				// Call this whenever you want to erase the entire disk
FILE* 		OSFS_Create(char* fileName);	// Call this whenever a new file needs to be created
FILE* 		OSFS_Open(char* fileName);	// Call this wehnever a file already created needs to be opened
int32_t    	OSFS_Read(FILE* fileDescriptor, BYTE* Buffer, uint32_t numBytes, uint32_t Offset);
bool 		OSFS_Write(FILE* fileDescriptor, BYTE* Buffer, uint32_t numBytes, uint32_t Offset);
bool 		OSFS_Append(FILE* fileDescriptor, BYTE* buffer, uint32_t numBytes);
bool 		OSFS_Close(FILE* fileToClose);
bool 		OSFS_Delete(char* fileName);

// File Information functions
uint32_t 	GetFileSize(FILE* fileToEval); // Returns the size of the file currently held



void 		SerialListFiles();			// Call this whenever you want to output the directory list to the serial output
void 		SerialPrintFile(FILE* fileToPrint);
FileError 	OSFS_GetError();				// Returns the reason why the file was not created/opened/closed/etc.

