/******************************************************************************
**
**Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

#include <ddfcom.h>
#include <ddfsem.h>
#include <ddfhash.h>
#include <erwsf.h>
#include <wsf.h>
#include <usrsession.h>
#include <wsmmo.h>

/*
**
**  Name: wsminit.c - Web session manager
**
**  Description:
**		WSM manage active sessions (one active sessions per HTML pages)
**		and user sessions 
**	
**  History:
**	5-Feb-98 (marol01)
**	    created
**      03-Jul-98 (fanra01)
**          Include ima objects for sessions.
**      11-Sep-1998 (fanra01)
**          Corrected case for usrsession to match piccolo.
**          Fixup compiler warnings on unix.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      16-Mar-2001 (fanra01)
**          Bug 104298
**          Add initialisation of the maxrowset and maxsetsize variables.
**/

GLOBALREF SEMAPHORE		actSessSemaphore;
GLOBALREF SEMAPHORE		usrSessSemaphore;
GLOBALREF SEMAPHORE		transSemaphore;
GLOBALREF SEMAPHORE		curSemaphore;
GLOBALREF SEMAPHORE		requested_pages_Semaphore;
GLOBALREF DDFHASHTABLE	usr_sessions;

GLOBALREF i4		system_timeout;
GLOBALREF i4		db_timeout;
GLOBALREF i4        minrowset;
GLOBALREF i4        maxsetsize;

/*
** Name: WSMInitialize() - 
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
**      03-Jul-98 (fanra01)
**          Rename wts_define to act_define and add usr_define.
**          Removed wps_define. Add class definitions for transactions and
**          cursors.
**      16-Mar-2001 (fanra01)
**          Add set up of rowset and setsize values read from config.
*/
GSTATUS 
WSMInitialize () 
{
	GSTATUS err = GSTAT_OK;
	STATUS status = OK;
	char*	size= NULL;
	char	szConfig[CONF_STR_SIZE];
	u_i4	user_sess_number = 50;

	if ((status = act_define()) != OK)
		err = DDFStatusAlloc(status);

	if ((err == GSTAT_OK) && (status = usr_define()) != OK)
		err = DDFStatusAlloc(status);

	if ((err == GSTAT_OK) && (status = usr_trans_define()) != OK)
		err = DDFStatusAlloc(status);

	if ((err == GSTAT_OK) && (status = usr_curs_define()) != OK)
		err = DDFStatusAlloc(status);

	STprintf (szConfig, CONF_SYSTEM_TIMEOUT, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
		CVan(size, &system_timeout);

	STprintf (szConfig, CONF_SESSION_TIMEOUT, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
		CVan(size, (i4*)&db_timeout);

	STprintf (szConfig, CONF_USER_SESSION, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
		CVan(size, (i4*)&user_sess_number);

	STprintf (szConfig, CONF_ROW_SET_MIN, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
        {
		CVan(size, (i4*)&minrowset);
        }
	STprintf (szConfig, CONF_SET_SIZE_MAX, PMhost());
	if (PMget( szConfig, &size ) == OK && size != NULL)
        {
            i4 kbsetsize;

		if (CVan(size, (i4*)&kbsetsize) == OK)
                {
                    maxsetsize = kbsetsize * SET_BLOCK_SIZE;
                }
        }

	err = DDFmkhash(user_sess_number, &usr_sessions);

	if (err == GSTAT_OK)
		err = DDFSemCreate(&actSessSemaphore, "Active Sessions");
	if (err == GSTAT_OK)
		err = DDFSemCreate(&usrSessSemaphore, "User Sessions");
	if (err == GSTAT_OK)
		err = DDFSemCreate(&transSemaphore, "Transactions");
	if (err == GSTAT_OK)
		err = DDFSemCreate(&curSemaphore, "Cursors");
	if (err == GSTAT_OK)
		err = DDFSemCreate(&requested_pages_Semaphore, "Requested files");
return(err);
}

/*
** Name: WSMTerminate() - 
**
** Description:
**
** Inputs:
**
** Outputs:
**
** Returns:
**	GSTATUS	:	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/
GSTATUS 
WSMTerminate () 
{
	DDFHASHSCAN sc;
	USR_PSESSION session = NULL;

	DDF_INIT_SCAN(sc, usr_sessions);
	while (DDF_SCAN(sc, session) == TRUE && session != NULL)
		CLEAR_STATUS(WSMRemoveUsrSession(session));

	CLEAR_STATUS(DDFrmhash(&usr_sessions));
	CLEAR_STATUS(DDFSemDestroy(&curSemaphore));
	CLEAR_STATUS(DDFSemDestroy(&transSemaphore));
	CLEAR_STATUS(DDFSemDestroy(&usrSessSemaphore));
	CLEAR_STATUS(DDFSemDestroy(&actSessSemaphore));
	CLEAR_STATUS(DDFSemDestroy(&requested_pages_Semaphore));
return(GSTAT_OK);
}
