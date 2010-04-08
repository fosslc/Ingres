/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <cv.h>
# include	<er.h>
# include       <me.h>
# include       <si.h>			/* needed for iicxfe.h */
# include	<st.h>
# include       <iicx.h>
# include       <iicxfe.h>
# include       <iicxtx.h>
# include       <iicxxa.h>
# include       <iicxinit.h>
# include       <iicxutil.h>
# include       <generr.h>
# include       <ercx.h>

/**
**  Name: iicxutil.c - Has a number of utility routines used throughout the
**                     IICX sub-component.
**
**  Description:
**	
**  Defines:
**
**      IICXlock_insert_cb()    -  lock a CX CB, and insert into list/hash tbl.
**      IICXunlock_remove_cb()  -  unlock a CX CB, remove from a list/hash tbl.
**      IICXlink_to_parents()   -  link a CX CB to all of its parents.
**      IICXdelink_from_parents() - delink a CX CB from it's parents.
**
**      IICXget_new_cb()        -  gets a new CB.
**      IICXget_new_cx_cb()     -  gets or creates a new CX CB.
**
**	IICXfree_cb()           -  physical deletion of a CB.
**      IICXfree_all_cbs()      -  physical deletion of all the CBs in a list.
**      IICXfree_sub_cb()       -  physical deletion of a SUB CB.
**
**      IICXadd_list_cb()       -  init and add a new LIST CB element.
**      IICXfind_list_cb()      -  find a LIST CB element based on an ID.
**      IICXdelete_list_cb()    -  logically delete a LIST CB element. 
**      IICXdelete_all_list_cbs() - logically delete a set of LIST CB elements.
**      IICXfree_all_list_cbs() -  free all the LIST CBs in a list.
**
**      IICXerror               -  log/report error in CX routines.
**      IICXassign_id           -  assign a CX ID to another.
**      IICXformat_id           -  format a CX ID into ascii/hex format.
**      IICXcompare_id          -  compares to CX IDs. Returns 0 if equal.     
**      
**  Notes:
**	<Specific notes and examples>
**
**  History:
**	25-sep-1992	- First written (mani)
**	23-nov-1992 (larrym)
**	    added er.h for ERx macro, and st.h for ST stuff.
**	07-apr-1993 (larrym,mani)
**	    modified tracking of thread association to transaction branch
**	04-may-93 (vijay)
**	    initialize auto variable db_status. 
**	05-may-93 (vijay)
**	    Fix a bunch more uninitialized variables.
**      07-Oct-93 (mhughes)
**          Added support for new tuxedo cb types
**      30-May-96 (stial01)
**          IICXassign_id() copy server_name and server_flag (B75073)
**      25-Sep-98 (thoda04)
**          Don't reference linked-list pointers in block after block is freed
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/



/*
**
**   Routine to lock a list head/hash table bucket slot, and insert a CX
**   CB into the list/hash bucket.
**
*/

/*
**   Name: IICXlock_insert_cb()
**
**   Description: 
**        Lock a CX CB (or the list head/hash bucket slot as appropriate)
**        and insert the CX CB into the list/hash table.
**
**   Inputs:
**        cx_cb_p  -  pointer to a CX CB. It already has the CX ID field
**                    in it, initialized.
**
**   Outputs:
**       Returns: the cx_cb_p is LOCKED if everything goes well.
**
**   History:
**      05-Oct-1992 - First written (mani)
**      07-Oct-93 (mhughes)
**          Added support for new tuxedo cb types
*/

