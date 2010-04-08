/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adfops.h>
#include    <ade.h>
#include    <ulf.h>
#include    <adfint.h>
#include    <adefebe.h>
#include    <adeint.h>
#include    <me.h>
/*  [@#include@]... */

/**
**
**  Name: ADFFUNC.C - Perform any non-aggregate function instance.
**
**  Description:
**      This file contains all of the routines necessary to
**      perform the external ADF call "adf_func()" which
**      actually performs any supplied non-aggregate function
**      instance on the supplied data values (if any).
**
**      This file defines:
**
**          adf_func()	    - Perform any non-aggregate function instance.
**	    adf_exec_quick()- Quickly execute a function instance.
**
**	As well as the following static routine:
**
**	    ad0_usecx() - Use a CX to perform the f.i.
**
**
**  History:
**      26-feb-86 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	31-jul-86 (thurston)
**	    Added code to handle f.i.'s that do not have a corresponding
**	    function in the f.i. lookup table.  If this is the case, we have
**	    to compile a CX and execute it using the ADE.
**	04-aug-86 (thurston)
**	    Added code to adf_func() for zero argument f.i.'s.
**	22-dec-86 (thurston)
**	    Fixed bug in adf_func() that caused problem when the length spec
**	    for the f.i. to get executed was ADI_RESLEN.  (e.g.  All of the
**	    coercions from a datatype into itself.)
**	29-dec-86 (thurston)
**          In the ad0_usecx() routine, I added a conditional compilation of the
**          ADE_AND instruction if the function instance was a comparison.
**          Otherwise, because of the way the expression evaluater works, the
**          expression will always return TRUE.  This was a bug with this
**          routine. 
**	18-feb-87 (thurston)
**	    In adf_func(), I removed result length check if function instance is
**	    a COERCION.  There are cases where the result length of a coercion
**	    needs to be bigger or smaller than what the lenspec would suggest.
**	26-feb-87 (thurston)
**	    Made changes to use server CB instead of GLOBALDEFs.
**	12-mar-87 (thurston)
**	    Moved definitions of forward static functions out of the routines
**	    that use them to the top of the file.
**	09-apr-87 (thurston)
**	    Fixed bug in adf_func() with functions of less than 2 input args.
**          adi_calclen() was being called to check the result length, and the
**          DB_DATA_VALUEs being passed to it were whatever happened to be in
**          the ADF_FN_BLK. Now, NULLs are passed in where appropriate.
**	06-may-87 (thurston)
**	    Made bug fix in adf_func() to handle nullable result where no
**	    inputs are nullable.
**	05-jun-87 (thurston)
**	    In adf_func(), I changed the call to adi_calclen() to call
**	    adi_0calclen(), a temporary name for the new adi_calclen().
**	    This is being used as a migration path from the old adi_calclen()
**	    which doesn't quite cut it in all cases.
**	01-jul-87 (thurston)
**	    Omit result length check for copy coercions in adf_func().
**	23-jun-88 (thurston)
**	    Added code to adf_func() and ad0_usecx() to handle the escape char
**	    for the ESCAPE clause of LIKE and NOT LIKE.
**	28-feb-89 (fred)
**	    Altered references to Adi_* global variables to be thru Adf_globs
**	    for UDADT support.
**	20-mar-89 (jrb)
**	    Made changes for adc_lenchk interface change.
**	23-may-89 (jrb)
**	    ad0_usecx was calling ade_instr_gen with the wrong number of parms.
**	13-dec-89 (fred)
**	    Altered to call ade_execute_cx() to call the expression evaluator
**	    with the new interface, as necessitated by large object support.
**      02-jun-92 (dkh, integrated by stevet)
**          Fixed ad0_usecx() to set the precision for a DB_DATA_VALUE
**          correctly.  Otherwise, expressions don't work when decimal
**          datatypes are used.
**      23-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	02-Mar-1999 (shero03)
**	    Support 4 operands in adi_0calclen
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-sep-2006 (gupsh01)
**	    Added static routine ad0_isdtfamily_same().
**	02-Apr-2007 (hweho01)
**	    Modified the prototype and function parameter list for  
**	    ad0_isdtfamily_same(), avoid compiling error on AIX.
**	22-Sep-2008 (kiria01) SIR121012
**	    Completed the support for operand count from 2 to 4 as
**	    supported in the adf_func parameter block.
**	    Allowed for the enhanced ADE_ESC_CHAR to be specified
**	    by the caller.
**      27-Oct-2008 (hweho01)
**          Fixed SIGBUS errror during iidbdb creation on Solaris/Sparc, 
**          use union for excb buffer in ad0_usecx().  
**	25-Nov-2008 (kiria01) b121290
**	    Support execution of operators with peripheral datatypes.
**	10-Dec-2008 (kiria01) b121290
**	    Include me.h explicitly to get ME functions linked in on VMS 
**/


