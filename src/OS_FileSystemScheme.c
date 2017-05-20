#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "venkatlib.h"
#include "Bitmap.h"
#include "OS_FileSystemScheme.h"

bool DiskInitialized =  FALSE;

BitMap* DataBitMap;
BitMap* InodeBitMap;

struct nRTOS_SuperBlock FileSystemProperties;

// Spoofed SD Card
char SDCARD_SPOOF[] = "myfilesystem.store";

// Forward declarations
bool 		_WriteToFile(BYTE* InputBuffer, uint64_t BlockNum);						// Write provided buffer to sector number
bool 		_ReadFromFile(BYTE* OutputBuffer, uint64_t BlockNum);					// Read sector number to provided buffer
// Update provided BitMap struct with sector data from sector number
bool 		_TranscribeBitMap(BYTE* blockToUse, BitMap* mapToUpdate, uint32_t NumBytes);

// Inode private declarations
uint32_t 	_GetNextFreeInode();													// Retrieve the next free inode by checking the bitmap
bool 		_CheckInodeOccupancy(uint32_t Inodenum);								// Check and see if the provided inode is full
void 		_MarkInodeAsOccupied(uint32_t InodeNum);								// Mark the volatile inode as occupied
void 		_MarkInodeAsFree(uint32_t InodeNum);									// Mark the volatile inode as free
void 		_UpdateNonVolatileInodeCopy(uint32_t InodeNum, INODE* volatileCopy);	// Write the provided inode to the non-volatile memory
int32_t 	_GetNextOccupiedInode(uint32_t StartLocation);							// Get the next occupied node from the provided node to get (-1 if none)
void 		_ReadInodeFromNonVolatileMemory(uint32_t InodeNum, INODE* location);	// Update provided memory address by reading data from non-volatile memory for particular inode
int32_t 	_GetInodeFromFileName(char* fileName);									// Returns the inode number of the inode that is associated with this filename (-1 if none).

// Block private declarations
uint32_t 	_GetNextFreeBlock();													// Returns the next avaliable block number (Starts at 0, iterates to the end). --> Panics!
void 		_MarkBlockAsOccupied(uint32_t BlockNum);								// Marks and returns the provided block as being occupied in the volatile bitmap
void 		_MarkBlockAsFree(uint32_t BlockNum);
void 		_UpdateNonVolatileDataBlockCopy(uint32_t BlockNum, BYTE* volatileCopy); // Update the copy of the block in disk

// BitMap private declarations
void 		_FlushBitMapToDisk();													// Writes the volatile copy of the bitmaps to non-volatile storage

// Used for temporary item movement
INODE	    CurrentNode;															// Use this to interact with the inode storage
BYTE 		tempBlock[SECTOR_SIZE];													// Use this to interact with any storage sector

//***************************************** Public Functions ************************************//

// Precondition: Atomic (since called from OS_Init, no threads should have been launched yet).
void OSFS_Init()
{

	// Read the superblock into memory
	memset(&tempBlock, 0, SECTOR_SIZE);
	if (_ReadFromFile((BYTE*) &tempBlock, SUPER_BLOCK_SECTOR_NUM))
	{
		memcpy(&FileSystemProperties, &tempBlock, sizeof(struct nRTOS_SuperBlock));
	}
	else
	{
		printf("File does not exist\n");
		exit(-1);
	}

	DiskInitialized = TRUE;

	// Transcribe the bitmaps into memory
	DataBitMap = BitMap_Init(DATA_BITMAP_SIZE_IN_WORDS);
	InodeBitMap = BitMap_Init(INODE_BITMAP_SIZE_IN_WORDS);

	if (DataBitMap == 0 || InodeBitMap == 0) exit(-1);

	memset(&tempBlock, 0, SECTOR_SIZE);
	if (_ReadFromFile((BYTE*) &tempBlock, INODE_BITMAP_SECTOR_NUM))
	{
		_TranscribeBitMap((BYTE*)&tempBlock, InodeBitMap, sizeof(BitMap));
	}
	else
	{
        exit(-1);
	}

	memset(&tempBlock, 0, SECTOR_SIZE);
	if (_ReadFromFile((BYTE*) &tempBlock, DATA_BITMAP_SECTOR_NUM))
	{
		_TranscribeBitMap((BYTE*)&tempBlock, DataBitMap, sizeof(BitMap));
	}
	else
	{
		exit(-1);
	}

}

// Precondition: Called BEFORE OS_Init! Make sure that OS_Init has not been called before this has been called!

