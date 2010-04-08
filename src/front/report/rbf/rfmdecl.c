/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include   "rbftype.h"
# include   "rbfglob.h"
# include	<ug.h>
# include	<cv.h>
# include	<ui.h>

/*
**   RFM_DECL - set up the .DECLARE statement/s for this report
**    	for any Columns that have the copt_select field filled in.
**	Each variable should have a .DECLARE statement.  rFm_decl creates
**	a 'de' tuple in ii_rcommands for each variable.  If the selection
**	criteria is a value, then the query has only one variable.  If it is
**	range, then two variables, min_<column name> and max_<column name>
**	are necessary.  
**
**    Parameters:
**        none.
**
**    Returns:
**		STATUS. OK   -- If successful.
**			FAIL -- If not successful.
**			(status passed back by SREPORTLIB)
**
**    Called by:
**        rFsave.
**
**    Side Effects:
**        none other than expected.
**
**	History:
**		30-nov-89 (sylviap)
**			Created.
**		2/20/90 (martym)
**			Changed to return STATUS passed back by the 
**			SREPORTLIB routines.
**		23-aug-90 (sylviap)
**			For label reports, will need to declare label variables.
**			Save the number of selection criteria variables
**			in Seq_declare so won't save over them.  (#32780)
**		29-aug-90 (cmr)
**			Removed tmp_buf (not used).
**		30-oct-1992 (rdrane)
**			Remove all FUNCT_EXTERN's since they're already
**			declared in included hdr files.  Ensure that we ignore
**			unsupported datatypes (of which there shouldn't be any).
**			Ensure attribute names are placed in Ctext in
**			unnormalized form, so re-add a tmp_buf.
**		9-mar-1993 (rdrane)
**			Handle proper construction of RW variable names from
**			delim id attributes.  This corrects/replaces most of
**			the 30-oct-1992 work.
**		1-jul-1993 (rdrane)
**			Add support for creation of the hdr/ftr date/time and
**			page # format variable declarations.  Remove adf_cb and
**			use global Adf_scb.
**		13-sep-1993 (rdrane)
**			Use MAX_FMTSTR as the size basis for the hdr/ftr
**			date/time and page # format variable declarations.
**			Also, use r_SectionBreakUp() to ensure that the
**			format strings do not break across rcotext lines
**			at inappropriate places.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/


STATUS rFm_decl()
{
    /* internal declarations */

	register i4	i;			  /* counter */
	register COPT	*copt;			  /* get a COPT given ordinal */
	register ATT	*att;			  /* get a ATT, given ordinal */
	DB_DATA_VALUE	dbdv1;		  	  /* used in call to 
						  ** afe_tyoutput */
	i4		len;		  	  /* length of column name */
        DB_USER_TYPE    user_type;
	STATUS 		stat = OK;
	char		q_opt_buf[((MAX_FMTSTR * 2) + 1)];
	char		b_opt_buf[((MAX_FMTSTR * 2) + 1)];
						/*
						** Buffers to escape quotes
						** and backslashes in format
						** specifications.
						*/
	char		attrib_name[(FE_UNRML_MAXNAME + 1)];
						/*
						** Unnormalized version of
						** base attribute name
						*/


    	/* start of routine */

    	save_em();        /* save current environment */

    	Csequence = 0;

    	STcopy(NAM_DECLARE, Ctype);
    	STcopy(ERx(" "), Csection);
    	STcopy(ERx(" "), Ccommand);
    	STcopy(ERx(""), Ctext);

	/* search trough attribute that has a selection criteria requested */

        for (i=1; i<=En_n_attribs; i++)
        {    /* next attribute */
            copt = rFgt_copt(i);
            if (copt->copt_select != 'n')
    	    {
		/* get attribute */
		if  ((att = r_gt_att(i)) == (ATT *)NULL)
		{
			continue;
		}
    
            	dbdv1.db_data = att->att_value.db_data;
            	dbdv1.db_length = att->att_value.db_length;
            	dbdv1.db_datatype = att->att_value.db_datatype;
            	dbdv1.db_prec = att->att_value.db_prec;
		
            	/* get the character representation of the datatype */
	 	if (afe_tyoutput(Adf_scb, &dbdv1, &user_type) != OK)
		{
			IIUGerr(E_RF0054_invalid_dbtype, UG_ERR_FATAL, 0);
		}
		
		IIUGxri_id(att->att_name,&attrib_name[0]);
		rFm_vnm(&attrib_name[0],copt);

		/* Create a .DECLARE based on the selection criteria */
            	switch(copt->copt_select)
                {
                case('v'):			/* Value */
		    STcopy (&copt->copt_var_name[0], Cattid);
		    STprintf (Ctext, ERget(S_RF0055_prompt_string),
			user_type.dbut_name, &attrib_name[0]);

		    RF_STAT_CHK(s_w_row());

    		    STcopy(ERx(""), Ctext);
                    break;
    
                case('r'):			/* Range */
		    STprintf (Cattid, ERx("%s%s"), ERget(S_RF0056_min), 
			&copt->copt_var_name[0]);
		    STprintf (Ctext, ERget(S_RF0058_min_prompt_string),
			user_type.dbut_name, &attrib_name[0]);

		    RF_STAT_CHK(s_w_row());

		    STprintf (Cattid, ERx("%s%s"), ERget(S_RF0057_max), 
			&copt->copt_var_name[0]);
		    STprintf (Ctext, ERget(S_RF0059_max_prompt_string),
			user_type.dbut_name, &attrib_name[0]);

		    RF_STAT_CHK(s_w_row());

		    STcopy(ERx(""), Ctext);
		    break; 
	       }
            }
        }

	/*
	** Generate the required .DECLAREs for the hdr/ftr date/time
	** and/or page # format strings.  We use MAX_FMTSTR for the size
	** instead of STlength() so that if the user finds these and passes
	** them on the command line, they won't overflow!  Don't forget
	** to escape any embedded single quotes, as well as double any
	** embedded backslashes.
	*/
	if  (Opt.rdate_inc_fmt == ERx('y'))
	{
		STprintf(Cattid,ERx("%s"),RBF_NAM_DATE_FMT);
		rF_bstrcpy(&q_opt_buf[0],&Opt.rdate_fmt[0]);
		rF_bsdbl(&b_opt_buf[0],&q_opt_buf[0]);
		STprintf(&q_opt_buf[0],ERx("varchar(%d) with value '%s'"),
			 MAX_FMTSTR,&b_opt_buf[0]);
		RF_STAT_CHK(r_SectionBreakUp(&q_opt_buf[0],Ctext,MAX_FMTSTR));
		STcopy(ERx(""), Ctext);
	}

	if  (Opt.rtime_inc_fmt == ERx('y'))
	{
		STprintf(Cattid,ERx("%s"),RBF_NAM_TIME_FMT);
		rF_bstrcpy(&q_opt_buf[0],&Opt.rtime_fmt[0]);
		rF_bsdbl(&b_opt_buf[0],&q_opt_buf[0]);
		STprintf(&q_opt_buf[0],ERx("varchar(%d) with value '%s'"),
			 MAX_FMTSTR,&b_opt_buf[0]);
		RF_STAT_CHK(r_SectionBreakUp(&q_opt_buf[0],Ctext,MAX_FMTSTR));
		STcopy(ERx(""), Ctext);
	}

	if  (Opt.rpageno_inc_fmt == ERx('y'))
	{
		STprintf(Cattid,ERx("%s"),RBF_NAM_PAGENO_FMT);
		rF_bstrcpy(&q_opt_buf[0],&Opt.rpageno_fmt[0]);
		rF_bsdbl(&b_opt_buf[0],&q_opt_buf[0]);
		STprintf(&q_opt_buf[0],ERx("varchar(%d) with value '%s'"),
			 MAX_FMTSTR,&b_opt_buf[0]);
		RF_STAT_CHK(r_SectionBreakUp(&q_opt_buf[0],Ctext,MAX_FMTSTR));
    		STcopy(ERx(""), Ctext);
	}

	/*
	** For label reports, will need to declare label variables, so need
	** to know how many selection criteria variables there are so won't
	** save over them. (#32780)
	*/
	if (St_style == RS_LABELS)
    		Seq_declare = Csequence;

    	get_em();        /* reset the environment */

    	return(OK);
}



