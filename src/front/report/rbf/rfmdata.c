/*
** Copyright (c) 2004 Ingres Corporation
*/
/* static char    Sccsid[] = "@(#)rfmdata.c    30.1    11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include   	"rbftype.h"
# include   	"rbfglob.h"
# include	<errw.h>
# include	<fedml.h>
# include	<qg.h>
# include	<mqtypes.h>
# include	<cm.h>
# include	<ug.h>
# include	<ui.h>

/*
**   RFM_DATA - set up the .DATA or .QUERY statement for this report.
**    If any of the Columns have the copt_select field filled in,
**    to prompt for a value at report time, a query is generated
**    with the correct parameters.
**
**    Parameters:
**        none.
**
**    Returns:
**	STATUS. OK   -- If successful.
**		!OK  -- If not successful.
**			(status passed back by SREPORTLIB)
**
**    Called by:
**        rFsave.
**
**    Side Effects:
**        none other than expected.
**
**    Trace Flags:
**        200.
**
**	History:
**		9/9/82 (ps)	written.
**		1/31/83 (gac)	run-time range fixed to >= minimum and <= max.
**		7/11/85 (drh)	Added capability to write .QUERY stmt in sql.
**		26-nov-1986 (yamamoto)
**			Modified for double byte characters.
**		1/28/87 (rld)	use call to afe_coerce to determine if data
**				  value in .QUERY statement should be quoted.
**		29-nov-88 (sylviap)
**			If SQL and at run time, will prompt for value, added
**			the LIKE clause to support SQL pattern matching.  If
**			NOT gateway, also added ESCAPE '\' clause to match
**			the pattern matching characters as literals.  
**			(ESCAPE may not be supported by all gateways).
**			SQL pattern matching is not supported if a range is
**			requested.  
**		23-jan-1989 (danielt)
**			changed to only add the LIKE operator if the variable
**			is text.
**		7-feb-1989 (danielt)
**			always create .QUERY (bug 3747)
**		14-apr-1989 (danielt)
**			fixed jupbug #5412
**		12-jul-89 (sylviap)
**			Took out CMtoupper of an attribute name because
**			it caused a bug on DG where it couldn't find a 
**			column name with first letter uppercased. On INGRES, 
**			it CVlowers the column name, but on DG, it does an
**			IIUGlbo_lowerBEobject which leave case alone.
**      	9/22/89 (elein) UG changes ingresug change #90045
**			changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & 
**			"rbfglob.h"
**		9/25/89 (martym)
**			>> GARNET <<
**			Added support for JoinDefs.
**		04-oct-89 (sylviap)
**			B8031 - Builds the where clause, then checks if it will
**			fit into rcotext.  If not, then breaks it up into
**			multiple strings.
**		12/14/89 (elein)
**			Replaced references to SourceIsJD with En_SrcTyp
**              2/8/90 (martym)
**                      B8715 - If selection criteria of value on a DATE
**                              column and the language is SQL don't add
**                              the LIKE predicate for pattern matching.
**		2/20/90 (martym) 
**			Changed to return STATUS passed back by the 
**			SREPORTLIB routines.
**		3/2/90 (martym)
**			Similar to the fix for bug B8715 but for money fields. 
**			If selection criteria of value on a MONEY column and 
**			the language is SQL don't add the LIKE predicate for 
**			pattern matching. Also fixed so that the selection 
**			criteria for cases when the source of data is a Joi-
**			nDef is handled correctly.
**		3/12/90 (martym)
**			Changed r_SectionBreakUp() to compensate for the 
**			fact that trailing blanks are trimmed when reports
**			are read back in.
**		4/2/90 (martym)
**			Changed to use "afe_fdesc()" to determine if the 
**			LIKE predicate could be used on the current field.
**		09-jul-90 (sylviap)
**			For joindef, rjoindef.c adds all closing parens around
**			   En_remainder, so rFm_data does not need to.  Took out
**			   code that add closing parens for joindefs. (#31482)
**		27-jul-90 (sylviap)
**			Builds the selection criteria for master/detail joindefs
**			based on the type of column (master or detail) it is.
**			#31757
**		29-jul-90 (cmr)
**			Use MQ_DFLT for tmp_buf size.  PG_SIZE is obsolete.
**		11-sep-90 (sylviap)
**			Uses En_union to initialize En_remainder. #32851.
**		07-oct-90 (sylviap)
**			Saves PAGEWIDTH in the column Rcocommand in the
**                      table, ii_rcommands.  6.4 RBF now saves the pg_width
**                      in the first row of the query, so 6.3 rbf/rw can
**                      continue to execute/edit 6.4 reports that do not take
**                      advantage of 6.4 features -- needed for backwards 
**			compatibility.
**		12-oct-90 (sylviap)
**			Surrounds the UNION SELECT clause for master/detail
**			reports with comments if there is a selection criteria
**			on a detail column.  This is done so the query does NOT
**			returns any masters w/an empty detail.  RBF puts in
**			comments so when the user changes the selection criteria
**			RBF does not have to recreate the UNION SELECT clause.
**			See rjoindef for more info. (#32781)
**		06-nov-90 (sylviap)
**			When writing out comments to database, now writes out 
**			one asterisk (instead of two) and adds 'DO NOT MODIFY..'
**			(#33894)
**		14-nov-90 (sylviap)
**			Initalized comment_out to FALSE.  Also added #defines
**			for ERx's.
**		02-may-90 (steveh)
**			MEfree of EN_remainder only takes place if it is not
**			a zero length string.  This eliminates the problem of
**			freeing the static string "" set in rreset.c.
**		9-oct-1992 (rdrane)
**			Replace En_relation with reference to global
**			FE_RSLV_NAME En_ferslv.name.
**		30-oct-1992 (rdrane)
**			Remove all FUNCT_EXTERN's since they're already
**			declared in included hdr files.  Declare
**			r_SectionBreakUp() as static to this module.  Ensure
**			that we ignore unsupported datatypes (of which there
**			shouldn't be any).
**		11-nov-92 (rudyw)
**			Account for EOS character as part of the length of a
**			string being copied to requested memory. In some cases,
**			the unaccounted for EOS stepped on a pointer. (#47797)
**		9-mar-1993 (rdrane)
**			Generate the attrib_name and rvar in the WHERE clause
**			as unnormalized (rvar should only be delimited if
**			a JoinDef).  Generate the actual variable name as
**			stripped, and ensure that it's unique!  Remove
**			bslashchar - it's unreferenced.
**		13-sep-1993 (rdrane)
**			Promote r_SectionBreakUp() to a generic routine (no
**			longer static to this module) to  further support user
**			override of hdr/ftr timestamp and page # formatting.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

#define O_COMMENT  ERx("/*")
#define C_COMMENT  ERx("*/")
#define BLANK  	   ERx(" ")