bool OSFS_Format()
{
	if (DiskInitialized == TRUE) return FALSE;

	const struct nRTOS_SuperBlock PermanentSuperBlock = {MAX_INODE_COUNT, MAX_BLOCKS_TRACKED, INODE_BLOCK_START_NUM};

	// Initialize the disk with the SuperBlock parameters so we know what exactly we're dealing with
	memset(&tempBlock, 0, SECTOR_SIZE);
	memcpy(&tempBlock, &PermanentSuperBlock, sizeof(struct nRTOS_SuperBlock));

	if (_WriteToFile((BYTE*) &tempBlock, SUPER_BLOCK_SECTOR_NUM) == FALSE)
	{
		exit(-1);
	}

	memset(&tempBlock, 0, SECTOR_SIZE);

	DataBitMap 	= BitMap_Init(DATA_BITMAP_SIZE_IN_WORDS);
	InodeBitMap 	= BitMap_Init(INODE_BITMAP_SIZE_IN_WORDS);

	if (DataBitMap == 0 || InodeBitMap == 0) exit(-1);

	// Store the data bit map into the proper area
	BitMap_SetBit(DataBitMap, SUPER_BLOCK_SECTOR_NUM); 				// The superblock location is taken.
	BitMap_SetBit(DataBitMap, INODE_BITMAP_SECTOR_NUM);				// The inode bitmap block is taken.
	BitMap_SetBit(DataBitMap, DATA_BITMAP_SECTOR_NUM);				// The data bitmap block is taken.
	BitMap_SetBit(DataBitMap, MAX_INODE_COUNT);						// Debugging instrument

	int inodeSectors = 0;
	for (inodeSectors = 0; inodeSectors < TOTAL_INODE_SECTORS; inodeSectors++)
	{
		BitMap_SetBit(DataBitMap, INODE_BLOCK_START_NUM + inodeSectors);		// Mark all the inode sectors as occupied.
	}

	//Since the bitmap has a dynamic array, we need to transcribe it manually
	//CurrentBlock.SizeInBytes = sizeof(BitMap) + (DATA_BITMAP_SIZE_IN_WORDS * sizeof(uint32_t)) - sizeof(uint32_t*); // everything but the pointer
	memset(&tempBlock, 0, SECTOR_SIZE);
	memcpy(&tempBlock[0], DataBitMap, sizeof(BitMap) - sizeof(uint32_t*)); // Store everything but the pointer.

	uint32_t SizeUsed = sizeof(BitMap) - sizeof(uint32_t*);	// The amount of sized used so far in the struct

	memcpy(((&tempBlock[SizeUsed])), DataBitMap->dataarray, DATA_BITMAP_SIZE_IN_WORDS * sizeof(uint32_t)); // Copy over the dynamic data

	if (_WriteToFile((BYTE*) &tempBlock, DATA_BITMAP_SECTOR_NUM) == FALSE)
	{
			exit(-1);
	}

	memset(&tempBlock, 0, SECTOR_SIZE);

	// Store the inode bit map into the proper area

	if (DataBitMap == 0 || InodeBitMap == 0) exit(-1);

	// Since the bitmap has a dynamic array, we need to transcribe it manually
	//CurrentBlock.SizeInBytes = sizeof(BitMap) + (INODE_BITMAP_SIZE_IN_WORDS * sizeof(uint32_t)) - sizeof(uint32_t*); // everything but the pointer

	memcpy(&tempBlock[0], InodeBitMap, sizeof(BitMap) - sizeof(uint32_t*)); // Store everything but the pointer.

	SizeUsed = sizeof(BitMap) - sizeof(uint32_t*);	// The amount of sized used so far in the struct

	memcpy(((&tempBlock[SizeUsed])), InodeBitMap->dataarray, INODE_BITMAP_SIZE_IN_WORDS * sizeof(uint32_t)); // Copy over the dynamic data

	if (_WriteToFile((BYTE*) &tempBlock, INODE_BITMAP_SECTOR_NUM) == FALSE)
	{
			exit(-1);
	}

	memset(&tempBlock, 0, SECTOR_SIZE);

	// Delete the allocations
	BitMap_DeInit(DataBitMap);
	BitMap_DeInit(InodeBitMap);


	// Start creating and storing the inodes (will take a while)
	uint32_t InodeIterator = 0;
	memset(&CurrentNode, 0, sizeof(INODE));
	memset(&tempBlock, 0, SECTOR_SIZE);

	for (InodeIterator = 0; InodeIterator < TOTAL_INODE_SECTORS; InodeIterator++)
	{
		uint32_t InsideIterator = 0;
		for(InsideIterator = 0; InsideIterator < INODES_PER_SECTOR; InsideIterator++)
		{
			CurrentNode.INODE_NUM = (INODES_PER_SECTOR * InodeIterator) + InsideIterator;
			memcpy(&tempBlock[InsideIterator * sizeof(INODE)], &CurrentNode, sizeof(INODE));
			memset(&CurrentNode, 0, sizeof(INODE));
		}

		if (_WriteToFile((BYTE*) &tempBlock, INODE_BLOCK_START_NUM + InodeIterator) == FALSE)	// Write to the proper inode sector location
		{
			exit(-1);
		}

		memset(&tempBlock, 0, SECTOR_SIZE);
	}

	return TRUE;

}

