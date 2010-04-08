
/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <er.h>	
# include       <cv.h>
# include	<si.h>		/* needed for iicxfe.h */
# include       <me.h>
# include       <iicx.h>
# include       <iicxfe.h>
# include       <generr.h>
# include       <ercx.h>

/**
**  Name: iicxfe.c - Has the routines for create/delete/find/manipulation
**                   of the FE THREAD CB.
**
**  IICXcreate_fe_thread_cb  - get an FE THREAD CB from a freelist, or
**                             create it.
**  IICXget_lock_fe_thread_cb- get and lock the current thread CB.
**  IICXfree_fe_thread_cb    - physically delete an FE THREAD CB.
**
**
**  Description:
**	
**  Defines:
**      
**  Notes:
**	<Specific notes and examples>
-*
**  History:
**	12-Oct-1992	- First written (mani)
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**   Name: IICXcreate_fe_thread_cb()
**
**   Description: 
**        The FE THREAD control block is obtained from a free list, or else
**        created and initialized with default values. 
**
**   Outputs:
**       Returns: Pointer to the CX CB, if properly created. 
**                NULL if there was an error.
**
**   History:
**	25-Aug-1992 - First written (mani)
**	23-nov-1992 (larrym)
**	    Added <er.h> include for ERx macro.
**	25-feb-1993 (larrym)
**	    Added IICXget_fe_thread_cb (no locking)
**	23-Aug-1993 (larrym)
**	    Changed fe_thread_cb_p->curr_xa_xn_cx_cb_p to
**	    fe_thread_cb_p->curr_xa_xn_thrd_cx_cb_p.  We now have the fe
**	    thread control block point to the xa_xn_thrd cb so that we
**	    can track overall transaction association to a thread with
**	    out regard to rmi.
*/

DB_STATUS
IICXcreate_fe_thread_cb(
            IICX_U_SUB_CB       *fe_thread_sub_cb_p,   /* IN [optional] */
            IICX_CB             **cx_cb_p_p     /* OUT */
           )
{
    DB_STATUS           db_status;
    STATUS              cl_status;
    IICX_FE_THREAD_CB   *fe_thread_cb_p;
    char                ebuf[20];

    /* No free lists for FE THREAD CBs. Hence create one if appropriate */
    db_status = IICXget_new_cx_cb( cx_cb_p_p );
    if (DB_FAILURE_MACRO(db_status))
    {
        return(db_status);
    }

    if (fe_thread_sub_cb_p == NULL) 
    {
        if ((fe_thread_cb_p = (IICX_FE_THREAD_CB *)MEreqmem((u_i4)0, 
                              (u_i4)sizeof(IICX_FE_THREAD_CB),
                              TRUE, (STATUS *)&cl_status)) == NULL)
        {
            CVna((i4)cl_status, ebuf);
            IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                      ERx("IICX_FE_THREAD_CB"));
            return( E_DB_FATAL );
        }
    }
    else fe_thread_cb_p = fe_thread_sub_cb_p->fe_thread_cb_p;

    fe_thread_cb_p->ingres_state = IICX_XA_T_ING_IDLE; 
    fe_thread_cb_p->rec_scan_in_progress = FALSE;
    fe_thread_cb_p->curr_xa_xn_thrd_cx_cb_p = NULL;

    (*cx_cb_p_p)->cx_sub_cb.fe_thread_cb_p = fe_thread_cb_p;

    return( E_DB_OK );

} /* IICXcreate_fe_thread_cb */




/*
**   Name: IICXget_fe_thread_cb()
**
**   Description: 
**        The FE THREAD control block for this thread is retrieved.
**
**   Inputs:
**      thread_key              - key to the thread desired - may be NULL
**
**   Outputs:
**      cx_cb_p_p               - pointer to the Thread CB, if properly
**                                retrieved.
**      DB_STATUS (return)      - success or failure
**
**   History:
**	25-feb-1993 (larrym)
**	    written.
*/
DB_STATUS
IICXget_fe_thread_cb(
            PTR                     thread_key,           /* IN, may be NULL */
            IICX_CB                 **fe_thread_cx_cb_p_p /* INOUT */
           )
{
   if (IIcx_fe_thread_main_cb->fe_thread_list == NULL)
   {
      IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1,
            ERx("The main FE thread list is NULL."));
      return( E_DB_ERROR );
   }

   *fe_thread_cx_cb_p_p = IIcx_fe_thread_main_cb->fe_thread_list;

   return( E_DB_OK );
}

/*
**   Name: IICXget_lock_fe_thread_cb()
**
**   Description: 
**        The FE THREAD control block for this thread is retrieved, and
**        locked.
**
**   Inputs:
**      thread_key              - key to the thread desired - may be NULL
**
**   Outputs:
**      cx_cb_p_p               - pointer to the Thread CB, if properly
**                                retrieved.
**      DB_STATUS (return)      - success or failure
**
**   History:
**       9-Oct-1992 - First written (mani)
*/

DB_STATUS
IICXget_lock_fe_thread_cb(
            PTR                     thread_key,           /* IN, may be NULL */
            IICX_CB                 **fe_thread_cx_cb_p_p /* INOUT */
           )
{
   DB_STATUS            db_status;

    /* LOCK FE THREAD MAIN CB ! CHECK !!! */

    db_status = IICXget_fe_thread_cb( thread_key, fe_thread_cx_cb_p_p );

    if (DB_FAILURE_MACRO( db_status ))
	return( db_status );

    IICXlock_cb( *fe_thread_cx_cb_p_p );

    /* UNLOCK FE THREAD MAIN CB ! CHECK !!! */

    return( E_DB_OK );

} /* IICXget_lock_fe_thread_cb */




/*
**   Name: IICXfree_fe_thread_cb()
**
**   Description: 
**        The FE THREAD control block is physically deleted.
**
**   Outputs:
**       Returns: 
**
**   History:
**       9-Oct-1992 - First written (mani)
*/

DB_STATUS
IICXfree_fe_thread_cb(
            IICX_FE_THREAD_CB       *fe_thread_cb_p    /* IN */
           )
{
    STATUS             cl_status;
    char               ebuf[20];

    if ((cl_status = MEfree( (PTR)fe_thread_cb_p )) != OK)
    {
      CVna((i4)cl_status, ebuf);
      IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                     ERx("IIcx_fe_thread_cb"));
      return( E_DB_FATAL );
    }

    return( E_DB_OK );

} /* IICXfree_fe_thread_cb */



