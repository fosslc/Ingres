/*
** Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <gl.h>
# include       <sl.h>
# include       <er.h>
# include       <iicommon.h>
# include       <si.h>
# include       <me.h>
# include       <st.h>
# include	<fe.h>
# include	<xa.h>
# include	<iixagbl.h>
# include       <iicx.h>
# include	<iicxxa.h>
# include	<iicxfe.h>
# include	<iixamain.h>
# include       <generr.h>
# include       <erlq.h>

/* TUXEDO specific headers */
# include	<iitxgbl.h>
# include	<iitxtms.h>
# include       <pc.h>
# include       <iixashm.h>


/*
**  Name:  iitxtms.c - TUXEDO TMS functions.
**
**  Description:
**	This file contains the routines necessary to support TUXEDO TMS
**	functionality.  The routines in this file are called indirectly
**	from iixamain (front!embed!libqxa) via the TUXEDO switch in
**	iitxmain.c (front!embed!libqtxxa).
**
**  Defines:
**	IItux_tms_coord_AS_2phase	- Coordinates TUXEDO transaction
**					  demarcation amoung AS'
**	IItux_tms_recover		- called by IIxa_recover; get's
**					  database into a state consistent
**					  with TUXEDO's logs.
**
**  Notes:
**
**  History:
**	02-nov-1993 (larrym)
**	    written.
**	03-Dec-1993 (mhughes)
**	    Renamed icas_purge_svn to icas_purge_xid.
**	07-Jan-1994 (mhughes) 
**	    Added debug mode. Stash tpcall flags in tuxedo global cb.
**	12-Jan-1994 (mhughes)
**	    Modified tpcall to send tpcall & tprply flags.
**      01-Jan-1995 (stial01)
**          Deleted functions not needed for new INGRES-TUXEDO
**          architecture.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-Aug-2009 (kschendel) 121804
**	    Need fe.h to satisfy gcc 4.3.
*/


