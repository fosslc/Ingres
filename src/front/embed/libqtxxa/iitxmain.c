/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<pc.h>
# include	<si.h>
# include	<sl.h>
# include	<st.h>
# include	<iicommon.h>
# include	<xa.h>
# include       <iixagbl.h>
# include       <iicx.h>
# include       <iicxxa.h>
# include       <iicxfe.h>
# include       <generr.h>
# include       <erlq.h>

/*
**  TUXEDO specific headers
*/
# include       <iitxgbl.h>
# include	<iitxinit.h>
# include	<iitxtms.h>
# include       <iixashm.h>

 
/*
**  Name: iitxmain.c
**
**  Description:
**	Head routine for TUXEDO version of INGRES/Open DTP. If the TUXEDO
**      symbol is switched on at compile time routines in iixamain will have 
**      a MACRO defined which will make a call into this switching routing. 
**      Arguments will be xa_call an action code and a variable number of 
**      arguments. This switch routine will then call the appropriate 
**      TUXEDO-specific function based upon the xa_call being made. 
**
**  Defines:
**      IItux_main - To-Level switch routine for TUXEDO specific add-ons
**      to libqxa
**      
**  Notes:
**	<Specific notes and examples>
**
**  History:
**	04-Oct-93 (mhughes) 
**          First implementation
**	10-Nov-93 (larrym)
**	    Added TMS and AS functionality.  Modified IItux_main to return 
**	    xa_ret_values.
**	11-nov-1993 (larrym)
**	    added process_type to IItux_tms_as_global_cb, moved 
**	    get_process_type() to iitxinit.c
**	03-Dec-1993 (mhughes)
**	    Log server process boot & shutdown
**	24-Dec-1993 (mhughes)
**	    Added rmi cb and rmid as 2nd & 3rd arguments to IItux_icas_open() 
**	    for IItux_tms_recover call
**	07-Jan-1994 mhughes)
**	    Add  argument to IItux_userlog calls.
**	10-jan-1994 (larrym)
**	    Added rmid to IItux_tms_recover.
**	08-Apr-1994 (mhughes)
**	    Added Recovery testing suicide point(s).
**      16-Jan-1995 (stial01)
**          Updated for new INGRES-TUXEDO architecture.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Jul-2008 (hweho01)
**	    For the function call IItux_tms_coord_AS_2phase(), replace
**	    the parameter data type from XID to DB_XA_DIS_TRAN_ID which
**	    is defined in iicommon.h.
*/

/*
**  Name: IItux_main
**
**  Description:
**      Call appropriate Tuxedo specific routine depending upon xa_call and 
**      requested action.
**
**  Inputs:
**      xa_call    - Current xa_call identifier (see list in iixagbl.h)
**      action     - Sequential count for each xa_call type.
**                   eg First tuxedo-specific call in xa_open will be action 
**                   one, second action two etc. Beware the first action 
**                   recieved for an process type may not be action one.
**      args       - Variable number of arguments, passed on to called function
**
**  Outputs:
**	Returns:
**          DB_STATUS 
**  Side Effects:
**	    None.
**	
**  History:
**	04-Oct-93 (mhughes)
**          First implementtaion.
**	10-Nov-93 (larrym)
**	    Added TMS and AS functionality.  Modified IItux_main to return 
**	    xa_ret_values.
**	11-nov-1993 (larrym)
**	    added process_type to IItux_tms_as_global_cb.  modified this 
**	    function to deal with it.
**      16-Nov-1993 (mhughes)
**	    Changed function declaration to integer as it's returning an
**	    xa error status. Tidied ICAS cases a bit.
**      16-Jan-1995 (stial01)
**          Updated for new INGRES-TUXEDO architecture.
*/
int
IItux_main( xa_call, xa_action, 
            arg1, arg2, arg3, arg4, 
            arg5, arg6, arg7, arg8 
           )
