/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include	<fe.h>
# include	<adf.h>
# include	<fmt.h>
# include	<ug.h>
# include	<rtype.h>
# include	<rglob.h>
# include	<er.h>
# include	<errw.h>
# include	<st.h>

/**
** Name:	rreset.c -	Report Writer Reset State Module.
**
** Description:
**	Contains the routine that resets the report writer state.
**
**	r_reset()	reset report writer state.
**
** History:	
**		Revision 50.0  86/04/11	 15:30:40  wong
**		Changes for PC:	 Remove references to temporary view.
**
**		8/23/82 (ps)
**		Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

/*{
** Name:	r_reset() -	Reset Report Writer State.
**
** Description:
**   R_RESET - routine to reset the values of variables in
**	the report writer.  This sets the values of variables
**	to defaults (or NULL) before various stages of the
**	report writer are done.
**
** Parameters:
**	type...	Type codes to indicate the nature of the reset.
**		The number of type codes cannot exceed the total number
**		of possible type codes plus one.  The last type code
**		must be RP_RESET_LIST_END.  The codes are defined in
**		rcons.h.
**
** Returns:
**	none.
**
** Side Effects:
**	resets everything.
**
** Called by:
**	many routines in Report, SREPORT and RBF.
**
** Trace Flags:
**	2.0, 2.3.
**
** History:
**	8/23/82 (ps)	written.
**	5/10/83 (nl) -	Put in changes since code was ported
**			to UNIX.
**	5/27/83 (nl) -	Put in more changes since code was ported
**			to UNIX.
**	11/19/83 (gac)	changed St_l_number to St_l_number+1.
**	4/5/84	(gac)	got rid of reference to r_set_time.
**	5/18/84 (gac)	bug 1765 fixed -- default width is 132 for file.
**	12/1/84 (rlm)	ACT reduction - calls to r_ach_del and phase 7.
**	12/20/84 (rlm)	SORT/SRT structure merger - remove Ptr_srt_arr
**	12/2/85 (prs)	Add -h flag.
**	4/11/86 (jhw)	Report writer no longer uses temporary view.
**	08-sep-89 (sylviap)
**		Added St_compatible60.  Resets to false in case 3.
**	1/24/90 (elein)
**		Use En_file_name as array, not char *. Decl changed.
**	3/21/90 (martym)
**		Added En_passflag, and En_gidflag. Also got rid 
**		of St_prompt.
**	17-apr-90 (sylviap)
**		Added En_pg_width. (#129, #588)
**	06-jul-90 (sylviap)
**		Added En_remainder. (#31502)
**	23-aug-90 (sylviap)
**		Added Seq_declare. (#32780)
**	05-nov-90 (sylviap)
**		Added St_pgwdth_cmd. (#33894)
**	01-may-91 (steveh)
**		En_remainder now allocated.  rFm_data attempted to MEfree
**		En_remainder which resulted in free of the static string "".
**		Included st.h as part of fix.
**	02-may-91 (steveh)
**		Previous fix rolled out.  A better check has been added
**		at the time En_remainder is freed.  This will eliminate
**		a potential memory leak in report.
**	5/13/91 (elein) b37507
**		Initialize setup and cleanup top pointers between reports.
**	12-aug-1992 (rdrane)
**		Replace bare constant reset state codes with #define constants.
**		Add explicit return statement.  Eliminate references to
**		r_syserr() and use IIUGerr() directly.
**	30-sep-1992 (rdrane)
**		Replace En_relation with reference to global FE_RSLV_NAME 
**		En_ferslv.name.  Eliminate En_dat_owner.  Declare r_reset() as
**		returning VOID, and declare r_rreset() as being static.
**	27-jan-1993 (rdrane)
**		Fix "hidden" bare constant reset state code in r_rreset()
**		recursive call in RP_RESET_RELMEM4.
**	3-feb-1993 (rdrane)
**		Remove references to Cdat variables, which are no longer used.
**	17-mar-1993 (rdrane)
**		Eliminate references to Bs_buf (eliminated in fix for
**		b49991) and C_buf (just unused).  Reset En_buffer in
**		conjunction with the Rst4_tag (re-)initialization.
**	21-jun-1993 (rdrane)
**		Add support for suppression of initial formfeed.
**	26-feb-2009 (gupsh01)
**		initialize En_utf8_adj_cnt.
*/

