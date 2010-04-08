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
# include       <si.h>			/* needed for iicxfe.h */
# include       <iicx.h>
# include       <iicxxa.h>
# include       <iicxutil.h>
# include	<iicxfe.h>
# include       <iixautil.h>
# include       <generr.h>
# include       <ercx.h>

/* Structures that are shared across modules */
    EXEC SQL INCLUDE SQLCA;
/**
**  Name: iicxxa.sc - Has a number of utility routines used for manipulating
**                   the ids/structures for XA support in the IICX-component.
**
**  IICXcreate_xa_rmi_cb  - get an XA RMI CB from a freelist, or create it.
**  IICXcreate_xa_rmi_sub_cb - create and init an XA RMI SUB CB.
**  IICXfree_xa_rmi_sub_cb   - physically delete an XA RMI CB.
**
**  IICXcreate_xa_xn_cb  - get an XA XN CB, or create it.
**  IICXdelete_xa_xn_cb  - logically delete an XA XN CB.
**  IICXfree_xa_xn_cb    - physically delete an XA XN CB.
**
**  IICXcreate_xa_xn_thrd_cb  - get an XA XN THRD CB, or create it.
**  IICXdelete_xa_xn_thrd_cb  - logically delete an XA XN THRD CB.
**  IICXfree_xa_xn_thrd_cb    - physically delete an XA XN THRD CB.
**
**  IICXassign_xa_xid     - assign an XA XID from source to dest.
**  IICXcompare_xa_xid - compare two XA XIDs, return 0 if equal.
**  IICXconv_to_struct_xa_xid - converts an XA XID from text format to
**                       an XID structure format.
**  IICXformat_xa_xid  - format an XID, place in a text buffer for writing to
**                       a file, or sending over the network.
**  IICXformat_xa_extd_xid - format an the extended portion of an XID i.e.
**                           the branch_flag and branch_seqnum.
**  IICXvalidate_xa_xid - validate the XA XID for proper gtrid and bqual 
**                        lengths.
**
**  Description:
**	
**  Defines:
**      
**  Notes:
**	<Specific notes and examples>
-*
**  History:
**	05-Oct-1992	- First written (mani)
**	01-dec-1992 (larrym)
**	    code review bug fixing, mostly with structure initialization.
**	09-mar-1993 (larrym)
**	    renamed to iicxxa.sc to allow for preprocessing of ESQL statements.
**	25-mar-1993 (larrym)
**	    Added IICXcreate_xa_xn_thrd_cb, IICXdelete_xa_xn_thrd_cb, 
**	    and IICXfree_xa_xn_thrd_cb
**	15-jun-1993 (larrym)
**	    Modified the way we track transaction branch associations to a 
**	    thread.  The association state is now kept in the xa_xn_cb
**	    (i.e., one per xid,rmid pair) instead of in the xa_xn_thrd_cb.
**	    at the moment we have no real use for the xa_xn_thrd_cb, but
**	    I'm sure we will someday (if for no other reason than tracking
**	    debugging info).
**	06-jul-1993 (larrym)
**	    added EXEC SQL INCLUDE SQLCA to allow user app error handling
**	30-Jul-1993 (larrym)
**	    moved DB_XA_MAXGTRIDSIZE and DB_XA_MAXBQUALSIZE to iicommon.h
**	24-aug-1993 (larrym)
**	    temporarily re-added DB_XA_MAXGTRIDSIZE and DB_XA_MAXBQUALSIZE back
**	    until iicommon.h get's propigated to the front-ends.
**      23-sep-1993 (iyer)
**          Added a new function IICXformat_xa_extd_xid. This would
**          format the branch_seqnum and branch_flag into text and append
**          it to the end of xid_str. In addition the IICXconv_to_struct_xa_xid
**          has been changed to accept two additional parameters, branch_sequm
**	    and branch_flag. The values are extracted from xid_str and
**          returned in these arguments.
**	11-nov-1993 (larrym)
**	    Modified iicxxa.h: changed IICX_XA_RMI_CB:  changed connect_name
**	    to connect_name_p (it's a pointer now, not an array) and
**	    added dbname_p.
**	18-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**	    Change code to reflect changes to DB_XA_DIS_TRAN_ID in iicommon.h
**	    (i.e. first 3 fields changed from i4 to long).
**      30-may-1996 (stial01)
**          IICXcreate_xa_xn_cb: check server_name if requested (B75073)
**          Added IICXset_xa_xn_server()
**      12-aug-1996 (stial01)
**          IICXcreate_xa_xn_cb: check listen address in the XA XN CB(B78248)
**      18-sep-1998 (thoda04)
**          Use CVuahxl for integer conversion of XID data to avoid sign overflow.
**      05-oct-1998 (thoda04)
**          Process XID data bytes in groups of four, high order to low, bytes for all platforms.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-apr-1999 (hanch04)
**	    Change code to reflect changes to DB_XA_DIS_TRAN_ID in iicommon.h
**	    (i.e. first 3 fields changed from i4 to long) for LP64
**      21-aug-2002 (stial01)
**          After DISCONNECT check sqlca.sqlcode before ERRORNO
**      28-aug-2002 (stial01)
**          IICXdelete_xa_xn_thrd_cb() ALWAYS remove the cb from list before 
**          freeing the control block or adding to free list. (b108606)
*/

/* this can be removed for the september 93 integration */
#ifndef DB_XA_MAXGTRIDSIZE
#define DB_XA_MAXGTRIDSIZE	64
#define	DB_XA_MAXBQUALSIZE	64
#endif

/*
**   Name: IICXcreate_xa_rmi_cb()
**
**   Description: 
**        The XA RMI control block is obtained from a free list, or else
**        created and initialized with default values. 
**
**   Outputs:
**       Returns: Pointer to the CX CB, if properly created. 
**                NULL if there was an error.
**
**   History:
**	25-Aug-1992 - First written (mani)
**	24-nov-1992 (larrym)
**	    added er.h for ERx macro and st.h for ST stuff.  Also changed
**	    IICXget_rmi_by_static_sid to IICXget_rmid_by_static_sid.
**	02-dec-1992 (larrym)
**	    modified to let libq generate an internal (xa) sid.
**	09-mar-1993 (larrym)
**	    modified to use connection names.
*/

