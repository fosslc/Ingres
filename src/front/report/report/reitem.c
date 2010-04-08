/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<ex.h>
# include	<fmt.h>
# include	<ug.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<errw.h>

/*
** NO_OPTIM=dgi_us5
*/

/*
**	R_E_ITEM -- Evaluate an item or arithmetic expression.
**
**
**	Parameters:
**		item	pointer to parse tree of expression.
**		result	pointer to value of item or arithmetic expression.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	History:
**		12/20/83 (gac)	written.
**		17-jun-86 (mgw) bug # 9593
**			Check to prevent dividing by zero
**		8/1/89 (elein) 7333
**			added parameter to defer evaluation of 
**			expressions when only need to find out width.
**		2/26/90 (elein) 8921
**			Missed a place to defer evaluation.  Do not
**			try to coerce anything (because the data is not there)
**			when we are only trying to evaluation the width of the
**			expression.  Exit after we've got the target type
**		06/27/92 (dkh) - Updated to match new afe_ficoerce() interface.
**		22-oct-1992 (rdrane)
**			Set/propagate precision to EXP/result DB_DATA_VALUE.
**			Eliminate references to r_syserr() and use IIUGerr()
**			directly.
**		11-dec-1992 (rdrane)
**			Correct oversight in propagating precision to
**			EXP/result DB_DATA_VALUE.  Converted r_runerr()
**			err_num values to hex to facilitate lookup in
**			errw.msg file.
**		23-Jan-1996 (allmi01)
**			Added NO_OPTIM=dgi_us5 to correct rw problems with .center $col2+3/2
**			getting E-AD2005 and E_RW1209 errors.
**		28-May-1996 (allmi01)
**			Added NO_OPTIM for dgi_us5 DG Intel to correct
**			strange behavior on rw sep suites.
**              18-Feb-1999 (hweho01)
**                      Changed EXCONTINUE to EXCONTINUES to avoid compile error
**                      of redefinition on AIX 4.3, it is used in <sys/context.h>. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

VOID
r_e_item(item, result,eval)
ITEM		*item;
DB_DATA_VALUE	*result;
bool		eval;
{
	EXP	*exp;
	i4	i;
	DB_DATA_VALUE	opval[MAX_OPANDS];
	AFE_OPERS	ops;
	DB_DATA_VALUE	*sops[MAX_OPANDS];
	STATUS	status;
	i4	funct_type;
	i4	arg1_type;
	i4	arg2_type;
	i4	ei_exhandler();
	EX_CONTEXT	context;

	if (EXdeclare(ei_exhandler, &context))
	{
		EXdelete();
		adc_getempty(Adf_scb, result);
		return;
	}

	switch(item->item_type)
	{
	case(I_EXP):
	    exp = item->item_val.i_v_exp;

	    ops.afe_ops = sops;
	    ops.afe_opcnt = 0;

	    for (i = 0; i < MAX_OPANDS; i++)
	    {
		if (exp->exp_operand[i].item_type != I_NONE)
		{
		    /* execute branch of the expression tree */
		    r_e_item(&exp->exp_operand[i], &opval[i], eval);

		    ops.afe_opcnt++;
		    sops[i] = &opval[i];
		}
	    }

	    if (exp->exp_fid == ADI_NOFI)
	    {
		/* first time operator/function to be executed */

		AFE_OPERS	coercops;
		DB_DATA_VALUE	*cops[MAX_OPANDS];
		DB_DATA_VALUE	cop[MAX_OPANDS];
		ADI_FI_DESC	fdesc;
		ADI_FI_ID	coercfid;
		i4		coerclen;
		i2		coercprec;
		EXP		*cf;
		DB_DATA_VALUE	coercval;
		AFE_OPERS	ecop;
		DB_DATA_VALUE	*ecops[1];
		DB_DATA_VALUE	tdbv;

		EXP		*r_mak_tree();

		coercops.afe_opcnt = ops.afe_opcnt;
		coercops.afe_ops = cops;
		for (i = 0; i < MAX_OPANDS; i++)
		{
		    cops[i] = &cop[i];
		}

		/* determine the operator id */

		status = afe_fdesc(Adf_scb, exp->exp_opid, &ops, &coercops,
				   result, &fdesc);
		if (status != OK)
		{
		    FEafeerr(Adf_scb);
		    r_runerr(0x209, FATAL);
		}
		/* 8921 */
	        if( eval == FALSE)
	        {
			EXdelete();
			return;
	        }

		exp->exp_fid = fdesc.adi_finstid;
		exp->exp_restype = result->db_datatype;
		exp->exp_reslen = result->db_length;
		exp->exp_resprec = result->db_prec;

		/*
		** coercops contains the datatype (and length) of each operand
		** to which the original operand must be coerced in order to
		** use this function instance id.
		*/

		for (i = 0; i < fdesc.adi_numargs; i++)
		{
		    if (sops[i]->db_datatype != cops[i]->db_datatype)
		    {
			/* determine coercion function id and its result
			** length for the operand */

			tdbv.db_datatype = cops[i]->db_datatype;
			tdbv.db_length = 0;
			tdbv.db_prec = 0;
			status = afe_ficoerce(Adf_scb, sops[i], &tdbv,
					      &coercfid);
			if (status != OK)
			{
			    FEafeerr(Adf_scb);
			    r_runerr(0x209, FATAL);
			}
			coerclen = tdbv.db_length;
			coercprec = tdbv.db_prec;

			/* put it into the expression tree */

			cf = r_mak_tree(OP_NONE, &(exp->exp_operand[i]));
			cf->exp_fid = coercfid;
			cf->exp_restype = cops[i]->db_datatype;
			cf->exp_reslen = coerclen;
			cf->exp_resprec = coercprec;

			/* execute the coercion function */

			ecop.afe_opcnt = 1;
			ecop.afe_ops = ecops;
			ecops[0] = &opval[i];

			coercval.db_datatype = cf->exp_restype;
			coercval.db_length = cf->exp_reslen;
			coercval.db_prec = cf->exp_resprec;
			coercval.db_data = (PTR) r_ges_next(cf->exp_reslen);

			status = afe_clinstd(Adf_scb, cf->exp_fid, &ecop,
					     &coercval);
			if (status != OK)
			{
			    FEafeerr(Adf_scb);
			    r_runerr(0x209, FATAL);
			}

			MEcopy((PTR)&coercval, (u_i2)sizeof(DB_DATA_VALUE),
			       (PTR)&opval[i]);
		    }
		}
	    }

	    /* execute operator/function */

	    result->db_datatype = exp->exp_restype;
	    result->db_length = exp->exp_reslen;
	    result->db_prec = exp->exp_resprec;
	    result->db_data = (PTR) r_ges_next(exp->exp_reslen);
	    /* B7333 -- don't evaluate expression when only
	      determining length */
	    if( eval == FALSE)
	    {
		EXdelete();
		return;
	    }

	    status = afe_clinstd(Adf_scb, exp->exp_fid, &ops, result);
	    if (status != OK)
	    {
		FEafeerr(Adf_scb);
		r_runerr(0x209, FATAL);
	    }

	    break;

	case(I_CON):
		MEcopy((PTR)(item->item_val.i_v_con),
		       (u_i2)sizeof(DB_DATA_VALUE), (PTR)result);
		break;

	case(I_PC):
		r_x_pc(item->item_val.i_v_pc, result);
		break;

	case(I_PV):
		result->db_datatype = DB_INT_TYPE;
		result->db_length = sizeof(i4);
		result->db_prec = 0;
		result->db_data = (PTR)(item->item_val.i_v_pv);
		break;

	case(I_ATT):
		r_x_att(item->item_val.i_v_att, result);
		break;

	case(I_PAR):
		r_e_par(item->item_val.i_v_par, result);
		break;

	case(I_ACC):
		r_a_acc(item->item_val.i_v_acc, result);
		break;

	case(I_CUM):
		r_a_cum(item->item_val.i_v_cum, result);
		break;

	case(I_NONE):
		IIUGerr(E_RW001E_r_e_item_No_item,UG_ERR_FATAL,0);

	default:
		IIUGerr(E_RW001F_r_e_item_Unknown_item,UG_ERR_FATAL,0);
	}

	EXdelete();
}

