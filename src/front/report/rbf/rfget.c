/*
** Copyright (c) 2004 Ingres Corporation
*/

/* static char	Sccsid[] = "@(#)rfget.qc	30.1	11/14/84"; */

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<st.h>
# include	"rbftype.h"
# include   	<rcons.h>
# include	"rbfglob.h"
# include	<uf.h>
# include	<ug.h>
# include	<ooclass.h>
# include    	<qg.h>    
# include    	<mqtypes.h>

/*
**   RFGET - load the report specification information into
**	the report formatter data structures and then the
**	frame structures.  This routine is
**	simply a controlling routine for a series of other
**	routines which actually do the work.
**	The name of the report to get is in En_report.
**	If the -m flag has been set, it means that the
**	default report is to be used, without question.
**	If the -r flag is set, it means that the report
**	is to be read from the catalogs for sure.
**
**	Parameters:
**		repname		name of report.
**		repspec		whether report name or table name is given.
**			TRUE	if report name is given.
**			FALSE	if table name is given.
**				This automates a default for this table.
**		style		style of the report.
**
**	Returns:
**		TRUE	if report properly set up.  FALSE if not.
**
**	Side Effects:
**		runs reset for reports.
**
**	Trace Flags:
**		200, 206.
**
**	Error Messages:
**		710.
**
**	History:
**		2/10/82 (ps)	written.
**		1/20/83 (gac)	checks if specified table exists.
**		8/8/83	(gac)	Bug 903 fixed -- now gets report instead of
**				table by default.
**		5/15/84 (gac)	Bug 792 fixed -- now does not get SYSERR when
**				report has errors.
**		27-may-88 (sylviap)
**			Changed MEalloc to MEreqmem.
**		15-dec-88 (sylviap)
**			Changed to use a local variable, tbl_name instead of
**			En_report, so will not write over memory.
**		07/17/89 (martym)
**			>> GARNET <<
**			The routines for which this routine is a driver of were 
** 			changed to support multi-table sources of data. Changed 
**			the calls to match the new interfaces. Also added support 
**			to load existing RBF reports based JoinDefs.
**      9/22/89 (elein) UG changes ingresug change #90045
**    changed <rbftype.h> & <rbfglob.h> to "rbftype.h" & "rbfglob.h"
**              11/27/89 (martym) -Added support for RBF report template styles
**	12/14/89 (Elein)
**		Set Rbf_ortype to OT_DUPLICATE not OT EDIT if
**		datasource is duplicate. Set En_rid to -1 for save.  This was 
**		done after rrcoset because rrcoset needs En_rid.
**	12/27/89 (elein)
**		remove copy of name into Rbf_orname.
**	1/10/90 (martym)
**		Added parameter to the call to r_dcl_set().
**  	1/13/90 (martym) 
**      	Changed the size of tbl_name[] from FE_MAXNAME to FE_MAXTABNAME.
**	2/7/90 (martym)
**		Coding standards cleanup.
**	17-apr-90 (sylviap)
**		Reads in the value for the .PAGEWIDTH command to accommodate
**		wide reports. (#129, #588)
**	4/26/90 (martym)
**		Modified to use the source of data information (name & type)
**		stored in the report for cases where the report is not based 
**		on a single table (for now these are only cases where the 
**		source is a JoinDef).
**	27-jul-90 (sylviap)
**		Now looks at the action list and searches for a comma to see
**		if the report is based on a joindef.  Previously used to only 
**		check if the number of tables were greater than 1, but a 
**		joindef report can have exactly one table. The name of 
**		the joindef is saved in the first action command, in the
**		format:  "RBFSETUP, #, <joindef name>".  If there is a comma,
**		then the report is based on a joindef.
**	21-jun-91 (pearl)
**		Fix bug 37430.  If it's a default report, or one where it's
**		not a query and the table has been specified via the .DATA
**		or .TABLE commands, then set NumRels to 1 and put the table
**		name in the RelList array before the call to rFchk_data().
**	02-jul-91 (pearl)
**		Fix rejection of previous submission.  Change 
**		"*En_relation != NULL" to "*En_relation != EOS" for portability.
**	31-aug-1992 (rdrane)
**		Prelim fixes for 6.5 - parameterization of FErelexists().
**	9-oct-1992 (rdrane)
**		Use new constants in (s/r/rbf)_reset invocations.
**		Replace En_relation with reference to global FE_RSLV_NAME
**		En_ferslv.name. Converted r_error() err_num value to hex to
**		facilitate lookup in errw.msg file.
**	5-nov-1992 (rdrane)
**		Handle owner.tablename recognition and processing.  This lets
**		us eliminate the tbl_name local variable.  Ensure that
**		En_report defaults to the tablename portion of the relation
**		name so we don't try to save an owner.report!
**		Demoted this file to a plain vanilla ".c".
**	8-dec-1992 (rdrane)
**		Use RBF specific global to determine if delimited identifiers
**		are enabled.  If at least 6.5, ensure delimited identifiers
**		are enabled if we're creating a report from a table.
**	30-dec-1992 (rdrane)
**		Allow for r_m_dsort() to return a bool (FALSE implies no
**		eligible columns in the report).  If it does return FALSE, then 
**		issue an error message, fail the set-up, and return FALSE.
**	3-feb-1993 (rdrane)
**		Eliminate call to r_dtpl_set().  Nothing seems to reference the
**		Cdat structures anymore, and the call undoes the NULL'ing of
**		the ATT db_data to indicate an unsupported datatype.
**	23-feb-1992 (rdrane)
**		The testing of FE_resolve()'s return code was backwards ...
**		Prompt for the table usage if the table DOES exist and IS
**		accessable!  This fixes bug 49805.  Sigh.
**	4-jan-1994 (rdrane)
**		Use OpenSQL level to determine 6.5 syntax features support,
**		and check dlm_Case capability directly to determine if
**		delimited identifiers are suppported.
**	17-jan-1994 (rdrane)
**		Don't force lowercase the target name unless it's clearly
**		specified as being a report name.  Note the implication for
**		report name not found and databases which don't store regular
**		identifiers as lower case!  Bug 58271.
**      20-oct-1995 (stoli02/thaju02)
**              bug #70910 - unable to create report based on table owned
**              by "$ingres".
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


bool r_JDLoadQuery();


bool
rFget(repname,repspec,style)
char	*repname;
bool	repspec;
i2	style;
{
	/* internal declarations */

	char		def_prompt[100];
	char		*RelList[MQ_MAXRELS];           /* Relation names used 
							** in report */
	i4         	NumRels = 0;			/* Number of relations 
							** used in report */
	bool 		qloaded = FALSE;		/* indicate query of a 
							** JD report has been 
							** loaded*/
	ACT  		*next;				/* points to actions */
	bool  		jd_flag = FALSE;		/* TRUE = joindef rep */


	/* Reset global variables */

	r_reset(RP_RESET_RELMEM4,RP_RESET_RELMEM5,RP_RESET_OUTPUT_FILE,
		RP_RESET_CLEANUP,RP_RESET_LIST_END);
	s_reset(RP_RESET_SRC_FILE,RP_RESET_REPORT,RP_RESET_LIST_END);
	rFreset(RP_RESET_REPORT,RP_RESET_LIST_END);

	if ((En_ferslv.name = (char *)FEreqmem((u_i4)Rst4_tag,
					(u_i4)(FE_MAXTABNAME+1),
					TRUE,(STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("rFget - name"));
	}
	*En_ferslv.name = EOS;


	/* In case report specified ... */

	En_report = repname;
	if  (repspec)
	{
		CVlower(En_report);
	}

	St_style = style;

	/* set up report environment. */
	if (St_style == RS_NULL && r_env_set())
	{	/* report found.  Read in report specifications */
		if (En_rtype != RT_RBF)
		{	/* report created by SREPORT */
			r_error(0x2C6,NONFATAL,En_report,0);
			return(FALSE);
		}
		if (!St_silent)
		{
		    IIUGmsg(ERget(S_RF0029_Loading_old_report_sp), 
			    (bool)FALSE, 0);
		}

		/*
		** Read the RCOMMANDS table into core.
		** Set up the ACT, QUR, and SRT structures.
		*/

		/*
		** Set up ACT, QUR, and SRT structures.
		** Propagate delimited identifier enable state.
		*/
		r_rco_set();
		Rbf_xns_given = St_xns_given;

		/* need to convert to number and set it to En_lmax */
		if (STlength(En_pg_width) > 0) 
		{
		   	if (CVan(En_pg_width, &En_lmax) != OK)
			{
				/* Unable to convert. Internal error  */
				r_error(0x406,FATAL,NULL);
			}
		}

		_VOID_ r_dcl_set(FALSE);
		if( En_SrcTyp != DupRepSrc )
		{
			Rbf_ortype = OT_EDIT;
		}
		else
		{
			Rbf_ortype = OT_DUPLICATE;
			En_rid = -1;
			*En_report = EOS;
		}
		/* 
		** Check if this is a joindef report by looking for 
		** the joindef name in the first action command in 
		** ii_rcommands.  It will have a comma if the joindef 
		** name is there, otherwise no comma for reports based
		** on tables.
		*/
		next = r_gch_act((i2) 1,(i4) 1);
		if (STindex(next->act_text, ERx(","),0) != NULL)
		{
			jd_flag = TRUE;
		}
	}
	else
	{	/*
		** No report found.  Use original "report name" (non-forced
		** case).  Ensure that we'll always break up any owner.table
		** so we can re-set En_report for prompts and things like
		** report names!
		*/
		STcopy(repname,En_ferslv.name);
		if  ((En_ferslv.name_dest =
			(char *)FEreqmem((u_i4)Rst4_tag,
				(u_i4)(FE_UNRML_MAXNAME+1),
				TRUE,(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("rFget - name_dest"));
		}
		if  ((En_ferslv.owner_dest =
			(char *)FEreqmem((u_i4)Rst4_tag,
				(u_i4)(FE_UNRML_MAXNAME+1),
				TRUE,(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("rFget - owner_dest"));
		}
		En_ferslv.is_nrml = FALSE;
		/*
		** If:	there's an owner specification AND
		**		the connection is pre-6.5 OR
		**		the owner is bogus		OR
		**	the tablename is bogus
		** then consider the report "not found".
		*/
		FE_decompose(&En_ferslv);
		if  (((En_ferslv.owner_spec) &&
		      ((STcompare(IIUIcsl_CommonSQLLevel(),UI_LEVEL_65) < 0) ||
		       ((IIUGdlm_ChkdlmBEobject(En_ferslv.owner_dest,
                      			       En_ferslv.owner_dest,
                      			       En_ferslv.is_nrml) ==
							 UI_BOGUS_ID) &&
		       STcompare(En_ferslv.owner_dest, UI_FE_CAT_ID_65)))) ||
		     (IIUGdlm_ChkdlmBEobject(En_ferslv.name_dest,
					     En_ferslv.name_dest,
                      			     En_ferslv.is_nrml) ==
							 UI_BOGUS_ID))
		{
		   	IIUGerr(E_RF002B_No_report__, UG_ERR_ERROR,
			        2, En_report, ERx(""));
			return(FALSE);
		}
		if (repspec)
		{	/*
		 	** Specified as report name, but not found.  Look
			** for a table having the same name.  If one is
			** found, ask the user if this will do.
			*/
			if  (FE_resolve(&En_ferslv,En_ferslv.name_dest,
					En_ferslv.owner_dest))
			{
				STcopy(ERget(S_RF002A_use_def_rep),def_prompt);
				if (!IIUFver(ERget(E_RF002B_No_report__), 2,
					     En_report, def_prompt))
				{	/* Not ok.  Return to main menu */
					return (FALSE);
				}
			}
			else
			{
				/*
				** Data relation does not exist or is not
				** accessable.
				*/
				IIUGerr(E_RF002B_No_report__, UG_ERR_ERROR,
				        2, En_report, ERx(""));
				return(FALSE);
			}
		}
		Rbf_ortype = OT_TABLE;
		En_rtype = RT_DEFAULT;
		En_report = En_ferslv.name_dest;
		if  (repspec)
		{
			CVlower(En_report);
		}
		/*
		** If the server supports delimited identifiers, then
		** ensure that .DELIMID is enabled since we're doing a report
		** based upon a table, and it may require such services.
		*/
		if  (IIUIdlmcase() != UI_UNDEFCASE)
		{
			Rbf_xns_given = TRUE;
			St_xns_given = TRUE;
		}
	}

	/* bug 37430:  If En_ferslv.name isn't null, then this report has been
	** specified with a .DATA or .TABLE command, whose value has been 
	** placed in En_ferslv.name in r_rco_set(), or it's a default report,
	** and En_ferslv.name has been populated in the previous block. 
	*/
	if ((En_ferslv.name != NULL) && (*En_ferslv.name != EOS))
	{
		RelList[0] = En_ferslv.name;
		NumRels = 1;
	}

	/* 
	** check to see if data relation(s) are valid:
	*/
	if(!rFchk_dat(&NumRels, RelList))	
	{
		return (FALSE);
	}
	if (*En_ferslv.name == EOS)
	{
		_VOID_ STcopy(RelList[0], En_ferslv.name);
	}
	if (jd_flag)
	{
		En_SrcTyp = JDRepSrc;
	        if (!r_JDLoadQuery())
		{
			return(FALSE);
		}
		qloaded = TRUE;
	}


	/*
	** Set up the ATT structures for each attribute
	** in the data relation.
	*/
	rf_att_dflt(NumRels, RelList);		/* set up ATT structure */

	if (En_rtype == RT_DEFAULT)
	{	/* set up the default report */
		if (!St_silent)
		{
		    IIUGmsg(ERget(S_RF002C_Setting_up_default_re),
			(bool) FALSE, 0);
		}

		/* Set up first column as sort column */

		if  (!r_m_dsort())
		{
			r_error(0x416,NONFATAL,NULL);
			return(FALSE);
		}
	}

	/*	Set up the SORT data structure */

	if (!r_srt_set())		/* set up the SORT data structures */
	{	/* bad RSORT */
		return(FALSE);
	}

	/*
	** Set up the BRK data structures for each sort
	** attribute with a RACTION tuple defined for it.
	*/

	r_brk_set();		/* set up the BRK data structures */

	/*
	** Set up the CDAT and DTPL variables for Equel
	** to read the data relation.
	*/

    /*
    ** set up the CDAT and DTPL variables
    */
/*
	r_dtpl_set();
*/

	if (En_rtype == RT_DEFAULT)
	{

		/*
                ** Since the ReportWriter does not support any of RBF's 
		** template styles, if  'St_style'  is set to a template style 
		** set it to a generic ReportWriter report type before calling 
		** the ReportWriter, and then set it back:
                */
                i2 SaveStyle = St_style;
                if (St_style == RS_TABULAR )
		{
                        St_style = RS_COLUMN;
		}
                else
                if (St_style == RS_INDENTED || St_style == RS_MASTER_DETAIL)
		{
                        St_style = RS_BLOCK;
		}
                r_m_rprt();
                St_style = SaveStyle;
	}
	else
	{
		/*	Set up the TCMD data structures */
		Tcmd_tag = FEbegintag();
		r_tcmd_set();
		FEendtag();

		if (En_SrcTyp == JDRepSrc && !qloaded)
		{
	            if (!r_JDLoadQuery())
			return(FALSE);
		}

	}

	r_lnk_agg();		/* set up the CLR and OP structures for
				** fast processing of aggregates.
				*/

	if (Err_count > 0)
	{	/* No errors allowed in report.	 */
		return(FALSE);
	}

	return (TRUE);

}