FileError RecentError;

FileError OSFS_GetError()
{
	return RecentError;
}
MYFILE* OSFS_Create(char* fileName)
{
	if (strlen(fileName) > MAX_FILE_NAME_CHARS)
	{
		RecentError = FILE_NAME_TOO_LONG;
		return 0;
	}

	INODE* newFile = (INODE*) malloc(sizeof(INODE) * 1);

	if (newFile == 0)
	{
		RecentError = FILE_INIT_FAILED;
		return 0;			// Out of memory, cannot proceed
	}

	if (_GetInodeFromFileName(fileName) > 0)
	{
		RecentError = FILE_ALREADY_EXISTS;
		return 0;	// We cannot create the same file name twice
	}

	// Initialize the new Inode with the proper items
	memcpy(newFile->FILE_NAME, fileName, strlen(fileName) * sizeof(char));
	newFile->FILE_BYTES = SECTOR_SIZE;	// Every file starts with 512 bytes of data
	newFile->BYTES_USED  = 0;
	newFile->LATEST_CURSOR = 0;
	uint32_t BlockToAssign = _GetNextFreeBlock();

	_MarkBlockAsOccupied(BlockToAssign);
	newFile->BLOCKS_USED[(newFile->FILE_BYTES/SECTOR_SIZE) - 1] = BlockToAssign; // if 512, index 0 gets new block. If 1024, index 1 does.

	uint32_t InodeNumToAssign = _GetNextFreeInode();
	_MarkInodeAsOccupied(InodeNumToAssign);

	newFile->INODE_NUM = InodeNumToAssign;

	_FlushBitMapToDisk();

	_UpdateNonVolatileInodeCopy(InodeNumToAssign, newFile);


	MYFILE* fileToReturn = (MYFILE*) malloc(sizeof(MYFILE) * 1);

	if (fileToReturn == 0)
	{
		RecentError = FILE_INIT_FAILED;
		free(newFile);
		return 0;
	}

	fileToReturn->FileInode = newFile;

	RecentError = FILE_OK;

	return fileToReturn;
}

MYFILE* OSFS_Open(char* fileName)
{
	INODE* createdFile = (INODE*) malloc(sizeof(INODE) * 1);

	if (createdFile == 0)
	{
		RecentError = FILE_INIT_FAILED;
		return 0;	// Not enough memory to initialize anything, so we cannot do anything
	}

	// Get the proper inode associated with this filename
	int32_t associatedInode = _GetInodeFromFileName(fileName);

	if (associatedInode == -1)
	{
		RecentError = FILE_DOES_NOT_EXIST;
		return 0;
	}

	_ReadInodeFromNonVolatileMemory(associatedInode, createdFile);

	MYFILE* returnFile = (MYFILE*) malloc(sizeof(INODE) * 1);

	returnFile->FileInode = createdFile;

	RecentError = FILE_OK;

	// TODO: Read the data chunks into memory as well

	return returnFile;
}