DB_STATUS
IICXcreate_xa_rmi_cb(
            IICX_U_SUB_CB    *xa_rmi_sub_cb_p,   /* IN [optional] */
            IICX_CB          **cx_cb_p_p     /* OUT */
           )
{
    DB_STATUS        db_status;
    STATUS           cl_status;
    char             ebuf[20];
    IICX_XA_RMI_CB   *xa_rmi_cb_p;

    /* No free lists for XA RMI CBs. Hence create one if appropriate */
    db_status = IICXget_new_cx_cb( cx_cb_p_p );
    if (DB_FAILURE_MACRO(db_status))
    {
        return(db_status);
    }

    if (xa_rmi_sub_cb_p != NULL) 
    {
        (*cx_cb_p_p)->cx_sub_cb.xa_rmi_cb_p =
                        xa_rmi_sub_cb_p->xa_rmi_cb_p;
    }
    else 
    {
        (*cx_cb_p_p)->cx_sub_cb.xa_rmi_cb_p = NULL;

        db_status = IICXcreate_xa_rmi_sub_cb( 
                        &((*cx_cb_p_p)->cx_sub_cb.xa_rmi_cb_p) );
        if (DB_FAILURE_MACRO( db_status ))
            return( db_status );

    }
    return( E_DB_OK );

} /* IICXcreate_xa_rmi_cb */


/*
**   Name: IICXcreate_xa_rmi_sub_cb()
**
**   Description: 
**        The XA RMI sub control block is created and initialized with
**        default values.
**
**   Outputs:
**       Returns: Pointer to the CX SUB CB, if properly created. 
**                NULL if there was an error.
**
**   History:
**       17-Nov-1992 - First written (mani)
**	11-nov-1993 (larrym)
**	    changed connect_name to connect_name_p;  just NULL it out
**	    now, don't STcopy over it.
*/

DB_STATUS
IICXcreate_xa_rmi_sub_cb(
            IICX_XA_RMI_CB      **xa_rmi_cb_p_p   /* OUT */
           )
{
    DB_STATUS        db_status;
    STATUS           cl_status;
    char             ebuf[20];

    if ((*xa_rmi_cb_p_p = (IICX_XA_RMI_CB *)MEreqmem((u_i4)0, 
                          (u_i4)sizeof(IICX_XA_RMI_CB),
                          TRUE, (STATUS *)&cl_status)) == NULL)
    {
        CVna((i4)cl_status, ebuf);
        IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                  ERx("IICX_XA_RMI_CB"));
        return( E_DB_FATAL );
    }

    (*xa_rmi_cb_p_p)->xa_state          = IICX_XA_R_CLOSED;
    (*xa_rmi_cb_p_p)->rec_sid           = 0;
    (*xa_rmi_cb_p_p)->usr_arg_count     = IICX_XA_MAX_OPENSTR_TOKENS;
    (*xa_rmi_cb_p_p)->flags_start_index = 0;
    (*xa_rmi_cb_p_p)->connect_name_p	= NULL;

    return( E_DB_OK );

} /* IICXcreate_xa_rmi_sub_cb */




/*
**   Name: IICXfree_xa_rmi_sub_cb()
**
**   Description: 
**        The XA RMI control block is physically deleted.
**
**   Outputs:
**       Returns: Pointer to the CX CB, if properly created. 
**                NULL if there was an error.
**
**   History:
**       9-Oct-1992 - First written (mani)
*/

DB_STATUS
IICXfree_xa_rmi_sub_cb(
            IICX_XA_RMI_CB       *xa_rmi_cb_p    /* IN */
           )
{
    STATUS             cl_status;
    char               ebuf[20];

    if ((cl_status = MEfree( (PTR) xa_rmi_cb_p )) != OK)
    {
      CVna((i4)cl_status, ebuf);
      IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                     ERx("IIcx_xa_rmi_cb"));
      return( E_DB_FATAL );
    }

    return( E_DB_OK );

} /* IICXfree_xa_rmi_sub_cb */




/*
**   Name: IICXcheck_if_unique_rmi()
**
**   Description: 
**       Checks if the input dbname, and/or the connection name have
**       been previously encountered, for any of the previously opened RMIs.
**
**   Inputs:
**       dbname       - database name.
**       connect_name - connection name for this RMI.
** 
**   Outputs:
**       Returns: DB_ERROR if this is a duplicate. DB_SUCCESS/WARN if this
**                dbname/connect_name is unique.
**
**   History:
**       02-Oct-1992 - First written (mani)
**	 04-Jun-1993 (larrym)
**	    Modified to use connection name
**	11-nov-1993 (larrym)
**	    changed connect_name to connect_name_p;  database name is
**	    now pointed to by dbname_p instead of usr_argv[1].
*/

FUNC_EXTERN  
DB_STATUS        IICXcheck_if_unique_rmi(
                           char                *dbname,
                           char                *connect_name
                          )
{
  IICX_CB         *tmp_cx_cb_p;
  IICX_XA_RMI_CB  *xa_rmi_cb_p;
  char            ebuf[20];

  /* LOCK the XA RMI MAIN CB */

  for(tmp_cx_cb_p = IIcx_xa_rmi_main_cb->xa_rmi_list;
      tmp_cx_cb_p != NULL;
      tmp_cx_cb_p = tmp_cx_cb_p->cx_next)
  {
      xa_rmi_cb_p = tmp_cx_cb_p->cx_sub_cb.xa_rmi_cb_p;
      if ((!STbcompare(xa_rmi_cb_p->dbname_p,0,dbname,0,TRUE)) ||
          (!STbcompare(xa_rmi_cb_p->connect_name_p,0,connect_name,0,FALSE)))
      {
          IICXerror(GE_LOGICAL_ERROR, E_CX0008_DUP_RMI_NAME_OR_ID, 2, 
			    connect_name, ERx(dbname));
          /* UNLOCK the XA RMI MAIN CB */
          return( E_DB_ERROR );
      }
  }

  /* UNLOCK the XA RMI MAIN CB */

  return( E_DB_OK );

} /* IICXcheck_if_unique_rmi */



/*
**   Name: IICXget_rmid_by_connect_name()
**
**   Description: 
**       Returns the rmid of the RMI that corresponds to this connection_name.
**       The rmid <--> connection_name mapping is established at the time of the
**       initialization (typically) of the Ingres Client/XA AS process.
**
**   Inputs:
**       connect_name   - connection_name of RMI
** 
**   Outputs:
**       rmid  - of RMI that corresponds to this static sid.
**
**   Returns: DB_ERROR if there was an error and/or rmid was not found.
**
**   History:
**	16-nov-1992 - First written (mani)
**	09-mar-1993 (larrym)
**	    modified to use connection names.
**	11-nov-1993 (larrym)
**	    changed connect_name to connect_name_p;  database name is
**	    now pointed to by dbname_p instead of usr_argv[1].
*/

