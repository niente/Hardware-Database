/**
 Hashing to a (binary) file and using advanced string manipulation functions.
 
 This program allows additions to, deletions from, or displays of database records in a  
 hardware store database.
*/
#define _CRT_SECURE_NO_DEPRECATE // allow use of fopen, etc in Visual Studio
#define ID_SIZE 4
#define NAME_SIZE 20
#define TABSIZE 40
#define BUCKETSIZE 3
#define OFLOWSIZE 40
#define FLUSH while( getchar() != '\n') // clean user input
#define DEFAULT_INPUT_FILENAME "input.txt"
#define DEFAULT_OUTPUT_FILENAME "output.txt"

#ifdef _MSC_VER
#include <crtdbg.h>  // needed to check for memory leaks
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct record RECORD;
struct record
{
	char id[ID_SIZE + 1]; // key
	char name[NAME_SIZE + 1]; // product name
	int qty;
};

// function prototypes
FILE *openFile(char *infilename);
void emptyFileTest(FILE *inFile);
FILE *createHashFile();
long hash(char *key, int size);
void insert(const RECORD newRecord, FILE *hashFile);
RECORD *parseLine(char line[100]);
void search_record(FILE *hashFile);
void insert_stdin(FILE *hashFile);
void user_control(FILE *hashFile);
void delete_record(FILE *hashFile);

// argc = 2, argv[] = "HardwareDatabase.c", "input.txt"
int main(int argc, char *argv[])
{
	// debugging
  	printf("Deleting old output.txt\n");
	remove(DEFAULT_OUTPUT_FILENAME);

	char infilename[100];
	strcpy(infilename, argv[1]); // argv[1] is input.txt

	FILE *inFile = openFile(infilename); // open the file
	if (!inFile) 
	{
		printf("Using default file input.txt\n", infilename);
		inFile = fopen(DEFAULT_INPUT_FILENAME, "r");
		if (!inFile) // check for default file
		{
			printf("Unable to open input.txt! Exiting.\n");
			exit(101);
		}
	}
	emptyFileTest(inFile); // check if input.txt is empty

	FILE *hashFile = createHashFile(); // initialize binary file for item database

	char line[100];
	RECORD *newRecord;
	// write item db from input file:
	while (fgets(line, 100, inFile))
	{
		newRecord = parseLine(line);
		insert(*newRecord, hashFile);
		free(newRecord);
	}

	user_control(hashFile);

	// close file validation
	if (fclose(hashFile) == EOF)
	{
		printf("Error closing hash file!\nExiting.\n");
		exit(104);
	}

	// check for memory leak
	#ifdef _MSC_VER
		_CrtDumpMemoryLeaks();
		printf(_CrtDumpMemoryLeaks() ? "Memory Leak\n" : "No Memory Leak\n");
		system("pause");
	#endif

    return 0;
}



/***********************OPENFILE****************************
The openFile function uses char* input for a filename
and opens the file. It returns a pointer to the file stream.
Pre   infilename - char ptr returns this to main()
Post  returns FILE * to the opened input file
*/
FILE *openFile(char *infilename)
{
	printf("Opening input file: %s\n\n", infilename);
	FILE *inFile = fopen(infilename, "r");
	if (!inFile) // check for infilename
		printf("Unable to open %s!\n", infilename);
	return inFile;
}

/***********************EMPTYFILETEST***************************
Accepts a file stream, checks to see if the file is empty.
If it is empty, exit program. Otherwise, rewind to the beginning
of the file.
*/
void emptyFileTest(FILE *inFile)
{
	int size = 1;
	if (inFile)
	{
		fseek(inFile, 0, SEEK_END);
		size = ftell(inFile); // what is distance (in bytes) from beginning of file?
		if (!size) // if it's zero...
		{
			printf("Empty input file!\nExiting.\n");
			inFile = NULL;
			exit(102);
		}
		rewind(inFile);
	}
}

