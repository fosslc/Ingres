/*
**  Copyright (c) 2001 Ingres Corporation
*/

/*
**  Name: msidepcheck.c
**
**  Description:
**	Generate a list of all missing files.
**	
**  History:
**	06-jul-2004 (penga03)
**	    Created.
**      21-Sep-2005 (drivi01)
**          Updated this file to check for zero-length
**          files and flag them if any found.
**      6-Mar-2006 (drivi01)
**          Added routines for taking -p argument which
**          would force msidepcheck to output all files
**          being shipped in %ING_BUILD%\picklist.dat
**	02-May-2006 (drivi01)
**	    In case of dotnet distributables don't lower
**	    case of the distributables. 
**	31-Jul-2008 (drivi01)
**	    Update the query for "-p" flag that returns
**	    list of files in the merge modules to return
**	    unique results without duplicates.
**	04-Jan-2010 (drivi01)
**	    Fix the way "\ingres\" is stripped from the
**	    path of each file included in ism as "\ingres\"
**	    may not be included in all the paths so do not
**	    move 7 spaces in the array blindly.
*/

#include <windows.h>
#include <winbase.h>
#include <msiquery.h>
#include <compat.h>

i4 
main(i4 argc, char* argv[])
{
    char szBuf[MAX_PATH];
	WIN32_FILE_ATTRIBUTE_DATA fInfo;
    MSIHANDLE hDatabase, hView, hView2, hRecord, hRecord2;
    FILE *fp=NULL;
	BOOL  bList=FALSE;
	int	  arg_num=1;

	ZeroMemory(&fInfo, sizeof(fInfo));

	if (strcmp(argv[arg_num], "-p") == 0)
	{
		bList=TRUE;
		arg_num++;
	}
    MsiOpenDatabase(argv[arg_num], MSIDBOPEN_READONLY, &hDatabase);
    if (hDatabase)
    {
	MsiDatabaseOpenView(hDatabase, "SELECT DISTINCT ISBuildSourcePath FROM File", &hView);
	if(hView && !MsiViewExecute(hView, 0))
	{
	    while (MsiViewFetch(hView, &hRecord)!= ERROR_NO_MORE_ITEMS)
	    {
		char szSourcePath[MAX_PATH], szPathVar[256];
		char *pdest1, *pdest2;
		DWORD dwSize, i, j, result;
		char szEnvName[256], szEnvValue[MAX_PATH];

		dwSize=sizeof(szSourcePath);
		MsiRecordGetString(hRecord, 1, szSourcePath, &dwSize);
		
		pdest1=strchr(szSourcePath, '<');
		pdest2=strchr(szSourcePath, '>');
		result=pdest2-pdest1+1;
		for (i=1; i<=result-2; i++)
		    szPathVar[i-1]=szSourcePath[i];
		szPathVar[result-2]='\0';

		for (i=result, j=0; i<strlen(szSourcePath); i++, j++)
		    szSourcePath[j]=szSourcePath[i];
		szSourcePath[j]='\0';

		if (bList)
		{
			char szSourcePathLower[MAX_PATH];
			int i;
			ZeroMemory(&szSourcePathLower, sizeof(szSourcePathLower));
			if (!GetEnvironmentVariable("ING_BUILD", szEnvValue, sizeof(szEnvValue)))
			{
				printf("You must set ING_BUILD\n");
				return 1;
			}
			if (strcmp(argv[arg_num], "IngresIIDotNETDataProvider.ism") != 0)
			     for (i=0; i<strlen(szSourcePath); i++)
				szSourcePathLower[i] = tolower(szSourcePath[i]);
			else
			     strncpy(szSourcePathLower, szSourcePath, strlen(szSourcePath));
			sprintf(szBuf, "%s\\picklist.dat", szEnvValue);
			if (strstr(szSourcePathLower, "version.rel") == NULL)
			{
			     if (!fp) fp = fopen(szBuf, "a+");
				 if (strstr(szSourcePathLower, "ingres\\") > 0)
				     sprintf(szBuf, ".%s", szSourcePathLower+7);
				 else
				     sprintf(szBuf, ".%s", szSourcePathLower);
			     if (fp) fprintf(fp, "%s\n", szBuf);
			}

		}
		else
		{

		sprintf(szBuf, "SELECT Value FROM ISPathVariable WHERE ISPathVariable=\'%s\'", szPathVar);
		MsiDatabaseOpenView(hDatabase, szBuf, &hView2);
		if (hView2 && !MsiViewExecute(hView2, 0) && !MsiViewFetch(hView2, &hRecord2))
		{
		    dwSize=sizeof(szEnvName);
		    MsiRecordGetString(hRecord2, 1, szEnvName, &dwSize);
		    MsiViewClose(hView2);
		}

		if (!GetEnvironmentVariable(szEnvName, szEnvValue, sizeof(szEnvValue)))
		{
		    printf("You must set %s!\n", szEnvName);
		    return 1;
		}
		if (szEnvValue[strlen(szEnvValue)-1]=='\\')
		    szEnvValue[strlen(szEnvValue)-1]='\0';

		sprintf(szBuf, "%s%s", szEnvValue, szSourcePath);

		if (GetFileAttributesEx(szBuf, GetFileExInfoStandard, &fInfo) == 0)
		{
		    if (!fp) fp=fopen("buildrel_report.txt", "a+");
		    if (fp)  fprintf(fp, "%s\n", szBuf);
		}
		if (fInfo.nFileSizeLow == 0 && fInfo.nFileSizeHigh == 0 )
		{
		    if (!fp) fp=fopen("buildrel_report.txt", "a+");
		    if (fp)  fprintf(fp, "%s - 0 Length File\n", szBuf);
		}
		} /* if (strcmp(argv, "-p") == 0) */
	    } /* end of while */
	    MsiViewClose(hView);
	}
	MsiCloseHandle(hDatabase);
    }

    if (fp) fclose(fp);
    return 0;
}

