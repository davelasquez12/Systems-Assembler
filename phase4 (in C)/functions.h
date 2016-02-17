#pragma once
FILE * openFileInReadMode(char * filename);
int isXDigits(char * str);

int getNumParams(char * param)
{
	int i = 0;
	int numParams = 0;
	int length = strlen(param);

	if (length == 0)
		return 0;

	for (i = 0; i < length; i++)
	{
		if (param[i] == ' ')
			continue;
		else if (param[i] != ' ' && (param[i + 1] == ' ' || param[i + 1] == '\0'))
			numParams++;
	}
	return numParams;
}


void help(char ** params, int * numParams)
{
	if (*numParams > 0)
	{
		printf("ERROR: The 'help' command does not take any parameters.\n\n");
		return;
	}

	printf("\n                              COMMAND LIST\n\n");
	printf("load [filename]\n");
	printf("execute\n");
	printf("debug\n");
	printf("dump [start] [end]\n");
	printf("help\n");
	printf("assemble [filename]\n");
	printf("directory or dir\n");
	printf("exit\n\n");
}

int * load(char ** params, int * numParams)
{
	char record[80], address[10] = "/0", length[5] = "/0", byte[5] = "/0";
	int i, lengthInt, byteInt;
	int * addressInt = malloc(sizeof(int));
	unsigned char byteTemp[1] = "\0";
	
	if (*numParams == 1)
	{
		FILE * of = openFileInReadMode(params[0]);
		if(of != NULL)
		{
			fgets(record, 80, of);							//read header record just to skip to first text record
			while(fgets(record, 80, of) != NULL && record[0] == 'T')
			{
				strncpy(address, record + 3, 4);			//store starting address
				*addressInt = strtol(address, NULL, 16);	//convert address from hex to decimal
				strncpy(length, record + 7, 2);				//store length of text record; used to loop through record
				lengthInt = strtol(length, NULL, 16);		//convert length from hex to decimal
				lengthInt = lengthInt * 2 + 9;
				
				for (i = 9; i < lengthInt; i+=2)
				{
					strncpy(byte, record + i, 2);			//store each byte (2 chars) into byte variable
					byteInt = strtol(byte, NULL, 16);		//convert hex value to decimal
					byteTemp[0] = (unsigned char)byteInt;	//store hex value in byteTemp as an unsigned char type
					PutMem(*addressInt, byteTemp, 0);		//load value into memory at current address
					*addressInt += 1;						//go to next address
				}
			}
			strncpy(address, record + 3, 4);				//get end record start address;
			*addressInt = strtol(address, NULL, 16);		//convert address to decimal, store it in addressInt pointer to be used by execute()
			fclose(of);
			printf("Loaded object file into memory successfully!\n\n");
			return addressInt;	//object file has been loaded into memory successfully, so return start address from end record
		}
		else
		{
			free(addressInt);
			return NULL;		//error
		}
	}
	else if (*numParams > 1)
		printf("ERROR: The 'load' command only takes a filename as a parameter.\n\n");
	else
		printf("ERROR: The 'load' command must take a filename as a parameter.\n\n");

	return NULL;
}

void debug(char ** params, int * numParams)
{
	if (*numParams > 0)
	{
		printf("ERROR: The 'debug' command does not take any parameters.\n\n");
		return;
	}
	printf("Function is under construction. Will be up soon.\n\n");
}

int execute(char ** params, int * numParams, int * loadAddr)
{
	if (*numParams > 0)
		printf("ERROR: The 'execute' command does not take any parameters.\n\n");
	else if (loadAddr != NULL)	//checks if object file has been loaded into memory
	{
		SICRun(loadAddr, 0);
		return 1;
	}
	else
		printf("ERROR: Object code has not been loaded into memory.\n\n");
		
	return 0;
}