/**********************CREATEHASHFILE*************************
The createHashFile function takes no input and opens a default
output file name. It tests to see if we can write to the file.
Post  returns FILE * to the opened output file
*/
FILE *createHashFile()
{
	printf("Opening output file: %s\n\n", DEFAULT_OUTPUT_FILENAME);
	FILE *hashFile = fopen(DEFAULT_OUTPUT_FILENAME, "w+b");
	RECORD hashtable[TABSIZE][BUCKETSIZE] = { "" };
	RECORD overflow[OFLOWSIZE] = { "" };

	if (!hashFile) // file validation
	{
		printf("Couldn't open %s for writing.\n", DEFAULT_OUTPUT_FILENAME);
		exit(201);
	}

	if (fwrite(&hashtable[0][0], sizeof (RECORD), TABSIZE * BUCKETSIZE, hashFile) < TABSIZE)
	{
		printf("Hash table could not be created. Abort!\n");
		exit(202);
	}

	if (fwrite(overflow, sizeof (RECORD), OFLOWSIZE, hashFile) < OFLOWSIZE)
	{
		printf("Could not create overflow area. Abort!\n");
		exit(203);
	}
	rewind(hashFile);
	return hashFile;
}

/************************HASH************************
Sum each ASCII code in id, cubed. Divide by 40, the
remainder is the hash key.
*/
long hash(char *key, int size)
{
	long address = 0;
	for (; *key != '\0'; key++)
	{
		address += *key * *key * *key;
	}
	return address % 40;
}

/****************************INSERT****************************
The insert function accepts a record and ptr to file as input.
It hashes the record id, and writes the information to hashFile.
*/
void insert(const RECORD newRecord, FILE *hashFile)
{
	RECORD detect, temp;
	int i;

	temp = newRecord;
	long address = hash(temp.id, ID_SIZE);

	// go to beginning of hash bucket
	if (fseek(hashFile, address * BUCKETSIZE * sizeof(RECORD), SEEK_SET) != 0)
	{
		printf("Fatal seek error! Abort!\n");
		exit(301);
	}
	
	// find first available slot in the bucket
	for (i = 0; i < BUCKETSIZE; i++)
	{
		fread(&detect, sizeof(RECORD), 1, hashFile);
		if (*detect.id == '\0') // available slot
		{
			fseek(hashFile, -1L * (signed)sizeof(RECORD), SEEK_CUR);
			fwrite(&temp, sizeof(RECORD), 1, hashFile);
			printf("Insert: Record %s added to bucket %ld.\n", temp.id, address);
			return; // nothing left to do
		}
		else if (strcmp(detect.id, newRecord.id) == 0) // do not insert duplicate IDs! (bucket)
		{
			printf("Duplicate ID detected! Unable to insert %s.\n", newRecord.name);
			return;
		}
	}
	// bucket full: insert into the overflow area
	fseek(hashFile, TABSIZE * BUCKETSIZE * sizeof(RECORD), SEEK_SET);
	for (i = 0; i < OFLOWSIZE; i++)
	{
		fread(&detect, sizeof(RECORD), 1, hashFile);
		if (*detect.id == '\0') // available slot
		{
			fseek(hashFile, -1L * (signed)sizeof(RECORD), SEEK_CUR);
			fwrite(&temp, sizeof(RECORD), 1, hashFile);
				printf("Insert: Record %s added to the overflow slot %d.\n", temp.id, i);
			return;
		}
		else if (strcmp(detect.id, newRecord.id) == 0) // do not insert duplicate IDs! (oflow)
		{
			printf("Duplicate ID detected! Unable to insert %s.\n", newRecord.name);
			return;
		}
	}
	// item not inserted!
	printf("Hash table overflow! Abort!\n");
	exit(302);
}

