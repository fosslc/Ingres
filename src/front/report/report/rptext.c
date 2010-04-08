/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<ug.h>
# include	 <rtype.h>
# include	 <rglob.h>
# include	<cm.h>
# include	<errw.h>

/*
**   R_P_TEXT - set up the TCMD structures from an RTEXT print command.
**	The RTEXT print (RTEXT commands are by default print commands)
**	are of the form:
**		PRINT expression [(format)] {, expression [(format)]}
**
**	where expression is an expression optionally followed
**	by a format in parentheses.
**
**	If an expression is enclosed in parentheses and it comes right
**	after another expression, they must be separated by commas
**	so RW doesn't think that it is a format.
**
**	This routine will break up the RTEXT print command into its
**	component parts.  Each text string and parameter will be set
**	up into a separate TCMD structure.
**
**	Parameters:
**		rcode	code of the command, either C_TEXT or C_PRINTLN.
**
**	Returns:
**		none.
**
**	Side Effects:
**		none.
**
**	Trace Flags:
**		3.0, 3.5.
**
**	History:
**		3/31/81 (ps) - written.
**		12/5/83 (gac)	allow unlimited-length parameters.
**		11/5/85 (mgw) - Fix Bug 6518 - NULL out fmt so that r_x_string()
**				knows how to print non date/time program
**				constants (eg. w_name)
**		23-mar-1987 (yamamoto)
**			Modified for double byte characters.
**		21-jul-89 (cmr)
**			Fix for bug 6858 -- local variable 'fmt' must
**			get initialized inside the while loop otherwise
**			a component of a .print could get the previous
**			component's format (instead of null).
**		1/3/90 (elein) Added char string declaration to be used
**			for FRM_CDATE, FRM_CTIME, FRM_VN constants.  These
**			default format constants are "changed" sooner or
**			later via Tokchar by STtrmwhite.  This causes abends
**			for IBM.  Change is to reassign these constants to
**			variable storage.  
**		8/1/90 (elein) 30449
**			Use a set date format for current_date variable.
**		22-may-92 (rdrane) 37360
**			Extend the set date formats for current_date
**			established by fix for 30449 to include all supported
**			DBMS formats.
**		23-oct-1992 (rdrane)
**			Converted r_error() err_num values to hex to
**			facilitate lookup in errw.msg file.  Eliminate
**			references to r_syserr() and use IIUGerr() directly.
**		07-oct-1998 (rodjo04) Bug 93450
**			Added support for MULTINATIONAL4.
**		23-oct-1998 (rodjo04) Bug 93785
**			Added support for YMD, DMY, and DMY.
**	24-Jun-1999 (shero03)
**	    Added support for ISO4.
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
r_p_text(rcode)
ACTION	rcode;
{
	/* internal declarations */

	i4	type;			/* type of token next in line */
	bool	getstruct = FALSE;	/* true if new structure needed */
	PEL	*pel;			/* print element */
	DB_DATA_VALUE	exp_type;	/* type of expression */
	FMT	*fmt;
	char	*save_Tokchar;
	STATUS	status;
	DB_DATA_VALUE	dvtype;
	bool	valid;
	char	*frmcdate_ptr;
	char	frmcdate[sizeof(FRM_CDATE)];
	char	frmctime[sizeof(FRM_CTIME)];
	char	frmvn[sizeof(FRM_VN)];

	/* start of routine */


	/*
	** 5.0 used to have month capitalized. 6.0 no longer does this
	** so for compatibility continue to do this.  For 6.0, set the
	** format based upon the in-force DB_***_DFMT.  The default will
	** always be 6.0 style US format, FRM_CDATE6.
	*/
	if (St_compatible && Adf_scb->adf_dfmt == DB_US_DFMT)
	{
		STcopy( FRM_CDATE, frmcdate );
	}
	else
	{
 		switch(Adf_scb->adf_dfmt)
		{
		case DB_MLTI_DFMT:
			STcopy( FRM_CDATE_MLTI, frmcdate );
			break;
		case DB_ISO_DFMT:
			STcopy( FRM_CDATE_ISO, frmcdate );
			break;
		case DB_ISO4_DFMT:
			STcopy( FRM_CDATE_ISO4, frmcdate );
			break;
		case DB_FIN_DFMT:
			STcopy( FRM_CDATE_FIN, frmcdate );
			break;
		case DB_GERM_DFMT:
			STcopy( FRM_CDATE_GERM, frmcdate );
			break;
		case DB_MLT4_DFMT:
			STcopy( FRM_CDATE_MLT4, frmcdate );
			break;
		case DB_YMD_DFMT:
			STcopy( FRM_CDATE_YMD, frmcdate );
			break;
		case DB_MDY_DFMT:
			STcopy( FRM_CDATE_MDY, frmcdate );
			break;
		case DB_DMY_DFMT:
			STcopy( FRM_CDATE_DMY, frmcdate );
			break;
		default:
			STcopy( FRM_CDATE6, frmcdate );
			break;
		}
	}
	STcopy( FRM_CTIME, frmctime );
	STcopy( FRM_VN, frmvn );

	if (rcode == C_PRINTLN)
	{
		St_prline = TRUE;
	}

	while ((type = r_g_eskip()) != TK_ENDSTRING)
	{	/* next token start */

		fmt = NULL;
		/* check to see if a new TCMD struct is needed */

		if (getstruct)
		{
			/* if the TCMD structure just found contains an
			** error, get out */

			if (Cact_tcmd->tcmd_code == P_ERROR)
			{
				break;
			}

			Cact_tcmd = r_new_tcmd(Cact_tcmd);
		}

		getstruct = TRUE;


		if (type == TK_COMMA)
		{
			CMnext(Tokchar);
			getstruct = FALSE;
		}
		else
		{
			pel = (PEL *) MEreqmem(0,sizeof(PEL),TRUE,
						(STATUS *) NULL);
			status = r_g_expr(&pel->pel_item, &exp_type);
			switch (status)
			{
			case(NO_EXP):
				r_error(0x65, NONFATAL, Cact_tname,
					Cact_attribute, Tokchar,
					Cact_command, Cact_rtext, NULL);
				return;

			case(BAD_EXP):
				return;

			case(GOOD_EXP):
				if (exp_type.db_datatype == DB_BOO_TYPE)
				{
					IIUGerr(E_RW003E_r_p_tcmd_Cannot_print,
						UG_ERR_FATAL,0);
				}
				else
				{
					Cact_tcmd->tcmd_code = P_PRINT;
					Cact_tcmd->tcmd_val.t_v_pel = pel;
				}
				break;
			}

			if (r_g_eskip() == TK_OPAREN)
			{	/*  should be a format specified */
				CMnext(Tokchar);
				if ((pel->pel_fmt = r_g_format()) == NULL ||
				    r_g_eskip() != TK_CPAREN)
				{	/* bad format or no closing paren */
					r_error(0x69, NONFATAL,
						ERget(S_RW003F_expression),
						Cact_tname, Cact_attribute,
						Cact_command, Cact_rtext, NULL);
					return;
				}

				CMnext(Tokchar);	/* skip closing paren */

				if (pel->pel_item.item_type == I_PAR &&
				    !pel->pel_item.item_val.i_v_par->par_declared)
				{
				    status = fmt_ftot(Adf_scb, pel->pel_fmt,
						      &dvtype);
				    if (status != OK)
				    {
					FEafeerr(Adf_scb);
					return;
				    }

				    r_par_type(pel->pel_item.item_val.i_v_par->par_name,
					       &dvtype);
				}
				else if (exp_type.db_datatype != DB_NODT)
				{
				    status = fmt_isvalid(Adf_scb, pel->pel_fmt,
							 &exp_type, &valid);
				    if (status != OK)
				    {
					FEafeerr(Adf_scb);
					return;
				    }

				    if (!valid)
				    {
					r_error(0x6A, NONFATAL,
						ERget(S_RW003F_expression),
						Cact_tname, Cact_attribute,
						Cact_command, Cact_rtext, NULL);
					return;
				    }
				}
			}
			else
			{
				switch (pel->pel_item.item_type)
				{
				case(I_PC):
					save_Tokchar = Tokchar;
					switch (pel->pel_item.item_val.i_v_pc)
					{
					case(PC_DATE):
						r_g_set(frmcdate);
						fmt = r_g_format();
						break;

					case(PC_TIME):
						r_g_set(frmctime);
						fmt = r_g_format();
						break;
					}
					Tokchar = save_Tokchar;
					break;

				case(I_PV):
					save_Tokchar = Tokchar;
					r_g_set(frmvn);
					fmt = r_g_format();
					Tokchar = save_Tokchar;
					break;

				default:
					break;
				}
				pel->pel_fmt = fmt;
			}
		}
	}

	return;
}
