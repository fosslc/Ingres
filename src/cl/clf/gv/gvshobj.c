/*
**  Copyright (c) 2006 Ingres Corporation
*/

# include <compat.h>
# include <st.h>
# include <gvos.h>

# ifdef NT_GENERIC
static const char* SharedPrefix[] = { "\0", "Global\\" };
OSVERSIONINFO	OSVers = { 0 };
# endif

/*
**  Name: GVSHOBJ.C	- Shared Object Prefix Function
**
**  Description: Retrieves major version of the OS and 
**				 determines if Global prefix is required
**			     to create shared objects for this
**				 platform.
**
**
**  History:
**	12-Dec-2006 (drivi01)
**	    Created.
**	18-Dec-2006 (drivi01)
**	    SharedPrefix and OSVers should be defined for windows only.
*/

VOID
GVshobj(char **shPrefix) 
{
# ifdef NT_GENERIC
    static int GlobalObject = -1;
	*shPrefix = "\0";
    if (GlobalObject < 0)
    {
	OSVers.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if (!GetVersionEx((OSVERSIONINFO *)&OSVers))
		return;
	GlobalObject = (OSVers.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		OSVers.dwMajorVersion >= 5) ? 1 : 0;
    }
    if (shPrefix != NULL && GlobalObject >= 0)
    {
	    *shPrefix = SharedPrefix[GlobalObject];
    }

	return;
# else /* NT_GENERIC */
	*shPrefix = "\0";
	return;
# endif /* NT_GENERIC */
}

/*
**  Name: GVvista	- Test if we're on Vista
**
**  Description: Retrieves major version of the OS to determine 
**	         if the machine is running Windows Vista.
**	   
**
**
**  History:
**	25-Jul-2007 (drivi01)
**	    Created.
**	18-Sep-2007 (drivi01)
**	    Replace BOOL with int.
*/

int
GVvista()
{
# ifdef NT_GENERIC
	static int isVista = -1;
	if (isVista < 0)
	{
		OSVers.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		if (GetVersionEx((OSVERSIONINFO *)&OSVers) &&
			OSVers.dwPlatformId == VER_PLATFORM_WIN32_NT && 
			OSVers.dwMajorVersion == 6)
			isVista = 1;
		else
			isVista = 0;
	}
	return isVista;
# else
	return 0;
# endif /* NT_GENERIC */
}