FUNC_EXTERN bool   	r_JDMaintAttrList();
FUNC_EXTERN VOID   	rfNextQueryLine();
FUNC_EXTERN DB_LANG	IIAFdsDmlSet();
FUNC_EXTERN bool	IIRF_mas_MasterTbl();

STATUS rFm_data(right_margin)
i4  	right_margin;
{
    /* internal declarations */

	bool		make_where;		/* TRUE if query required */
	register i4	i;			/* counter */
	register COPT	*copt;			/* get a COPT, given ordinal */
	register ATT	*att;			/* get a ATT, given ordinal */
	char		*andword;		/* "and " or "" depending */
	char		t_attrib_name[(FE_MAXNAME + 1)];
	char		attrib_name[(FE_UNRML_MAXNAME + 1)];
						/* hold the attribute name */
	DB_DATA_VALUE	dbdv1, dbdv2;		/* used in call to afe_coerce */
	bool		quotable;		/* TRUE if type needs quotes */
	char		*quotechar;		/* quote string for language */
	i4		retstatus;		/* function return status */
	char		tmp_buf[MQ_DFLT];  	/* buffer for WHERE clause. */
	STATUS		stat = OK;		/* Return Status */
	char 		t_rvar[(FE_MAXNAME + 1)];
	char 		rvar[(FE_UNRML_MAXNAME + 1)];
						/*
						** Range Variable Name - may be
						** delim ID if JoinDef
						*/
	char 		*MRemainAdd = (char *)NULL;/* Addition to En_remainder*/
						   /* for selection criteria */
						   /* on master tables */
	char 		*DRemainAdd = (char *)NULL;/* Addition to En_remainder*/
						   /* for selection criteria */
						   /* on detail tables */
	i4		err;			   /* error occurred */
	bool		comment_out = FALSE;	   /* TRUE = comment out UNION 
						   ** SELECT clause */


    /* start of routine */

#    ifdef    xRTR1
    if (TRgettrace(200,0))
    {
        SIprintf(ERx("rFm_data: entry.\r\n"));
    }
#    endif


    save_em();        /* save current environment */
    Csequence = 0;
    /*
    **              ****  THIS IS A KLUDGE *****
    ** We need to save the right margin somewhere that would allow 6.3
    ** RBF and RW to continue working (backwards compatibility).  6.4 RBF 
    ** saves the right margin in the column Rcocommand of the the table 
    ** ii_rcommands for the FIRST row of the 'query' (either sq or qu) type. 
    ** 6.3 RBF and RW does not look for anything in this column, since this 
    ** column is usually blank.  This is a 
    ** workaround for RBF generating a .PAGEWIDTH command for RBF reports 
    ** because 6.3 doesn't know about it and would fail with the error:
    **     "E_RW0041 r_rco_set: Error in RCOMMANDS table. Code is 2"
    **
    ** It's not pretty, but it works!
    */

    CVna (right_margin, Ccommand);


    /* First find out if a query is needed */

    make_where = FALSE;

    /*
    ** Need to copy En_remainder from En_union each time the report is saved
    ** because will still build other qualifiers to the end of it.
    ** Will build an incorrect query if saving a m/d report twice without
    ** exiting to the report catalog. (#32851)
    */

    if (En_remainder != NULL && *En_remainder != EOS)
    {
	_VOID_ MEfree((PTR) En_remainder);
	En_remainder = NULL;
    }

    if ((En_union != NULL) && ((i = STlength(En_union)) != 0))
    {
	i++;      /* account for EOS character as part of length needed */
	if ((En_remainder=(char *)MEreqmem((u_i4)0,(u_i4)i, TRUE, 
		(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("rFm_data"));
	}
	STcopy (En_union, En_remainder);
    }

    if ((En_SrcTyp == JDRepSrc) && (En_q_qual != NULL || En_remainder != NULL))
    {
        make_where = TRUE;
    }
    else
    {
        for (i=1; i<=En_n_attribs; i++)
        {    /* test next attribute */
            copt = rFgt_copt(i);
            if (copt->copt_select != 'n')
            {    /* got at least one */
                make_where = TRUE;
                break;
            }
        }
    }


    if ( En_qlang == FEDMLSQL )
        STcopy(NAM_SQL, Ctype);
    else
        STcopy(NAM_QUERY, Ctype);


    /* If QUEL, generate a RANGE statement for the table */


    if ( En_qlang == FEDMLQUEL )
    {
        STcopy(NAM_RANGE, Csection);
        STcopy(ERx("RBF"), Cattid);            /* arbitrary range variable */
        STcopy(En_ferslv.name, Ctext);

        RF_STAT_CHK(s_w_row());

        STcopy(BLANK, Cattid);            /* for the rest of the commands */
        STcopy(BLANK, Ccommand);
    }


    /* Now the targetlist, or the SQL select clause */


    STcopy(BLANK, Cattid);
    STcopy(NAM_TLIST, Csection);
    if ( En_qlang == FEDMLSQL )
    {
        if (En_SrcTyp == JDRepSrc)
        {
            RF_STAT_CHK(r_SectionBreakUp(En_tar_list, Ctext, (MAXRTEXT-25)));
        }
        else
        {
            STcopy(ERx("*"), Ctext);

            RF_STAT_CHK(s_w_row());
        }
        STcopy(BLANK, Ccommand);     /* for the rest of the commands */
    }
    else
    {
        STcopy(ERx("RBF.all"), Ctext);        /* get all */

        RF_STAT_CHK(s_w_row());
    }

    /* If SQL, generate a FROM statement, instead of a RANGE statement */

    if ( En_qlang == FEDMLSQL )
    {
        STcopy(NAM_FROM, Csection);
        if (En_SrcTyp == JDRepSrc)
        {
            RF_STAT_CHK(r_SectionBreakUp(En_sql_from, Ctext, (MAXRTEXT-25)));
        }
        else
        {
	    STprintf(Ctext,ERx("%s RBF"),En_ferslv.name);

            RF_STAT_CHK(s_w_row());
        }
        STcopy(BLANK, Cattid);
    }

    if (!make_where)
    {
    	if (En_SrcTyp == JDRepSrc)
    		/* 
		** save the table type (master/detail) in a comment after 
		** the query 
		*/
    		IIRFtty_TableType();

        get_em();        /* reset the environment */
        /* 
        ** fix for bug 3747.  always make a .QUERY, but
        ** just don't do a query qualification
        */
        return(OK);  
    }
            
    /* Now the qualification.  Precede all except the first with "and " */
    STcopy(NAM_WHERE, Csection);
    andword = ERx("");


    if (En_q_qual != NULL && *En_q_qual != EOS)
    {
       	RF_STAT_CHK(r_SectionBreakUp(En_q_qual, Ctext, (MAXRTEXT-25)));
    	andword = NAM_AND;
    }


    /*
    ** Generate qualifications for columns with selection criteria:
    */
    _VOID_ STcopy(ERx("RBF"), rvar);
    for (i=1; i<=En_n_attribs; i++)
    {    /* next attribute */
    	copt = rFgt_copt(i);
        if (copt->copt_select == 'n')
	{
                continue;
	}
   
        if  ((att = r_gt_att(i)) == (ATT *)NULL)
	{
		continue;
	}
    
	/*
	** If we have got a JoinDef, figure out the correct rvar, and 
	** field name.  In any case, always build the variable from the
	** unnormalized representation.  This will force post-fixing
	** of a sequence number due to the presence of the delimiting
	** quotes, and so avoid collisions when identifiers are case-sensitive.
	*/
    	if (En_SrcTyp == JDRepSrc)
	{
		if (!r_JDMaintAttrList(JD_ATT_GET_RVAR_NAME, att->att_name, 
				NULL, &t_rvar [0]))
		{
			return(FAIL);
		}

		if (!r_JDMaintAttrList(JD_ATT_GET_FIELD_NAME, &t_rvar[0], 
				att->att_name, &t_attrib_name [0]))
		{
			return(FAIL);
		}

        	IIUGxri_id(&t_rvar[0],&rvar[0]);
        	IIUGxri_id(&t_attrib_name[0],&attrib_name[0]);
		rFm_vnm(&attrib_name[0],copt);
	}
	else
	{
        	IIUGxri_id(att->att_name,&attrib_name[0]);
		rFm_vnm(&attrib_name[0],copt);
	}

        /* determine if field is quotable (if coercible to char type) */
        dbdv1.db_datatype = DB_CHR_TYPE;
        dbdv2.db_datatype = att->att_value.db_datatype;
        retstatus = afe_cancoerce(FEadfcb(), &dbdv1, &dbdv2, &quotable);
        if (retstatus != OK)
                IIUGerr(E_RF0033_rFm_data__Can_t_deter, UG_ERR_FATAL, 0);

        /* set quoting character */
        if (quotable)
	{
                if (En_qlang == FEDMLSQL)
                    quotechar = NAM_SQLQUOTE;
                else
                    quotechar = NAM_QUELQUOTE;
	}
        else
	{
                quotechar = ERx("");
	}
    
        switch(copt->copt_select)
        {
                case('v'):

                    /* Select by value */
                    /* need to put LIKE operator if type is text */
                    if ((En_qlang == FEDMLSQL) && quotable) 
                    {
			/*
			** B8715: Even though date is coercible to char
			**        the SQL "LIKE" predicate can not
			**        be used on it:
			**
			** The same is true for money fields.
			**
			** (4/2/90  martym) This dosen't take into 
			** account user-defined data types which though 
			** are not supported by RBF yet, may be supported 
			** in the future. So instead use "afe_fdesc()" to 
			** see if the LIKE predicate (operator) can be 
			** used on the current field type.
			**
			if (abs(att->att_value.db_datatype) == DB_DTE_TYPE ||
				abs(att->att_value.db_datatype) == DB_MNY_TYPE)
			{
			       STprintf(tmp_buf,
					ERx(" %s (%s.%s = %s$%s%s)"),
					andword, rvar, attrib_name, quotechar,
					copt->copt_var_name, quotechar);
			}
			*/

			AFE_OPERS 	ops;
			DB_DATA_VALUE 	*sops[2];
			DB_DATA_VALUE 	result;
			ADI_FI_DESC   	fdesc;
			ADI_OP_ID	LikeOpID;
			DB_LANG 	OldDML;

			dbdv2.db_datatype = att->att_value.db_datatype;
			sops[0] = &dbdv2;
			sops[1] = &dbdv2;
			ops. afe_ops = sops;
			ops. afe_opcnt = 2;
			OldDML = IIAFdsDmlSet(DB_SQL);
			_VOID_ afe_opid(Adf_scb, ERx("like"), &LikeOpID);
			_VOID_ IIAFdsDmlSet(OldDML);
			retstatus = afe_fdesc(Adf_scb, LikeOpID, &ops, NULL, 
						&result, &fdesc);
			if (retstatus != OK)
			{
			       if (Adf_scb->adf_errcb. ad_errcode != 
						E_AF6006_OPTYPE_MISMATCH)
			       {
				   FEafeerr(Adf_scb);
                		   IIUGerr(E_RF0086_rFm_data__Failed_LIKE, 
					UG_ERR_FATAL, 0);
			       }
					
			       STprintf(tmp_buf,
					ERx(" %s (%s.%s = %s$%s%s)"),
					andword, rvar, attrib_name, quotechar,
					copt->copt_var_name, quotechar);
			}
			else
                        /* 
			** fix for bug 5412 - added 'OR 
			** RBF.colname = colname 
			*/
                        if (IIUIdml() != UI_DML_GTWSQL)
                        {
                          /*
                          ** Generate ESCAPE clause if not a
                          ** gateway.
                          */
                          STprintf(tmp_buf,
                            ERx(" %s (%s.%s LIKE '$%s' ESCAPE '\\\\' OR %s.%s = '$%s' )"),
                            andword, rvar, attrib_name, copt->copt_var_name, 
			    rvar, attrib_name, copt->copt_var_name);
                        }
                        else
                        {
                          STprintf(tmp_buf,
                            ERx(" %s (%s.%s LIKE '$%s' OR %s.%s = '$%s' )"),
                            andword, rvar, attrib_name, copt->copt_var_name, 
			    rvar, attrib_name, copt->copt_var_name);
                        }
                        /* end fix for bug 5412 */
                    }
                    else
                    {
                        STprintf(tmp_buf,
                           ERx(" %s (%s.%s = %s$%s%s)"),
                           andword, rvar, attrib_name, quotechar,
			   copt->copt_var_name, quotechar);
                    }
		    /* 
		    ** B 8031 - where clause is too big to fit in one
		    ** line, so break it up into multiple lines.
		    */
                    RF_STAT_CHK(r_SectionBreakUp(tmp_buf, Ctext, 
				(MAXRTEXT-25)));

		    /* 
		    ** En_remainder is only set when the report is based on a 
		    ** joindef.  Add the selection criteria to either
		    ** MRemainAdd (master) or DRemainAdd (detail).
		    */
		    if (En_remainder != NULL && *En_remainder != EOS)
		    {
			if (IIRF_mas_MasterTbl(rvar, &err))
			{
			   _VOID_ rfNextQueryLine(&MRemainAdd, tmp_buf);
			}
			else if (err == OK)		
			{
			   _VOID_ rfNextQueryLine(&DRemainAdd, tmp_buf);
			}
			else 
			{
				IIUGerr(E_RF0092_no_range_var, UG_ERR_ERROR, 1,
					rvar);
			        return (FAIL);
			}
		    }

                    andword = NAM_AND;
                    break;
    
                case('r'):
                    /* Select by range of values */
                    STprintf(tmp_buf,
                        ERx(" %s ((%s.%s >= %s$Min_%s%s) and (%s.%s <= %s$Max_%s%s))"),
                        andword, rvar, attrib_name,
			quotechar, copt->copt_var_name, quotechar,
			rvar, attrib_name,
			quotechar, copt->copt_var_name, quotechar);

		    /* 
		    ** B 8031 - where clause is too big to fit in one
		    ** line, so break it up into multiple lines.
		    */
                    RF_STAT_CHK(r_SectionBreakUp(tmp_buf, Ctext, 
					(MAXRTEXT-25)));

		    /* 
		    ** En_remainder is only set when the report is based on a 
		    ** joindef.  Add the selection criteria to either
		    ** MRemainAdd (master) or DRemainAdd (detail).
		    */
		    if (En_remainder != NULL && *En_remainder != EOS)
		    {
			if (IIRF_mas_MasterTbl(rvar, &err))
			{
				_VOID_ rfNextQueryLine(&MRemainAdd, tmp_buf);
			}
			else if (err == OK)
			{
				_VOID_ rfNextQueryLine(&DRemainAdd, tmp_buf);
			}
			else
			{
				IIUGerr(E_RF0092_no_range_var, UG_ERR_ERROR, 1,
					rvar);
			        return (FAIL);
			}
		    }

                    andword = NAM_AND;
                    break;

        } /* switch */
    } /* for */


    STcopy(NAM_REMAINDER, Csection);

    /* En_remainder is only set when the report is based on a joindef.  */
    if (En_remainder != NULL && *En_remainder != EOS)
    {
	/* 
	** Check if there is a selection criteria on a detail column.
	** If so, then will comment out the UNION SELECT clause.  This 
	** will fix the bug
	** that would return masters w/no detail rows when no detail rows
	** fit the selection criteria.  We want to build a query that will
	** follow QBF's behavior.  (#32781)
	**
	** The UNION SELECT clause is for master w/NO detail rows.  
	** Since there is a selection criteria on a detail column,
	** we do NOT want to have any masters w/no detail rows.
	*/

	if (DRemainAdd != (char *)NULL) 
	{
		/* 
		** Add comment text so if report is copyrep/archived out,
		** user will have an explanation of the commented query clause.
		*/
		STcopy (O_COMMENT, Ctext);
            	RF_STAT_CHK(s_w_row());
		STcopy (ERget(E_RW1413_do_not_modify), Ctext);
            	RF_STAT_CHK(s_w_row());
		STcopy (ERx("*"), Ctext);
            	RF_STAT_CHK(s_w_row());
		STcopy (ERget(F_RF0089_union_comment1), Ctext);
            	RF_STAT_CHK(s_w_row());
		STcopy (ERget(F_RF008A_union_comment2), Ctext);
            	RF_STAT_CHK(s_w_row());
		STcopy (C_COMMENT, Ctext);	/* close w/one asterisk
						** or sreport gets confused */
            	RF_STAT_CHK(s_w_row());

		/* open comment for union clause */
		STcopy (O_COMMENT, Ctext);
            	RF_STAT_CHK(s_w_row());
		comment_out = TRUE;
	}
	else
	{
		comment_out = FALSE;
	}


	/* If there's no detail selection criteria, then close the clause. */
 	if (DRemainAdd == (char *)NULL)
		_VOID_ rfNextQueryLine(&En_remainder, ERx(")"));

	/* Add the UNION SELECT clause for the join */
       	RF_STAT_CHK(r_SectionBreakUp(En_remainder, Ctext, 
			(MAXRTEXT-25)));

	/* 
	** If a selection criteria in the detail table was made, add to the
	** end of the where clause.
	*/
	if (DRemainAdd != (char *)NULL)
	{
		/* closes the clause for the selection criteria */
		_VOID_ rfNextQueryLine(&DRemainAdd, ERx(")"));
		RF_STAT_CHK(r_SectionBreakUp(DRemainAdd, Ctext,
				(MAXRTEXT-25)));
		_VOID_ MEfree((PTR) DRemainAdd);
	}
	if (MRemainAdd != (char *)NULL)
	{
		RF_STAT_CHK(r_SectionBreakUp(MRemainAdd, Ctext,
				(MAXRTEXT-25)));
		_VOID_ MEfree((PTR) MRemainAdd);
	}
    }


    if (comment_out)
    {
	/* close comment for union clause */
	STcopy (C_COMMENT, Ctext);
        RF_STAT_CHK(s_w_row()); 
    }

    /* save the table type (master/detail) in a comment after the query */

    IIRFtty_TableType();

    get_em();        /* reset the environment */

    return(OK);
}

/*
**    Name:    R_SECTIONBREAKUP
**
**    Description:
**	Copy a string conforming to query token rules a query, a chunk at a
**	time into a buffer, and call s_w_row(). Will only return FAIL if the
**	call to s_w_row() fails, otherwise it will return OK.
**
**    Parameters:
**	source     -- The source string.
**	target     -- The target string.
**	buf_size    -- The number of chars to copy each time.
**
**    Returns:
**	STATUS	OK   -- If successful.
**		!OK  -- If not successful.
**			(status passed back by SREPORTLIB)
**
**    History:
**	9-mar-1993 (rdrane)
**		Correct ordering of intial "anything to do?" test - check
**		for NULL pointer before empty string!
**	21-jul-1993 (rdrane)
**		Re-write this routine so as to not break string constants
**		across rcotext lines (b46373), to support hexadecimal
**		string literals (future?), and to support all compound
**		operators (added **, !=, and ^=).
**	13-sep-1993 (rdrane)
**		Promote r_SectionBreakUp() to a generic routine (no
**		longer static to this module) to  further support user
**		override of hdr/ftr timestamp and page # formatting.
**		In this regard, it is used to break up long strings which
**		while not part of a query, do obey the token rules of queries.
**	28-sep-1993 (rdrane)
**		Don't eat the character following a string - it might be
**		a parens or other operator.  Add a leading space to any
**		"remainder" string if we've gone through the break-up loop.
**		If we don't fit while processing a string, make sure we
**		reset in_quote - otherwise we'll get confused!
**		Don't allow the line to be split after the delimiting
**		period of a compound identifier - compound identifiers,
**		like strings, cannot be split across lines.  relax the rules
**		concerning operators - we weren't allowing breaks except on
**		+, -, and /.  These all address aspects of bug 54949, and the
**		ramifications arising from it correction.
*/
STATUS
r_SectionBreakUp(source,target,buf_size)
	char	*source;
	char	*target;
	i4	buf_size;
{
	i4	i;
	i4	len;
	STATUS	stat;
	bool	in_quote;
	bool	in_compound;
	bool	in_brk;
	char	q_delim;
	char	*i_ptr;
	char	*o_ptr;
	char	*i_end_ptr;
	char	*ie_char_ptr;
	char	*oe_char_ptr;

	/*
	** Ensure that we're dealing with a non-NULL, non-empty
	** input string, and that we have a place to put our result.
	*/					
	if  ((source == NULL) || (*source == EOS) || (target == NULL) ||
	     (buf_size <= 0))
	{
		return(OK);
	}
	
	i_ptr = source;
	ie_char_ptr = i_ptr;
	o_ptr = target;
	oe_char_ptr = o_ptr;
	in_quote = FALSE;
	in_compound = FALSE;
	in_brk = FALSE;
	stat = OK;
	while (TRUE)
	{
		/*
		** If we're lucky, what's left will all fit ...
		*/
		if  (STlength(i_ptr) < buf_size)
		{
			if  (in_brk)
			{
				STpolycat(2,ERx(" "),i_ptr,target);
			}
			else
			{
				STcopy(i_ptr,target);
			}
			RF_STAT_CHK(s_w_row());
			break;
		}

		/*
		** Establish how far we can go with the input string
		** before we're forced to break the line.  Allow for
		** the preceding space (starting w/ second segment)
		** and the EOS.
		*/
		i_end_ptr = i_ptr + (buf_size - 2);
		while (i_ptr <= i_end_ptr)
		{
			/*
			** Strings are assumed to always fit on an
			** unbroken line ...
			*/
			if  (in_quote)
			{
				if  ((q_delim == ERx('\"')) &&
				     (*i_ptr == ERx('\\')) &&
				     (*(i_ptr + 1) == ERx('\"')))
				{
					/*
					** If it's a double quoted string,
					** handle any escaped double quote.
					*/
					CMcpyinc(i_ptr,o_ptr);
					CMcpyinc(i_ptr,o_ptr);
					continue;
				}
				if  (*i_ptr != q_delim)
				{
					/*
					** Not embedded single quote and not
					** end of string.
					*/
					CMcpyinc(i_ptr,o_ptr);
					continue;
				}
				CMcpyinc(i_ptr,o_ptr);
				if  (*i_ptr == q_delim)
				{
					/*
					** Escaped quote - delim id or single
					** quoted string
					*/
					CMcpyinc(i_ptr,o_ptr);
					continue;
				}
				/*
				** End of string literal or delim id.  We
				** already copied the ending quote.  To ensure
				** that we don't breakup compound identifiers,
				** look for a delimiter (.).  If not there,
				** set our new break point.
				*/
				in_quote = FALSE;
				if  (*i_ptr == ERx('.'))
				{
					in_compound = TRUE;
				}
				else
				{
					ie_char_ptr = i_ptr;
					oe_char_ptr = o_ptr;
				}
				continue;
			}			
			if  ((*i_ptr == ERx('\'')) || (*i_ptr == ERx('\"')))
			{
				/*
				** Start of a string.  Unless we're in a
				** compound identifier, mark the break here
				** and do not include this character.
				**  This ensures that the previous character
				** is included in any output segment, and that
				** the next input segment will begin with the
				** delimiter.
				*/
				if  (!in_compound)
				{
					ie_char_ptr = i_ptr;
					oe_char_ptr = o_ptr;
				}
				q_delim = *i_ptr;
				CMcpyinc(i_ptr,o_ptr);
				in_quote = TRUE;
			}
			else if  ((CMoper(i_ptr)) &&
				  (*i_ptr != ERx('$')) &&
				  (*(i_ptr + 1) != ERx('=')))
			{
	       			/*
				** Always include this character.  We've
	       			** ensured that we don't break up a compound
				** operator equals operator (!=, ** etc.),
				** or a parameter name ($) Now, just ensure
				** that we recognize a compound delimiter and
				** don't trip over <> and **.  If we don't,
				** then mark the following character as
				** the new break.  Note that we effectively
				** ignore illegal compound operators.
				*/
				if  (*i_ptr == ERx('.'))
				{
					in_compound = TRUE;
					CMcpyinc(i_ptr,o_ptr);
					continue;
				}
				CMcpyinc(i_ptr,o_ptr);
				in_compound = FALSE;
				if ((*i_ptr != ERx('>')) &&
					 (*i_ptr != ERx('*')))
				{
					ie_char_ptr = i_ptr;
					oe_char_ptr = o_ptr;
				}
			}
			else if (CMwhite(i_ptr))		
			{
				/*
				** Include this character, and mark the
				** following character as the new break
				*/
				in_compound = FALSE;
				CMcpyinc(i_ptr,o_ptr);
				ie_char_ptr = i_ptr;
				oe_char_ptr = o_ptr;
			}
			else if  (((*i_ptr == ERx('X')) ||
				   (*i_ptr == ERx('x'))) &&
				    (*(i_ptr + 1) == ERx('\'')))
			{
				/*
				** Without including this character, mark
				** it as the new break point.  Skip the hex
				** introducer and the initial  quote, then
				** treat as a string literal.
				*/
				in_compound = FALSE;
				ie_char_ptr = i_ptr;
				oe_char_ptr = o_ptr;
				CMcpyinc(i_ptr,o_ptr);
				q_delim = *i_ptr;
				CMcpyinc(i_ptr,o_ptr);
				in_quote = TRUE;
			}
			else
			{
				/*
				** Include this character and move on.
				*/
				in_compound = FALSE;
				CMcpyinc(i_ptr,o_ptr);
			}
		}
		*oe_char_ptr = EOS;
		RF_STAT_CHK(s_w_row());
		in_brk = TRUE;
		/*
		** Reset in_quote, since we'll restart the
		** scan at the leading quote, and so immediately
		** re-establish the parse state.  Otherwise, escaped
		** embedded quotes will cause premature string termination.
		** Ditto for in_compound.
		*/
		in_quote = FALSE;
		in_compound = FALSE;
		i_ptr = ie_char_ptr;
		o_ptr = target;
		*o_ptr++ = ERx(' ');
		oe_char_ptr = o_ptr;
	}
	
	return(OK);
}

/* 
** Name: IIRFtty_TableType
**
** Description: Saves the 'types' of tables in a comment block in the 
**		'remainder' section of the ii_rcommands.  The type 
**		(master/detail) is necessary when building the query for
**		the selection criteria for a master detail joindef.  For
**		selection criteria on a detail column, the clause goes in:
**			NOT EXISTS (SELECT *.... WHERE <detail_sel_criteria>)
**
**		The selection criteria for a master column goes:
**			NOT EXISTS (SELECT *.... WHERE ...)
**			AND <master_sel_criteria>
**
**		The types are saved corresponding to the tables in the FROM
**		clause in the query.  This order is preserved in the linked
**		TBL list.
*/

STATUS			
IIRFtty_TableType ()
{
    STATUS		stat = OK;		/* Return Status */
    char		tmp_buf[MQ_DFLT];  	/* buffer for WHERE clause. */
    TBL			*cur_tbl;
    char		rels[MQ_MAXRELS+1];	/* types (M or D) of tables 
						** in TARG list */ 
    /* 
    ** Save the type (master or detail) of table - only for joindef reports 
    ** Save either 'm' or 'd' for each table in the FROM clause in a comment 
    ** after the query
    */

    STcopy(NAM_REMAINDER, Csection);
    if (Ptr_tbl != NULL)
    {
	rels[0] = '\0';
	cur_tbl = Ptr_tbl->first_tbl;
	while (cur_tbl != NULL)
	{
		if (cur_tbl->type == TRUE)
			_VOID_ STcat (rels, ERx("m"));
		else
			_VOID_ STcat (rels, ERx("d"));
		cur_tbl = cur_tbl->next_tbl;
	}
	STcopy (O_COMMENT, Ctext);
        RF_STAT_CHK(s_w_row());
	STcopy (ERget(E_RW1413_do_not_modify), Ctext);
        RF_STAT_CHK(s_w_row());
	STcopy (ERx("*"), Ctext);
        RF_STAT_CHK(s_w_row());
	STprintf(tmp_buf, ERx("*  %s"), rels);

	/* Now write out to the database in the 'remainder' section */
	RF_STAT_CHK(r_SectionBreakUp(tmp_buf, Ctext, (MAXRTEXT-25)));
	STcopy (C_COMMENT, Ctext);
        RF_STAT_CHK(s_w_row());
    }
}