static	VOID	r_rreset();

/* VARARGS1 */
VOID
r_reset(t1,t2,t3,t4,t5,t6,t7)
i2	t1,t2,t3,t4,t5,t6,t7;
{
	/* start of routine */

	if (t1 != RP_RESET_LIST_END)
	{
	   r_rreset(t1);
	   if (t2 != RP_RESET_LIST_END)
	   {
	      r_rreset(t2);
	      if (t3 != RP_RESET_LIST_END)
	      {
		 r_rreset(t3);
		 if (t4 != RP_RESET_LIST_END)
		 {
		    r_rreset(t4);
		    if (t5 != RP_RESET_LIST_END)
		    {
		       r_rreset(t5);
		       if (t6 != RP_RESET_LIST_END)
		       {
			  r_rreset(t6);
			  if (t7 != RP_RESET_LIST_END)
			  {
			     IIUGerr(E_RW0046_r_reset_Too_many_arg,
				     UG_ERR_FATAL,0);
			  }
		       }
		    }
		 }
	      }
	   }
	}

	return;
}

static
VOID
r_rreset(type)
i2	type;
{
	switch (type)
	{
	case RP_RESET_PGM:
		/* initialize program files */
		Fl_stdin = stdin;	/* standard input file number */
		Fl_stdout = stdout;	/* standard output file number */
		Fl_input = NULL;	/* Buffer for input text file */
		Fl_rcommand = NULL;	/* Buffer for RCOMMAND file */
		break;

	case RP_RESET_DB:
		/* Start of new database */
		En_database = ERx("");		/* database name */
		En_uflag = NULL;		/* -u flag, if specified */
		En_gidflag = NULL;		/* -G flag, if specified */
		En_passflag = NULL; 		/* if -P flag specified  */
		r_xpipe = NULL;			/* -X pipe, if specified */
		St_db_open = FALSE;		/* true if database open */
		MEfill((u_i2)FE_MAXNAME, '\0', (PTR)Usercode);
		MEfill((u_i2)FE_MAXNAME, '\0', (PTR)Dbacode);
		break;

	case RP_RESET_CALL:
		/* start of new call */
		St_repspec = FALSE;	/* TRUE if -r flag set */
		St_style = RS_NULL;	/* style of report to use, if
					** one is to be set up */
		St_parms = FALSE;	/* TRUE if report has parameters */
		St_hflag_on = FALSE;	/* TRUE if -h flag set */
		St_truncate = FALSE;	/* TRUE if +t; FALSE if -t */
		St_tflag_set = FALSE;	/* TRUE if +t or -t */
		St_compatible = FALSE;	/* TRUE if -5 flag set */
		St_compatible60 = FALSE;/* TRUE if -6 flag set */
		St_silent = FALSE;	/* TRUE if -s flag set */
		En_fspec = NULL;	/* file specified on call to
					** report.  Overrides default */
		St_ing_error = FALSE;	/* True if INGRES error occurs */
		Err_ingres = 0;		/* INGRES or EQUEL error number */
		En_report = ERx("");	/* report name */
		Err_count= 0;		/* count number of non-fatal errors */
		Err_type = NOABORT;	/* type of exit to take */
		En_qmax = MQ_DFLT;	/* maximum query size */
		St_lspec = FALSE;	/* TRUE if -ln specified */
		En_lmax = FO_DFLT;	/* maximum line size */
		En_wmax = MW_DFLT;	/* maximum # lines to wrap */
		St_ispec = FALSE;	/* TRUE if -ifilename specified */
		break;

	case RP_RESET_RELMEM4:
		/* start of new report specification */

		/* release memory */
		if (Rst4_tag <= 0)
		{
			IIUGerr(E_RW0047_level_4_memory_alloca,
				UG_ERR_FATAL,0);
		}
		FEfree (Rst4_tag);
		Seq_declare = 0;	/* counter for declared variables */
		En_rep_owner = ERx(" ");/* user code for report owner */
					/*
					** name and owner information for data
					** relation
					*/
		En_ferslv.name = ERx(" ");
		En_ferslv.name_dest = ERx(" ");
		En_ferslv.owner_dest = ERx(" ");
		En_ferslv.is_nrml = FALSE;
		En_ferslv.owner_spec = FALSE;
		En_ferslv.owner_dlm = FALSE;
		En_ferslv.name_dlm = FALSE;
		En_ferslv.is_syn = FALSE;
		En_file_name[0] = '\0'; /* file to write report to  */
		En_tar_list = NULL;	/* target list if query */
		En_srt_list = NULL;	/* sort key list */
		En_q_qual = NULL;	/* qualification clause if query */
		En_n_attribs = 0;	/* number of attributes in data relation */
		En_n_sorted = 0;	/* number of attributes in sort list */
		En_n_breaks = 0;	/* number of breaks in report */
		En_lc_bottom = 0;	/* number of lines in page footer */
		En_lc_top = 0;		/* number of lines in page header */
		En_scount = 0;		/* number of sort vars in REPORTS */
		En_bcount = 0;		/* number of break vars in REPORTS */
		En_qcount = 0;		/* number of query lines in REPORTS */
		En_acount = 0;		/* number of action lines in REPORTS */
		En_ren = NULL;		/* REN struct for report */
		En_rtype = RT_DEFAULT;	/* either 's' for sreport, or 'f'
					** for RBF, or 'd' for default */
		En_pg_width[0] = '\0';	/* .PAGEWIDTH command (#129, #588) */
		St_pgwdth_cmd = FALSE;	/* no .PAGEWIDTH command */
		En_remainder = ERx(""); /* remainder of the query */
		Oact_type[0] = '\0';		/* current text type (header,
						** footer,or detail */
		Oact_attribute[0] = '\0';	/* current attribute name */
		St_rbe = NULL;			/* current report B/E stack */
		St_pbe = NULL;			/* current page B/E stack */
		St_cbe = NULL;			/* current B/E stack */

		r_ach_del(type);

		/*	Free the ATT list */

		Ptr_att_top = NULL;	/* start of attribute list */

		Ptr_prs_top = NULL;	/* start of array of PRS structures */
		Ptr_sort_top = NULL;	/* start of sort attribute list */
		Ptr_brk_top = NULL;	/* top of breaks stack */
		Ptr_dcl_top = NULL;	/* top of declared variable list */
                Ptr_set_top   = NULL;   /* top of setup commands*/
                Ptr_clean_top = NULL;   /* top of cleanup commands*/
		Ptr_last_brk = NULL;	/* ptr to bottom of break stack */
		Ptr_det_brk = NULL;	/* ptr to detail break struct */
		Ptr_pag_brk = NULL;	/* ptr to page break structure */
		Ptr_par_top = NULL;	/* start of list of parameters */
		Ptr_estk_top = NULL;	/* start of environment stack */
		Ptr_rng_top = NULL;	/* start of range linked list */
		Ptr_rln_top = NULL;	/* top of report line buffers */
		Ptr_pln_top = NULL;	/* top of page break line buffers */
		Ptr_cln_top = NULL;	/* top of centering line buffers */

		/*	Free the B/E stack */

		Ptr_t_rbe = NULL;	/* top of report B/E stack */
		Ptr_t_pbe = NULL;	/* top of page B/E stack */
		Ptr_t_cbe = NULL;	/* top of current B/E stack */

		En_buffer = NULL;	/* buffer to report print line */
		Q_buf = NULL;		/* buffer to hold query */

		/*
		**  Free the TCMD structure.  This must be done in level 4
		**  due to stack pointers into TCMDs.
		*/

		r_rreset(RP_RESET_TCMD);
		break;

	case RP_RESET_RELMEM5:

		/* release memory */
		if (Rst5_tag <= 0)
		{
			IIUGerr(E_RW0048_level_5_memory_alloca,
				UG_ERR_FATAL,0);
		}
		FEfree (Rst5_tag);

		/* start of the run for a specified report */
		St_in_retrieve = FALSE; /* true when in data retrieve loop */
		St_before_report = TRUE;	/* true if no data read */
		St_no_write = TRUE;		/* TRUE until first writing
						** to the report */
		St_do_phead = TRUE;		/* true when page headers
						** are to be done */
		St_tf_set = FALSE;	/* true if any "tformat" are set */
		St_in_pbreak = FALSE;	/* true when printing page breaks */
		St_rep_break = FALSE;	/* true when end of data reached */
		St_h_tname = NULL;	/* name of break type when
					** page break occurs */
		St_h_attribute = NULL;	/* name of attribute for break
					** when page break occurs */
		St_h_written = FALSE;	/* TRUE if Cact_written true
					** for break when page occurs */
		St_err_on = TRUE;	/* TRUE if errors on */
		St_underline = FALSE;	/* TRUE in underlining mode */
		if (!St_ff_set)
		{
			St_ff_on = FFDFLTB;	/* TRUE if formfeed command set */
		}
		if  (!St_no1stff_set)
		{
			St_no1stff_on = FALSE;
		}
		St_brk_ordinal = 0;	/* break order of current break. */
		St_break = NULL;	/* current BRK structure found
					** while reading data.
					*/
		if (!St_pl_set)
		{
			St_p_length = PL_DFLT;	/* current page length */
		}
		St_right_margin = RM_DFLT;	/* current right margin */
		St_left_margin = LM_DFLT;	/* current left margin */
		St_p_rm = RM_DFLT;	/* page break right margin */
		St_r_rm = RM_DFLT;	/* report right margin */
		St_p_lm = LM_DFLT;	/* page break left margin */
		St_r_lm = LM_DFLT;	/* report left margin */
		St_rm_set = FALSE;	/* TRUE when right margin set */
		St_lm_set = FALSE;	/* TRUE when left margin set */
		St_ulchar = UL_DFLT;	/* default character for underlining */
		St_adjusting = FALSE;	/* TRUE when centering, or justifying */
		St_a_tcmd = NULL;	/* current TCMD structure used
					** in justifying */

		St_prline = FALSE;	/* TRUE if last print command
					** was a print line command */

		St_cline = NULL;	/* current line structure for report */
		if (En_need_utf8_adj)
	            En_utf8_adj_cnt = 0;/* reset utf8 adjustment count */
		St_c_lntop = NULL;	/* top fo current LN linked list */
		St_c_estk = NULL;	/* current spot in environment stack */

		St_c_rblk = 0;		/* report level of nesting of .BLK */
		St_c_pblk = 0;		/* page level of nesting of .BLK */
		St_c_cblk = 0;		/* current level of nesting of .BLK */

		St_rwithin = NULL;	/* current .WITHIN command for report */
		St_pwithin = NULL;	/* current .WITHIN command for page */
		St_cwithin = NULL;	/* current .WITHIN command */

		St_rcoms = NULL;	/* report first format command for .WI */
		St_pcoms = NULL;	/* page first format command for .WI */
		St_ccoms = NULL;	/* first format command for .WI */
		Cact_tname = NULL;	/* i4  name of current text type */
		Cact_break = NULL;	/* current BRK structure */
		Cact_written = FALSE;	/* TRUE if a write in
					** current break */
		Cact_type = NULL;	/* current text type (header,footer,or detail */
		Cact_tcmd = NULL;	/* TCMD list */
		Cact_attribute = NULL;	/* current attribute name */
		Cact_rtext = NULL;	/* current RTEXT string */
		Cact_command = NULL;		/* current RTEXT command name */
		Cact_bordinal = NULL;		/* ordinal of current break */
		Err_count= 0;			/* # of non-fatal errors */
		Err_type = NOABORT;		/* type of exit to take */

		break;

	case RP_RESET_OUTPUT_FILE:
		/* at the start of a new file */
		En_rf_unit = NULL;	/* unit number of report file */
		St_rf_open = FALSE;	/* true if report file open */
		St_to_term = FALSE;	/* TRUE if report going to terminal */
		St_p_term = TRUE;	/* TRUE if prompting when going
					** to the terminal */
		St_l_number = 1;	/* current line number */
		St_p_number = 1;	/* current page number */
		St_pos_number = 0;	/* current line position number */
		St_nxt_page = -1;	/* next page number, if set
					** by ".newpage" command. */
		break;

	case RP_RESET_CLEANUP:
		/* post load cleanup */
		r_ach_del(type);
		break;

	case RP_RESET_TCMD:
		/*	Tcmd Cleanup */

		/*
		**  Free the TCMD structure.
		*/

		if (Tcmd_tag >= 0)
		{
			FEfree (Tcmd_tag);
			Tcmd_tag = -1;
		}

	}

	return;
}