/*
**  Definition of static variables and forward static functions.
*/

static  DB_STATUS   ad0_usecx(ADF_CB	     *adf_scb,
			      UCS2	     *eptr,
			      u_i2	     pat_flags,
			      ADI_FI_DESC    *fdesc,
			      DB_DATA_VALUE  *dv[ADI_MAX_OPERANDS]);

static  bool ad0_isdtfamily_same(
			      DB_DT_ID	dt1,
			      DB_DT_ID	dt2);
				
/*
[@static_variable_or_function_definition@]...
*/


/*{
** Name: adf_func() - Perform any non-aggregate function instance.
**
** Description:
**      This function is the external ADF call "adf_func()" which
**      actually performs any supplied non-aggregate function
**      instance on the supplied data values (if any).
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      adf_fnblk
**	    .adf_fi_id			Function instance id for the operation
**					being requested.  (Must be that of a
**					non-aggregate function instance.)
**	    .adf_dv[0]			This is the DB_DATA_VALUE where the
**					result of the operation will be placed.
**		.db_datatype		Datatype id of result.
**		.db_length		Length of result.
**		.db_data		Pointer to location to place the actual
**					data for the result.
**	    .adf_dv[1..n]		This is the DB_DATA_VALUE of the 1st to nth
**					operand, if needed.
**		.db_datatype		Datatype id of operand.
**		.db_length		Length of operand.
**		.db_data		Pointer to the actual data
**					operand.
**	    .adf_escape			*** ONLY USED with the LIKE family of
**					operators ***
**					adf_escape has valid info in it *ONLY*
**					if bit AD_PAT_HAS_ESCAPE set in
**					adf_pat_flags.
**	    .adf_pat_flags		*** ONLY USED with the LIKE family of
**					operators ***
**					These modify the LIKE predicate to give
**					control to the type of pattern matching
**					to perform.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**      adf_fnblk
**	    .adf_dv[0]			The result data value.
**		.db_data		The actual data that is the result of
**					the requested operation is placed here.
**
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          E_AD2005_BAD_DTLEN		Length is illegal for the
**					given datatype.  One of the
**					DB_DATA_VALUEs in the call block
**					has a length that is illegal for
**					its datatype.
**          E_AD2010_BAD_FIID		Function instance id unknown to ADF.
**          E_AD4001_FIID_IS_AG         The function instance id is that
**					of an aggregate function.
**          E_AD5503_BAD_DTID_FOR_FIID	Datatype of one of the input or output
**					DB_DATA_VALUEs is not correct for the
**					supplied function instance.
**	    E_AD5504_BAD_RESULT_LEN	Length of the result is incorrect based
**					the function instance and the input
**					DB_DATA_VALUEs.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-jan-86 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	31-jul-86 (thurston)
**	    Added code to handle f.i.'s that do not have a corresponding
**	    function in the f.i. lookup table.  If this is the case, we have
**	    to compile a CX and execute it using the ADE.
**	04-aug-86 (thurston)
**	    Added code for zero argument f.i.'s.
**	22-dec-86 (thurston)
**	    Fixed bug that caused problem when the length spec for the f.i. to
**	    get executed was ADI_RESLEN.  (e.g.  All of the coercions from a
**	    datatype into itself.)
**	18-feb-87 (thurston)
**	    Removed result length check if function instance is a COERCION.
**	    There are cases where the result length of a coercion needs to be
**	    bigger or smaller than what the lenspec would suggest.
**	09-apr-87 (thurston)
**	    Fixed bug with functions of less than 2 input args.  adi_calclen()
**	    was being called to check the result length, and the DB_DATA_VALUEs
**	    being passed to it were whatever happened to be in the ADF_FN_BLK.
**	    Now, NULLs are passed in where appropriate.
**	06-may-87 (thurston)
**	    Made bug fix to handle nullable result where no inputs are nullable.
**	05-jun-87 (thurston)
**	    Changed call to adi_calclen() to call adi_0calclen(), a temporary
**	    name for the new adi_calclen().  This is being used as a migration
**	    path from the old adi_calclen() which doesn't quite cut it in all
**	    cases.
**	01-jul-87 (thurston)
**	    Omit result length check for copy coercions.
**	23-jun-88 (thurston)
**	    Handle the escape char for the ESCAPE clause on LIKE or NOT LIKE.
**	09-mar-92 (jrb, merged by stevet)
**	   Handle DB_ALL_TYPE for outer-join project.
**	06-sep-06 (gupsh01)
**	   Ignore datatype check for datatype family function instances. 
**	11-sep-06 (gupsh01)
**	   Fixed the datatype family check. fixed the case when we use a function
**	   instance whose operands belong to same datat type family as 
**	   input datatypes.
**	15-Oct-2008 (kiria01) b121061
**	   Handle the data in array form where the result slot is zero and
**	   p1 is in 1, p2 in 2 etc.
**	   Now any changes to ADI_MAX_OPERANDS will be followed with no extra work.
**	   Correct the setup of the escape character and, if present, ensure
**	   CX route taken to generate appropriate ADE_ESC_CHAR.
**	08-May-2009 (kiria01) SIR 121012
**	   If pre-instruction is ADE_0CXI_IGNORE, make sure that we init
**	   the NVL bit for nullable results.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	21-May-2009 (kiria01) b122051
**	   Reduce uninitialised db_collID
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
*/

