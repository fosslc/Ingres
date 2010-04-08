/******************************************************************************
**
** Copyright (c) 1994, 1997 Ingres Corporation
**
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <compat.h>
#include <nm.h>
#include <ep.h>
#include "setperm.h"

BOOL SetPermsOnFile(LPSTR filename, PSECURITY_DESCRIPTOR pSD);
VOID ErrorExit();

/*
**	File: Setperm.c
**
**	Description:
**
**	This module will set the permissions for an INGRES installation
**	that is installed on an NTFS file system. This program will fail
**	if you try to run it on any other file system. We have made a rough
**	attempt to mimic the permissions as seen on INGRES under Unix.
**
**	History:
**
**	19-apr-94 (marc_b) Created.
**
**	10-may-94 (marc_b)
**		servproc.exe needs to have RX priv's for Everyone. 
**		Otherwise we would fail when attempting to start the
**		service.
**	23-may-94 (marc_b)
**		We need to allow generic read access to the dbtmplt
**		directory so that any use can read the templates and
**		do createdb's. Also remove some redundant files in the
**		dictfile directory.
**	27-may-94 (johnr)
**		We need to consider the perms on the five directories
**		that may have been set to locations other than in II_SYSTEM.
**		This includes II_LOG_FILE, II_CHECKPOINT, II_DUMP and
**		II_DATABASE.
**	31-may-94 (marc_b)
**		We don't want to set perms for dbtmplt (or any files in the
**		subdirectory) or log. We need these to be readable by
**		everyone.
**	26-feb-95 (emmag)
**		Add CLNTPROC.EXE to the list of executables and the
**		tools help files to the list of help files.
**	03-mar-95 (emmag)
**		RCP.PAR is generated during startup so it shouldn't be
**		included in the list.
**	27-jul-95 (fauma01)
**		Corrected discrepencies for NT and added SERVPROC.EXE
**		and OPINGSVC.EXE to the file list.	    
**	09-aug-95 (emmag)
**		Since CA-Install has already set the default permissions for
**		us, the majority of this code was redundant.  All we
**		really need to change are the permissions on files
**		and directories that only the INGRES user can access -
**		added OPINGSVC.EXE to this list.
**      08-dec-95 (tutto01)
**              Allowing access to the ingres account did not take into
**              account the notion of a domain user called ingres.  This
**              caused a problem on NT stations that had locally defined
**              an ingres account.  The access granted by these routines
**              would only be for the local ingres account, causing the
**              domain ingres account to have no more rights than the average
**              user!  Instead of granting access to ingres, we now give it
**              to members of the Administrators group.
**      08-apr-1997 (hanch04)
**          Added ming hints
**      14-may-97 (mcgem01)
**          Clean up compiler warnings.
**	15-nov-97 (mcgem01) 87104
**	    Level of file permissions granularity provided by the CA-Instller
**	    was no adequate.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**	25-Jul-2007 (drivi01)
**	    Added routines to force this program to exit if user doesn't
**	    have elevated privileges to run it.
*/

/*
**      mkming hints

NEEDLIBS =      COMPATLIB 


PROGRAM =       setperm

*/