DB_STATUS  
IICXlock_insert_cb(IICX_CB  *cx_cb_p)
{
    DB_STATUS    db_status=E_DB_OK;
    char         ebuf[20];

    cx_cb_p->cx_prev = NULL;
    
    switch( cx_cb_p->cx_id.cx_type )
    {
       case  IICX_FE_THREAD:  /* Lock the FE THREAD MAIN CB */
                              cx_cb_p->cx_next = 
                                IIcx_fe_thread_main_cb->fe_thread_list;
                              if (cx_cb_p->cx_next != NULL)
      		              {
                                  cx_cb_p->cx_next->cx_prev = cx_cb_p;
			      }
                              IIcx_fe_thread_main_cb->fe_thread_list =
                                  cx_cb_p;
                              break;

       case  IICX_XA_XN_THRD: /* Lock the XA XN MAIN CB */
                              cx_cb_p->cx_next = 
                                IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_list;
                              if (cx_cb_p->cx_next != NULL)
      		              {
                                  cx_cb_p->cx_next->cx_prev = cx_cb_p;
			      }
                              IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_list =
                                  cx_cb_p;
                              IIcx_xa_xn_thrd_main_cb->num_known_xids++;
                              break;

       case  IICX_XA_XN    :  /* Lock the XA XN MAIN CB */
                              cx_cb_p->cx_next = 
                                IIcx_xa_xn_main_cb->xa_xn_list;
                              if (cx_cb_p->cx_next != NULL)
      		              {
                                  cx_cb_p->cx_next->cx_prev = cx_cb_p;
			      }
                              IIcx_xa_xn_main_cb->xa_xn_list =
                                  cx_cb_p;
                              break;

       case  IICX_XA_RMI   :  /* Lock the XA RMI MAIN CB */
                              cx_cb_p->cx_next = 
                                IIcx_xa_rmi_main_cb->xa_rmi_list;
                              if (cx_cb_p->cx_next != NULL)
      		              {
                                  cx_cb_p->cx_next->cx_prev = cx_cb_p;
			      }
                              IIcx_xa_rmi_main_cb->xa_rmi_list =
                                  cx_cb_p;
                              break;

       case  IICX_ICAS_XN:    /* Lock the ICAS XN MAIN CB */
                              cx_cb_p->cx_next = 
                                IIcx_icas_xn_main_cb->icas_xn_list;
                              if (cx_cb_p->cx_next != NULL)
      		              {
                                  cx_cb_p->cx_next->cx_prev = cx_cb_p;
			      }
                              IIcx_icas_xn_main_cb->icas_xn_list =
                                  cx_cb_p;
                              IIcx_icas_xn_main_cb->num_active_xids++;
                              break;

       default             :  CVna((i4)cx_cb_p->cx_id.cx_type, ebuf);
                              IICXerror(GE_LOGICAL_ERROR, 
                                   E_CX0005_BAD_CX_TYPE, 2, ebuf, 
                                   ERx("lock_insert_cb"));
                              db_status = E_DB_FATAL;
                              break;
     }
     
     if (DB_FAILURE_MACRO( db_status ))
        return(db_status);

     return( E_DB_OK );

} /* IICXlock_insert_cb */


/*
**   Name: IICXunlock_remove_cb()
**
**   Description: 
**        Unlock a CX CB after removing it from a list/hash table. 
**
**   Inputs:
**        cx_cb_p  -  pointer to a CX CB. It already has the CX ID field
**                    in it, initialized.
**
**   Outputs:
**
**   History:
**       12-Oct-1992 - First written (mani)
**       07-Oct-93 (mhughes)
**          Added support for new tuxedo cb types
*/

DB_STATUS  
IICXunlock_remove_cb(IICX_CB  *cx_cb_p)
{
    DB_STATUS    db_status=E_DB_OK;
    IICX_CB      *tmp_cx_cb;
    char         ebuf[20];
    
    switch( cx_cb_p->cx_id.cx_type )
    {
       case  IICX_FE_THREAD:  if (cx_cb_p->cx_prev != NULL)
                                  cx_cb_p->cx_prev->cx_next = cx_cb_p->cx_next;
                              else
                                  IIcx_fe_thread_main_cb->fe_thread_list =
                                              cx_cb_p->cx_next;
                              if (cx_cb_p->cx_next != NULL)
                                  cx_cb_p->cx_next->cx_prev = cx_cb_p->cx_prev;
                              
                              /* Unlock the FE Thread Main CB */
                              db_status = IICXfree_cb( cx_cb_p );
                              break;

       case  IICX_XA_XN_THRD: db_status = IICXdelete_xa_xn_thrd_cb( cx_cb_p );
                              IIcx_xa_xn_thrd_main_cb->num_known_xids--;
                              break;

       case  IICX_XA_XN    :  db_status = IICXdelete_xa_xn_cb( cx_cb_p );
                              break;

       case  IICX_XA_RMI   :  if (cx_cb_p->cx_prev != NULL)
                                  cx_cb_p->cx_prev->cx_next = cx_cb_p->cx_next;
                              else
                                  IIcx_xa_rmi_main_cb->xa_rmi_list =
                                              cx_cb_p->cx_next;
                              if (cx_cb_p->cx_next != NULL)
                                  cx_cb_p->cx_next->cx_prev = cx_cb_p->cx_prev;
                              db_status = IICXfree_cb( cx_cb_p );
                              break;

       case  IICX_ICAS_XN:    db_status = IICXdelete_icas_xn_cb( cx_cb_p );
                              IIcx_icas_xn_main_cb->num_active_xids--;
                              break;

       default             :  CVna((i4)cx_cb_p->cx_id.cx_type, ebuf);
                              IICXerror(GE_LOGICAL_ERROR, 
                                   E_CX0005_BAD_CX_TYPE, 2, ebuf, 
                                   ERx("unlock_remove_cb"));
                              db_status = E_DB_FATAL;
                              break;
     }
     
     if (DB_FAILURE_MACRO( db_status ))
         return(db_status);

     return( E_DB_OK );

} /* IICXunlock_remove_cb */