/*************************PARSELINE****************************
The parseLine function accepts a string as input and uses it 
to build a dynamically allocated record. 
- ID must be 4 numbers
- Name must be 20 chars or less, letters () or space
- Qty must be a number
Pre: char line[100]
Post: RECORD * which later needs free()
*/
RECORD *parseLine(char line[100])
{
	char *tempID, *tempName, *strQty, *end;
	char *digits = "0123456789";
	char *nameChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ()\040";
	int counter, i = 0, tempQty = 0;


	// parse line for ID. Must be 4 numbers, saved as a string
	tempID = strtok(line, ",");
	if (!tempID)
	{
		printf("Unable to read in a value for ID!\nExiting. \n");
		return NULL;
	}
	counter = strspn(tempID, digits);
	if (counter != strlen(tempID) || strlen(tempID) != ID_SIZE)
	{
		printf("ID must be %d digits! Unable to read %s\nExiting.\n", ID_SIZE, tempID);
		return NULL;
	}

	// parse line for name. Can only be [a-zA-Z()\040], 20 chars max
	tempName = strtok(NULL, ":");
	if (!tempName)
	{
		printf("Unable to read in a name!\nExiting. \n");
		return NULL;
	}
	for (i = 0; i < strlen(tempName); i++)
		tempName[i] = toupper(tempName[i]);
	counter = strspn(tempName, nameChars);
	if (counter != strlen(tempName))
	{
		printf("Invalid characters found in name! Unable to read %s\nExiting.\n", tempName);
		return NULL;
	}
	if (strlen(tempName) > NAME_SIZE)
	{
		printf("Name cannot be longer than %d characters! Unable to read %s\nExiting.\n", 
			   NAME_SIZE, tempName);
		return NULL;
	}

	// parse line for qty. Change from string to int
	strQty = strtok(NULL, "\n");
	if (!strQty)
	{
		printf("Unable to read in a value for quantity!\nExiting. \n");
		return NULL;
	}
	tempQty = strtol(strQty, &end, 10);
	if (*end != '\0')
	{
		printf("Error reading qty! Non-numeric characters found.\n");
		return NULL;
	}
	else if (tempQty < 0 || tempQty > 9999)
	{
		printf("Qty %d is out of range! Must be 0-9999.\n", tempQty);
		return NULL;
	}

	// create record
	RECORD *newRecord = (RECORD *)malloc(sizeof(RECORD));
	strcpy(newRecord->id, tempID);
	strcpy(newRecord->name, tempName);
	newRecord->qty = tempQty;
	return newRecord;
}

/*********************SEARCH_RECORD************************
This function takes a filstream to a hashed binary file as
input. It prompts the user to enter an ID to search for.
It hashes the ID, searches the file bucket & overflow area,
and prints the data if found.
*/
void search_record(FILE *hashFile)
{
	RECORD detect;
	int i, counter, found;
	char *digits = "1234567890";
	char targetID[100];

	while (printf("Please enter a 4 digit ID to search for, or type Q to quit:\n"),
		   gets(targetID), strcmp(targetID, "q") != 0 && strcmp(targetID, "Q") != 0)
	{
		found = 0;
		counter = strspn(targetID, digits);
		if (counter != strlen(targetID) || strlen(targetID) != ID_SIZE)
			printf("ID must be %d digits! Unable to read %s.\n", ID_SIZE, targetID);
		else
		{
				long address = hash(targetID, ID_SIZE);
				if (fseek(hashFile, address * BUCKETSIZE * sizeof(RECORD), SEEK_SET) != 0)
				{
					printf("Fatal seek error! Abort");
					exit(4);
				}
				// should stop searching once its found
				for (i = 0; i < BUCKETSIZE && found == 0; i++)
				{
					fread(&detect, sizeof(RECORD), 1, hashFile);
					if (strcmp(detect.id, targetID) == 0) // found it!
					{
						printf("ID %s found:\n%s %s %d\n", targetID, detect.id, detect.name, detect.qty);
						found = 1;
					}
				}
				// check the overflow area
				fseek(hashFile, TABSIZE * BUCKETSIZE * sizeof(RECORD), SEEK_SET);
				for (i = 0; i < OFLOWSIZE && found == 0; i++)
				{
					fread(&detect, sizeof(RECORD), 1, hashFile);
					if (strcmp(detect.id, targetID) == 0) // found it!
					{
						printf("ID %s found in overflow slot %d:\n%s %s %d\n",
							targetID, i, detect.id, detect.name, detect.qty);
						found = 1;
					}
				}
				// not found
				if (found == 0)
					printf("Records with ID %s not found.\n", targetID);
		}
	}
}

