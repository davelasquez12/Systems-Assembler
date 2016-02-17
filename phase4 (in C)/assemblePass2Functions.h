#pragma once
void assemblePass2(struct hashTable * h, char * startAddressHex, char * programLength,char * rootFileName);
struct lineData * readNextIntermedBlock(FILE * imfp);
void writeToListingFile(struct lineData * lb, char ** errTable, char * singleObjCode, FILE * lfp);
void writeHeaderToObjFile(struct lineData * lb, char * startAddressHex, char * lengthOfProgram, FILE * of);
void writeTextRecToObjFile(char * startAddress, char * objCode, FILE * of);
void writeEndRecToObjFile(char * startAddress, FILE * of);
char * generateObjCode(struct hashTable * h, struct lineData * lb);
int isStringAlpha(char * str);
char * convertConstToObjCode(char * opcode, char * operand);
char * formatAddrToObjCode(char * address);
char * getAddress(struct hashTable * h, char * operand);

struct lineData
{
	char sourceLine[80];
	char address[15];
	char label[15];
	char opcode[15];
	char mnemonic[15];
	char operand[15];
	char errorcodes[100];
	char comment[60];
};

void assemblePass2(struct hashTable * h, char * startAddressHex, char * programLength, char * rootFileName)
{
	char lf[30] = "\0";
	strcat(lf, rootFileName);
	strcat(lf, ".lst");

	char of[30] = "\0";
	strcat(of, rootFileName);
	strcat(of, ".obj");

	char * fn = "intermediate.txt"; /* , *lf = "listingFile.txt", *of = "objectFile.txt";*/
	FILE * imfp = openFileInReadMode(fn);	//opens intermediate file in read mode and checks for any errors opening the file
	FILE * lfp = openFileInWriteMode(lf);	//opens listing file in write mode
	FILE * ofp = openFileInWriteMode(of);	//opens object file in write mode
	char startAddrTextRec[7] = "\0";
	char objCode[61] = "\0";				//holds 60 chars plus one for null terminating character
	char * singleObjCode;		//will be storing obj code of each instruction that will be appended to objCode each iteration
	int objCodeCount = 0;					//used to decide when to write a text record to the object file
	int errorFlag = 0;
	char ** errTable = createErrorTable();	//create error table; free when done

	if (imfp == NULL || lfp == NULL || ofp == NULL)	//if there was a file that did not open properly, don't perform pass 2
		return;

	struct lineData * lineBlock = readNextIntermedBlock(imfp);
	if (strcmp(lineBlock->mnemonic, "START") == 0)		//if the opcode is the START mnemonic
	{
		writeToListingFile(lineBlock, errTable, objCode, lfp);
		writeHeaderToObjFile(lineBlock, startAddressHex, programLength, ofp);	//write header record to object file
		if (errorsExist(lineBlock->errorcodes))
			errorFlag = 1;

		lineBlock = readNextIntermedBlock(imfp);
	}
	else
		writeHeaderToObjFile(lineBlock, startAddressHex, programLength, ofp);	//write header record to object file

	strcpy(startAddrTextRec, lineBlock->address);		//store starting address of first instruction of text record; will be used when writing objCode to the Text Record

	while (strcmp(lineBlock->mnemonic, "END") != 0)	//while the opcode does not equal "END"
	{
		if (lineBlock->sourceLine[0] != '.')		//if this is not a comment line
		{
			if (lineBlock->opcode[0] == '0' && lineBlock->opcode[1] == 'x')	//if the opcode is an actual opcode (ex: 0x4C) and not START, END, RESW, etc
			{
				if (strcmp(lineBlock->operand, "") != 0)	//if there is a symbol in the operand field
				{
					if (strcmp(lineBlock->address, "-111") == 0)	//if operand does not have an address (ie, operand would not be found in the symbol table)
						strcpy(lineBlock->address, "0");
				}

				singleObjCode = generateObjCode(h, lineBlock);		//generates object code of current instruction and stores it in singleObjCode
				if (strcmp(singleObjCode, "") == 0)
					strcat(lineBlock->errorcodes, "23 ");	//23 is the error code for undefined symbol in operand field (ie, if operand is misspelled, error on label in pass 1, etc)
			}
			else if (strcmp(lineBlock->mnemonic, "BYTE") == 0 || strcmp(lineBlock->mnemonic, "WORD") == 0)	//if opcode is "BYTE" or "WORD"
			{
				if (!errorsExist(lineBlock->errorcodes))		//if errors do not exist, convert const to obj code, else do no conversion
					singleObjCode = convertConstToObjCode(lineBlock->mnemonic, lineBlock->operand);	//convert constant value into object code
			}

			if (objCodeCount < 10)											//if object code can fit into the Text record (up to 10 single object codes), append singleObjCode to objCode
			{
				if (singleObjCode != NULL && strlen(singleObjCode) > 6)								//if a single object code is very long write it on its own text record
				{
					writeTextRecToObjFile(startAddrTextRec, objCode, ofp);			//write current obj code to object file
					strcpy(startAddrTextRec, lineBlock->address);					//get new starting address
					writeTextRecToObjFile(startAddrTextRec, singleObjCode, ofp);	//write long object code to its own text record
					strcpy(objCode, "");											//empty out objCode
					objCodeCount = 0;
				}
				else
				{
					if(singleObjCode != NULL)
						strcat(objCode, singleObjCode);							//single object code is short so append it to full object code
					objCodeCount++;
				}
			}
			else
			{																//else, the objCode can no longer store more codes so it's ready to be written to text record
				writeTextRecToObjFile(startAddrTextRec, objCode, ofp);		//write objCode to object file
				strcpy(startAddrTextRec, lineBlock->address);				//get new starting address
				if(singleObjCode != NULL)
					strcpy(objCode, singleObjCode);							//write singleObjCode using strcpy to empty objCode string and prepare it for new text record
				objCodeCount = 1;											//reset objCodeCount to 1
			}							
		}

		writeToListingFile(lineBlock, errTable, singleObjCode, lfp);		//write data to listing file
		if (errorsExist(lineBlock->errorcodes))
			errorFlag = 1;

		free(lineBlock);													//free lineBlock before getting a new lineBlock
		lineBlock = NULL;
		lineBlock = readNextIntermedBlock(imfp);							//read next block of data from intermediate file
		if (singleObjCode != NULL)
		{
			free(singleObjCode);											//free char array pointed to by singleObjCode
			singleObjCode = NULL;
		}
	}
	
	writeTextRecToObjFile(startAddrTextRec,objCode, ofp);					//write last text record to object file
	writeEndRecToObjFile(startAddressHex, ofp);								//write End record to object file
	writeToListingFile(lineBlock, errTable, "", lfp);						//write data to last listing line
	if (errorsExist(lineBlock->errorcodes))
		errorFlag = 1;

	//close files
	fclose(imfp);
	fclose(lfp);
	fclose(ofp);

	//if at least one error was found, delete the object file
	if (errorFlag)															
		remove(of);

	//free allocated data
	freeErrorTable(errTable);
	free(lineBlock);
}

