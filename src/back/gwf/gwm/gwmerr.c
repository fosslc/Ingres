/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <dudbms.h>
# include    <er.h>
# include    <cm.h>
# include    <cs.h>
# include    <st.h>
# include    <tm.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <adf.h>
# include    <scf.h>
# include    <dmf.h>
# include    <dmucb.h>
# include    <dmrcb.h>
# include    <dmtcb.h>
# include    <gwf.h>
# include    <gwfint.h>
# include    <gca.h>
# include    "gwmint.h"


/**
** Name:	gwmerr.c -- error functions for GWM
**
** Description:
**	This file defines some prototyped error handling functions for
**	varying numbers of arguments.  What's done here could be generalized
**	for use elsewhere in GWF.
**
** Exports the following functions:
**
**	GM_uerror	-- general error routine, arbitrary parameters.
**	GM_error	-- log internal error, no parameters.
**	GM_0error	-- handle any error, no parameters.
**	GM_1error	-- handle any error, 1 parameter.
**	GM_2error	-- handle any error, 2 parameters.
**	GM_3error	-- handle any error, 3 parameters.
**	GM_4error	-- handle any error, 4 parameters.
**	GM_5error	-- handle any error, 5 parameters.
**	GM_6error	-- handle any error, 6 parameters.
**	GM_7error	-- handle any error, 7 parameters.
**	GM_8error	-- handle any error, 8 parameters.
**	GM_9error	-- handle any error, 9 parameters.
**	GM_10error	-- handle any error, 10 parameters.
**
** History:
**	2-Feb-1993 (daveb)
**	    Improved comments, ERx the default error string.
**	21-May-1998 (hanch04)
**	    Removed declaration of ule_format
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
**/


/*{
** Name:	GM_uerror -- error subroutine with up to 10 arguments.
**
** Description:
**	Format and send an error to the right places, substituting given
**	parameters.  This is inconvenient to call directly; instead, you should
**	use one of the GM_Nerror functions that take the number of arguments
**	you are handing in and let it format up the args to this function.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	num_parm	Number of parmeters
**	psizeN		Size of parmN
**	pvalN		ParmN.
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented, ERx default error string.
*/
VOID
GM_uerror ( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type, i4  num_parm,
	   i4 psize1, PTR pval1,
	   i4 psize2, PTR pval2,
	   i4 psize3, PTR pval3,
	   i4 psize4, PTR pval4,
	   i4 psize5, PTR pval5,
	   i4 psize6, PTR pval6,
	   i4 psize7, PTR pval7,
	   i4 psize8, PTR pval8,
	   i4 psize9, PTR pval9,
	   i4 psize10, PTR pval10 )
{
    i4             uletype;
    i4		ulecode;
    DB_STATUS           uleret;
    DB_SQLSTATE		sqlstate;
    i4		msglen;
    DB_STATUS		ret_val = E_DB_OK;
    char		errbuf[1024];
    SCF_CB		scf_cb;

    /* don't incr error count here, because incr function
       calls us directly. */

    for (;;)	/* Not a loop, just gives a place to break to */
    {
	/*
	** Log internal errors and errors which the caller asked to be
	** logged.
	*/
	if ((error_type & GM_ERR_INTERNAL) || (error_type & GM_ERR_LOG))
	    uletype = ULE_LOG;
	else
	    uletype = 0L;

	/*
	** Get error message text.  
	*/
	uleret = ule_format(error_no,
			    0L,
			    uletype,
			    &sqlstate,
			    errbuf,
			    (i4) sizeof(errbuf),
			    &msglen,
			    &ulecode,
			    num_parm,
			    psize1, pval1,
			    psize2, pval2,
			    psize3, pval3,
			    psize4, pval4,
			    psize5, pval5,
			    psize6, pval6,
			    psize7, pval7,
			    psize8, pval8,
			    psize9, pval9,
			    psize10, pval10 );

	/*
	** If ule_format failed, we probably can't report any error.
	** Instead, just propagate the error up to the user.
	*/
	if (uleret != E_DB_OK)
	{
		STprintf(errbuf,
		 ERx("GWM Gateway: Error message for %d (X%x) not found in INGRES error file"),
			 error_no, error_no);
		msglen = STlength(errbuf);
		ule_format(0L, 0L, ULE_MESSAGE|ULE_LOG, NULL, errbuf, msglen, 0,
		    &ulecode, 0);
	}
	/*
	**	Only send to session if USER or INTERNAL
	*/
	if(error_type & (GM_ERR_INTERNAL|GM_ERR_USER))
	{
		scf_cb.scf_length = sizeof(scf_cb);
		scf_cb.scf_type = SCF_CB_TYPE;
		scf_cb.scf_facility = DB_GWF_ID;
		scf_cb.scf_session = DB_NOSESSION;
		scf_cb.scf_nbr_union.scf_local_error = error_no;
		STRUCT_ASSIGN_MACRO(sqlstate,scf_cb.scf_aux_union.scf_sqlstate);
		scf_cb.scf_len_union.scf_blength = msglen;
		scf_cb.scf_ptr_union.scf_buffer = errbuf;
		ret_val = scf_call(SCC_ERROR, &scf_cb);
		if (ret_val != E_DB_OK)
		{
		    if (gwx_rcb!=NULL)
		    {
			gwx_rcb->xrcb_error.err_code =
			    scf_cb.scf_error.err_code;
		    }
		    break;
		}
	}
	break;
    }
    return;
}



