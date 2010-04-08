
/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	<ug.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<errw.h>


/**
** Name:	rplet.c		Contains routine to create a TCMD structure for
**				a .LET assignment command.
**
** Description:
**	This file defines:
**
**	r_p_let		Creates a TCMD structure for a .LET assignment command.
**
** History:
**	17-jun-87 (grant)	created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

/* # define's */

# define	WANT_VAR	1	/* expecting variable name */
# define	WANT_COLON	2	/* expecting := or = */
# define	WANT_EQUAL	3	/* expecting only = */
# define	WANT_EXPR	4	/* expecting expression */

/* GLOBALDEF's */
/* extern's */
/* static's */

/*{
** Name:	r_p_let		creates a TCMD structure for a .LET assignment
**				command.
**
** Description:
**	This routine creates a TCMD structure for a .LET assignment command.
**	The format of a .LET assignment statement is:
**		.LET variable_name [:]= expression
**
** Inputs:
**	none.
**
** Outputs:
**	none.
**
** Side Effects:
**	Cact_tcmd will point to new .LET TCMD created by this routine.
**
** History:
**	17-jun-87 (grant)	written.
**		08-oct-92 (rdrane)
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Allow for delimited
**			identifiers and normalize them before lookup.
**		23-oct-1992 (rdrane)
**			Ensure set/propagate precision in DB_DATA_VALUE
**			structures. Converted r_error() err_num values to hex
**			to facilitate lookup in errw.msg file.  Eliminate
**			references to r_syserr() and use IIUGerr() directly.
**		11-nov-1992 (rdrane)
**			Fix-up parameterization of r_g_ident().
*/

VOID
r_p_let()
{
    i4			tok_type;
    DB_DATA_VALUE	exp_type;
    i4			state = WANT_VAR;
    char		*name;
    ITEM		item;
    DB_DATA_VALUE	*dbdv;
    LET			*let;
    i4			status;
    DB_DATA_VALUE	*con;

    while ((tok_type = r_g_eskip()) != TK_ENDSTRING)
    {
	switch(state)
	{
	case WANT_VAR:
	    if ((tok_type == TK_ALPHA) ||
		((tok_type == TK_QUOTE) && (St_xns_given)))
	    {
		name = r_g_ident(FALSE);
		_VOID_ IIUGdlm_ChkdlmBEobject(name,name,FALSE);
		status = r_p_tparam(name, FALSE, &item, &exp_type);
	    }

	    if (((tok_type != TK_ALPHA) && (tok_type != TK_QUOTE)) ||
	    	((tok_type == TK_QUOTE) && (!St_xns_given)) ||
		(status == NO_EXP))
	    {
		r_error(0x3C, NONFATAL, Cact_tname, Cact_attribute,
			Cact_command, Cact_rtext, NULL);
		return;
	    }

	    dbdv = &(item.item_val.i_v_par->par_value);
	    state = WANT_COLON;
	    break;

	case WANT_COLON:
	    if (tok_type == TK_COLON)
	    {
		Tokchar++;
		state = WANT_EQUAL;
		break;
	    }
	    /* fall through */

	case WANT_EQUAL:
	    if (tok_type != TK_EQUALS)
	    {
		r_error(0x3C, NONFATAL, Cact_tname, Cact_attribute,
			Cact_command, Cact_rtext, NULL);
		return;
	    }

	    Tokchar++;
	    state = WANT_EXPR;
	    break;

	case WANT_EXPR:
	    status = r_g_expr(&item, &exp_type);
	    switch (status)
	    {
	    case NO_EXP:
		r_error(0x3E, NONFATAL, Cact_tname, Cact_attribute, Tokchar,
			Cact_command, Cact_rtext, NULL);
		return;

	    case BAD_EXP:
		return;

	    case GOOD_EXP:
		if (exp_type.db_datatype == DB_BOO_TYPE)
		{
		    IIUGerr(E_RW003D_r_p_let_No_boolean,UG_ERR_FATAL,0);
		}
		break;

	    case NULL_EXP:
		if (!AFE_NULLABLE_MACRO(dbdv->db_datatype))
		{
		    r_error(0x41, NONFATAL, Cact_tname, Cact_attribute,
			    Cact_command, Cact_rtext, NULL);
		    return;
		}

		item.item_type = I_CON;
		con = (DB_DATA_VALUE *) MEreqmem(0,sizeof(DB_DATA_VALUE),TRUE,
						(STATUS *) NULL);
		item.item_val.i_v_con = con;
		con->db_datatype = dbdv->db_datatype;
		con->db_length = dbdv->db_length;
		con->db_prec = dbdv->db_prec;
		con->db_data = (PTR) MEreqmem(0,con->db_length,TRUE,
					      (STATUS *) NULL);
		adc_getempty(Adf_scb, con);
		break;
	    }

	    Cact_tcmd->tcmd_code = P_LET;
	    let = (LET *) MEreqmem(0,sizeof(LET),TRUE,(STATUS *) NULL);
	    Cact_tcmd->tcmd_val.t_v_let = let;
	    let->let_left = dbdv;
	    MEcopy((PTR)&item, (u_i2)sizeof(ITEM), (PTR)&(let->let_right));

	    return;
	}
    }
}
