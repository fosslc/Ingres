/*
**  Copyright (c) 1983, 2002 Ingres Corporation
*/

# include	<compat.h>
# include	<cs.h>
# include	<pc.h>
# include	<di.h>
# include   <st.h>
# include   <Psapi.h>

# define    DEFAULT_PROCESS_COUNT   1024


/*
**  Name: PCis_alive	- Is this process still alive?
**
**  Description:
**	This function can be called to determine whether a given process is
**	alive or not.
**
**  Inputs:
**	pid		    Process to check
**
**  Outputs:
**	none
**
**  Returns:
**	TRUE		    Process appears to be alive. We also return TRUE if
**			    we can't tell whether the process is alive because
**			    a system problem keeps us from finding out.
**	FALSE		    Process is definitely NOT alive
**
**  History:
**      23-Jun-97 (fanra01)
**          Bug 83230
**          Add temporary grant of privilege to view all processes in the
**          system.
**      15-Sep-97 (fanra01)
**          Modified to release used handles.
**	08-dec-1997 (canor01)
**	    If there aren't enough system resources to open a handle on the
**	    process, try freeing some disk system resources via DIlru.  Also,
**	    if we're asking about current process, assume the best.
**	28-jan-1998 (canor01)
**	    Eliminate retries, which can be very expensive when a server
**	    has died and all the semaphore handles must be re-checked.
**	04-mar-1998 (canor01)
**	    Parameters to DIlru_flush were changed on Unix side.  Change
**	    them here too.
**	06-apr-1998 (canor01)
**	    If granting privileges is not supported (Windows95), just try
**	    the OpenProcess().
**	12-may-1998 (canor01)
**	    A valid handle can be returned on a process that has exited
**	    if its parent has not waited for it and is itself still alive.
**  06-aug-1999 (mcgem01)
**      Replace nat and longnat with i4.
**	28-jul-2005 (stephenb)
**	    Remove calls to SetPrivilege, we don't need to do this on 
**	    currently supported Windows platforms and it causes excesive
**	    CPU usage in lsass.exe when running more the 4 servers 
**	    (as we do in an MDB large environment)
**	03-Oct-2005 (fanra01)
**      Bug 115298
**      The change to remove SetPrivilege call can cause PCis_alive to report
**      a process is not alive if the caller receives an access denied status.
**      Re-implement the function using the EnumProcesses call.
**	06-Dec-2006 (drivi01)
**	    Move SetPrivilege to pcpriv.c and isolate usage of that function 
**      to that file only.
*/
bool
PCis_alive(PID pid)
{
    bool    blStatus= FALSE;
    HANDLE  hProcess= NULL;
    DWORD   dwError;
    
    DWORD aProcesses[DEFAULT_PROCESS_COUNT], cbNeeded, cProcesses;
    DWORD* pProcs = aProcesses;
	DWORD* hpProcs = NULL;
    DWORD  dwSize = sizeof(aProcesses);
    unsigned int i;
    
    blStatus = ( pid == GetCurrentProcessId() );

    while (!blStatus)
    {
        if ( !EnumProcesses( pProcs, dwSize, &cbNeeded ) )
        {
            dwError = GetLastError();
            break;
        }
        if (dwSize == cbNeeded)
        {
            /*
            ** hpProcs is used to store the pointer that can be freed.
            ** It also acts as an indicator for the need to free
            ** memory.
            */
            if (hpProcs)
            {
                MEfree( hpProcs );
                /*
                ** Re-initialize so that if the allocation fails
                ** don't try and free a bad pointer.
                */
                hpProcs = NULL;
            }
            dwSize += dwSize;
            if ((pProcs = MEreqmem(0, dwSize, TRUE, NULL)) == NULL)
            {
                break;
            }
            else
            {
                /*
                ** Save the allocated memory pointer.
                */
				hpProcs = pProcs;
                continue;
            }
        }
        cProcesses = cbNeeded / sizeof(DWORD);
        for (i = 0; i < cProcesses; i++)
        {
			if (pProcs && (*pProcs++ == pid))
            {
                blStatus = TRUE ;
                break;
            }
        }
        break;
    }
    if (hpProcs)
    {
        MEfree( hpProcs );
    }
    return (blStatus);
}