/*{
** Name: GM_error	- log error encountered in gw processing.
**
** Description:
**	Log an error message to errlog.log, but doesn't
**	send it to the user.  It's equivalent to GM_0error with GM_ERR_INTERNAL.
**	as the destination.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	error_number	status value to log.
**
** Outputs:
**	none.
**
** Returns:
**	none
**
** History:
**	22-sep-92 (daveb)
**		documented
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
VOID
GM_error( STATUS error_number )
{
    GM_0error( (GWX_RCB *)0, error_number, GM_ERR_INTERNAL );
}


/*{
** Name:	GM_0error	-- error function with no parameters
**
** Description:
**	Format and send an error to the right place, with no given
**	substitution parameters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_0error( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0 );
}


/*{
** Name:	GM_1error	-- error function with 1 parameter
**
** Description:
**	Format and send an error to the right place, with 1 given
**	substitution parameter.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	psize1		size of parm1
**	pval1		value of parm1
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_1error( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type,
	  i4 psize1, PTR pval1 )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 1,
	      psize1, pval1,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0 );
}


/*{
** Name:	GM_2error	-- error function with 2 parameters
**
** Description:
**	Format and send an error to the right place, with 2 given
**	substitution parameters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	psize1		size of parm1
**	pval1		value of parm1
**	psize2		size of parm2
**	pval2		value of parm2
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_2error ( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type,
	   i4 psize1, PTR pval1,
	   i4 psize2, PTR pval2 )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 2,
	      psize1, pval1,
	      psize2, pval2,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0 );
}


/*{
** Name:	GM_3error	-- error function with 3 parameters
**
** Description:
**	Format and send an error to the right place, with 3 given
**	substitution parameters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	psize1		size of parm1
**	pval1		value of parm1
**	psize2		size of parm2
**	pval2		value of parm2
**	psize3		size of parm3
**	pval3		value of parm3
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_3error ( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type,
	   i4 psize1, PTR pval1,
	   i4 psize2, PTR pval2,
	   i4 psize3, PTR pval3 )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 3,
	      psize1, pval1,
	      psize2, pval2,
	      psize3, pval3,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0 );
}


/*{
** Name:	GM_4error	-- error function with 4 parameters
**
** Description:
**	Format and send an error to the right place, with 4 given
**	substitution parameters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	psize1		size of parm1
**	pval1		value of parm1
**	psize2		size of parm2
**	pval2		value of parm2
**	psize3		size of parm3
**	pval3		value of parm3
**	psize4		size of parm4
**	pval4		value of parm4
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_4error ( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type,
	   i4 psize1, PTR pval1,
	   i4 psize2, PTR pval2,
	   i4 psize3, PTR pval3,
	   i4 psize4, PTR pval4 )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 4,
	      psize1, pval1,
	      psize2, pval2,
	      psize3, pval3,
	      psize4, pval4,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0 );
}


/*{
** Name:	GM_5error	-- error function with 5 parameters
**
** Description:
**	Format and send an error to the right place, with 5 given
**	substitution parameters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	psize1		size of parm1
**	pval1		value of parm1
**	psize2		size of parm2
**	pval2		value of parm2
**	psize3		size of parm3
**	pval3		value of parm3
**	psize4		size of parm4
**	pval4		value of parm4
**	psize5		size of parm5
**	pval5		value of parm5
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_5error ( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type,
	   i4 psize1, PTR pval1,
	   i4 psize2, PTR pval2,
	   i4 psize3, PTR pval3,
	   i4 psize4, PTR pval4,
	   i4 psize5, PTR pval5 )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 5,
	      psize1, pval1,
	      psize2, pval2,
	      psize3, pval3,
	      psize4, pval4,
	      psize5, pval5,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0 );
}


/*{
** Name:	GM_6error	-- error function with 6 parameters
**
** Description:
**	Format and send an error to the right place, with 6 given
**	substitution parameters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	psize1		size of parm1
**	pval1		value of parm1
**	psize2		size of parm2
**	pval2		value of parm2
**	psize3		size of parm3
**	pval3		value of parm3
**	psize4		size of parm4
**	pval4		value of parm4
**	psize5		size of parm5
**	pval5		value of parm5
**	psize6		size of parm6
**	pval6		value of parm6
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_6error ( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type,
	   i4 psize1, PTR pval1,
	   i4 psize2, PTR pval2,
	   i4 psize3, PTR pval3,
	   i4 psize4, PTR pval4,
	   i4 psize5, PTR pval5,
	   i4 psize6, PTR pval6 )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 6,
	      psize1, pval1,
	      psize2, pval2,
	      psize3, pval3,
	      psize4, pval4,
	      psize5, pval5,
	      psize6, pval6,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0 );
}


/*{
** Name:	GM_7error	-- error function with 7 parameters
**
** Description:
**	Format and send an error to the right place, with 7 given
**	substitution parameters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	psize1		size of parm1
**	pval1		value of parm1
**	psize2		size of parm2
**	pval2		value of parm2
**	psize3		size of parm3
**	pval3		value of parm3
**	psize4		size of parm4
**	pval4		value of parm4
**	psize5		size of parm5
**	pval5		value of parm5
**	psize6		size of parm6
**	pval6		value of parm6
**	psize7		size of parm7
**	pval7		value of parm7
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_7error ( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type,
	   i4 psize1, PTR pval1,
	   i4 psize2, PTR pval2,
	   i4 psize3, PTR pval3,
	   i4 psize4, PTR pval4,
	   i4 psize5, PTR pval5,
	   i4 psize6, PTR pval6,
	   i4 psize7, PTR pval7 )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 7,
	      psize1, pval1,
	      psize2, pval2,
	      psize3, pval3,
	      psize4, pval4,
	      psize5, pval5,
	      psize6, pval6,
	      psize7, pval7,
	      0L, (PTR)0,
	      0L, (PTR)0,
	      0L, (PTR)0 );
}


/*{
** Name:	GM_8error	-- error function with 8 parameters
**
** Description:
**	Format and send an error to the right place, with 8 given
**	substitution parameters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	psize1		size of parm1
**	pval1		value of parm1
**	psize2		size of parm2
**	pval2		value of parm2
**	psize3		size of parm3
**	pval3		value of parm3
**	psize4		size of parm4
**	pval4		value of parm4
**	psize5		size of parm5
**	pval5		value of parm5
**	psize6		size of parm6
**	pval6		value of parm6
**	psize7		size of parm7
**	pval7		value of parm7
**	psize8		size of parm8
**	pval8		value of parm8
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_8error ( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type,
	   i4 psize1, PTR pval1,
	   i4 psize2, PTR pval2,
	   i4 psize3, PTR pval3,
	   i4 psize4, PTR pval4,
	   i4 psize5, PTR pval5,
	   i4 psize6, PTR pval6,
	   i4 psize7, PTR pval7,
	   i4 psize8, PTR pval8 )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 8,
	      psize1, pval1,
	      psize2, pval2,
	      psize3, pval3,
	      psize4, pval4,
	      psize5, pval5,
	      psize6, pval6,
	      psize7, pval7,
	      psize8, pval8,
	      0L, (PTR)0,
	      0L, (PTR)0 );
}


/*{
** Name:	GM_9error	-- error function with 9 parameters
**
** Description:
**	Format and send an error to the right place, with 9 given
**	substitution parameters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	psize1		size of parm1
**	pval1		value of parm1
**	psize2		size of parm2
**	pval2		value of parm2
**	psize3		size of parm3
**	pval3		value of parm3
**	psize4		size of parm4
**	pval4		value of parm4
**	psize5		size of parm5
**	pval5		value of parm5
**	psize6		size of parm6
**	pval6		value of parm6
**	psize7		size of parm7
**	pval7		value of parm7
**	psize8		size of parm8
**	pval8		value of parm8
**	psize9		size of parm9
**	pval9		value of parm9
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_9error ( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type,
	   i4 psize1, PTR pval1,
	   i4 psize2, PTR pval2,
	   i4 psize3, PTR pval3,
	   i4 psize4, PTR pval4,
	   i4 psize5, PTR pval5,
	   i4 psize6, PTR pval6,
	   i4 psize7, PTR pval7,
	   i4 psize8, PTR pval8,
	   i4 psize9, PTR pval9 )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 9,
	      psize1, pval1,
	      psize2, pval2,
	      psize3, pval3,
	      psize4, pval4,
	      psize5, pval5,
	      psize6, pval6,
	      psize7, pval7,
	      psize8, pval8,
	      psize9, pval9,
	      0L, (PTR)0 );
}



/*{
** Name:	GM_10error	-- error function with 10 parameters
**
** Description:
**	Format and send an error to the right place, with 10 given
**	substitution parameters.
**
** Re-entrancy:
**	yes.
**
** Inputs:
**	gwx_rcb		RCB for the session, may be NULL.
**	error_no	Error number
**	error_type	Type of error (GM_ERR_INTERNAL, LOG, USER)
**	psize1		size of parm1
**	pval1		value of parm1
**	psize2		size of parm2
**	pval2		value of parm2
**	psize3		size of parm3
**	pval3		value of parm3
**	psize4		size of parm4
**	pval4		value of parm4
**	psize5		size of parm5
**	pval5		value of parm5
**	psize6		size of parm6
**	pval6		value of parm6
**	psize7		size of parm7
**	pval7		value of parm7
**	psize8		size of parm8
**	pval8		value of parm8
**	psize9		size of parm9
**	pval9		value of parm9
**	psize10		size of parm10
**	pval10		value of parm10
**
** Outputs:
**	none.
**
** Returns:
**	none.
**
** History:
**	2-Feb-1993 (daveb)
**	    Documented.
*/

VOID
GM_10error ( GWX_RCB *gwx_rcb, i4 error_no, i4  error_type,
	   i4 psize1, PTR pval1,
	   i4 psize2, PTR pval2,
	   i4 psize3, PTR pval3,
	   i4 psize4, PTR pval4,
	   i4 psize5, PTR pval5,
	   i4 psize6, PTR pval6,
	   i4 psize7, PTR pval7,
	   i4 psize8, PTR pval8,
	   i4 psize9, PTR pval9,
	   i4 psize10, PTR pval10 )
{
    GM_incr( &GM_globals.gwm_stat_sem, &GM_globals.gwm_errors );

    GM_uerror( gwx_rcb, error_no, error_type, 10,
	      psize1, pval1,
	      psize2, pval2,
	      psize3, pval3,
	      psize4, pval4,
	      psize5, pval5,
	      psize6, pval6,
	      psize7, pval7,
	      psize8, pval8,
	      psize9, pval9,
	      psize10, pval10 );
}