/*
**   Name: IICXlink_to_parents()
**
**   Description:
**        Link a CX CB to it's parents.
**
**   Inputs:
**        cx_cb_p  -   pointer to the CX CB.
**
**   Outputs:
**
**   History:
**       12-Oct-1992 - First written (mani)
*/

DB_STATUS  
IICXlink_to_parents(IICX_CB      *cx_cb_p)
{
    DB_STATUS      db_status=E_DB_OK;
    char           ebuf[20];

    switch( cx_cb_p->cx_id.cx_type )
    {
      case     IICX_XA_RMI:
      case     IICX_FE_THREAD:
      case     IICX_XA_XN:
      case     IICX_XA_XN_THRD:
      case     IICX_ICAS_XN:
                             break;

      default             :  CVna((i4)cx_cb_p->cx_id.cx_type, ebuf);
                             IICXerror(GE_LOGICAL_ERROR, 
                                   E_CX0005_BAD_CX_TYPE, 2, ebuf, 
                                   ERx("link_to_parents"));
                             db_status = E_DB_FATAL;
                             break;
     }
     
     if (DB_FAILURE_MACRO( db_status ))
        return(db_status);

     return( E_DB_OK );

} /* IICXlink_to_parents */





/*
**   Name: IICXdelink_from_parents()
**
**   Description:
**        Delink a CX CB from it's parents.
**
**   Inputs:
**        cx_cb_p  -   pointer to the CX CB.
**
**   Outputs:
**
**   History:
**       12-Oct-1992 - First written (mani)
*/

DB_STATUS  
IICXdelink_from_parents(IICX_CB      *cx_cb_p)
{
    DB_STATUS    db_status=E_DB_OK;
    char         ebuf[20];
    
    switch( cx_cb_p->cx_id.cx_type )
    {
      case     IICX_XA_RMI:
      case     IICX_FE_THREAD:
      case     IICX_XA_XN:
      case     IICX_XA_XN_THRD:
      case     IICX_ICAS_XN:
                             break;
      default             :  CVna((i4)cx_cb_p->cx_id.cx_type, ebuf);
                              IICXerror(GE_LOGICAL_ERROR, 
                                   E_CX0005_BAD_CX_TYPE, 2, ebuf, 
                                   ERx("delink_from_parents"));
                              db_status = E_DB_FATAL;
                              break;
     }
     
     if (DB_FAILURE_MACRO( db_status ))
        return(db_status);

     return( E_DB_OK );

} /* IICXdelink_from_parents */





/*
**
**   Routine to get a new CB from a free/reserv list, or to create the CB.
**
*/

/*
**   Name: IICXget_new_cb()
**
**   Description: 
**        Get a new CB from a free/reserv list for this CX type. Create a
**        CB if necessary. If an [optional] pointer to a SUB CB was specified,
**        assign that to the CX CB, else, attach a default SUB CB to the
**        CX CB.
**
**   Inputs:
**        cx_type      -  type of CB.
**        cx_sub_cb_p  -  if non-NULL, is a pointer to the SUB CB.
**
**   Outputs:
**       Returns: cx_cb_p_p - address of a pointer to a CX CB.
**
**   History:
**       5-Oct-1992 - First written (mani)
**      07-Oct-93 (mhughes)
**          Added support for new tuxedo cb types
*/

DB_STATUS  
IICXget_new_cb(IICX_ID        *cx_id,
               IICX_U_SUB_CB  *cx_sub_cb_p,
               IICX_CB        **cx_cb_p_p)
{
    DB_STATUS    db_status=E_DB_OK;
    char         ebuf[20];
    
    switch( cx_id->cx_type )
    {
       case  IICX_FE_THREAD:  db_status = IICXcreate_fe_thread_cb(
                                 cx_sub_cb_p, 
                                 cx_cb_p_p );
                              break;
       case  IICX_XA_XN_THRD:  db_status = IICXcreate_xa_xn_thrd_cb(
                                  cx_id,  
                                  cx_cb_p_p );
                              break;
       case  IICX_XA_XN    :  db_status = IICXcreate_xa_xn_cb(
                                  cx_id,  
                                  cx_sub_cb_p, 
                                  cx_cb_p_p );
                              break;
       case  IICX_XA_RMI   :  db_status = IICXcreate_xa_rmi_cb(
                                  cx_sub_cb_p, 
                                  cx_cb_p_p );
                              break;
       case  IICX_ICAS_XN:    db_status = IICXcreate_icas_xn_cb(
                                  cx_id,  
                                  cx_cb_p_p );
                              break;
       default             :  CVna((i4)cx_id->cx_type, ebuf);
                              IICXerror(GE_LOGICAL_ERROR, 
                                   E_CX0005_BAD_CX_TYPE, 2, ebuf, 
                                   ERx("get_new_cb"));
                              db_status = E_DB_FATAL;
                              break;
     }
     
     if (DB_FAILURE_MACRO( db_status ))
        return(db_status);

     db_status = IICXassign_id( cx_id, &((*cx_cb_p_p)->cx_id) );
     if (DB_FAILURE_MACRO( db_status ))
        return(db_status);

     return( E_DB_OK );

} /* IICXget_new_cb */





