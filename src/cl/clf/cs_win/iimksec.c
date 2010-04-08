#define _WIN32_WINNT 0x0501

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <string.h>
#include <compat.h>


/******************************************************************************
**
** Name: iimksec  - Create a SECURITY_ATTRIBUTES structure
**
** Description:
**  	This routine is used to set the security attributes of the 
**	SECURITY_ATTRIBUTES structure that is passed to it. In its
**	initial design we set this equal to a NULL Discretionary 
**	Access Control List (DACL). This allows implicit access to 
**	everyone.
**	It is called by routines which create objects (mutexes, processes, etc)
**	that need to be accessed by more than one user.
**
** Inputs:
**      sa				A pointer to the SECURITY_ATTRIBUTE
**					structure.	
** Outputs:
**      none
**  Returns:
**      OK
**      FAIL
**  Exceptions:
**      none
**
** Side Effects:
**      The sa structure is filled appropriately.
**
**
**	History:
**
**	26-may-94 (marc_b) Created.
**
**      11-nov-95 (lawst01)
**        Added sizeof operator for structure pointer to SECURITY_ATTRIBUTES
**        object.
**	21-mar-1996 (canor01)
**	    Make the default PSECURITY_DESCRIPTOR static and reuse
**	    it for all callers.
**	  
*/
STATUS
iimksec(SECURITY_ATTRIBUTES *sa)
{
    static PSECURITY_DESCRIPTOR pSD = NULL;

    if (pSD == NULL)
    {
	pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
    		SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (pSD == NULL)
	    goto cleanup;
	
	if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
	{
   	    goto cleanup;
	}

	/* Initialize new ACL */
	if (!SetSecurityDescriptorDacl(pSD, TRUE, (PACL) NULL, FALSE))
	{
   	    goto cleanup;
	}
    }

    sa->nLength = sizeof(SECURITY_ATTRIBUTES);
    sa->lpSecurityDescriptor = pSD;
    sa->bInheritHandle = TRUE;
    return(OK);

cleanup:
    if (pSD != NULL)
	LocalFree((HLOCAL) pSD);
    return (FAIL);
}

/*{
** Name: iimksecdacl
**
** Description:
**      This routine is used to initialize a SECURITY_ATTRIBUTES structure
**      with a security descriptor that allows full control to the
**      administrators group.  This routine should be used by creators
**      of shared cross process objects that require the same privilege but
**      not the everyone access granted by a NULL DACL.
**
**      Windows objects are currently created in one of two modes in Ingres
**          NULL SECURITY_ATTRIBUTES.
**              The object inherits privileges and permissions from the
**              parent.
**              When owned by the LocalSystem account the object inherits a
**              restricted set of privileges and permissions which does not
**              allow access by a user account.
**          NULL    DACL in the security descriptor.
**              The object grants fill privileges and permissions to everyone.
**
**      With this function an object can be created in a third mode that
**      allows full control by the administrators group.
**
** Inputs:
**      sa      pointer to SECURITY_ATTRIBUTES structure.
**
** Outputs:
**      .nLength
**      .lpSecurityDescriptor
**      .bInheritHandle
**
** Returns:
**      OK      Structure initialized successfully.
**      FAIL
**
** History:
**      28-Sep-2005 (fanra01)
**          Bug 115298
**          Created.
}*/
STATUS
iimksecdacl(SECURITY_ATTRIBUTES *sa)
{
    static PSECURITY_DESCRIPTOR pSdDacl = NULL;
    DWORD   sidSize = SECURITY_MAX_SID_SIZE;
    DWORD   aclSize;
    PSID    pSid = NULL;
    PACL    pDacl = NULL;
    DWORD   accessMask = 0;
        
    if (pSdDacl == NULL)
    {
        /*
        ** Allocate memory for the security identifier
        */
        if ((pSid = (PSID)LocalAlloc( LPTR, sidSize )) == NULL)
            goto cleanup;

        /*
        ** Create the SID that we're going to bestow all power to.
        ** In this instance the Administrators group.
        */
        if (CreateWellKnownSid( WinBuiltinAdministratorsSid, NULL, pSid,
            &sidSize) == 0)
        {
            goto cleanup;
        }

        /*
        ** Calculate size an ACL using some funky formula provided by MS.
        ** To calculate the initial size of an ACL, add the following together
        **      Size of the ACL structure.
        **      Size of each ACE structure that the ACL is to contain minus
        **          the SidStart member (DWORD) of the ACE.
        **      Length of the SID that each ACE is to contain.
        */
        aclSize = sizeof(ACL) + 
            ((sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)));
        aclSize += GetLengthSid( pSid );

        /*
        ** Allocate memory for the DACL
        */
        if ((pDacl = (PACL)LocalAlloc( LPTR, aclSize )) == NULL)
            goto cleanup;
        
        if (!InitializeAcl( pDacl, aclSize, ACL_REVISION ))
        {
            goto cleanup;
        }
        
        accessMask = FILE_ALL_ACCESS;
        if (!AddAccessAllowedAce( pDacl, ACL_REVISION, accessMask, pSid ))
        {
            goto cleanup;
        }

        pSdDacl = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR,
    		SECURITY_DESCRIPTOR_MIN_LENGTH);
        if (pSdDacl == NULL)
            goto cleanup;
	
        if (!InitializeSecurityDescriptor(pSdDacl, SECURITY_DESCRIPTOR_REVISION))
        {
            goto cleanup;
        }

        /* Initialize new ACL */
        if (!SetSecurityDescriptorDacl(pSdDacl, TRUE, (PACL) pDacl, FALSE))
        {
            goto cleanup;
        }
    }

    sa->nLength = sizeof(SECURITY_ATTRIBUTES);
    sa->lpSecurityDescriptor = pSdDacl;
    sa->bInheritHandle = TRUE;
    return(OK);

cleanup:
    if (pSid != NULL)
    LocalFree((HLOCAL) pSid);
    if (pDacl != NULL)
    LocalFree((HLOCAL) pDacl);
    if (pSdDacl != NULL)
	LocalFree((HLOCAL) pSdDacl);
    return (FAIL);
}
