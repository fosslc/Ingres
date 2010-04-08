/*
**  Copyright (c) 2005 Ingres Corporation. All Rights Reserved.
*/

/*
**  Name: patchcl.c: Functions used by custom actions and 
**                   Patch Installer.
**
**  History:
**		01-Dec-2005 (drivi01)
**			Created.
**		03-Mar-2006 (drivi01)
**			If AppendToLog fails to retrieve II_SYSTEM then
**			we will output logging info to %TEMP% directory.
**		20-may-2009 (smeke01) b121784
**			Added IsSuffixed() helper function.
**
*/

#include <stdio.h>
#include <windows.h>
#include "patchcl.h"


/*
**  Name: checksum
**  
**  Description: Calculates checksum for binaries.
**
**	Parameters:  filename - Path to binary.
**
**  Return Code: Checksum
**
**  History:
**      24-May-2007 (wanfr01)
**        Bug 118398 - need to close each 
**        file after opening it.
*/
unsigned long 
checksum(char *filename) 
{ 
  int ch; 
  unsigned long sum = 0; 
  FILE *fp;

  fp = fopen(filename, "r");
  if(fp) 
  { 
    while((ch = fgetc(fp)) != EOF) 
    { 
      sum <<= 1; 
      sum ^= ch; 
    } 
  } 
  fclose(fp);
  return sum; 

} 

/*
**  Name: AppendToLog
**  
**  Description: Prints log information to patch.log and
**               places it in II_SYSTEM/ingres/files assuming
**               II_SYSTEM is defined.
**
**	Parameters:  str - Information to be written to the log.
**
**  Return Code:  
*/
void 
AppendToLog(char *str)
{
    DWORD   dw, rv;
    HANDLE	hLogFile;
    char 	ii_system[MAX_PATH+1];
    char	log_path[MAX_PATH+1];
    char*	pstr = str;
    char	ii_id[II_SIZE];
    char 	szBuf[MAX_PATH];
    HKEY	hRegKey;
        
    if ( !( rv = GetEnvironmentVariable("II_SYSTEM", ii_system, sizeof(ii_system)) ) )
    {
	rv = GetEnvironmentVariable("TEMP", ii_system, sizeof(ii_system));
	sprintf(log_path, "%s\\patch.log", ii_system);
    }
    else
    {
	sprintf(log_path, "%s\\ingres\\files\\patch.log", ii_system);
    }


    if (rv)
    {
	    
    	hLogFile = CreateFile( log_path, GENERIC_READ|GENERIC_WRITE , 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (hLogFile != NULL)
		{
		     SetFilePointer(hLogFile, 0, NULL, FILE_END);
		     WriteFile(hLogFile, str, strlen(str), &dw, NULL); 
		}
		FlushFileBuffers(hLogFile);
		CloseHandle(hLogFile);
    }
}

/*
**  Name: IsSuffixed
**  
**  Description: Determines whether a given string has a given suffix.
**
**	Parameters:  The string and suffix to compare.
**
**  Return Code: 
**
**	TRUE - iff the given string is at least one character greater in 
**  length than the given suffix, and the final characters of the given
**  string match all the characters in the given suffix.
**  FALSE - for all other cases.
*/
BOOL 
IsSuffixed(const char * thestring, const char * thesuffix)
{
	size_t lenstring, lensuffix;

	lenstring = strlen(thestring);
	lensuffix = strlen(thesuffix);
	
	if (lenstring < lensuffix + 1)
		return FALSE;
	if (strcmp((const char *)&(thestring[lenstring - lensuffix]), thesuffix) == 0)
		return TRUE;
	else
		return FALSE;
}
