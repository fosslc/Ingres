#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <imagehlp.h>

/*
**  Name: space.c
**
**  Description:
**	This execitable takes a filelist and makes fsizes.roc (which is
**	included by installer.c), a plist for the patch, and also updates
**	the files in II_PATCH_DIR so that they are in 8.3 format (if they
**	are not already set up that way).
**
**  History:
**	28-jan-1999 (somsa01)
**	    Remade this file from scratch as per the new patch procedures.
**	09-Mar-1999 (andbr08)
**	    Disabled the checksum for windows 95 as the function
**	    MapFileAndChecksumA() does not work correctly under windows 95
**	    on all files.
**	24-Mar-1999 (andbr08)
**	    Added checksum used in UNIX, defined in ip_file_info_comp()
**	    in front!st!install ipfile.sc
**	27-May-1999 (andbr08)
**	    Fixed problem with adding a '\' for files in the VERS file.  Even
**	    though the VERS file is suppose to be formatted without a backslash
**	    between the path and filename, one was not added.
**	09-jun-2000 (kitch01)
**	    Add adding of PatchVersion to the fsizes.roc file for the installer
**	    to pick up. This is used to check the version of Ingres the patch
**	    is being installed against.
**	15-May-2002 (inifa01)
**	    Modified to accomodate longer 2.6 path lengths		
**	27-jun-2002 (somsa01)
**	    Changed II_RELEASE_DIR to II_PATCH_DIR.
*/

#define MAXSTRING 256

BOOL	FileExists(CHAR *);
LONG	GetCurrentFileSize(CHAR *);
VOID	GetTimeOfDay(CHAR *);

DWORD GetCurrentFileChecksum(CHAR*);

struct WeekDay
{
    CHAR	Day[4];
};

struct WeekDay DayOfWeek[] = {
	"Sun",
	"Mon",
	"Tue",
	"Wed",
	"Thu",
	"Fri",
	"Sat"
};

struct MonthNum
{
    CHAR	Month[4];
};

struct MonthNum Month[] = {
	"Jan",
	"Feb",
	"Mar",
	"Apr",
	"May",
	"Jun",
	"Jul",
	"Aug",
	"Sep",
	"Oct",
	"Nov",
	"Dec"
};

