/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <dbms.h>
#include    <er.h>
#include    <cm.h>
#include    <si.h>
#include    <st.h>
#include    <duerr.h>
#include    <pc.h>
#include    <fe.h>
#include    <ug.h>
EXEC SQL INCLUDE 'dut.sh';
EXEC SQL INCLUDE SQLCA;

#define	    DUT_MX_ERR_PARAMS	(10)

/**
**
**  Name: DUTERR.C - Error message handling routine.
**
**  Description: File for error message functions.
**
**	dut_ee1_error	- error handling routine.
**	dut_ee2_exit	- error handling routine.
**	dut_ee3_error	- error handling routine.
**	dut_ee4_dummy	- error handling routine.
**	dut_ee5_none_starview - error handler for none-starview errors.
**
**
**  History:
**      13-dec-1988 (alexc)
**          Creation.
**	08-mar-1989 (alexc)
**	    Alpha test version.
**      29-apr-1989 (alexc)
**          Remove reference to status in sub-structure.
**	02-jul-1991 (rudyw)
**	    Create a line of forward function references to remedy compiler
**	    complaint of redefinition.
**	29-oct-93 (swm)
**	    Added comments to indicate that code which causes lint
**	    int/pointer truncation warnings is valid.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of pc.h, fe.h and ug.h plus return type for
**	    dut_ee4_dummy to eliminate gcc 4.3 warnings.
**/


/*
**  Forward and/or External typedef/struct references.
*/

DUT_STATUS dut_ee2_exit(), dut_ee3_error();