FUNC_EXTERN  
DB_STATUS        
IICXget_rmid_by_connect_name(
                           char	               *connect_name,
                           int                 *rmid_p
                          )
{

  IICX_CB         *tmp_cx_cb_p;
  IICX_XA_RMI_CB  *xa_rmi_cb_p;
  char            ebuf[20];

  /* LOCK the XA RMI MAIN CB */

  for(tmp_cx_cb_p = IIcx_xa_rmi_main_cb->xa_rmi_list;
      tmp_cx_cb_p != NULL;
      tmp_cx_cb_p = tmp_cx_cb_p->cx_next)
  {
      xa_rmi_cb_p = tmp_cx_cb_p->cx_sub_cb.xa_rmi_cb_p;
      if (!STbcompare(xa_rmi_cb_p->connect_name_p, 0, connect_name,0,FALSE))
      {
          *rmid_p = tmp_cx_cb_p->cx_id.u_cx_id.xa_rmi_id;

          /* UNLOCK the XA RMI MAIN CB */
          return( E_DB_OK );
      }
  }

  /* UNLOCK the XA RMI MAIN CB */
  /* There was NO XA RMI CB for this connection name. Return an error */
  IICXerror(GE_LOGICAL_ERROR, E_CX0009_RMI_NOT_FOUND, 1, connect_name);

  return( E_DB_ERROR );


} /* IICXget_rmid_by_connect_name */



/*
**   Name: IICXcreate_xa_xn_cb()
**
**   Description: 
**        The XA XN control block is obtained from a reserve list, or else
**        created and initialized with default values. 
**
**   Outputs:
**       Returns: Pointer to the CX CB, if properly created. 
**                NULL if there was an error.
**
**   History:
**       25-Aug-1992 - First written (mani)
**       30-may-1996 (stial01)
**          IICXcreate_xa_xn_cb: check server_name if requested (B75073)
**       12-aug-1996 (stial01)
**          IICXcreate_xa_xn_cb: check listen address in the XA XN CB(B78248)
*/

DB_STATUS
IICXcreate_xa_xn_cb(
            IICX_ID              *cx_id,         /* IN */
            IICX_U_SUB_CB        *xa_xn_sub_cb_p,/* IN, optional */
            IICX_CB              **cx_cb_p_p     /* OUT */
           )
{
    DB_STATUS          db_status;
    STATUS             cl_status;
    char               ebuf[20];
    IICX_XA_XN_CB      *xa_xn_cb_p;

    /* For now, an XA XN SUB CB as input is UNEXPECTED ! CHECK !!! */
    if (xa_xn_sub_cb_p != NULL)
    {
       IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1,
                         ERx("create_xa_xn_cb:xa_xn_sub_cb unexpected"));
       return( E_DB_FATAL );
    }

    /* LOCK the XA XN MAIN CB. CHECK !!! */

    if (IIcx_xa_xn_main_cb->num_free_xa_xn_cbs > 0)
    {
        for ( *cx_cb_p_p = IIcx_xa_xn_main_cb->xa_xn_cb_free_list;
              *cx_cb_p_p != NULL;
              *cx_cb_p_p = (*cx_cb_p_p)->cx_next )
        {
            if ( ((*cx_cb_p_p)->cx_id.u_cx_id.xa_xn_id.rmid == 
                                     cx_id->u_cx_id.xa_xn_id.rmid )
		&& (!cx_id->u_cx_id.xa_xn_id.server_flag ||
		   !(STcompare((*cx_cb_p_p)->cx_sub_cb.xa_xn_cb_p->listen_addr,
			     cx_id->u_cx_id.xa_xn_id.server_name))))
	    {
		 /* found one with the correct RMI, snag it off the list */

                 if ((*cx_cb_p_p)->cx_prev != NULL)
                     (*cx_cb_p_p)->cx_prev->cx_next = (*cx_cb_p_p)->cx_next;
                 else
                    IIcx_xa_xn_main_cb->xa_xn_cb_free_list =
                                          (*cx_cb_p_p)->cx_next;

                 if ((*cx_cb_p_p)->cx_next != NULL)
                     (*cx_cb_p_p)->cx_next->cx_prev = (*cx_cb_p_p)->cx_prev;
         
                 IIcx_xa_xn_main_cb->num_free_xa_xn_cbs--;
                 
                 /* UNLOCK the XA XN Main CB. CHECK !!! */
                 return( E_DB_OK );
	    }
	}

	/* 
	** if we're here, we didn't find one with correct rmi, so just 
	** create a new control block (fall into next block of code).
	*/

        /* UNLOCK the XA XN Main CB. CHECK !!! */
    }


    /* UNLOCK the XA XN Main CB. CHECK !!! */

    /* 
    ** No free XA XN CBs (or no matching free ones). 
    ** Create one if appropriate 
    */

    db_status = IICXget_new_cx_cb( cx_cb_p_p );
    if (DB_FAILURE_MACRO(db_status))
    {
        return(db_status);
    }

    if ((xa_xn_cb_p = (IICX_XA_XN_CB *)MEreqmem((u_i4)0, 
                                (u_i4)sizeof(IICX_XA_XN_CB),
                                TRUE, (STATUS *)&cl_status)) == NULL)
    {
        CVna((i4)cl_status, ebuf);
        IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                   ERx("IICX_XA_XN_CB"));
        return( E_DB_FATAL );
    }

    /* intialize xa_xn_cb with sessio startup params */
    xa_xn_cb_p->xa_state       = IICX_XA_X_NONEXIST;
    xa_xn_cb_p->xa_assoc_state = IICX_XA_X_NO_ASSOC;
    xa_xn_cb_p->ingres_state   = IICX_XA_X_ING_NEW;
    xa_xn_cb_p->xa_sid         = 0; /* unknown at this time */
    xa_xn_cb_p->xa_rb_value    = 0;
    (*cx_cb_p_p)->cx_sub_cb.xa_xn_cb_p = xa_xn_cb_p;
    STcopy("", xa_xn_cb_p->connect_name);  /* also unknown at this time */

    return( E_DB_OK );

} /* IICXcreate_xa_xn_cb */





/*
**   Name: IICXdelete_xa_xn_cb()
**
**   Description: 
**        The XA XN control block is logically deleted. It deletes all of
**        its children. If the last of its children are gone, it places
**        itself in a process-scoped free list.
**
**   Outputs:
**
**   History:
**       9-Oct-1992 - First written (mani)
*/

