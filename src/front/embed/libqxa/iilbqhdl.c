/*
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<er.h>
# include	<erlq.h>
# include	<erusf.h>
# include	<generr.h>
# include       <si.h>			/* needed for iicxfe.h */
# include	<st.h>
# include       <xa.h>
# include       <iixa.h>
# include       <iicx.h>
# include       <iicxfe.h>
# include       <iicxxa.h>
# include	<iisqlca.h>
# include       <iilibq.h>
# include       <iilibqxa.h>
# include       <iixaapi.h>
# include       <iixainit.h>

/*{
+*  Name:  iilbqhdl.c       - Provides handler for libq to call libqxa.
**
**  Description:
**      Defines a global pointer and function for libq to use for processing 
**      XA information.
**
**  Defines:
**	IIxa_check_ingres_state		- check state before SQL query
**	IIxa_check_set_ingres_state	- check state, set to error if error.
**	IIxa_get_and_set_xa_sid  	- maps connection_name to xa_sid
**	IIxa_set_rollback_value		- set's the xa-xn-cb's rollback value.
**
**  Notes:
**	If you add a function here, you must add the function constant define
**	to front!embed!hdr!iilibqxa.h
-*
**  History:
**      03-nov-92 (larrym)
**          Derived from iixamain.c
**	24-feb-1993 (larrym)
**	    added IIxa_check_ingres_state to check the ingres state without
**	    setting to error on failure.  Needed to tell when a statement is
**	    being executed internally by XA or by an application.
**	25-feb-1993 (larrym)
**	    added IIxa_set_rollback_value
**	09-mar-1993 (larrym)
**	    converted to use connection_names
**	07-apr-1993 (larrym,mani)
**	    modified tracking of thread association to transaction branch
**	15-jun-1993 (larrym)
**	    added error messages where it said CHECK.
**	30-Jul-1993 (larrym)
**	    added another error message where it also said CHECK.  Fixed
**	    above error message to use the correct error number (message was
**	    ok).
**	25-Aug-1993 (fredv)
**	    Needed to include <st.h>.
**	18-Dec-97 (gordy)
**	    LIBQ current session moved to thread-local-storage.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
+* Name:   IIxaHdl_libq	- Handler that is always called by libq
**
** Description:
**	This handler is called by libq to access libqxa information. 
**
** Inputs:
**      hflag - What condition triggered this.
**      h_arg_p*  - The data associated with the condition.
**      The following definitions describe the flag and the argument:
**
**        Flag             		Data
**	IIXA_CODE_CHECK_SET_INGRES_STATE	*fe_thread_cx_cb_p,
**						state_mask
**
**	IIXA_CODE_CHECK_INGRES_STATE		*fe_thread_cx_cb_p,
**						state_mask
**
**	IIXA_CODE_GET_AND_SET_XA_SID		*fe_thread_cx_cb_p,
**						connection_name,
**						*xa_sid
**
**	IIXA_CODE_SET_ROLLBACK_VALUE		*fe_thread_cx_cb_p
**	
**	IIXA_CODE_SET_INGRES_STATE		*fe_thread_cx_cb_p,
**						new_ingres_state,
**						*old_ingres_state
**
** Outputs:
**     Returns: 
**        DB_STATUS.
**
** History:
**	19-nov-1992 (larrym)
**	    written.
**	24-feb-1993 (larrym)
**	    Added IIxa_check_ingres_state.
**	25-feb-1993 (larrym)
**	    Added IIxa_set_rollback_value
**	24-mar-1993 (larrym)
**	    Added IIxa_set_ingres_state
*/