/*
**   Name: IICXget_new_cx_cb()
**
**   Description: 
**        Get a new CX CB from either the main free list, or create one.
**
**   Inputs:
**        cx_cb_p_p  -  address of pointer to CX CB.
**
**   Outputs:
**       Returns: A CX CB to use.
**
**   History:
**       5-Oct-1992 - First written (mani)
*/

DB_STATUS  
IICXget_new_cx_cb(IICX_CB      **cx_cb_p_p)
{
    DB_STATUS    db_status=E_DB_OK;
    STATUS       cl_status=OK;
    char         ebuf[20];

    /* LOCK the MAIN CB. CHECK !! */
    
    if (IIcx_main_cb->cb_free_list != NULL) {
        *cx_cb_p_p = IIcx_main_cb->cb_free_list;
        IIcx_main_cb->cb_free_list = (*cx_cb_p_p)->cx_next;
        
        /* UNLOCK the MAIN CB .. */
    }
    else
    {
        /* UNLOCK the MAIN CB */

        if ((*cx_cb_p_p = (IICX_CB *)MEreqmem(
              (u_i4)0, (u_i4)sizeof(IICX_CB), TRUE, 
              (STATUS *)&cl_status)) == NULL)
        {
          CVna((i4)cl_status, ebuf);
          IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                    ERx("IICX_CB"));
          return( E_DB_FATAL );
        }
        (*cx_cb_p_p)->cx_next = NULL;
        (*cx_cb_p_p)->cx_prev = NULL;
        (*cx_cb_p_p)->cx_state= IICX_DELETED;
        (*cx_cb_p_p)->cx_sub_cb.generic_cb_p = NULL;
    }

    return( E_DB_OK );

} /* IICXget_new_cx_cb */


/*
**
**   Routines that deal w/physical deletion of various CBs
**
*/


/*
**   Name: IICXfree_cb()
**
**   Description: 
**        Physically deletes a CB. If there is a sub_p that's non-NULL,
**        it physically deletes the SUB CB, along with all the info/lists
**        attached to the SUB CB.
**
**   Inputs:
**       cx_cb_p    -  pointer to a CB.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

DB_STATUS  
IICXfree_cb(IICX_CB    *cx_cb_p)
{
    char      ebuf[20];
    STATUS    cl_status=OK;
    DB_STATUS db_status=E_DB_OK;

    if (cx_cb_p == NULL)
    {
        IICXerror(GE_WARNING, E_CX0001_INTERNAL, 1, 
                     ERx("Trying to free NULL CB. Unexpected"));
        return( E_DB_WARN );
    }

    if (cx_cb_p->cx_sub_cb.generic_cb_p != NULL)
    {
        db_status = IICXfree_sub_cb( cx_cb_p->cx_id.cx_type,
                                     &(cx_cb_p->cx_sub_cb) );
        if (DB_FAILURE_MACRO(db_status))
            return( db_status );
    }


    if ((cl_status = MEfree( (PTR) cx_cb_p )) != OK)
    {
        CVna((i4)cl_status, ebuf);
        IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                   ERx("cx_cb_p"));
        return( E_DB_FATAL );
    }

    return( E_DB_OK );

} /* IICXfree_cb */




/*
**   Name: IICXfree_all_cbs()
**
**   Description: 
**        Physically deletes all the CBs in a list. If there is a sub_p
**        that's non-NULL for any of the CBs, it deletes the SUB CB, along
**        with all the info/lists attached to the SUB CB.
**
**   Inputs:
**       cx_cb_p    -  pointer to a CB list.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/

DB_STATUS
IICXfree_all_cbs(IICX_CB    *cx_cb_p)
{
    IICX_CB   *temp_cb;
    IICX_CB   *curr_cb;
    DB_STATUS db_status=E_DB_OK;
   
    for (temp_cb = cx_cb_p; temp_cb != NULL;) 
    {         
         curr_cb = temp_cb;          /* curr_cb  -> block to be freed */
         temp_cb = temp_cb->cx_next; /* temp_cb -> next blk in chain */
         db_status = IICXfree_cb( curr_cb );
         if (DB_FAILURE_MACRO(db_status))
	 {
            return( db_status );
         }
     }  /* end for() */
     return( E_DB_OK );

} /* IICX_free_all_cbs */




/*
**   Name: IICXfree_sub_cb()
**
**   Description: 
**        Physically deletes a SUB CB, along with all the info/lists
**        attached to the SUB CB.
**
**   Inputs:
**       cx_type     -  type of the SUB CB.
**       cx_sub_cb_p -  pointer to the SUB CB.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
**       07-Oct-93 (mhughes)
**          Added support for new tuxedo cb types
*/

