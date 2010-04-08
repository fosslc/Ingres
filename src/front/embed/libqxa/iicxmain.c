/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include       <cv.h>
# include       <er.h>
# include       <si.h>		/* needed for iicxfe.h */
# include       <st.h>
# include       <iicx.h>
# include       <iicxfe.h>
# include       <iicxtx.h>
# include       <iicxxa.h>
# include       <iicxutil.h>
# include       <generr.h>
# include       <ercx.h>

/**
**  Name: iicxmain.c - Has a number of utility routines used for manipulating
**                     the ids/structures for XA support in the IICX-component.
**
**  IICXcreate_lock_cb  - creates/inserts the CB, and places in a CB-specific
**                        hash table.
**  IICXfind_lock_cb    - finds and locks the CB.
**  IICXdelete_cb       - logically [and maybe physically] delete the CB.
**
**  Description:
**	
**  Defines:
**      
**  Notes:
**	<Specific notes and examples>
-*
**  History:
**	7-Oct-1992	- First written (mani)
**	23-nov-1992 (larrym)
**	    added er.h for ERx macro and st.h for ST stuff.
**	07-apr-93 (mani,larrym)
**	    modified tracking of thread association to transaction branch
**      07-Oct-93 (mhughes)
**          added support for new icas cb type
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


/*
**   Name: IICXcreate_lock_cb()
**
**   Description: 
**       Checks if the CB is already present in it's table/list. If yes, it
**       merely returns the CB, locked.
**
**       If not, a free CB is obtained from a free list [or created]. It is
**       initialized with the input CX ID, and inserted into the table/list.
**       It is returned, locked.
**
**   Inputs:
**       cx_id          - pointer to the CX ID.
**       cx_sub_cb_p    - [optional] pointer to the SUB CB.
**       cx_cb_p_p      - pointer to the CX CB that was found/created.
**
**   Outputs:
**       Returns: Nothing.
**
**   History:
**       05-Oct-1992 - First written (mani)
*/
DB_STATUS
IICXcreate_lock_cb(IICX_ID          *cx_id,        /* IN */
                   IICX_U_SUB_CB    *cx_sub_cb_p,  /* IN, optional */
                   IICX_CB          **cx_cb_p_p    /* OUT */
                  )
{
    DB_STATUS db_status;

    db_status = IICXfind_lock_cb(cx_id, cx_cb_p_p);
    if (DB_FAILURE_MACRO(db_status))
        return( db_status );

    if (*cx_cb_p_p != NULL)
    {
        return( E_DB_OK );       /* merely return the CB, locked, */
                                 /* if already present.           */
    }

    /* get a new CB, and assign the CX ID to the CB */
    db_status = IICXget_new_cb( cx_id, cx_sub_cb_p, cx_cb_p_p );
    if (DB_FAILURE_MACRO(db_status))
        return( db_status );

    (*cx_cb_p_p)->cx_state = IICX_ACTIVE;
    
    /* link the CB to any parent CBs. NO-OP for now !!! */
    db_status = IICXlink_to_parents( *cx_cb_p_p );
    if (DB_FAILURE_MACRO(db_status))
        return( db_status );    

    /* lock and insert the CB into the proper hash_table/linked_list */
    db_status = IICXlock_insert_cb( *cx_cb_p_p );
    if (DB_FAILURE_MACRO(db_status))
        return( db_status );

    return( E_DB_OK );

}  /* IICXcreate_lock_cb */



/*
**   Name: IICXfind_lock_cb()
**
**   Description: 
**       Finds the CB if present. If not present, returns NULL.
**
**   Inputs:
**       cx_id       - pointer to a CX ID.
**
**   Outputs:
**       cx_cb_p_p   - address of pointer to a CX CB. Non-null on return
**                     if the CX CB was found.
**   History:
**       05-Oct-1992 - First written (mani)
**       07-Oct-93   (mhughes)
**           Added support for new icas xn cb type
*/
DB_STATUS
IICXfind_lock_cb(IICX_ID  *cx_id,      /* IN */
                 IICX_CB  **cx_cb_p_p)
{
    char    ebuf[20];

    switch( cx_id->cx_type )
    {
       case  IICX_FE_THREAD :  /* Lock the FE THREAD MAIN CB */
                               *cx_cb_p_p = 
                                 IIcx_fe_thread_main_cb->fe_thread_list;
                               break;

       case  IICX_XA_XN     :  /* Lock the XA XN MAIN CB */
                               *cx_cb_p_p = 
                                 IIcx_xa_xn_main_cb->xa_xn_list;
                               break;

       case  IICX_XA_RMI    :  /* Lock the XA RMI MAIN CB */
                               *cx_cb_p_p = 
                                 IIcx_xa_rmi_main_cb->xa_rmi_list;
                               break;

       case  IICX_XA_XN_THRD:  /* Lock the XA XN THREAD MAIN CB */
                               *cx_cb_p_p = 
                                 IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_list;
                               break;

       case  IICX_ICAS_XN:     /* Lock the ICAS XN MAIN CB */
                               *cx_cb_p_p = 
                                 IIcx_icas_xn_main_cb->icas_xn_list;
                               break;

       default              :  CVna((i4)cx_id->cx_type, ebuf);
                               IICXerror(GE_LOGICAL_ERROR, 
                                   E_CX0005_BAD_CX_TYPE, 2, ebuf, 
                                   ERx("find_lock_cb"));
                               return( E_DB_FATAL );
                               break;
     }
     
     for (; 
          (*cx_cb_p_p != NULL); 
          (*cx_cb_p_p  = (*cx_cb_p_p)->cx_next))
     {         
         if( !IICXcompare_id( cx_id, &((*cx_cb_p_p)->cx_id)) )
             break;
     }

     return( E_DB_OK );

}  /* IICXfind_lock_cb */




/*
**   Name: IICXdelete_cb()
**
**   Description: 
**       Logically deletes the CB. Does not move the CB to a free list,
**       or physically delete the CB, until all of the sub-lists for the
**       CB (list CB's in its SUB CB) are NULL. 
**
**   Inputs:
**       cx_id          - pointer to the CX ID.
**
**   Outputs:
**       Returns: Nothing.
**
**   History:
**       05-Oct-1992 - First written (mani)
*/
DB_STATUS
IICXdelete_cb(IICX_ID        *cx_id)
{
    DB_STATUS db_status;
    IICX_CB   *cx_cb_p = NULL;

    db_status = IICXfind_lock_cb( cx_id, &cx_cb_p );
    if (DB_FAILURE_MACRO( db_status ))
        return( db_status );

    if (cx_cb_p != NULL) 
    {
        /* delink the CB from it's parent CBs. NO-OP for now !!! */
        db_status = IICXdelink_from_parents( cx_cb_p );
        if (DB_FAILURE_MACRO(db_status))
            return( db_status );    

        /* remove the CB from the list/hash table, if there are no    */
        /* elements in the lists associated with its SUB CB, i.e. all */
        /* the children are freed/removed.                            */
        /* the CB is placed in a free list, and the list/hash table is*/
        /* unlocked. */

        db_status = IICXunlock_remove_cb( cx_cb_p );
        if (DB_FAILURE_MACRO( db_status ))
            return( db_status );
    }

    return( E_DB_OK );

}  /* IICXdelete_cb */



























