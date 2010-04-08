/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <psftrmwh.h>

/**
**
**  Name: PSTRSERR.C - Functions for handling errors from pst_resolve
**
**  Description:
**      The pst_resolve function returns error statuses that need some
**	special processing.  This file contains the functions for handling
**	those errors.
**
**          pst_rserror - Handle errors from pst_resolve
**
**
**  History:    $Log-for RCS$
**      30-may-86 (jeff)    
**          written
**	14-nov-88 (stec)
**	    Initialize ADF_CB allocated on the stack.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	10-Jan-2001 (jenjo02)
**	    ADF_CB* is available in session's cb; use it
**	    instead of SCU_INFORMATION.
[@history_template@]...
**/


/*{
** Name: pst_rserror	- Handle errors from pst_resolve
**
** Description:
**      This function handles the reporting of error conditions from the
**	pst_resolve function.  This is necessary because pst_resolve doesn't
**	report some errors to the user that ought to be reported.  The reason
**	is that pst_resolve can be used by the optimizer (and others?), and
**	so it shouldn't make assumptions about which errors are user errors.
**
** Inputs:
**	lineno				Line number the error occurred on
**      opnode                          The operator node we got the error on
**	err_blk				The error block filled in by pst_resolve
**
** Outputs:
**      err_blk
**	    .err_code			The error code is changed in some cases
**	Returns:
**	    E_DB_ERROR			Always indicates an error
**	    E_DB_SEVERE			If not a user error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can send error to user, or log it
**
** History:
**	30-may-86 (jeff)
**          written
**	14-nov-88 (stec)
**	    Initialize ADF_CB allocated on the stack.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-Jan-2001 (jenjo02)
**	    ADF_CB* is available in session's cb; use it
**	    instead of SCU_INFORMATION.
**	3-july-01 (inkdo01)
**	    Support PST_MOP node type & wrong no. of parms error.
**	19-Feb-2004 (schka24)
**	    Get MOP operands from the right place, prevent AD2004 error.
**	12-july-06 (dougi)
**	    Detect "sum" that's really a transformed "avg" and fix text to
**	    avoid confusion.
*/
DB_STATUS
pst_rserror(
	i4		lineno,
	PST_QNODE       *opnode,
	DB_ERROR	*err_blk)
{
    i4             err_code;
    DB_STATUS		status;
    DB_DT_ID		lefttype, righttype;
    ADI_DT_NAME		leftparm;
    ADI_DT_NAME		rightparm;
    ADI_OP_NAME		opname;
    ADF_CB		*adf_scb;
    PTR			ad_errmsgp;
    i4			children;
    PSS_SESBLK		*sess_cb;

    /* First, check for those errors that should be internal errors */
    if (err_blk->err_code == E_PS0C02_NULL_PTR ||
	err_blk->err_code == E_PS0C03_BAD_NODE_TYPE ||
	err_blk->err_code == E_PS0C04_BAD_TREE ||
	err_blk->err_code == E_PS0C05_BAD_ADF_STATUS)
    {
	(VOID) psf_error(err_blk->err_code, 0L, PSF_INTERR,
	    &err_code, err_blk, 0);
	return (E_DB_SEVERE);
    }

    /* Figure out how many descendants this node has */
    if (opnode->pst_sym.pst_type == PST_COP ||
		err_blk->err_code == 2903)	/* no need for descendants */
	children = 0;
    else if (opnode->pst_sym.pst_type == PST_BOP ||
	    opnode->pst_sym.pst_type == PST_MOP)
	children = 2;
    else
	children = 1;

    /* Get the ADF session control block */
    sess_cb = psf_sesscb();
    adf_scb = (ADF_CB*)sess_cb->pss_adfcb;

    /* Preserve and restore ad_errmsgp */
    ad_errmsgp = adf_scb->adf_errcb.ad_errmsgp;

    /* Get the function or operator name */
    adf_scb->adf_errcb.ad_errmsgp = NULL;
    status = adi_opname(adf_scb, opnode->pst_sym.pst_value.pst_s_op.pst_opno,
	&opname);

    if (STcompare(opname.adi_opname, "sum") == 0 &&
	opnode->pst_sym.pst_value.pst_s_op.pst_flags & PST_AVG_AOP)
	STcopy("avg", opname.adi_opname);	/* it's really "avg" */

    if (DB_FAILURE_MACRO(status))
    {
	/* Restore ad_errmsgp */
	adf_scb->adf_errcb.ad_errmsgp = ad_errmsgp;

	if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	{
	    (VOID) psf_error(E_PS0C04_BAD_TREE, adf_scb->adf_errcb.ad_usererr,
		PSF_INTERR, &err_code, err_blk, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0C04_BAD_TREE, adf_scb->adf_errcb.ad_errcode,
		PSF_INTERR, &err_code, err_blk, 0);
	}
	return (E_DB_SEVERE);
    }

    /* Get the names of the left & right operands, if any */
    STmove("<none>", ' ', sizeof (ADI_DT_NAME), (char *) &leftparm);
    STmove("<none>", ' ', sizeof (ADI_DT_NAME), (char *) &rightparm);

    /* For unary/binary ops, the first/second args are the opnode's left
    ** and right children respectively.  For MOP's, though, the operands
    ** are the right children of the op node and its left children.
    ** FIXME should invent a multiple-arg errmsg, do more than 2 types.
    */
    if (children > 0)
    {
	if (opnode->pst_sym.pst_type != PST_MOP)
	{
	    /* Usual case */
	    lefttype = opnode->pst_left->pst_sym.pst_dataval.db_datatype;
	    if (children >= 2)
		righttype = opnode->pst_right->pst_sym.pst_dataval.db_datatype;
	}
	else
	{
	    lefttype = opnode->pst_right->pst_sym.pst_dataval.db_datatype;
	    righttype = opnode->pst_left->pst_right->pst_sym.pst_dataval.db_datatype;
	}
    }
    if (children >= 1)
    {
	status = adi_tyname(adf_scb, lefttype, &leftparm);
	if (status != E_DB_OK)
	{
	    /* Restore ad_errmsgp */
	    adf_scb->adf_errcb.ad_errmsgp = ad_errmsgp;

	    if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE, adf_scb->adf_errcb.ad_usererr,
		    PSF_INTERR, &err_code, err_blk, 0);
	    }
	    else
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE, adf_scb->adf_errcb.ad_errcode,
		    PSF_INTERR, &err_code, err_blk, 0);
	    }
	    return (E_DB_SEVERE);
	}
    }

    if (children == 2)
    {
	status = adi_tyname(adf_scb, righttype, &rightparm);
	if (status != E_DB_OK)
	{
	    /* Restore ad_errmsgp */
	    adf_scb->adf_errcb.ad_errmsgp = ad_errmsgp;

	    if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE, adf_scb->adf_errcb.ad_usererr,
		    PSF_INTERR, &err_code, err_blk, 0);
	    }
	    else
	    {
		(VOID) psf_error(E_PS0C04_BAD_TREE, adf_scb->adf_errcb.ad_errcode,
		    PSF_INTERR, &err_code, err_blk, 0);
	    }
	    return (E_DB_SEVERE);
	}
    }

    /* Restore ad_errmsgp */
    adf_scb->adf_errcb.ad_errmsgp = ad_errmsgp;

    /* Now report the error */
    (VOID) psf_error(err_blk->err_code, 0L, PSF_USERERR, &err_code, err_blk, 4,
	sizeof(lineno), &lineno, 
	psf_trmwhite(sizeof (ADI_DT_NAME), (char *) &opname), &opname, 
	psf_trmwhite(sizeof (ADI_DT_NAME), (char *) &leftparm), &leftparm, 
	psf_trmwhite(sizeof (ADI_DT_NAME), (char *) &rightparm), &rightparm);

    return (E_DB_ERROR);
}
