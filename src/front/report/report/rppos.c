/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<me.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h> 
# include	<rglob.h> 
# include	<cm.h>

/*
**   R_P_POS - process a POSITION command, which can set
**	up some number of default column positions.  
**	The format of the command is:
**		POSITION colname {,colname} (position [,width]) 
**			{,colname {,colname} (position [,width]) }
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Called by:
**		r_act_set.
**
**	Side Effects:
**		Updates the Cact_tcmd, maybe.
**
**	Error Messages:
**		103, 130, 140, 141, 144, 184, 1026.
**
**	Trace Flags:
**		3.0, 3.6.
**
**	History:
**		7/16/81 (ps) - written.
**		12/5/83 (gac)	allow unlimited-length parameters.
**		8/14/89 (elein) garnet
**			Added variables evaluation for column name,
**			position and width
**		02-mar-90 (sylviap)
**			Added check if the position + width of a .POSITION
**			command is greater than En_lmax.  Flags error - 1026.
**			(jupbug #9629, US #157)
**		08-oct-92 (rdrane)
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Allow for delimited
**			identifiers and normalize them before lookup.
**		11-nov-1992 (rdrane)
**			Fix-up parameterization of r_g_ident().
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
r_p_pos()
{
	/* internal declarations */

	ATTRIB		ordinal = 0;		/* ordinal of found attribute */
	char		*aname;			/* name of found att */
	i4		type;			/* token type */
	TCMD		*ftcmd;			/* first TCMD structure in list
						** if more than one with same
						** position given */
	TCMD		*wtcmd;			/* first TCMD structure in list
						** if more than one with same
						** width given */
	bool		getstruct = FALSE;	/* TRUE when new TCMD needed */
	i4		position = 0;		/* position value found */
	i4		width = 0;		/* width value found */
	i4		error = 0;		/* set to -1 on error in number */
	char		lmax_str[20];		/* used to hold converted integer for En_lmax */
	char		*tmpstr;		/* used to hold column name */

	/* start of routine */


	ftcmd = Cact_tcmd;


	while ((type = r_g_eskip()) != TK_ENDSTRING)
	{
		switch (type)
		{
			case(TK_COMMA):
				CMnext(Tokchar);	/* ignore it */
				break;

			case(TK_ALPHA):
			case(TK_QUOTE):
			case(TK_DOLLAR):
				if  ((type == TK_QUOTE) && (!St_xns_given))
				{
					r_error(0x8C, NONFATAL, Tokchar, Cact_tname,
						    Cact_attribute, Cact_command, Cact_rtext, NULL);
					return;
				}
				if  (type == TK_DOLLAR)
				{
					CMnext(Tokchar);
					tmpstr = r_g_name();
					if ((aname = r_par_get(tmpstr)) == NULL)
					{
						r_error(0x3F0, NONFATAL, tmpstr, NULL);
						return;
					}
				}
				else
				{
					aname = r_g_ident(FALSE);
				}
				_VOID_ IIUGdlm_ChkdlmBEobject(aname,aname,FALSE);

				/* new TCMD structure needed ? */
				position = width = 0;
				if (getstruct == TRUE)
				{
					Cact_tcmd = r_new_tcmd(Cact_tcmd);
					if (ordinal == 0)
					{	/* first att for next position */
						ftcmd = Cact_tcmd;
					}
				}
				getstruct = TRUE;

				if ((ordinal = r_mtch_att(aname)) == A_ERROR)
				{	/* not an attribute */
					r_error(0x82, NONFATAL, aname, Cact_tname,
						Cact_attribute, Cact_command, Cact_rtext, NULL);
					return;
				}
				if (!r_grp_set(ordinal))
				{	/* FALSE if W_COLUMN speced out of .WI */
					r_error(0xB8, NONFATAL, Cact_tname, Cact_attribute,
						Cact_command, Cact_rtext, NULL);
					return;
				}
				Cact_tcmd->tcmd_val.t_v_ps = (PS *) MEreqmem(0,sizeof(PS),
									TRUE,(STATUS *) NULL);
				(Cact_tcmd->tcmd_val.t_v_ps)->ps_ordinal = ordinal;
				break;

			case(TK_OPAREN):
				/* make sure at least one att specified */

				if (ordinal <= 0)
				{
					r_error(0x8C, NONFATAL, Tokchar, Cact_tname, Cact_attribute,
						Cact_command, Cact_rtext, NULL);
					return;
				}

				/* position found for these attributes */

				CMnext(Tokchar);		/* skip open paren */

				/* get the value of position */

				switch(type = r_g_eskip())
				{
					case(TK_DOLLAR):
						CMnext(Tokchar);
						tmpstr = r_g_name();
						if( (aname = r_par_get(tmpstr)) == NULL)
						{
							r_error(0x3F0, NONFATAL, tmpstr, NULL);
							return;
						}
						if( CVan( aname, &position ) != OK )
						{
							error= -1;
						}
						break;
					case(TK_SIGN):
					case(TK_NUMBER):
						error = r_g_long(&position);
						break;

					default:
						error = -1;
						break;
				}

				if (error < 0)
				{	/* error in number */
					r_error(0x67, NONFATAL, Cact_tname, Cact_attribute,
						Cact_command, Cact_rtext, NULL);
					return;
				}

				if ((position<0) || (position>En_lmax))
				{
					/* bug 9382
					**	Send En_lmax instead of hard
					**	coding a value in the error file
					**	First convert En_lmax.
					*/

					CVna((i4)En_lmax, lmax_str);
 					r_error(0x8D, NONFATAL, Cact_tname,
 						Cact_attribute, lmax_str, 
 						Cact_command, Cact_rtext, NULL);
					position = 0;
				}

				if((type=r_g_eskip()) == TK_COMMA)
				{	/* should be followed by width */
					CMnext(Tokchar);
					switch(type = r_g_eskip())
					{
						case(TK_DOLLAR):
							CMnext(Tokchar);
							tmpstr = r_g_name();
							if( (aname = r_par_get(tmpstr)) == NULL)
							{
								r_error(0x3F0, NONFATAL, tmpstr, NULL);
								return;
							}
							if( CVan( aname, &width ) != OK )
							{
								error= -1;
							}
							break;
						case(TK_SIGN):
						case(TK_NUMBER):
							error = r_g_long(&width);
							break;

						default:
							error = -1;
							break;
					}
					if (error < 0)
					{	/* error in number */
						r_error(0x67, NONFATAL, Cact_tname, Cact_attribute,
							Cact_command, Cact_rtext, NULL);
						return;
					}
					if ((width<=0) || (width>En_lmax))
					{
						/* send maximum width after
						** converting it to a string.
						*/
						CVna((i4)En_lmax, lmax_str);
						r_error(0x90, NONFATAL, 
							Cact_tname,
 							Cact_attribute,
 							lmax_str,
 							Cact_command,
 							Cact_rtext, NULL);
						width = 1;
					}
					/* 
					** Also check if the sum > En_lmax
					** jupbug #9629, US #157.
					*/
					if ((position + width) >En_lmax)
					{
						/* send maximum width after
						** converting it to a string.
						*/
						CVna((i4)En_lmax, lmax_str);
						r_error(0x402, NONFATAL, 
							Cact_tname,
 							Cact_attribute,
 							lmax_str,
 							Cact_command,
 							Cact_rtext, NULL);
						position = 0;
						width = 1;
					}

					type = r_g_eskip();	/* get closing paren */
				}

				if (type != TK_CPAREN)
				{	/* no closing parenthesis */
					r_error(0x8C, NONFATAL, Tokchar, Cact_tname, Cact_attribute,
						Cact_command, Cact_rtext, NULL);
					return;
				}
				CMnext(Tokchar);		/* skip closing paren */

				/* now fill in the list of TCMD structures */

				wtcmd = NULL;
				if (width>0)
				{
					Cact_tcmd = r_new_tcmd(Cact_tcmd);
					wtcmd = Cact_tcmd;
					getstruct = FALSE;
				}

				for (; ftcmd!=wtcmd; ftcmd=ftcmd->tcmd_below)
				{
					(ftcmd->tcmd_val.t_v_ps)->ps_position = position;
					ftcmd->tcmd_code = P_POSITION;
					if (width>0)
					{	/* put in a width  TCMD */
						if (getstruct)
						{
							Cact_tcmd = r_new_tcmd(Cact_tcmd);
						}
						getstruct = TRUE;
						Cact_tcmd->tcmd_val.t_v_ws = (WS *) 
									MEreqmem(0,sizeof(WS),TRUE,
										(STATUS *) NULL);
						(Cact_tcmd->tcmd_val.t_v_ws)->ws_ordinal = 
							(ftcmd->tcmd_val.t_v_ps)->ps_ordinal;
						(Cact_tcmd->tcmd_val.t_v_ws)->ws_width = width;
						Cact_tcmd->tcmd_code = P_WIDTH;
					}	
				}
				ordinal = 0;		/* flag for new list */
				break;

			default:
				r_error(0x8C, NONFATAL, Tokchar, Cact_tname, Cact_attribute, Cact_command, Cact_rtext, NULL);
				return;
		}

	}

	return;
}