/*
**  Name:  IItux_tms_recover - 
**
**  Description:
**	This function is called as of a call to xa_recover.  It goes through
**	the lgmo_xa_dis_tran_ids table to enure that any 2phase operation 
**	started by us, but not finished, is finished.  Refere to the
**	Tuxedo Bridge Architechture and Design Spec, section 4.2, for
**	the rationale.
**
**  Inputs:
**	IICX_XA_RMI_CB		- The CB of the RMI to recover.  
**
**  Outputs:
**	Returns:	
**	    xa_ret_value
**
**  Side Effects:
**	None.
**
**  History:
**	04-Nov-1993 (larrym)
**	    Written.
**      22-Dec-1993 (larrym)
**          Added recovery functionality
**	12-May-1994 (mhughes)
**	    Modified recovery procedure. Again, we're not guessing 
**	    branch_flags correctly. 
*/
int
IItux_tms_recover( int rmid, IICX_XA_RMI_CB *rmi)
{
    int				xa_ret_value = XA_OK;
    IITUX_TMS_2P_ACTION		*iitux_tms_xid_action = NULL;
    IITUX_TMS_2P_ACTION_LIST	iitux_tms_2P_action_list;
    i4				rec_count = 0;
    i4				xid_count;
    i4				tp_flags;
    i4				sum_flags;
    i4				seqnum;
    i4				min_seqnum;
    i4				max_seqnum;
    i4				size_of_xid;


    /* get tag for this action list (so we can free the whole thing later) */
    iitux_tms_2P_action_list.action_tag = FEgettag();
    iitux_tms_2P_action_list.action_list = NULL;
    iitux_tms_2P_action_list.action_pool = NULL;

    /* get list of XIDs to work on */
    xa_ret_value = IItux_build_rec_action_table( 
			rmi,
			&iitux_tms_2P_action_list, 
			&rec_count);
    if (xa_ret_value != XA_OK)
    {
	/* CHECK error */
	IIUGtagFree(iitux_tms_2P_action_list.action_tag);
	return (xa_ret_value);
    }

    for (iitux_tms_xid_action=iitux_tms_2P_action_list.action_list;
	 iitux_tms_xid_action != NULL;
	 iitux_tms_xid_action = iitux_tms_xid_action->action_next)
    {
	switch (iitux_tms_xid_action->tp_flags & 
		( DB_XA_EXTD_BRANCH_FLAG_2PC 
		| DB_XA_EXTD_BRANCH_FLAG_FIRST 
		| DB_XA_EXTD_BRANCH_FLAG_LAST)
	       )
	{
    	    case (DB_XA_EXTD_BRANCH_FLAG_2PC 
		| DB_XA_EXTD_BRANCH_FLAG_FIRST 
		| DB_XA_EXTD_BRANCH_FLAG_LAST):
		/* it's all there, let TUXEDO deal with it */
                if( IIcx_fe_thread_main_cb->fe_main_flags & 
                    IICX_XA_MAIN_TRACE_XA)
                {
		    char	outbuf[256];
		    char	xidstr[256];

            	    IICXformat_xa_xid(
		      (DB_XA_DIS_TRAN_ID *)&(iitux_tms_xid_action->xid),
		        xidstr );
	            STpolycat (3 ,ERx("\n\tNo action needed for XID: "), xidstr, ERx("\n"), outbuf);
                    IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
                }
		break;

	    case (DB_XA_EXTD_BRANCH_FLAG_FIRST): 
	    case (DB_XA_EXTD_BRANCH_FLAG_LAST 
	        | DB_XA_EXTD_BRANCH_FLAG_FIRST): 
	    case (DB_XA_EXTD_BRANCH_FLAG_2PC 
	        | DB_XA_EXTD_BRANCH_FLAG_FIRST): 
		/* no TP_LAST.  died during commit */
		/* commit them decending */
                if( IIcx_fe_thread_main_cb->fe_main_flags & 
                    IICX_XA_MAIN_TRACE_XA)
                {
		    char	outbuf[256];
		    char	xidstr[256];

            	    IICXformat_xa_xid(
		      (DB_XA_DIS_TRAN_ID *)&(iitux_tms_xid_action->xid),
		        xidstr );
	            STpolycat (3,
			       ERx("\n\tCommitting XID: "), 
			       xidstr, 
			       ERx("\n"), 
			       outbuf);
                    IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
                }

	    for (seqnum = iitux_tms_xid_action->last_seq;
		 (seqnum >= iitux_tms_xid_action->first_seq &&
		  xa_ret_value == XA_OK);
		 seqnum--)
	    {
		/* this should reconnect to the db and commit */
		/* 
		** It needs to be more flexible to account for 
		** AS's still being up. CHECK 
		*/

		/* 
		** For now, you need to reconnect with the same
		** branch_flag as you prepared with.  We should remove
		** that restriction, but for now, we'll figure out 
		** what the branch_flag was and pass it in.
		*/
		sum_flags = iitux_tms_xid_action->tp_flags;
		min_seqnum = iitux_tms_xid_action->first_seq;
		max_seqnum = iitux_tms_xid_action->last_seq;

		tp_flags = DB_XA_EXTD_BRANCH_FLAG_NOOP;
		    
		if (seqnum == max_seqnum)
		{
		    if (sum_flags & DB_XA_EXTD_BRANCH_FLAG_LAST)
		    {
			tp_flags |= DB_XA_EXTD_BRANCH_FLAG_LAST;
		    }

		    if ( seqnum == 0 )
		    {
			if ( sum_flags & DB_XA_EXTD_BRANCH_FLAG_FIRST )
			    tp_flags |= DB_XA_EXTD_BRANCH_FLAG_FIRST;
			    
			if ( sum_flags & DB_XA_EXTD_BRANCH_FLAG_2PC )
			    tp_flags |= DB_XA_EXTD_BRANCH_FLAG_2PC;
		    }
		}
		else if (seqnum == 0)
		{
		    if ( max_seqnum > 0 )
			tp_flags = (sum_flags & ~DB_XA_EXTD_BRANCH_FLAG_LAST);
		    else
			tp_flags = sum_flags;
		}
		    
		xa_ret_value = IIxa_do_2phase_operation(IIXA_COMMIT_CALL,
							&iitux_tms_xid_action->xid,
							rmid,
							TMNOFLAGS,
							seqnum,
							tp_flags );
	    }
	    /* CHECK, return error ? */
	    break;
	case DB_XA_EXTD_BRANCH_FLAG_LAST:
	    /* died during prepare or rollback */
	    /* roll them back ascending */
	    if( IIcx_fe_thread_main_cb->fe_main_flags & 
	       IICX_XA_MAIN_TRACE_XA)
	    {
		char	outbuf[256];
		char	xidstr[256];

            	    IICXformat_xa_xid(
		      (DB_XA_DIS_TRAN_ID *)&(iitux_tms_xid_action->xid),
		        xidstr );
	            STpolycat (3 ,ERx("\n\tAborting XID: "), xidstr, ERx("\n"), outbuf);
                    IIxa_fe_qry_trace_handler( (PTR) 0, outbuf, TRUE);
                }
		for (seqnum = iitux_tms_xid_action->first_seq;
		     (seqnum <= iitux_tms_xid_action->last_seq &&
		     xa_ret_value == XA_OK);
		     seqnum++)
		{
		    /* this should reconnect to the db and commit */
		    /* 
		    ** It needs to be more flexible to account for 
		    ** AS's still being up. CHECK 
		    */

		    /* 
		    ** For now, you need to reconnect with the same
		    ** branc_flag as you prepared with.  We should remove
		    ** that restriction, but for now, we'll figure out 
		    ** what the branch_flag was and pass it in.
		    */
		    sum_flags = iitux_tms_xid_action->tp_flags;
		    min_seqnum = iitux_tms_xid_action->first_seq;
		    max_seqnum = iitux_tms_xid_action->last_seq;

		    tp_flags = DB_XA_EXTD_BRANCH_FLAG_NOOP;

		    if (seqnum == max_seqnum)
		    {
			if (sum_flags & DB_XA_EXTD_BRANCH_FLAG_LAST)
			{
			    tp_flags |= DB_XA_EXTD_BRANCH_FLAG_LAST;
			}

			if ( seqnum == 0 )
			{
			    if ( sum_flags & DB_XA_EXTD_BRANCH_FLAG_FIRST )
				tp_flags |= DB_XA_EXTD_BRANCH_FLAG_FIRST;
			    
			    if ( sum_flags & DB_XA_EXTD_BRANCH_FLAG_2PC )
				tp_flags |= DB_XA_EXTD_BRANCH_FLAG_2PC;
			}
		    }
		    else if (seqnum == 0)
		    {
			if ( max_seqnum > 0 )
			    tp_flags = (sum_flags & 
					~DB_XA_EXTD_BRANCH_FLAG_LAST);
			else
			    tp_flags = sum_flags;
		    }

		    xa_ret_value = IIxa_do_2phase_operation( 
							    IIXA_ROLLBACK_CALL,
							    &(iitux_tms_xid_action->xid),
							    rmid,
							    TMNOFLAGS,
							    seqnum,
							    tp_flags );
		}
	    /* CHECK, return error ? */
	    break;
	default:
	    /* way massive hemorage CHECK */;
	}
    }

    /* anything left in the lgmo table will be processed by xa_recover */

    /* free up the action list */
    IIUGtagFree(iitux_tms_2P_action_list.action_tag);
    return (xa_ret_value);
}

