/*
** Copyright (c) 1994, 2001 Ingres Corporation
*/
# ifndef NT_GENERIC
# error This should only be built under Windows NT
# endif
# include <compat.h>
# include <st.h>
# include <si.h>
# include <ep.h>
# include <er.h>
# include <me.h>
# include <pc.h>
# include <windows.h>
# include <winsvc.h>
# include <gl.h>
# include <iicommon.h>
# include "repntsvc.h"

/**
** Name:	repstart.c - start Replicator service
**
** Description:
**	Controls Replicat as an NT Service.
**
** History:
**	13-jan-97 (joea)
**		Created based on repstart.c in replicator library.
**      25-Jun-97 (fanra01)
**              Add includes required by repntsvc.h
**	24-jun-99 (abbjo03)
**		Change usage message since this is now the rpserver executable.
**	09-aug-1999 (mcgem01)
**		Replace nat with i4
**	23-dec-1999 (abbjo03)
**		Add include of me.h.
**	28-jan-2000 (somsa01)
**	    Services are now keyed off of the installation identifier.
**	02-jul-2001 (somsa01)
**	    Cleaned up some holes whereby we were not closing the NT
**	    service handles.
**	25-Jul-2007 (drivi01)
**	    On Vista, elevation is required to run this program. 
**	    If user running this program doesn't have elevated 
**	    privileges, the program will exit.
**/

/*{
** Name:	main
**
** Description:
**	Start Replicator (replicat) as an NT Service.
**
** Inputs:
**	argc and argv from the command line
**		server_no	- server number
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**      14-Aug-97 (fanra01)
**          Modified the Query loop for service status give a chance to the
**          service control manager to update status.
**	28-jan-2000 (somsa01)
**	    Services are now keyed off of the installation identifier.
*/
i4
main(
i4	argc,
char	*argv[])
{
	SC_HANDLE	scm;
	SC_HANDLE	scs;
	i4		svc_argc;
	char		**svc_argv;
	SERVICE_STATUS	svc_status;
	TCHAR		svc_name[32];
	DWORD		ckpt = 0;
        BOOL            blCheck = TRUE;
	char		*cptr;

        MEfill (sizeof (SERVICE_STATUS), 0, &svc_status);
        svc_status.dwWaitHint = 2000;   /* default time set after start */
                                        /* service */

	if (argc < 2)
	{
		SIprintf("usage: rpserver <server_no> [args]\n");
		PCexit(FAIL);
	}
	NMgtAt("II_INSTALLATION", &cptr);
	STprintf(svc_name, ERx("%s_%s_%s"), SVC_NAME, cptr, argv[1]);

	/* connect to the service control manager */
	scm = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
	if (scm == NULL)
	{
		SIprintf("Error %d in OpenSCManager.\n", (i4)GetLastError());
		PCexit(FAIL);
	}

	scs = OpenService(scm, svc_name, SERVICE_START | SERVICE_QUERY_STATUS);
	if (scs == NULL)
	{
		DWORD dwError = GetLastError();
		switch(dwError)
		{
		     case ERROR_ACCESS_DENIED:
			SIprintf(ERROR_DENIED_PRIVILEGE);
			SIprintf(ERROR_REQ_ELEVATION);
			CloseServiceHandle(scm);
			PCexit(dwError);
		     default:
			SIprintf("Error %d in OpenService.\n", dwError);
			CloseServiceHandle(scm);
			PCexit(FAIL);
		}
	}

	svc_argc = argc - 2;
	svc_argv = (svc_argc == 0 ? NULL : argv + 2);
	if (!StartService(scs, svc_argc, svc_argv))
	{
		SIprintf("Error %d in StartService.\n", (i4)GetLastError());
		CloseServiceHandle(scs);
		CloseServiceHandle(scm);
		PCexit(FAIL);
	}

        Sleep (svc_status.dwWaitHint);

        do
        {
            ckpt = svc_status.dwCheckPoint; /* update check point */

            if (!QueryServiceStatus(scs, &svc_status))
            {
                SIprintf("Error %d in QueryServiceStatus.\n",
                         (i4)GetLastError());
		CloseServiceHandle(scs);
		CloseServiceHandle(scm);
                PCexit(FAIL);
            }

            /*
            ** if the check point hasn't moved since last
            ** check stop checking
            */
            switch (svc_status.dwCurrentState)
            {
                case SERVICE_START_PENDING:
                case SERVICE_CONTINUE_PENDING:
                    Sleep(svc_status.dwWaitHint);
                    break;
                case SERVICE_RUNNING:
                case SERVICE_STOPPED:
                default:
                    blCheck = FALSE;
                    break;
            }
        } while (blCheck);

	if (svc_status.dwCurrentState != SERVICE_RUNNING)
	{
		SIprintf("%s service not started:\n", svc_name);
		SIprintf("  Current State: %d\n",
			(i4)svc_status.dwCurrentState);
		SIprintf("  Exit Code: %d\n",
			(i4)svc_status.dwWin32ExitCode);
		SIprintf("  Service Specific Exit Code: %d\n",
			(i4)svc_status.dwServiceSpecificExitCode);
	}

	CloseServiceHandle(scs);
	CloseServiceHandle(scm);
}
