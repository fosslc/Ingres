/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <er.h>
#include    <ex.h>
#include    <pc.h>
#include    <tm.h>
#include    <cs.h>
#include    <cv.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <scf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <gca.h>
#include    <ulm.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <scfcontrol.h>

/**
**
**  Name: scdnote.c - General inside SCF scd services
**
**  Description:
**      This file contains a variety of server level services 
**      which are called from inside SCF. 
**
**
**          scd_note - Note that some sort of severe error has occurred
[@func_list@]...
**
**
**  History:    $Log-for RCS$
**      24-Jul-86 (fred)
**          Created on Jupiter.
**	13-jul-92 (rog)
**	    Included ddb.h and er.h.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	2-Jul-1993 (daveb)
**	    prototyped.
**      10-jul-1993 (shailaja)
**	    Cast parameters to match prototypes.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**      14-jun-1995(chech02)
**          Added SCF_misc_sem semaphore to protect critical sections.(MCT)
**	03-jun-1996 (canor01)
**	    Replace external semaphore with one localized to Sc_main_cb.
**      08-Feb-1999 (fanra01)
**          Renamed scd_note to ascd_note for the icesvr.
[@history_template@]...
**/

/*
**  Forward and/or External function references.
*/

GLOBALREF SC_MAIN_CB                    *Sc_main_cb;

/*{
** Name: ASCD_NOTE	- Note that an exceptional event has taken place
**
** Description:
**	This routine is called whenever some abnormal event takes place 
**      in SCF.  If the error is not E_DB_FATAL, then the error is noted 
**      in the scb for the session.  When the count in the scb reaches 
**      the allowable threshold, the session is terminated.
**
**	If the error is fatal, the error is noted in the server control block.
**	Once again, when the error reaches the allowable threshold,
**	the server will be terminated.
**
** Inputs:
**      status                          DB_STATUS level of the error
**      cluprit                         the facility responsible for the error.
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    May cause the session or server to evaporate.
**
** History:
**      24-Jul-1986
**          Created on Jupiter.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	28-Jul-1993 (daveb)
**	    put right args to sc0e_put.
[@history_template@]...
*/
VOID
ascd_note( DB_STATUS status, i4   culprit )
{
    SCD_SCB	    *scb;
    i4		    count;

    if ((culprit < DB_MIN_FACILITY) || (culprit > DB_MAX_FACILITY))
    {
	sc0e_put(E_SC0223_BAD_SCDNOTE_FACILITY, 0, 2,
		 sizeof (status), (PTR)&status,
		 sizeof(culprit), (PTR)&culprit,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0,
		 0, (PTR)0 );
	culprit = DB_SCF_ID;
    }

    switch (status)
    {
	case E_DB_OK:
	case E_DB_WARN:
	case E_DB_INFO:
	case E_DB_ERROR:
	    return;

	case E_DB_SEVERE:
	    CSget_scb((CS_SCB **)&scb);
	    scb->scb_sscb.sscb_noerrors++;
	    if (scb->scb_sscb.sscb_noerrors > 1)
	    {
		sc0e_put(E_SC0220_SESSION_ERROR_MAX, 0, 1,
			 sizeof(i4), (PTR)&culprit,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		CSremove(scb->cs_scb.cs_self);
	    }
	    break;

	case E_DB_FATAL:
	    CSp_semaphore( TRUE, &Sc_main_cb->sc_misc_semaphore );
	    Sc_main_cb->sc_errors[culprit]++;
	    if (Sc_main_cb->sc_errors[culprit] > 1)
	    {
		CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );
		sc0e_put(E_SC0221_SERVER_ERROR_MAX, 0, 1,
			sizeof(i4), (PTR)&culprit,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0,
			 0, (PTR)0);
		CSterminate(CS_KILL, &count);
	    }
	    else
	        CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );
	    break;

	default:
	    sc0e_put(E_SC0222_BAD_SCDNOTE_STATUS, 0, 2, 
		     sizeof(status), (PTR)&status,
		     sizeof (culprit), (PTR)&culprit,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0);
	    break;
    }
}

