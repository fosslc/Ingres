/* 
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <raat.h>
 
/*
** Name: IIraat_tx_begin - Begin raat transaction.
**
** Description:
**      Use embedded QUEL to begin transaction
**
** Inputs:
**	raat_cb		RAAT control block
**
** Outputs:
**      None.
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**      7-jul-1995 (shust01)
**        Created.
**	16-jul-1996 (sweeney)
**	    Add tracing.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
STATUS
IIraat_tx_begin (RAAT_CB    *raat_cb)
{
##  i4          errno;

    /* log any trace information */
    IIraat_trace(RAAT_TX_BEGIN, raat_cb);

    /*
    ** Start Session
    */
##  begin transaction;
##  inquire_ingres(errno = dbmserror);

    if (errno)
    {
        raat_cb->err_code = errno;
        return (FAIL);
    }

    return (OK);
}