// 	Reads the specified amount of bytes from the file into the provided buffer. Offset allows specification of where in file to read from.
// 	Parameters:
//		fileDescriptor: 	FILE structure pointer provided by OSFS_Create
//		Buffer:			Buffer to write the file data to
//		numBytes:		Number of bytes to read from the file and into the buffer
//		Offset:			Byte number in file to read from
int32_t OSFS_Read(MYFILE* fileDescriptor, BYTE* Buffer, uint32_t numBytes, uint32_t Offset)
{
	if (fileDescriptor == 0 || Buffer==0 || numBytes==0) return FALSE;			// Invalid/useless parameters

	INODE* FileInode = fileDescriptor->FileInode;

	if (FileInode == 0) return FALSE;											// Invalid/corrupted Inode

	uint32_t BlockListIndex = Offset / SECTOR_SIZE;							// If offset < 512, we need block 0. If < 1024, we need block 1, etc.
	uint32_t BlocksAllocated = FileInode->FILE_BYTES / SECTOR_SIZE;			// Number of blocks already allocated

	while (BlocksAllocated < (BlockListIndex + 1))								// Not enough blocks allocated yet
	{
		if (BlocksAllocated >= MAX_FILE_SECTORS) return FALSE;						// Cannot create new space to index into this address
		// Give this file another block
		uint32_t BlockToAssign = _GetNextFreeBlock();

		_MarkBlockAsOccupied(BlockToAssign);										// Assign the (BlockToAssign + 1)-th block to this space (assign 3rd block to space 2)
		FileInode->BLOCKS_USED[BlocksAllocated] = BlockToAssign;					// New block has been assigned to this
		FileInode->FILE_BYTES += SECTOR_SIZE;									// Another block has been added
		BlocksAllocated = FileInode->FILE_BYTES/SECTOR_SIZE;
	}

	uint32_t BlockWanted = FileInode->BLOCKS_USED[BlockListIndex];
	uint32_t IntraBlockIndex = Offset % SECTOR_SIZE;

	memset(&tempBlock, 0, SECTOR_SIZE);

	// Load the needed block from memory
	_ReadFromFile((BYTE*) &tempBlock, BlockWanted);

	BYTE* byteToStartReadingFrom = &tempBlock[IntraBlockIndex];

	// The number of bytes that can be written within this block (in case of overlapping writes)
	uint32_t ValidBytes = SECTOR_SIZE - IntraBlockIndex;

	if (numBytes <= ValidBytes)
	{
		// TODO: No need to load another block from SD
		memcpy(Buffer, byteToStartReadingFrom, numBytes);
	}
	else
	{
		// TODO: Need to load another block from SD
		// Do a recursive call by splitting the array
		// BlockSize - IntraBlockIndex is the size of the first array within this block
		// Remaining is put into offset+1
		memcpy(Buffer, byteToStartReadingFrom, ValidBytes);

		// Recursive call
		// Buffer + validBytes: we're currently validBytes bytes into the buffer, so we ignore stuff already written
		// numBytes - validBytes: we've already written validBytes bytes out of the total numBytes byte count, so we subtract it
		// We're starting from square 0 at this new block due to contiguous overlap

		if (!OSFS_Read(fileDescriptor, Buffer + ValidBytes, numBytes - ValidBytes, Offset + (SECTOR_SIZE - Offset)))
		{
			return FALSE;
		}
	}

	return TRUE;
	// Say I want to write 1000 bytes in area 300 of the third sector (so offset is 512*3 + 300 = 1836).
	// BlockListIndex would be 1836/512 = 3
	// BlockWanted would be BLOCKS[3] = 913 (assume this).
	// IntraBlockIndex would be 1836%512 = 300.
	// I can write 512-300 = 212 bytes within this block before I need to reload another block

}

