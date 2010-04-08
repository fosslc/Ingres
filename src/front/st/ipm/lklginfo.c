/*
**      Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
*/

/*
** includes and defines
*/
#include <compat.h>
#include <cs.h>
#include <dbms.h>
#include <fe.h>
#include <pc.h>
#include <lk.h>
#include <lg.h>
#include <er.h>
#include "lists.h"

/*
** Forward and external references
*/
VOID showlksumm();
VOID displklists();
VOID displkres();
VOID showlgsumm();
VOID displgpr();
VOID displgtx();
VOID displgdb();
VOID showlghdr();

/*
**  File: lklginfo.c
**
**  Purpose - this file contains the routines which determine
**	what locking/logging system information form to call.  
**
**  This file contains:
**	lklginfo() - determines which lock/log form to call
**
**  History
**	1/6/89		tmt		created
**	2/21/89		tmt		added support for locking summary
**	8/16/89		tmt		added support for logging info and
**					changed name to lklginfo()
**      5/26/93 (vijay)
**              include pc.h
**      21-Sep-1993 (smc)
**              Added <cs.h> for CS_SID.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
VOID
lklginfo(option)
i4  option;	/* what info the user wants to see */
{
    switch (option) {
	case GET_LOCKSTAT:
	    showlksumm();
	    break;

	case GET_LOCKLIST:
	    displklists();
	    break;

	case GET_LOCKRES:
	    displkres();
	    break;

	case GET_LOGSTAT:
	    showlgsumm();
	    break;

        case GET_LOGPROCESS:
	    displgpr();
	    break;

	case GET_LOGHEADER:
	    showlghdr();
	    break;

	case GET_LOGDBLIST:
	    displgdb();
	    break;

	case GET_LOGXACTLIST:
	    displgtx();
	    break;

	default:
	    terminate(FAIL, "lklginfo: bad option passed (%d)\n", option);
    }
}
