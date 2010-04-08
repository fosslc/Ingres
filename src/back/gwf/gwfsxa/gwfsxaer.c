/*
**Copyright (c) 2004 Ingres Corporation
*/
# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <dudbms.h>
# include    <cm.h>
# include    <cs.h>
# include    <st.h>
# include    <tm.h>
# include    <ulf.h>
# include    <adf.h>
# include    <scf.h>
# include    <dmf.h>
# include    <dmucb.h>
# include    <dmrcb.h>
# include    <dmtcb.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <gwf.h>
# include    <gwfint.h>
# include    "gwfsxa.h"

FUNC_EXTERN	DB_STATUS	scf_call();
/*
** Name: GWFSXAER.C - Error handler for SXA gateway
**
** Description:
**	This file contains the error handler for  SXA gateway.
**
** History:
**	14-sep-92 (robf)
**	    Created
**	11-nov-92 (robf)
**	    Changed error handling from generic_error to sqlstate
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      21-May-1998 (hanch04)
**          Removed declaration of ule_format
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-Feb-2004 (kodse01)
**          Removed gwxit.h inclusion which is not required.
*/
/*
**	Name: gwsxa_error - Generic SXA gateway error printing routines
**
**	Inputs:
**		gwx_rcb		RCB for the session
**		error_no	Error number
**		error_type	Type of error
**		num_parm	Number of parmeters
**		parms		Parameters to print
**
*/
/* VARARGS4 */
VOID
gwsxa_error ( gwx_rcb, error_no, error_type, num_parm, parm1, parm2, parm3,
	parm4, parm5, parm6, parm7, parm8, parm9, parm10, parm11, parm12,
	parm13, parm14, parm15, parm16, parm17, parm18, parm19, parm20)
GWX_RCB	    *gwx_rcb;
i4 error_no;
i4      error_type;
i4	num_parm;
i4	parm1;
PTR	parm2;
i4	parm3;
PTR	parm4;
i4	parm5;
PTR	parm6;
i4	parm7;
PTR	parm8;
i4	parm9;
PTR	parm10;
i4	parm11;
PTR	parm12;
i4	parm13;
PTR	parm14;
i4	parm15;
PTR	parm16;
i4	parm17;
PTR	parm18;
i4	parm19;
PTR	parm20;
{
    i4             uletype;
    i4		ulecode;
    DB_STATUS           uleret;
    DB_SQLSTATE		sqlstate;
    i4		msglen;
    DB_STATUS		ret_val = E_DB_OK;
    char		errbuf[1024];
    SCF_CB		scf_cb;

    for (;;)	/* Not a loop, just gives a place to break to */
    {
	/*
	** Log internal errors and errors which the caller asked to be
	** logged.
	*/
	if ((error_type & SXA_ERR_INTERNAL) ||
	    (error_type & SXA_ERR_LOG))
	    uletype = ULE_LOG;
	else
	    uletype = 0L;

	/*
	** Get error message text.  
	*/
	uleret = ule_format(error_no, 0L, uletype, &sqlstate, errbuf,
	(i4) sizeof(errbuf), &msglen, &ulecode, num_parm,
	parm1, parm2, parm3, parm4, parm5, parm6,
	parm7, parm8, parm9, parm10,
	parm11, parm12, parm13, parm14, parm15, parm16,
	parm17, parm18, parm19, parm20);

	/*
	** If ule_format failed, we probably can't report any error.
	** Instead, just propagate the error up to the user.
	*/
	if (uleret != E_DB_OK)
	{
		STprintf(errbuf, "SXA Gateway: Error message corresponding to %d (X%x) \
		    not found in INGRES error file", error_no, error_no);
		msglen = STlength(errbuf);
		ule_format(0L, 0L, ULE_MESSAGE|ULE_LOG, NULL, errbuf, msglen, 0,
		    &ulecode, 0);
	}
	/*
	**	Only send to session if USER or INTERNAL
	*/
	if(error_type & (SXA_ERR_INTERNAL|SXA_ERR_USER))
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
			gwx_rcb->xrcb_error.err_code= scf_cb.scf_error.err_code;
		    }
		    break;
		}
	}
	break;
    }

    return;
}

