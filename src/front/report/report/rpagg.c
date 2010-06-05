/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<me.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<afe.h>
# include	<fmt.h>
# include	 <rtype.h> 
# include	 <rglob.h> 
# include	<st.h>
# include	<cm.h>
# include	<ug.h>
# include	<cv.h>
# include	<er.h>
# include	<cv.h>

/*
**   R_P_AGG - set up the TCMD and ACC structures for an aggregate, after
**	checking that it is legal.  A legal aggregate must be specified
**	in footer text (except cumulatives), must refer to an attribute lower
**	in the sort order, and must refer to a numeric or date item (except
**	for COUNT).  Legal aggregates are SUM(U), COUNT(U), MIN, MAX and AVG(U).
**	Any of these
**	can be preceded by CUM, which means not to reset it, and give
**	a running value.  When a legal aggregate is determined, a
**	TCMD structure is set up to point to an accumulator in an ACC
**	structure, which is placed in a linked list for the attribute
**	being accumulated (minor to major).  These are started in the
**	ATT ACC structures.  After all text has been processed for all
**	breaks, the ATT ACC structures are again read, and pointers to ACC
**	structures are set up to directly reference
**	accumulators. 
**
**	The full format of an aggregate specification is:
**		[cum [(brk)]] aggname(attname [,preset])
**	Alternatively, "cum" can be called "cumulative".
**
**	Parameters:
**		name - ptr to string containing the name of the aggregate
**			to check for.
**		item	pointer to item returned by this procedure.
**		type	datatype of aggregate.
**
**	Returns:
**		status:
**			GOOD_EXP if legal aggregate found.
**			BAD_EXP if error found.
**			NO_EXP if no aggregate found.
**
**	Called by:
**		r_p_param.
**
**	Side Effects:
**		Updates Tokchar.
**
**	Error Messages:
**		108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 
**		118, 119, 123, 184, 185.
**
**	Trace Flags:
**		3.0, 3.10.
**
**	History:
**
**		5/8/81 (ps)	written.
**		5/5/82 (ps)	change format of command and add unique aggs.
**		12/5/83 (gac)	allow unlimited-length parameters.
**		12/20/83 (gac)	added item for expressions.
**		2/8/84	(gac)	added aggregates on dates.
**		5/12/85	(ac)	bug # 5586 fixes. The condition checking for
**			  	error # 118 was wrong. Should include the 
** 				conditions of T_STRING and T_DATE.
**		2/13/86 (mgw)	Fixed bug 7966 - added argument to r_g_string
**		6-aug-1986 (Joe)
**			Added support for TK_SQUOTE.  Passed string
**			delimiter to r_g_string.  Also took out strip slash
**			argument to r_g_string.
**		23-mar-1987 (yamamoto)
**			Modified for dobule byte characters.
**		17-dec-1987 (rdesmond)
**			changed "cnt" to "count" for adf.
**		02-dec-88 (sylviap)
**			Added error message if try to use aggregate functions
**			'sum' or 'avg' on a date datatype.   In 6.1, these
**			functions are not supported.  Once they are, this check
**			can be deleted.
**		16-dec-88 (sylviap)
**			Changed memory allocation to use FEreqmem.
**		3/17/89 (martym)
**			Fixed so that acc data structure are created correctly 
**			for all aggregates. Bug #4684.
**		3/23/89 (martym)
**			Added check to make sure that a preset and its attribute
**			are coerceable, so that they can be coerced at report-
**			run-time.
**		1/19/90 (elein) B6979
**			Set preset dbv to attribute dbv for presets which
**			are attributes.  Otherwise when we see if we can
**			coerce the types, it will say no becuase we have
**			not specified the preset type via dbv correctly.
**              3/20/90 (elein) performance
**                      Change STcompare to STequal
**            	12-oct-90 (sylviap)
**			Took out check for avg/sums on dates.  INGRES dbms now
**			supports aggregates.  Note: Only works for date 
**			intervals, not absolute dates. #33777
**		08-oct-92 (rdrane)
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Allow for delimited
**			identifiers and normalize them before lookup.  Remove
**			function declrations since they're now in the hdr
**			files.  Force CVlower() on attname before comparing
**			against NAM_REPORT or NAM_PAGE since those constants
**			are lower cased strings.  The comments regarding the
**			local variable "temp" would indicate that this was an
**			oversight when IIUGlbo_lowerBEobject() was introduced.
**			Customized IIUGbmaBadMemoryAllocation() text to
**			disambiguate which may have occured.
**		11-nov-1992 (rdrane)
**			Fix-up parameterization of r_g_ident().  Correct
**			setting of tok_type when getting the aggregate attname.
**			Collapse tok_type and tokvalue usages.
**		9-dec-1992 (rdrane)
**			Fix-up parameterization of r_g_double() to support
**			interpretation of DECIMAL numeric constants, as well as
**			setting of the DB_DATA_VALUE.
**      08-30-93 (huffman)
**              add <me.h>
**		17-oct-97 (kitch01)
**			Bug 86273. After checking the preset value is a valid number
**			ensure we set preset_type to the relevent datatype. Previously
**			preset_type remained uninitialised causing the test for COUNT
**			preset values being integer to fail => E_RW1080.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	05-oct-2006 (gupsh01)
**	    Change date to ingresdate.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/



i4
r_p_agg(name, item, aggrestype)
char		*name;
ITEM		*item;
DB_DATA_VALUE	*aggrestype;
{

	/* internal declarations */

	char		*nname;			/* aggregate name */
	char		*temp;			/*
						** (mmm) used to break up into
						** steps a call to CVlower
						*/
	register ATT	*attribute;		/* ptr to ATT being agg'ed */
	char		*attname;		/* name of attribute to agg */
	ATTRIB		aordinal;		/* ordinal specified in agg */
	ATTRIB		ordinal;		/* ordinal of attribute to agg */
	BRK		*brk = NULL;		/*
						** break structure of
						** aggregating break
						*/
	ACC		*acc;			/* temp use of ACC structure */
	bool		cumfound = FALSE;	/* true if cumulative found */
	PRS		*preset = NULL;		/*
						** PRS structure if preset
						** specified
						*/
	i4		error = 0;		/* set to -1 if error found on
						** preset operations */
	ATT		*patt=NULL;		/*
						** ATT struct ordinal of
						** preset
						*/
	PTR		t_ptr;
	i2		preset_type;
	i4		tok_type;
	ADI_OP_ID	op_code;
	ADI_FI_ID	fid;
	i4		wrkspc_size;
	ADI_OP_ID	opid;
	bool		isunique;
	DB_DATA_VALUE	pdbv;
	STATUS		status;
	i4		fitype;
	bool		can_coerce;

	/* start of routine */



	/* preset break structure to current break.  This may be
	** changed by cumulative. */

	brk = Cact_break;
	nname = name;			/* store agg name to the side */

	/* first see if it is a cumulative specification */

	if (r_gt_cagg(name))
	{	/* cumulative found */
		cumfound = TRUE;

		/* see if aggregator break specified */

		if (r_g_eskip() == TK_OPAREN)
		{
			CMnext(Tokchar);		/* skip open paren */
			tok_type =  r_g_eskip();
			if ((tok_type == TK_ALPHA) ||
			    ((tok_type == TK_QUOTE) && (St_xns_given)))
			{
				attname = r_g_ident(FALSE);
				_VOID_ IIUGdlm_ChkdlmBEobject(attname,attname,
							     FALSE);
				ordinal=r_mtch_att(attname);
				if (ordinal==A_GROUP)
				{	/* CUM (W_COLUMN) found illegal */
					r_error(0xB9, NONFATAL, Cact_tname, 
						Cact_attribute, Cact_command,
						Cact_rtext, NULL);
					return(BAD_EXP);
				}

				if (ordinal > 0)
				{	/* found an attribute */
					for(brk=Ptr_brk_top; brk!=NULL;
					    brk=brk->brk_below)
					{
						if (ordinal ==
							 brk->brk_attribute)
						{
							break;
						}
					}

					if(brk == NULL)
					{	/* not a break attribute */
						r_error(0x72, NONFATAL, attname,
							Cact_tname,
							Cact_attribute,
							Cact_command,
							Cact_rtext,NULL);
						return(BAD_EXP);
					}
				}	/* found an attribute */
				else
				{	/* see if it is "page" or "report" */
					CVlower(attname);
					if (STequal(attname,NAM_REPORT))
					{
						brk = Ptr_brk_top;
					}
					else if (STequal(attname,NAM_PAGE))
					{
						brk = Ptr_pag_brk;
					}
					else
					{	/* nothing found */
						r_error(0x73, NONFATAL,
							attname, 
							Cact_tname,
							Cact_attribute,
							Cact_command,
							Cact_rtext,
							NULL);
						return(BAD_EXP);
					}
				}
			}
			else
			{	/* something bad found after ( */
				r_error(0x74, NONFATAL, Cact_tname,
					Cact_attribute, 
					Cact_command,Cact_rtext,NULL);
				return(BAD_EXP);
			}

			/* should be closing parenthesis */

			if (r_g_eskip() == TK_CPAREN)
			{
				CMnext(Tokchar);		/* skip it */
			}
			else
			{
				/* closing parenthesis expected */
				r_error(0x71, NONFATAL, Cact_tname,
					Cact_attribute, Cact_command,
					Cact_rtext,NULL);
				return BAD_EXP;
			}
		}	/* found an opening parenthesis */
		else
		{	/* default is cumulative for the report */
			brk = Ptr_brk_top;
		}

		/* now get the attribute for the cumulative */

		if (r_g_eskip() != TK_ALPHA)
		{
			r_error(0x75, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}

		nname = r_g_name();
		_VOID_ IIUGdbo_dlmBEobject(nname,FALSE);

	}	/* cumulative found */

	status = afe_fitype(Adf_scb, nname, &fitype);
	if (status != OK)
	{
		if (Adf_scb->adf_errcb.ad_errcode == E_AD2001_BAD_OPNAME)
		{
			fitype = ~ADI_AGG_FUNC;	/* not an aggregate function */
		}
		else
		{
			FEafeerr(Adf_scb);
			r_error(0x81, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return BAD_EXP;
		}
	}

	if (fitype != ADI_AGG_FUNC)
	{
		/* name is not an aggregate */

		if (cumfound)
		{
			/* bad primitive with cumulative */

			r_error(0x75, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}
		else
		{
			return (NO_EXP);	/* no match found */
		}
	}


	isunique = (nname[STlength(nname)-1] == 'u');

	/* find the attribute and format (if specified) */

	if (r_g_eskip() != TK_OPAREN)
	{
		r_error(0x6C, NONFATAL, nname, Cact_tname, Cact_attribute, 
			Cact_command,Cact_rtext,NULL);
		return(BAD_EXP);
	}

	CMnext(Tokchar);		/* skip the open paren */

	attname = ERx("");
	tok_type = r_g_eskip();
	if ((tok_type == TK_ALPHA) ||
	    ((tok_type == TK_QUOTE) && (St_xns_given)))
	{
		attname = r_g_ident(FALSE);
		_VOID_ IIUGdlm_ChkdlmBEobject(attname,attname,FALSE);
	}
	else
	{	/* bad attribute name found */
		r_error(0x6D, NONFATAL, attname, nname, Cact_tname, 
			Cact_attribute,Cact_command,Cact_rtext,NULL);
		return(BAD_EXP);
	}
	if ((aordinal=r_mtch_att(attname))==A_ERROR)
	{	/* bad attribute name found */
		r_error(0x6D, NONFATAL, attname, nname, Cact_tname, 
			Cact_attribute,Cact_command,Cact_rtext,NULL);
		return(BAD_EXP);
	}

	/* now check to see if the Preset was specified */

	if (r_g_eskip() == TK_COMMA)
	{
		CMnext(Tokchar);		/* skip it */
	}

	switch(tok_type = r_g_eskip())
	{
		case(TK_CPAREN):
			CMnext(Tokchar);		/* not specified */
			break;			/* get out */

		case(TK_SIGN):
		case(TK_NUMBER):
			error = r_g_double(&pdbv);
			if (r_g_eskip() != TK_CPAREN)
			{
				error = -1;
				break;
			}
			CMnext(Tokchar);	/* skip closing paren */
			if (error == 0)
			{   /* no error */
			    if ((t_ptr = (PTR)FEreqmem((u_i4)Rst4_tag,
					 	(u_i4)pdbv.db_length,
					 	TRUE,(STATUS *)NULL))
						 == (PTR)NULL)
      			    {
		      		IIUGbmaBadMemoryAllocation(
					    ERx("r_p_agg - preset realloc"));
			    }
			    MEcopy(pdbv.db_data,pdbv.db_length,t_ptr);
			    _VOID_ MEfree(pdbv.db_data);
			    pdbv.db_data = t_ptr;
			    preset = r_prs_get(PT_CONSTANT,(PTR)&pdbv);
				/* Bug 86273. Make sure preset_type is initialised */
				preset_type = pdbv.db_datatype;
			}
			break;

		case(TK_ALPHA):
		case(TK_QUOTE):
			if ((tok_type == TK_ALPHA) ||
			    ((tok_type == TK_QUOTE) && (St_xns_given)))
			{
				/* should be an attribute name */
				temp = r_g_ident(FALSE);
				_VOID_ IIUGdlm_ChkdlmBEobject(temp,temp,FALSE);
				error = r_mtch_att(temp);

				if (r_g_eskip() != TK_CPAREN)
				{
					error = -1;
					break;
				}
				CMnext(Tokchar);	/* skip closing paren */

				if (error == A_GROUP)
				{
					r_error(0xB9, NONFATAL, Cact_tname,
						Cact_attribute, 
						Cact_command,Cact_rtext,NULL);
					return(BAD_EXP);
				}
	
				if (error > 0)
				{
					patt = r_gt_att(error);
					if (patt == NULL)
					{
						error = -1;
						break;
					}

					/* get the PRS structure for this att */
					preset = r_prs_get(PT_ATTRIBUTE,
							   (PTR)&error);
					preset_type =
						 patt->att_value.db_datatype;
					pdbv = patt->att_value;
				}
				break;
			}
			/*
			** Yes, it's kludgy but we need to fall through to the
			** single quote (string) processing if we had a double
			** quote and 6.5 expanded namespace is NOT enabled.
			*/
		case(TK_SQUOTE):
			/*
			** We may fall into here from TK_QUOTE case!
			*/
			temp = r_g_string(tok_type);

			if (r_g_eskip() != TK_CPAREN)
			{
				error = -1;
				break;
			}

			CMnext(Tokchar);		/* skip closing paren */

			/* convert string into date */

			status = r_convdbv(temp, ERx("ingresdate"), &pdbv);
			if (status != OK)
			{
				FEafeerr(Adf_scb);
				error = -1;
				break;
			}

			if (error >= 0)
			{
				preset = r_prs_get(PT_CONSTANT, (PTR)&pdbv);
				preset_type = DB_DTE_TYPE;
			}
			break;

		default:
			error = -1;
			break;

	}

	/* if an error occurred, report and get out */

	if (error < 0)
	{
		r_error(0x76, NONFATAL, nname, Cact_tname, Cact_attribute,
			Cact_command, Cact_rtext, NULL);
		return(BAD_EXP);
	}

	if (preset != NULL)
	{
		afe_opid(Adf_scb, nname, &op_code);

		afe_opid(Adf_scb, ERx("avg"), &opid);
		if (op_code == opid)
		{
			/* average aggregate cannot have preset */

			r_error(0x77, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}

		afe_opid(Adf_scb, ERx("count"), &opid);
		if (op_code == opid && (abs(preset_type)) != DB_INT_TYPE)
		{
			/* count aggregate must have integer preset */

			r_error(0x80, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}
	}


	/* aggregate must be cumulative in header and detail */

	if (!cumfound && !STequal(Cact_type, B_FOOTER) )
	{
		r_error(0x6E, NONFATAL, nname, Cact_tname, Cact_attribute,
			Cact_command, Cact_rtext, NULL);
		return(BAD_EXP);
	}

	/* Now go through the possible list of .WITHIN columns, because
	** all must be legal.  Note if a simple column name was given
	** here, then the r_grp_set and r_grp_next routines will return
	** the ordinal of that column  */

	if (!r_grp_set(aordinal))
	{
		r_error(0xB8, NONFATAL, Cact_tname, Cact_attribute, 
			Cact_command,Cact_rtext,NULL);
		return(BAD_EXP);
	}

	while ((ordinal=r_grp_nxt()) > 0)
	{
		attribute = r_gt_att(ordinal);
		attname = attribute->att_name;


		/* make sure the aggregate is legal in these circumstances 
		** subordinate attribute to current break */

		if ((brk->brk_attribute == A_DETAIL) ||
		    ((attribute->att_brk_ordinal > 0) &&
		     (brk->brk_attribute > 0) &&
		     (r_fnd_sort(brk->brk_attribute) >
			attribute->att_brk_ordinal)))
		{
			r_error(0x6F, NONFATAL, nname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}

		status = afe_agfind(Adf_scb, nname, &(attribute->att_value),
				    &fid, aggrestype, &wrkspc_size);
		if (status != OK)
		{
			FEafeerr(Adf_scb);
			r_error(0x81, NONFATAL, Cact_tname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return BAD_EXP;
		}

		/* Make sure the unique aggregates only on break columns */

		if (isunique && attribute->att_brk_ordinal <= 0)
		{
			r_error(0x7B, NONFATAL, nname, Cact_attribute,
				Cact_command, Cact_rtext, NULL);
			return(BAD_EXP);
		}

		/* Check that preset type matches all within column types
		** (unless aggregate is count) */

		if (preset != NULL && op_code != opid)
		{
			afe_cancoerce(Adf_scb, &pdbv, &(attribute->att_value),
				      &can_coerce);
			if (!can_coerce)
			{
				r_error(0x76, NONFATAL, nname, Cact_tname,
					Cact_attribute, Cact_command,
					Cact_rtext, NULL);
				return(BAD_EXP);
			}
		}

		r_mk_agg(ordinal, brk->brk_attribute, preset, fid,
			wrkspc_size, isunique, aggrestype);

	}

	/* now add the aggregate to the correct structures */

	if (cumfound)
	{
		/* cumulative primitive found */

		item->item_type = I_CUM;
		item->item_val.i_v_cum = r_mk_cum(aordinal, brk->brk_attribute,
						  preset, fid, wrkspc_size,
						  isunique, aggrestype);
	}
	else
	{
		/* primitive aggregate.  add ACC's to ATT ACC structure */

		item->item_type = I_ACC;
		acc = r_acc_make(brk->brk_attribute, aordinal, preset, fid,
				 wrkspc_size, isunique, aggrestype);
		item->item_val.i_v_acc = acc;
	}

	return GOOD_EXP;
}