// 	Writes the specified amount of bytes from buffer into the provided file. Offset allows specification of where in file to start write from.
// 	Parameters:
//		fileDescriptor: 	FILE structure pointer provided by OSFS_Create
//		Buffer:			Buffer to write the file data to
//		numBytes:		Number of bytes to read from the file and into the buffer
//		Offset:			Byte number in file to read from
bool OSFS_Write(MYFILE* fileDescriptor, BYTE* Buffer, uint32_t numBytes, uint32_t Offset)
{
	if (fileDescriptor == 0 || Buffer==0 || numBytes==0) return FALSE;			// Invalid/useless parameters

	INODE* FileInode = fileDescriptor->FileInode;

	if (FileInode == 0) return FALSE;											// Invalid/corrupted Inode

	uint32_t BlockListIndex = Offset / SECTOR_SIZE;							// If offset < 512, we need block 0. If < 1024, we need block 1, etc.
	uint32_t BlocksAllocated = FileInode->FILE_BYTES / SECTOR_SIZE;			// Number of blocks already allocated

	while (BlocksAllocated < (BlockListIndex + 1))								// Not enough blocks allocated yet
	{
		if (BlocksAllocated >= MAX_FILE_SECTORS) return FALSE;						// Cannot create new space to index into this address
		// Give this file another block
		uint32_t BlockToAssign = _GetNextFreeBlock();

		_MarkBlockAsOccupied(BlockToAssign);										// Assign the (BlockToAssign + 1)-th block to this space (assign 3rd block to space 2)
		FileInode->BLOCKS_USED[BlocksAllocated] = BlockToAssign;					// New block has been assigned to this
		FileInode->FILE_BYTES += SECTOR_SIZE;									// Another block has been added
		BlocksAllocated = FileInode->FILE_BYTES/SECTOR_SIZE;
	}

	uint32_t BlockWanted = FileInode->BLOCKS_USED[BlockListIndex];
	uint32_t IntraBlockIndex = Offset % SECTOR_SIZE;

	memset(&tempBlock, 0, SECTOR_SIZE);

	// Load the needed block from memory
	_ReadFromFile((BYTE*) &tempBlock, BlockWanted);

	BYTE* byteToStartWritingFrom = &tempBlock[IntraBlockIndex];

	// The number of bytes that can be written within this block (in case of overlapping writes)
	uint32_t ValidBytes = SECTOR_SIZE - IntraBlockIndex;

	if (numBytes <= ValidBytes)
	{
		memcpy(byteToStartWritingFrom, Buffer, numBytes);
		if (_WriteToFile((BYTE*) &tempBlock, BlockWanted))
		{
			uint32_t ByteWrittenTo = (((SECTOR_SIZE) * BlockListIndex) + IntraBlockIndex + numBytes);
			if (ByteWrittenTo > FileInode->BYTES_USED)
			{
				FileInode->BYTES_USED = ByteWrittenTo;
			}

			fileDescriptor->FileInode->LATEST_CURSOR = ((SECTOR_SIZE) * BlockListIndex) + IntraBlockIndex + numBytes;
		}
	}
	else
	{
		// TODO: Need to load another block from SD
		// Do a recursive call by splitting the array
		// BlockSize - IntraBlockIndex is the size of the first array within this block
		// Remaining is put into offset+1
		memcpy(byteToStartWritingFrom, Buffer, ValidBytes);

		if (_WriteToFile((BYTE*) &tempBlock, BlockWanted))
		{
			uint32_t ByteWrittenTo = (((SECTOR_SIZE) * BlockListIndex) + IntraBlockIndex + numBytes);
			if (ByteWrittenTo > FileInode->BYTES_USED)
			{
				FileInode->BYTES_USED = ByteWrittenTo;
			}
		}

		// Recursive call
		// Buffer + validBytes: we're currently validBytes bytes into the buffer, so we ignore stuff already written
		// numBytes - validBytes: we've already written validBytes bytes out of the total numBytes byte count, so we subtract it
		// We're starting from square 0 at this new block due to contiguous overlap

		if (!OSFS_Write(fileDescriptor, Buffer + ValidBytes, numBytes - ValidBytes, Offset + (SECTOR_SIZE - Offset)))
		{
			return FALSE;
		}
	}

	return TRUE;
	// Say I want to write 1000 bytes in area 300 of the third sector (so offset is 512*3 + 300 = 1836).
	// BlockListIndex would be 1836/512 = 3
	// BlockWanted would be BLOCKS[3] = 913 (assume this).
	// IntraBlockIndex would be 1836%512 = 300.
	// I can write 512-300 = 212 bytes within this block before I need to reload another block
}

bool OSFS_Close(MYFILE* fileToClose)
{
	// Write the inode to non-volatile memory
	_UpdateNonVolatileInodeCopy(fileToClose->FileInode->INODE_NUM, fileToClose->FileInode);

	free(fileToClose->FileInode);
	free(fileToClose);

	fileToClose = 0;

	// Write the associated data blocks to non-volatile memory
	// TODO: Write the data blocks back to non-volatile memory
	return TRUE;
}

bool OSFS_Append(MYFILE* fileDescriptor, BYTE* buffer, uint32_t numBytes)
{
	return OSFS_Write(fileDescriptor, buffer, numBytes, fileDescriptor->FileInode->LATEST_CURSOR);
}

bool OSFS_Delete(char* fileName)
{
	INODE* createdFile = (INODE*) malloc(sizeof(INODE) * 1);

	if (createdFile == 0)
	{
		RecentError = FILE_INIT_FAILED;
		return FALSE;	// Not enough memory to initialize anything, so we cannot do anything
	}

	// Get the proper inode associated with this filename
	int32_t associatedInode = _GetInodeFromFileName(fileName);

	if (associatedInode == -1)
	{
		RecentError = FILE_DOES_NOT_EXIST;
		return FALSE;
	}

	_ReadInodeFromNonVolatileMemory(associatedInode, createdFile);

	RecentError = FILE_OK;

	uint32_t BlocksAllocated = createdFile->FILE_BYTES / SECTOR_SIZE;			// Number of blocks already allocated
	uint32_t BlockIterator   = 0;

	for (BlockIterator = 0; BlockIterator < BlocksAllocated; BlockIterator++)
	{
		_MarkBlockAsFree(createdFile->BLOCKS_USED[BlockIterator]);
	}

	_MarkInodeAsFree(associatedInode);

	free(createdFile);

	return TRUE;

}

