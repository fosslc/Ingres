/* 
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <cv.h>          /* 6-x_PC_80x86 */
# include       <er.h>
# include       <me.h>
# include       <cm.h>
# include       <st.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <gca.h>
# include       <iicgca.h>
# include       <erlc.h>
# include       <generr.h>
# include       <sqlstate.h>
#define CS_SID      SCALARP
# include       <raat.h>
# include	<iisqlca.h>
# include       <iilibq.h>
  
/*
** Name: allocate_big_buffer 
**
** Description:
**	will allocate the raat_cb->internal_buf, based on size passed in
**
** Inputs:
**	raat_cb		RAAT control block
**	buf_size	size to be allocated
**
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	13-sep-95 (shust01/thaju02)
**	    created.
**	11-feb-97 (cohmi01)
**	    remove unused code.
**	18-Dec-97 (gordy)
**	    Libq session data now accessed via function call.
**	    Converted to GCA control block interface.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
STATUS
allocate_big_buffer (RAAT_CB      *raat_cb, i4 buf_size)
{
    IICGC_MSG *buf_ptr;
    STATUS gca_stat;
    GCA_PARMLIST *gca_parm;
    GCA_FO_PARMS *gca_fmt;

    if (raat_cb->internal_buf != NULL)
    {
       MEfree(raat_cb->internal_buf);
    }
    /* take number and round up some */
    buf_size = (buf_size / 1024 + 1) * 1024 + 512;
    raat_cb->internal_buf = 
	 (char *)MEreqmem((u_i4)0, (u_i4)(sizeof(IICGC_MSG) + buf_size),
	 TRUE, &gca_stat);
    if (gca_stat != OK)
       return(FAIL);
    buf_ptr = (IICGC_MSG *)raat_cb->internal_buf;

    /* do GCA FORMAT */
    buf_ptr->cgc_b_length = buf_size;
    buf_ptr->cgc_buffer   = ((char *)buf_ptr) + sizeof(IICGC_MSG);

    gca_parm = &buf_ptr->cgc_parm;
    gca_fmt = &gca_parm->gca_fo_parms;

    gca_fmt->gca_buffer    = buf_ptr->cgc_buffer;
    gca_fmt->gca_b_length  = buf_ptr->cgc_b_length;
    gca_fmt->gca_data_area = (char *)0;
    gca_fmt->gca_d_length  = 0;
    gca_fmt->gca_status	   = E_GC0000_OK;

    IIGCa_cb_call( &IIgca_cb, GCA_FORMAT, gca_parm, 
		   GCA_SYNC_FLAG, 0, IICGC_NOTIME, &gca_stat );

    if (gca_fmt->gca_status != E_GC0000_OK)
    {
        if (gca_stat == E_GC0000_OK)
	   gca_stat = gca_fmt->gca_status;
    }
    if (gca_stat == E_GC0000_OK)
    {
       raat_cb->internal_buf_size = 
	   buf_ptr->cgc_d_length = gca_fmt->gca_d_length;
       buf_ptr->cgc_d_used   = 0;
       buf_ptr->cgc_data     = gca_fmt->gca_data_area;
       gca_stat = OK;
    }
    else
       gca_stat = FAIL;

    return(gca_stat);
}


/*
** Name: IIcraat_init 
**
** Description:
**	will initialize the RAAT_CB control block.  Called by user.
**
** Inputs:
**	raat_cb		RAAT control block
**
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	02-oct-95 (shust01/thaju02)
**	    created.
*/
STATUS
IIcraat_init (RAAT_CB      *raat_cb)
{
    STATUS	status;

    raat_cb->row_request = 0;  /* initialize prefetch to default value */
    raat_cb->internal_buf = NULL;
 
    /* initialize internal buffer to default value */
    status = allocate_big_buffer(raat_cb, 4096);

    return(status);
}


/*
** Name: IIcraat_end
**
** Description:
**	will cleanup the RAAT_CB control block.  Called by user.
**
** Inputs:
**	raat_cb		RAAT control block
**
** Outputs:
**	None.
**
** Returns:
**	STATUS		OK or FAIL
**
** History:
**	02-oct-95 (shust01/thaju02)
**	    created.
*/
STATUS
IIcraat_end (RAAT_CB      *raat_cb)
{
    STATUS	status;

    /* free up internal buffer (if allocated) */
    if (raat_cb->internal_buf != NULL)
    {
        status = MEfree(raat_cb->internal_buf);
        raat_cb->internal_buf = NULL;
        raat_cb->internal_buf_size = 0;
    }
    return (status);  
}