DB_STATUS
IIxaHdl_libq (
	i4 	hflag, 
	i4 	h_arg1_n, 
	PTR 	h_arg1_p, 
	PTR 	h_arg2_p, 
	PTR 	h_arg3_p,
	PTR 	h_arg4_p
	)
{
    switch (hflag)
    {
	case IIXA_CODE_CHECK_SET_INGRES_STATE:
	    return IIxa_check_set_ingres_state( 
		(IICX_CB *) h_arg1_p, h_arg1_n);
	case IIXA_CODE_CHECK_INGRES_STATE:
	    return IIxa_check_ingres_state( 
		(IICX_CB *) h_arg1_p, h_arg1_n);
	case IIXA_CODE_GET_AND_SET_XA_SID:
	    return IIxa_get_and_set_xa_sid( 
		(IICX_CB *) h_arg1_p, h_arg2_p, (i4 *) h_arg3_p );
	case IIXA_CODE_SET_ROLLBACK_VALUE:
	    return IIxa_set_rollback_value( (IICX_CB *) h_arg1_p );
	case IIXA_CODE_SET_INGRES_STATE:
	    return IIxa_set_ingres_state( 
		(IICX_CB *) h_arg1_p, h_arg1_n, (i4 *) h_arg2_p );
	default:
	    IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
	    ERx("IIxaHdl_libq passed bad function code (internal error)."));

	;
    }
}

/*
+* Name:   IIxa_check_ingres_state()
**
** Description:
**     If LIBXA is linked in, and XA support is turned on, this routine
**     checks the thread XA state, makes sure it's in an appropriate state (i.e.
**     there was an xa_start() in this thread earlier for the RMIs under
**     consideration.)   Error reporting is left to the caller (most likely 
**     IIxa_check_set_ingres_state()).
**
** Inputs:
**	fe_thread_cx_cb_p	- thread control block, can be NULL
**	state_mask		- bitmask of acceptable/desired states.
**
** Outputs:
**	TRUE, if currently in an acceptable/desired state.
**	FALSE otherwise
*/
FUNC_EXTERN 
DB_STATUS   IIxa_check_ingres_state(IICX_CB   *fe_thread_cx_cb_p,
					i4	  state_mask)
{
   IICX_CB              *thread_cx_cb_p = fe_thread_cx_cb_p;
   IICX_FE_THREAD_CB    *fe_thread_cb_p;
   DB_STATUS            db_status;

   if (thread_cx_cb_p == NULL)
   {
      db_status = IICXget_fe_thread_cb( NULL, &thread_cx_cb_p );
      if (DB_FAILURE_MACRO( db_status ))
      {
          return( db_status );
      }
   }
   else IICXlock_cb( thread_cx_cb_p );

    fe_thread_cb_p = 
           thread_cx_cb_p->cx_sub_cb.fe_thread_cb_p;

    if (fe_thread_cb_p->ingres_state & IICX_XA_T_ING_ERROR)
    {
	/* already reported the error, no more error logging desired */
	return( E_DB_ERROR );
    }

    if (fe_thread_cb_p->ingres_state & state_mask)
    {
	db_status = E_DB_OK;
    }
    else
    {
	db_status = E_DB_ERROR;
    }

    return ( db_status );

}

/*
+* Name:   IIxa_set_ingres_state()
**
** Description:
**     If LIBXA is linked in, and XA support is turned on, this routine
**     sets the thread XA state, and returns the old state in a pointer
**     to facilate pushing and poping of the state while performing
**     internal operations.
**
** Inputs:
**	fe_thread_cx_cb_p	- thread control block, can be NULL
**	new_ingres_state	- state to set INGRES state to.
**	old_ingres_state	- What the INGRES state was before you set it.
**
** Outputs:
**	DB_STATUS.		- did it work or not?
*/
FUNC_EXTERN 
DB_STATUS   IIxa_set_ingres_state(IICX_CB   *fe_thread_cx_cb_p,
					i4	  new_ingres_state,
					i4	  *old_ingres_state)
{
   IICX_CB              *thread_cx_cb_p = fe_thread_cx_cb_p;
   IICX_FE_THREAD_CB    *fe_thread_cb_p;
   DB_STATUS            db_status;

   if (thread_cx_cb_p == NULL)
   {
        db_status = IICXget_lock_fe_thread_cb( NULL, &thread_cx_cb_p );
        if (DB_FAILURE_MACRO( db_status ))
        {
            return( db_status );
        }
    }
    else IICXlock_cb( thread_cx_cb_p );

    fe_thread_cb_p = 
           thread_cx_cb_p->cx_sub_cb.fe_thread_cb_p;

    *old_ingres_state = fe_thread_cb_p->ingres_state; 
    fe_thread_cb_p->ingres_state = new_ingres_state;

    IICXunlock_cb( thread_cx_cb_p );

    return( E_DB_OK );

} /* IIxa_check_set_ingres_state */