DB_STATUS
IICXdelete_xa_xn_cb(
            IICX_CB       *cx_cb_p    /* IN */
           )
{
    DB_STATUS          db_status;
    IICX_XA_XN_CB      *xa_xn_cb_p;

    /* LOCK the XA XN MAIN CB. CHECK !!! */

    /* remove the cb from the list */
    if (cx_cb_p->cx_prev != NULL)
        cx_cb_p->cx_prev->cx_next = cx_cb_p->cx_next;
    else
        IIcx_xa_xn_main_cb->xa_xn_list = cx_cb_p->cx_next;
    if (cx_cb_p->cx_next != NULL)
        cx_cb_p->cx_next->cx_prev = cx_cb_p->cx_prev;


    if ( IIcx_xa_xn_main_cb->num_free_xa_xn_cbs ==
         IIcx_xa_xn_main_cb->max_free_xa_xn_cbs )
    {
        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
        {
            IIxa_fe_qry_trace_handler( (PTR) 0,
                ERx("\treached maximum session cache.  Freeing session...\n"), 
                TRUE);
	}

	/* free list is too full.  Physically delete this one */
        /* UNLOCK the XA XN MAIN CB. CHECK !!! */
        db_status = IICXfree_cb( cx_cb_p );
        if (DB_FAILURE_MACRO( db_status ))
            return( db_status );
    }
    else
    {    
	/* Add this one to the free list (after cleaning it up) */
        IIcx_xa_xn_main_cb->num_free_xa_xn_cbs++;
        /* retain the connection_name and the xa_sid.               */
        /* if the ingres_state is set to ING_IN_USE then we'll reuse the */
        /* connect for a new XA XID that is subsequently xa_started.     */
        cx_cb_p->cx_sub_cb.xa_xn_cb_p->xa_state = IICX_XA_X_NONEXIST;
        cx_cb_p->cx_sub_cb.xa_xn_cb_p->xa_assoc_state = IICX_XA_X_NO_ASSOC;
	cx_cb_p->cx_sub_cb.xa_xn_cb_p->xa_rb_value  = 0;

        if (IIcx_xa_xn_main_cb->xa_xn_cb_free_list != NULL) 
	{
            IIcx_xa_xn_main_cb->xa_xn_cb_free_list->cx_prev = cx_cb_p;
	}
        cx_cb_p->cx_next = IIcx_xa_xn_main_cb->xa_xn_cb_free_list;
        cx_cb_p->cx_prev = NULL;
        IIcx_xa_xn_main_cb->xa_xn_cb_free_list = cx_cb_p;
        
        /* UNLOCK the XA session MAIN CB. Check !!! */
    }

    return( E_DB_OK );

} /* IICXdelete_xa_xn_cb */




/*
**   Name: IICXfree_xa_xn_cb()
**
**   Description: 
**        The XA XN control block is physically deleted.
**
**   Outputs:
**       Returns: Pointer to the CX CB, if properly created. 
**                NULL if there was an error.
**
**   History:
**       9-Oct-1992 - First written (mani)
*/

DB_STATUS
IICXfree_xa_xn_cb(
            IICX_XA_XN_CB       *xa_xn_cb_p    /* IN */
           )
{
    STATUS             cl_status;
    char               ebuf[120];
    i4		       save_thread_state;
    IICX_CB            *thread_cx_cb_p;
    IICX_FE_THREAD_CB  *fe_thread_cb_p;
    DB_STATUS          db_status;

    EXEC SQL BEGIN DECLARE SECTION;
    i4			free_session;
    i4			err_number;
    EXEC SQL END DECLARE SECTION;

    /* switch thread to INTERNAL state */
    db_status = IICXget_lock_fe_thread_cb( NULL, &thread_cx_cb_p );
    if (DB_FAILURE_MACRO( db_status ))
    {
        return( db_status );
    }
    fe_thread_cb_p = thread_cx_cb_p->cx_sub_cb.fe_thread_cb_p;
    save_thread_state = fe_thread_cb_p->ingres_state;
    fe_thread_cb_p->ingres_state = IICX_XA_T_ING_INTERNAL;

    free_session = xa_xn_cb_p->xa_sid;
    if (free_session != 0)
    {
        if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
	{
            char	outbuf[80]; 

            STprintf(outbuf,
                ERx("\tdisconnecting %s (INGRES/OpenDTP session %d)\n"),
                xa_xn_cb_p->connect_name, free_session);
            IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
	}

	EXEC SQL DISCONNECT SESSION :free_session;
	if (sqlca.sqlcode < 0)
        {
	    EXEC SQL INQUIRE_SQL (:err_number = ERRORNO);
            STprintf(ebuf,
                ERx("IICXfree_xa_xn_cb:  Could not disconnect from session %d sqlcode %d err_number %d"),
                free_session, sqlca.sqlcode, err_number);
            IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1, ebuf);
	    /*
	    **  Shouldn't return.  We're probably trying to shutdown so we
	    **  we should continue freeing the CB.
            ** IICXunlock_cb( thread_cx_cb_p );
            ** return( E_DB_FATAL );
	    */
        }
    }

    fe_thread_cb_p->ingres_state = save_thread_state;
    IICXunlock_cb( thread_cx_cb_p );
	
    if ((cl_status = MEfree( (PTR) xa_xn_cb_p )) != OK)
    {
      CVna((i4)cl_status, ebuf);
      IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                     ERx("IIcx_xa_xn_cb"));
      return( E_DB_FATAL );
    }

    return( E_DB_OK );

} /* IICXfree_xa_xn_cb */



/*
**   Name: IICXcreate_xa_xn_thrd_cb()
**
**   Description:
**        The XA XN THRD CB is created, if needed. Else it is obtained from a
**        free list.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: Pointer to the XA XA THRD CB, if properly created.
**                NULL if there was an error.
**
**   History:
**      24-Mar-1993 - First written (mani)
**	23-Aug-1993 (larrym)
**	    Modified to initialize new element of the IICX_XA_XN_THRD_CB
**	    which is an array of i4  indexed by possible Association states
**	    so we can track how many xa_xn cb's have changed state.  When
**	    all xa_xn's have changed to a new state we can move the xa_xn_thrd
**	    state.
*/