DB_STATUS
adf_func(
ADF_CB             *adf_scb,
ADF_FN_BLK         *adf_fnblk)
{
    ADI_FI_ID           fi = adf_fnblk->adf_fi_id;
    ADI_FI_DESC         *fdesc = adf_fnblk->adf_fi_desc;
    i4                  nargs;
    i4                  cmp;
    DB_STATUS           db_stat;
    DB_STATUS		(*func)();
    DB_DATA_VALUE	*dv[ADI_MAX_OPERANDS+1];
    DB_DATA_VALUE	tmp_dv[ADI_MAX_OPERANDS+1];
    i4			bdt[ADI_MAX_OPERANDS+1];
    i4                  rlen;
    bool		nullable_seen = FALSE;
    i4			i;
    ALIGN_RESTRICT	ws_buff[4096/sizeof(ALIGN_RESTRICT)];
    PTR			ws_alloc = NULL;

    /* First check for legal function instance ID */
    /* ------------------------------------------ */
    if (!fdesc && (
		fi < (ADI_FI_ID) 0 ||
		fi > Adf_globs->Adi_fi_biggest ||
	  !(fdesc = (ADI_FI_DESC*)
		    Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(fi)].adi_fi)))
	return (adu_error(adf_scb, E_AD2010_BAD_FIID, 0));

    /* Then make sure f.i. is not an aggregate */
    /* --------------------------------------- */
    if (fdesc->adi_fitype == ADI_AGG_FUNC)
	return (adu_error(adf_scb, E_AD4001_FIID_IS_AG, 0));

    nargs = fdesc->adi_numargs;
    if (nargs >= ADI_MAX_OPERANDS)
	return (adu_error(adf_scb, E_AD2010_BAD_FIID, 0));

    /* Now init & check datatypes of input and output data values */
    /* ---------------------------------------------------------- */
    for (i = 0; i <= nargs; i++)
    {
	dv[i] = &adf_fnblk->adf_dv[i];
	if ((bdt[i] = dv[i]->db_datatype) < 0)
	{
	    bdt[i] = -bdt[i];
	    nullable_seen = TRUE;
	}
	if (!i
	    /*
	    ** adi_dtresult precedes adi_dt so adi_dtresult==adi_dt[-1]
	    ** so the special casing of 0 for result could be avoidable
	    */
	    ?  (bdt[i] != fdesc->adi_dtresult &&
		fdesc->adi_dtresult != DB_ALL_TYPE &&
		!(ad0_isdtfamily_same(bdt[i], fdesc->adi_dtresult)))
	    :  (bdt[i] != fdesc->adi_dt[i-1] &&
		fdesc->adi_dt[i-1] != DB_ALL_TYPE &&
		!(ad0_isdtfamily_same(bdt[i], fdesc->adi_dt[i-1])))
	    )
	    return (adu_error(adf_scb, E_AD5503_BAD_DTID_FOR_FIID, 0));

	if (DB_COLL_UNSET(*dv[i]))
	    dv[i]->db_collID = DB_UNSET_COLL;
	/* Now check lengths of data values */
	if (db_stat = adc_lenchk(adf_scb, FALSE, dv[i], NULL))
	    return db_stat;
    }

    /* Set data value ptrs to NULL if inputs don't exist */
    /* ------------------------------------------------- */
    for (i = nargs+1; i < ADI_MAX_OPERANDS; i++)
	dv[i] = NULL;

    /* Is result length what is expected?  (Only do check if not COERCION.) */
    /* -------------------------------------------------------------------- */
    if (    fdesc->adi_fitype != ADI_COERCION
	&&  fdesc->adi_fitype != ADI_COPY_COERCION
       )
    {
	rlen = dv[0]->db_length;	/* Save supplied result length */

	/* {@fix_me@} ... the following call to adi_0calclen() will have to be
	**                replaced with adi_calclen() when that is updated.
	*/
	if (db_stat = adi_0calclen(adf_scb, &fdesc->adi_lenspec, nargs,
			&dv[1], dv[0])) 
	    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));

	if (rlen < dv[0]->db_length)
	{
	    dv[0]->db_length = rlen;  /* Restore lousy supplied result length */
	    return (adu_error(adf_scb, E_AD5504_BAD_RESULT_LEN, 0));
	}
	dv[0]->db_length = rlen;  /* Restore lousy supplied result length */
    }