main()
{
    FILE	*fptr, *FileListPtr, *PlistPtr;
    CHAR	mline[MAXSTRING], CdimageFileName[1024], dirname[MAXSTRING];
    CHAR	mline2[512], filename[MAXSTRING], newfile[2], *CharPtr;
    CHAR	CdimageName[MAXSTRING], BeginName[32], FilePath[1024];
    INT		i, j;
    LONG	numfiles = 0;
    BOOL	NotFirstTime = FALSE;
    CHAR	Version[64], Patch[8], CurrentTime[25], TmpBfr[512], *CharPtr2;
    CHAR	ReleaseDir[1024];
    DWORD	HeaderSum, CheckSum;
	CHAR	verrel[64];

    if (!getenv("IngVersion"))
    {
	printf("Error! IngVersion not defined!\n");
	exit(1);
    }

    if (!getenv("PATCHNO"))
    {
	printf("Error! PATCHNO not defined!\n");
	exit(1);
    }

    if (!getenv("II_PATCH_DIR"))
    {
	printf("Error! II_PATCH_DIR not defined!\n");
	exit(1);
    }

    strcpy(Version, getenv("IngVersion"));
    j = 0;
    for (i=0; i<strlen(Version); i++)
    {
	if (Version[i] == '"')
	    continue;
	mline2[j++] = Version[i];
    }
    mline2[j] = '\0';
    strcpy(Version, mline2);

	strcpy(verrel, Version);
    j = 0;
	/* miss out the first 3 chars e.g. 'OI ' */
    for (i=3; i<strlen(verrel); i++)
    {
	if (verrel[i] == '.' || verrel[i] == '(' || verrel[i] == ')' || verrel[i] == '/'
		|| verrel[i] == ' ')
	    continue;
	mline2[j++] = verrel[i];
    }
    mline2[j] = '\0';
    strcpy(verrel, mline2);

    strcpy(Patch, getenv("PATCHNO"));
    j = 0;
    for (i=0; i<strlen(Patch); i++)
    {
	if (Patch[i] == '"')
	    continue;
	mline2[j++] = Patch[i];
    }
    mline2[j] = '\0';
    strcpy(Patch, mline2);

    strcpy(ReleaseDir, getenv("II_PATCH_DIR"));
    j = 0;
    for (i=0; i<strlen(ReleaseDir); i++)
    {
	if (ReleaseDir[i] == '"')
	    continue;
	mline2[j++] = ReleaseDir[i];
    }
    mline2[j] = '\0';
    strcpy(ReleaseDir, mline2);

    if ((fptr = fopen("fsizes.roc", "w")) == NULL)
    {
	printf("Error! Unable to open fsizes.roc for write!\n");
	exit(1);
    }

    if ((FileListPtr = fopen("filelist", "r")) == NULL)
    {
	printf("Error! Unable to open filelist for read!\n");
	fclose(fptr);
	exit(1);
    }

    if ((PlistPtr = fopen("plist", "w")) == NULL)
    {
	printf("Error! Unable to open filelist for read!\n");
	fclose(fptr);
	fclose(FileListPtr);
	exit(1);
    }

	/*
    ** Define some essentials first.
    */
    fprintf(fptr, "#define IngVersion \"%s\"\n", Version);
    fprintf(fptr, "#define PATCHNO \"%s\"\n", Patch);
    fprintf(fptr, "#define PatchVersion \"%s\"\n", verrel);
    if (getenv("INGRESII"))
	fprintf(fptr, "#define IngresII\n");

    /*
    ** Set up the plist headers.
    */
    fprintf(PlistPtr, "## Version:	%s\n", Version);
    fprintf(PlistPtr, "## Patch:	%s\n", Patch);
    fprintf(PlistPtr, "## Task:	build_plist\n");
    GetTimeOfDay(CurrentTime);
    fprintf(PlistPtr, "## Performed:	%s\n", CurrentTime);
    fprintf(PlistPtr, "##\n##\n## Contents:\n");
    fprintf(PlistPtr, "##	Packing list for patch ID recorded above.\n");
    fprintf(PlistPtr, "##\n##\n");
    fprintf(PlistPtr, "##                  file                        size     checksum\n##\n");

    fprintf(fptr, "\nstruct FILEINF filesizes[] = {\n");

    /*
    ** Now, set up the filesizes array. As we do this, if we find a file
    ** which is not in 8.3 (cdimage) format, rename it as such. Also,
    ** if we are dealing with files which are located in dictfiles or
    ** collation, the directories on the cdimage will be dictfile
    ** or collatio.
    */
    while (fgets(mline, MAXSTRING -1, FileListPtr) != NULL)
    {
	numfiles++;

	if (NotFirstTime)
	    fprintf(fptr, ",\n");

	if (!NotFirstTime)
	    NotFirstTime = TRUE;

	mline[strlen(mline)-1] = '\0';
	CharPtr = mline;
	strcpy(dirname, mline);
	CharPtr2 = dirname;
	while (*CharPtr2 != ' ')
	{
	    CharPtr++;
	    CharPtr2++;
	}
	*CharPtr2 = '\0';
	strcpy(TmpBfr, dirname);
	if (dirname[strlen(dirname)-1] == '\\')
	    dirname[strlen(dirname)-1] = '\0';
	else 
	{
		TmpBfr[strlen(TmpBfr)+1] = '\0';
		TmpBfr[strlen(TmpBfr)] = '\\';
	}

	/*
	** Convert single slashes to double slashes.
	*/
	j = 0;
	for (i=0; i<strlen(dirname); i++)
	{
	    mline2[j] = dirname[i];
	    if (dirname[i] == '\\')
		mline2[++j] = '\\';

	    j++;
	}
	mline2[j] = '\0';
	strcpy(dirname, mline2);

	CharPtr++;
	strcpy(filename, CharPtr);
	CharPtr2 = filename;
	while (*CharPtr2 != ' ')
	{
	    CharPtr++;
	    CharPtr2++;
	}
	*CharPtr2 = '\0';
	strcat(TmpBfr, filename);

	CharPtr++;
	strcpy(newfile, CharPtr);
	newfile[1] = '\0';

	sprintf(CdimageFileName, "%s%s\\%s", ReleaseDir, dirname, filename);

	if (!FileExists(CdimageFileName))
	{
	    printf("Error! %s does not exist!\n", CdimageFileName);
	    fclose(PlistPtr);
	    fclose(FileListPtr);
	    fclose(fptr);
	}

	strcpy(CdimageName, filename);
/*	strcpy(mline2, filename);
	if ((CharPtr2 = strchr(mline2, '.')) != NULL)
	    *CharPtr2 = '\0';

	if (strlen(mline2) > 8)
	{
	    j = 1;
	    strncpy(BeginName, filename, 7);
	    BeginName[7] = '\0';
	    CharPtr2 = strchr(filename, '.');
	    if (CharPtr2)
		sprintf(CdimageName, "%s%i%s", BeginName, j, CharPtr2);
	    else
		sprintf(CdimageName, "%s%i", BeginName, j);
	    sprintf(mline2, "%s%s\\%s", ReleaseDir, dirname, CdimageName);
	    while (FileExists(mline2))
	    {
		j++;
	   	if (CharPtr2)
		    sprintf(CdimageName, "%s%i%s", BeginName, j, CharPtr2);
		else
		    sprintf(CdimageName, "%s%i", BeginName, j);
		sprintf(mline2, "%s%s\\%s", ReleaseDir, dirname, CdimageName);
	    }

	    CopyFile(CdimageFileName, mline2, FALSE);
	    DeleteFile(CdimageFileName);
	    strcpy(CdimageFileName, mline2);
	} */
	
	fprintf(fptr, "\"  \",	\"%s\",	\"%s\",	\"%s\",	%d,	%i,	0",
		filename, CdimageName, dirname,
		GetCurrentFileSize(CdimageFileName), atoi(newfile));

	CheckSum = GetCurrentFileChecksum(CdimageFileName);
	fprintf(PlistPtr, "%-58s  %9d  %10u\n", TmpBfr,
		GetCurrentFileSize(CdimageFileName), CheckSum);
    }

    fprintf(fptr, "\n};\n\n#define NUMFILES %d\n", numfiles);

    fprintf(PlistPtr, "##\n##\n## File format version:	1\n");
    fprintf(PlistPtr, "##\n##\n## State:	completed\n");

    fclose(PlistPtr);
    fclose(FileListPtr);
    fclose(fptr);
}