struct lineData * readNextIntermedBlock(FILE * imfp)
{
	struct lineData * lineBlock = malloc(sizeof(struct lineData));
	char temp[80];
	int i = 1;

	//adds data from intermediate file to lineBlock and removes the newline character except from the source line
	while (i % 9 != 0 && fgets(temp, 80, imfp) != NULL)
	{
		if (i == 1)
			strcpy(lineBlock->sourceLine, temp);
		else if (i == 2)
		{
			strcpy(lineBlock->address, temp);
			lineBlock->address[strlen(lineBlock->address) - 1] = '\0';
		}
		else if (i == 3)
		{
			strcpy(lineBlock->label, temp);
			lineBlock->label[strlen(lineBlock->label) - 1] = '\0';
		}
		else if (i == 4)
		{
			strcpy(lineBlock->mnemonic, temp);
			lineBlock->mnemonic[strlen(lineBlock->mnemonic) - 1] = '\0';
		}
		else if (i == 5)
		{
			strcpy(lineBlock->opcode, temp);
			lineBlock->opcode[strlen(lineBlock->opcode) - 1] = '\0';
		}
		else if (i == 6)
		{
			strcpy(lineBlock->operand, temp);
			lineBlock->operand[strlen(lineBlock->operand) - 1] = '\0';
		}
		else if (i == 7)
			strcpy(lineBlock->comment, temp);
		else if (i == 8)
		{
			strcpy(lineBlock->errorcodes, temp);
			lineBlock->errorcodes[strlen(lineBlock->errorcodes) - 1] = '\0';
		}
		i++;
	}
	return lineBlock;
}

