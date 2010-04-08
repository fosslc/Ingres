
/*
**  Name: msiversupdate.c
**
**  Description:
**	Update MSI database with correct Ingres version.
**	
**  History:
**	10-Jan-2008 (drivi01)
**	    Created.
*/

#include <windows.h>
#include <winbase.h>
#include <msiquery.h>
#include <compat.h>
#include <gv.h>

GLOBALREF ING_VERSION ii_ver;

i4 
main(i4 argc, char *argv[])
{
     int ret_val=0;
     MSIHANDLE hDatabase;
     char vers[12];
     char view[1024];

	 DWORD dw;
	 dw = GetFileAttributes(argv[1]);
	
     if ((argc < 2) || (GetFileAttributes(argv[1])==-1))
     {
	printf("Usage:\nmsiversupdate <full path to MSI file>\n");
	ret_val=1;
     }
     if (argc == 2 && GetFileAttributes(argv[1]) != -1)
     {
	MsiOpenDatabase(argv[1], MSIDBOPEN_DIRECT, &hDatabase);
	if (!hDatabase)
		ret_val=1;

	sprintf(vers, "%d.%d.%d", ii_ver.major, ii_ver.minor, ii_ver.genlevel);
	

	/*
	** Automatically update version in the MSI module
	**
	*/
	if (hDatabase && strstr(argv[1], ".msm"))
	{
			
		sprintf(view, "UPDATE ModuleSignature SET Version='%s'", vers);	
		if (!ViewExecute(hDatabase, view))
			ret_val=1;
	}
	if (hDatabase && strstr(argv[1], ".msi"))
	{
		sprintf(view, "UPDATE Property SET Value='%s' WHERE Property='ProductVersion'", vers);
		if (!ViewExecute(hDatabase, view))
			ret_val=1;
	    if(MsiDatabaseCommit(hDatabase)!=ERROR_SUCCESS)
			ret_val=1;
	    if (strstr(argv[1], "Ingres.msi") || strstr(argv[1], "IngresII.msi"))
	    {
		sprintf(view, "UPDATE ModuleSignature SET Version='%s' WHERE Version='9.1.0'", vers);
		if (!ViewExecute(hDatabase, view))
			ret_val=1;
            }
	}

     }
     else
     {
     	printf("Wrong number of arguments or MSI file is inaccessible.\n");
	ret_val=1;
     }


    if(MsiDatabaseCommit(hDatabase)!=ERROR_SUCCESS)
		ret_val=1;
    if(MsiCloseHandle(hDatabase)!=ERROR_SUCCESS)
		ret_val=1;

	return ret_val;
}

BOOL 
ViewExecute(MSIHANDLE hDatabase, char *szQuery)
{
	MSIHANDLE hView;
	
	if(MsiDatabaseOpenView(hDatabase, szQuery, &hView)!=ERROR_SUCCESS)
		return FALSE;
	if(MsiViewExecute(hView, 0)!=ERROR_SUCCESS)
		return FALSE;
	if(MsiCloseHandle(hView)!=ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}
