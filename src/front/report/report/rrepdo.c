

/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<nm.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ut.h>
# include	<st.h>
# include	<lo.h>
# include	<adf.h>
# include	<fmt.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<si.h>
# include	<er.h>
# include	<ug.h>
# include	<qr.h>
# include	<errw.h>

static	bool	  r_do_end();
static	i4	  r_scroll();

/**
** Name:	rrepdo.c -	Report Writer Create Report From Data Module.
**
** Description:
**	Contains the routine that retrieves the data and creates the report.
**
**	r_rep_do()	create report from retrieved data.
**
** History:	
**		Revision 50.0  86/04/11	 15:31:00  wong
**		Changes for PC:	 Report Writer now retrieves data directly
**		using parameterized query and does not use temporary view.
**
**		4/3/81 (ps)
**		Initial revision.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	15-feb-96 (harpa06)
**		When using the undocumented flag to suppress the banner to
**		stdout as well as other text, also suppress the message "No
**		rows found. No report will be generated."
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      13-oct-00 (bespi01) bug: #102675 issue #9298736 problem ingcbt#306
**              On Linux report was dieing due to an attempt to close 
**              En_rf_unit twice. Once here and once in r_exit().
**              Changed r_do_end() to set the St_rf_open to false. 
**              This prevents the second close from being attempted.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	r_rep_do() -	Create Report From Retrieved Data.
**
** Description:
**   R_REP_DO - routine to perform the overall logic of the report formatter.
**	All internal structures have been set up by this point, and errors
**	reported.  This runs the EQUEL retrieve loop either with a sort
**	list or without.
**
** Parameters:
**	none.
**
** Returns:
**	TRUE	if no errors in running query.
**	FALSE	if errors in running query.
**
** Side Effects:
**	Sets St_before_report.
**
** Called by:
**	r_main().
**
** Trace Flags:
**	4.0, 4.1.
**
** History:
**	4/3/81 (ps)	written.
**	11/30/82 (ps)	add call to r_nxt_row() and
**			sort in this routine.
**	1/17/83 (ps)	add return TRUE or FALSE on query err.
**	8/24/83 (gac)	bug 1210 -- now doesn't print rpt footer when do data.
**	8/25/83 (gac)	bug 1211 -- now doesn't omit formfeed at end of last
**			page if no page footer.
**	11/22/83 (gac)	bug 1786 -- now doesn't print extra blank line at end
**			of report.

**	1/31/84 (gac)	fixup parameter list here.
**	6/12/84 (gac)	bug 2328 -- ff instead of newlines.
**	6/27/84 (gac)	bug 2208 -- implied newlines (with wraps at
**			right margin) now check for new page.
**	12/2/85 (prs)	Print header and footer for report when
**			no data is found and the -h flag is set.
**	4/11/86 (jhw)	Changes for PC:	 Report writer now retrieves directly
**			and does not use temporary view.
**	17-apr-87 (grant)	changed for native-SQL.
**	26-jun-89 (sylviap)	
**		Added direct printing capabilities.  Uses En_printer, En_copies
**		and St_print.
**	02-aug-89 (sylviap)	
**		Added scrollable output, by calling routines in the fstmlib.
**		Created r_scroll and r_do_end.
**	31-aug-89 (sylviap)	
**		Fixed bug with scrollable output to work with .CLEANUP.
**		Needed to do an IIflush to sync up with BE.
**	11-sep-89 (sylviap)	
**		Moved S_RW0045_No_data_in_table so is compatible with scrollable
**		output.  User can now read error message on a report with no 
**		data.
**	21-sep-89 (sylviap)	
**		Fixed bug so when outputting to a file, 'No rows of data' is
**		  displayed.
**		If output goes to screen and no .PRINT commands are in the 
**		  report, then RW exits forms system.
**	17-oct-89 (sylviap)	
**		Changed the FS calls to IIUF.
**	02-jan-90 (sylviap)	
**		Moved call to r_fix_parl up to r_report, so if any errors 
**		(Err_count) occurred, report execution stops and r_rep_do
**		is NOT called.  This is necessary for batch reports.  (If an
**		undeclared variable appears in the .FOOTER, the report would
**		continues execution).
**	03-jan-90 (sylviap)	
**		Added parameter to IIUFint_Init to have the report name 
**		displayed on the title of the output browser. 
**	09-feb-90 (sylviap)	
**		Added parameter to IIUFdsp_Display to control either 'page'
**		scrolling or 'context' scrolling.
**	01-may-90 (sylviap)	
**		Added parameters to IIUFdsp_Display for popup titles in
**		scrollable output.
**	8/17/90 (elein) 32593
**		Check for warning messages and prompt hit return if
**		output to screen
**	7/17/91 (steveh)
**		Fixed bug 38434: report message flies by too quickly to read.
**	16-dec-1991 (pearl) 40202
**		Initialize new BCB field print_time to FALSE.
**	20-oct-1992 (rdrane)
**		Demote from a .qsc file, and remove the extern declarations
**		since they're now (and probably were) all in the hdr files.
**	18-jan-1993 (rdrane)
**		Uncomment scrollable output function declarations so the
**		ANSI compiler doesn't complain.
**	8-mar-1993 (rogerl)
**		Add parm to IIUFdsp_Display to avoid turning off interrupts
**	19-mar-1993 (rdrane)
**		Declare r_do_end() and r_scroll as being static to this module.
**	23-apr-1993 (rdrane)
**		Remove calls to FEendforms().  This is now done as part of
**		r_exit() processing.
**	12-jul-1993 (rdrane)
**		Rework the initiation/termination of the Forms System so that
**		we can better control the appearance of our messages.
**	01-nov-93 (connie) Bug #47423
**		Save the report file location since r_rep_set calls NMloc
**		which stores the name in a static buffer which will
**		be overwritten by another call to NMloc
**	15-nov-93 (connie) Bug #47423
**		Moved the above fix to r_rep_set().
**	07-apr-95 (kch)
**		Increase the number of rows passed to IIUFmro_MoreOutput()
**		by one (En->mx_rows-2 rather than En_bcb->mx_rows-3) in
**		order to show the bottom detail line of a page during the
**		intial paging forward in a report with no .FOOTER section.
**		This change fixes bug 45106.
**	15-feb-96 (harpa06)
**		When using the undocumented flag to suppress the banner to
**		stdout as well as other text, also suppress the message "No
**		rows found. No report will be generated."
**	17-Jun-2004 (schka24)
*	    Safe env variable handling.
*/