char * generateObjCode(struct hashTable * h, struct lineData * lb)
{
	char * singleObjCode = malloc(sizeof(char) * 62);		//stores object code; free when done
	char * targetAddr;			//will need to be freed at the end since it stores address returned from getAddress
	singleObjCode[0] = '\0';

	if (strcmp(lb->opcode, "0x4C") == 0)		//opcode for RSUB
	{
		//write opcode to object record
		char * oc = lb->opcode;
		oc += 2;											//removes the "0x" from the opcode and only keeps the hex part
		strcpy(lb->opcode, oc);								//write hex only opcode to the lineblock
		strcat(singleObjCode, lb->opcode);					//write opcode to object code record
		strcat(singleObjCode, "0000");						//add zeros to object code
	}
	else
	{
		//write opcode to object record
		char * oc = lb->opcode;
		oc += 2;											//removes the "0x" from the opcode and only keeps the hex part
		strcpy(lb->opcode, oc);								//write hex only opcode to the lineblock
		strcat(singleObjCode, lb->opcode);					//write opcode to object code record

		if (strchr(lb->operand, ',') != NULL)			//checks if index value is set to 0 or 1 in order to modify the address or not
		{
			targetAddr = getAddress(h, "BUFFER");
			if (strcmp(targetAddr, "") == 0)			//There was an error assigning an address to BUFFER in pass 1, so the "BUFFER,X" operand is undefined
			{
				strcpy(singleObjCode, targetAddr);
				free(targetAddr);
				return singleObjCode;
			}

			char tempAddr[5];							//will store modified address
			strcpy(tempAddr, targetAddr);				//get copy of unmodified address, store it in tempAddr		
			char firstPos[2];
			firstPos[0] = tempAddr[0];					//get value in first position of address
			firstPos[1] = '\0';							//terminate string
			int firstVal = strtol(firstPos, NULL, 10);	//convert value at firstPos to decimal
			firstVal += 8;								//add 8 to value
			char newVal[2];
			sprintf(newVal, "%X", firstVal);			//convert decimal value back to a hex string, store result in newVal
			tempAddr[0] = newVal[0];					//replace the value at first position of unmodified address with new hex value
			strcat(singleObjCode, tempAddr);			//append modified address to object code string
		}
		else
		{
			targetAddr = getAddress(h, lb->operand);			//look up address of operand in symbol table
			if (strcmp(targetAddr, "") == 0)					//if address was not found
			{
				strcpy(singleObjCode, targetAddr);
				free(targetAddr);
				targetAddr = NULL;
				return singleObjCode;
			}
			strcat(singleObjCode, targetAddr);
		}
		free(targetAddr);
		targetAddr = NULL;
	}
	return singleObjCode;
}

void writeToListingFile(struct lineData * lb, char ** errTable, char * singleObjCode, FILE * lfp)
{
	char line[85] = "\0";
	char temp[85] = "\0";
	char formattedTemp[85] = "\0";

	if (lb->sourceLine[0] == '.')		//if source line is a comment line
	{
		strcat(line, "        ");
		strcat(line, lb->sourceLine);
	}
	else
	{
		//get load address, format it and append it to line
		strcpy(temp, lb->address);
		sprintf(formattedTemp, "%-8s", temp);
		strcat(line, formattedTemp);

		//get label, format it and append it to line
		strcpy(temp, lb->label);
		sprintf(formattedTemp, "%-8s", temp);
		strcat(line, formattedTemp);

		//get mnemonic, format it and append it to line
		strcpy(temp, lb->mnemonic);
		sprintf(formattedTemp, "%-8s", temp);
		strcat(line, formattedTemp);

		//get operand, format it and append it to line
		strcpy(temp, lb->operand);
		sprintf(formattedTemp, "%-12s", temp);
		strcat(line, formattedTemp);

		//get single object code, format it and append it to line
		if (singleObjCode != NULL)
			strcpy(temp, singleObjCode);
		else
			strcpy(temp, "");

		sprintf(formattedTemp, "%-10s", temp);
		strcat(line, formattedTemp);

		//get comment and append it to new line (comment should have a newline char at the end)
		strcpy(temp, lb->comment);
		strcat(line, temp);
	}

	fputs(line, lfp);

	if (errorsExist(lb->errorcodes))
		writeErrMessages(lb->errorcodes, errTable, lfp);	//write error code messages to listing file

}

void writeHeaderToObjFile(struct lineData * lb, char * startAddressHex, char * lengthOfProgram, FILE * of)
{
	char header[21];	//1 for 'H', 6 for name, 6 for starting address, 6 for length of program, and 2 for newline and null chars
	char name[7], startAddress[7], length[7];
	int i, j;

	//initialize arrays
	strcpy(header, "H");
	name[0] = '\0';
	startAddress[0] = '\0';
	length[0] = '\0';
	
	strcat(name, lb->label);
	//adds spaces at the end to 'name' string to fill up bit positions not used
	if (strlen(lb->label) < 6)
	{
		i = 6 - strlen(name);
		for (j = 0; j < i; j++)		
			strcat(name, " ");
	}

	//add name to header string
	strcat(header, name);

	//adds zeros to 'address' string to fill up bit positions not used
	if (strlen(startAddressHex) < 6)
	{
		i = 6 - strlen(startAddressHex);
		for (j = 0; j < i; j++)
			strcat(startAddress, "0");
	}

	//add starting address to header string
	strcat(startAddress, startAddressHex);
	strcat(header, startAddress);
	
	//adds zeros to legnth string to fill up bit positions not used
	if (strlen(lengthOfProgram) < 6)
	{
		i = 6 - strlen(lengthOfProgram);
		for (j = 0; j < i; j++)
			strcat(length, "0");
	}

	//adds length of program to header string
	strcat(length, lengthOfProgram);
	strcat(header, length);
	strcat(header, "\n");	//add newline since this is the end of the header file

	//write header to object file
	fputs(header, of);
}