/* ======================================= */
/* Finally, execute the function instance: */
/* ======================================= */

    /* If nullable, handle specially */
    /* ------------------------------------------------------------------ */
    if (nullable_seen)
    {
	switch (fdesc->adi_npre_instr)
	{
	case ADE_0CXI_IGNORE:
	    /* Clear null bit in result if nullable. */
	    if (dv[0]->db_datatype < 0)
		ADF_CLRNULL_MACRO(dv[0]);
	    break;

	case ADE_1CXI_SET_SKIP:
	case ADE_3CXI_CMP_SET_SKIP:

	    for (i = 1; i <= nargs; i++)
	    {
		if (ADI_ISNULL_MACRO(dv[i]))
		{
		/*
		** Null value found:  Set null value bit in result and return.
		*/
		    if (dv[0]->db_datatype > 0)   /* Result type MUST be nullable */
			return (adu_error(adf_scb, E_AD1012_NULL_TO_NONNULL, 0));
		    ADF_SETNULL_MACRO(dv[0]);
		    return (E_DB_OK);
		}
	    }
	    /*
	    ** No null values found:  Clear null bit in result, set up temp
	    ** DB_DATA_VALUEs and execute instr as though non-nullable.
	    */
	    if (dv[0]->db_datatype < 0)   /* Result type MUST be nullable */
		ADF_CLRNULL_MACRO(dv[0]);
	    for (i = 0; i <= nargs; i++)
	    {
		if (dv[i]->db_datatype < 0)
		{
		    /* Use temp locn for non-nullable form */
		    tmp_dv[i] = *dv[i];
		    tmp_dv[i].db_datatype = bdt[i];
		    tmp_dv[i].db_length--;
		    dv[i] = &tmp_dv[i];
		}
	    }
	    break;

	case ADE_2CXI_SKIP:	/* Only valid for aggregates */
	default:
	    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
	}	/* END OF SWITCH STATEMENT */
    }

    /* We need an extra parameter for workspace of peripheral */
    if (fdesc->adi_fiflags & ADI_F4_WORKSPACE)
    {
	/* Build WS into n+1 th param */
	nargs++;
	if (fdesc->adi_agwsdv_len <= sizeof(ws_buff))
	    tmp_dv[nargs].db_data = (PTR) ws_buff;
	else
	{
	    ws_alloc = MEreqmem(0, fdesc->adi_agwsdv_len, FALSE, NULL);
	    if (!(tmp_dv[nargs].db_data = ws_alloc))
		return adu_error(adf_scb, E_AD7012_ADP_WORKSPACE_TOOLONG, 0);
	}
	tmp_dv[nargs].db_datatype = 0;	/* Workspace is typeless */
	tmp_dv[nargs].db_length = fdesc->adi_agwsdv_len;
	tmp_dv[nargs].db_prec = 0;
	tmp_dv[nargs].db_collID = DB_NOCOLLATION;
	dv[nargs] = &tmp_dv[nargs];
    }

    /* If pat_flags is anything other than 0 or AD_PAT_DOESNT_APPLY
    ** then we need to setup things to cause an ADE_ESC_CHAR
    ** operator to be emitted and this in turn requires that we
    ** handle this using the CX dispatch. We also need to use
    ** CX form if there is no function address i.e. intrinsic
    ** to ade_execute_cx() */
    /* --------------------------------------------------------------------- */
    if (adf_fnblk->adf_pat_flags != 0 && 
	adf_fnblk->adf_pat_flags != AD_PAT_DOESNT_APPLY ||
	!(func = Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(fi)].adi_func))
    {
	db_stat = ad0_usecx(adf_scb, &adf_fnblk->adf_escape,
		adf_fnblk->adf_pat_flags, fdesc, dv);
    }

    /* Else, is the f.i. a comparison? */
    /* ------------------------------- */
    else if (fdesc->adi_fitype == ADI_COMPARISON)
    {
	/*
	** NOTE!!!  LIKE and NOT LIKE are currently assumed to be handled by
	** using a CX, since I supply no mechanism here for sending the escape
	** char to the generic function call.  Eventually, this will be done
	** by a third argument to those functions, but not now.
	*/
	
	if ((db_stat = (*func)(adf_scb, dv[1], dv[2], &cmp)) == E_DB_OK)
	{
	    switch (fdesc->adi_fiopid)
	    {
	      case ADI_EQ_OP:
	      case ADI_LIKE_OP:
	      case ADI_ISNUL_OP:
		*(bool *)(dv[0]->db_data) = (cmp == 0);
		break;
	      case ADI_NE_OP:
	      case ADI_NLIKE_OP:
	      case ADI_NONUL_OP:
		*(bool *)(dv[0]->db_data) = (cmp != 0);
		break;
	      case ADI_LT_OP:
		*(bool *)(dv[0]->db_data) = (cmp < 0);
		break;
	      case ADI_LE_OP:
		*(bool *)(dv[0]->db_data) = (cmp <= 0);
		break;
	      case ADI_GT_OP:
		*(bool *)(dv[0]->db_data) = (cmp > 0);
		break;
	      case ADI_GE_OP:
		*(bool *)(dv[0]->db_data) = (cmp >= 0);
		break;
	    }
	}
    }

    else switch (nargs)
    {
    /* Else, does the f.i. have zero arguments? */
    /* ---------------------------------------- */
    case 0:
	db_stat = (*func)(adf_scb, dv[0]);
	break;

    /* Else, does the f.i. have one argument? */
    /* -------------------------------------- */
    case 1:
	db_stat = (*func)(adf_scb, dv[1], dv[0]);
	break;

    /* Else, does the f.i. have two arguments? */
    /* --------------------------------------- */
    case 2:
	db_stat = (*func)(adf_scb, dv[1], dv[2], dv[0]);
	break;

    /* Else, does the f.i. have three arguments? */
    /* --------------------------------------- */
    case 3:
	db_stat = (*func)(adf_scb, dv[1], dv[2], dv[3], dv[0]);
	break;

    /* Else, does the f.i. have four arguments? */
    /* --------------------------------------- */
    case 4:
	db_stat = (*func)(adf_scb, dv[1], dv[2], dv[3], dv[4], dv[0]);
	break;

    /* Else, does the f.i. have five arguments? */
    /* --------------------------------------- */
    case 5:
	db_stat = (*func)(adf_scb, dv[1], dv[2], dv[3], dv[4], dv[5], dv[0]);
	break;

    /* Else, we have a major problem here */
    /* ---------------------------------- */
    default:
	db_stat = adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0);
	break;
    }
    if (ws_alloc)
	MEfree(ws_alloc);
    return (db_stat);
}