/*
**  Name:  IItux_tms_coord_AS_2phase - Coordinate TUXEDO transaction closure 
**				       For a given XID within a group.
**
**  Description:
**	This function is called as the result of a call to IIxa_prepare,
**	IIxa_commit, or IIxa_rollback.  This function takes responsibility
**	for doing the work for all the AS's that may be involved in
**	the transaction, and also guarantees database consistency and 
**	atomicity of the operation.  The theory of operation of this function
**	is discussed in the TUXEDO Bridge Architecture and Design Specification
**	section 4.2.2, TMS Operation.
**      This routine differs from the original proposal as follows:
**         This process will do the prepare/commit/rollback (with xa_xid=)
**         for all the AS's that may be involved int the transaction. 
**
**  Inputs:
**	xa_call		- type of operation (commit, rollback, prepare)
**	xa_xid		- The XID being purged, which comprises
**	    format_id		- long.
**	    bqual_length	- length of the bqual portion.  1-64
**	    gtrid_length	- length of the gtrid portion.  1-64
**	    data		- The actual XA transaction ID.
**	    rmid		- The RMID as passed in by TUXEDO.
**	flags		- The flags argument from the tm's xa_call
**
**  Outputs:
**	tux_ret_value	- Error number if return status is TP_FAIL
**	Returns:
**	     TP_SUCCESS		- normal execution
**	     TP_FAIL		- abnormal execution
**
**  Side Effects:
**	None.
**
**  History:
**	05-Nov-1993 (larrym)
**	    Written.
**	10-May-1994 (mhughes)
**	    When reattatching to a thread. If it's the one & only branch_flag 
**	    should be set to DB_XA_EXT_BRANCH_FLAG_FIRST,
**	    DB_XA_EXT_BRANCH_FLAG_2PC and DB_XA_EXT_BRANCH_FLAG_LAST.
**	    Also if a commit/rollback fails because the "get" call to the
**	    ICAS failed, this may be initiated during xa_recover processing
**	    so attempt to grab the thread ourself.
**      01-Jan-1995 (stial01)
**          Updated for new INGRES-TUXEDO architecture.
**      11-Jul-2008 (hweho01)
**          For the parameter list in IItux_tms_coord_AS_2phase(), replace 
**          XID with DB_XA_DIS_TRAN_ID which is defined in iicommon.h.
*/
int
IItux_tms_coord_AS_2phase( i4  xa_call, DB_XA_DIS_TRAN_ID *xa_xid,
		        	int rmid, long flags)
{

    i4				i; 	/* max is 256 */
    i4				num_of_svns = 0;
    int				xa_ret_value;
    i4                          first_seq, last_seq;
    i4                          branch_flag;
    long                        xa_flags;
    IITUX_XN_ENTRY              *max_xn_entry;

    /* Determine the number of AS involved in the transaction */
    max_xn_entry = IItux_max_seq((DB_XA_DIS_TRAN_ID *)xa_xid);
    if (max_xn_entry)
	num_of_svns = last_seq = max_xn_entry->xn_seqnum;
    else
	num_of_svns = 0;

    first_seq = 1;
    xa_flags = flags;

    /*
    ** Go to work
    */
    if(num_of_svns == 0)
    {
	/*
	** Might be a recovery operation.  Try to recover. 
	** If that fails then return XAER_NOTA
	*/
	xa_ret_value = IItux_tms_recover_xid( xa_call, xa_xid, rmid);

        if (xa_ret_value == XAER_NOTA)
        {
	    char ebuf[255];
	    STprintf(ebuf,
		     ERx("No AS's registered in shared memory for XID: %s"),
		     xa_xid);
	    IIxa_error(GE_NO_RESOURCE, E_LQ00D1_XA_USER_OR_TM_ERR, 1, ebuf,
			   (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
	    return(XAER_NOTA);
	}
	else
	{
	    return(xa_ret_value);
	}
    }

    if(xa_call == IIXA_PREPARE_CALL )
	max_xn_entry->xn_state |= TUX_XN_2PC;

    /*
    **  This code is designed to work for both SINGLE and MULTI-AS 
    **  scenarios
    */
    switch(xa_call)
    {
	case IIXA_COMMIT_CALL:
	    if ((flags == TMONEPHASE) && (num_of_svns > 1))
	    {
		/*  
		** prepare decending (without the ...2PC flag) then commit.
		*/
                xa_flags = TMNOFLAGS;
                branch_flag = (long)DB_XA_EXTD_BRANCH_FLAG_LAST; 

		for( i = last_seq;
		     i >= first_seq;
		     i--, branch_flag = (long)DB_XA_EXTD_BRANCH_FLAG_NOOP) 
		{
		    if ( i == first_seq )
		    {
    	                branch_flag |= (long)DB_XA_EXTD_BRANCH_FLAG_FIRST;
		    }
		    xa_ret_value = IIxa_do_2phase_operation(
					(i4)IIXA_PREPARE_CALL,
					xa_xid, rmid, xa_flags, 
					i, branch_flag);
	            if (xa_ret_value != XA_OK)
	            {
			i4	rbndx;

			/* 
		        ** we need to ROLLBACK this XID and return RBCOMMFAIL 
			** CHECK should we try to rollback i (the one that
			** failed) ?
			*/

			for ( rbndx = i + 1; 
			      rbndx <= last_seq;
			      rbndx ++ )
			{
			    /* FIX ME fix branch_flag ?? */
			    xa_ret_value = IIxa_do_2phase_operation(
						(i4)IIXA_ROLLBACK_CALL,
						xa_xid, rmid, xa_flags,
						rbndx, branch_flag);
	                   if (xa_ret_value != XA_OK)
			   {
			       /* all hell's breaking loose */
	                       return(XAER_RMERR); /* serious error */
			   }
			}

	                return(XA_RBCOMMFAIL);
	            } /* prepare failed, all rolled back */
		} /* for */
		/*
		** now go on to commit them
		*/
	    } /* if TMONEPHASE */

	    /*
	    ** In either case, we need to commit them (descending)
	    */
	    
	    branch_flag |= DB_XA_EXTD_BRANCH_FLAG_LAST; 

	    for ( i = last_seq; 
		i >= first_seq;
		i--, branch_flag = (long)DB_XA_EXTD_BRANCH_FLAG_NOOP)
	    {
		if (i == first_seq)
		{
		    if (flags == TMONEPHASE)
		    {
			/* CHECK does a TMS ever get TMONEPHASE flag? 
			** also does the txn ever get prepared? & if not
			** there won't be any orphaned threads to fix up
			*/
			branch_flag |= DB_XA_EXTD_BRANCH_FLAG_FIRST;
		    }
		    else
		    {
			branch_flag |= (DB_XA_EXTD_BRANCH_FLAG_FIRST |
					     DB_XA_EXTD_BRANCH_FLAG_2PC);
		    }
		}

		xa_ret_value = IIxa_do_2phase_operation (
			      (i4)IIXA_COMMIT_CALL,
			      xa_xid, rmid, xa_flags, i, branch_flag);

		if (xa_ret_value != XA_OK)
		{
		    /* 
		    ** Can't commit it.  May have committed other AS's.
		    ** Tell the TPsystem about it.
		    */
		    if (i == last_seq)
		    {
			/* no damage done yet */
			if (flags != TMONEPHASE)
			{
			    /* since it's prepared, can try again later */
			    return(XA_RETRY);
			}
			else
			{
			    return(xa_ret_value);
			}
		    }
		    else
		    {
			/* we've probably committed a few of them */
			return(XA_HEURHAZ);
		    }
		}
	    } /* for */
	    break;

	case IIXA_PREPARE_CALL:
	    /* prepare them descending with branch_flag | ...2PC */
	    /* validate flags == TMNOFLAGS */
	    if (flags != TMNOFLAGS)
	    {
		char ebuf[120];

		STprintf(ebuf, 
			 ERx("XA_PREPARE called with invalid flags: %x"),
			 flags);
	        IIxa_error(GE_NO_RESOURCE, E_LQ00D1_XA_USER_OR_TM_ERR, 1, ebuf,
			    (PTR) 0, (PTR) 0, (PTR)0, (PTR) 0);
		return(XAER_PROTO);
	    }

	    for( i = last_seq, branch_flag = DB_XA_EXTD_BRANCH_FLAG_LAST;
		 i >= first_seq;
    	         i--, branch_flag = (long)DB_XA_EXTD_BRANCH_FLAG_NOOP)
	    {
		if ( i == first_seq )
		{
    	            branch_flag |= (DB_XA_EXTD_BRANCH_FLAG_2PC | 
					DB_XA_EXTD_BRANCH_FLAG_FIRST);
		}
		xa_ret_value = IIxa_do_2phase_operation((i4)IIXA_PREPARE_CALL,
				    xa_xid, rmid, xa_flags, i, branch_flag);
	        if (xa_ret_value != XA_OK)
	        {
		    /* Tuxedo should call us to ROLLBACK this XID */
	            return(XAER_RMERR);
	        }
	    } /* for */
	    break;

        case IIXA_ROLLBACK_CALL:

	    branch_flag = DB_XA_EXTD_BRANCH_FLAG_NOOP;
	    if (max_xn_entry->xn_state & TUX_XN_2PC)
	    {
		 branch_flag |= (DB_XA_EXTD_BRANCH_FLAG_FIRST |
				    DB_XA_EXTD_BRANCH_FLAG_2PC);
	    }
	    else
	    {
		branch_flag |= DB_XA_EXTD_BRANCH_FLAG_FIRST;
	    }

	    /* rollback ascending */
	    for( i = first_seq;
		 i <= last_seq;
		 i++, branch_flag = DB_XA_EXTD_BRANCH_FLAG_NOOP) 
	    {
		if (i == last_seq)
		{
		    branch_flag |= DB_XA_EXTD_BRANCH_FLAG_LAST;
		}

		xa_ret_value = IIxa_do_2phase_operation (
			      (i4)IIXA_ROLLBACK_CALL,
			      xa_xid, rmid, xa_flags, i, branch_flag);

		if (xa_ret_value != XA_OK)
		{
		    return(xa_ret_value);
		}
	    } /* for */
	    break;
    } /* switch (xa_call) */

    return(xa_ret_value);
}
