/*
** Copyright (c) 1999, 2003  Ingres Corporation
** All rights reserved.
*/

#include    <compat.h>
#include    <gl.h>
#include    <gc.h>
#include    <st.h>
#include    <tm.h>

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
**	26-oct-2001 (kinte01)
**	    clean up compiler warnings
**	05-sep-2003 (abbjo03)
**	    Really silence the compiler warning by returning a value. Note
**	    however, that current callers of ERmsg_hdr don't check the return.
*/

/*{
** Name: ERhdrtm - 
**
** Description:
**      This procedure 
**
** Inputs:
**	flag
**	    ER_ERROR_MSG		write message to errlog.log
**	    ER_AUDIT_MSG		obsolete
**	    ER_OPER_MSG			write to operator (system log, event
**					log, or system-configurable log)
**      message                         Address of buffer containing the message.
**      msg_length                      Length of the message.
**
** Outputs:
**      err_code                        Operating system error code.
**	Returns:
**	    OK
**	    ER_BADOPEN
**	    ER_BADSEND
**	    ER_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
*/

STATUS
ERmsg_hdr( svr_id, session, msg_header)
char		   *svr_id;
SCALARP		   session;
char		   *msg_header;
{
    char        host_name[33];
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
    return OK;
}