/*
** Name: ad0_usecx() - Use a CX to perform the f.i.
**
** Description:
**      This function gets called when the f.i. lookup table does not have a
**      pointer to a routine to call for the given function instace.  In other
**      words, only the ADE expression evaluator knows how to execute it.
**	Therefore, this routine will compile a CX for the f.i. and then execute
**	it.
**
**	This routine should NOT be used to execute the ADE_REDEEM operation.
**	This cannot be done because their is no provision for returning the
**	continuation indicator in any sensible manner.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      eptr				Pointer to the escape character for LIKE
**					or NOT LIKE.  This *MUST* be set to NULL
**					if the operation is not LIKE or NOT LIKE
**					or if no escape char is in use.
**      fdesc				Pointer to the function instance
**					description for the f.i. to be executed.
**	dv[0]				Pointer to result DB_DATA_VALUE.
**	    .db_datatype		Datatype id of result.
**	    .db_length			Length of result.
**	    .db_data			Pointer to location to place
**					the actual data for the result.
**	dv[1..n]			Pointer to the DB_DATA_VALUE for
**					the nth operand, if there is one.
**	    .db_datatype		Datatype id of operand.
**	    .db_length			Length of operand.
**	    .db_data			Pointer to the actual data
**					of the operand.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**	dv[0]				Pointer to result DB_DATA_VALUE.
**	    .db_data			The actual data for the result will be
**					placed here.
**
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**          { Other error codes from the ADE module can be returned here }
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      31-jul-86 (thurston)
**          Initial creation and coding.
**	29-dec-86 (thurston)
**	    Added a conditional compilation of the ADE_AND instruction if the
**	    function instance was a comparison.  Otherwise, because of the way
**	    the expression evaluater works, the expression will always return
**	    TRUE.  This was a bug with this routine.
**	23-jun-88 (thurston)
**	    Handle the escape char for the ESCAPE clause on LIKE or NOT LIKE.
**	23-may-89 (jrb)
**	    This routine was calling ade_instr_gen with the wrong number of
**	    parms.  I added a query constant map variable to the call, but
**	    since we are compiling and then executing immediately afterward the
**	    caller really can't determine what a "query" is since there aren't
**	    distinct compilation and execution phases from his vantage point.
**	07-aug-89 (jrb)
**	    Added support for new opr_prec field in ADE_OPERAND struct.
**	13-dec-89 (fred)
**	    Added support for new ade_execute_cx() interface as required by
**	    large objects.
**	18-Nov-2004 (jenjo02)
**	    Null ADE_GLOBALBASE slot in excb_bases.
**	20-Jan-2005 (schka24)
**	    Another 64-bit stack alignment fix, for cx.
*/

