/*
**	Copyright (c) 2004 Ingres Corporation
*/
#include <compat.h>
#include <gl.h>

#include <gc.h>
#include <me.h>
#include <mu.h>
#include <qu.h>
#include <si.h>
#include <sl.h>
#include <tm.h>

#include <iicommon.h>
#include <gca.h>
#include <gcaint.h>
#include <gcn.h>
#include <gcnint.h>


/*
** Name: gcninit.c
**
** Description:
**	Initialization and termination routines for the name server
**	client interface.
**
**	IIgcn_initiate()
**	IIgcn_terminate()
**
** History:
**	? from the void
**	22-May-89 (seiwald)
**	    Use MEreqmem().
**	01-Jun-89 (seiwald)
**	    Check return status from GCA calls.
**	25-Nov-89 (seiwald)
**	    Removed E_GCN006_DUP_INIT error status.  It was undocumented
**	    and stupid.
**	2-June-92 (seiwald)
**	    Request GCA_PROTOCOL_LEVEL_50 so that we can talk to remote
**	    Name Servers.
**	18-Nov-92 (gautam)
**	    Request GCA_PROTOCOL_LEVEL_60 for password prompt support.
**	10-mar-93 (robf)
**	    Request GCA_PROTOCOL_LEVEL_61 for security label support
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	14-nov-95 (nick)
**	    Request GCA_PROTOCOL_LEVEL_62 for II_DATE_CENTURY_BOUNDARY support.
**	29-Nov-95 (gordy)
**	    Added IIgcn_terminate(), rename IIgcn_init() to IIgcn_initiate().
**	29-Jan-96 (gordy)
**	    Allow GCA to be initialized prior to initializing this interface.
**	 3-Sep-96 (gordy)
**	    Moved all GCA access to gcngca.c for new control block
**	    interface.  Moved communications buffers here since this
**	    is the primary access point.
**	27-sep-1996 (canor01)
**	    Move global data definitions to gcndata.c.
**	19-Mar-97 (gordy)
**	    Removed global communication buffers.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Local data.
*/
static	bool	initialized = FALSE;



/*
** Name: IIgcn_initiate
**
** Description:
**	Initialize the name server client interface.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	STATUS		E_GC0006_DUP_INIT
**
** History:
**	29-Nov-95 (gordy)
**	    Renamed to IIgcn_initiate().
**	29-Jan-96 (gordy)
**	    Allow GCA to be initialized prior to initializing this interface.
**	 3-Sep-96 (gordy)
**	    Moved all GCA access to gcngca.c for new control block
**	    interface.  
**	19-Mar-97 (gordy)
**	    Removed global communication buffers.
*/

STATUS
IIgcn_initiate( VOID )
{
    CL_ERR_DESC		cl_err;
    STATUS		status;

    if ( initialized )  
	status = E_GC0006_DUP_INIT;
    else  if ( (status = gcn_init()) == OK )
	initialized = TRUE;

    return( status );
}


/*
** Name: IIgcn_terminate
**
** Description:
**	Shutdown the name server client interface.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	STATUS		E_GC0007_NO_PREV_INIT
**
** History:
**	29-Nov-95 (gordy)
**	    Created.
**	29-Jan-96 (gordy)
**	    Terminate GCA only if it was initialized int IIgcn_initialize().
**	 3-Sep-96 (gordy)
**	    Moved all GCA access to gcngca.c for new control block
**	    interface.  
**	19-Mar-97 (gordy)
**	    Removed global communication buffers.
*/

STATUS
IIgcn_terminate( VOID )
{
    STATUS	status = OK;

    if ( ! initialized )  
	status = E_GC0007_NO_PREV_INIT;
    else
    {
	gcn_term();
	initialized = FALSE;
    }

    return( status );
}