DB_STATUS
IICXfree_sub_cb(i4              cx_type,
                IICX_U_SUB_CB    *cx_sub_cb_p)
{
    char      ebuf[20];
    STATUS    cl_status=OK;
    DB_STATUS db_status=E_DB_OK;

    if (cx_sub_cb_p == NULL)
    {
        IICXerror(GE_WARNING, E_CX0001_INTERNAL, 1, 
                     ERx("Trying to free NULL SUB CB. Unexpected"));
        return( E_DB_WARN );
    }

    switch( cx_type )
    {
       case  IICX_FE_THREAD:  db_status = IICXfree_fe_thread_cb(
                                  cx_sub_cb_p->fe_thread_cb_p );
                              break;
       case  IICX_XA_XN_THRD: db_status = IICXfree_xa_xn_thrd_cb(
                                  cx_sub_cb_p->xa_xn_thrd_cb_p );
                              break;
       case  IICX_XA_XN    :  db_status = IICXfree_xa_xn_cb(
                                  cx_sub_cb_p->xa_xn_cb_p );
                              break;
       case  IICX_XA_RMI   :  db_status = IICXfree_xa_rmi_sub_cb(
                                  cx_sub_cb_p->xa_rmi_cb_p );
                              break;
       case  IICX_ICAS_XN:    db_status = IICXfree_icas_xn_cb(
                                  cx_sub_cb_p->icas_xn_cb_p );
                              break;
       default             :  CVna((i4)cl_status, ebuf);
                              IICXerror(GE_LOGICAL_ERROR, 
                                   E_CX0005_BAD_CX_TYPE, 2, ebuf, 
                                   ERx("free_sub_cb_p"));
                              db_status = E_DB_FATAL;
                              break;
     }
     
     if (DB_FAILURE_MACRO( db_status ))
        return(db_status);

     return( E_DB_OK );

} /* IICX_free_sub_cb */




/*
**
**   Routines that deal w/manipulating LIST CBs
**
*/

/*
**   Name: IICXadd_list_cb()
**
**   Description: 
**        Adds a list CB element, if it's not already present.
**
**   Inputs:
**       cx_id       -  CX ID to add, if it's not already in the list.
**       cx_list_cb_head -  pointer to the head of the list of LIST CBs.
**                          updated if necessary.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/
DB_STATUS
IICXadd_list_cb(IICX_ID        *cx_id,
                IICX_LIST_CB   **cx_list_cb_head)
{
    char           ebuf[20];
    char           ebuf1[IICX_ID_ST_SIZE_MAX];
    STATUS         cl_status=OK;
    DB_STATUS      db_status=E_DB_OK;
    IICX_LIST_CB   *temp_list_cb = NULL;

    if (*cx_list_cb_head != NULL)
    {
        db_status = IICXfind_list_cb(cx_id, *cx_list_cb_head, &temp_list_cb );
        if (DB_FAILURE_MACRO(db_status))
            return( db_status );
        if (temp_list_cb != NULL)
        {
            CVna( cx_id -> cx_type, ebuf );
            IICXformat_id(cx_id, ebuf1);
            IICXerror(GE_WARNING, E_CX0006_DUP_CX_ID, 3, ebuf, 
                      ERx("add_list_cb."),
                      ebuf1);
            return( E_DB_ERROR );
        }
    }

    /* Lock the cx_main_cb here. CHECK ! */

    if (IIcx_main_cb->list_cb_free_list != NULL)
    { 
        temp_list_cb = IIcx_main_cb->list_cb_free_list;
        IIcx_main_cb->list_cb_free_list = 
                IIcx_main_cb->list_cb_free_list->cx_next;
    }

    /* Unlock the cx_main_cb here. CHECK ! */

    if (temp_list_cb == NULL) 
    {
       if ((temp_list_cb = (IICX_LIST_CB *)MEreqmem(
                 (u_i4)0, (u_i4)sizeof(IICX_LIST_CB), TRUE, 
                 (STATUS *)&cl_status)) == NULL)
       {
           CVna((i4)cl_status, ebuf);
           IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                   ERx("cx_list_cb element"));
           return( E_DB_FATAL );
        }
    }

    /* assign the CX ID into the LIST CB */
    IICXassign_id( cx_id, &temp_list_cb->cx_id );

    if (*cx_list_cb_head != NULL)
    {
        temp_list_cb->cx_next = *cx_list_cb_head;
    }
    else
    {
        temp_list_cb->cx_next = NULL;
    }

    *cx_list_cb_head     = temp_list_cb;

    return( E_DB_OK );        
        
} /* IICX_add_list_cb */