FUNC_EXTERN  DB_STATUS  IICXcreate_xa_xn_thrd_cb(
                           IICX_ID             *cx_id,          /* IN */
                           IICX_CB             **cx_cb_p_p      /* OUT */
                        )
{
    DB_STATUS		db_status;
    STATUS		cl_status;
    char		ebuf[20];
    IICX_XA_XN_THRD_CB  *xa_xn_thrd_cb_p;
    i4			i;

    /* LOCK the XA XN THRD MAIN CB. CHECK !!! */

    if (IIcx_xa_xn_thrd_main_cb->num_free_xa_xn_thrd_cbs > 0)
    {
        *cx_cb_p_p = IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_cb_free_list;
        IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_cb_free_list =
                                          (*cx_cb_p_p)->cx_next;

        if ((*cx_cb_p_p)->cx_next != NULL)
            (*cx_cb_p_p)->cx_next->cx_prev = (*cx_cb_p_p)->cx_prev;
         
        IIcx_xa_xn_thrd_main_cb->num_free_xa_xn_thrd_cbs--;
    }

    if (*cx_cb_p_p != NULL)
    {
	/* UNLOCK the XA XN THRD Main CB. CHECK !!! */
        return( E_DB_OK );
    }

    /* UNLOCK the XA XN THRD Main CB. CHECK !!! */

    /* No free XA XN THRD CBs. Create one if appropriate */
    db_status = IICXget_new_cx_cb( cx_cb_p_p );
    if (DB_FAILURE_MACRO(db_status))
    {
       return(db_status);
    }

    if ((xa_xn_thrd_cb_p = (IICX_XA_XN_THRD_CB *)MEreqmem((u_i4)0, 
                                (u_i4)sizeof(IICX_XA_XN_THRD_CB),
                                TRUE, (STATUS *)&cl_status)) == NULL)
    {
           CVna((i4)cl_status, ebuf);
           IICXerror(GE_NO_RESOURCE, E_CX0002_CX_CB_ALLOC, 2, ebuf, 
                                      ERx("IICX_XA_XN_THRD_CB"));
           return( E_DB_FATAL );
    }

    xa_xn_thrd_cb_p->xa_assoc_state    	  = IICX_XA_T_NO_ASSOC;
    for( i=0; i < IICX_XA_T_NUM_STATES; i++)
        xa_xn_thrd_cb_p->assoc_state_by_rmi[ i ] = 0;
    (*cx_cb_p_p)->cx_sub_cb.xa_xn_thrd_cb_p = xa_xn_thrd_cb_p;

    xa_xn_thrd_cb_p->curr_xa_xn_cx_cb_p   = NULL;

    return( E_DB_OK );

} /* IICXcreate_xa_xn_thrd_cb */





/*
**   Name: IICXdelete_xa_xn_thrd_cb()
**
**   Description: 
**        The XA XN THRD control block is logically deleted. It deletes all of
**        its children. If the last of its children are gone, it places
**        itself in a process-scoped free list.
**
**   Outputs:
**
**   History:
**      26-Mar-1993 - First written (mani)
**	23-Aug-1993 (larrym)
**	    Modified to (re)initialize new element of the IICX_XA_XN_THRD_CB
**	    which is an array of i4  indexed by possible Association states
**	    so we can track how many xa_xn cb's have changed state.  When
**	    all xa_xn's have changed to a new state we can move the xa_xn_thrd
**	    state.
*/

DB_STATUS
IICXdelete_xa_xn_thrd_cb(
            IICX_CB       *cx_cb_p    /* IN */
           )
{
    DB_STATUS		db_status;
    IICX_XA_XN_THRD_CB 	*xa_xn_thrd_cb_p;
    i4			i;

    /* LOCK the XA XN THRD MAIN CB. CHECK !!! */

    /* remove the cb from the list */
    if (cx_cb_p->cx_prev != NULL)
        cx_cb_p->cx_prev->cx_next = cx_cb_p->cx_next;
    else
	IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_list = cx_cb_p->cx_next;
    if (cx_cb_p->cx_next != NULL)
        cx_cb_p->cx_next->cx_prev = cx_cb_p->cx_prev;

    if ( IIcx_xa_xn_thrd_main_cb->num_free_xa_xn_thrd_cbs ==
         IIcx_xa_xn_thrd_main_cb->max_free_xa_xn_thrd_cbs )
    {

        /* UNLOCK the XA XN THRD MAIN CB. CHECK !!! */
        db_status = IICXfree_cb( cx_cb_p );
        if (DB_FAILURE_MACRO( db_status ))
            return( db_status );
    }
    else
    {    
        IIcx_xa_xn_thrd_main_cb->num_free_xa_xn_thrd_cbs++;

        cx_cb_p->cx_sub_cb.xa_xn_thrd_cb_p->xa_assoc_state = IICX_XA_T_NO_ASSOC;
	for( i=0; i < IICX_XA_T_NUM_STATES; i++)
	    cx_cb_p->cx_sub_cb.xa_xn_thrd_cb_p->assoc_state_by_rmi[ i ] = 0;

        cx_cb_p->cx_sub_cb.xa_xn_thrd_cb_p->curr_xa_xn_cx_cb_p   = NULL;

        if (IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_cb_free_list != NULL) 
	{
            IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_cb_free_list->cx_prev = 
                                                   cx_cb_p;
	}
        cx_cb_p->cx_next = IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_cb_free_list;
        cx_cb_p->cx_prev = NULL;
        IIcx_xa_xn_thrd_main_cb->xa_xn_thrd_cb_free_list = cx_cb_p;
        
    }

    return( E_DB_OK );

} /* IICXdelete_xa_xn_thrd_cb */




/*
**   Name: IICXfree_xa_xn_thrd_cb()
**
**   Description: 
**        The XA XN THRD control block is physically deleted.
**
**   Outputs:
**       Returns: Pointer to the CX CB, if properly created. 
**                NULL if there was an error.
**
**   History:
**       26-Mar-1993 - First written (mani)
*/

DB_STATUS
IICXfree_xa_xn_thrd_cb(
            IICX_XA_XN_THRD_CB       *xa_xn_thrd_cb_p    /* IN */
           )
{
    STATUS             cl_status;
    char               ebuf[20];

    if ((cl_status = MEfree( (PTR) xa_xn_thrd_cb_p )) != OK)
    {
      CVna((i4)cl_status, ebuf);
      IICXerror(GE_LOGICAL_ERROR, E_CX0003_CX_CB_FREE, 2, ebuf, 
                                     ERx("IIcx_xa_xn_thrd_cb"));
      return( E_DB_FATAL );
    }

    return( E_DB_OK );

} /* IICXfree_xa_xn_thrd_cb */