/****************************INSERT_STDIN****************************
This function prompts the user to enter a line manually from 
standard input to be added to the database.
*/
void insert_stdin(FILE *hashFile)
{
	char input[100] = "test";
	RECORD *newRecord;
	// instructions
	printf("To insert an item, please enter a line of text in the following format:\n");
	printf("####,ITEM NAME:##\n(ID),         :Quantity\n");
	
	while (printf("Please enter a line to parse, or type Q to quit:\n"),
		   gets(input), strcmp(input, "q") != 0 && strcmp(input, "Q") != 0)
	{
		newRecord = parseLine(input);
		if (newRecord)
		{
			insert(*newRecord, hashFile);
			free(newRecord);
		}
	}
}

/****************************INSERT_FILE****************************
This function prompts the user to enter a filename, and inserts
it in the same way as the original input file.
*/
void insert_file(FILE *hashFile)
{
	char infilename[100];

	while (printf("\nPlease enter the name of the file to insert, or Q to quit:\n"),
		gets(infilename), strcmp(infilename, "q") != 0 && strcmp(infilename, "Q") != 0)
	{
		FILE *inFile = openFile(infilename); // open the file
		if (inFile)
		{
			emptyFileTest(inFile); // check if empty
			RECORD *newRecord;
			char line[100];
			while (fgets(line, 100, inFile))
			{
				newRecord = parseLine(line);
				insert(*newRecord, hashFile);
				free(newRecord);
			}
			// close file validation
			if (fclose(inFile) == EOF)
			{
				printf("Error closing input file!\nExiting.\n");
				exit(103);
			}
		}
	}
}
/****************************DELETE****************************
This function prompts the user to enter an ID to delete.
It searches for the ID and replaces it with an empty record.
*/
void delete_record(FILE *hashFile)
{
	RECORD detect;
	RECORD emptyRecord = { "", "", 0 };
	int i, counter, found;
	char *digits = "1234567890";
	char targetID[100];
	while (printf("Enter the ID of a record you want to delete, or Q to quit.\n"),
		gets(targetID), strcmp(targetID, "q") != 0 && strcmp(targetID, "Q") != 0)
	{
		found = 0;
		counter = strspn(targetID, digits);
		if (counter != strlen(targetID) || strlen(targetID) != ID_SIZE)
			printf("ID must be %d digits! Unable to read %s.\n", ID_SIZE, targetID);
		else
		{
			long address = hash(targetID, ID_SIZE);
			if (fseek(hashFile, address * BUCKETSIZE * sizeof(RECORD), SEEK_SET) != 0)
			{
				printf("Fatal seek error! Abort");
				exit(4);
			}
			for (i = 0; i < BUCKETSIZE && found == 0; i++)
			{
				fread(&detect, sizeof(RECORD), 1, hashFile);
				if (strcmp(detect.id, targetID) == 0) // found it!
				{
					printf("Deleting record:\n%s %s %d\n", detect.id, detect.name, detect.qty);
					fseek(hashFile, -1L * (signed)sizeof(RECORD), SEEK_CUR);
					fwrite(&emptyRecord, sizeof(RECORD), 1, hashFile);
					found = 1;
				}
			}
			// check the overflow area
			fseek(hashFile, TABSIZE * BUCKETSIZE * sizeof(RECORD), SEEK_SET);
			for (i = 0; i < OFLOWSIZE && found == 0; i++)
			{
				fread(&detect, sizeof(RECORD), 1, hashFile);
				if (strcmp(detect.id, targetID) == 0) // found it!
				{
					printf("Deleting record from overflow:\n%s %s %d\n", detect.id, detect.name, detect.qty);
					fseek(hashFile, -1L * (signed)sizeof(RECORD), SEEK_CUR);
					fwrite(&emptyRecord, sizeof(RECORD), 1, hashFile);
					found = 1;
				}
			}
			// not found
			if (found == 0)
				printf("Records with ID %s not found.\n", targetID);
		}
	}
}