/*
**   Name: IICXfind_list_cb()
**
**   Description: 
**        Finds a list CB element, if it's present in the list. It does a
**        compare of the input CX_ID against the CX_ID's in the list.
**
**   Inputs:
**       cx_id       -  CX ID to add, if it's not already in the list.
**       cx_list_cb_head -  pointer to the head of the list of LIST CBs.
**       cx_list_cb_p_p  - pointer to pointer to LIST CB element. Is set to
**                         NULL if LIST CB element was not found.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**                cx_list_cb_p_p is set to NULL if nothing found.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/
DB_STATUS
IICXfind_list_cb(IICX_ID        *cx_id,
                 IICX_LIST_CB    *cx_list_cb_head,
                 IICX_LIST_CB    **cx_list_cb_p_p)
{

    for (*cx_list_cb_p_p = cx_list_cb_head; 
         *cx_list_cb_p_p != NULL; 
         *cx_list_cb_p_p = (*cx_list_cb_p_p)->cx_next)
    {         
         if( !IICXcompare_id( cx_id,
                              &((*cx_list_cb_p_p)->cx_id)))
             break;
     }

     return( E_DB_OK );

} /* IICX_find_list_cb */




/*
**   Name: IICXdelete_list_cb()
**
**   Description: 
**        Deletes a list CB element. Places it in a free list of LIST CBs.
**
**   Inputs:
**       cx_id       -  CX ID to delete.
**       cx_list_cb_head -  pointer to the head of the list of LIST CBs.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/
DB_STATUS
IICXdelete_list_cb(IICX_ID        *cx_id,
                   IICX_LIST_CB   **cx_list_cb_head)
{
    IICX_LIST_CB  *prev_list_cb = NULL;
    IICX_LIST_CB  *del_list_cb  = *cx_list_cb_head;

    while (del_list_cb != NULL)
    {
       if( !IICXcompare_id( cx_id, &(del_list_cb->cx_id)))
            break;
       prev_list_cb = del_list_cb;
       del_list_cb  = del_list_cb->cx_next;
    }

    if (del_list_cb != NULL)
    {
        if (prev_list_cb != NULL && prev_list_cb != *cx_list_cb_head)
	{
            prev_list_cb->cx_next = del_list_cb->cx_next;
	}
        else *cx_list_cb_head = del_list_cb->cx_next;

        
        /* stash away the LIST CB in a free list */
   
        /* LOCK the main CB. CHECK ! */
        
            del_list_cb->cx_next =  IIcx_main_cb -> list_cb_free_list;
            IIcx_main_cb -> list_cb_free_list = del_list_cb;
            IIcx_main_cb -> num_free_list_cbs++;

        /* UNLOCK the main CB. CHECK ! */
    }

    return( E_DB_OK );
     
} /* IICX_delete_list_cb */



/*
**   Name: IICXfree_all_list_cbs()
**
**   Description: 
**        Free all the LIST CB elements.
**
**   Inputs:
**       cx_list_cb_head -  pointer to the head of the list of LIST CBs.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if successful.
**
**   History:
**       25-Aug-1992 - First written (mani)
*/
DB_STATUS
IICXfree_all_list_cbs(IICX_LIST_CB   *cx_list_cb_head)
{
    char          ebuf[20];
    STATUS        cl_status=OK;
    IICX_LIST_CB  *temp_list_cb;
    IICX_LIST_CB  *curr_list_cb;

    for (temp_list_cb = cx_list_cb_head;
         temp_list_cb != NULL;)
    {
         curr_list_cb = temp_list_cb;          /* -> blk to be freed   */
         temp_list_cb = temp_list_cb->cx_next; /* -> next blk in chain */
         if ((cl_status = MEfree( (PTR) curr_list_cb )) != OK)
         {
             CVna((i4)cl_status, ebuf);
             IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                   ERx("temp_list_cb"));
             return( E_DB_FATAL );
         }
    }

    return( E_DB_OK );

} /* IICXfree_all_list_cbs */




/*
**
**   Routines that deal w/various CX IDs. i.e. assign to/compare/format CX IDs.
**
*/

