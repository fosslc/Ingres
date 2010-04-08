/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <st.h>
#include    <er.h>
#include    <qu.h>
#include    <si.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <gca.h>
#include    <gcn.h>
#include    <gcnint.h>
#include    <gcu.h>

/**
**
**  Name: gcnseror.c - Format and log error messages.
**
**  Description:
**      This utility routine formats and send error message to the
**	client.
**
**  History:    $Log-for RCS$
**	 26-may-1988	(LIN)
**		modified from uleformat.c
**      23-jan-1989 (roger)
**          Revised to conform to latest ERlookup specification.
**	01-Mar-89 (seiwald)
**	    Moved checkerr() and err_print() out.
**	24-Apr-89 (jorge)
**	    re-applied Roger's ERlookup parm fix 
**	01-Jun-89 (seiwald)
**	    Check return status from GCA calls.
**	16-Jul-89 (jorge)
**	    Changed call to ERlookup for new Generic error interface change.
**	17-Jul-89 (seiwald)
**	    Built upon gca_erlog() and gca_erfmt().
**	25-Nov-89 (seiwald)
**	    Reworked for async GCN.
**	24-Apr-91 (bxk)
**	    Added include for tm.h
**	1-Sep-92 (seiwald)
**	    gca_erlog() renamed to gcu_erlog() and now takes ER_ARGUMENT's.
**      11-aug-93 (ed)
**          added missing includes
**	 4-Dec-95 (gordy)
**	    Moved global references to gcnint.h.  Added prototypes.
**	11-Mar-97 (gordy)
**	    Moved gcn utility functions to gcu.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: gcn_error	- Format and write Name Server error message.
**
** Description:
**      This routine manages formatting and writing Name Server error messages.
**	The language is specified by the client in GCA_MD_ASSOC information
**	and is used when ERlookup is called.
**
** Inputs:
**      error_code                      Error_code.
**	clerror				CL error code.
**	arg				Single potential param for message.
**					Must be a string.
**
** History:
**      23-jan-1989 (roger)
**          Revised to conform to latest ERlookup specification: changed
**	    CL_SYS_ERR to CL_ERR_DESC; pass this to ERlookup along with
**	    `error_code' if the latter is a CL return status.  Also got
**	    rid of Code From Hell that assumed it could look inside the
**	    the CL_ERR_DESC (at `error', which no longer exists in the
**	    UNIX implementation).  Clarified messages.
**	17-Jul-89 (seiwald)
**	    Seriously simplified interface.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
*/

VOID
gcn_error(i4 error_code, CL_ERR_DESC *clerror, char *arg, GCN_MBUF *mbuf)
{
    char	    *bp;
    GCA_ER_DATA	    *er_data;
    GCA_E_ELEMENT   *element;
    i4		    l_eargs;
    ER_ARGUMENT	    eargs[1];
    char	    *buf, *post;
    STATUS	    status;

    /* Pick up arg */

    if( l_eargs = ( arg ? 1 : 0 ) )
    {
	eargs[0].er_value = (PTR)arg;
	eargs[0].er_size = STlength( arg );
    }

    /* Log message */

    gcu_erlog( 0, IIGCn_static.language, 
	       error_code, clerror, l_eargs, (PTR)eargs );

    /* Get a response buffer for the message. */

    mbuf = gcn_add_mbuf( mbuf, TRUE, &status );

    if( status != OK )
	return;

    mbuf->type = GCA_ERROR;
    buf = mbuf->data;
    post = mbuf->data + mbuf->len;

    /* Prepare to send GCA_ER_DATA message. */

    er_data = (GCA_ER_DATA *) buf;
    er_data->gca_l_e_element = 1;
    element = &er_data->gca_e_element[0];
    element->gca_id_error = error_code;
    element->gca_severity = 0;
    element->gca_l_error_parm = 1;
    element->gca_error_parm[0].gca_type = DB_CHA_TYPE;

    /* Fill message buffer with error message. */

    bp = element->gca_error_parm[0].gca_value;

    gcu_erfmt( &bp, post, IIGCn_static.language, 
	       error_code, clerror, l_eargs, (PTR)eargs );

    element->gca_error_parm[0].gca_l_value = 
	bp - element->gca_error_parm[0].gca_value;

    mbuf->used = bp - mbuf->data;
}