/*
**   Name: IICXassign_xa_xid()
**
**   Description: 
**       Assigns an XA XID from source to destination.
**
**   Inputs:
**       src_xid_p      - pointer to the source XA XID.
**       dest_xid_p     - pointer to the dest   XA XID.
**
**   Outputs:
**       Returns: the dest XA XID initialized.
**
**   History:
**       02-Oct-1992 - First written (mani)
*/
DB_STATUS
IICXassign_xa_xid(DB_XA_DIS_TRAN_ID     *src_xid_p,
                  DB_XA_DIS_TRAN_ID     *dest_xid_p)
{
    char      text_buff[IICX_ID_ST_SIZE_MAX];
    DB_STATUS db_status;

    db_status = IICXvalidate_xa_xid( src_xid_p );
    if (DB_FAILURE_MACRO( db_status ))
        return( db_status );

    dest_xid_p->formatID = src_xid_p->formatID;
    dest_xid_p->gtrid_length = src_xid_p->gtrid_length;
    dest_xid_p->bqual_length = src_xid_p->bqual_length;

    MECOPY_VAR_MACRO( src_xid_p->data, src_xid_p->gtrid_length + 
                      src_xid_p->bqual_length, dest_xid_p->data );

    return( E_DB_OK );

}  /* IICXassign_xa_xid */




/*
**   Name: IICXcompare_xa_xid()
**
**   Description: 
**       Compares one XA XID with another. Returns 0 if they are equal.
**
**   Inputs:
**       xid1_p      - pointer to an XA XID.
**       xid2_p      - pointer to another XA XID.
**
**   Outputs:
**       Returns: 0, if they are equal. 1 otherwise.
**
**   History:
**       02-Oct-1992 - First written (mani)
*/
i4
IICXcompare_xa_xid(DB_XA_DIS_TRAN_ID     *xid1_p,
                   DB_XA_DIS_TRAN_ID      *xid2_p)
{
    return( ((xid1_p->formatID == xid2_p->formatID) &&
             (xid1_p->gtrid_length == xid2_p->gtrid_length) &&
             (xid1_p->bqual_length == xid2_p->bqual_length) &&
             (!MEcmp(xid1_p->data,xid2_p->data,
                    xid1_p->gtrid_length+xid1_p->bqual_length))) ? 0 : 1 );


}  /* IICXcompare_xa_xid */





/*
**   Name: IICXconv_to_struct_xa_xid()
**
**
**   Description: 
**       Converts the XA XID from a text format to an XID structure format.
**
**   Inputs:
**       xid_text       - pointer to a text buffer that has the XID in text
**                        format. 
**       xid_str_max_len- max length of the xid string.
**       xa_xid_p       - address of an XA XID structure.
**
**   Outputs:
**       Returns: the XA XID structure, appropriately filled in.
**
**   History:
**       11-Jan-1993 - First written (mani)
**       23-Sep-1993 (iyer)
**            The function has been modified to accept two additional
**            arguments of type *nat i.e. branch_seqnum and branch_flag.
**            The xid string retreived now has the additional fields
**            appended when it is logged onto the IMA table. The two fields
**            are extracted from xid string and returned. 
**	06-jan-1994 (larrym)
**	    if'd out iyer's changes above as the lgmo view has been changed
**	    to return the branch_seqnum and branch_flag as separate fields.
**	18-jan-1996 (toumi01; from 1.1 axp_osf port) (schte01)
**	    changed call to convert routine for axp-osf to convert to a long
**	    of 8 bytes.
**	05-oct-1998 (thoda04)
**	    Process XID data bytes in groups of four, high to low bytes for all platforms.
**      24-Mar-1999 (hweho01)
**          Extended the Changes for axp-osf to ris_u64 (AIX 64-bit
**          platform).
**      08-Sep-2000 (hanje04)
**          Extended the Changes for axp-osf to axp_lnx (Alpha Linux).
**      11-Mar-2004 (hanal04) Bug 111923 INGSRV2751
**          Changed cast in call to CVuahxl8() to quiet compiler warnings.
**      12-Mar-2004 (hanal04) Bug 111923 INGSRV2751
**          Modify all XA xid fields to i4 (int for supplied files)
**          to ensure consistent use across platforms.
*/
DB_STATUS
IICXconv_to_struct_xa_xid(
              char                 *xid_text,
              i4              xid_str_max_len,
              DB_XA_DIS_TRAN_ID    *xa_xid_p,
              i4                   *branch_seqnum,
              i4                   *branch_flag)
{
   char        *cp, *next_cp, *prev_cp;
   u_char      *tp;
   char        c;
   i4          j;
   i4          num_xid_words;
   i4     num;            /* work i4 */
   u_i4        unum;
   STATUS      cl_status;

   if ((cp = STindex(xid_text, ":", xid_str_max_len)) == NULL)
   {
       IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text);
       return( E_DB_ERROR );
   }

   c = *cp;
   *cp = '\0';

   if ((cl_status = CVuahxl(xid_text,(u_i4 *)(&xa_xid_p->formatID))) != OK)
   {
      IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1,
           ERx("conv_to_struct_xa_xid:formatID field conversion failed."));
      IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text);
      return( E_DB_ERROR );
   }

   *cp = c;    

   if ((next_cp = STindex(cp + 1, ":", 0)) == NULL)
   {
       IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text);
       return( E_DB_ERROR );
   }

   c = *next_cp;
   *next_cp = '\0';

   if ((cl_status = CVal(cp + 1, &num)) != OK)
   {
      IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1,
           ERx("conv_to_struct_xa_xid:gtrid_length field conversion failed."));
      IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text);
      return( E_DB_ERROR );
   }
   xa_xid_p->gtrid_length = num;  /* 32 or 64 bit long gtrid_length = i4 num */

   *next_cp = c;    

   if ((cp = STindex(next_cp + 1, ":", 0)) == NULL)
   {
       IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text);
       return( E_DB_ERROR );
   }

   c = *cp;
   *cp = '\0';

   if ((cl_status = CVal(next_cp + 1, &num)) != OK)
   {
      IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1,
           ERx("conv_to_struct_xa_xid:bqual_length field conversion failed."));
      IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text);
      return( E_DB_ERROR );
   }
   xa_xid_p->bqual_length = num;  /* 32 or 64 bit long bqual_length = i4 num */

   *cp = c;    

    /* Now copy all the data                            */
    /* We now go through all the hex fields separated by */
    /* ":"s.  We convert it back to bytes and store it in */
    /* the data field of the distributed transaction id  */
    /*     cp      - points to a ":"                       */
    /*     prev_cp - points to the ":" previous to the     */
    /*               ":" cp points to, as we get into the  */
    /*               loop that converts the XID data field */
    /*     tp    -   points to the data field of the       */
    /*               distributed transaction id            */

    prev_cp = cp;
    tp = (u_char*) (&xa_xid_p->data[0]);
    num_xid_words = (xa_xid_p->gtrid_length+
            xa_xid_p->bqual_length+
            sizeof(u_i4)-1)/sizeof(u_i4);
    for (j=0;
         j< num_xid_words; 
         j++)
    {

         if ((cp = STindex(prev_cp + 1, ":", 0)) == NULL)
         {
             IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text);
             return( E_DB_ERROR );
         }
         c = *cp;
         *cp = '\0';
         if ((cl_status = CVuahxl(prev_cp + 1,&unum)) != OK)
         {
             IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1,
               ERx("conv_to_struct_xa_xid:xid_data field conversion failed."));
             IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text);
             return( E_DB_ERROR );
         }
         tp[0] = (u_char)((unum >> 24) & 0xff);  /* put back in byte order */
         tp[1] = (u_char)((unum >> 16) & 0xff);
         tp[2] = (u_char)((unum >>  8) & 0xff);
         tp[3] = (u_char)( unum        & 0xff);
         tp += sizeof(u_i4);
         *cp = c;
         prev_cp = cp;
    }