VOID 
main()
{
	PSECURITY_DESCRIPTOR pSD, pSD1;
	PACL pACL, pACL1;
	PSID pSIDAdmins, pSIDEveryone;
	DWORD cbACL = 1024;
	DWORD cbSID = 1024;
	LPSTR lpszAccountAdmins = "Administrators";
	LPSTR lpszAccountEveryone = "everyone";
	LPSTR lpszDomain, lpszDomain1;
	PSID_NAME_USE psnuType, psnuType1;
	DWORD cchDomainName = 80;
	TCHAR tchBuf[_MAX_PATH];
	char  *ptr;
	char  fname[256];
	int   i;

	/* If this is Vista, check that user running has permissions to 
	** execute this application.
	*/
	ELEVATION_REQUIRED();

	/* 
	** Get II_SYSTEM 
	*/

	if ( (GetEnvironmentVariable("II_SYSTEM", tchBuf, _MAX_PATH)) == 0) 
		printf("II_SYSTEM is not defined!\n");

	/* Initialize new security descriptor */

	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, 
			SECURITY_DESCRIPTOR_MIN_LENGTH);
	pSD1 = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
			SECURITY_DESCRIPTOR_MIN_LENGTH);

	if ( (pSD == NULL) || (pSD1 == NULL) )
	{
		ErrorExit();
	}

	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
	{
		ErrorExit();
		goto cleanup;
	}
	if (!InitializeSecurityDescriptor(pSD1, SECURITY_DESCRIPTOR_REVISION))
	{
		ErrorExit();
		goto cleanup;
	}

	/* Initialize new ACL */

	pACL = (PACL) LocalAlloc(LPTR, cbACL);
	if (pACL == NULL) 
	{
		ErrorExit();
		goto cleanup;
	}

	pACL1 = (PACL) LocalAlloc(LPTR, cbACL);
	if (pACL1 == NULL) 
	{
		ErrorExit();
		goto cleanup;
	}
  
  
	if (!InitializeAcl(pACL, cbACL, ACL_REVISION2))
	{
		ErrorExit();
	}

	if (!InitializeAcl(pACL1, cbACL, ACL_REVISION2))
	{
		ErrorExit();
	}


	pSIDAdmins = (PSID) LocalAlloc(LPTR, cbSID);
	pSIDEveryone = (PSID) LocalAlloc(LPTR, cbSID);
	psnuType = (PSID_NAME_USE) LocalAlloc(LPTR, 1024);
	psnuType1 = (PSID_NAME_USE) LocalAlloc(LPTR, 1024);
	lpszDomain = (LPSTR) LocalAlloc(LPTR, cchDomainName);
	lpszDomain1 = (LPSTR) LocalAlloc(LPTR, cchDomainName);

	if ( (pSIDAdmins == NULL) || (psnuType == NULL) || 
		(lpszDomain == NULL) || (pSIDEveryone == NULL) || 
		(lpszDomain1 == NULL) || (psnuType1 == NULL) )
	{
		ErrorExit();
		goto cleanup;
	}

	if (!LookupAccountName((LPSTR) NULL, /* get SID for Admins group */
		lpszAccountAdmins,
		pSIDAdmins,
		&cbSID,
		lpszDomain,
		&cchDomainName,
		psnuType))
	{
		ErrorExit();
		goto cleanup;
	}

	if (!LookupAccountName((LPSTR) NULL, /* get SID for group "everyone" */
		lpszAccountEveryone,
		pSIDEveryone,
		&cbSID,
		lpszDomain1,
		&cchDomainName,
		psnuType1))
	{
		ErrorExit();
		goto cleanup;
	}

	/* 
	** Set access rights for file 
	*/

	if (!AddAccessAllowedAce(pACL,
		ACL_REVISION2,
		GENERIC_ALL,
		pSIDAdmins))
	{
		ErrorExit();
		goto cleanup;
	}


	/* Add a new ACL to the security descriptor */

	if (!SetSecurityDescriptorDacl(pSD,
		TRUE,	/* fDaclPresent flag */
		pACL,
		FALSE))	/* not a default disc. ACL */
	{
		ErrorExit();
		goto cleanup;
	}

	/* Set perms on directories where only an Administrator
	** should be allowed access.
	*/

	i = 0;
	while (strcmp(dirlist[i], "THEEND") != 0)
	{
		ptr = NULL;
		if ( envnamelist[i] != NULL )
		{
			NMgtAt( envnamelist[i], &ptr);
		}
		if (!(ptr && *ptr))
		{
			sprintf(fname,"%s\\ingres%s", tchBuf, dirlist[i]);
			if (!SetPermsOnFile(fname, pSD))
				printf("Failed to set perms on %s\n", fname); 
			i++;
		}
		else
		{
			sprintf(fname,"%s\\ingres%s", ptr, dirlist[i]);
			if (!SetPermsOnFile(fname, pSD))
				printf("Failed to set perms on %s\n", fname); 
			i++;
		}
	}

	/* 
	** Set permissions on special files where only Administrators
	** should be allowed access.
	*/

	i = 0;
	while (strcmp(special_list[i], "THEEND") != 0)
	{
		sprintf(fname,"%s\\ingres%s", tchBuf, special_list[i]);
		if (!SetPermsOnFile(fname, pSD))
			printf("Failed to set permission on %s\n", fname); 
		i++;
	}

	/* 
	** Next we want to grant the group "everyone" read and execute 
	** access to the appropriate files 
	*/

	if (!AddAccessAllowedAce(pACL1, ACL_REVISION2, GENERIC_READ, 
		pSIDEveryone))
	{
	   ErrorExit();
	   goto cleanup;
	}

	if (!AddAccessAllowedAce(pACL1, ACL_REVISION2, GENERIC_EXECUTE, 
		pSIDEveryone))
	{
		ErrorExit();
		goto cleanup;
	}

	/* 
	** Next we want to grant the Administrators group full access 
	** to the files.  
	*/

	if (!AddAccessAllowedAce(pACL1, ACL_REVISION2, GENERIC_ALL, pSIDAdmins))
	{
		ErrorExit();
		goto cleanup;
	}


	if (!SetSecurityDescriptorDacl(pSD1, TRUE, pACL1, FALSE))	
	{
		ErrorExit();
		goto cleanup;
	}

	i = 0;
	while (strcmp(filelist[i], "THEEND") != 0)
	{
		sprintf(fname,"%s\\ingres%s", tchBuf, filelist[i]);
		if (!SetPermsOnFile(fname, pSD1))
		    printf("Failed to set permission on %s\n", fname);
		i++;
	}

cleanup:
	FreeSid(pSIDAdmins);
	if (pSD != NULL)
		LocalFree((HLOCAL) pSD);
	if (pSD1 != NULL)
		LocalFree((HLOCAL) pSD1);
	if (pACL != NULL)
		LocalFree((HLOCAL) pACL);
	if (pACL1 != NULL)
		LocalFree((HLOCAL) pACL1);
	if (psnuType != NULL)
		LocalFree((HLOCAL) psnuType);
	if (lpszDomain != NULL)
		LocalFree((HLOCAL) lpszDomain);
	if (psnuType1 != NULL)
		LocalFree((HLOCAL) psnuType1);
	if (lpszDomain1 != NULL)
		LocalFree((HLOCAL) lpszDomain1);
}

VOID 
ErrorExit()
{
	/* For the DLL we will ignore any errors */
	return;
}

BOOL 
SetPermsOnFile(LPSTR filename, PSECURITY_DESCRIPTOR pSD)
{
    int status = 0;
    printf("Setting permissions on %s\n", filename); 

    /* 
    ** Apply the new security descriptor to the file. 
    */
    if (!SetFileSecurity(filename, DACL_SECURITY_INFORMATION, pSD))
    {
	status = GetLastError();
	return (FALSE);
    }
    else
	return (TRUE);
}