void dump(char ** params, int * numParams)
{
	BYTE value;

	while (1)
	{
		if (*numParams == 2 && isXDigits(params[0]) && isXDigits(params[1]))
		{
			int i, j = 0;
			int start = strtol(params[0], NULL, 16);
			int end = strtol(params[1], NULL, 16);
			if (start > end)
			{
				printf("ERROR: The start address must be less than the end address.\n\n");
				return;
			}
			
			printf("Start = %X  End = %X\n", start, end);
			for(i = start; i <= end; i++, j++)
			{
				if(j % 16 == 0)
					printf("\n%X: ", i);
					
				GetMem(i, &value, 0);
				printf("%02X ", value);
			}
			
			printf("\n\n");
			break;
		}
		else if (*numParams == 1 && isXDigits(params[0]))
		{
			char end[100];
			printf("Enter end address (hex): ");
			fgets(end, 100, stdin);
			end[strlen(end) - 1] = '\0';
			int i = getNumParams(end);
			if (*numParams + i > 2 || *numParams + i == 1)
				printf("ERROR: enter ONE end address.\n\n");
			else
			{
				*numParams += i;
				strcpy(params[1], end);
			}
		}
		else if (*numParams == 0)
		{
			char start[100];
			printf("Enter start address (hex): ");
			fgets(start, 100, stdin);
			start[strlen(start) - 1] = '\0';
			int i = getNumParams(start);
			if (*numParams + i > 1 || (*numParams + i) == 0)
				printf("ERROR: enter ONE start address.\n\n");
			else
			{
				*numParams += i;
				strcpy(params[0], start);
			}
		}
		else
			printf("ERROR: The 'dump' function must take 2 address in hex [start] [end].\n\n");
	}
}

void dir(char ** params, int * numParams)
{
	if (*numParams == 0)
		system("ls");
	else
	{
		printf("ERROR: The 'dir' or 'directory' command does not take any parameters.\n\n");
		return;
	}
}

void exitShell(char ** params, int * numParams)
{
	if (*numParams == 0)
	{
		printf("Thank you for using Skynet Command Shell!\nGoodbye!\n\n");
		exit(0);
	}
	else if (*numParams > 0)
		printf("ERROR: The 'exit' command does not have any parameters.\n\n");
}

char * getInput()
{
	int n, size = 260;
	char * input = malloc(sizeof(char) * size);
	if (!input)					//make sure memory allocation worked
		return NULL;

	do
	{
		printf("cmd> ");			//prompt
		fgets(input, 256, stdin);	//get user input/commands
		n = strlen(input);
	} while (n <= 1);

	if (input[n - 1] == '\n')			//remove new line from input array
		input[n - 1] = '\0';

	return input;
}

//formats user input into command and parameters and stores values in 2D array (inputArray[0] == command)
char ** getInputArray(char * input)
{
	int i, row = 0, rows = 2, col = 0, cols = 20, numParams = 0;
	char ** inputArr = malloc(rows * sizeof(char*));	//initialize 2 rows
	for (i = 0; i < rows; i++)
		inputArr[i] = malloc(cols * sizeof(char));		//initialize 20 columms per row (will grow if needed)

	int inputLength = strlen(input);

	for (i = 0; i < inputLength; i++)
	{
		if (input[i] == ' ')
			continue;
		else
		{
			if (numParams >= 2)													//dynamically adds one more row if there are 2 or more parameters
			{
				inputArr = realloc(inputArr, (rows + 1)*sizeof(char*));
				inputArr[rows++] = malloc(cols * sizeof(char));			//allocate columns in new row, update number of total rows
			}

			inputArr[row][col++] = input[i];	//character is detected, so insert character into 2D array

			if (col == cols - 1)				//if true, reallocate column size on current row because next char won't fit
			{									//in params[row] when including the null character					
				inputArr[row] = realloc(inputArr[row], 2 * cols * sizeof(char)); //update params[row] with new size of columns
				cols = cols * 2;
			}

			if (input[i + 1] == ' ' || input[i + 1] == '\0')
			{
				inputArr[row][col] = '\0';		//null terminate string since this is the end of the parameter/command read
				row++;							//go to next row since the command, or a parameter has been fully stored
				numParams++;					//increment number of parameters
				col = 0;						//reset current column to 0 to iterate through next row
			}
		}
	}

	if (numParams == 1)							//there was only one parameter stored so free the second unused row
		free(inputArr[1]);

	return inputArr;
}
