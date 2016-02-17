#pragma once

//This hash table is implemented using separate chaining linked lists.

struct listNode
{
	char * label;
	int address;
	struct listNode * next;
};

struct hashTableNode
{
	int blockCount;			//number of elements in a block
	struct listNode * firstNode;
};

struct hashTable
{
	int tableSize;
	int count;				//number of elements in the table
	struct hashTableNode * table;
};

struct hashTable * createHashTable(int size)
{
	struct hashTable * ht;
	ht = malloc(sizeof(struct hashTable));

	if (!ht)
		return NULL;

	ht->tableSize = size;
	ht->count = 0;
	ht->table = malloc(sizeof(struct hashTableNode) * ht->tableSize);

	if (!ht->table)
	{
		printf("Memory error\n");
		return NULL;
	}

	int i;
	for (i = 0; i < ht->tableSize; i++)
	{
		ht->table[i].blockCount = 0;
		ht->table[i].firstNode = NULL;
	}

	return ht;
}

/*hash function: adds up the ascii values of each
character, multiplies by a prime number (37) and mods the sum wih the table size*/
int hash(char * label, int tableSize)
{
	int hashVal = 0;
	size_t i;

	for (i = 0; i < strlen(label); i++)
		hashVal = 37 * hashVal + label[i];

	hashVal %= tableSize;
	if (hashVal < 0)
		hashVal += tableSize;

	return hashVal;
}

int getIndex(struct hashTable * ht, char * label)
{
	return hash(label, ht->tableSize);
}

int symTabSearch(struct hashTable * h, char * label)
{
	struct listNode * temp;
	temp = h->table[hash(label, h->tableSize)].firstNode; //temp points to the first listNode in table[hashedIndex]

	while (temp)
	{
		if (strcmp(temp->label, label) == 0)
			return 1;	//found

		temp = temp->next;		//go to next link
	}
	return 0;	//not found
}

int insertToSymTab(struct hashTable * h, char * label, int locctr)
{
	int index;
	struct listNode * currentNode, *newNode;

	index = hash(label, h->tableSize);
	currentNode = h->table[index].firstNode;

	newNode = malloc(sizeof(struct listNode));
	newNode->label = malloc(sizeof(char) * 7);	//allocates 7 chars to store label up to 6 chars long (0-5), last one is for the '\0'

	if (!newNode)	//if new node is null
	{
		printf("Error creating new node\n");
		return 0;
	}

	strcpy(newNode->label, label);
	newNode->address = locctr;

	if (h->table[index].firstNode == NULL)		//if first node at table index is empty
	{
		h->table[index].firstNode = newNode;
		h->table[index].firstNode->next = NULL;
	}
	else
	{											//firstNode was not empty, so chain newNode to the next empty node
		while (currentNode->next != NULL)				//go to next available node
			currentNode = currentNode->next;

		currentNode->next = newNode;
		newNode->next = NULL;
	}

	h->table[index].blockCount++;
	h->count++;
	return 1;
}

void freeHashTable(struct hashTable * h)		//might not free memory properly, might crash too, test later
{
	int i, j;
	struct listNode * current, *temp;
	char * tempStr;

	if (!h)		//make sure table even has memory to be freed
		return;

	for (i = 0; i < h->tableSize; i++)
	{
		current = h->table[i].firstNode;
		for (j = 0; j < h->table[i].blockCount; j++)
		{
			temp = current;
			tempStr = current->label;
			current = current->next;
			free(temp);
			free(tempStr);
			temp = NULL;
			tempStr = NULL;
		}
	}

	free(h->table);
	h->table = NULL;
	free(h);
	h = NULL;
}

int getTableCount(struct hashTable * h)
{
	return h->count;
}

void writeHashTableDataToFile(struct hashTable * h)
{
	FILE * fp;
	fp = fopen("hashData.txt", "w");
	struct listNode * temp;

	if (fp == NULL)
	{
		printf("ERROR: file '%s' was not found or not able to open.\n\n", "hashData.txt");
		return;
	}

	int i;

	fprintf(fp, "HASH TABLE DATA\n\n");
	for (i = 0; i < h->tableSize; i++)
	{
		temp = h->table[i].firstNode;
		while(temp != NULL)
		{
			fprintf(fp, "%6s  %X\n", temp->label, temp->address);
			temp = temp->next;
		}
	}
	fclose(fp);
}

char * getAddress(struct hashTable * h, char * operand)
{
	int index = getIndex(h, operand);
	char * address = malloc(sizeof(char) * 10);
	struct listNode * currentNode = h->table[index].firstNode;
	strcpy(address, "");

	if (currentNode == NULL)
		return address;

	if (strcmp(currentNode->label, operand) == 0)				//if label and operand are identical
	{
		sprintf(address, "%X", currentNode->address);			//convert decimal address to a hex string
		return address;											//return hex address
	}
	else
	{															//label and operand were not identical so check nodes in the linked chain
		currentNode = currentNode->next;						//go to next node in chain
		while(currentNode != NULL)
		{
			if (strcmp(currentNode->label, operand) == 0)
			{
				sprintf(address, "%X", currentNode->address);
				return address;									//label was found, so return its address
			}
			else
				currentNode = currentNode->next;
		}
		return address;											//label was not found, so return address which should just have a null char
	}
}
