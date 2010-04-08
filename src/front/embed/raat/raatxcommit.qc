/* 
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <raat.h>
 
/*
** Name: IIraat_tx_commit - Commit raat transaction.
**
** Description:
**      Use embedded QUEL to commit transaction
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
**	08-sep-1997 (somsa01)
**	    Since we are committing the transaction, we need to also mark that
**	    the table is closed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
STATUS
IIraat_tx_commit (RAAT_CB    *raat_cb)
{
##  i4          errno;

    /* log any trace information */
    IIraat_trace(RAAT_TX_COMMIT, raat_cb);

    /*
    ** Start Session
    */
##  end transaction;
##  inquire_ingres(errno = dbmserror);

    if (errno)
    {
        raat_cb->err_code = errno;
        return (FAIL);
    }

    if (raat_cb->table_handle)
	raat_cb->table_handle->table_status &= ~(TABLE_OPEN);

    return (OK);
}
