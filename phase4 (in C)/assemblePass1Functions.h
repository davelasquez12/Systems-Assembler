#pragma once
#include "hash.h"
#include "errorFunctions.h"

void assemble(char ** params, int * numParams);
FILE * openFileInReadMode(char * filename);
FILE * openFileInWriteMode(char * filename);
void assemblePass1(FILE * fp, char * sourceFile);
struct opT * createOPtable();
char ** breakUpLine(char * line);
int isSpaces(char * str);
char ** readAndParseLine(FILE * sourceF);
void writeLineToIntermed(char ** brokenUpLine, FILE * imfp);
void freeBrokenUpLine(char ** brokenUpLine);
char * searchOpTab(struct opT * opTab, char * mnemonic);
int getLengthInBytes(char * operand);
void assemblePass2(struct hashTable * h, char * startAddressHex, char * programLength, char * rootFileName);
char * removeLeadingSpaces(char * str);
char * getFileRootName(char * sourceFile);

struct opT
{
	char * opCode;
	const char * mnemonic;
};

void assemble(char ** params, int * numParams)
{
	if (*numParams == 1)
	{
		FILE * fp = openFileInReadMode(params[0]);	//opens source file in read mode
		if (fp != NULL)	//file was opened properly
			assemblePass1(fp, params[0]);
		else
			return;	//error

		fclose(fp);		//close file
	}
	else if (*numParams == 0)
		printf("ERROR: The 'assemble' command must have ONE filename parameter.\n\n");
	else
		printf("ERROR: The 'assemble' command must only have ONE filename as a parameter.\n\n");
}

FILE * openFileInReadMode(char * filename)
{
	FILE * fp;
	fp = fopen(filename, "r");
	if (fp == NULL)
	{
		printf("ERROR: file '%s' was not found or not able to open.\n\n", filename);
		return NULL;
	}
	return fp;
}

FILE * openFileInWriteMode(char * filename)
{
	FILE * fp;
	fp = fopen(filename, "w");
	if (fp == NULL)
	{
		printf("ERROR: file '%s' was not found or not able to open.\n\n", filename);
		return NULL;
	}
	return fp;
}

char * getFileRootName(char * sourceFile)
{
	char * rootName = malloc(sizeof(char) * 30); //free when done
	char temp[30] = "\0";
	strcpy(temp, sourceFile);
	strtok(temp, ".");			//store filename to temp until the '.' is reached
	strcpy(rootName, temp);		//store filename without its extension to rootName
	return rootName;			//free this in assemblePass1
}