/*
/*
**	EI_EXHANDLER -- Handle an Expression
**
**
**	Parameters:
**		exargs	pointer to expression arguments.
**
**	Returns:
**		EX succeed/fail indication.
**
**	History:
**	 22-oct-1992 (rdrane)
**		Converted r_error() err_num values to hex to facilitate lookup
**		in errw.msg file.
*/

i4
ei_exhandler(exargs)
EX_ARGS *exargs;
{
	EX	num;

	num = exargs->exarg_num;

	if (num == EXFLTDIV || num == EXINTDIV || num == EXMNYDIV)
	{
		r_runerr(0x203, NONFATAL);
	}
	else if (num == EXFLTOVF || num == EXINTOVF)
	{
		r_runerr(0x204, NONFATAL);
	}
	else if (num == EXFLTUND)
	{
		r_runerr(0x205, NONFATAL);
	}
	else
	{
		adx_handler(Adf_scb, exargs);
		if (Adf_scb->adf_errcb.ad_errcode == E_AD0001_EX_IGN_CONT)
		{
			return EXCONTINUES;
		}
		else if (Adf_scb->adf_errcb.ad_errcode == E_AD0115_EX_WRN_CONT)
		{
			while (adx_chkwarn(Adf_scb) != E_DB_OK)
			{
				FEafeerr(Adf_scb);
			}
			return EXCONTINUES;
		}
		else if (Adf_scb->adf_errcb.ad_errcode == E_AD0116_EX_UNKNOWN)
		{
			return EXRESIGNAL;
		}
		else	/* an ADF error not handled by RW */
		{
			FEafeerr(Adf_scb);
		}
	}

	return EXDECLARE;
}
