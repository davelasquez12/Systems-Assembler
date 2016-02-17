#pragma once
char ** createErrorTable();
char * checkForLabelErrors(char * str);
char * checkForDirOperandErrors(char * dir, char * operand);
char * checkSTARTErrors(char * operand);
char * checkENDErrors(char * operand);
char * checkIfProgramTooLong(int locctr);
char * checkBYTEErrors(char * operand);
char * checkOperandErrors(char * operand);
void freeErrorTable(char ** errorTable);

int isDigits(char * str);
int isXDigits(char * str);
int isSpaces(char * str);
int errorExists(char * errStr, char * code);
int errorsExist(char * errStr);
void writeErrMessages(char * errCodes, char ** errTable, FILE * lfp);

char ** createErrorTable()
{
	int rows = 26;
	char ** table = malloc(sizeof(char *) * rows);	//25 possible error messages (won't be using the 0 index)
	if (!table)
		return NULL;

	table[0] = "";
	table[1] = "Error 1: Duplicate label definition\n";
	table[2] = "Error 2: Duplicate or misplaced START statement\n";
	table[3] = "Error 3: Illegal format in label field\n";
	table[4] = "Error 4: Illegal operand in START statement\n";
	table[5] = "Error 5: Illegal operand in BYTE statement\n";
	table[6] = "Error 6: Illegal operand field\n";
	table[7] = "Error 7: Illegal operand in END statement\n";
	table[8] = "Error 8: Illegal operand in WORD statement\n";
	table[9] = "Error 9: Illegal operand in RESW statement\n";
	table[10] = "Error 10: Illegal operand in RESB statement\n";
	table[11] = "Error 11: Missing or misplaced operand in RESB statement\n";
	table[12] = "Error 12: Missing or misplaced operand in START statement\n";
	table[13] = "Error 13: Missing or misplaced operand in instruction\n";
	table[14] = "Error 14: Missing operation code\n";
	table[15] = "Error 15: Missing or misplaced START statement\n";
	table[16] = "Error 16: Missing or misplaced operand in RESW statement\n";
	table[17] = "Error 17: Missing or misplaced operand in WORD statement\n";
	table[18] = "Error 18: Missing or misplaced operand in END statement\n";
	table[19] = "Error 19: Missing or misplaced operand in BYTE statement\n";
	table[20] = "Error 20: Odd length hex string in BYTE statement\n";
	table[21] = "Error 21: Program too long\n";
	table[22] = "Error 22: Too many symbols in source program\n";
	table[23] = "Error 23: Undefined symbol in operand\n";
	table[24] = "Error 24: Unrecognized operation code\n";
	table[25] = "Error 25: Memory allocation failed.\n";

	return table;
}

void freeErrorTable(char ** errorTable)
{
	free(errorTable);
	errorTable = NULL;
}

//check for illegal labels/symbols/operands, return error code "3" if there is an error, else returns "0"
char * checkForLabelErrors(char * str)
{
	int i;
	if (strlen(str) > 6 || isalpha(str[0]) == 0)//if the str is greater than 6 characters, or if first character is not a letter
		return "3 ";
	else
	{
		for (i = 0; i < strlen(str); i++)
			if (isalnum(str[i]) == 0)			//if a character is not alphanumeric
				return "3 ";
	}
	return "0 ";		//if this is reached, there were no errors
}

//checks for missing or illegal operands on a data storage directive (BYTE, WORD, RESB, RESW)
char * checkForDirOperandErrors(char * dir, char * operand)
{
	if (strcmp(dir, "BYTE") == 0)
		return checkBYTEErrors(operand);
	else if (strcmp(dir, "WORD") == 0)
	{
		//operand for a WORD directive cannot be empty, a nondigit, or a value that cannot fit in 24 bits (3 bytes)
		if (strcmp(operand, "") == 0)
			return "17 ";

		if (!isDigits(operand) || atoi(operand) > 8388607) //2^(23)-1
			return "8 ";
	}
	else if (strcmp(dir, "RESW") == 0)
	{
		//operand for a RESW directive cannot be empty, a nondigit, and should fit in memory (2 bytes)
		if (strcmp(operand, "") == 0)
			return "16 ";

		if (!isDigits(operand) || (atoi(operand) * 3) > 32767)	//2^(15)-1
			return "9 ";
	}
	else if (strcmp(dir, "RESB") == 0)
	{
		//operand for a RESB directive cannot be empty, a nondigit, and should fit in memory (2 bytes)
		if (strcmp(operand, "") == 0)
			return "11 ";

		if (!isDigits(operand) || atoi(operand) > 32767)	//2^(15)-1
			return "10 ";
	}

	return "0 ";	//there were no directive mnemonics, or there were no errors with the directives' operands
}

