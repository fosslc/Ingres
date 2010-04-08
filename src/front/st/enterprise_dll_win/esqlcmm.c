/*
**  Copyright (c) 2001, 2006 Ingres Corporation.
*/

/*
**  Name: esqlmm.c
**
**  Description:
**	This file becomes the DLL for the IngresIIEsqlCEsqlCobol Merge Module. It
**	contains custom actions that are required to be executed.
**
**  History:
**	17-Feb-2007 (drivi01)
**	    Created.
**
*/




#include <windows.h>
#include <msi.h>
#include <msiquery.h>
#include <string.h>
#include <stdio.h>


/*
**  Name: ingres_remove_from_includeandlib
**
**  Description:
**	Removes INCLUDE/LIB variable from Environment table in MSI file 
**	if INGRES_INCLUDEINPATH is not set.
**
**  History:
**	17-Feb-2007 (drivi01)
**	    Created.
*/
ingres_remove_from_includeandlib(MSIHANDLE hInstall)
{
	MSIHANDLE msih, hView, hRec;
	char cmd[MAX_PATH+1];

	msih = MsiGetActiveDatabase(hInstall);
	if (!msih)
		return FALSE;
	
	sprintf(cmd, "SELECT * from Environment WHERE Name = '*=-INCLUDE'");

	if (!(MsiDatabaseOpenView(msih, cmd, &hView) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewExecute(hView, 0) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewFetch(hView, &hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewModify(hView, MSIMODIFY_DELETE, hRec) == ERROR_SUCCESS))
		return FALSE;


	if (!(MsiCloseHandle(hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewClose( hView ) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiCloseHandle(hView) == ERROR_SUCCESS))
		return FALSE;
		/* Commit changes to the MSI database */
	if (!(MsiDatabaseCommit(msih)==ERROR_SUCCESS))
	    return FALSE;
	

	sprintf(cmd, "SELECT * from Environment WHERE Name = '*=-LIB'");

	if (!(MsiDatabaseOpenView(msih, cmd, &hView) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewExecute(hView, 0) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewFetch(hView, &hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewModify(hView, MSIMODIFY_DELETE, hRec) == ERROR_SUCCESS))
		return FALSE;


	if (!(MsiCloseHandle(hRec) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiViewClose( hView ) == ERROR_SUCCESS))
		return FALSE;

	if (!(MsiCloseHandle(hView) == ERROR_SUCCESS))
		return FALSE;
		/* Commit changes to the MSI database */
	if (!(MsiDatabaseCommit(msih)==ERROR_SUCCESS))
	    return FALSE;
	


	if (!(MsiCloseHandle(msih)==ERROR_SUCCESS))
	    return FALSE;

	return ERROR_SUCCESS;


}