void assemblePass1(FILE * sourceF, char * sourceFile)
{
	int start, LOCCTR, programLength, found;
	int sourceI = 0, addressI = 1, labelI = 2, mnemonicI = 3, opcodeI = 4, operandI = 5, commentI = 6, errorI = 7;
	char * startAddressHex = malloc(sizeof(char) * 7);
	char * programLengthHex = malloc(sizeof(char) * 7);
	char * opCode, *errC;
	char ** brokenUpLine;
	startAddressHex[0] = '\0';
	programLengthHex[0] = '\0';
	char * rootFileName = getFileRootName(sourceFile);	//gets "source" from "source.asm"; free rootFileName when done
	struct opT * opTab = createOPtable();				//creates op table; free it when done

	if (opTab == NULL)
	{
		printf("Error allocating memory!\n\n");
		return;
	}

	//create symbol table (hash table)
	struct hashTable * ht = createHashTable(769);	//769 is prime and provides good distribution for 500 label strings
	if (!ht)
	{
		printf("Error creating hash table!\n\n");
		return;
	}

	//create intermediate file, check for errors opening
	char * fn = "intermediate.txt";
	FILE * imfp = openFileInWriteMode(fn);
	if (imfp == NULL)
		return;

	//reads first source line and parses it
	brokenUpLine = readAndParseLine(sourceF);
	if (brokenUpLine != NULL)
	{
		if (strcmp(brokenUpLine[mnemonicI], "START") == 0)
		{
			errC = checkSTARTErrors(brokenUpLine[operandI]);
			if (errC[0] != '0')							//if there's an error with start's operand
			{
				start = 0;
				strcpy(startAddressHex, "0");
				strcat(brokenUpLine[errorI], errC);		//add error code to brokenUpLine array, so it can be written on the intermediate file
			}
			else
			{
				strcpy(startAddressHex, brokenUpLine[operandI]);		//store hex value of start address
				start = (int)strtol(brokenUpLine[operandI], NULL, 16);	//if no errors, save #operand as starting address, convert hex to decimal and store in an int
			}
				
			LOCCTR = start;								//initialize LOCCTR to starting address
			writeLineToIntermed(brokenUpLine, imfp);	//write line to intermediate file
			freeBrokenUpLine(brokenUpLine);				//free brokenUpLine
			brokenUpLine = readAndParseLine(sourceF);	//read next line
		}
		else
		{
			LOCCTR = 0;									//initialize LOCCTR to 0
			strcat(brokenUpLine[errorI], "15 ");		//missing START statement error code
		}
	}

	while (strcmp(brokenUpLine[mnemonicI], "END") != 0)
	{
		if (brokenUpLine[sourceI][0] != '.')					//if this is not a comment line
		{
			if (strcmp(brokenUpLine[labelI], "") != 0)			//if there is a symbol in the LABEL field
			{
				found = symTabSearch(ht, brokenUpLine[labelI]);	//search symbol table
				if (found)								
					strcat(brokenUpLine[errorI], "1 ");			//1 means duplicate error
				else
				{
					if (getTableCount(ht) < 500)				//a max of 500 symbols are allowed in the symbol table
					{
						if (!errorExists(brokenUpLine[errorI], "3") && !errorExists(brokenUpLine[errorI], "1"))	//if there are no errors with the label, then insert to hash table
						{
							if (!insertToSymTab(ht, brokenUpLine[labelI], LOCCTR))	//insert address to label in symbol table, if a 0 is returned there was a memory error
								strcat(brokenUpLine[errorI], "25 ");
						}
					}
					else
						strcat(brokenUpLine[errorI], "22 ");						//there are too many symbols (error code 7), do not store symbol to the symbol table
				}
			}

			sprintf(brokenUpLine[addressI], "%X", LOCCTR);			//save LOCCTR in hex to every source line;
			opCode = searchOpTab(opTab, brokenUpLine[mnemonicI]);	//search for mnemonic in opTable, return opcode
							
			if (opCode != NULL)		
			{														
				strcpy(brokenUpLine[opcodeI], opCode);				//store opcode in opcode row
				LOCCTR += 3;
			}
			else if (strcmp(brokenUpLine[mnemonicI], "WORD") == 0)
				LOCCTR += 3;
			else if (strcmp(brokenUpLine[mnemonicI], "RESW") == 0)
				LOCCTR += 3 * atoi(brokenUpLine[operandI]);
			else if (strcmp(brokenUpLine[mnemonicI], "RESB") == 0)
				LOCCTR += atoi(brokenUpLine[operandI]);
			else if (strcmp(brokenUpLine[mnemonicI], "BYTE") == 0 && !errorExists(brokenUpLine[errorI], "5") && !errorExists(brokenUpLine[errorI], "20"))
				LOCCTR += getLengthInBytes(brokenUpLine[operandI]);
			else
				strcat(brokenUpLine[errorI], "24 ");	//24 is the error code for invalid/illegal operation/mnemonic
		}//end (if not a comment)

		writeLineToIntermed(brokenUpLine, imfp);
		freeBrokenUpLine(brokenUpLine);
		brokenUpLine = readAndParseLine(sourceF);	//read and parse next source line
		if (strcmp(brokenUpLine[mnemonicI], "START") == 0)
			strcat(brokenUpLine[errorI], "2 ");		//error 2: duplicate or misplaced start statement
	}//end while loop

	programLength = LOCCTR - start;											//save (LOCCTR - starting address) as program length
	strcat(brokenUpLine[errorI], checkENDErrors(brokenUpLine[operandI]));	//add error code to error row for the END directive
	strcat(brokenUpLine[errorI], checkIfProgramTooLong(programLength));		//check if program is too long
	writeLineToIntermed(brokenUpLine, imfp);								//write last line to intermediate file
	fclose(imfp);															//close intermediate file
	sprintf(programLengthHex, "%X", programLength);							//convert program length from decimal to hex
	assemblePass2(ht, startAddressHex, programLengthHex, rootFileName);		//begin pass 2

	//write hash table data to hashData.txt file and free allocated memory
	writeHashTableDataToFile(ht);
	free(startAddressHex);
	free(programLengthHex);
	free(rootFileName);
	freeHashTable(ht);
	free(opTab);
	printf("Assembled source file successfully!\n\n");
}