void writeTextRecToObjFile(char * startAddress, char * objCode, FILE * of)
{
	char textRecord[71];
	char * formattedAddr;
	char objCodeLengthHex[20];

	int objCodeLength = strlen(objCode);			//get number of chars in objcode to determine length in bytes
	objCodeLength = objCodeLength / 2;				//divide length by 2 since every two hex digits is one byte
	sprintf(objCodeLengthHex, "%X", objCodeLength);	//convert length of object code in bytes to hex
	strcpy(textRecord, "T");						//write "T" to text record

	formattedAddr = formatAddrToObjCode(startAddress);
	strcat(textRecord, formattedAddr);				//append the starting address to text record

	if (strlen(objCodeLengthHex) == 1)				//if the object code length in hex only has one 1 digit, add a leading zero to the text record
		strcat(textRecord, "0");

	strcat(textRecord, objCodeLengthHex);			//append length of objcode to text record
	strcat(textRecord, objCode);					//append object code to text record
	strcat(textRecord, "\n");						//append a new line
	fputs(textRecord, of);							//write text record to object file
	free(formattedAddr);							//free memory allocated when formatAddrToObjCode was called
}

void writeEndRecToObjFile(char * startAddress, FILE * of)
{
	char endRecord[8] = "\0";
	int i;
	int zeros = 6 - strlen(startAddress);	//used to determine how many leading zeros to write
	strcat(endRecord, "E");					//append "E" to end record

	for (i = 0; i < zeros; i++)				//write leading zeros
		strcat(endRecord, "0");	

	strcat(endRecord, startAddress);		//append first address of program to end record
	fputs(endRecord, of);					//write end record to object file
}

//fills address with leading zeros if the address is less than 6 chars in length
char * formatAddrToObjCode(char * address)
{
	char * formattedAddr = malloc(sizeof(char) * 7);
	int i;
	strcpy(formattedAddr, "");

	if (strlen(address) < 6)						
	{
		for (i = 0; i < 6 - strlen(address); i++)
			strcat(formattedAddr, "0");

		strcat(formattedAddr, address);
		return formattedAddr;
	}
	else	
		return address;
}

char * convertConstToObjCode(char * mnemonic, char * operand)
{
	char * singleObjCode = malloc(sizeof(char) * 61);

	if (strcmp(mnemonic, "BYTE") == 0)
	{
		if (operand[0] == 'X')							//operand is in the form of: X'...'
		{
			int i;
			char temp[33] = "\0";						//32 chars allowed
			for (i = 0; i < strlen(operand) - 3; i++)
				temp[i] = operand[i + 2];

			temp[i] = '\0';
			strcpy(singleObjCode, temp);
		}
		else if (operand[0] == 'C')						//operand is in the form of: C'...'
		{
			char temp[31] = "\0";						//up to 30 chars allowed
			char tempHex[61] = "\0";
			char hexVal[7] = "\0";						
			int i;
			for (i = 0; i < strlen(operand) - 3; i++)	//loop gets characters within quotes and stores them in temp
				temp[i] = operand[i + 2];

			temp[i] = '\0';
			for (i = 0; i < strlen(temp); i++)			//loop through each character
			{
				sprintf(hexVal, "%X", temp[i]);			//convert each character to hex, store hex value in hexVal
				strcat(tempHex, hexVal);				//append hex values to tempHex
			}
			strcpy(singleObjCode, tempHex);				//append tempHex to singleObjCode for complete object code value
		}
	}
	else
	{													//mnemonic is either WORD, RESB, or RESW
		char tempHex[7] = "\0";
		sprintf(tempHex, "%X", atoi(operand));
		int numHDigits = strlen(tempHex);
		int numLeadZeros = 6 - numHDigits;
		int i;
		strcpy(singleObjCode, "");
		for (i = 0; i < numLeadZeros; i++)
			strcat(singleObjCode, "0");

		strcat(singleObjCode, tempHex);
	}
	return singleObjCode;
}

//returns 1 if entire string is alphabetic, else returns 0
int isStringAlpha(char * str)
{
	int i;
	for (i = 0; i < strlen(str); i++)
	{
		if (!isalpha(str[i]))
			return 0;
	}
	return 1;
}