/*
**	RFM_VNM - Construct a unique Report-Writer variable name from
**		  the supplied attribute name.  This is accomplished by
**
**		  o Stripping any non-FE characters from the attribute name
**		    (this handles delimited identifiers).
**
**		  o Assigning any "degenerate" version (all non-FE characters)
**		    a default base name with postpended (hopefully unique)
**		    sequence number.  (This is the only rationale for
**		    maintaining a global sequence number - it should save time
**		    in the default case).
**
**		  o Ensure that the generated name is not already in use by
**		    scanning the COPT table.  If it is, repeat the postpend
**		    operation on the base name until it is unique.
**
**		  Note how the following column names are processed, and how
**		  all derived names must always be tested for collisions!
**
**			col1	==> col1 Initial, so no collison. Use as is.
**			"col 1"	==> col1 Collision, postpend '1' which yields
**					 col1_1.  No collision, so use as is.
**			col1_1	==> col1_1 Collision, postpend '2' which yields
**					   col1_1_2.  No collision, so use as
**					   is.
**
**		  So, an exhaustive search of those variable names already
**		  in use is required to ensure uniqueness IN ALL CASES,
**
**		  Note that since we're dealing with RBF variables being
**		  generated for COPT use, we don't have to worry about
**		  multiple variables being associated with a single attribute
**		  (besides $Min_* and $Max_*, which use the same "suffix" and
**		  so are not really multiple instances).  So, once we establish
**		  a variable name for an attribute, we should never need
**		  to change it.
**
**	Parameters:
**      	att_name -	Pointer to the attribute name associated with
**				this variable.  This may not be NULL.
**		copt -		Pointer to the attribute's associated COPT
**				structure.  This may not be NULL (if it is,
**				then serious problems exist!).
**				
**	Outputs:
**		copt -		The copt_var_name field is filled with the
**				resultant variable name to use.
**
**	Returns:
**		Nothing.
**
**	History:
**		10-mar-1993 (rdrane)
**			Created.
**		12-mar-1993 (rdrane)
**			Yes, attribute names will be unique, but a derived
**			parameter name may "collapse" to an attribute
**			name EVEN WITH ITS SEQ and so later collide, e.g., 
**				"col 1" ==> col1_1
**				col1_1  ==> col1_1
**			So, let's verify all names, not just the collapsed
**			and/or defaulted ones!
*/