uint32_t GetFileSize(MYFILE* fileToEval)
{
	return fileToEval->FileInode->BYTES_USED; // Latest byte written to
}
//***************************************** Private Functions ************************************//

// Private functions for interacting with physical disk

bool _WriteToFile(BYTE* InputBuffer, uint64_t BlockNum)
{
    FILE* fileptr = fopen(SDCARD_SPOOF, "rb");
    if (!fileptr) {
        fileptr = fopen(SDCARD_SPOOF, "w");
    }
    fseek(fileptr, 0, SEEK_END);
    long filelen = ftell(fileptr);
    rewind(fileptr);

    char* buffer;
    if (filelen <= (BlockNum * SECTOR_SIZE)) {
        filelen = ((BlockNum + 1) * SECTOR_SIZE)  + sizeof(InputBuffer);
    }
    buffer = (char *)malloc((filelen+1)*sizeof(char)); // Enough memory for file + \0
    fread(buffer, filelen, 1, fileptr); // Read in the entire file
    fclose(fileptr); // Close the file
    remove(SDCARD_SPOOF);
    
    char* index = &buffer[BlockNum * SECTOR_SIZE];
    memcpy(index, InputBuffer, SECTOR_SIZE);
    
    fileptr = fopen(SDCARD_SPOOF, "w");
    if (!fileptr) {
        free(buffer);
        return FALSE;
    }
    fwrite(buffer, sizeof(char), (filelen)*sizeof(char), fileptr);
    fclose(fileptr);
    free(buffer);
    return TRUE;
}


bool _ReadFromFile(BYTE* OutputBuffer, uint64_t BlockNum)
{
    FILE* fileptr = fopen(SDCARD_SPOOF, "rb");
    if (!fileptr) {
    	OSFS_Format();
    	fileptr = fopen(SDCARD_SPOOF, "rb");
    	if (!fileptr) return FALSE;
    }
    fseek(fileptr, 0, SEEK_END);
    long filelen = ftell(fileptr);
    rewind(fileptr);
    if (filelen <= (BlockNum * SECTOR_SIZE)) {
        filelen = (BlockNum + 1) * SECTOR_SIZE;
    }
    char* buffer = (char *)malloc((filelen+1)*sizeof(char)); // Enough memory for file + \0
    fread(buffer, filelen, 1, fileptr); // Read in the entire file
    fclose(fileptr); // Close the file
    char* index = &buffer[BlockNum * SECTOR_SIZE];
    memcpy(OutputBuffer, index, SECTOR_SIZE);
    free(buffer);
    return TRUE;
}

// Block tools

// Function used to update the bitmap in memory from the Disk
bool _TranscribeBitMap(BYTE* blockToUse, BitMap* mapToUpdate, uint32_t NumBytes)
{
	uint32_t* ArrayInput = (uint32_t*) blockToUse;

	mapToUpdate->bitsize = ArrayInput[0];
	mapToUpdate->wordsize = ArrayInput[1];

	// The pointer itself is not saved, so do not count that in the offset calculations
	memcpy(mapToUpdate->dataarray, &ArrayInput[2], mapToUpdate->wordsize * sizeof(uint32_t));

	return TRUE;
}


// Private Inode Functions

// Return condition:
// TRUE if inode is occupied
// FALSE if inode is free
bool 	_CheckInodeOccupancy(uint32_t Inodenum)
{
	if (Inodenum >= MAX_INODE_COUNT) return FALSE;

	BitMap* localBitMapRef = InodeBitMap;

	return BitMap_TestBit(localBitMapRef, Inodenum);

}

// Returns the next free inode. Does NOT mark the inode as occupied.
uint32_t _GetNextFreeInode()
{
	uint32_t InodeStart = 0;

	for (InodeStart = 0; InodeStart < MAX_INODE_COUNT; InodeStart++)
	{
		if (_CheckInodeOccupancy(InodeStart) == FALSE) return InodeStart;
	}

	exit(-1);

	return 0; // never executes since kernel panics
}