int isDigits(char * str)
{
	int i, len = strlen(str);
	for (i = 0; i < len; i++)
	{
		if (isdigit(str[i]));
		else
			return 0;	//str does not consist of only digits
	}
	return 1;			//str does consist of only digits
}

int isXDigits(char * str)
{
	int i, len = strlen(str);
	for (i = 0; i < len; i++)
	{
		if (isxdigit(str[i]));
		else
			return 0;	//str does not consist of only hex values
	}
	return 1;			//str does consist of only hex values
}

char * checkBYTEErrors(char * operand)
{
	if (isSpaces(operand))					//if operand is only spaces
		return "5 ";
	
	if (strlen(operand) <= 2)
		return "5 ";						//operand is too short

	if (strcmp(operand, "") == 0)			//if operand is missing
		return "19 ";

	int i = 2;

	if (operand[0] == 'C' || operand[0] == 'X')
	{
		if (operand[0] == 'C' && operand[1] == '\'' && operand[strlen(operand) - 1] == '\'')
		{
			while (operand[i] != '\'' && i < 32)	//loop while endquote has not been reached, or the number of characters is less than 30
			{
				if (isalpha(operand[i++] == 0))		//if character is not alphabetical
					return "5 ";
			}

			if (i >= 32)
				return "5 ";		//the char is more than 30 chars long so its an error (32 includes the 'C' and the first single quote)
		}
		else if (operand[0] == 'X' && operand[1] == '\'' && operand[strlen(operand) - 1] == '\'')
		{
			while (operand[i] != '\'' && i < 34)
			{
				if (isxdigit(operand[i++] == 0))//if character is not a hex value
					return "5 ";
			}

			if (i >= 34)					//there are more than 32 hex digits 
				return "5 ";				//there is an error (34 includes the 'X' and the first single quote)

			if (i % 2 != 0)					//if the number of hex digits is odd
				return "20 ";
		}
		else
			return "5 ";
	}
	else
		return "5 ";
	
	return "0 ";				//no errors were detected with BYTE's operand
}

char * checkOperandErrors(char * operand)
{
	if (strchr(operand, ',') != NULL)			//if the operand has a comma in it
	{
		if (strcmp(operand, "BUFFER,X") != 0)	//the operand must be "BUFFER,X", if its not, its an error
			return "23 ";
	}
	return "0 ";
}

char * checkSTARTErrors(char * operand)
{
	if(strcmp(operand, "") == 0)	//if there is no START operand
		return "12 ";

	if (!isXDigits(operand) || strlen(operand) > 4)		//if operand is not in hex, or hex value is too big
		return "4 ";

	return "0 ";
}

char * checkENDErrors(char * operand)
{
	if(strcmp(operand, "") == 0)		//if there is no operand
		return "18 ";

	if (strcmp(operand, "FIRST") != 0)	//if END's operand is not "FIRST"
		return "7 ";

	return "0 ";
}

char * checkIfProgramTooLong(int programLength)
{
	if (programLength > 32767)
		return "21 ";

	return "0 ";
}

int errorExists(char * errStr, char * code)	//checks if a specific error exists given a error code to look for
{
	char line[60];
	char * temp;

	strcpy(line, errStr);
	temp = strtok(line, " ");

	while (temp != NULL)
	{
		if (strcmp(temp, code) == 0)	//check if error exists
			return 1;

		temp = strtok(NULL, " ");		//get next error code
	}
	return 0;
}

int errorsExist(char * errStr)			//checks if ANY errors exist for an instruction
{
	char line[60];
	char * temp;

	strcpy(line, errStr);
	temp = strtok(line, " ");

	while (temp != NULL)
	{
		if (strcmp(temp, "0") != 0)
			return 1;

		temp = strtok(NULL, " ");
	}
	return 0;
}

void writeErrMessages(char * errCodes, char ** errTable, FILE * lfp)
{
	char * errMessage = "\0";
	char * temp = "\0";
	char errCodesCopy[60];
	strcpy(errCodesCopy, errCodes);

	temp = strtok(errCodesCopy, " ");
	while (temp != NULL)
	{
		if (strcmp(temp, "0") != 0)					//if error code is not a "0"
		{
			errMessage = errTable[atoi(temp)];		//error code maps to index with corresponding error message
			fputs(errMessage, lfp);					//write error message to listing file
		}				
		temp = strtok(NULL, " ");					//get next error code
	}
}