bool
r_rep_do()
{
	char	  *_Retrieving	= ERget(F_RW0003_Retrieving_data);
	LOCATION  loc;		/* file where the report will be written */
        char      title[80+1];      /* the help title buffer */
	char      help_scr[80+1];   /* the help screen buffer */
        char      pr_title[80+1];   /* print report title */
	char      file_title[80+1]; /* file report title */


	/*	
	** The following is for scrollable output.
	*/
FUNC_EXTERN	BCB	*IIUFint_Init();
FUNC_EXTERN	bool	IIUFmro_MoreOutput();
		bool	scrl_fl;


	r_rep_set(&loc);

	if (St_ing_error != 0)
		return FALSE;

	if (!St_silent)
	{	/* give message while waiting for retrieve */
		SIprintf(_Retrieving);
		SIflush(stdout);
	}
	if (St_to_term)
	{
		if( Warning_count != 0 )
		{
			r_prompt(ERget(S_RW0025_Hit_RETURN_when_done), FALSE);
		}

		/* 
		** If output is the terminal, then initialize forms to do
		** scrollable output.
		*/
		if  (r_init_frms() != OK)
		{
			return FALSE;						    
		}
		/* set up the bcb (buffer control block) */
		STprintf (title, ERget(S_RW1400_report_title), En_report);
		En_bcb = IIUFint_Init((char *)NULL, title);
		if (En_bcb == NULL)	/* IIUFint_Init failed */
		{
			return FALSE;  
		}
		/* initialize bcb */
		En_bcb->nxrec = r_scroll;  /* routine to handle scrolling */
		En_bcb->req_begin = TRUE;
		En_bcb->req_complete = FALSE;
		En_bcb->eor_warning = FALSE;
		LOdelete(&En_bcb->bfloc);  /* check directory is writable */
		En_bcb->nrecs = 0;
		En_bcb->mxcol = 0;
		En_bcb->print_time = FALSE;
		if (En_bcb->rd_ahead)
		{
			MEfree(En_bcb->rd_ahead);
			En_bcb->rd_ahead = NULL;
		}
		/* get first screen to display */
		if (!IIUFmro_MoreOutput(En_bcb, En_bcb->mx_rows-2, NULL))
		{
			r_end_frms();
			/* suppress message if undocumented flag is used */
			if (!St_copyright)
			{
				SIprintf(ERget(S_RW0045_No_data_in_table));
			}
			SIflush(stdout);
			return FALSE;  
		}
		STcopy (ERx("rwoutput.hlp"), help_scr);
		STcopy (ERget(S_RW13F1_RW_output_helpfile), title);
		STcopy (ERget(S_RW1408_print_report_title), pr_title);
		STcopy (ERget(S_RW1409_file_report_title), file_title);

		/* If there is nothing to display, then just exit */
		if (En_bcb->rows_added != 0) 
		{
			/*
			** If the pagelength is equal to screen display size
			** (4 less lines for the report title, status line, 
			** solid line and menu line in the scrollable output)
			** then do NOT scroll keeping the current line visible.
			** This is the default behavior.  Want to scroll a 
			** full page at a time.
			*/
			if (St_p_length == (En_lines-4))
				/* 
				** page length is the same as window size, so 
				** want page scrolling 
				*/
				scrl_fl = FALSE;
			else
				/*
				** different page size so want to scroll, 
				** leaving the one line for context.
				*/
				scrl_fl = TRUE;
			if (!IIUFdsp_Display(En_bcb, NULL, help_scr, title, 
				scrl_fl, pr_title, file_title, FALSE))
			{
				return FALSE;  
			}			
		}
		IIUFdone (En_bcb);
	}
	else 
	{ 
		while (r_getrow() == OK)
		{
			r_nxt_row();	/* process next row */
		}
		return (r_do_end(&loc));
	} 
	return TRUE;
}