/*
** if'd out the following code.  See history comments for 06-jan-1994 
*/

#if 0
    next_cp = cp;                           /* Store last ":" position */
                                            /* Skip "XA" & look for next ":" */

    if ((cp = STindex(next_cp + 1, ":", 0)) == NULL) 
       {                                   /* First ":" before branch seq */
            IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text); 
            return( E_DB_ERROR );
       }

    next_cp = cp;                          /* Skip branch seq for next ":" */

    if ((cp = STindex(next_cp + 1, ":", 0)) == NULL) 
       {                                   /* First ":" after branch seq */
             IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text); 
             return( E_DB_ERROR );
       }

    c = *cp;                               /* Store actual char in that POS */
    *cp = EOS;                             /* Pretend end of string */

    if ((cl_status = CVan(next_cp + 1, branch_seqnum)) != OK)
    {
      IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1,
           ERx("conv_to_struct_xa_xid:branch_seqnum conversion failed."));
      IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text);
      return( E_DB_ERROR );
    }

   *cp = c;                              /* Restore char in that Position */
    
   if ((next_cp = STindex(cp + 1, ":", 0)) == NULL)
   {
       IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text); 
       return( E_DB_ERROR );
   }

   c = *next_cp;
   *next_cp = EOS;

   if ((cl_status = CVan(cp + 1, branch_flag)) != OK)
   {
      IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1,
           ERx("conv_to_struct_xa_xid:branch_flag field conversion failed."));
      IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, xid_text);
      return( E_DB_ERROR );
   }


   *next_cp = c;    

#endif

   return( E_DB_OK );

} /* IICXconv_to_struct_xa_xid */





/*
**   Name: IICXformat_xa_xid()
**
**   Description: 
**       Formats the XA XID into the text buffer.
**
**   Inputs:
**       xa_xid_p       - pointer to the XA XID.
**       text_buffer    - pointer to a text buffer that will contain the text
**                        form of the ID.
**   Outputs:
**       Returns: the character buffer with text.
**
**   History:
**       02-Oct-1992 - First written (mani)
**       05-oct-1998 (thoda04)
**          Process XID data bytes in groups of four, high order to low, bytes for all platforms.
*/
VOID
IICXformat_xa_xid(DB_XA_DIS_TRAN_ID    *xa_xid_p,
              char       *text_buffer)
{
  char     *cp = text_buffer;
  i4       data_lword_count = 0;
  i4       data_byte_count  = 0;
  u_char   *tp;                  /* pointer to xid data */
  u_i4 unum;                /* unsigned i4 work field */
  i4       i;

  CVlx(xa_xid_p->formatID,cp);
  STcat(cp, ERx(":"));
  cp += STlength(cp);
 
  CVla((i4)(xa_xid_p->gtrid_length),cp); /* length 1-64 from i4 or i8 long gtrid_length */
  STcat(cp, ERx(":"));
  cp += STlength(cp);

  CVla((i4)(xa_xid_p->bqual_length),cp); /* length 1-64 from i4 or i8 long bqual_length */
  cp += STlength(cp);

  data_byte_count = (i4)(xa_xid_p->gtrid_length + xa_xid_p->bqual_length);
  data_lword_count = (data_byte_count + sizeof(u_i4) - 1) / sizeof(u_i4);
  tp = (u_char*)(xa_xid_p->data);  /* tp -> B0B1B2B3 xid binary data */

  for (i = 0; i < data_lword_count; i++)
  {
     STcat(cp, ERx(":"));
     cp++;
     unum = (u_i4)(((i4)(tp[0])<<24) |   /* watch out for byte order */
                        ((i4)(tp[1])<<16) | 
                        ((i4)(tp[2])<<8)  | 
                         (i4)(tp[3]));
     CVlx(unum, cp);     /* build string "B0B1B2B3" in byte order for all platforms*/
     cp += STlength(cp);
     tp += sizeof(u_i4);
  }
  STcat(cp, ERx(":XA"));


} /* IICXformat_xa_xid */
       



/*
**   Name: IICXformat_xa_extd_xid()
**
**   Description: 
**       Formats the branch_seqnum and branch_flag into the text buffer.
**       The additional fields are formatted after :XA as
**       :XXXX:XXXX:EX.
**
**   Inputs:
**       branch_seqnum   - Value of the branch Seq Number
**       branch_flag     - Value of Branch Flag
**       text_buffer     - pointer to a text buffer that will contain the text
**                         form of the ID.
**   Outputs:
**       Returns: the character buffer with branch_seqnum and branch_flag
**                appended to its end.
**
**   History:
**       23-Sep-1993 - First written (iyer)
*/
VOID
IICXformat_xa_extd_xid(i4  branch_seqnum,
                       i4   branch_flag,
                       char *text_buffer)
{
  char     *cp = text_buffer;

  STcat(cp, ERx(":"));  
  cp += STlength (cp);

  CVna(branch_seqnum,cp);
  STcat(cp, ERx(":"));  
  cp += STlength(cp);

  CVna(branch_flag,cp);
  STcat(cp, ERx(":EX"));  


} /* IICXformat_xa_extd_xid */
       