/*
+* Name:   IIxa_check_set_ingres_state()
**
** Description:
**     If LIBXA is linked in, and XA support is turned on, this routine
**     checks the thread XA state, makes sure it's in an appropriate state (i.e.
**     there was an xa_start() in this thread earlier for the RMIs under
**     consideration.) 
**     If it doesn't match state_mask, the thread's "ingres state" is moved to 
**     "OUTSIDE". The next xa_start() in this thread will bounce back to
**     the TP system w/an error XAER_OUTSIDE.
**
** Inputs:
**	fe_thread_cx_cb_p	- thread control block, can be NULL
**	state_mask		- bitmask of acceptable/desired states.
**
** Outputs:
**     Returns: 
**        DB_STATUS.
**
** History:
**	03-Nov-1992 - First written (mani)
**	24-feb-1993 (larrym)
**	    Modified to call IIxa_check_ingres_state.
**	24-mar-1993 (larrym)
**	    Modified to call IIxa_set_ingres_state.
**	23-Aug-1993 (larrym)
**	    Fixed bug 54374; If we get called when the 
**	    state is OUTSIDE, we just return (because the error 
** 	    already been reported).
*/

FUNC_EXTERN 
DB_STATUS   IIxa_check_set_ingres_state(IICX_CB   *fe_thread_cx_cb_p,
					i4	  state_mask)
{
   IICX_CB              *thread_cx_cb_p = fe_thread_cx_cb_p;
   IICX_FE_THREAD_CB    *fe_thread_cb_p;
   DB_STATUS            db_status;
   i4			dummy_old_ingres_state;
   char			ebuf[120];

    if (thread_cx_cb_p == NULL)
    {
        db_status = IICXget_lock_fe_thread_cb( NULL, &thread_cx_cb_p );
        if (DB_FAILURE_MACRO( db_status ))
        {
            return( db_status );
        }
    }
    else IICXlock_cb( thread_cx_cb_p );

    fe_thread_cb_p = 
           thread_cx_cb_p->cx_sub_cb.fe_thread_cb_p;

    if ( IIxa_check_ingres_state(thread_cx_cb_p, state_mask) != E_DB_OK )
    {
	  /*  
	  ** NOTE, if IIxa_set_rollback_value fails, then we probably have no
	  ** current transaction, so blow it off (i.e., we don't care about its
	  ** return value). 
	  */
	     
	  switch( fe_thread_cb_p->ingres_state)
	  {
	      case IICX_XA_T_ING_ACTIVE_PEND:
		  if (state_mask & IICX_XA_T_ING_ACTIVE)
		  {
		      /* user failed to SET CONNECTION */
		      /*
                      IIxa_set_ingres_state(
		          thread_cx_cb_p,
		          IICX_XA_T_ING_APPL_ERROR,
		          &dummy_old_ingres_state);
		      */
	              IIxa_error( GE_LOGICAL_ERROR, 
				  E_LQ00D7_XA_AMBIG_CONNECTION, 0, (PTR)0 );
		      IIxa_set_rollback_value( thread_cx_cb_p );
		      break;
		  }
		  /* else do case IICX_XA_T_ING_ACTIVE: */
	      case IICX_XA_T_ING_ACTIVE:
		  /* App tried to execute "INTERNAL only" statement */
		  /*
                  IIxa_set_ingres_state(
		      thread_cx_cb_p,
		      IICX_XA_T_ING_APPL_ERROR,
		      &dummy_old_ingres_state);
		  */
	          IIxa_error( GE_LOGICAL_ERROR, E_LQ00D5_XA_ILLEGAL_STMT, 0,
	              (PTR)0 );
		  IIxa_set_rollback_value( thread_cx_cb_p );
		  break;
              case IICX_XA_T_ING_IDLE:
		  /* ESQL was executed OUTSIDE a valid XN */
                  IIxa_set_ingres_state(
		      thread_cx_cb_p,
	              IICX_XA_T_ING_OUTSIDE,
		      &dummy_old_ingres_state);
	          IIxa_error( GE_LOGICAL_ERROR, E_LQ00D6_XA_OUTSIDE_STMT, 0,
	              (PTR)0 );
		  IIxa_set_rollback_value( thread_cx_cb_p );
		  break;
	      case IICX_XA_T_ING_APPL_ERROR:
	      case IICX_XA_T_ING_OUTSIDE:
		  /* error already reported */
		  break;
	      default:
		  /* something's gone horribly wrong */
		  STprintf(ebuf, ERx("IIxa_check_set_ingres_state detected an invalid ingres_state: 0x%x"), fe_thread_cb_p->ingres_state);
	          IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1, ebuf);
                  IIxa_set_ingres_state(
		      thread_cx_cb_p,
		      IICX_XA_T_ING_APPL_ERROR,
		      &dummy_old_ingres_state);
		  break;
	  } /* switch */
	  db_status = E_DB_ERROR;
    }
    else
	db_status = E_DB_OK;

   IICXunlock_cb( thread_cx_cb_p );

   return( db_status );

} /* IIxa_check_set_ingres_state */