/****************************USER_CONTROL****************************
This function prompts the user to enter a code corresponding to 
what task they want to do, and runs the specified function
1: search
2: insert from stdin
3: insert from file
4: delete
Q: exit
*/
void user_control(FILE *hashFile)
{
	char flag[10] = "";
	while (printf("\nTo search the item database, press 1.\nTo insert from standard input, press 2.\n"),
		   printf("To insert from a file, press 3.\nTo delete a record, press 4.\nTo quit, press Q.\n"),
		   gets(flag), strcmp(flag, "q") != 0 && strcmp(flag, "Q") != 0)
	{
		switch (*flag) // dereference flag (string) to get char
		{
		case '1':
			search_record(hashFile);
			break;
		case '2':
			insert_stdin(hashFile);
			break;
		case '3':
			insert_file(hashFile);
			break;
		case '4':
			delete_record(hashFile);
			break;
		default:
			printf("%s is an invalid flag!\n", flag);
			break;
		}
	}	
}
/******************************SAMPLE OUTPUT 1*********************************

Deleting old output.txt
Opening input file: input.txt

Opening output file: output.txt

Insert: Record 6745 added to bucket 4.
Insert: Record 5675 added to bucket 33.
Insert: Record 1235 added to bucket 17.
Insert: Record 2341 added to bucket 28.
Insert: Record 8624 added to bucket 8.
Insert: Record 9162 added to bucket 26.
Insert: Record 7146 added to bucket 16.
Insert: Record 2358 added to bucket 24.
Insert: Record 1622 added to bucket 33.
Insert: Record 1832 added to bucket 36.
Insert: Record 3271 added to bucket 35.
Insert: Record 4717 added to bucket 7.
Insert: Record 9524 added to bucket 38.
Insert: Record 1524 added to bucket 14.
Insert: Record 5219 added to bucket 39.
Insert: Record 6275 added to bucket 36.
Insert: Record 5392 added to bucket 1.
Insert: Record 5192 added to bucket 39.

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
1
Please enter a 4 digit ID to search for, or type Q to quit:
9162
ID 9162 found:
9162 FLASH LIGHT 25
Please enter a 4 digit ID to search for, or type Q to quit:
1832
ID 1832 found:
1832 THERMOSTAT 78
Please enter a 4 digit ID to search for, or type Q to quit:
5192
ID 5192 found:
5192 SCREW DRIVER 789
Please enter a 4 digit ID to search for, or type Q to quit:
q

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
2
To insert an item, please enter a line of text in the following format:
####,ITEM NAME:##
(ID),         :Quantity
Please enter a line to parse, or type Q to quit:
1111, TEST ITEM:100
Insert: Record 1111 added to bucket 36.
Please enter a line to parse, or type Q to quit:
2222, snickers (yum):555
Insert: Record 2222 added to bucket 0.
Please enter a line to parse, or type Q to quit:
3333, notepad (green):0
Insert: Record 3333 added to bucket 4.
Please enter a line to parse, or type Q to quit:
q

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
3

Please enter the name of the file to insert, or Q to quit:
moreinput.txt
Opening input file: moreinput.txt

Insert: Record 1238 added to the overflow slot 0.
Insert: Record 1327 added to bucket 35.
Insert: Record 8123 added to the overflow slot 1.
Insert: Record 5934 added to bucket 9.
Duplicate ID detected! Unable to insert ACETYLENE TORCH.
Insert: Record 5349 added to bucket 9.
Insert: Record 2756 added to the overflow slot 2.
Insert: Record 3495 added to bucket 9.

Please enter the name of the file to insert, or Q to quit:
q

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
4
Enter the ID of a record you want to delete, or Q to quit.
3495
Deleting record:
3495 BOLT (HEX) 987
Enter the ID of a record you want to delete, or Q to quit.
1238
Deleting record from overflow:
1238 WELDING TORCH 18
Enter the ID of a record you want to delete, or Q to quit.
5192
Deleting record:
5192 SCREW DRIVER 789
Enter the ID of a record you want to delete, or Q to quit.
2222
Deleting record:
2222  SNICKERS (YUM) 555
Enter the ID of a record you want to delete, or Q to quit.
q

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
q
No Memory Leak
Press any key to continue . . .

*/