/*
**  Name: FileExists
**
**  Description:
**	Determines whether the given file/path exists... well, sort of.
*/
BOOL
FileExists(CHAR *filename)
{
    if (GetFileAttributes(filename) != -1)
	return TRUE;

    return FALSE;
}

/*
**  Name: GetCurrentFileSize
**
**  Description:
**	Retrieves the current size of the given file name.
**
**  History:
**	24-jan-1999 (somsa01)
**	    Created.
*/
LONG
GetCurrentFileSize (CHAR *fname)
{
    HANDLE	hFile;
    DWORD	size;

    hFile = CreateFile( fname, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, 0 ,0 );

    size = GetFileSize(hFile, NULL);
    CloseHandle(hFile);

    return (size);
}

/*
**  Name: GetTimeOfDay
**
**  Description:
**      This procedure gets the current time of day and returns it as an
**      ASCII string.
**
**  History:
**      24-jan-1999 (somsa01)
**          Created.
*/
VOID
GetTimeOfDay(CHAR *TimeString)
{
    SYSTEMTIME  LocalTime;

    GetLocalTime(&LocalTime);

    sprintf(TimeString, "%s %s %02d %02d:%02d:%02d %d",
	    DayOfWeek[LocalTime.wDayOfWeek].Day,
	    Month[LocalTime.wMonth - 1].Month,
	    LocalTime.wDay, LocalTime.wHour, LocalTime.wMinute,
	    LocalTime.wSecond, LocalTime.wYear);
}

/*
**  Name: GetCurrentFileChecksum()
**
**  Description:
**	This method returns the checksum of a file passed into the method as
**	a character pointer.  The checksum algorithm is the one used in the UNIX
**	ingbuild executable and is defined in ip_file_info_comp() in 
**	front!st!install ipfile.sc
**
**  History:
**	24-Mar-1999 (andbr08)
**	    Created.
*/
DWORD GetCurrentFileChecksum(CHAR* fname)
{
	HANDLE		hFile;
    CHAR		dataRead[1024];
    CHAR		*dataptr;
	DWORD		CheckSum;
    int			numBytesRead;
    int			readDataFlag;
	DWORD		ByteCount;

	hFile = CreateFile( fname, GENERIC_READ, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, 0 ,0 );
	if (hFile == NULL) {
		return 0;
	}

	ByteCount = 0;
	CheckSum = 0;
    while (1) {
		readDataFlag = ReadFile(hFile, &dataRead, 1024, &numBytesRead, NULL); 
		if (numBytesRead == 0) {
			break;
		}
		else if (readDataFlag == 0) {
			break;
		}
		else {
			dataptr = dataRead;
			while (numBytesRead > 0) {
				--numBytesRead;
				ByteCount++;
				CheckSum ^= (*dataptr) << ( ByteCount & 3 );
				++dataptr;
			}
		}
    }
	
    CloseHandle(hFile);

    return CheckSum;
}
