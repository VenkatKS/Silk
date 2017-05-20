/*
 * Shell.c
 *
 *  Created on: Feb 1, 2016
 *      Author: Venkat
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Shell.h"
#include "OS_FileSystemScheme.h"

#define COMMAND_COUNT 7

typedef void (*fp)(int); //Declares a type of a void function that accepts an int
extern void OutCRLF(void);


// Command Functions
void Help_Output(int one);
void Shell_NewFile(int one);
void Shell_DeleteFile(int one);
void Shell_AppendToFile(int one);
void Shell_PrintFile(int one);
void Shell_FormatFS(int one);
void Shell_LS(int one);

char*			commandDef[]			=		{
												"help:\n Output command information.\n\n",
												"creat:\n Creates a new file and allows writes to it.\n\n",
												"del:\n Deletes a file\n\n",
												"app:\n Appends attached string to the end of the file\n\n",
												"printfile:\n Prints content of file\n\n",
												"ls:\n List all the files on the SD card.\n\n",
												"format:\n Formats the entire filesystem.\n\n"
												};

char* 			commandFormat[]		= 		{
												"help\n",
												"creat <filename>\n",
												"del <filename>\n",
												"app <filename> <string>\n",
												"printfile <filename>\n",
												"format\n",
												"ls\n"
											};

char* 			commands[] 			= 		{
												"help",
												"creat",
												"del",
												"app",
												"printfile",
												"format",
												"ls"

											};

fp 				function_array[] 	= 		{
												Help_Output,
												Shell_NewFile,
												Shell_DeleteFile,
												Shell_AppendToFile,
												Shell_PrintFile,
												Shell_FormatFS,
												Shell_LS
											};

unsigned int		CommandCount[]	    =       {
												0,
												1,
												1,
												2,
												1,
												0,
												0
											};

char CommandTokens[PARAMS_MAX_NUM][PARAMS_MAX_SIZE];
unsigned int CurrentCommandParamCount;



char* Command;
char ExecuteName[PARAMS_MAX_SIZE];


void OutCRLF(void)
{
    putchar(CR);
    putchar(LF);
}

void Interpreter()
{
    char *line = NULL;  /* forces getline to allocate with malloc */
    size_t len = 0;     /* ignored when line = NULL */
    ssize_t read;

    printf("Please enter command:");
    //ADC_Open(0, 1000);
    OutCRLF();
	  while ((read = getline(&Command, &len, stdin)) != -1)
	  {
          Command[strcspn(Command, "\n")] = 0;

		  //uint32_t PreviousState = StartCritical();
		  Shell_CommandTokenize(Command);

		  Shell_StringRegex(CommandTokens[0], ExecuteName);
		  int match = Shell_CommandNumber(ExecuteName);
		  if (match >= 0)
		  {
			  Shell_RunCommand(match);
		  }
		  else if (match == -1)
		  {
			  OutCRLF();
			  printf("Command not found.");
		  }
		  else
		  {
				OutCRLF();
				printf("ERROR: Command parameters not correct.");
		  }

		  OutCRLF();
		  OutCRLF();
		  //EndCritical(PreviousState);

		  printf("Please enter command: \n");
	  }

}

void Shell_CommandTokenize(char* pCommand)
{
	memset(CommandTokens, 0, sizeof(CommandTokens[0][0]) * PARAMS_MAX_NUM * PARAMS_MAX_SIZE);

	unsigned int TokenIterator = 0;

	char* Token = strtok(pCommand, " ");

	while (Token)
	{

		strcpy(CommandTokens[TokenIterator], Token);

		Token = strtok(NULL, " ");

	    TokenIterator++;
	}

	CurrentCommandParamCount = TokenIterator - 1; // Do not count the command name itself

}

void Shell_FreeTokens(char** tokens)
{
	char** token_back = tokens;

	while(*tokens)
	{
		free(*tokens);
		tokens++;
	}

	free(token_back);

}

int Shell_CommandNumber(char* commandString)
{
	int fnIterator = 0;
	for (fnIterator = 0; fnIterator < COMMAND_COUNT; fnIterator++)
	{
		unsigned int compare = strcmp(commandString, commands[fnIterator]);
		if (compare == 0)
		{
			if (CurrentCommandParamCount != CommandCount[fnIterator])
			{
				return -2;
			}
			return fnIterator;
		}
	}

	return -1;
}

void Shell_StringRegex(char* src, char* dst) // Null terminated strings
{
	for (; *src; src++) {
	   if (('a' <= *src && *src <= 'z')
	    || ('0' <= *src && *src <= '9')
	    || *src == '_') *dst++ = *src;
	}
	*dst = '\0';
}

void Shell_RunCommand(unsigned int Index)
{
	function_array[Index](0);
}

void Help_Output(int one)
{
	int i = 0;
	OutCRLF();
	OutCRLF();
	printf("Command Information:\n");
	OutCRLF();
	for (i = 0; i < COMMAND_COUNT; i++)
	{
		printf("%s", commandDef[i]);
	}
	printf("\n\n");
	printf("Command Formats: \n");
	OutCRLF();
	for (i = 0; i < COMMAND_COUNT; i++)
	{
		printf("%s", commandFormat[i]);
	}
}

void Shell_NewFile(int one)
{
	if (OSFS_Create(CommandTokens[1]))
	{
		printf("\nCreated.\n");
	}
	else
	{
		if (OSFS_GetError() == FILE_ALREADY_EXISTS) printf("\nFile already exists.\n");
		else if(OSFS_GetError() == FILE_NAME_TOO_LONG) printf("\nFile name too long. Please limit it to 9 characters.\n");
	}
}

void Shell_DeleteFile(int one)
{
	if (OSFS_Delete(CommandTokens[1]))
	{
		printf("\nDeleted.\n");
	}
	else
	{
		if (OSFS_GetError() == FILE_DOES_NOT_EXIST) printf("\nFile not found.\n");
		else printf("\nAn Error Occurred.\n");
	}
}

void Shell_AppendToFile(int one)
{
	MYFILE* OpenedFile = OSFS_Open(CommandTokens[1]);


	if (OpenedFile == 0)
	{
		if (OSFS_GetError() == FILE_DOES_NOT_EXIST) printf("\nFile not found.\n");
		else printf("\nAn Error Occurred.\n");
		return;
	}

	if (OSFS_Append(OpenedFile, (BYTE*) CommandTokens[2], (uint32_t) strlen(CommandTokens[2])))
	{
		printf("\nAppended.\n");
	}

	OSFS_Close(OpenedFile);


}

void Shell_PrintFile(int one)
{
	printf("\n");
	MYFILE* OpenedFile = OSFS_Open(CommandTokens[1]);

	if (OpenedFile == 0)
	{
        printf("Opening file: %s", CommandTokens[1]);
		if (OSFS_GetError() == FILE_DOES_NOT_EXIST) printf("\nFile not found.\n");
		else printf("\nAn Error Occurred.\n");
		return;
	}

	SerialPrintFile(OpenedFile);

	printf("\n");
}

extern bool DiskInitialized;
extern int main();
void Shell_FormatFS(int one)
{
	printf("\nFormatting the filesystem.\n");
	DiskInitialized = FALSE;
	OSFS_Format();
	printf("\nPlease restart device.\n");
    OSFS_Init();
}

void Shell_LS(int one)
{
	printf("\n");
	SerialListFiles();
}