static  DB_STATUS
ad0_usecx(
ADF_CB		    *adf_scb,
UCS2		    *eptr,
u_i2		    pat_flags,
ADI_FI_DESC	    *fdesc,
DB_DATA_VALUE	    *dv[ADI_MAX_OPERANDS+1])
{
    DB_STATUS		db_stat;
    union {
            char	excb_buf[sizeof(ADE_EXCB) + 
                                (ADE_ZBASE+ADI_MAX_OPERANDS+2)*sizeof(PTR)];
            PTR         align ;  /* force align at the boundary of pointer */
          }  excb_buf_align; 
    ADE_EXCB		*excb = (ADE_EXCB *) &(excb_buf_align.excb_buf[0]);
    ALIGN_RESTRICT	cxbuf[
			      (sizeof(ADE_CXHEAD) +
			      3 * sizeof(ADE_INSTRUCTION) +
			      (ADI_MAX_OPERANDS+3) * sizeof(ADE_I_OPERAND) +
			      sizeof(ALIGN_RESTRICT) - 1) / sizeof(ALIGN_RESTRICT)
			     ];			/* Buffer for CX, if needed */
    i4			qconst_map = 0;
    PTR			cx = (PTR) &cxbuf[0];
    ADE_OPERAND		ops[ADI_MAX_OPERANDS+1];
    i4                  nops;
    i4			unaligned;
    i4			cxrsize;
    i4			i;

    /* begin compile */
    /* ------------- */
    if (db_stat = ade_bgn_comp(adf_scb, cx, (i4) sizeof(cxbuf)))
	return (db_stat);

    /* if we have an escape char (for LIKE or */
    /* NOT LIKE) gen ADE_ESC_CHAR instruction */
    /* -------------------------------------- */
    if (pat_flags != AD_PAT_DOESNT_APPLY)
    {
	if ((pat_flags & AD_PAT_ISESCAPE_MASK) == AD_PAT_HAS_UESCAPE)
	{
	    ops[0].opr_dt = DB_NCHR_TYPE;
	    ops[0].opr_len = sizeof(UCS2);
	}
	else
	{
	    ops[0].opr_dt = DB_CHA_TYPE;
	    if (pat_flags & AD_PAT_HAS_ESCAPE)
		ops[0].opr_len = CMbytecnt((char*)eptr);
	    else
		ops[0].opr_len = 1;
	}
	ops[0].opr_base = ADE_ZBASE+ADI_MAX_OPERANDS;
	ops[0].opr_offset = 0;
	ops[0].opr_prec = 0;
	ops[0].opr_collID = DB_NOCOLLATION;
	nops = 1;
	if (pat_flags & ~AD_PAT_HAS_ESCAPE)
	{
	    ops[1].opr_dt = DB_INT_TYPE;
	    ops[1].opr_len = sizeof(i2);
	    ops[1].opr_base = ADE_ZBASE+ADI_MAX_OPERANDS+1;
	    ops[1].opr_offset = 0;
	    ops[1].opr_prec = 0;
	    ops[1].opr_collID = DB_NOCOLLATION;
	    nops++;
	}
	if (db_stat = ade_instr_gen(adf_scb, cx, ADE_ESC_CHAR,
			    ADE_SMAIN, nops, ops, &qconst_map, &unaligned))
	    return (db_stat);
    }

    /* set up operands */
    /* --------------- */
    nops = fdesc->adi_numargs;
    /* Adjust in case of workspace */
    if (fdesc->adi_fiflags & ADI_F4_WORKSPACE)
	nops++;
    if (fdesc->adi_fitype == ADI_COMPARISON)
    {
	/* Comparisons don't have a result operand in CX */
	for (i = 0; i < nops; i++)
	{
	    ops[i].opr_dt = dv[i+1]->db_datatype;
	    ops[i].opr_len = dv[i+1]->db_length;
	    ops[i].opr_prec = dv[i+1]->db_prec;
	    ops[i].opr_collID = dv[i+1]->db_collID;
	    ops[i].opr_base = ADE_ZBASE+i+1;
	    ops[i].opr_offset = 0;
	}
    }
    else
    {
	nops++;
	for (i = 0; i < nops; i++)
	{
	    ops[i].opr_dt = dv[i]->db_datatype;
	    ops[i].opr_len = dv[i]->db_length;
	    ops[i].opr_prec = dv[i]->db_prec;
	    ops[i].opr_collID = dv[i]->db_collID;
	    ops[i].opr_base = ADE_ZBASE+i;
	    ops[i].opr_offset = 0;
	}
    }

    /* gen instruction */
    /* --------------- */
    if (db_stat = ade_instr_gen(adf_scb, cx, (i4)fdesc->adi_finstid,
			ADE_SMAIN, nops, ops, &qconst_map, &unaligned))
	return (db_stat);

    if (fdesc->adi_fitype == ADI_COMPARISON)
    {
	if (db_stat = ade_instr_gen(adf_scb, cx, ADE_AND,
			ADE_SMAIN, 0, ops, &qconst_map, &unaligned))
	    return (db_stat);
	/* Adjust nops now to reflect that we fake the return operand */
	nops++;
    }

    /* end compile */
    /* ----------- */
    if (db_stat = ade_end_comp(adf_scb, cx, &cxrsize))
	return (db_stat);

    /* prepare excb for execution */
    /* -------------------------- */
    excb->excb_cx = cx;
    excb->excb_seg = ADE_SMAIN;
    excb->excb_bases[ADE_CXBASE] = cx;
    /* No global base array */
    excb->excb_bases[ADE_GLOBALBASE] = NULL;

    /* {@fix_me@} excb->excb_bases[ADE_DV_BASE] = ????? Front ends ????? */

    if (fdesc->adi_fitype != ADI_COMPARISON)
	excb->excb_bases[ADE_ZBASE+0] = dv[0]->db_data;
    for (i = 1; i < nops; i++)
	excb->excb_bases[ADE_ZBASE+i] = dv[i]->db_data;
    excb->excb_bases[ADE_ZBASE+ADI_MAX_OPERANDS] = (PTR) eptr;
    excb->excb_bases[ADE_ZBASE+ADI_MAX_OPERANDS+1] = (PTR) &pat_flags;

    excb->excb_nbases = ADE_ZBASE+ADI_MAX_OPERANDS+2;
    excb->excb_size = 0;
    excb->excb_limited_base = ADE_NOBASE;
    excb->excb_continuation = 0;

    /* now execute the CX */
    /* ------------------ */
    if (db_stat = ade_execute_cx(adf_scb, excb))
	return (db_stat);


    /* if comparison, do funny stuff */
    /* ----------------------------- */
    if (fdesc->adi_fitype == ADI_COMPARISON)
    {
	*(bool *)(dv[0]->db_data) = excb->excb_value;
    }

    return (db_stat);
}