// Gets the next inode number that is occupied starting from the provided index
int32_t _GetNextOccupiedInode(uint32_t StartLocation)
{
	uint32_t InodeStart = StartLocation;

	for (InodeStart = StartLocation; InodeStart < MAX_INODE_COUNT; InodeStart++)
	{
		if (_CheckInodeOccupancy(InodeStart) == TRUE) return InodeStart;
	}

	return -1;
}

bool _MakeInodeResident(uint32_t InodeNum, INODE* volatileLocation)
{
	if (InodeNum > MAX_INODE_COUNT) return FALSE;
	memset(&tempBlock, 0, SECTOR_SIZE);

	uint32_t InodeResidentInBlock = INODE_BLOCK_START_NUM + (InodeNum / INODES_PER_SECTOR); // The block where the Inode we want lives in

	_ReadFromFile((BYTE*) &tempBlock, InodeResidentInBlock);

	// Read the inode into the specified location
	memcpy(volatileLocation, &tempBlock[InodeNum%INODES_PER_SECTOR * sizeof(INODE)], sizeof(INODE));

	return TRUE;
}

void _MarkInodeAsOccupied(uint32_t InodeNum)
{
	if (InodeBitMap == 0) exit(-1);
	BitMap_SetBit(InodeBitMap, InodeNum);
}

void _MarkInodeAsFree(uint32_t InodeNum)
{
	if (InodeBitMap == 0) exit(-1);
	BitMap_ClearBit(InodeBitMap, InodeNum);

}
// Accepts a null terminated file name string, gives back a uint32_t inode number
// that is associated with this file.
// if return val is greater than 0, the correct INODE has been identified
// if return val is -1, the inode does not exist
int32_t _GetInodeFromFileName(char* fileName)
{
	uint32_t NodeUntilNow = 0;
	memset(&CurrentNode, 0, sizeof(INODE));

	while(TRUE)
	{
		int32_t NextNodeOccupied = _GetNextOccupiedInode(NodeUntilNow);

		if (NextNodeOccupied == -1) return -1;

		// Make the INODE resident into StaticNode
		_MakeInodeResident(NextNodeOccupied, &CurrentNode);

		if (strcmp(CurrentNode.FILE_NAME, fileName) == 0)					// the file names match up
		{
			return NextNodeOccupied;
		}

		NodeUntilNow = NextNodeOccupied + 1;

		memset(&CurrentNode, 0, sizeof(INODE));

	}
}

// Update non-volatile inode with copy of volatile inode
void _UpdateNonVolatileInodeCopy(uint32_t InodeNum, INODE* volatileCopy)
{
	uint32_t InodeResidentInBlock = INODE_BLOCK_START_NUM + (InodeNum / INODES_PER_SECTOR); // The block where the Inode we want lives in
	_ReadFromFile((BYTE*)&tempBlock, InodeResidentInBlock);
	memcpy(&tempBlock[InodeNum%INODES_PER_SECTOR * sizeof(INODE)], volatileCopy, sizeof(INODE));
	_WriteToFile((BYTE*)&tempBlock, InodeResidentInBlock);

}

void _ReadInodeFromNonVolatileMemory(uint32_t InodeNum, INODE* location)
{
	uint32_t InodeResidentInBlock = INODE_BLOCK_START_NUM + (InodeNum / INODES_PER_SECTOR); // The block where the Inode we want lives in
	_ReadFromFile((BYTE*)&tempBlock, InodeResidentInBlock);
	memcpy(location, &tempBlock[InodeNum%INODES_PER_SECTOR * sizeof(INODE)], sizeof(INODE));
	memset(tempBlock, 0, SECTOR_SIZE);

}
// Block operations

bool _CheckBlockOccupancy(uint32_t BlockNum)
{
	if (BlockNum >= MAX_BLOCKS_TRACKED) return FALSE;

	BitMap* localBitMapRef = DataBitMap;

	return BitMap_TestBit(localBitMapRef, BlockNum);

}

// Returns the next free block. Does NOT mark the inode as occupied.
uint32_t _GetNextFreeBlock()
{
	uint32_t BlockStart = 0;

	for (BlockStart = 0; BlockStart < MAX_BLOCKS_TRACKED; BlockStart++)
	{
		if (_CheckBlockOccupancy(BlockStart) == FALSE) return BlockStart;
	}

	exit(-1);

	return 0; // never executes since kernel panics
}

void _MarkBlockAsOccupied(uint32_t BlockNum)
{
	if (DataBitMap == 0) exit(-1);
	BitMap_SetBit(DataBitMap, BlockNum);
}

