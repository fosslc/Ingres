/*
**  Copyright (c) 2001, 2004 Ingres Corporation
*/

# include <compat.h>
# include <st.h>
#include <gvos.h>

/*
**  Name: GVOSVER.C	- OS Version functions
**
**  Description:
**	This file contains functions which are used to grab the OS
**	version from various platforms.
**
**  History:
**	28-jun-2001 (somsa01)
**	    Created.
**	26-Jul-2001 (hanje04)
**	    Changed BOOL to bool. Unix doesn't like BOOL
**	11-apr-2002 (abbjo03)
**	    Add GVverifyOSversion.
**	26-apr-2002 (abbjo03)
**	    Fix GVverifyOSversion to not use VerifyVersionInfo.
**	14-nov-2003 (somsa01)
**	    Initialize dwBufLen.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
*/

bool
GVosvers(char *OSVersionString)
{
# ifdef NT_GENERIC

    static char		VersString[64] = "";
    char		TempVers[64];
    OSVERSIONINFO	OSVers;
    HKEY		hKey;
    char		szProductType[80];
    DWORD		dwBufLen = 80;

    /*
    ** For Microsoft platforms, we will return the OS version as a string.
    */
    if (VersString[0] == '\0')
    {
	ZeroMemory(&OSVers, sizeof(OSVERSIONINFO));
	OSVers.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx((OSVERSIONINFO *)&OSVers))
	    return FALSE;

	switch (OSVers.dwPlatformId)
	{
	    case VER_PLATFORM_WIN32_NT:
		/* Test for the product. */
		if (OSVers.dwMajorVersion <= 4)
		    STcopy("Microsoft Windows NT ", TempVers);

		if (OSVers.dwMajorVersion == 5)
		    STcopy("Microsoft Windows 2000 ", TempVers);

		/* Test for workstation versus server. */
		RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			"SYSTEM\\CurrentControlSet\\Control\\ProductOptions",
			0, KEY_QUERY_VALUE, &hKey);
		RegQueryValueEx(hKey, "ProductType", NULL, NULL,
				(LPBYTE)szProductType, &dwBufLen);
		RegCloseKey(hKey);

		if (lstrcmpi("WINNT", szProductType) == 0)
		    STcat(TempVers, "Workstation ");

		if (lstrcmpi("SERVERNT", szProductType) == 0)
		    STcat(TempVers, "Server ");

		/*
		** Display version, service pack (if any), and build
		** number.
		*/
		STprintf(VersString, "%sversion %d.%d %s (Build %d)\n",
			 TempVers, OSVers.dwMajorVersion,
			 OSVers.dwMinorVersion, OSVers.szCSDVersion,
			 OSVers.dwBuildNumber & 0xFFFF );

		break;

	    case VER_PLATFORM_WIN32_WINDOWS:
		if (OSVers.dwMajorVersion > 4 || 
		    (OSVers.dwMajorVersion == 4 && OSVers.dwMinorVersion > 0))
		{
		    STcopy("Microsoft Windows 98", VersString);
		} 
		else
		    STcopy("Microsoft Windows 95", VersString);

		break;

	    case VER_PLATFORM_WIN32s:
		printf ("Microsoft Win32s ");
		break;
	}
    }

    STcopy(VersString, OSVersionString);
    return TRUE; 

# else  /* NT_GENERIC */

    /* No platform-specific function; return FALSE */
    return FALSE;

# endif  /* NT_GENERIC */
}

/*{
** Name: GVverifyOSversion - Verify Operating System Version
**
** Description:
**	Verifies that the running operating system has a version number equal 
**	to or greater than the one specified in the versinfo structure.
**
** Inputs:
**	versinfo	minimum required OS version
**
** Outputs:
**	none
**
** Returns:
**	OK		the running system meets the minimum requirement
**	GV_BAD_PARAM	size of GV_OSVERSIONINFO is incorrect or bad platform
**	FAIL		system does not meet requirements
**
** Side Effects:
**	none
**
** History:
**	11-apr-2002 (abbjo03)
**	    Written.
**	26-apr-2002 (abbjo03)
**	    Do not use VerifyVersionInfo since it's only available starting with
**	    Windows 2000.
*/
STATUS GVverifyOSversion(
GV_OSVERSIONINFO	*versinfo)
{
    if (!versinfo || versinfo->ver_size != sizeof(GV_OSVERSIONINFO))
	return FAIL;

#ifdef NT_GENERIC
  {
    OSVERSIONINFOEX osver;

    ZeroMemory(&osver, sizeof(osver));
    osver.dwOSVersionInfoSize = sizeof(osver);
    if (!GetVersionEx((OSVERSIONINFO *)&osver))
    {
	osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx((OSVERSIONINFO *)&osver))
	    return FAIL;
    }

    if (osver.dwMajorVersion > versinfo->ver_major)
	return OK;
    if (osver.dwMinorVersion > versinfo->ver_minor)
	return OK;
    if (osver.dwMajorVersion >= 5 && versinfo->ver_sub > 0)
    {
	if (osver.wServicePackMajor < (WORD)versinfo->ver_sub)
	    return FAIL;
    }

    return OK;
  }
#else
    return OK;
#endif
}