char ** readAndParseLine(FILE * sourceF)
{
	char ** brokenUpLine;
	char line[260];

	if (fgets(line, 260, sourceF) != NULL)
	{
		brokenUpLine = breakUpLine(line);
		return brokenUpLine;
	}
	return NULL;
}

char ** breakUpLine(char * line)
{
	int row = 8, cols = 200, i = 0;
	int hasLabel = 0, hasComment = 0, hasOperand = 0;	//my boolean variables: 1 means true, 0 means false
	char * token = NULL;

	char ** arr = malloc(row * sizeof(char*));	//allocate 8 rows of memory (source line [0], address [1], label [2], mnemonic [3], opcode[4], operand[5], comment[6], errorcode[7])
	for (i = 0; i < row; i++)
	{
		arr[i] = malloc(cols * sizeof(char));	//allocates 200 columns to store a string per row
		strcpy(arr[i], "");								//set to default an empty string
	}

	strcpy(arr[0], line);								//store source line
	strcpy(arr[1], "-111");								//store a default address of -111
	strcpy(arr[4], "");									//make opcode field empty

	//check for label, get label if it exists and store it in arr[2]
	if (line[0] == '\t' || line[0] == ' ' || line[0] == '\n' || line[0] == '\0');	//if there is no label, just continue since an empty string is already stored in the label row
	else if (line[0] == '.')							//if the line is a comment
	{
		token = strtok(line, "\n");
		strcpy(arr[6], token);
		strcpy(arr[7], "0");
		return arr;
	}
	else
	{													//there is a label
		token = strtok(line, " \t");					//parse label
		strcat(arr[7], checkForLabelErrors(token));		//stores error code for labels
		strcpy(arr[2], token);							//store label in label row
		hasLabel = 1;
	}

	//get mnemonic/directive, store it in arr[3]
	if (hasLabel)	
		token = strtok(NULL, " \t");
	else
		token = strtok(line, " \t");	//does not have a label
	strcpy(arr[3], token);				//store parsed mnemonic in mnemonic row

	//check for operand, get operand if it exists and store in operand row (arr[4])
	if (strcmp(arr[3], "RSUB") != 0)	//if the mnenomic is not "RSUB", the operand should exist
	{
		token = strtok(NULL, " \t\n\r");	//parse operand
		if (token == NULL)					//if their is no operand
		{
			strcat(arr[7], checkForDirOperandErrors(arr[3], ""));
			return arr;
		}
			
		strcat(arr[7], checkOperandErrors(token));					//checks to make sure if the operand has a comma, then the operand should be "BUFFER,X", 
		strcat(arr[7], checkForDirOperandErrors(arr[3], token));	//add error code to error line, returns a "4 " if an error was found
		strcpy(arr[5], token);		//store operand in operand row
	}
	else
		strcpy(arr[5], "");		//store empty string to operand row since there is no operand

	//check if comment exists, store in arr[6]
	token = strtok(NULL, "\t\n");
	if (token == NULL)
		strcpy(arr[6], "");
	else
	{
		if (isSpaces(token))		//if token was ONLY spaces
			token = strtok(NULL, "\t\n");

		if (token[0] == ' ')
			strcpy(token, removeLeadingSpaces(token));

		strcpy(arr[6], token);
	}
	return arr;
}

void freeBrokenUpLine(char ** brokenUpLine)
{
	int i;
	for (i = 0; i < 8; i++)
	{
		free(brokenUpLine[i]);
		brokenUpLine[i] = NULL;
	}
	free(brokenUpLine);
	brokenUpLine = NULL;
}

void writeLineToIntermed(char ** brokenUpLine, FILE * imfp)
{
	//add new lines so each string is stored on its own line in the intermediate file
	strcat(brokenUpLine[1], "\n");
	strcat(brokenUpLine[2], "\n");
	strcat(brokenUpLine[3], "\n");
	strcat(brokenUpLine[4], "\n");
	strcat(brokenUpLine[5], "\n");
	strcat(brokenUpLine[6], "\n");
	strcat(brokenUpLine[7], "\n");
	
	fputs(brokenUpLine[0], imfp);			//write entire source line to intermediate file
	fputs(brokenUpLine[1], imfp);			//write address to intermediate file
	fputs(brokenUpLine[2], imfp);			//write label to intermediate file
	fputs(brokenUpLine[3], imfp);			//write mnemonic to intermediate file
	fputs(brokenUpLine[4], imfp);			//write opcode to intermediate file
	fputs(brokenUpLine[5], imfp);			//write operand to intermediate file
	fputs(brokenUpLine[6], imfp);			//write comments to intermediate file
	fputs(brokenUpLine[7], imfp);			//write error code to intermediate file
}

