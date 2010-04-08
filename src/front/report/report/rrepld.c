/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<er.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<cm.h>
# include	<si.h>
# include	<ui.h>
# include	<ug.h>
# include	<uigdata.h>
# include	<fedml.h>
# include	<errw.h>
# include	<rtype.h>	/* includes the rcons.h file */
# include	<rglob.h>
# include	<oocat.h>


/*
**   R_REP_LOAD - load the report specification information into
**	the report formatter data structures.  This routine is
**	simply a controlling routine for a series of other
**	routines which actually do the work.  If the St_style
**	flag (-m) flag was specified on the command line, don't
**	even look for the report, but simply use default.
**	If the -r flag was set, only look for the report, don't
**	look for the default.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.  (The program will abort from the underlying
**			routines when fatal errors occur).
**
**	History:
**		3/5/81 (ps)	written.
**		4/1/82 (ps)	add default report.
**		12/1/84 (rlm)	removed unneeded parameter from r_rco_set call.
**		04-dec-1987 (rdesmond)
**			added call to OOinit(), after call to FEingres().
**		03-feb-1988 (rdesmond)
**			initialize En_rid to -1 so RBF can set it from
**			catalog frame and eliminate a redundant DB access.
**		28-aug-89 (sylviap)
**			B7584: If the report specified by the -r flag is not 
**			a valid report, error E_RW13F7 is printed, rather than 
**			E_RW1004.  New error states 'Report is not found'(13F7),
**			old error 'Report or table is not found' (1004).
**		1/11/90  (martym)
**			Changed to pass parameter into r_dcl_set().
**    		16-apr-90 (sylviap)
**            		Added support for .PAGEWIDTH. (#129, #588)
**		4/30/90 (elein)
**			r_sendqry and rbrkset were changed to return a
**			true/false value for success (or not)  Check return
**			value for more consistent error messages.
**              10/9/90 (elein) 33889
**                      Add set autocommit off calls.  In order to be consistent
**                      with the TM and language products (what a concept!)
**                      we will set autocommit off if language is SQL and
**                      auto commit wasn't on in the first place.
**		28-sep-1992 (rdrane)
**			Converted r_error() err_num value to hex to facilitate
**			lookup in errw.msg file.  Replace En_relation
**			reference with global FE_RSLV_NAME structure name
**			reference.  If at least 6.5, ensure St_xns_given
**			is enabled if we're creating a report on a table.
**		25-nov-1992 (rdrane)
**			Rename references to .ANSI92 to .DELIMID as per
**			the LRC.
**		21-dec-1992 (rdrane)
**			Allow for r_m_dsort() to return a bool and to
**			return if it's FALSE (no eligible columns in the
**			report).  Note that failures in other set-up routines
**			will effect the same type or return.  The assumption is
**			that they will have issued an error and so upon return
**			to r_report(), r_report() will see a non-zero
**			Err_count and abort.
**		14-jan-1993 (rdrane)
**			Allocate En_ferslv.name_dest and En_ferslv.owner_dest
**			buffers here for a default report based upon a table.
**			This type of report does not go through r_rco_set(),
**			which is where it was occuring.
**		4-jan-1994 (rdrane)
**			Check dlm_Case capability directly to determine if
**			delimited identifiers are suppported, rather than
**			trusting the StandardCatalogLevel.
**		21-jun-1994 (harpa06)
**			Get the proper BE representation of a table name 
**			that has been specified on the command line.
**		14-feb-1996 (angusm)
**			Back out previous change: not needed for bugs 
**			 68241, 68233, and causes default report on
**			delim id table to fail with E_RW1004 (bug 74665).
**			Table names for default report are fully parsed
**			and interpreted in ui!ferslv.qsc!FE_resolve()
**		04-jun-1997 (consi01) 81583
**			Report names passed from ABF char type variables
**			have trailing white spaces which were trimmed in
**			call to IIUGdlm_ChkdlmBEobject. Fix for 74665
**			backed out this call, causing bug 81583. Added
**			call to STtrmwhite to remove trailing white space.
**      03-may-1999 (rodjo04) bug 96801
**          Using 'set role' statement in the .SETUP section can
**          change the 'effective user' thus requiring that the 
**          IIUIdbdata structure to be updated. We must ensure that 
**          this structure is updated before the query is sent. Created
**          hack to call IIUIdlmcase(). We don't care what the call 
**          returns. But by calling IIUIdlmcase(), if the structure needs
**          to be updated, then it will be, otherwise nothing will be done. 
**          We could call IIUIdci_initcap(), but why update when you 
**          don't have to.                   
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	3-May-2007 (kibro01) b118230
**	    ReportWriter needs to know about date_type_alias
**	18-Jun-2007 (kibro01) b118230/b118511
**	    Backed out change for b118230, now fixed in FEadfcb()
**	26-Aug-2009 (kschendel) b121804
**	    Bool prototypes to keep gcc 4.3 happy.
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

FUNC_EXTERN bool r_att_dflt(void);

VOID
r_rep_load ()
{
	char    *outname;
	char    *tempname;


	/* start of routine */

	r_open_db();		/* open database. If not there, abort */

	STtrmwhite(En_report);

	/* start object-oriented facility for retrieving reports */
	IIOOinit((OO_OBJECT **)NULL);

	En_rid = -1;

	/* set up report environment. */
	if (St_style==RS_NULL && (St_ispec || r_env_set()))
	{	/* report found.  Read in report specifications */
		if (!St_silent)
		{
			SIprintf(ERget(F_RW0005_Reading_report_spec));
			SIflush(stdout);
		}

		r_rco_set();	/* read in the RCOMMANDS table into core */
	}
	else
	{	/* no report found.  Use report name */
		if (St_repspec)
		{	/* error because report does not exist */
			r_error(0x3F7, FATAL, En_report, NULL);
		}
		En_rtype = RT_DEFAULT;
		En_ferslv.name = En_report;
		if  ((En_ferslv.name_dest = (char *)FEreqmem((u_i4)Rst4_tag,
		     (u_i4)(FE_MAXNAME+1),TRUE,(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(
					ERx("r_rep_loadld - name_dest"));
		}

		if  ((En_ferslv.owner_dest = (char *)FEreqmem((u_i4)Rst4_tag,
		     (u_i4)(FE_MAXNAME+1),TRUE,(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(
					ERx("r_rep_loadld - owner_dest"));
		}
		/*
		** If the server supports delimited identifiers, then
		** ensure that .DELIMID is enabled since we're doing a report
		** based upon a table, and it may require such services.
		*/
		if  (IIUIdlmcase() != UI_UNDEFCASE)
		{
			St_xns_given = TRUE;
		}
	}

	/*
	** set up (and maybe prompt for) declared variables
	*/
	r_dcl_set(TRUE);

        if ((En_qlang == FEDMLSQL) && (IIUIdbdata()->firstAutoCom == FALSE))
	{
		IIUIautocommit(UI_AC_OFF);
	}

	if( r_sc_qry(Ptr_set_top) == FAIL )
	{
		return;
	}
    
	IIUIdlmcase();          /* Hack for bug 96801 ingcbt198 */

	if(!r_chk_dat())		/* check for a valid data relation */
	{
		return;
	}


	if( !r_sendqry())/* send the query to the backend */
	{
		return;
	}
	

	if (!r_att_dflt())	/* set up ATT structures for each */
	{			/* attribute in the data relation */
		return;
	}


	if (En_rtype == RT_DEFAULT)
	{	/* set up the default report */
		if (!St_silent)
		{
			SIprintf(ERget(F_RW0006_Setting_up_default));
			SIflush(stdout);
		}

		/* Set up first column as a sort column */

		if  (!r_m_dsort())
		{
			r_error(0x416,NONFATAL,NULL);
			return;
		}
	}

	if (!r_srt_set())		/* set up the SORT data structures */
	{	/* bad RSORT */
		r_error(0x0C, FATAL, NULL);
	}
	/* set up the BRK data structures
	** for each sort attribute with a
	** RACTION tuple defined for it
	*/
	if( !r_brk_set())	
	{
		return;
	}


	/* 
	** If there is a .PAGEWIDTH command and no -l flag has been specified 
	** on the commandline, then reset En_lmax. (The -l flag overrides the 
	** .PAGEWIDTH command). (#129, #588)
	*/
        if( STlength(En_pg_width) > 0 && !St_lspec)
	{
		/* If the pagewidth value is preceded by
		* a dollar, evaluate it as a parameter
		*/
		r_g_set(En_pg_width);
		if( r_g_skip() == TK_DOLLAR )
		{
			CMnext(Tokchar);
			outname = r_g_name();
			if( (tempname = r_par_get(outname)) == NULL )
			{
				r_error(0x3F0, FATAL, outname, NULL);
			}
			STcopy(tempname, En_pg_width);
		}
		/* need to convert to a number and set it to En_lmax */
		if (CVan(En_pg_width, &En_lmax) != OK)
		{
			/* Unable to convert so ignore input */
			r_error(0x403, NONFATAL, outname, NULL);
		}
	}
		
	if (En_rtype == RT_DEFAULT)
	{	/* set up default report */
		r_m_rprt();
	}
	else
	{
		Tcmd_tag = FEbegintag();
		r_tcmd_set();		/* set up the tcmd data structures */
		FEendtag();
	}


	r_lnk_agg();		/* set up the CLR and OP structures for
				** fast processing of aggregates.
				*/

	return;
}