/**
** Name:	r_do_end
**
** Description:
**	Does last page headers and footers.  Prints out report if necessary.
**
**	Input:
**		LOCATION     *loc	location of the report file
**
**	Output: bool	     - TRUE if no errors occurred
**
** History:	
**	02-aug-89 (sylviap)
**		Initial version.
**	13-May-97 (kitch01)
**		Bug 82041. Windows95 only. As print.exe is not provided on 
**		Win95 we use COPY. However this requires a formfeed character 
**		else the printer hangs on user intervention. So we add one 
**		if we're on Win95 and +/-B flag is not set. By doing this 
**		here we ensure that output captured to a file is also 
**		printable on Win95, as it too requires a formfeed.
**	03-sep-97 (mcgem01)
**		Use of the copy command on both NT and '95 means that we
**		should perform the above operation for both.
**	29-oct-98 (kitch01)
**		Remove the 13-may-97 change completely. Testing shows that it is
**		now not required. Bug 93979.
**       2-Dec-2010 (hanal04) Bug 124652
**              If no rows are printed and the empty report has not been forced
**              to the printer we need to remove the ra*.tmp file we created
**              in II_TEMPORARY.
*/


static
bool
r_do_end(loc)
LOCATION  *loc;		/* file where the report will be written */
{

	/* sync up with BE */
	IIflush((char *)0, 0);

	if (St_ing_error != 0)
		return FALSE;

	if (!St_in_retrieve && St_hflag_on)
	{	/* No data in report.  Print out report header if -h flag on */
		St_break = Ptr_brk_top;
		Cact_attribute = NAM_REPORT;
		Cact_tname = NAM_HEADER;
		r_do_header(St_break);
	}

	/* process the end of the report */

	St_rep_break = TRUE;
	if (St_in_retrieve || St_hflag_on)
	{
		St_break = Ptr_brk_top;
		Cact_attribute = NAM_REPORT;
		Cact_tname = NAM_FOOTER;

		r_do_footer(St_break);

		St_in_retrieve = FALSE;

		if (St_c_lntop->ln_started)
		{
			r_x_newline(1);
		}

		St_do_phead = FALSE;	/* don't do the last page header */

		if (St_p_length > 0 && St_l_number > 1)
		{
			r_x_npage(1, B_RELATIVE);
		}

		/*  Send report to the print if the -o (output) flag was set */
		if (St_print)
		{
			bool   delete_file;
			char   *u_printer;
			char   *buf;
			char   *jobname;
			STATUS stat;
        		char   user_print[MAX_LOC];      /* stores printer 
							  ** command 
							  */
			if ((SIclose (En_rf_unit)) != OK)
			{
				LOtos (loc, &buf);
				IIUGerr(E_RW13EC_Cannot_close_unit, 
					UG_ERR_ERROR, 1, buf);
			}
			else
			{
				St_rf_open = 0;
				if (En_fspec != NULL)
				{
					/* 
					** if -f flag was also specified, don't 
					** delete the report 
					*/
					delete_file = FALSE;
					/* 
					** Job name will be filename by default 
					*/
					jobname = NULL;
				}
				else
				{
					delete_file = TRUE;
					/* 
					** No filename specified, so job name 
					** will be report name
					*/
					jobname = En_report;
				}
               			NMgtAt(ERx("ING_PRINT"), &u_printer);
              			if (u_printer != (char *)NULL && 
					*u_printer != '\0') 
				{
					STlcopy(u_printer, user_print, sizeof(user_print)-1);
				}
				else
				{
					user_print[0] = '\0';
				}
				stat = UTprint (user_print, loc,
					delete_file, En_copies, 
					jobname, En_printer);
				if (stat != OK)
				{
					LOtos (loc, &buf);

					IIUGerr(E_RW13ED_Error_printing, 
						UG_ERR_ERROR, 1, buf);
					SIflush(stdout);

					/* (38434) return status to caller. */
					return(FALSE);
				}
			}
		}
	}
	else
	{
		/* 
		** if scrollable output, just return FALSE, so above routine
		** can first turn off forms system  before printing out error.
		*/
		if (St_to_term)
			return (FALSE);
		/* suppress message if undocumented flag is given */
		else if (!St_copyright)
                {
			SIprintf(ERget(S_RW0045_No_data_in_table));
                        if (St_rf_open && loc)
                        {
                            /* No rows and not forced to printer, delete
                            ** the raXXXXXXXXXXXXX.tmp file to clean up.
                            */
                            SIclose (En_rf_unit);
                            LOdelete(loc);
                            St_rf_open = FALSE; /* so r_exit() knows */
                        }
                }
	}
	SIflush(stdout);
	return(TRUE);
}

