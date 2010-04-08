/*
**	drveval.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/


/* # includes's */
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<afe.h>
# include	<frame.h>
# include	<frsctblk.h>
# include	"dnode.h"
# include	"derive.h"
# include	<ex.h>
# include	<si.h>
# include	<er.h>
# include	<me.h>
# include	<nm.h>
# include	<ug.h>
# include	<erfi.h>
# include	"erfv.h"
# include	<erug.h>

/*
**  Name:	drveval.c - Evaluate a derived field.
**
**  Description:
**
**	This file contains routines to evaluate/execute the derivation
**	formula for a field/column.
**
**	This file defines:
**
**	IIFViaInvalidateAgg	Invalidate aggregate values.
**	IIFVliaLocInvalAgg	Invalidate aggregate values.
**	IIFVedEvalDerive	Evalute the derivation formula for a field.
**	IIFVadfxhdlr		Exception handler for derivation evaluation.
**	IIFVcaCalcAgg		Calculate aggregates for a column (slow).
**	IIFVfcFindCol		Find cross column values.
**	IIFVeaEvalAgg		Evaluate aggregates in a derivation tree (fast).
**	IIFVfdeFastDrvEval	Fast derivation evaluation.
**	IIFVddeDoDeriveEval	Slow derivation evaluation routine.
**
**  TO DOs:
**	May want to trace execution to file at some future time.
**
**  History:
**	06/26/89 (dkh) - Initial version.
**	09-jan-90 (bruceb)
**		Modified the IIFVia routine (and added IIFVlia) to more
**		closely parallel the set/get mechanisms of derivation
**		evaluation.
**	19-oct-1990 (mgw)
**		Fixed #include of local dnode.h and derive.h to use ""
**		instead of <>
**	11/26/90 (dkh) - Added EXDECOVF, EXDECDIV, EXMNYDIV and EXMNYOVF
**			 cases to exception handler.
**	01/13/91 (dkh) - Put in changes for IIFVadfxhdlr per suggestions
**			 from daveb.
**	06/13/92 (dkh) - Added support for decimal datatype for 6.5.
**	03/04/93 (dkh) - Fixed bug 49987.  Added missing EXdelete() calls.
**      24-sep-96 (hanch04)
**          Global data moved to data.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* # define's */
# define	NOCOLNO		BADCOLNO

GLOBALREF	bool		IIFVcvConstVal ;
GLOBALREF	DB_DATA_VALUE	*IIFVdc1DbvConst ;
GLOBALREF	DB_DATA_VALUE	*IIFVdc2DbvConst ;


/* extern's */
FUNC_EXTERN	ADF_CB	*FEadfcb();
FUNC_EXTERN	STATUS	afe_cvinto();
FUNC_EXTERN	FLDHDR	*FDgethdr();
FUNC_EXTERN	DB_LANG	IIAFdsDmlSet();
FUNC_EXTERN	i4	(*IIseterr())();

FUNC_EXTERN	STATUS	IIFVadfxhdlr();
FUNC_EXTERN	VOID	IIFVliaLocInvalAgg();

GLOBALREF	FRS_RFCB	*IIFDrfcb;
GLOBALREF	FRS_EVCB	*IIFDevcb;


/* static's */
static	bool	need_to_init = TRUE;
static	bool	slow = TRUE;

static	bool	use1 = TRUE;

static	i4	(*olderr)() = NULL;


/*{
** Name:	IIFViaInvaldateAgg - Invalidate aggregate values.
**
** Description:
**	This routine is called to invalidate the various aggregate
**	values.  What to invalidate is passed in as parameter "type".
**
** Inputs:
**	colhdr		Pointer to a FLDHDR structure for a column.
**	type		Type of invalidation to perform:
**			fdIANODE - just invalidate the immediate
**				   aggregate value.
**			fdIAGET  - invalidate those nodes that may be
**				   modified during a column validation.
**			fdIASET  - invalidate those nodes that may be
**				   modified when setting the value of
**				   a column.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/26/89 (dkh) - Initial version.
*/
VOID
IIFViaInvalidateAgg(colhdr, type)
FLDHDR	*colhdr;
i4	type;
{
    if (colhdr->fhdrv == NULL)
    {
	return;
    }

    switch (type)
    {
	case fdIANODE:
	case fdIASET:
	    IIFVliaLocInvalAgg(colhdr, type);
	    break;

	case fdIAGET:
	    /*
	    ** The single instance of a call on IIFVia() with fdIAGET
	    ** occurs in tbutil.c.  The code there calls FDgetdata, which
	    ** in turn derives source fields, and then dependents of the
	    ** specified table field cell.  Calling IIFVlia() with fdIAGET,
	    ** and then with fdIASET, mirrors that action.
	    */
	    IIFVliaLocInvalAgg(colhdr, fdIAGET);
	    IIFVliaLocInvalAgg(colhdr, fdIASET);
	    break;
    }
}


/*{
** Name:	IIFVliaLocInvalAgg - Invalidate aggregate values.
**
** Description:
**	This routine is called to invalidate the various aggregate
**	values.  What to invalidate is passed in as parameter "type".
**	Called recursively to follow the source/dependent chains
**	while performing the bulk of the work. 
**
** Inputs:
**	colhdr		Pointer to a FLDHDR structure for a column.
**	type		Type of invalidation to perform:
**			fdIANODE - just invalidate the immediate
**				   aggregate value.
**			fdIAGET  - invalidate those nodes that may be
**				   modified during a column validation.
**			fdIASET  - invalidate those nodes that may be
**				   modified when setting the value of
**				   a column.
**
** Outputs:
**	None.
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	01/09/90 (bruceb) - Initial version.
*/
VOID
IIFVliaLocInvalAgg(colhdr, type)
FLDHDR	*colhdr;
i4	type;
{
    DRVAGG	*agginfo;
    DRVREL	*drvlist;
    FIELD	*fld;
    FLDCOL	*col;
    FLDHDR	*hdr;
    i4		colnum;

    if ((agginfo = colhdr->fhdrv->agginfo) != NULL)
    {
	agginfo->aggflags &= ~AGG_ISVALID;
    }

    if (type == fdIANODE)
    {
	return;
    }
    else if (type == fdIASET)
    {
	colhdr->fhd2flags |= fdVISITED;
	for (drvlist = colhdr->fhdrv->deplist; drvlist != NULL;
	    drvlist = drvlist->next)
	{
	    if ((colnum = drvlist->colno) == NOCOLNO)
	    {
		continue;
	    }
	    else
	    {
		fld = drvlist->fld;
		col = fld->fld_var.fltblfld->tfflds[colnum];
		hdr = &(col->flhdr);
		/*
		** Deriving dependent values of the cell will take the
		** form of obtaining the values of all sources for all
		** dependents, then going down another level.  Mirror that
		** action here.
		*/
		IIFVliaLocInvalAgg(hdr, fdIAGET);
		IIFVliaLocInvalAgg(hdr, fdIASET);
	    }
	}
	colhdr->fhd2flags &= ~fdVISITED;
    }
    else /* type == fdIAGET */
    {
	for (drvlist = colhdr->fhdrv->srclist; drvlist != NULL;
	    drvlist = drvlist->next)
	{
	    colnum = drvlist->colno;
	    fld = drvlist->fld;
	    col = fld->fld_var.fltblfld->tfflds[colnum];
	    hdr = &(col->flhdr);
	    if (hdr->fhd2flags & fdVISITED)
	    {
		/* Prevent repetitive work on already invalidated columns. */
		continue;
	    }
	    else
	    {
		/* Invalidate aggregates for all sources for this cell. */
		IIFVliaLocInvalAgg(hdr, fdIAGET);
	    }
	}
    }
}