int isSpaces(char * str)
{
	size_t i;
	for (i = 0; i < strlen(str); i++)
	{
		if (isspace(str[i]))	//if a space, continue to check next character
			continue;
		else
			return 0;
	}
	return 1;
}

int getLengthInBytes(char * operand)	//used with the BYTE directive's operand
{
	int length = 0, i = 2;

	while (operand[i++] != '\'')
		length++;					//number of char or hex values

	if (operand[0] == 'C')
		return length;
	else
		return length / 2;			//2 hex digits per byte
}

struct opT * createOPtable()
{
	const int size = 25;		//will hold 25 structs in array

	struct opT * opTab = malloc(size * sizeof(struct opT));	//make array op table of 25 op structs
	if (opTab == NULL)
		return NULL;			//if error allocating memory

	opTab[0].mnemonic = "ADD";
	opTab[0].opCode = "0x18";

	opTab[1].mnemonic = "AND";
	opTab[1].opCode = "0x58";

	opTab[2].mnemonic = "COMP";
	opTab[2].opCode = "0x28";

	opTab[3].mnemonic = "DIV";
	opTab[3].opCode = "0x24";

	opTab[4].mnemonic = "J";
	opTab[4].opCode = "0x3C";

	opTab[5].mnemonic = "JEQ";
	opTab[5].opCode = "0x30";

	opTab[6].mnemonic = "JGT";
	opTab[6].opCode = "0x34";

	opTab[7].mnemonic = "JLT";
	opTab[7].opCode = "0x38";

	opTab[8].mnemonic = "JSUB";
	opTab[8].opCode = "0x48";

	opTab[9].mnemonic = "LDA";
	opTab[9].opCode = "0x00";

	opTab[10].mnemonic = "LDCH";
	opTab[10].opCode = "0x50";

	opTab[11].mnemonic = "LDL";
	opTab[11].opCode = "0x08";

	opTab[12].mnemonic = "LDX";
	opTab[12].opCode = "0x04";

	opTab[13].mnemonic = "MUL";
	opTab[13].opCode = "0x20";

	opTab[14].mnemonic = "OR";
	opTab[14].opCode = "0x44";

	opTab[15].mnemonic = "RD";
	opTab[15].opCode = "0xD8";

	opTab[16].mnemonic = "RSUB";
	opTab[16].opCode = "0x4C";

	opTab[17].mnemonic = "STA";
	opTab[17].opCode = "0x0C";

	opTab[18].mnemonic = "STCH";
	opTab[18].opCode = "0x54";

	opTab[19].mnemonic = "STL";
	opTab[19].opCode = "0x14";

	opTab[20].mnemonic = "STX";
	opTab[20].opCode = "0x10";

	opTab[21].mnemonic = "SUB";
	opTab[21].opCode = "0x1C";

	opTab[22].mnemonic = "TD";
	opTab[22].opCode = "0xE0";

	opTab[23].mnemonic = "TIX";
	opTab[23].opCode = "0x2C";

	opTab[24].mnemonic = "WD";
	opTab[24].opCode = "0xDC";

	return opTab;
}

//uses binary search
char * searchOpTab(struct opT * opTab, char * mnemonic)
{
	size_t bottom = 0, mid, top = 24;

	while (bottom <= top)
	{
		mid = (bottom + top) / 2;
		if (strcmp(mnemonic, opTab[mid].mnemonic) == 0)
			return opTab[mid].opCode;	//opCode found
		else if (strcmp(mnemonic, opTab[mid].mnemonic) < 0)
			top = mid - 1;		//search left
		else
			bottom = mid + 1;	//search right
	}
	return NULL;	//item not found in op table
}

char * removeLeadingSpaces(char * str)
{
	int i = 0;
	char temp[100] = "\0";
	char * ptemp = temp;
	strcpy(ptemp, str);
	while (temp[i] == ' ')
	{
		ptemp++;
		i++;
	}
	return ptemp;
}