/**
** Name:	r_scroll
**
** Description:
**	Routine to handle scrollable output.  Includes a QRB (query block) as 
**	a parameter (even though it it always passed in as NULL and never used)
**	to remain compatible with the fstm (full screen terminal monitor)
**	output routines.  Other parameters, bfr and bfrlen are also not used, 
**	but necessary for compatibility.
**
**	R_SCROLL returns the length of the last line of the report inserted into
**	the output buffer.  R_SCROLL works with one row/tuple at a time which
**	may translate to one or more actual lines in the report.  En_rows_added
**	corresponds to the number of actual report rows.
**
**	Input:
**		BCB     *bcb		buffer control block
**		char    *bfr		stores a line of the report (not used)
**		i4     bfrlen		length of bfr (not used)
**		PTR     qrb		query control block (not used)
**		i4	*rows_added	number of rows added to report
**
**	Output: i4	>= 0 	-length of last line added to report
**		 	 < 0	-no more rows to add, report complete
**
** History:	
**	02-aug-89 (sylviap)
**		Initial version.
**	17-mar-1993 (rdrane)
**		Re-work this routine so that En_buffer will accomodate
**		a "double line" - text and its underlining characters
**		(when ulc == '_').  This addresses b49991.  Eliminated
**		some unused/redundant local variables.  Allocate En_buffer
**		with Rst4_tag so that r_reset() can reinitialize it for
**		each report.
*/

static
i4
r_scroll(bcb, bfr, bfrlen, qrb, rows_added)
BCB     *bcb;
char    *bfr;
i4      bfrlen;
PTR     qrb;
i4	*rows_added;
{
	i4	maxrow;

        if (bcb->req_complete)
	{
		return(-1);
	}

	if  (En_buffer == NULL)
	{
		/*
		** Create the report buffer to accomodate
		** the max line text, a carriage return,
		** and max line underlining
		*/
		maxrow = max(En_lmax,St_right_margin);
		if  ((En_buffer = (char *)FEreqmem (Rst4_tag,
					(u_i4)((maxrow * 2) + 2),
					TRUE,(STATUS *)NULL)) == NULL)
		{
			IIUGbmaBadMemoryAllocation(ERx("r_rep_do - En_buffer"));
		}
	}
	En_rows_added = 0;

	if (r_getrow() == OK)
	{
		r_nxt_row();    /* process next row */
	}
	else
	{			/* finish up footers */
		if (!r_do_end(NULL))
		{
			*rows_added = -1;
			return (-1);
		}
		En_bcb->req_complete = TRUE;
	}

	*rows_added = En_rows_added;
	return (STlength(En_buffer));
}

