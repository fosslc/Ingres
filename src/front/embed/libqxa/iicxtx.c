/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <cv.h>
# include       <er.h>
# include       <me.h>
# include       <st.h>
# include       <si.h>		/* needed for iicxfe.h */
# include       <iicx.h>
# include       <iicxtx.h>
# include       <iicxutil.h>
# include	<iicxfe.h>
# include       <generr.h>
# include       <ercx.h>

/*
**  Name: iicxtx.sc - Has a number of utility routines used for manipulating
**                    the ids/structures for TX support in the IICX-component.
**
**  IICXcreate_icas_xn_cb  - get an ICAS XN CB, or create it.
**  IICXdelete_icas_xn_cb  - logically delete an ICAS XN CB.
**  IICXfree_icas_xn_cb    - physically delete an ICAS XN CB.
**
**  Description:
**	
**  Defines:
**      
**  Notes:
**	<Specific notes and examples>
**
**  History:
**	30-Sep-1993 (mhughes)
**	    First Written
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**   Name: IICXcreate_icas_xn_cb()
**
**   Description:
**        The ICAS XN CB is created, if needed. Else it is obtained from a
**        free list.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: Pointer to the ICAS CB, if properly created.
**                NULL if there was an error.
**
**   History:
**	30-Sep-1993 (mhughes)
**	    Modified XA_XN_THRD routines for new ICAS_XN cb
*/

FUNC_EXTERN  DB_STATUS  IICXcreate_icas_xn_cb(
                           IICX_ID             *cx_id,          /* IN */
                           IICX_CB             **cx_cb_p_p      /* OUT */
                        )
{
    DB_STATUS		db_status;
    STATUS		cl_status;
    char		ebuf[20];
    IICX_ICAS_XN_CB     *icas_xn_cb_p;
    i4			i;

    /* LOCK the ICAS XN MAIN CB. CHECK !!! */

    if (IIcx_icas_xn_main_cb->num_free_icas_xn_cbs > 0)
    {
        *cx_cb_p_p = IIcx_icas_xn_main_cb->icas_xn_cb_free_list;
        IIcx_icas_xn_main_cb->icas_xn_cb_free_list =
                                          (*cx_cb_p_p)->cx_next;

        if ((*cx_cb_p_p)->cx_next != NULL)
            (*cx_cb_p_p)->cx_next->cx_prev = (*cx_cb_p_p)->cx_prev;
         
        IIcx_icas_xn_main_cb->num_free_icas_xn_cbs--;
    }

    if (*cx_cb_p_p != NULL)
    {
	/* UNLOCK the ICAS XN  Main CB. CHECK !!! */
        return( E_DB_OK );
    }

    /* UNLOCK the ICAS XN Main CB. CHECK !!! */

    /* No free ICAS XN CBs. Create one if appropriate */
    db_status = IICXget_new_cx_cb( cx_cb_p_p );
    if (DB_FAILURE_MACRO(db_status))
    {
       return(db_status);
    }

    if ((icas_xn_cb_p = (IICX_ICAS_XN_CB *)MEreqmem((u_i4)0, 
                                (u_i4)sizeof(IICX_ICAS_XN_CB),
                                TRUE, (STATUS *)&cl_status)) == NULL)
    {
           CVna((i4)cl_status, ebuf);
           IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                      ERx("IICX_ICAS_XN_CB"));
           return( E_DB_FATAL );
    }

    (*cx_cb_p_p)->cx_sub_cb.icas_xn_cb_p = icas_xn_cb_p;

    return( E_DB_OK );

} /* IICXcreate_icas_xn_cb */


/*
**   Name: IICXdelete_icas_xn_cb()
**
**   Description: 
**        The ICAS XN control block is logically deleted. It deletes all of
**        its children. If the last of its children are gone, it places
**        itself in a process-scoped free list.
**
**   Outputs:
**
**   History:
**      30-Sep-93 (mhughes)
**	    Modified xa_xn_thrd delete to_icas_delete
*/

DB_STATUS
IICXdelete_icas_xn_cb(
            IICX_CB       *cx_cb_p    /* IN */
           )
{
    DB_STATUS		db_status;
    IICX_ICAS_XN_CB 	*icas_xn_cb_p;
    i4			i;

    /* LOCK the ICAS XN MAIN CB. CHECK !!! */

    if ( IIcx_icas_xn_main_cb->num_free_icas_xn_cbs ==
         IIcx_icas_xn_main_cb->max_free_icas_xn_cbs )
    {

        /* UNLOCK the ICAS XN MAIN CB. CHECK !!! */
        db_status = IICXfree_cb( cx_cb_p );
        if (DB_FAILURE_MACRO( db_status ))
            return( db_status );
    }
    else
    {    
        IIcx_icas_xn_main_cb->num_free_icas_xn_cbs++;
        if (cx_cb_p->cx_prev != NULL)
            cx_cb_p->cx_prev->cx_next = cx_cb_p->cx_next;
        else
            IIcx_icas_xn_main_cb->icas_xn_list = cx_cb_p->cx_next;
        if (cx_cb_p->cx_next != NULL)
            cx_cb_p->cx_next->cx_prev = cx_cb_p->cx_prev;

        if (IIcx_icas_xn_main_cb->icas_xn_cb_free_list != NULL) 
	{
            IIcx_icas_xn_main_cb->icas_xn_cb_free_list->cx_prev = 
                                                   cx_cb_p;
	}
        cx_cb_p->cx_next = IIcx_icas_xn_main_cb->icas_xn_cb_free_list;
        cx_cb_p->cx_prev = NULL;
        IIcx_icas_xn_main_cb->icas_xn_cb_free_list = cx_cb_p;
        
    }

    return( E_DB_OK );

} /* IICXdelete_icas_xn_cb */

/*
**   Name: IICXfree_icas_xn_cb()
**
**   Description: 
**        The ICAS XN control block is physically deleted.
**
**   Outputs:
**       Returns: Pointer to the CX CB, if properly created. 
**                NULL if there was an error.
**
**   History:
**	30-Sep-1993 (mhughes)
**	    Modified XA_XN_THRD routines for new ICAS_XN cb
*/

DB_STATUS
IICXfree_icas_xn_cb(
            IICX_ICAS_XN_CB       *icas_xn_cb_p    /* IN */
           )
{
    STATUS             cl_status;
    char               ebuf[20];

    if ((cl_status = MEfree( (PTR)icas_xn_cb_p )) != OK)
    {
      CVna((i4)cl_status, ebuf);
      IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                     ERx("IIcx_icas_xn_cb"));
      return( E_DB_FATAL );
    }

    return( E_DB_OK );

} /* IICXfree_icas_xn_cb */