/*********************SAMPLE OUTPUT 2 (ERROR CHECKING)**********************

Deleting old output.txt
Opening input file: input.txt

Opening output file: output.txt

Insert: Record 6745 added to bucket 4.
Insert: Record 5675 added to bucket 33.
Insert: Record 1235 added to bucket 17.
Insert: Record 2341 added to bucket 28.
Insert: Record 8624 added to bucket 8.
Insert: Record 9162 added to bucket 26.
Insert: Record 7146 added to bucket 16.
Insert: Record 2358 added to bucket 24.
Insert: Record 1622 added to bucket 33.
Insert: Record 1832 added to bucket 36.
Insert: Record 3271 added to bucket 35.
Insert: Record 4717 added to bucket 7.
Insert: Record 9524 added to bucket 38.
Insert: Record 1524 added to bucket 14.
Insert: Record 5219 added to bucket 39.
Insert: Record 6275 added to bucket 36.
Insert: Record 5392 added to bucket 1.
Insert: Record 5192 added to bucket 39.

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
1
Please enter a 4 digit ID to search for, or type Q to quit:
abc
ID must be 4 digits! Unable to read abc.
Please enter a 4 digit ID to search for, or type Q to quit:
0
ID must be 4 digits! Unable to read 0.
Please enter a 4 digit ID to search for, or type Q to quit:
11111
ID must be 4 digits! Unable to read 11111.
Please enter a 4 digit ID to search for, or type Q to quit:
q

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
1
Please enter a 4 digit ID to search for, or type Q to quit:
1111
Records with ID 1111 not found.
Please enter a 4 digit ID to search for, or type Q to quit:
q

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
2
To insert an item, please enter a line of text in the following format:
####,ITEM NAME:##
(ID),         :Quantity
Please enter a line to parse, or type Q to quit:
11111,Test Item:0
ID must be 4 digits! Unable to read 11111
Exiting.
Please enter a line to parse, or type Q to quit:
4717, Duplicate Test:100
Duplicate ID detected! Unable to insert  DUPLICATE TEST.
Please enter a line to parse, or type Q to quit:
abcd,Test Item:100
ID must be 4 digits! Unable to read abcd
Exiting.
Please enter a line to parse, or type Q to quit:
1111, Long Item Name (very long):100
Name cannot be longer than 20 characters! Unable to read  LONG ITEM NAME (VERY LONG)
Exiting.
Please enter a line to parse, or type Q to quit:
1111, Weird Item (2):100
Invalid characters found in name! Unable to read  WEIRD ITEM (2)
Exiting.
Please enter a line to parse, or type Q to quit:
1111, Item.Test:100
Invalid characters found in name! Unable to read  ITEM.TEST
Exiting.
Please enter a line to parse, or type Q to quit:
1111, Test Item:
Unable to read in a value for quantity!
Exiting.
Please enter a line to parse, or type Q to quit:
1111, Test Item:10000
Qty 10000 is out of range! Must be 0-9999.
Please enter a line to parse, or type Q to quit:
1111, Test Item:abc
Error reading qty! Non-numeric characters found.
Please enter a line to parse, or type Q to quit:
1234TestItem26
ID must be 4 digits! Unable to read 1234TestItem26
Exiting.
Please enter a line to parse, or type Q to quit:
q

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
3

Please enter the name of the file to insert, or Q to quit:
emptyfile.txt
Opening input file: emptyfile.txt

Empty input file!
Exiting.

Please enter the name of the file to insert, or Q to quit:
thisfiledoesntexist.txt
Opening input file: thisfiledoesntexist.txt

Unable to open thisfiledoesntexist.txt!

Please enter the name of the file to insert, or Q to quit:
q

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
4
Enter the ID of a record you want to delete, or Q to quit.
1111
Records with ID 1111 not found.
Enter the ID of a record you want to delete, or Q to quit.
abcd
ID must be 4 digits! Unable to read abcd.
Enter the ID of a record you want to delete, or Q to quit.
11111
ID must be 4 digits! Unable to read 11111.
Enter the ID of a record you want to delete, or Q to quit.
5192
Deleting record:
5192 SCREW DRIVER 789
Enter the ID of a record you want to delete, or Q to quit.
5192
Records with ID 5192 not found.
Enter the ID of a record you want to delete, or Q to quit.
3
ID must be 4 digits! Unable to read 3.
Enter the ID of a record you want to delete, or Q to quit.
q

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
5
5 is an invalid flag!

To search the item database, press 1.
To insert from standard input, press 2.
To insert from a file, press 3.
To delete a record, press 4.
To quit, press Q.
q
No Memory Leak
Press any key to continue . . .

*/