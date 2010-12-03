/*
**    Copyright (c) 1987, 1997 Ingres Corporation
*/

# include	<compat.h>
# include       <cs.h>
# include       <pc.h>
# include	<gl.h>
# include	<clconfig.h>
# include	<systypes.h>
# include	<rusage.h>
# include	<tm.h>			/* hdr file for TM stuff */
# include 	"tmlocal.h"


/**
** Name: TMSTART.C - Start system measurements
**
** Description:
**      This file contains the following tm routines:
**    
**      TMstart()        -  Start system measurements.
**
** History:
 * Revision 1.2  88/08/10  14:25:32  jeff
 * added daveb's fixs for non-bsd
 * 
 * Revision 1.1  88/08/05  13:46:56  roger
 * UNIX 6.0 Compatibility Library as of 12:23 p.m. August 5, 1988.
 * 
**      Revision 1.3  88/01/06  17:54:03  anton
**      use getrusage
**      
**      20-jul-87 (mmm)
**          Updated to meet jupiter coding standards.      
**	28-Feb-1989 (fredv)
**	    Include <systypes.h>.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	14-apr-95 (emmag)
**	    NT porting changes.
**	11-oct-95 (tutto01)
**	    NT porting changes for Alpha.
**      12-Dec-95 (fanra01)
**          Added definitions for extracting data for DLLs on windows NT
**	15-nov-1997 (canor01)
**	    Strip out Unix-specific code, return better numbers.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	11-Nov-2010 (kschendel) SIR 124685
**	    Delete CS_SYSTEM ref, not used or needed.
**/



/*{
** Name: TMstart	- Start system measurements
**
** Description:
**	Set timer for start information
**
** Inputs:
**
** Outputs:
**      time                            start time info
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      15-sep-86 (seputis)
**          initial creation
**	06-jul-87 (mmm)
*/

VOID
TMstart(tm)
register TIMER          *tm;
{
    SYSTIME		    stime;
    bool   ret;

    CSstatistics( &tm->timer_start, TRUE );

    TMnow( &stime );
    tm->timer_start.stat_wc = stime.TM_secs;

    return;
}
