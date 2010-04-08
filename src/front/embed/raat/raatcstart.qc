/* 
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <raat.h>
 
/*
** Name: IIraat_session_start - Start raat session.
**
** Description:
**      Use embedded QUEL to start session.
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
**      8-may-1995 (lewda02/thaju02)
**        Created.
**        Extracted from api.qc.
**	  Changed naming convention.
**     16-jun-1995 (shust01)
**        added initialization of row_request (here's as good a place as any)
**     14-sep-1995 (shust01/thaju02)
**        added initial allocation of RAAT internal_buf (used by blobs and
**	  others)
**	16-jul-1996 (sweeney)
**	  Addded tracing.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
STATUS
IIraat_session_start (RAAT_CB    *raat_cb)
{
##  char	*dbname = raat_cb->dbname;
##  i4          errno;
    STATUS	status;

    /* log any trace information */
    IIraat_trace(RAAT_SESS_START, raat_cb);

    /*
    ** Start Session
    */
##  ingres dbname;
##  inquire_ingres(errno = dbmserror);

    if (errno)
    {
        raat_cb->err_code = errno;
        return (FAIL);
    }
    raat_cb->row_request = 0;  /* initialize prefetch to default value */
    raat_cb->internal_buf = NULL;
   
    /* initialize internal buffer to default value */
    status = allocate_big_buffer(raat_cb, 4096);

    return (status);
}