void _MarkBlockAsFree(uint32_t BlockNum)
{
	if (DataBitMap == 0) exit(-1);
	BitMap_ClearBit(DataBitMap, BlockNum);
}

// Required: BYTE Array has to be exactly 512 bytes.
void _UpdateNonVolatileDataBlockCopy(uint32_t BlockNum, BYTE* volatileCopy)
{
	_WriteToFile((BYTE*)&volatileCopy, BlockNum);
}

// Bitmap operations

// Put volatile bitmaps into non-volatile storage
void _FlushBitMapToDisk()
{
	// Since the bitmap has a dynamic array, we need to transcribe it manually
	//CurrentBlock.SizeInBytes = sizeof(BitMap) + (DATA_BITMAP_SIZE_IN_WORDS * sizeof(uint32_t)) - sizeof(uint32_t*); // everything but the pointer
	memset(&tempBlock, 0, SECTOR_SIZE);

	memcpy(&tempBlock, DataBitMap, sizeof(BitMap) - sizeof(uint32_t*)); // Store everything but the pointer.

	uint32_t SizeUsed = sizeof(BitMap) - sizeof(uint32_t*);	// The amount of sized used so far in the struct

	memcpy(((&tempBlock[SizeUsed])), DataBitMap->dataarray, DATA_BITMAP_SIZE_IN_WORDS * sizeof(uint32_t)); // Copy over the dynamic data

	if (_WriteToFile((BYTE*) &tempBlock, DATA_BITMAP_SECTOR_NUM) == FALSE)
	{
			exit(-1);
	}

	memset(&tempBlock, 0, SECTOR_SIZE);

	// Store the inode bit map into the proper area

	if (DataBitMap == 0 || InodeBitMap == 0) exit(-1);

	// Since the bitmap has a dynamic array, we need to transcribe it manually
	//CurrentBlock.SizeInBytes = sizeof(BitMap) + (INODE_BITMAP_SIZE_IN_WORDS * sizeof(uint32_t)) - sizeof(uint32_t*); // everything but the pointer

	memcpy(&tempBlock[0], InodeBitMap, sizeof(BitMap) - sizeof(uint32_t*)); // Store everything but the pointer.

	SizeUsed = sizeof(BitMap) - sizeof(uint32_t*);	// The amount of sized used so far in the struct

	memcpy(((&tempBlock[SizeUsed])), InodeBitMap->dataarray, INODE_BITMAP_SIZE_IN_WORDS * sizeof(uint32_t)); // Copy over the dynamic data

	if (_WriteToFile((BYTE*) &tempBlock, INODE_BITMAP_SECTOR_NUM) == FALSE)
	{
			exit(-1);
	}

	memset(&tempBlock, 0, SECTOR_SIZE);
}

// Iterates through the root directory and prints out the files
// that are contained within it.
void SerialListFiles()
{
	uint32_t StartLoc = 0;
	memset(tempBlock, 0, SECTOR_SIZE);

	while(TRUE)
	{
		int32_t nextInode = _GetNextOccupiedInode(StartLoc);
		if (nextInode < 0) return;
		_ReadInodeFromNonVolatileMemory(nextInode, &CurrentNode);
        printf("%s :: %d  bytes\n", CurrentNode.FILE_NAME, CurrentNode.BYTES_USED);
		StartLoc = nextInode + 1;
	}
}

void SerialPrintFile(MYFILE* fileToPrint)
{
	INODE* filenode = fileToPrint->FileInode;

	uint32_t ByteIterator  = 0;

	BYTE thisByte;
	memset(&thisByte, 0, 1);

	BYTE* fileContents = malloc(filenode->BYTES_USED + 1);
	memset(fileContents, 0, filenode->BYTES_USED + 1);

	if (fileContents)
	{
		for(ByteIterator = 0; ByteIterator < filenode->BYTES_USED; ByteIterator++)
		{
			OSFS_Read(fileToPrint, (BYTE*) &thisByte, 1, ByteIterator);
			fileContents[ByteIterator] = (thisByte);
			memset(&thisByte, 0, 1);
		}
		printf("%s", (char*) fileContents);
		free(fileContents);

	}
	else
	{
		for(ByteIterator = 0; ByteIterator < filenode->BYTES_USED; ByteIterator++)
		{
			OSFS_Read(fileToPrint, (BYTE*) &thisByte, 1, ByteIterator);
			printf("%c", thisByte);
			memset(&thisByte, 0, 1);
		}
	}
}