/*{
** Name:	IIFVedEvalDerive - Evalute the derivation formula for a field.
**
** Description:
**	Main entry point for evaluating the derivation formula for a
**	field/column.  This routine first determines whether to use
**	a slow or fast derivation method.  Constant derivation formulas
**	are done only once.
**
**	Evaluation is not done (FAIL returned) if IIFDevcb->eval_aggs
**	is FALSE.
**
** Inputs:
**	frm		Form containing field/column to be evaluated.
**	hdr		Pointer to FLDHDR structure for field/column.
**	type		Pointer to FLDTYPE structure for field/column.
**
** Outputs:
**	dbv		Result of evaluation.  If NULL, no copying is
**			performed.
**
**	Returns:
**		OK	Evaluation was sucessful.
**		FAIL	Evaluation failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	Aggregate values may be recalculated.
**
** History:
**	06/26/89 (dkh) - Initial version.
*/
STATUS
IIFVedEvalDerive(frm, hdr, type, dbv)
FRAME		*frm;
FLDHDR		*hdr;
FLDTYPE		*type;
DB_DATA_VALUE	*dbv;
{
	DB_DATA_VALUE	*result;
	STATUS		retval;
	ADF_CB		*adfcb;
	DNODE		*node;
	char		*cp;
	char		errbuf[ER_MAX_LEN + 1];
	i4		errlen = ER_MAX_LEN;
	EX_CONTEXT	context;

	if (type->ftvalchk == NULL)
	{
		return(OK);
	}

	/*
	**  If a constant value and its been derived already,
	**  send it back.
	*/
	if ((hdr->fhdrv->drvflags & fhDRVCNST) &&
		(hdr->fhdrv->drvflags & fhDRVDCNST))
	{
		adfcb = FEadfcb();
		node = (DNODE *) type->ftvalchk;
		if (node->cresult)
		{
			result = node->cresult;
		}
		else
		{
			result = node->result;
		}

		/*
		**  Only copy to output dbv if not NULL.
		*/
		if (dbv != NULL)
		{
			if (EXdeclare(IIFVadfxhdlr, &context) != OK)
			{
				EXdelete();
				return(FAIL);
			}

			if (afe_cvinto(adfcb, result, dbv) != OK)
			{
				/*
				**  Error moving value to field.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D3_8403, 2, hdr->fhdname,
					errbuf);
				(VOID) IIseterr(olderr);
				EXdelete();
				return(FAIL);
			}
			EXdelete();
		}
		return(OK);
	}

	/*
	**  If field depends on an aggregate but this evaluation is
	**  triggered by application code, then don't do any
	**  evaluation for now.
	*/
	if ((hdr->fhdrv->drvflags & fhDRVAGGDEP) && !IIFDevcb->eval_aggs)
	{
		return(FAIL);
	}

	if (need_to_init)
	{
		NMgtAt(ERx("II_SLOW_DRV"), &cp);
		if (cp == NULL || *cp == EOS)
		{
			slow = FALSE;
		}
		else
		{
			slow = TRUE;
		}
		need_to_init = FALSE;
	}
	if (slow)
	{
		if ((retval = IIFVddeDoDeriveEval(frm, (DNODE *)type->ftvalchk,
			hdr, &result)) != OK)
		{
			/*
			**  Error messages have already been
			**  put out by lower level routines.
			**  Just return.
			*/
			return(retval);
		}
	}
	else
	{
		if ((retval = IIFVfdeFastDrvEval(frm, (DNODE *)type->ftvalchk,
			hdr, &result)) != OK)
		{
			/*
			**  Error messages have already been
			**  put out by lower level routines.
			**  Just return.
			*/
			return(retval);
		}
	}

	/*
	**  Assign result to field value (which may require coercion.
	*/
	adfcb = FEadfcb();

	/*
	**  Only copy to output dbv if not NULL.
	*/
	if (dbv != NULL)
	{
		if (EXdeclare(IIFVadfxhdlr, &context) != OK)
		{
			EXdelete();
			return(FAIL);
		}

		if (afe_cvinto(adfcb, result, dbv) != OK)
		{
			/*
			**  Error moving value into field.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20D3_8403, 2, hdr->fhdname, errbuf);
			(VOID) IIseterr(olderr);
			EXdelete();
			return(FAIL);
		}
		EXdelete();
	}

	/*
	**  Set flag indicatiing that a constant derivation
	**  has already been calculated.
	*/
	if (hdr->fhdrv->drvflags & fhDRVCNST)
	{
		hdr->fhdrv->drvflags |= fhDRVDCNST;
	}

	return(OK);
}




/*{
** Name:	IIFVadfxhdlr - Exception handler for derivation evaluation.
**
** Description:
**	This is the exception handler for derivation evaluation.
**	Its basically here to handle math exceptions on systems
**	that signal such occurrences.  Exceptions that are handled are:
**
**	EXFLTOVF	Floating overflow.
**	EXINTOVF	Integer overflow.
**	EXFLTUND	Floating underflow.
**	EXINTDIV	Integer divide.
**	EXFLTDIV	Floating divide.
**	EXMNYDIV	Money divide.
**	EXMNYOVF	Money overflow.
**	EXDECDIV	Decimal divide.
**	EXDECOVF	Decimal overflow.
**
**	The following hard math exceptions, if supported on a
**	particular platform, causes an exit.
**
**	EXHFLTDIV	Hard floating divide by zero.
**	EXHFLTOVF	Hard floating overflow.
**	EXHFLTUND	Hard floating underflow.
**	EXHINTDIV	Hard integer divide by zero.
**	EXHINTOVF	Hard integer overflow.
**
**	The above exceptions generate error for terminal display and
**	then cause stack to unwind to declaring routine.
**
**	Any other exceptions are simply resignalled for handling by
**	higher level exception handlers.
**
** Inputs:
**	ex			Exception that occurred.
**
** Outputs:
**	None.
**
**	Returns:
**		EXRESIGNAL	Re-signal exception if sent an exception
**				that it does not know how to handle.
**		EXDECLARE	Tell exception mechanism to return to
**				the declaring routine to unwind stack
**				after handling exception.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	06/26/89 (dkh) - Initial version.
*/
STATUS
IIFVadfxhdlr(ex)
EX_ARGS	*ex;
{
	i4	ex_index;
	char	*reason;
	char	buf[ER_MAX_LEN + 1];

	olderr = IIseterr(NULL);
	if (ex->exarg_num == EXFLTOVF || ex->exarg_num == EXINTOVF)
	{
		IIFDerror(E_FI20C8_8392, 0);
	}
	else if (ex->exarg_num == EXFLTUND)
	{
		IIFDerror(E_FI20C9_8393, 0);
	}
	else if (ex->exarg_num == EXINTDIV)
	{
		IIFDerror(E_FI20CA_8394, 0);
	}
	else if (ex->exarg_num == EXFLTDIV)
	{
		IIFDerror(E_FI20CB_8395, 0);
	}
	else if (ex->exarg_num == EXMNYDIV)
	{
		IIFDerror(E_FI20D8_8408, 0);
	}
	else if (ex->exarg_num == EXDECDIV)
	{
		IIFDerror(E_FI20D9_8409, 0);
	}
	else if (ex->exarg_num == EXMNYOVF)
	{
		IIFDerror(E_FI20DA_8410, 0);
	}
	else if (ex->exarg_num == EXDECOVF)
	{
		IIFDerror(E_FI20DB_8411, 0);
	}
# ifdef EX_HARD_MATH
	else if ((ex_index = EXmatch(ex, 5, EXHFLTDIV, EXHFLTOVF, EXHFLTUND,
		EXHINTDIV, EXHINTOVF)) != 0)
	{
		FTclose(S_UG0030_Exiting);
		STcopy(S_UG0031_Because, buf);
		switch (ex_index)
		{
			case 1:
				reason = ERget(S_FV0022_FloatHardDivide);
				break;

			case 2:
				reason = ERget(S_FV0021_FloatHardOverflow);
				break;

			case 3:
				reason = ERget(S_FV0022_FloatHardUnderflow);
				break;

			case 4:
				reason = ERget(S_FV0023_IntHardDivide);
				break;

			case 5: reason = ERget(S_FV0024_IntHardOverflow);
				break;
		}
		STcat(buf, reason);
		STcat(buf, ERget(S_FV0025_DuringDerive);
		STcat(buf, ERget(S_UG003f_Contact);
		STcat(buf, ERx("\n"));
		SIfprintf(stdout, buf);
		SIflush(stdout);
		PCexit(0);
	}
# endif
	else
	{
		(VOID) IIseterr(olderr);
		return(EXRESIGNAL);
	}

	(VOID) IIseterr(olderr);
	return(EXDECLARE);
}



/*{
** Name:	IIFVcaCalcAgg - Calculate aggreates for a column.
**
** Description:
**	Calculate the aggregates for a column using slow method.
**	Note that ADF currently ignores absolute dates when doing
**	date aggregates and returns OK.  If this changes, then
**	the code below must change.
**
** Inputs:
**	frm		Form containing column to be evaluated
**	node		Pointer to DNODE for field that depends on agg.
**	hdr		FLDHDR pointer of field that depnds on agg (unused).
**	agginfo		Pointer to DRVAGG for column to be aggregated.
**
** Outputs:
**
**	Returns:
**		OK	Evaluation was successful.
**		FAIL	Evaluation failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	Causes table field displayed values and internal values
**	to be synchronized and cross column values to be evaluated.
**
** History:
**	06/26/89 (dkh) - Initial version.
*/
STATUS
IIFVcaCalcAgg(frm, node, hdr, agginfo)
FRAME	*frm;
DNODE	*node;
FLDHDR	*hdr;
DRVAGG	*agginfo;
{
	ADF_CB		*adfcb;
	DB_DATA_VALUE	*dbv;
	DAGGVAL		*cntagg;
	DAGGVAL		*maxagg;
	DAGGVAL		*minagg;
	DAGGVAL		*avgagg;
	DAGGVAL		*sumagg;
	char		errbuf[ER_MAX_LEN + 1];
	i4		errlen = ER_MAX_LEN;

	/*
	**  Call setup routine for running aggregates.
	*/

	adfcb = FEadfcb();

	if ((*IIFDrfcb->aggsetup)(frm, node->fld, node->colno) != OK)
	{
		/*
		**  Should never happen.
		*/
		return(FAIL);
	}

	if ((cntagg = agginfo->cntagg) != NULL)
	{
		if (adf_agbegin(adfcb, &(cntagg->adfagstr)) != OK)
		{
			/*
			**  Aggregate setup failure.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20CD_8397, 3, ERx("count"), hdr->fhdname,
				errbuf);
			(VOID) IIseterr(olderr);
			return(FAIL);
		}
	}

	if ((maxagg = agginfo->maxagg) != NULL)
	{
		if (adf_agbegin(adfcb, &(maxagg->adfagstr)) != OK)
		{
			/*
			**  Aggregate setup failure.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20CD_8397, 3, ERx("max"), hdr->fhdname,
				errbuf);
			(VOID) IIseterr(olderr);
			return(FAIL);
		}
	}

	if ((minagg = agginfo->minagg) != NULL)
	{
		if (adf_agbegin(adfcb, &(minagg->adfagstr)) != OK)
		{
			/*
			**  Aggregate setup failure.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20CD_8397, 3, ERx("min"), hdr->fhdname,
				errbuf);
			(VOID) IIseterr(olderr);
			return(FAIL);
		}
	}

	if ((avgagg = agginfo->avgagg) != NULL)
	{
		if (adf_agbegin(adfcb, &(avgagg->adfagstr)) != OK)
		{
			/*
			**  Aggregate setup failure.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20CD_8397, 3, ERx("avg"),
				hdr->fhdname, errbuf);
			(VOID) IIseterr(olderr);
			return(FAIL);
		}
	}

	if ((sumagg = agginfo->sumagg) != NULL)
	{
		if (adf_agbegin(adfcb, &(sumagg->adfagstr)) != OK)
		{
			/*
			**  Aggregate setup failure.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20CD_8397, 2, ERx("sum"), hdr->fhdname,
				errbuf);
			(VOID) IIseterr(olderr);
			return(FAIL);
		}
	}

	for (;;)
	{
		(*IIFDrfcb->aggnxval)(&dbv);
		if (dbv == NULL)
		{
			break;
		}
		if (cntagg)
		{
			if (adf_agnext(adfcb, dbv,
				&(cntagg->adfagstr)) != OK)
			{
				/*
				**  Aggregate calculation failure.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20CE_8398, 3, ERx("count"),
					hdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if (maxagg)
		{
			if (adf_agnext(adfcb, dbv,
				&(maxagg->adfagstr)) != OK)
			{
				/*
				**  Aggregate calculation failure.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20CE_8398, 3, ERx("max"),
					hdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if (minagg)
		{
			if (adf_agnext(adfcb, dbv,
				&(minagg->adfagstr)) != OK)
			{
				/*
				**  Aggregate calculation failure.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20CE_8398, 3, ERx("min"),
					hdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if (avgagg)
		{
			if (adf_agnext(adfcb, dbv,
				&(avgagg->adfagstr)) != OK)
			{
				/*
				**  Aggregate calculation failure.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20CE_8398, 3, ERx("avg"),
					hdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if (sumagg)
		{
			if (adf_agnext(adfcb, dbv,
				&(sumagg->adfagstr)) != OK)
			{
				/*
				**  Aggregate calculation failure.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20CE_8398, 3, ERx("sum"),
					hdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
	}

	/*
	**  Now close the aggregates and set the values.
	*/
	if (cntagg)
	{
		if (adf_agend(adfcb, &(cntagg->adfagstr),
			&(cntagg->aggdbv)) != OK)
		{
			/*
			**  Aggregate completion failure.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20CF_8399, 3, ERx("count"),
				hdr->fhdname, errbuf);
			(VOID) IIseterr(olderr);
			return(FAIL);
		}
	}
	if (maxagg)
	{
		if (adf_agend(adfcb, &(maxagg->adfagstr),
			&(maxagg->aggdbv)) != OK)
		{
			/*
			**  Aggregate completion failure.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20CF_8399, 3, ERx("max"),
				hdr->fhdname, errbuf);
			(VOID) IIseterr(olderr);
			return(FAIL);
		}
	}
	if (minagg)
	{
		if (adf_agend(adfcb, &(minagg->adfagstr),
			&(minagg->aggdbv)) != OK)
		{
			/*
			**  Aggregate completion failure.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20CF_8399, 3, ERx("min"),
				hdr->fhdname, errbuf);
			(VOID) IIseterr(olderr);
			return(FAIL);
		}
	}
	if (avgagg)
	{
		if (adf_agend(adfcb, &(avgagg->adfagstr),
			&(avgagg->aggdbv)) != OK)
		{
			/*
			**  Aggregate completion failure.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20CF_8399, 3, ERx("avg"),
				hdr->fhdname, errbuf);
			(VOID) IIseterr(olderr);
			return(FAIL);
		}
	}
	if (sumagg)
	{
		if (adf_agend(adfcb, &(sumagg->adfagstr),
			&(sumagg->aggdbv)) != OK)
		{
			/*
			**  Aggregate completion failure.
			**  Get adf error by calling afe_errtostr().
			**  Ignoring return value since am passing
			**  a buffer of ER_MAX_LEN long, which is the
			**  max error string that can be in a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20CF_8399, 3, ERx("sum"),
				hdr->fhdname, errbuf);
			(VOID) IIseterr(olderr);
			return(FAIL);
		}
	}

	return(OK);
}



/*{
** Name:	IIFVfcFindCol - Find cross column values.
**
** Description:
**	This routine is called to obtain the proper table field
**	column values before a compiled evaluation is done.
**	The entire derivation tree is descended.
**	This is necessary to handle cross column derivations
**	that are not currently visible (e.g., a value loaded
**	into the dataset by the application).
**
** Inputs:
**	frm		Form containing the evaluation to be done.
**	node		Root DNODE for a derivation formula.
**
** Outputs:
**
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	Noe.
**
** History:
**	06/26/89 (dkh) - Initial version.
*/
VOID
IIFVfcFindCol(frm, node)
FRAME	*frm;
DNODE	*node;
{
	DB_DATA_VALUE	*dbv;

	if (node->nodetype == TCOL_NODE)
	{
		/*
		**  Just copy data value since the DBVs are exactly the same.
		*/
		dbv = (*IIFDrfcb->getTFdbv)(frm, node->fld, node->colno);
		MEcopy(dbv->db_data, dbv->db_length, node->result->db_data);
	}
	if (node->left)
	{
		IIFVfcFindCol(frm, node->left);
	}
	if (node->right)
	{
		IIFVfcFindCol(frm, node->right);
	}
}





/*{
** Name:	IIFVeaEvalAgg - Evaluate aggregates in a derivation tree.
**
** Description:
**	The derivation tree is descended to find and evaluate any
**	aggregates that the tree depends on.  Note that this uses
**	the faster compiled version of aggregate processing.  Call
**	back routines into "tbacc" are used to obtain the rows in
**	the dataset.  Even if the tree only depends on one aggregate
**	of a particular column, all aggregates for the column are
**	evaluated at the same time.  If an aggregate is already
**	marked as being valid, no work is done.
**
**	Note that ADF currently ignores absolute dates when doing
**	date aggregates and returns OK.  If this changes, then
**	the code below must change.
**
** Inputs:
**	frm		Form conataining aggregates to be evaulated.
**	node		Root DNODE of a field that depends on an aggregate.
**
** Outputs:
**
**	Returns:
**		OK	Evaluation was sucessful.
**		FAIL	Evaluation failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	Values in the table field will be synchronized and any
**	Cross column derivations will be performed for rows in table field. 
**
** History:
**	06/26/89 (dkh) - Initial version.
*/
STATUS
IIFVeaEvalAgg(frm, node)
FRAME	*frm;
DNODE	*node;
{
	ADF_CB		*adfcb;
	DB_DATA_VALUE	*dbv;
	DRVAGG		*agginfo;
	DAGGVAL		*cntagg;
	DAGGVAL		*maxagg;
	DAGGVAL		*minagg;
	DAGGVAL		*avgagg;
	DAGGVAL		*sumagg;
	ADE_EXCB	*cnt_excb;
	ADE_EXCB	*max_excb;
	ADE_EXCB	*min_excb;
	ADE_EXCB	*avg_excb;
	ADE_EXCB	*sum_excb;
	i4		goodvals = FALSE;
	char		errbuf[ER_MAX_LEN + 1];
	i4		errlen = ER_MAX_LEN;

	if (!IIFDevcb->eval_aggs)
	{
		return(FAIL);
	}

	switch (node->nodetype)
	{
	    case MAX_NODE:
	    case MIN_NODE:
	    case AVG_NODE:
	    case SUM_NODE:
	    case CNT_NODE:
		adfcb = FEadfcb();
		agginfo = node->agghdr->fhdrv->agginfo;
		if (agginfo->aggflags & AGG_ISVALID)
		{
			goodvals = TRUE;
			cntagg = agginfo->cntagg;
			maxagg = agginfo->maxagg;
			minagg = agginfo->minagg;
			avgagg = agginfo->avgagg;
			sumagg = agginfo->sumagg;
			break;
		}
		if ((*IIFDrfcb->aggsetup)(frm, node->fld, node->colno) != OK)
		{
			/*
			**  Should never happen.
			*/
			return(FAIL);
		}
		if ((cntagg = agginfo->cntagg) != NULL)
		{
			cnt_excb = cntagg->agg_excb;
			cnt_excb->excb_seg = ADE_SINIT;
			cnt_excb->excb_nbases = ADE_ZBASE;
			if (ade_execute_cx(adfcb, cnt_excb) != OK)
			{
				/*
				**  Aggregate setup failure.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D4_8404, 3, ERx("count"),
					node->agghdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if ((maxagg = agginfo->maxagg) != NULL)
		{
			max_excb = maxagg->agg_excb;
			max_excb->excb_seg = ADE_SINIT;
			max_excb->excb_nbases = ADE_ZBASE;
			if (ade_execute_cx(adfcb, max_excb) != OK)
			{
				/*
				**  Aggregate setup failure.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D4_8404, 3, ERx("max"),
					node->agghdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if ((minagg = agginfo->minagg) != NULL)
		{
			min_excb = minagg->agg_excb;
			min_excb->excb_seg = ADE_SINIT;
			min_excb->excb_nbases = ADE_ZBASE;
			if (ade_execute_cx(adfcb, min_excb) != OK)
			{
				/*
				**  Aggregate setup failure.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D4_8404, 3, ERx("min"),
					node->agghdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if ((avgagg = agginfo->avgagg) != NULL)
		{
			avg_excb = avgagg->agg_excb;
			avg_excb->excb_seg = ADE_SINIT;
			avg_excb->excb_nbases = ADE_ZBASE;
			if (ade_execute_cx(adfcb, avg_excb) != OK)
			{
				/*
				**  Aggregate setup failure.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D4_8404, 3, ERx("avg"),
					node->agghdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if ((sumagg = agginfo->sumagg) != NULL)
		{
			sum_excb = sumagg->agg_excb;
			sum_excb->excb_seg = ADE_SINIT;
			sum_excb->excb_nbases = ADE_ZBASE;
			if (ade_execute_cx(adfcb, sum_excb) != OK)
			{
				/*
				**  Aggregate setup failure.
				**  Get adf error by calling afe_errtostr().
				**  Ignoring return value since am passing
				**  a buffer of ER_MAX_LEN long, which is the
				**  max error string that can be in a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D4_8404, 3, ERx("sum"),
					node->agghdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}

		if (cntagg)
		{
			cnt_excb->excb_seg = ADE_SMAIN;
			cnt_excb->excb_nbases = ADE_ZBASE + 1;
		}
		if (maxagg)
		{
			max_excb->excb_seg = ADE_SMAIN;
			max_excb->excb_nbases = ADE_ZBASE + 1;
		}
		if (minagg)
		{
			min_excb->excb_seg = ADE_SMAIN;
			min_excb->excb_nbases = ADE_ZBASE + 1;
		}
		if (avgagg)
		{
			avg_excb->excb_seg = ADE_SMAIN;
			avg_excb->excb_nbases = ADE_ZBASE + 1;
		}
		if (sumagg)
		{
			sum_excb->excb_seg = ADE_SMAIN;
			sum_excb->excb_nbases = ADE_ZBASE + 1;
		}

		for (;;)
		{
			(*IIFDrfcb->aggnxval)(&dbv);
			if (dbv == NULL)
			{
				/*
				**  No more values, get out of loop.
				*/
				break;
			}
			if (cntagg)
			{
				cnt_excb->excb_bases[ADE_ZBASE+1] =dbv->db_data;
				if (ade_execute_cx(adfcb, cnt_excb) != OK)
				{
					/*
					**  Aggregate calculation failure.
					**  Get adf error by calling
					**  afe_errtostr().  Ignoring return
					**  value since am passing a buffer
					**  of ER_MAX_LEN long, which is the
					**  max error string that can be in
					**  a msg file.
					*/
					olderr = IIseterr(NULL);
					(VOID) afe_errtostr(adfcb, errbuf,
						&errlen);
					IIFDerror(E_FI20D5_8405, 3,
						ERx("count"),
						node->agghdr->fhdname, errbuf);
					(VOID) IIseterr(olderr);
					return(FAIL);
				}
			}
			if (maxagg)
			{
				max_excb->excb_bases[ADE_ZBASE+1] =dbv->db_data;
				if (ade_execute_cx(adfcb, max_excb) != OK)
				{
					/*
					**  Aggregate calculation failure.
					**  Get adf error by calling
					**  afe_errtostr().  Ignoring return
					**  value since am passing a buffer
					**  of ER_MAX_LEN long, which is the
					**  max error string that can be in
					**  a msg file.
					*/
					olderr = IIseterr(NULL);
					(VOID) afe_errtostr(adfcb, errbuf,
						&errlen);
					IIFDerror(E_FI20D5_8405, 3, ERx("max"),
						node->agghdr->fhdname, errbuf);
					(VOID) IIseterr(olderr);
					return(FAIL);
				}
			}
			if (minagg)
			{
				min_excb->excb_bases[ADE_ZBASE+1] =dbv->db_data;
				if (ade_execute_cx(adfcb, min_excb) != OK)
				{
					/*
					**  Aggregate calculation failure.
					**  Get adf error by calling
					**  afe_errtostr().  Ignoring return
					**  value since am passing a buffer
					**  of ER_MAX_LEN long, which is the
					**  max error string that can be in
					**  a msg file.
					*/
					olderr = IIseterr(NULL);
					(VOID) afe_errtostr(adfcb, errbuf,
						&errlen);
					IIFDerror(E_FI20D5_8405, 3, ERx("min"),
						node->agghdr->fhdname, errbuf);
					(VOID) IIseterr(olderr);
					return(FAIL);
				}
			}
			if (avgagg)
			{
				avg_excb->excb_bases[ADE_ZBASE+1] =dbv->db_data;
				if (ade_execute_cx(adfcb, avg_excb) != OK)
				{
					/*
					**  Aggregate calculation failure.
					**  Get adf error by calling
					**  afe_errtostr().  Ignoring return
					**  value since am passing a buffer
					**  of ER_MAX_LEN long, which is the
					**  max error string that can be in
					**  a msg file.
					*/
					olderr = IIseterr(NULL);
					(VOID) afe_errtostr(adfcb, errbuf,
						&errlen);
					IIFDerror(E_FI20D5_8405, 3,
						ERx("avg"),
						node->agghdr->fhdname, errbuf);
					(VOID) IIseterr(olderr);
					return(FAIL);
				}
			}
			if (sumagg)
			{
				sum_excb->excb_bases[ADE_ZBASE+1] =dbv->db_data;
				if (ade_execute_cx(adfcb, sum_excb) != OK)
				{
					/*
					**  Aggregate calculation failure.
					**  Get adf error by calling
					**  afe_errtostr().  Ignoring return
					**  value since am passing a buffer
					**  of ER_MAX_LEN long, which is the
					**  max error string that can be in
					**  a msg file.
					*/
					olderr = IIseterr(NULL);
					(VOID) afe_errtostr(adfcb, errbuf,
						&errlen);
					IIFDerror(E_FI20D5_8405, 3, ERx("sum"),
						node->agghdr->fhdname, errbuf);
					(VOID) IIseterr(olderr);
					return(FAIL);
				}
			}
		}
		if (cntagg)
		{
			cnt_excb->excb_seg = ADE_SFINIT;
			cnt_excb->excb_nbases = ADE_ZBASE + 2;
			if (ade_execute_cx(adfcb, cnt_excb) != OK)
			{
				/*
				**  Aggregate result failure.
				**  Get adf error by calling
				**  afe_errtostr().  Ignoring return
				**  value since am passing a buffer
				**  of ER_MAX_LEN long, which is the
				**  max error string that can be in
				**  a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D6_8406, 3, ERx("count"),
					node->agghdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if (maxagg)
		{
			max_excb->excb_seg = ADE_SFINIT;
			max_excb->excb_nbases = ADE_ZBASE + 2;
			if (ade_execute_cx(adfcb, max_excb) != OK)
			{
				/*
				**  Aggregate result failure.
				**  Get adf error by calling
				**  afe_errtostr().  Ignoring return
				**  value since am passing a buffer
				**  of ER_MAX_LEN long, which is the
				**  max error string that can be in
				**  a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D6_8406, 3, ERx("max"),
					node->agghdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if (minagg)
		{
			min_excb->excb_seg = ADE_SFINIT;
			min_excb->excb_nbases = ADE_ZBASE + 2;
			if (ade_execute_cx(adfcb, min_excb) != OK)
			{
				/*
				**  Aggregate result failure.
				**  Get adf error by calling
				**  afe_errtostr().  Ignoring return
				**  value since am passing a buffer
				**  of ER_MAX_LEN long, which is the
				**  max error string that can be in
				**  a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D6_8406, 3, ERx("min"),
					node->agghdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if (avgagg)
		{
			avg_excb->excb_seg = ADE_SFINIT;
			avg_excb->excb_nbases = ADE_ZBASE + 2;
			if (ade_execute_cx(adfcb, avg_excb) != OK)
			{
				/*
				**  Aggregate result failure.
				**  Get adf error by calling
				**  afe_errtostr().  Ignoring return
				**  value since am passing a buffer
				**  of ER_MAX_LEN long, which is the
				**  max error string that can be in
				**  a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D6_8406, 3, ERx("avg"),
					node->agghdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}
		if (sumagg)
		{
			sum_excb->excb_seg = ADE_SFINIT;
			sum_excb->excb_nbases = ADE_ZBASE + 2;
			if (ade_execute_cx(adfcb, sum_excb) != OK)
			{
				/*
				**  Aggregate result failure.
				**  Get adf error by calling
				**  afe_errtostr().  Ignoring return
				**  value since am passing a buffer
				**  of ER_MAX_LEN long, which is the
				**  max error string that can be in
				**  a msg file.
				*/
				olderr = IIseterr(NULL);
				(VOID) afe_errtostr(adfcb, errbuf, &errlen);
				IIFDerror(E_FI20D6_8406, 3, ERx("sum"),
					node->agghdr->fhdname, errbuf);
				(VOID) IIseterr(olderr);
				return(FAIL);
			}
		}

		goodvals = TRUE;

		agginfo->aggflags |= AGG_ISVALID;

		break;

	    default:
		break;
	}

	if (goodvals)
	{
		switch(node->nodetype)
		{
		    case MAX_NODE:
			MEcopy(maxagg->aggdbv.db_data, maxagg->aggdbv.db_length,
				node->result->db_data);
			break;
		    case MIN_NODE:
			MEcopy(minagg->aggdbv.db_data, minagg->aggdbv.db_length,
				node->result->db_data);
			break;
		    case AVG_NODE:
			MEcopy(avgagg->aggdbv.db_data, avgagg->aggdbv.db_length,
				node->result->db_data);
			break;
		    case SUM_NODE:
			MEcopy(sumagg->aggdbv.db_data, sumagg->aggdbv.db_length,
				node->result->db_data);
			break;
		    case CNT_NODE:
			MEcopy(cntagg->aggdbv.db_data, cntagg->aggdbv.db_length,
				node->result->db_data);
			break;
		}
	}

	if (node->left)
	{
		if (IIFVeaEvalAgg(frm, node->left) != OK)
		{
			/*
			**  Recursive call will put out error message.
			**  Just return FAIL.
			*/
			return(FAIL);
		}
	}
	if (node->right)
	{
		if (IIFVeaEvalAgg(frm, node->right) != OK)
		{
			/*
			**  Recursive call will put out error message.
			**  Just return FAIL.
			*/
			return(FAIL);
		}
	}

	return(OK);
}




/*{
** Name:	IIFVfdeFastDrvEval - Fast derivation evaluation.
**
** Description:
**	Routine to evaluation a derivation formula using compiled
**	expressions.  Routine first obtains any need table field
**	column values and aggregate values before actually doing
**	the evaluation.
**
** Inputs:
**	frm		Form containing derivation to be evaluated.
**	node		Root DNODE of the derivation tree.
**	hdr		FLDHDR structure of field to be evaluated.
**
** Outputs:
**	dbv		Pointer to a DBV pointer to return evaluation
**			result in.  Not filled in if return is FAIL.
**	Returns:
**		OK	Evaluation was successful.
**		FAIL	Evaluation failed.
**	Exceptions:
**		<exception codes>
**
** Side Effects:
**	If aggregates are part of the derivation, table field display
**	and internal values are synchronized and cross column
**	evaluation may also be performed.
**
** History:
**	06/26/89 (dkh) - Initial version.
*/
STATUS
IIFVfdeFastDrvEval(frm, node, hdr, dbv)
FRAME		*frm;
DNODE		*node;
FLDHDR		*hdr;
DB_DATA_VALUE	**dbv;
{
	EX_CONTEXT	context;
	char		errbuf[ER_MAX_LEN + 1];
	i4		errlen = ER_MAX_LEN;

	if (node == NULL)
	{
		return(FAIL);
	}

	/*
	**  If there is no CX, then just execute the old (slow) way.
	*/
	if (node->drv_excb == NULL)
	{
		return(IIFVddeDoDeriveEval(frm, node, hdr, dbv));
	}

	if (EXdeclare(IIFVadfxhdlr, &context) != OK)
	{
		EXdelete();
		return(FAIL);
	}
	/*
	**  If this field depends on a tf column or an aggregate,
	**  then do those first before using CX.
	*/
	if (hdr->fhdrv->drvflags & fhDRVTFCDEP)
	{
		IIFVfcFindCol(frm, node);
	}
	if (hdr->fhdrv->drvflags & fhDRVAGGDEP)
	{
		/*
		**  Don't recalculate if triggered by application code.
		*/
		if (!IIFDevcb->eval_aggs)
		{
			EXdelete();
			return(FAIL);
		}

		/*
		**  No need to set DML in ADF_CB since the
		**  compiled expression should have set
		**  things to use SQL semantics.
		*/
		if (IIFVeaEvalAgg(frm, node) != OK)
		{
			/*
			**  No need to put out error message since
			**  IIFVeaEvalAgg() will do so.
			*/
			EXdelete();
			return(FAIL);
		}
	}
	if (ade_execute_cx(FEadfcb(), node->drv_excb) != OK)
	{
		/*
		**  Compiled calculation failure.
		**  Get adf error by calling
		**  afe_errtostr().  Ignoring return
		**  value since am passing a buffer
		**  of ER_MAX_LEN long, which is the
		**  max error string that can be in
		**  a msg file.
		*/
		olderr = IIseterr(NULL);
		(VOID) afe_errtostr(FEadfcb(), errbuf, &errlen);
		IIFDerror(E_FI20D7_8407, 2, hdr->fhdname, errbuf);
		(VOID) IIseterr(olderr);
		EXdelete();
		return(FAIL);
	}
	if (node->cresult)
	{
		/*
		**  No errors should happen here due to
		**  checks at parse time.
		*/
		(VOID) afe_cvinto(FEadfcb(), node->result, node->cresult);
		*dbv = node->cresult;
	}
	else
	{
		*dbv = node->result;
	}

	EXdelete();
	return(OK);
}




/*{
** Name:	IIFVddeDoDeriveEval - Slow derivation evaluation routine.
**
** Description:
**	This routine evaluates a derivation tree in a slow way by
**	traversing the tree to obtain values and do various calculations.
**
** Inputs:
**	frm		Form containing derivation to be evaluated.
**	node		Root DNODE of derivation tree.
**	hdr		FLDHDR structure for field being evaluated.
**
** Outputs:
**	dbv		Pointer to DBV pointer to pass back results in.
**			Not set if return is FAIL.
**
**	Returns:
**		OK	Evaluation was successful.
**		FAIL	Evaluation failed.
**	Exceptions:
**		None.
**
** Side Effects:
**	If aggregates are involved in the derivation, table field
**	display and internal values will be synchronized and
**	cross column evaluation may be done.
**
** History:
**	06/26/89 (dkh) - Initial version.
*/
STATUS
IIFVddeDoDeriveEval(frm, node, hdr, dbv)
FRAME		*frm;
DNODE		*node;
FLDHDR		*hdr;
DB_DATA_VALUE	**dbv;
{
	DB_DATA_VALUE	*ldbv;
	DB_DATA_VALUE	*rdbv;
	DB_DATA_VALUE	*usedbv;
	FLDVAL		*val;
	AFE_OPERS	ops;
	DB_DATA_VALUE	*opers[2];
	ADF_CB		*adfcb;
	STATUS		retval = OK;
	DRVAGG		*agginfo;
	DAGGVAL		*aggval;
	EX_CONTEXT	context;
	DB_LANG		odmllang;
	DNODE		*cnode;
	FLDTYPE		*ctype;
	FLDCOL		*ccol;
	DB_DATA_VALUE	*tdbv;
	char		errbuf[ER_MAX_LEN + 1];
	i4		errlen = ER_MAX_LEN;

	if (node == NULL)
	{
		/*
		**  Should put out error message at some point.
		**  Just return FAIL for now.
		*/
		return(FAIL);
	}

	if (EXdeclare(IIFVadfxhdlr, &context) != OK)
	{
		EXdelete();
		return(FAIL);
	};

	adfcb = FEadfcb();

	switch(node->nodetype)
	{
	    case ICONST_NODE:
	    case FCONST_NODE:
	    case SCONST_NODE:
	    case DCONST_NODE:
		if (node->cresult != NULL)
		{
			/*
			**  No errors should happen here due
			**  checks at parse time.
			*/
			(VOID) afe_cvinto(adfcb, node->result, node->cresult);
			*dbv = node->cresult;
		}
		else
		{
			*dbv = node->result;
		}
		break;

	    case PLUS_NODE:
	    case MINUS_NODE:
	    case MULT_NODE:
	    case DIV_NODE:
	    case EXP_NODE:
		/*
		**  Simple math nodes can be handled in the same
		**  manner since all that is important is the "fdesc"
		**  that was set up before.  Once we get the
		**  left and right dbvs, we can call afe_clinstd()
		**  to do calculation.  Result is stored in "result"
		**  member.
		*/
		if ((retval = IIFVddeDoDeriveEval(frm, node->left,
			hdr, &ldbv)) != OK)
		{
			/*
			**  Lower level call will put out error message.
			*/
			break;
		}
		if ((retval = IIFVddeDoDeriveEval(frm, node->right,
			hdr, &rdbv)) != OK)
		{
			/*
			**  Lower level call will put out error message.
			*/
			break;
		}
		ops.afe_opcnt = 2;
		ops.afe_ops = opers;
		opers[0] = ldbv;
		opers[1] = rdbv;
		if ((retval = afe_clinstd(adfcb, node->fdesc->adi_finstid,
			&ops, node->result)) != OK)
		{
			/*
			**  Calculation failure.
			**  Get adf error by calling
			**  afe_errtostr().  Ignoring return
			**  value since am passing a buffer
			**  of ER_MAX_LEN long, which is the
			**  max error string that can be in
			**  a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20D7_8407, 2, hdr->fhdname, errbuf);
			(VOID) IIseterr(olderr);
			break;
		}

		if (node->cresult != NULL)
		{
			/*
			**  No need to check for return value
			**  here due to parse time checks.
			*/
			(VOID) afe_cvinto(adfcb, node->result, node->cresult);
			*dbv = node->cresult;
		}
		else
		{
			*dbv = node->result;
		}
		break;

	    case UMINUS_NODE:
		/*
		**  Just get left node and run afe_clinstd().
		*/
		if ((retval = IIFVddeDoDeriveEval(frm, node->left,
			hdr, &ldbv)) != OK)
		{
			/*
			**  Lower level calls will put out error message.
			*/
			break;
		}
		opers[0] = ldbv;
		ops.afe_opcnt = 1;
		ops.afe_ops = opers;
		if ((retval = afe_clinstd(adfcb, node->fdesc->adi_finstid,
			&ops, node->result)) != OK)
		{
			/*
			**  Calculation failure.
			**  Get adf error by calling
			**  afe_errtostr().  Ignoring return
			**  value since am passing a buffer
			**  of ER_MAX_LEN long, which is the
			**  max error string that can be in
			**  a msg file.
			*/
			olderr = IIseterr(NULL);
			(VOID) afe_errtostr(adfcb, errbuf, &errlen);
			IIFDerror(E_FI20D7_8407, 2, hdr->fhdname, errbuf);
			(VOID) IIseterr(olderr);
			break;
		}

		if (node->cresult != NULL)
		{
			/*
			**  No need to check return value due
			**  to parse time checks.
			*/
			(VOID) afe_cvinto(adfcb, node->result, node->cresult);
			*dbv = node->cresult;
		}
		else
		{
			*dbv = node->result;
		}
		break;

	    case MAX_NODE:
	    case MIN_NODE:
	    case AVG_NODE:
	    case SUM_NODE:
	    case CNT_NODE:
		/*
		**  Don't evaluate an aggregate if this evaluation
		**  is triggered by application code.  This will
		**  hopefully reduce the overhead for rescanning
		**  the dataset to recalculate the aggregate.
		*/
		if (!IIFDevcb->eval_aggs)
		{
			retval = FAIL;
			break;
		}
		if (slow)
		{
			agginfo = node->agghdr->fhdrv->agginfo;
			switch(node->nodetype)
			{
			    case MAX_NODE:
				aggval = agginfo->maxagg;
				break;

			    case MIN_NODE:
				aggval = agginfo->minagg;
				break;

			    case AVG_NODE:
				aggval = agginfo->avgagg;
				break;

			    case SUM_NODE:
				aggval = agginfo->sumagg;
				break;

			    case CNT_NODE:
				aggval = agginfo->cntagg;
				break;
			}

			/*
			**  If value is currently valid, then no need
			**  to do anything.
			*/
			if (!(agginfo->aggflags & AGG_ISVALID))
			{
				/*
				**  If invalid, then we must rescan dataset.
				**  A call back routine pointer will used
				**  for this.  Set up DML in ADF_CB to
				**  be SQL to get proper semantics.  Reset
				**  to original on completion of call.
				*/
				odmllang = IIAFdsDmlSet(DB_SQL);
				IIFVcaCalcAgg(frm, node, hdr, agginfo);
				(VOID) IIAFdsDmlSet(odmllang);

				agginfo->aggflags |= AGG_ISVALID;
			}

			/*
			**  Move agg value to "result".
			**  Assuming conversion works with no errors
			**  due to parse time checks..
			*/
			/*
			**  Just copy into result since they are the same DBV.
			*/
			MEcopy(aggval->aggdbv.db_data, node->result->db_length,
				node->result->db_data);
		}
		else
		{
			/*
			**  No need to set DML in ADF_CB since
			**  compiled expressions already use
			**  SQL semantics.
			*/
			if ((retval = IIFVeaEvalAgg(frm, node)) != OK)
			{
				/*
				**  Lower level calls will put out
				**  error message.
				*/
				break;
			}
		}

		if (node->cresult != NULL)
		{
			/*
			**  No need to check return value due
			**  to parse time checks.
			*/
			(VOID) afe_cvinto(adfcb, node->result, node->cresult);
			*dbv = node->cresult;
		}
		else
		{
			*dbv = node->result;
		}
		break;

	    case SFLD_NODE:
		/*
		**  Assuming source fields are valid.
		*/
		if (IIFVcvConstVal)
		{
			ctype = &(node->fld->fld_var.flregfld->fltype);
			cnode = (DNODE *)ctype->ftvalchk;
			if (cnode->cresult)
			{
				tdbv = cnode->cresult;
			}
			else
			{
				tdbv = cnode->result;
			}
			if (use1)
			{
				usedbv = IIFVdc1DbvConst;
				use1 = FALSE;
			}
			else
			{
				usedbv = IIFVdc2DbvConst;
				use1 = TRUE;
			}
			usedbv->db_datatype = ctype->ftdatatype;
			usedbv->db_length = ctype->ftlength;
			usedbv->db_prec = ctype->ftprec;
			(VOID) afe_cvinto(adfcb, tdbv, usedbv);
			*dbv = usedbv;
		}
		else
		{
			val = &(node->fld->fld_var.flregfld->flval);
			tdbv = val->fvdbv;
			if (node->cresult != NULL)
			{
				/*
				**  No need to check return value due
				**  to parse time checks.
				*/
				(VOID) afe_cvinto(adfcb, tdbv, node->cresult);
				*dbv = node->cresult;
			}
			else
			{
				*dbv = tdbv;
			}
		}
		break;

	    case TCOL_NODE:
		/*
		**  Get dbv for a cell via callback routine.
		*/
		if (IIFVcvConstVal)
		{
			ccol = node->fld->fld_var.fltblfld->tfflds[node->colno];
			ctype = &(ccol->fltype);
			cnode = (DNODE *)ctype->ftvalchk;
			if (cnode->cresult)
			{
				*dbv = cnode->cresult;
			}
			else
			{
				*dbv = cnode->result;
			}
			if (use1)
			{
				usedbv = IIFVdc1DbvConst;
				use1 = FALSE;
			}
			else
			{
				usedbv = IIFVdc2DbvConst;
				use1 = TRUE;
			}
			usedbv->db_datatype = ctype->ftdatatype;
			usedbv->db_length = ctype->ftlength;
			usedbv->db_prec = ctype->ftprec;
			(VOID) afe_cvinto(adfcb, *dbv, usedbv);
			*dbv = usedbv;
		}
		else
		{
			*dbv = (*IIFDrfcb->getTFdbv)(frm,node->fld,node->colno);
			if (node->cresult != NULL)
			{
				/*
				**  No need to check return value due
				**  to parse time checks.
				*/
				(VOID) afe_cvinto(adfcb, *dbv, node->cresult);
				*dbv = node->cresult;
			}
		}
		break;

	    default:
		/*
		**  Bad node found.  Put out error message and return
		**  FAIL.
		*/
		retval = FAIL;
	}

	EXdelete();
	return(retval);
}
