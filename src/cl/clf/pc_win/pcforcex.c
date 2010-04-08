/*****************************************************************************
**
** Copyright (c) 1989, 2005 Ingres Corporation
**
*****************************************************************************/

#include    <compat.h>
#include    <pc.h>

/******************************************************************************
**
** Name: PCforce_exit	    - force a process to exit.
**
** Description:
**	This function can be called to force the indicated process to exit.
**
** Inputs:
**	pid		    Process to force to exit
**	err_code	    CL error
**
** Outputs:
**	none
**
** Returns:
**	STATUS
**
** History:
**	26-feb-97 (cohmi01)
**	    Get handle of process, use for call to TerminateProcess. Bug 79047.
**	05-Jul-2005 (drivi01)
**	    Modified PCforce_exit to set SE_DEBUG_NAME privilege.  Moved
**	    function from pcexit.c to pcforceex.c.
**	06-Dec-2006 (drivi01)
**	    Replace direct calls to SetPrivilege function with 
**	    PCadjust_SeDebugPrivilege function.
**	02-Dec-2008 (whiro01)
**	    SD issue 132612 - crash in dmfrcp from within IVM -- due to error
**	    from PCadjust_SeDebugPrivilege stepping on stack b/c &err_code
**	    passed instead of just err_code.
**
******************************************************************************/
STATUS
PCforce_exit(PID pid,
             CL_ERR_DESC *err_code)
{

	HANDLE hProc = NULL;
	HANDLE hToken = NULL;
	STATUS ret_val = FAIL;
	BOOL   adjust_priv = FALSE;

	CLEAR_ERR(err_code);


	while(TRUE)
	{
	     if (PCadjust_SeDebugPrivilege(TRUE, err_code) == FAIL)
		break;

   	     if ((hProc = OpenProcess(PROCESS_TERMINATE, FALSE, pid)) == NULL)
    	     {
		err_code->errnum = GetLastError();
		break;
	     }
	

	     if (!TerminateProcess(hProc, 0))
	     {
		err_code->errnum = GetLastError();
		break;
	     }
		
	     /* If didn't break out of the loop at this point then
	     ** process terminated successfully.
	     */
	     ret_val = OK;
	     break;

	}
	   
	if (hProc != NULL)
  	     CloseHandle(hProc);

        ret_val = PCadjust_SeDebugPrivilege(FALSE, err_code);
	
	return ret_val;
}