/*{
+*  Name: IIxa_get_and_set_xa_sid  - 
**              Returns the XA sid corresponding to a connection_name.
**
**  Description: 
**      Maps from a connectino_name to the corresponding XA session id. This 
**      function is invoked in the context of a specific AS thread.
**
**  Inputs:
**      thread CB    - pointer, may be NULL. default is caller's thread cntxt
**      connect_name - connection name from the ESQL SET CONNECTION stmt.
**      
**      This routine will do the DYNAMIC REGISTRATION call, once we support
**      that feature, post-65. (i.e. ax_reg()).
**
**  Outputs:
**	Returns:
**	    XA sid that corresponds to the connection_name.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
**	09-mar-1993 (larrym)
**	    modified to use connection names instead of session ids.
**	23-Aug-1993 (larrym)
**	    Modified to reference new element of fe_thread_cb which is the
**	    current xa_xn_thrd cb instead of xa_xn cb.  See iicxxa.sc for
**	    more info.
*/

DB_STATUS
IIxa_get_and_set_xa_sid( IICX_CB             *fe_thread_cx_cb_p,
                         PTR                 connect_name,
                         i4                  *xa_sid )
{
   IICX_CB              *thread_cx_cb_p = fe_thread_cx_cb_p;
   IICX_CB              *xa_xn_cx_cb_p;
   IICX_CB              *xa_xn_thrd_cx_cb_p;
   IICX_FE_THREAD_CB    *fe_thread_cb_p;
   IICX_XA_XN_CB        *xa_xn_cb_p;
   IICX_XA_XN_THRD_CB   *xa_xn_thrd_cb_p;
   int                  rmid;
   IICX_ID              rmi_cx_id;
   DB_STATUS            db_status;


   /* retrieve the current thread and XA XN context */
   if (thread_cx_cb_p == NULL)
   {
     db_status = IICXget_lock_fe_thread_cb( NULL, &thread_cx_cb_p );
      if (DB_FAILURE_MACRO( db_status ))
      {
          return( db_status );
      }
   }
   else IICXlock_cb( thread_cx_cb_p );

    fe_thread_cb_p = 
           thread_cx_cb_p->cx_sub_cb.fe_thread_cb_p;

    /* We would like to allow "esql set connection" statements from  */
    /* within LIBQXA. These will be executed while the fe_thread's   */
    /* state has been set (temporarily) to "internal". It's also     */
    /* already invoked by LIBQXA w/the appropriate session id value  */
    /* There is no "patchup" of session ids needed in this scenario. */
    /* Hence we return here from the call.                           */


    if (fe_thread_cb_p->ingres_state == IICX_XA_T_ING_INTERNAL) {
        /* now that we've gone to connection_names, this function should not */
        /* be called by LIBQXA so this code (the if block) is not needed     */
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
        ERx("IIxa_get_and_set_xa_sid called from INTERNAL mode (internal error)."));
        return( E_DB_ERROR );
    }
    else if (!((fe_thread_cb_p->ingres_state == IICX_XA_T_ING_ACTIVE) ||
               (fe_thread_cb_p->ingres_state == IICX_XA_T_ING_ACTIVE_PEND)))
    {
        *xa_sid = 0;
        IICXunlock_cb( thread_cx_cb_p );
        
        /* log an XA internal error. check ! */
        return( E_DB_ERROR );
    }

   /* retrieve the current xn context */
   xa_xn_thrd_cx_cb_p = fe_thread_cb_p->curr_xa_xn_thrd_cx_cb_p;
   IICXlock_cb( xa_xn_thrd_cx_cb_p );
   xa_xn_thrd_cb_p = xa_xn_thrd_cx_cb_p->cx_sub_cb.xa_xn_thrd_cb_p;
   xa_xn_cx_cb_p = xa_xn_thrd_cb_p->curr_xa_xn_cx_cb_p;

   if (xa_xn_cx_cb_p == NULL)
   {
       IICXunlock_cb( xa_xn_thrd_cx_cb_p );
       IICXunlock_cb( thread_cx_cb_p );
       return( E_DB_ERROR );
   }

   /* first find the rmid that corresponds to this connection_name. Search */
   /* thru the chain of XA RMI CBs for this connection name.          */
   /* 
   **  CHECK: Actually, since we maintain connection_name in the xa_xn_cb, we 
   **  could just skip this step, and modify IICXget_lock_xa_xn_cx_cb_by_rmi
   **  to be IICXget_lock_xa_xn_cx_cb_by_conname.  Something for post beta.
   */

   db_status = IICXget_rmid_by_connect_name( connect_name, &rmid );
   if (DB_FAILURE_MACRO( db_status )) 
   {
       IICXunlock_cb( xa_xn_thrd_cx_cb_p );
       IICXunlock_cb( thread_cx_cb_p );
       return( db_status );
   }

   /* See if the curr XA XN CB is for the rmid of interest. If not, */
   /* note the XID, and search for the XID/rmid of interest.        */
   /* Switch the XA XN CB, if necessary                             */
   db_status = IICXget_lock_xa_xn_cx_cb_by_rmi( rmid, &xa_xn_cx_cb_p );
   if (DB_FAILURE_MACRO( db_status )) 
   {
       IICXunlock_cb( xa_xn_thrd_cx_cb_p );
       IICXunlock_cb( thread_cx_cb_p );
       return( db_status );
   }

   xa_xn_cb_p = xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p;

   /* Switch the XA XN CB, return the proper xa sid                 */
   *xa_sid = xa_xn_cb_p->xa_sid;

   xa_xn_thrd_cb_p->curr_xa_xn_cx_cb_p = xa_xn_cx_cb_p;
   IICXunlock_cb( xa_xn_thrd_cx_cb_p );

   if (fe_thread_cb_p->ingres_state == IICX_XA_T_ING_ACTIVE_PEND) {
       fe_thread_cb_p->ingres_state = IICX_XA_T_ING_ACTIVE;
   }

   /* Unlock the XA XN CB, and the FE THREAD CB */

   IICXunlock_cb( xa_xn_cx_cb_p );
   IICXunlock_cb( thread_cx_cb_p );

    /* if tracing on...*/
    if(IIcx_fe_thread_main_cb->fe_main_flags & IICX_XA_MAIN_TRACE_XA)
    {
        char outbuf[200];

        STprintf(outbuf, 
  ERx("Appl Code:\n  set connection to rmi: %s (INGRES/OpenDTP session %d)\n"),
	    connect_name, *xa_sid);
        IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
        IIxa_fe_qry_trace_handler( (PTR) 0, IIXATRCENDLN, TRUE);
    }


   return( E_DB_OK );

} /* IIxa_get_and_set_xa_sid */