i4  xa_call;
i4  xa_action;
PTR arg1;
PTR arg2;
PTR arg3;
PTR arg4;
PTR arg5;
PTR arg6;
PTR arg7;
PTR arg8;
{
    int	      xa_ret_value = XA_OK;

    if (IItux_tms_as_global_cb == NULL)
    {
        if( (xa_call == IIXA_OPEN_CALL) 
	  &&(xa_action    == IITUX_ACTION_ONE))
	{
	    /*
	    ** Classic chicken & egg problem.  Can't do the switch
	    ** until the IItux_tms_as_global_cb is alloc'd.
	    */
            xa_ret_value = IItux_init( arg1 );
            if (xa_ret_value != XA_OK)
            {
	        return (xa_ret_value );
            }
	}
	else
	{
            IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
            ERx("IItux_main was passed a bad action code. Internal Error."));
	    return (XAER_RMERR);
	}
    }

    if (!IItux_lcb.lcb_tux_main_cb)
    {
        IIxa_error( GE_LOGICAL_ERROR, E_LQ00D1_XA_USER_OR_TM_ERR, 1,
           ERx("tux_main: LIBQTXXA not initialized properly earlier."));
        return( XAER_PROTO );
    }

#ifdef TUXEDO_RECOVERY_TEST
    /*
    ** If Recovery testing & fail point MAIN_TOP specified...... Die
    */
    if ( IItux_tms_as_global_cb->failure_point == IITUX_FAIL_MAIN_TOP )
    {
	PCexit();
    }
#endif /* TUXEDO_RECOVERY_TEST */ 

    switch( xa_call )
    {

      case IIXA_OPEN_CALL:
	  switch( xa_action )
	  {
	  case IITUX_ACTION_ONE:
	      /* only called once per process.  Inits data structures */
	      /* xa_ret_value = IItux_init( arg1 ); */
	      xa_ret_value = XA_OK;
	      break;
	  case IITUX_ACTION_TWO:
	      /*
	      ** called for each open
	      ** this will check the lgmo table and recover partial XIDs
	      ** xa_ret_value = IItux_tms_init( (IICX_XA_RMI_CB *)arg1 );
	      */
	      xa_ret_value = XA_OK;
	      break;
          default:
              IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
              ERx("IItux_main was passed a bad action code. Internal Error."));
	      xa_ret_value = XAER_RMERR;
	      break;
	  }
          break;

      case IIXA_CLOSE_CALL:
          switch( xa_action )
	  {
	  case IITUX_ACTION_ONE:
	      /*
	      ** only called once per process
	      ** deallocate global control block 
	      */
	      xa_ret_value = IItux_shutdown( );
	      break;
          default:
              IIxa_error( GE_LOGICAL_ERROR, E_LQ00D0_XA_INTERNAL, 1,
              ERx("IItux_main was passed a bad action code. Internal Error."));
	      xa_ret_value = XAER_RMERR;
	      break;
	  }
          break;

      case IIXA_ROLLBACK_CALL:
      case IIXA_COMMIT_CALL:
      case IIXA_PREPARE_CALL:

          switch( xa_action )
	  {
	  case IITUX_ACTION_ONE:
              xa_ret_value = IItux_tms_coord_AS_2phase( 
		  xa_call, (DB_XA_DIS_TRAN_ID *) arg1, *(int *)arg2, 
		  *(long *)arg3 );

	      break;
          default:
	      xa_ret_value = XAER_RMERR;
	      break;
	  }
          break;


      case IIXA_RECOVER_CALL:
          switch( xa_action )
	  {
	  case IITUX_ACTION_ONE:
	      /* get lgmo table in sync with TUXEDO */
              xa_ret_value = IItux_tms_recover( *(int *)arg1, 
						(IICX_XA_RMI_CB *)arg2 );
	      break;
          default:
	      xa_ret_value = XAER_RMERR;
	      break;
	  }
          break;


      default:
	  xa_ret_value = XAER_RMERR;
          break;

    }  
      

    return( xa_ret_value );
}
