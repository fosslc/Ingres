/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <er.h>
#include    <qu.h>
#include    <si.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcu.h>
#include    <gcnint.h>

/*
** Name: gcnerror.c
**
** Description:
**     Contains the following functions:
**
**	    gcn_checkerr    - GCA error checker for IINAMU, NETU, or NETUTIL
**
**          gcn_seterr_func - Set up error message handler function. 
**
**
** History:
**	01-Mar-89 (seiwald)
**	    Extracted from netu.c and namu.c.
**	01-Jun-89 (seiwald)
**	    Replaces old checkerr().
**	16-Jul-89 (jorge)
**	    Changed call to ERlookup for new Generic error interface change.
**	17-Jul-89 (seiwald)
**	    Built upon gca_erfmt() now - a gca version of ule_format().
**	05-Dec-90 (seiwald)
**	    Null terminate message before printing it.
**	1-Sep-92 (seiwald)
**	    gca_erlog() renamed to gcu_erlog() and now takes ER_ARGUMENT's.
**	08-Sep-92 (alan)
**	    ulf.h is no longer needed.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	29-Sep-93 (joplin)
**	    Put in support for an error message handler.  gcn_seterr_func()
**	 4-Dec-95 (gordy)
**	    Added prototypes.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      15-Nov-2010 (stial01) SIR 124685 Prototype Cleanup
**          Changes to eliminate compiler prototype warnings.
*/


/*
** When gcn_err_func is NULL, errors are displayed using SIprintf,
** otherwise this points to an error message handler function.
*/

static i4  ((*gcn_err_func)(char *)) = NULL;

/*
** Name: gcn_checkerr() - GCA error checker for IINAMU, NETU, or NETUTIL
**
** Description:
**      Check returns from GCA calls; format and optionally print or
**      invoke a message handler for an error message.
**
** Inputs:
**      service - name of GCA call (ignored)
**      status, gca_status, cl_status - status milieu
**
** Outputs:
**      status - OR or !OK
**
*/

VOID
gcn_checkerr( char *service, STATUS *status, 
	      STATUS gca_status, CL_ERR_DESC *cl_error )
{
    char	message[ER_MAX_LEN];
    char	*p = message;

    if( *status == OK &&
        gca_status != E_GC0000_OK  &&
	gca_status != E_GCFFFF_IN_PROCESS )
    {
	*status = gca_status;
    }

    if( *status == OK )
	return;

    /* 
    ** Format & dump message.
    */
    gcu_erfmt( &p, message + ER_MAX_LEN, 1, *status, cl_error, 0, NULL );
    *p = 0;

    /*
    ** Before displaying an error, check for a message handler function.
    */
    if (gcn_err_func == NULL)
	SIprintf( "%s", message );
    else
	(*gcn_err_func)( message );
}

/*
** Name: gcn_seterr_func() - Establish an error message handler.
**
** Description:
**      Set the error message handler function. This allows a forms based
**      app, such as NETUTIL, to receive error messages.  This keeps messages 
**      from being displayed with an SIprintf and messing up the forms.
**
** Inputs:
**      err_func - address of an error message handling function
**                 When this value is NULL the default message handler
**                 SIprintf() will display messages, otherwise this
**                 should point to a function as follows:
** 
**                 i4  gcn_errmsg_func( msg )
**                 char *msg;
**                 {
**                     .. process the string containing the error (perhaps) ..
**                     IIUGeppErrorPrettyPrint(stderr,msg,FALSE); 
**                 }
**
** Outputs:
**      gcn_err_func is set to point to the message handler.  
**
*/

VOID
gcn_seterr_func( i4  ((*err_func)(char *)) )
{
    gcn_err_func = err_func;
}
