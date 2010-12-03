/*
**Copyright (c) 2004 Ingres Corporation
** All rights reserved.
*/

#include    <compat.h>
#include    <gl.h>
#include    <gc.h>
#include    <st.h>
#include    <tm.h>
#include    <er.h>

/**
**
**  Name: ERMSGHDR.C - Implements the ER compatibility library routines.
**
**  Description:
**      The module contains the routines that implement the ER logging
**	portion	of the compatibility library.
**
**          ERmsg_hdr- Build the header with a timestamp for the errlog.log
**
**
**  History:    
**	22-Sep-1999 (hanch04)
**	    Created.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, cleaned-up warnings.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**	11-Nov-2010 (kschendel) SIR 124685
**	    Prototype fixes.
*/


/*{
** Name: ERhdrtm - 
**
** Description:
**      This procedure 
**
** Inputs:
**	svr_id		A server ID string, eg iidbms
**	session		The session ID (CS_SID but we'll call it a SCALARP
**			to avoid circular header references)
**	msg_header	char[] to put the output into
**
** Outputs:
**	Fills in msg_header, returns OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
*/

STATUS
ERmsg_hdr( char *svr_id, SCALARP session, char *msg_header)
{
    char        host_name[65];
    i4		length;
    SYSTIME	stime;

    GChostname( host_name, sizeof( host_name ) );
    STprintf( msg_header, "%-8.8s::[%-16.16s, %08.8x]: ",
	host_name, svr_id, session );
    TMnow(&stime);
    TMstr(&stime,msg_header + STlength(msg_header));
    length = STlength(msg_header);
    msg_header[length++] = ' ';
    msg_header[length] = EOS;
    return (OK);
}
