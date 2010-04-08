/*
**  Copyright (c) 2006 Ingres Corporation
*/

# include	<compat.h>
# include 	<shlobj.h>
# include	<pc.h>


/*
**  Name: PCadjust_SeDebugPrivilege - Adjusts SeDebugPrivilege for the process.
**
**  Description:
**	This function can be called to turn on or turn off SeDebugPrivilege for
**	current process at any point.
**
**  Inputs:
**	bEnablePrivilege	TRUE to enable SeDebugPrivilege
**				FALSE to disable SeDebugPrivilege
**
**  Outputs:
**	err_code	    Returns GetLastError() error code if OpenProcessToken
**			    failed.
**
**  Returns:
**	FAIL		    Failed to adjust process token and provide 
**			    SeDebugPrivilege.
**	OK		    Successfully granted SeDebugPrivilege.
**
**  History:
**      06-Dec-2006 (drivi01)
**	   Added function in efforts to port to Vista.
**	   Vista has a strickter security and doesn't
**	   allow administrators to query SYSTEM processes
**	   unless they have SeDebugPrivilege.
*/

STATUS
PCadjust_SeDebugPrivilege(
	BOOL bEnablePrivilege,
	CL_ERR_DESC *err_code
	)
{
	HANDLE hToken = NULL;
	BOOL adjust_priv = FALSE;
	STATUS ret_val = FAIL;

	CLEAR_ERR(err_code)	
	

	if (OpenProcessToken( GetCurrentProcess(), 
				TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
				&hToken ) == FALSE)
	{
	     err_code->errnum = GetLastError();
	}	
	
	if (hToken != NULL)
		adjust_priv = SetPrivilege(hToken, SE_DEBUG_NAME, bEnablePrivilege);


	if (hToken)
		CloseHandle(hToken);

	if (adjust_priv)
		ret_val = OK;

	return ret_val;

}

/*
**  Name: PCisAdmin
**
**  Description:
**	Checks if user running the applicaton has Administrative privileges
**
**  Inputs:
**
**  Outputs:
**	none
**
**  Returns:
**	TRUE		    has admin privileges ( has no meaning on UNIX )
**	FALSE		    doesn't have admin privileges
**
**  History:
**	21-Jun-2007 (drivi01)
**	     Created.
*/
BOOL
PCisAdmin()
{

	return IsUserAnAdmin();

}

/*
**  Name: SetPrivilege
**
**  Description:
**	Enables or disables a security privilege.
**
**  Inputs:
**	hToken		    handle to the token
**	Privilege	    Privilege to enable/disable
**	bEnablePrivilege    TRUE to enable.  FALSE to disable
**
**  Outputs:
**	none
**
**  Returns:
**	TRUE		    success
**	FALSE		    failure
**
**  History:
**	24-oct-2002 (somsa01)
**	    Initialize tpPrevious to avoid improper attribute settings
**	    for a privilege.
**	05-Jul-2005 (drivi01)
**	    SetPrivilege function no longer static. Function being used 
**	    by pcforcex.c.
*/


BOOL 
SetPrivilege(
    HANDLE hToken,
    LPCTSTR Privilege,
    BOOL bEnablePrivilege
    )
{
    TOKEN_PRIVILEGES tp;
    LUID luid;
    TOKEN_PRIVILEGES tpPrevious ZERO_FILL;
    DWORD cbPrevious = sizeof(TOKEN_PRIVILEGES);

    if (!LookupPrivilegeValue( NULL, Privilege, &luid )) 
	return (FALSE);

    /*
    ** first pass.  get current privilege setting
    */
    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            &tpPrevious,
            &cbPrevious
            );

    if (GetLastError() != ERROR_SUCCESS) 
	return (FALSE);

    /*
    ** second pass.  set privilege based on previous setting
    */
    tpPrevious.PrivilegeCount       = 1;
    tpPrevious.Privileges[0].Luid   = luid;

    if (bEnablePrivilege) 
    {
        tpPrevious.Privileges[0].Attributes |= (SE_PRIVILEGE_ENABLED);
    }
    else 
    {
        tpPrevious.Privileges[0].Attributes ^= (SE_PRIVILEGE_ENABLED &
                                           tpPrevious.Privileges[0].Attributes);
    }

    AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tpPrevious,
            cbPrevious,
            NULL,
            NULL
            );

    if (GetLastError() != ERROR_SUCCESS) 
	return (FALSE);

    return (TRUE);
}