/*
**   Name: IICXassign_id()
**
**   Description: 
**       Assigns the CX ID from source to destination.
**
**   Inputs:
**       src_cx_id_p      - pointer to the source CX ID.
**       dest_cx_id_p     - pointer to the dest   CX ID.
**
**   Outputs:
**       Returns: the dest CX ID initialized.
**
**   History:
**       02-Oct-1992 - First written (mani)
**       07-Oct-93 (mhughes)
**          Added support for new tuxedo cb types
**       26-Nov-93 (mhughes)
**	    Missing break in icas_xn_id block.
**       30-May-96 (stial01)
**          IICXassign_id() copy server_name and server_flag (B75073)
*/
DB_STATUS
IICXassign_id(IICX_ID    *src_cx_id_p,
              IICX_ID    *dest_cx_id_p)
{
   
    char      ebuf[20];
    STATUS    cl_status=OK;
    DB_STATUS db_status = E_DB_OK;

    if (src_cx_id_p == NULL || dest_cx_id_p == NULL)
    {
        IICXerror(GE_WARNING, E_CX0001_INTERNAL, 1, 
                     ERx("Trying to assign from/to NULL CX ID. Unexpected"));
        return( E_DB_FATAL );
    }

    dest_cx_id_p -> cx_type =  src_cx_id_p -> cx_type;

    switch( src_cx_id_p -> cx_type )
    {
       case  IICX_FE_THREAD:  dest_cx_id_p->u_cx_id.fe_thread_id =   
                                  src_cx_id_p->u_cx_id.fe_thread_id; 
                              break;                                 
       case  IICX_XA_XN_THRD: dest_cx_id_p->u_cx_id.xa_xn_thrd_id.xid_p =
                                &(dest_cx_id_p->u_cx_id.xa_xn_thrd_id.xid);
                              db_status = IICXassign_xa_xid(
                                   src_cx_id_p->u_cx_id.xa_xn_thrd_id.xid_p,
                                   dest_cx_id_p->u_cx_id.xa_xn_thrd_id.xid_p); 
                              break;                                 
       case  IICX_XA_XN    :  dest_cx_id_p->u_cx_id.xa_xn_id.xid_p =
                                &(dest_cx_id_p->u_cx_id.xa_xn_id.xid);
                              dest_cx_id_p->u_cx_id.xa_xn_id.rmid =
                                   src_cx_id_p->u_cx_id.xa_xn_id.rmid;
			      dest_cx_id_p->u_cx_id.xa_xn_id.server_flag = 
				   src_cx_id_p->u_cx_id.xa_xn_id.server_flag;
			      MEcopy(src_cx_id_p->u_cx_id.xa_xn_id.server_name,
				    DB_MAXNAME,
				    dest_cx_id_p->u_cx_id.xa_xn_id.server_name);
                              db_status = IICXassign_xa_xid(
                                   src_cx_id_p->u_cx_id.xa_xn_id.xid_p,
                                   dest_cx_id_p->u_cx_id.xa_xn_id.xid_p); 
                              break;                                 
       case  IICX_XA_RMI   :  dest_cx_id_p->u_cx_id.xa_rmi_id =   
                                  src_cx_id_p->u_cx_id.xa_rmi_id; 
                              break;                                 
       case  IICX_ICAS_XN:    dest_cx_id_p->u_cx_id.icas_xn_id.xid_p =
                                &(dest_cx_id_p->u_cx_id.icas_xn_id.xid);
                              db_status = IICXassign_xa_xid(
                                   src_cx_id_p->u_cx_id.icas_xn_id.xid_p,
                                   dest_cx_id_p->u_cx_id.icas_xn_id.xid_p); 
	                      break;
       case  IICX_ICAS_SVN_LIST:
	   		      dest_cx_id_p->u_cx_id.icas_svn_list_id.SVN =
				src_cx_id_p->u_cx_id.icas_svn_list_id.SVN;
                              break;
       default             :  CVna((i4)src_cx_id_p->cx_type, ebuf);
                              IICXerror(GE_LOGICAL_ERROR, 
                                   E_CX0005_BAD_CX_TYPE, 2, ebuf, 
                                   ERx("assign_id"));
                              db_status = E_DB_FATAL;
                              break;
     }
     
     return( db_status );

} /* IICXassign_id */




/*
**   Name: IICXcompare_id()
**
**   Description: 
**       Compares the CX ID with another.
**
**   Inputs:
**       cx_id_1_p      - pointer to a CX ID.
**       cx_id_2_p      - pointer to another CX ID.
**
**   Outputs:
**       Returns: 0 if they are equal.
**
**   History:
**	02-Oct-1992 - First written (mani)
**	08-dec-1992 (mani and larrym)
**	    fixed compare logic for xa_xn's. 
**      07-Oct-93 (mhughes)
**          Added support for new tuxedo cb types
**      05-Nov-93 (mhughes)
**	    Added logic for icas_svn_list's.
*/
i4
IICXcompare_id(IICX_ID    *cx_id_1_p,
               IICX_ID    *cx_id_2_p)
{
    char     ebuf[20];

  
    if (cx_id_1_p -> cx_type != cx_id_2_p -> cx_type)
        return( 1 );

    switch(cx_id_1_p -> cx_type )
    {
       case  IICX_FE_THREAD:  return(
                               (cx_id_1_p->u_cx_id.fe_thread_id ==   
                                cx_id_2_p->u_cx_id.fe_thread_id)? 0 : 1); 
                              break;                                 

       case  IICX_XA_XN_THRD:  return( 
                                 IICXcompare_xa_xid(
                                   cx_id_1_p->u_cx_id.xa_xn_thrd_id.xid_p,
                                   cx_id_2_p->u_cx_id.xa_xn_thrd_id.xid_p));
                              break;                                 

       case  IICX_XA_XN    :  return( 
                                ((IICXcompare_xa_xid(
                                   cx_id_1_p->u_cx_id.xa_xn_id.xid_p,
                                   cx_id_2_p->u_cx_id.xa_xn_id.xid_p)) ||
                                ((cx_id_1_p->u_cx_id.xa_xn_id.rmid ==
                                 cx_id_2_p->u_cx_id.xa_xn_id.rmid) ? 0 : 1))); 
                              break;                                 

       case  IICX_XA_RMI   :  return(
                               (cx_id_1_p->u_cx_id.xa_rmi_id ==   
                                cx_id_2_p->u_cx_id.xa_rmi_id)? 0 : 1); 
                              break;                                 

       case  IICX_ICAS_XN:    return( 
                                 IICXcompare_xa_xid(
                                   cx_id_1_p->u_cx_id.icas_xn_id.xid_p,
                                   cx_id_2_p->u_cx_id.icas_xn_id.xid_p));

       case  IICX_ICAS_SVN_LIST:
	   		      return( 
                              ( cx_id_1_p->u_cx_id.icas_svn_list_id.SVN ==   
			        cx_id_2_p->u_cx_id.icas_svn_list_id.SVN ) ? 
				0 : 1); 
                              break;                                 

       default             :  CVna((i4)cx_id_1_p->cx_type, ebuf);
                              IICXerror(GE_LOGICAL_ERROR, 
                                   E_CX0005_BAD_CX_TYPE, 2, ebuf, 
                                   ERx("compare_id"));
                              break;
     }
     

} /* IICXcompare_id */