/*
**   Name: IICXvalidate_xa_xid()
**
**   Description: 
**       Validates an XA XID for proper gtrid_length, bqual_length.
**
**   Inputs:
**       xid_p      - pointer to the XA XID.
**
**   Outputs:
**       Returns: E_DB_OK/WARN if everything was all right.
**
**   History:
**       02-Oct-1992 - First written (mani)
*/
DB_STATUS
IICXvalidate_xa_xid(DB_XA_DIS_TRAN_ID     *xid_p)
{
    char      text_buff[IICX_ID_ST_SIZE_MAX];

    if (xid_p == NULL)
    {
       IICXerror(GE_LOGICAL_ERROR, E_CX0001_INTERNAL, 1, 
                     ERx("Trying to validate NULL XA XID. Unexpected"));
        return( E_DB_ERROR );
    }

    if (xid_p->gtrid_length < 1 ||
        xid_p->gtrid_length > DB_XA_MAXGTRIDSIZE ||
        xid_p->bqual_length < 1 ||
        xid_p->bqual_length > DB_XA_MAXBQUALSIZE)
    {
        DB_XA_DIS_TRAN_ID     bad_xid;
        
	if (xid_p->gtrid_length < 1) 
	{
	    bad_xid.gtrid_length = 0;
	}
	else
	{
	    bad_xid.gtrid_length = DB_XA_MAXGTRIDSIZE;
	}
	if (xid_p->bqual_length < 1) 
	{
	    bad_xid.bqual_length = 0;
	}
	else
	{
	    bad_xid.bqual_length = DB_XA_MAXBQUALSIZE;
	}

        MECOPY_VAR_MACRO( xid_p->data, bad_xid.gtrid_length + 
                      bad_xid.bqual_length, bad_xid.data );

        IICXformat_xa_xid(&bad_xid,text_buff);
        IICXerror(GE_LOGICAL_ERROR, E_CX0007_BAD_XA_XID, 1, text_buff);
        return( E_DB_ERROR );
    }
       
    return( E_DB_OK );

}  /* IICXvalidate_xa_xid */






/*
**   Name: IICXget_lock_xa_xn_cx_cb_by_rmi()
**
**   Description: 
**         Checks if the input XA XN CB pointer is set up for the right
**         RMI. If yes, then it merely locks the XA XN CB and returns.
**         If not, then it retrieves the right XA XN CB, based on the input
**         rmid, and the XID of the input XA XN CB. It switches to this new
**         XA XN CB.
**
**   Inputs:
**         rmid            - rmid of interest.
**         xa_xn_cx_cb_p_p - pointer to pointer to an XA XN CB.
** 
**   Outputs:
**         xa_xn_cx_cb_p_p - (possibly different) ptr to ptr to XA XN CB.
**
**
**   Returns: E_DB_OK/WARN if successful.
**
**   History:
**       17-nov-1992 - First written (mani)
*/

FUNC_EXTERN  
DB_STATUS        IICXget_lock_xa_xn_cx_cb_by_rmi(
                           i4                  rmid,
                           IICX_CB             **xa_xn_cx_cb_p_p
                          )
{
   IICX_ID     xa_xn_cx_id;
   DB_STATUS   db_status;

   if ((*xa_xn_cx_cb_p_p)->cx_id.u_cx_id.xa_xn_id.rmid == rmid)
   {
      IICXlock_cb( *xa_xn_cx_cb_p_p );
      return( E_DB_OK );
   }

   /* copy contents of the input XA XN CX ID */
   db_status = IICXassign_id( &((*xa_xn_cx_cb_p_p)->cx_id), &xa_xn_cx_id );
   if (DB_FAILURE_MACRO( db_status ))
       return( db_status );

   /* fixup the rmid field of the XA XN CX ID, and search for the resulting */
   /* XA XN CB.  */

   xa_xn_cx_id.u_cx_id.xa_xn_id.rmid = rmid;

   db_status = IICXfind_lock_cb( &xa_xn_cx_id, xa_xn_cx_cb_p_p );
   if (DB_FAILURE_MACRO( db_status ))
       return( db_status );

   return( E_DB_OK );     

} /* IICXget_lock_xa_xn_cx_cb_by_rmi */


/*
**   Name: IICXget_lock_curr_xa_xn_cx_cb()
**
**   Description: 
**         Locates the "current" xa_xn_cx_cb based on LIBQ's notion of the 
**         current session; used to mark xa_xn_cb's as ROLLBACK only
**	   in the case of rogue SQL statements out of band.
**
**   Inputs:
**         xa_xn_cx_cb_p_p - pointer to pointer to an XA XN CB.
** 
**   Outputs:
**         xa_xn_cx_cb_p_p - (possibly different) ptr to ptr to XA XN CB.
**
**
**   Returns: E_DB_OK/WARN if successful.
**
**   History:
**       17-Aug-1993 - First written (larrym)
**	    Needed for bug 54308
*/

FUNC_EXTERN  
DB_STATUS        
IICXget_lock_curr_xa_xn_cx_cb(
                              IICX_CB             **xa_xn_cx_cb_p_p
                             )
{
    EXEC SQL BEGIN DECLARE SECTION;
    i4			curr_session;
    EXEC SQL END DECLARE SECTION;

    IICX_CB	*cx_cb_p;

    *xa_xn_cx_cb_p_p = NULL; /* until further notice */

    EXEC SQL INQUIRE_SQL ( :curr_session = SESSION );
    if (curr_session == 0)
    {
	/* no chance */
	return (E_DB_OK);
    }
    for ( cx_cb_p = IIcx_xa_xn_main_cb->xa_xn_list;
          cx_cb_p != NULL;
          cx_cb_p = cx_cb_p->cx_next )
    {
        if ( cx_cb_p->cx_sub_cb.xa_xn_cb_p->xa_sid == curr_session)
        {
            /* found one with the correct xa_sid, yoke it and bail. */
            *xa_xn_cx_cb_p_p = cx_cb_p;
	    IICXlock_cb( *xa_xn_cx_cb_p_p );
	    break;
        }
    }
    return( E_DB_OK );
}

/*
**   Name: IICXset_xa_xn_server()
**
**   Description:
**        Copy server name into XA XN ID
**
**   Inputs:
**       cx id
**       Server Name
**
**   Outputs:
**       None.
**
**   History:
**      30-May-1996 (stial01)
**          Created. (B75073)
*/
FUNC_EXTERN
VOID
IICXset_xa_xn_server(
	IICX_ID     *cx_id_p,
	char        *server_name)
{

	MEcopy( server_name, 
		DB_MAXNAME,
		cx_id_p->u_cx_id.xa_xn_id.server_name);
	cx_id_p->u_cx_id.xa_xn_id.server_flag = 1; 
}
