/* 
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <raat.h>
/*
** Name: IIraat_session_end - End raat session.
**
** Description:
**      Use embedded QUEL to end session.
**
** Inputs:
**      raat_cb          RAAT control block
**
** Outputs:
**      None.
**
** Returns:
**      STATUS          OK or FAIL
**
** History:
**      8-may-1995 (lewda02/thaju02)
**          Created.
**          Extracted from api.qc.
**	    Changed naming convention.
**      14-sep-1995 (shust01/thaju02)
**          free up RAAT internal_buf (if allocated)
**	16-jul-1996 (sweeney)
**	    Add tracing.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
STATUS
IIraat_session_end (RAAT_CB    *raat_cb)
{
##  i4          errno;

    /* log any trace information */
    IIraat_trace(RAAT_SESS_END, raat_cb);

    /*
    ** End Session: for now just use the quel disconnect command
    */
##  exit;
##  inquire_ingres(errno = dbmserror);
 
    if (errno)
    {
        raat_cb->err_code = errno;
        return (FAIL);
    }

    /* free up internal buffer (if allocated) */
    if (raat_cb->internal_buf != NULL)
    {
	MEfree(raat_cb->internal_buf);
        raat_cb->internal_buf = NULL;
        raat_cb->internal_buf_size = 0;
    }
    return (OK);
}