/*
** Name: adf_exec_quick() - Quickly execute a function instance
**
** Description:
**	This function quickly executes a non-aggregate, non-CX function instance
**	with non-NULL inputs.  Note that the LIKE and NOT LIKE operations are
**	not supported here (because the escape character cannot be passed to the
**	pattern matching routine yet).
**
**	This routine provides a way for pre-validated operations to execute
**	quickly much like a CX would.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**      fid				Function instance id. to be executed.
**	dvarr				Array of db_data_values, 0th item is
**					the result, the rest of them are inputs.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**	dvarray[0]			Result of function instance.
**
**      Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0000_OK                 Operation succeeded.
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	16-apr-90 (jrb)
**	    Created.
*/

# ifdef ADF_BUILD_WITH_PROTOS
DB_STATUS
adf_exec_quick(
ADF_CB		*adf_scb,
ADI_FI_ID	fid,
DB_DATA_VALUE	*dvarr)
# else
DB_STATUS
adf_exec_quick( adf_scb, fid, dvarr)
ADF_CB		*adf_scb;
ADI_FI_ID	fid;
DB_DATA_VALUE	*dvarr;
# endif
{
    DB_STATUS	    db_stat;
    DB_STATUS	    (*func)();
    i4		    cmp;
    ADI_FI_DESC	    *fidesc;
    i4		    nargs;

    if ((func =
	Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(fid)].adi_func)
	== NULL)
    {
	return (adu_error(adf_scb, E_AD2010_BAD_FIID, 0));
    }

    fidesc = Adf_globs->Adi_fi_lkup[ADI_LK_MAP_MACRO(fid)].adi_fi;
    nargs = fidesc->adi_numargs;

    if (fidesc->adi_fitype == ADI_COMPARISON)
    {
	/* NOTE: although LIKE and NLIKE are listed here, this routine really
	** does NOT support them yet.
	*/
	if ((db_stat = (*func)(adf_scb, dvarr[1], dvarr[2], &cmp)) == E_DB_OK)
	{
	    switch (fidesc->adi_fiopid)
	    {
	      case ADI_EQ_OP:
	      case ADI_LIKE_OP:
	      case ADI_ISNUL_OP:
		*(bool *)(dvarr[0].db_data) = (cmp == 0);
		break;
	      case ADI_NE_OP:
	      case ADI_NLIKE_OP:
	      case ADI_NONUL_OP:
		*(bool *)(dvarr[0].db_data) = (cmp != 0);
		break;
	      case ADI_LT_OP:
		*(bool *)(dvarr[0].db_data) = (cmp < 0);
		break;
	      case ADI_LE_OP:
		*(bool *)(dvarr[0].db_data) = (cmp <= 0);
		break;
	      case ADI_GT_OP:
		*(bool *)(dvarr[0].db_data) = (cmp > 0);
		break;
	      case ADI_GE_OP:
		*(bool *)(dvarr[0].db_data) = (cmp >= 0);
		break;
	    }
	}
	return (db_stat);
    }

    /* Else, does the f.i. have zero arguments? */
    /* ---------------------------------------- */
    else if (nargs == 0)
	return ((*func)(adf_scb, &dvarr[0]));

    /* Else, does the f.i. have one argument? */
    /* -------------------------------------- */
    else if (nargs == 1)
	return ((*func)(adf_scb, &dvarr[1], &dvarr[0]));

    /* Else, does the f.i. have two arguments? */
    /* --------------------------------------- */
    else if (nargs == 2)
	return ((*func)(adf_scb, &dvarr[1], &dvarr[2], &dvarr[0]));

    /* Else, does the f.i. have three arguments? */
    /* --------------------------------------- */
    else if (nargs == 3)
	return ((*func)(adf_scb, &dvarr[1], &dvarr[2], &dvarr[3], &dvarr[0]));

    /* Else, does the f.i. have four arguments? */
    /* --------------------------------------- */
    else if (nargs == 4)
	return ((*func)(adf_scb, &dvarr[1], &dvarr[2], &dvarr[3], &dvarr[2], &dvarr[0]));

    /* Else, we have a major problem here */
    /* ---------------------------------- */
    return (adu_error(adf_scb, E_AD9999_INTERNAL_ERROR, 0));
}

/*
** Name: ad0_isdtfamily_same - Checks to compare to see if the two datatypes 
**			       belong to the same datatype family.
**
** Description:
**	This routine is used to compare two datatypes to find out if they 
**	belong to the same datatype family. 
**	This is used to validate a function instance and make sure that the
**	operands this function instance is selected for belongs to the 
**	same datatype family.
**	It returns true if the datatypes are of same datatype family, 
**	else it returns false. 
**
**	Assumption is that the passed datatypes are absolute.
**
** History:
**	13-sep-2006 (gupsh01)
**	    Created.
**	08-May-2009 (kiria01) SIR 121012
**	    Correct the indexing of Adi_dtptrs for UDTs
*/
bool
ad0_isdtfamily_same(
DB_DT_ID	dt1,
DB_DT_ID	dt2 )
{
    DB_DT_ID            fdt1 = 0;
    DB_DT_ID            fdt2 = 0;
    ADI_DATATYPE        *dt;

    dt = Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(dt1)];
    if (dt)
      fdt1 = dt->adi_dtfamily;

    dt = Adf_globs->Adi_dtptrs[ADI_DT_MAP_MACRO(dt2)];
    if (dt)
      fdt2 = dt->adi_dtfamily;

    return (fdt1 == fdt2);
}

