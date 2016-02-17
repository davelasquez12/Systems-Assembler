/*David Velasquez
Systems Programming - Phase 4
Due: December 10th, 2015*/

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
//#include "sicengine.c"
#include "sic.h"
#include "functions.h"
#include "hash.h"
#include "assemblePass1Functions.h"
#include "assemblePass2Functions.h"

int main()
{
	int i = 0, j, k;
	int * numParams = malloc(sizeof(int));
	*numParams = 0;
	char ** inputArray, ** freeThisArray;
	int * loadAddress = NULL;
	int isExecuted = 0;
	SICInit();
	printf("\nWelcome to the Skynet Command Shell!\n\n");

	while (1)
	{
		char * input = getInput();				//prompt function for user input
		char * savedInput = input;				//free this memory allocated at the end
		char command[260];						//stores only the command (not the parameters)
		int inputLength = strlen(input);

		//get string up to first whitespace to separate the command and its parameters
		for (i = 0, j = 0; i < inputLength; i++)
		{
			if (input[i] == ' ')
				continue;
			else
			{
				command[j++] = input[i];
				if (input[i + 1] == ' ' || input[i + 1] == '\0')
					break;	//command has been fully stored
			}
		}
		command[j] = '\0';
		for (k = 0; k <= i; k++)	//removes command and space and leaves parameters
			input++;

		*numParams = getNumParams(input);
		inputArray = getInputArray(input);
		freeThisArray = inputArray;			//free this memory at the end

		if (strcmp(command, "help") == 0)			//help command
			help(inputArray, numParams);
		else if (strcmp(command, "dir") == 0 || strcmp(command, "directory") == 0)	//directory command
			dir(inputArray, numParams);
		else if (strcmp(command, "load") == 0)		//load command
			loadAddress = load(inputArray, numParams);
		else if (strcmp(command, "execute") == 0)	//execute command
			isExecuted = execute(inputArray, numParams, loadAddress);
		else if (strcmp(command, "debug") == 0)		//debug command
			debug(inputArray, numParams);
		else if (strcmp(command, "dump") == 0)		//dump command
			dump(inputArray, numParams);
		else if (strcmp(command, "assemble") == 0)	//assemble command
			assemble(inputArray, numParams);
		else if (strcmp(command, "exit") == 0)		//exit command
			exitShell(inputArray, numParams);
		else
			printf("Command '%s' not recognized.\nTry entering the command 'help' for more information.\n\n", command);

		free(savedInput);							//frees user input array memory
		for (i = 0; i < *numParams; i++)				//frees 2D parameter array memory
			free(freeThisArray[i]);

		free(freeThisArray);
		freeThisArray = NULL;
			
		if(isExecuted)
		{
			free(loadAddress);
			loadAddress = NULL;
			isExecuted = 0;
		}
	}
	free(numParams);
	free(freeThisArray);
	return 0;
}
