VOID
rFm_vnm(att_name,att_copt)
	char	*att_name;
	COPT	*att_copt;
{
	COPT	*copt;
	i4	ordinal;
	bool	vnm_match;
	char	VarSeqStr[(RBF_MAX_VAR_SEQ + 1)];

	
	/*
	** If we've already got one, we're all done!
	*/
	if  (att_copt->copt_var_name[0] != EOS)
	{
		return;
	}

	/*
	** Set the attribute's associated variable name by
	** stripping non-FE characters, and guard against names
	** composed of all "bogus" characters!
	*/
	IIUGfnFeName(att_name,&att_copt->copt_var_name[0]);
	if  (att_copt->copt_var_name[0] == EOS)
	{
		/*
		** Default names wil always be non-unique, so
		** add a sequence number up front.  Note the
		** assumption that RBF_DEF_VAR_NAME plus a
		** max. length VarSeqStr will always fit in
		** copt_var_name.
		*/
		Rbf_var_seq++;
		CVna(Rbf_var_seq,&VarSeqStr[0]);
		STpolycat(3,
			  RBF_DEF_VAR_NAME,
			  ERx("_"),
			  &VarSeqStr[0],
			  &att_copt->copt_var_name[0]);
	}

	/*
	** Attribute names will be unique, but their derived parameter
	** names may not be.  So, concatenate an underscore/sequence number
	** whenever we get a collision.
	*/
	do
	{
		vnm_match = FALSE;
		ordinal = 1;
		while ((copt = rFgt_copt(ordinal)) != (COPT *)NULL)
		{
			ordinal++;
			/*
			** Skip entries with no associated variable names
			** as well as the entry we're working on.
			*/
			if  ((copt->copt_var_name[0] == EOS) ||
			     (att_copt == copt))
			{
				continue;
			}
			if  (STequal(&att_copt->copt_var_name[0],
				     &copt->copt_var_name[0]) != 0)
			{
				vnm_match = TRUE;
				break;
			}
		}
		if  (vnm_match)
		{
			/*
			** Truncate any overly-long name, and erase
			** any previous underscore/sequence number.
			*/
			*(&att_copt->copt_var_name[0] +
			  (FE_MAXNAME - (sizeof(VarSeqStr) - 1) - 1)) =
						EOS;
			Rbf_var_seq++;
			CVna(Rbf_var_seq,&VarSeqStr[0]);
			STpolycat(3,
				  &att_copt->copt_var_name[0],
				  ERx("_"),
				  &VarSeqStr[0],
				  &att_copt->copt_var_name[0]);
		}
	} while (vnm_match);

	return;
}

