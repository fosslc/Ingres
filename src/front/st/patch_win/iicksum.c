/*
**  Copyright (c) 2005 Ingres Corporation. All Rights Reserved.
*/

/*
**  Name: iicksum.c: Calculates checksum of the binaries.
**					 USAGE: iicksum [filename]
**
**					 Outputs results in the follwing form:
**					 checksum	size	"full path to the file"
**
**  History:
**		01-Dec-2005 (drivi01)
**			Created.
**
*/

#include <stdio.h>
#include <windows.h>
#include "patchcl.h"


void
main(int argc, char *argv[])
{
unsigned long sum = 0;
HANDLE	hFile = NULL;
DWORD	size = 0;

     if (argc !=2)
     {
	printf("USAGE:    iicksum [filename]\n");
     }
     else
     {
	if ((hFile = CreateFile(argv[1], GENERIC_READ , 0, 
			NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0)) != NULL)
	{
		size = GetFileSize(hFile, NULL);
		CloseHandle(hFile);
		sum = checksum(argv[1]);
		printf("%u\t%d\t%s\n", sum, size, argv[1]);
	}
	else
	{
		printf("Couldn't find file\n");
	}

     }

	return;

}