#include <windows.h>
#include <stdio.h>
#include "ii_ver.h"

EP_II_GETVERSION lpfnGetVersionInfo;
HINSTANCE   hApiModule=NULL;

int 
main(int argc, char *argv[])
{

    DWORD dwError;
    int   opCode = 0;
    int   iRetCode = 0;
    II_VERSIONINFO version;
    
    hApiModule=LoadLibrary("iigetver.dll");

    if (!hApiModule)
    {
	dwError = GetLastError();

	switch (dwError)
	{
		case ERROR_INVALID_DLL:
		    printf("iigetver.dll is not a valid 32-bit DLL.\n");
		    break;

		case ERROR_MOD_NOT_FOUND:
		case ERROR_DLL_NOT_FOUND:
		    printf("iigetver.dll could not be found in the path.\n");
		    break;

		default:
		    printf ("LoadLibrary failed with error %d.\n", dwError);
		    break;
	}
	exit (2);
    }

    lpfnGetVersionInfo = (EP_II_GETVERSION)GetProcAddress(hApiModule,"II_GetVersion");

    if (!lpfnGetVersionInfo)
    {
	  dwError = GetLastError();
	  printf ("Unable to locate II_GetVersion in the DLL.  Error: %d\n", dwError);
	  exit (dwError);
    }

    iRetCode = lpfnGetVersionInfo(&version);

    if (hApiModule)
    {
 	  FreeLibrary (hApiModule);
	  hApiModule = NULL;
    }

    printf("\nII_SYSTEM:\t'%s'\nVersion String:\t'%s'\nReturn Code:\t%i\n",
                version.szII_SYSTEM, version.szII_VERSIONSTRING, iRetCode);

    return iRetCode;
}