/*
** Name: dut_ee1_error	- Handles error locally or by SQLprint.
**
** Description:
**      Handle error messages for StarView.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      13-dec-1988 (alexc)
**          Creation.
*/
DUT_STATUS
dut_ee1_error(dut_errcb, dut_errno, dut_pcnt, 
		p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
DUT_ERRCB   *dut_errcb;
DUT_STATUS  dut_errno;
i4	    dut_pcnt;
i4	    *p1, *p2, *p3, *p4, *p5, *p6, *p7, *p8, *p9, *p10;
{
    DUT_STATUS	duerr_status;

    dut_errcb->dut_cb->dut_c8_status = DUT_ERROR;
    duerr_status = dut_ee3_error(dut_errcb, 
			    dut_errno, dut_pcnt, 
			    p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);
    dut_errcb->dut_e2_erstatus = duerr_status;
    if (duerr_status != E_DU_OK)
    {
	dut_ee2_exit(dut_errcb->dut_cb, dut_errcb);
	return(duerr_status);
    }

    if (dut_errcb->dut_e10_form_init == TRUE)
    {
	IIUGeppErrorPrettyPrint(stdout, dut_errcb->dut_e8_errmsg, FALSE);
    }
    else
    {
	SIprintf(dut_errcb->dut_e8_errmsg);
    }
    return(E_DU_OK);
}

/*
** Name: dut_ee2_exit	-
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**
** Side Effects:
**
** History:
**      13-dec-1988 (alexc)
**          Creation.
*/
DUT_STATUS
dut_ee2_exit(dut_cb, dut_errcb)
DUT_CB      *dut_cb;
DUT_ERRCB   *dut_errcb;
{
    switch(dut_errcb->dut_e3_status)
    {
	case E_DU_IERROR:
	case E_DU_UERROR:
	    if (dut_errcb->dut_e5_utilerr == E_DU2002_BAD_SYS_ERLOOKUP)
	    {
		/*    IIUGerr(dut_errno, 0, dut_pcnt, 
			p1, p2, p3, p4, p5, p6, p7, p8, p9, p10);*/
	        IIUGeppErrorPrettyPrint(stdout, "Cannot find error message", 
					FALSE);
	    }
	    IIUGeppErrorPrettyPrint(stdout, 
		"Encountered Fatal error when in StarView, Exiting...",
		FALSE);

	    EXEC SQL disconnect;
	    dut_uu4_cleanup(dut_cb, dut_errcb);
	    SIprintf(
		"Encountered Fatal error when in StarView, Exiting...\n\n");
	    SIprintf(dut_errcb->dut_e8_errmsg);
	    PCexit(FAIL);
	    break;
	default:
	    break;
    }
    return(E_DU_OK);
}

/*
** Name: dut_ee3_error	-
**
** Description:
**
** Inputs:
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**
** Side Effects:
**
** History:
**      13-dec-1988 (alexc)
**          Creation.
**	29-oct-93 (swm)
**	    Added comments to indicate that code which causes lint
**	    int/pointer truncation warnings is valid.
*/
DUT_STATUS
dut_ee3_error(dut_errcb, dut_errno, dut_pcnt, 
		p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)
DUT_ERRCB	    *dut_errcb;
i4		    dut_errno;
i4		    dut_pcnt;
i4		    *p1;
i4		    *p2;
i4		    *p3;
i4		    *p4;
i4		    *p5;
i4		    *p6;
i4		    *p7;
i4		    *p8;
i4		    *p9;
i4		    *p10;
{
    char		*msg_ptr;
    DUT_STATUS          ret_status;
    CL_SYS_ERR		sys_err;
    i4			rslt_msglen;
    i4			save_msglen;
    i4			buf_len	    = ER_MAX_LEN;
    ER_ARGUMENT		loc_param;
    i4			tmp_status;
    i4			lang_code;
    ER_ARGUMENT		param[DUT_MX_ERR_PARAMS];
    static CL_SYS_ERR		cl_sys_errcb;

    dut_errcb->dut_e7_clsyserr = (PTR) &cl_sys_errcb;
    /* First get the language code. */
    ERlangcode((char *) NULL, &lang_code);

    /* Check if the call was done correctly */

    if ((dut_pcnt % 2) == 1 || dut_pcnt > DUT_MX_ERR_PARAMS)
    {
	dut_errno    = E_DU2001_BAD_ERROR_PARAMS;
	dut_pcnt	    = 0;
    }


    /* Classify the severity of the error */
    if (dut_errno < DU_OK_BOUNDARY)
	ret_status    = E_DU_OK;
    else if (dut_errno < DU_INFO_BOUNDARY)
	ret_status    = E_DU_INFO;
    else if (dut_errno < DU_WARN_BOUNDARY)
	ret_status    = E_DU_WARN;
    else if (dut_errno < DU_IERROR_BOUNDARY)
	ret_status    = E_DU_IERROR;
    else
	ret_status    = E_DU_UERROR;

    dut_errcb->dut_e3_status	= ret_status;

    /* Format the database utility error message */
    dut_errcb->dut_e5_utilerr    = dut_errno;


    /* put arguments into argument array */
	/* lint truncation warning if size of ptr > int, but code valid */
    param[0].er_size  = (i4) p1;
    param[0].er_value = (PTR)p2;
	/* lint truncation warning if size of ptr > int, but code valid */
    param[1].er_size  = (i4) p3;
    param[1].er_value = (PTR)p4;
	/* lint truncation warning if size of ptr > int, but code valid */
    param[2].er_size  = (i4) p5;
    param[2].er_value = (PTR)p6;
	/* lint truncation warning if size of ptr > int, but code valid */
    param[3].er_size  = (i4) p7;
    param[3].er_value = (PTR)p8;
	/* lint truncation warning if size of ptr > int, but code valid */
    param[4].er_size  = (i4) p9;
    param[4].er_value = (PTR)p10;

    tmp_status  = ERlookup(dut_errno, (CL_SYS_ERR *) 0, 0, 0,
			   dut_errcb->dut_e8_errmsg, buf_len, lang_code,
			   &rslt_msglen, &sys_err, dut_pcnt/2, param);

    if (tmp_status != OK)
    {
	loc_param.er_size   = sizeof(dut_errno);
	loc_param.er_value  = (PTR)&dut_errno;

	ERlookup((i4) E_DU2000_BAD_ERLOOKUP, (CL_SYS_ERR *) 0, 0,
		 0, dut_errcb->dut_e8_errmsg, buf_len, lang_code,
		 &rslt_msglen, &sys_err, 1, (ER_ARGUMENT *) &loc_param);

	dut_errcb->dut_e8_errmsg[rslt_msglen]    = EOS;
	ret_status			= E_DU_IERROR;
	dut_errcb->dut_e3_status	= ret_status;
	dut_errcb->dut_e5_utilerr	= E_DU2000_BAD_ERLOOKUP;
    }
    else
    {
	/* Format an operating system error */
	dut_errcb->dut_e8_errmsg[rslt_msglen++]    = '\n';
	save_msglen  = rslt_msglen;
	buf_len	    -= rslt_msglen;
	tmp_status = ERlookup((i4) 0, dut_errcb->dut_e7_clsyserr, 0, 0,
				&dut_errcb->dut_e8_errmsg[rslt_msglen],
				buf_len, lang_code, &rslt_msglen,
				&sys_err, 0, (ER_ARGUMENT *) NULL);
	if (!rslt_msglen)
	    dut_errcb->dut_e8_errmsg[save_msglen - 1] = ' ';
		/* delete newline */

	if (tmp_status != OK)
	{
	    /* Report that the operating system error couldn't be found */
	    ERlookup((i4) E_DU2002_BAD_SYS_ERLOOKUP, (CL_SYS_ERR *) 0, 0,
		     0, &dut_errcb->dut_e8_errmsg[save_msglen], buf_len,
		     lang_code, &rslt_msglen, &sys_err, 0,
		     (ER_ARGUMENT *) NULL);
	    ret_status	= E_DU_IERROR;
	    dut_errcb->dut_e3_status	= ret_status;
	    dut_errcb->dut_e5_utilerr    = E_DU2002_BAD_SYS_ERLOOKUP;
	}
	dut_errcb->dut_e8_errmsg[save_msglen + rslt_msglen]    = EOS;
    }

    /* If the message being formatted is really informational or a warning,
    ** print it and reset the error-handling control block.
    */
    if (ret_status == E_DU_WARN || ret_status == E_DU_INFO)
    {
	if ((dut_errcb->dut_e9_flags & DU_SAVEMSG) == 0)
	{
	    /* Print the warning or informative msg and reset the error
	    ** control block.
	    */
	    msg_ptr = STindex(dut_errcb->dut_e8_errmsg, "\t", 0);
	    if (msg_ptr != NULL)
	    {
		msg_ptr++;
		/*
		**  while (CMspace(msg_ptr))
		**	CMnext(msg_ptr);
		*/
		SIprintf("%s\n", msg_ptr);
		SIflush(stdout);
	    }
	    dut_errcb->dut_e3_status		= E_DU_OK;
	    dut_errcb->dut_e4_ingerr		= DU_NONEXIST_ERROR;
	    dut_errcb->dut_e5_utilerr		= DU_NONEXIST_ERROR;
	    dut_errcb->dut_e6_clerr		= 0;
	    dut_errcb->dut_e8_errmsg[0]		= EOS;
	    dut_errcb->dut_e9_flags		= 0;
	}
	/* In either case, whether we reset the error control block or not
	** return an OK status.
	*/
	ret_status  = E_DU_OK;
    }
    else
    {
	dut_errcb->dut_e3_status		= E_DU_OK;
	/* Replace the TAB after the DUF facility code id with a NEWLINE. */
	msg_ptr = STindex(dut_errcb->dut_e8_errmsg, "\t", 0);
	if (msg_ptr)
	    *msg_ptr = '\n';
	ret_status = E_DU_OK;
    }

    return(ret_status);
}

void
dut_ee4_dummy()
{
	/* Do nothing dummy routine */
}


/*
** Name: dut_ee5_none_starview	- determine if the error is a fatal error for
**			  starview.
**
** Description:
**      compare the error number to a set of fatal errors for StarView.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    E_DB_OK
**	Exceptions:
**
** Side Effects:
**
** History:
**      07-dec-1989 (alexc)
**          Creation.
*/
DUT_STATUS
dut_ee5_none_starview()
{
    EXEC SQL BEGIN DECLARE SECTION;
    char	errbuf[DB_ERR_SIZE];
    EXEC SQL END DECLARE SECTION;
    i4		errstatus;

    errstatus = sqlca.sqlcode;
    EXEC SQL copy sqlerror into :errbuf;

    if (Dut_gcb->dut_on_error_noop == TRUE)
	return(E_DU_OK);
    if (Dut_gerrcb->dut_e10_form_init == TRUE)
    {
	IIUGeppErrorPrettyPrint(stdout, errbuf, FALSE);
    }
    else
    {
	SIprintf(errbuf);
    }
    if (Dut_gcb->dut_on_error_exit == TRUE)
    {
	if (Dut_gcb->dut_c7_status == DUT_S_CONNECT_IIDBDB)
	{
	    dut_ee1_error(Dut_gerrcb, 0/* Local connection dropped */, 0);
	    EXEC FRS clear screen;
	    EXEC FRS endforms;
	    PCexit(errstatus);
	}
	if (Dut_gcb->dut_c7_status == DUT_S_NULL)
	{
	    return(errstatus);
	}
	if (Dut_gcb->dut_c7_status != DUT_S_CONNECT_DDB)
	{
	    if (dut_uu2_dbop(DUT_DDBOPEN, Dut_gcb, Dut_gerrcb) != E_DU_OK)
	    {
		dut_ee1_error(Dut_gerrcb, 0/* Star connection problem */, 0);
	    }
	    else
	    {
		/* Session is still okay with STAR, but not local */
		dut_ee1_error(Dut_gerrcb, 0/* Local connection dropped */, 0);
	    }
	    EXEC FRS clear screen;
	    EXEC FRS endforms;
	    PCexit(errstatus);
	}
	else
	{
	    /* Fatal error encountered */
	    dut_ee1_error(Dut_gerrcb, 0/* Fatal error */, 0);
	    EXEC FRS clear screen;
	    EXEC FRS endforms;
	    PCexit(errstatus);
	}
    }
    else
    {
	return(E_DU_OK);
    }
}