/*
**   Name: IICXformat_id()
**
**   Description: 
**       Formats the CX ID into the text buffer. Expects a non-NULL CX ID.
**       Expects a long-enough text buffer for output. 
**
**       Caller should ideally have used IICX_ID_ST_SIZE_MAX for the text
**       buffer size.
**
**   Inputs:
**       cx_id       - pointer to the CX ID.
**       text_buffer - pointer to a text buffer that will contain the text
**                     form of the ID.
**   Outputs:
**       Returns: the character buffer with text.
**
**   History:
**       02-Oct-1992 - First written (mani)
**       07-Oct-93 (mhughes)
**          Added support for new tuxedo cb types
*/
VOID
IICXformat_id(IICX_ID    *cx_id,
              char       *text_buffer)
{
    char      ebuf[20];
    char      *p = text_buffer;

    switch( cx_id -> cx_type )
    {
       case  IICX_FE_THREAD:  CVla(cx_id->u_cx_id.fe_thread_id,
                                   text_buffer);
                              break;                                 
       case  IICX_XA_XN_THRD: STcat(p," XA XID = ");
                              p+= STlength(p);
                              IICXformat_xa_xid(
                                   cx_id->u_cx_id.xa_xn_thrd_id.xid_p, p);
                              break;                                 
       case  IICX_XA_XN    :  STcat(p,"rmid = ");
                              p += STlength(p);
                              CVna(cx_id->u_cx_id.xa_xn_id.rmid, p);
                              STcat(p," XA XID = ");
                              p+= STlength(p);
                              IICXformat_xa_xid(
                                   cx_id->u_cx_id.xa_xn_id.xid_p, p);
                              break;                                 
       case  IICX_XA_RMI   :  CVna(cx_id->u_cx_id.xa_rmi_id,  
                                   text_buffer); 
                              break;                                 
       case  IICX_ICAS_XN:     STcat(p," XA XID = ");
                              p+= STlength(p);
                              IICXformat_xa_xid(
                                   cx_id->u_cx_id.icas_xn_id.xid_p, p);
                              break;                                 
       default             :  CVna((i4)cx_id->cx_type, ebuf);
                              IICXerror(GE_LOGICAL_ERROR, 
                                   E_CX0005_BAD_CX_TYPE, 2, ebuf, 
                                   ERx("format_id"));
                              break;
     }

} /* IICXformat_id */




/*
**
**   Routines that deal w/error handling in the IICX sub-component.
**
*/

/*
**   Name: IICXerror()
**
**   Description: 
**       Writes/reports errors in the CX subcomponent.
**
**   Inputs:
**       Variable number of arguments for writing to error log.
**
**   Outputs:
**       Returns: Nothing.
**
**   History:
**       02-Oct-1992 - First written (mani)
**	18-jan-1993 (larrym)
**	    modified to call IIlocerr
*/
VOID
IICXerror(i4   generic_errno,
          i4   iicx_errno,
          i4        err_param_count,
          PTR       err_param1,
          PTR       err_param2,
          PTR       err_param3,
          PTR       err_param4,
          PTR       err_param5)
{

#define IICX_ERR 0		/* temp */

    IIlocerr(generic_errno, iicx_errno, IICX_ERR, err_param_count, err_param1,
		err_param2, err_param3, err_param4, err_param5 );

} /* IICXerror */