/*{
+*  Name: IIxa_set_rollback_value  - 
**
**  Description: 
**	Set's the rollback value in the current xa_xn_cb based on
**	LIBQ's current understanding of what went wrong.
**
**  Inputs:
**      thread CB    - pointer, may be NULL. default is caller's thread cntxt
**      
**  Outputs:
**	Returns:
**	   DB_STATUS - We only return error if we couldn't lock the cb's.
**
**	Errors:
**  History:
**	25-feb-1993 (larrym)
**	    First written.
**	23-Aug-1993 (larrym)
**	    Fixed bug 54308 - we were trying to get the current transaction
**	    based on the current RMI; we really need to get the current
**	    transaction based on the current xa_sid, which is unique among
**	    all xa_xn_cb's
**	18-Dec-97 (gordy)
**	    LIBQ current session moved to thread-local-storage.
*/
DB_STATUS
IIxa_set_rollback_value ( IICX_CB             *fe_thread_cx_cb_p )
{
    II_THR_CB		*thr_cb;
    II_LBQ_CB		*IIlbqcb;
    IICX_CB              *thread_cx_cb_p = fe_thread_cx_cb_p;
    IICX_CB              *xa_xn_cx_cb_p;
    IICX_CB              *xa_xn_thrd_cx_cb_p;
    IICX_FE_THREAD_CB    *fe_thread_cb_p;
    IICX_XA_XN_CB        *xa_xn_cb_p;
    IICX_XA_XN_THRD_CB   *xa_xn_thrd_cb_p;
    DB_STATUS            db_status;
    int			 rb_value;	/* value is exposed to user; use int */

    /* retrieve the current thread and XA XN context */
    if (thread_cx_cb_p == NULL)
    {
        db_status = IICXget_lock_fe_thread_cb( NULL, &thread_cx_cb_p );
        if (DB_FAILURE_MACRO( db_status ))
        {
            return( db_status );
        }
    }
    else IICXlock_cb( thread_cx_cb_p );

    fe_thread_cb_p =
           thread_cx_cb_p->cx_sub_cb.fe_thread_cb_p;

    /* retrieve the current (or last known) xn context */
    xa_xn_thrd_cx_cb_p = fe_thread_cb_p->curr_xa_xn_thrd_cx_cb_p;
    if (xa_xn_thrd_cx_cb_p != NULL)
    {
        xa_xn_thrd_cb_p = xa_xn_thrd_cx_cb_p->cx_sub_cb.xa_xn_thrd_cb_p;
        xa_xn_cx_cb_p = xa_xn_thrd_cb_p->curr_xa_xn_cx_cb_p; /* could be NULL */
    }
    else
    {
	xa_xn_cx_cb_p = NULL;
    }
 
    if (xa_xn_cx_cb_p == NULL)
    {
        /* 
	** This could have been called on account of an RM_OUTSIDE error 
	** so get the xa_xn_cx_cb associated with the current session
	*/
	IICXget_lock_curr_xa_xn_cx_cb(&xa_xn_cx_cb_p);

	/* if it's still NULL, we haven't even connected yet. */
	if (xa_xn_cx_cb_p == NULL)
	{
            IICXunlock_cb( thread_cx_cb_p );
            return( E_DB_ERROR );
	}
    }

    xa_xn_cb_p = xa_xn_cx_cb_p->cx_sub_cb.xa_xn_cb_p;

    /* set the rollback value (based on the DBMS error */
    thr_cb = IILQthThread();
    IIlbqcb = thr_cb->ii_th_session;
    switch (IIlbqcb->ii_lq_error.ii_er_other)
    {
	case E_US125C_4700:
	    rb_value = XA_RBDEADLOCK;
	    break;
	case E_US125E_4702:
	    rb_value = XA_RBTIMEOUT;
	    break;
	case E_US1261_4705:
	case E_US1262_4706:
	case E_US1263_4707:
	    /* other error */
	    rb_value = XA_RBOTHER;
	    break;
	default:
	    /* unspecified error - could be from ESQL issued OUTSIDE a TX */
	    rb_value = XA_RBROLLBACK;
	    break;
    }
    xa_xn_cb_p->xa_rb_value = rb_value;

    IICXunlock_cb( xa_xn_cx_cb_p );
    IICXunlock_cb( thread_cx_cb_p );

    return( E_DB_OK );
}

