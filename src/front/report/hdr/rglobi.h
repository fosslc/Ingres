/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
**	HISTORY:
**	6/19/89 (elein) garnet
**		Added external declaration of setup/cleanup top list ptrs.
**      26-jun-89 (sylviap)
**              Added En_printer, En_copies, and St_print for sending the
**              report directly to the printer.
**      28-jul-89 (sylviap)
**              Added En_buffer, En_bcb and En_rows_added for scrollable output.
**		Changed #include from si.h to fstm.h.  fstm.h already includes
**		si.h.
**	9/5/89 (elein)
**		Add St_cache--boolean. Don't look ahead for some commands
**      01-sep-89 (sylviap)
**		Added St_compatible60 for the -6 flag.
**      28-sep-89 (sylviap)
**		Added En_ctrl for control sequences (Q format).
**	10/27/89 (elein)
**		Added St_setup_continue [on error] and
**		St_in_setup 
**      04-dec-89 (sylviap)
**              Added St_copyright for copyright suppression and St_batch for 
**		batch reporting.
**	12/14/89 (martym) (garnet)
**		Added LabelInfo for label style reports.
**	1/24/90 (elein)
**		Change to decl of En_file_name & En_rcommands to ensure that 
**		LOfroms sequence has correct scope.
**      06-feb-90 (sylviap)
**		En_copies is initialized to 0 instead of 1.
**	3/26/90 (martym)
**		Took out St_prompt. Added En_passflag, and En_gidflag to 
**		support flags -Ggroupid, -P (password).
**      27-mar-90 (sylviap)
**              Added St_wrap for #724.
**      02-apr-90 (sylviap)
**              Deleted St_wrap.
**      17-apr-90 (sylviap)
**		Added En_pg_width. (#129, #588)
**      24-apr-90 (sylviap)
**		Added St_one_name (US#347)
**      25-jul-90 (sylviap)
**		Added Ptr_tbl for joindef reports.
**      17-aug-90 (sylviap)
**              Added En_lmax_orig for label reports.  Needs to know what the
**              En_lmax was originally set, but En_lmax changes while setting
**              up the report.
**      8/20/90 (elein) 32395
**              Added Warning_count for initialization errors.  These really
**              are warnings and are only noted so that we can allow hit return
**              for them to be viewed.
**      23-aug-90 (sylviap)
**              Added Seq_declare for label reports.  (#32780)
**      31-oct-90 (sylviap)
**              Added St_pgwdth_cmd.  (#33894)
**	26-aug-1992 (rdrane)
**		Add ing_deadlock global.  Add the function declarations.
**		Remove obsolete CFEVMSTAT define.
**	16-sep-1992 (rdrane)
**		Add St_xns_given flag and En_ferslv.
**		Removed En_relation and En_dat_owner, since these are now part
**		of En_ferslv's FE_RSLV_NAME structure.  Add IISRxs_XNSSet(),
**		r_acc_make(), r_reset(), and s_reset() function declarations.
**		Add r_starg_order(), r_gtarg_order(), and r_ctarg_order()
**		function declrations (Sort ordinal matching routines - bugs
**		35335, 41861, and 41871).
**	25-nov-92 (rdrane)
**		Renamed references to .ANSI92 to .DELIMID as per the LRC.
**	21-dec-1992 (rdrane)
**		Add declaration of r_m_dsort() since it now returns a bool.
**	3-feb-1993 (rdrane)
**		Remove references to the Cdat variables - no longer
**		referenced.  Add declaration of new routine r_gord_targ().
**	23-feb-1993 (rdrane)
**		Remove references to the En_ctrl (46393 and 46713) - no longer
**		used in a global context.  Explicitly declare r_clear_line
**		and r_ln_clr() as returning VOID.  Add declarations of
**		IIRWmct_make_ctrl() and IIRWpct_put_ctrl().
**	17-mar-1993 (rdrane)
**		Eliminate references to Bs_buf (eliminated in fix for
**		b49991) and C_buf (just unused).
**	05-Apr-93 (fredb)
**		Removed "St_txt_open" as it is preferred to use "Fl_input".
**	21-jun-1993 (rdrane)
**		Add St_no1stff_on and St_no1stff_set support suppression of
**		the initial formfeed when formfeeds are enabled.
**	12-jul-1993 (rdrane)
**		Add declrations of r_init_frms() and r_end_frms.
**      28-apr-2004 (srisu02)
                Defined num_oflag for new flag "numeric_overflow=ignore|fail|warn"
**	09-Apr-2005 (lakvi01)
**	    Corrected prototype of r_ges_next().
**	14-Nov-2008 (gupsh01)
**		Added support for -noutf8align flag for report writer.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes to eliminate gcc 4.3 warnings.
*/

# include	 <fstm.h>

/*
**	Global Variables in Report Formatter
**	------ --------- -- ------ ---------
*/


/*
**	En - contains overall report environment information.
**		Many of these variables come from the RENVIR relation.
*/

	GLOBALDEF char	*En_database = "";	/* database name */
	GLOBALDEF char	*En_report = "";	/* report name */
	GLOBALDEF char	*En_rep_owner = " ";	/* user code for report owner */
	GLOBALDEF FE_RSLV_NAME En_ferslv =
		{
			NULL,
			NULL,
			NULL,
			FALSE,
			FALSE,
			FALSE,
			FALSE,
			FALSE
		};
						/*
						** FE_RSLV_NAME structure to
						** input and resolved table
						** name and owner name for
						** data relation
						*/
	GLOBALDEF char	*En_pre_tar = {0};	/* 'unique' or 'distinct/all'
						** before the target list */
	GLOBALDEF char	*En_tar_list = {0};	/* target list if query */
	GLOBALDEF char	*En_q_qual = {0};	/* qualification clause if query */
	GLOBALDEF char	*En_srt_list = {0};	/* list of sort keys, if specified */
	GLOBALDEF char	*En_sql_from = {0};	/* select from list if sql query */
	GLOBALDEF char	*En_remainder = {0};	/* replaces qual and sort.
						** contains remainder of query
						** after target or from list */
	GLOBALDEF char	En_file_name[MAX_LOC+1];/* file to write report to  */
	GLOBALDEF char	*En_fspec = {0};	/* file specified on call to
						** report.  Overrides default */
	GLOBALDEF char	*En_printer = {0};	/* printer to write report to  */
	GLOBALDEF i4	En_copies = {0};	/* number of copies to print */
	GLOBALDEF i4	En_num_oflag = NUM_O_FAIL;	/* "-numeric_overflow" flag  */
	GLOBALDEF char	*En_uflag = NULL;	/* -u flag, if specified */
	GLOBALDEF char	*En_gidflag = NULL;	/* -G flag, if specified */
	GLOBALDEF char	*En_passflag = NULL;	/* -P flag, if specified */
	GLOBALDEF ATTRIB En_n_attribs = {0};	/* number of attributes in data relation */
	GLOBALDEF ATTRIB En_n_sorted = {0};	/* number of attributes in sort list */
	GLOBALDEF ATTRIB En_n_breaks = {0};	/* number of breaks in report */
	GLOBALDEF FILE	*En_rf_unit = {0};	/* unit number of report file */
	GLOBALDEF REN	*En_ren = {0};		/* REN structure for report */
	GLOBALDEF i4	En_lc_bottom = {0};	/* number of lines in page footer */
	GLOBALDEF i4	En_lc_top = {0};	/* number of lines in page header */
	GLOBALDEF i4	En_qmax = MQ_DFLT;	/* maximum query size */
	GLOBALDEF i4	En_lmax = ML_DFLT;	/* maximum line size */
	GLOBALDEF i4	En_lmax_orig = ML_DFLT;	/* maximum line size */
	GLOBALDEF i4	En_wmax = MW_DFLT;	/* maximum # lines to wrap */
	GLOBALDEF i4	En_scount = 0;		/* number of sort vars in REPORTS */
	GLOBALDEF i4	En_bcount = 0;		/* number of break vars in REPORTS */
	GLOBALDEF i4	En_qcount = 0;		/* number of query lines in REPORTS */
	GLOBALDEF i4	En_acount = 0;		/* number of action lines in REPORTS */
	GLOBALDEF i2	En_program = {0};	/* Program which is running */
	GLOBALDEF char	En_rtype = RT_DEFAULT;	/* either 's' for sreport, or 'f'
						** for RBF, or 'd' for default */
	GLOBALDEF i4	En_cols = {0};		/* # of columns on terminal */
	GLOBALDEF i4	En_lines = {0};		/* # of lines on terminal */
	GLOBALDEF char	En_termbuf[80];		/* buffer for TDtermsize */
  	GLOBALDEF char	*En_initstr = {0};	/* initialization string for terminal */
	GLOBALDEF i4	En_amax = ACH_DFLT;	/* maximum action cache size */
	GLOBALDEF i4	En_qlang = 0;		/* report query language */
	GLOBALDEF OOID	En_rid = 0;		/* object-oriented id for report */
	/* For scrollable output */
  	GLOBALDEF char	*En_buffer = {0};	/* buffer for one line of the 
						** report */
  	GLOBALDEF i4	En_rows_added = 0;	/* number of rows added to 
						** scrolling buffer */
  	GLOBALDEF BCB	*En_bcb = {0};		/* buffer control block for 
						** scrolling */
	GLOBALDEF i4    En_SrcTyp;              /* the type of the data
						** source for the report */
	GLOBALDEF LABST_INFO LabelInfo = {0}; 	/* Contains the dimensions
						** of a labels style report. */
	GLOBALDEF char	En_pg_width[MAX_LOC+1];/* page width  */
	GLOBALDEF bool    En_need_utf8_adj = FALSE;/* Is UTF8 adjustment needed */
	GLOBALDEF bool    En_need_utf8_adj_frs = FALSE;/* Is UTF8 adjustment needed 
						       ** for reports printed to a file
						       ** from within the forms system 
						       */
	GLOBALDEF i4      En_utf8_adj_cnt = {0};   /* num of spaces for utf8 adjustment */
	GLOBALDEF bool    En_utf8_set_in_within = FALSE; /* UTF8 adj done in r_x_within*/
	GLOBALDEF i4	En_utf8_lmax = { 2 * ML_DFLT };	/* Maximum line buffer in UTF8 install */


/*
**	St  - contains the current status flags during the
**		running of the report formatter.  Whereas the
**		EN variables represent global constants, the
**		ST variables represent current status information
**		in the running of the report.
*/

	GLOBALDEF bool	St_db_open = FALSE;	/* true if database open */
	GLOBALDEF bool	St_in_retrieve = FALSE; /* true when in data retrieve loop */
	GLOBALDEF bool	St_before_report = TRUE;/* true if no data read */
	GLOBALDEF bool	St_no_write = TRUE;	/* TRUE until first writing
						** to the report */
	GLOBALDEF bool	St_do_phead = TRUE;	/* true when page headers
						** are to be done */
	GLOBALDEF bool	St_rf_open = FALSE;	/* true if report file open */
	GLOBALDEF bool	St_tf_set = FALSE;	/* true if any "tformat" are set */
	GLOBALDEF bool	St_in_pbreak = FALSE;	/* true when printing page breaks */
	GLOBALDEF bool	St_rep_break = FALSE;	/* true when end of data reached */
	GLOBALDEF char	*St_h_tname = {0};	/* name of header/footer for
						** break when in page break */
	GLOBALDEF char	*St_h_attribute = {0};	/* name of break att for break
						** when in page break */
	GLOBALDEF bool	St_h_written = FALSE;	/* TRUE if Cact_written true in
						** break when in page break */
	GLOBALDEF bool	St_lspec = FALSE;	/* TRUE if -ln specified */
	GLOBALDEF bool	St_parms = FALSE;	/* TRUE if report has params */
	GLOBALDEF bool	St_repspec = FALSE;	/* TRUE if -r flag set */
	GLOBALDEF bool	St_silent = FALSE;	/* TRUE if -s flag set */
	GLOBALDEF bool	St_truncate = FALSE;	/* TRUE if +t flag set, FALSE if -t flag set */
	GLOBALDEF bool	St_tflag_set = FALSE;	/* TRUE if -t or +t flag set */
	GLOBALDEF bool	St_ispec = FALSE;	/* TRUE if -i flag set; if so,
						** En_sfile (in SREPORT) is 
						** set to file name */
	GLOBALDEF bool	St_ing_error = FALSE;	/* True if INGRES error occurs */
	GLOBALDEF bool	St_hflag_on = FALSE;	/* TRUE if -h flag set */

	GLOBALDEF bool	St_err_on = TRUE;	/* TRUE if errors on */
	GLOBALDEF bool	St_to_term = FALSE;	/* TRUE if report going to terminal */
	GLOBALDEF bool	St_p_term = TRUE;	/* TRUE if prompting when going
						** to the terminal */
	GLOBALDEF bool	St_underline = FALSE;	/* TRUE in underlining mode */
	GLOBALDEF bool	St_called = FALSE;	/* TRUE if report called from
						** program */
	GLOBALDEF bool	St_ff_on = FFDFLTB;	/* TRUE if formfeed command set */
	GLOBALDEF bool	St_ff_set = FALSE;	/* TRUE if formfeed set from command line */
	GLOBALDEF bool	St_no1stff_on = FALSE;	/* TRUE if no initial formfeed command set */
	GLOBALDEF bool	St_no1stff_set = FALSE;	/* TRUE if no initial formfeed set from command line */
	GLOBALDEF ATTRIB St_brk_ordinal = {0};	/* break order of current break. */
	GLOBALDEF BRK	*St_break = {0};	/* current BRK structure found
						** while reading data.
						*/
	GLOBALDEF i2	St_style = RS_NULL;	/* style of report to use, if
						** one is to be set up */

	GLOBALDEF i4	St_p_length = PL_DFLT;	/* current page length */
	GLOBALDEF bool	St_pl_set = FALSE;	/* TRUE if page length set from command line */
	GLOBALDEF i4	St_right_margin = RM_DFLT;	/* current right margin */
	GLOBALDEF i4	St_left_margin = LM_DFLT;	/* current left margin */
	GLOBALDEF i4	St_p_rm = RM_DFLT;	/* page break right margin */
	GLOBALDEF i4	St_r_rm = RM_DFLT;	/* report right margin */
	GLOBALDEF i4	St_p_lm = LM_DFLT;	/* page break left margin */
	GLOBALDEF i4	St_r_lm = LM_DFLT;	/* report left margin */
	GLOBALDEF bool	St_rm_set = FALSE;	/* TRUE when right margin set */
	GLOBALDEF bool	St_lm_set = FALSE;	/* TRUE when left margin set */
	GLOBALDEF char	St_ulchar = UL_DFLT;	/* default character for underlining */
	GLOBALDEF i4	St_l_number = 1;	/* current line number */
	GLOBALDEF i4	St_top_l_number = 0;	/* line number of block top */
	GLOBALDEF i4	St_p_number = 1;	/* current page number */
	GLOBALDEF i4	St_pos_number = 0;	/* current line position number */
	GLOBALDEF DB_DATA_VALUE	St_cdate = {0}; /* current_date */
	GLOBALDEF DB_DATA_VALUE	St_ctime = {0}; /* current_time */
	GLOBALDEF DB_DATA_VALUE	St_cday = {0}; /* current_day */
	GLOBALDEF i4	St_nxt_page = -1;	/* next page number, if set
						** by ".newpage" command.
						*/

	GLOBALDEF bool	St_adjusting = FALSE;	/* TRUE when centering, or justifying */
	GLOBALDEF TCMD	*St_a_tcmd = {0};	/* current TCMD structure used
						** in justifying */

	GLOBALDEF bool	St_prline = FALSE;	/* TRUE if last print command
						** was a print line command */

	GLOBALDEF LN	*St_cline = {0};	/* current line structure for report */
	GLOBALDEF LN	*St_c_lntop = {0};	/* top fo current LN linked list */
	GLOBALDEF ESTK	*St_c_estk = {0};	/* current spot in environment stack */

	GLOBALDEF STK	*St_rbe = {0};		/* current report B/E stack */
	GLOBALDEF STK	*St_pbe = {0};		/* current page B/E stack */
	GLOBALDEF STK	*St_cbe = {0};		/* current B/E stack */

	GLOBALDEF i4	St_c_rblk = {0};	/* report level of nesting of .BLK */
	GLOBALDEF i4	St_c_pblk = {0};	/* page level of nesting of .BLK */
	GLOBALDEF i4	St_c_cblk = {0};	/* current level of nesting of .BLK */

	GLOBALDEF TCMD	*St_rwithin = {0};	/* current .WITHIN command for report */
	GLOBALDEF TCMD	*St_pwithin = {0};	/* current .WITHIN command for page */
	GLOBALDEF TCMD	*St_cwithin = {0};	/* current .WITHIN command */

	GLOBALDEF TCMD	*St_rcoms = {0};	/* report first format command for .WI */
	GLOBALDEF TCMD	*St_pcoms = {0};	/* page first format command for .WI */
	GLOBALDEF TCMD	*St_ccoms = {0};	/* first format command for .WI */
	GLOBALDEF bool	St_compatible = FALSE;	/* TRUE if -5 flag set */
	GLOBALDEF bool	St_compatible60 = FALSE;	
						/* TRUE if -6 flag set */
	GLOBALDEF bool	St_print = FALSE;	/* TRUE if -o flag set 
						** send report to the printer*/
	GLOBALDEF bool	St_cache = TRUE;	/* TRUE if action look ahead is ok
						** set to F and set back to T in ractset
						** checked in rgeskip()*/
	GLOBALDEF bool	St_setup_continue = FALSE;
	GLOBALDEF bool	St_in_setup = FALSE;	/* TRUE if in setup or cleanup sections
						*/
	GLOBALDEF bool  St_copyright = FALSE;   /* TRUE if copyright banner
						** should be suppressed */
	GLOBALDEF bool  St_batch = FALSE;       /* TRUE if RW is being run
						** in the background, so
						** suppress all prompts */
	GLOBALDEF bool  St_one_name = FALSE;    /* TRUE if has read in one
						** .NAME command */
	GLOBALDEF bool  St_pgwdth_cmd = FALSE;   /* TRUE if report has a 
						** .PAGEWIDTH command */
	GLOBALDEF bool	St_xns_given = FALSE;	/* TRUE if report has an
						** .DELIMID (eXpanded Name
						** Space) Command
						*/

/*
**	Ptr - contains pointers to the start of many of
**		the dynamically allocated linked lists and arrays used by
**		the report formatter.
*/

	GLOBALDEF ATT	*Ptr_att_top = {0};	/* start of attribute list */
	GLOBALDEF i2	Att_tag = -1;		/* Memory Tag of ATT list */
	GLOBALDEF i2	Fmt_tag = -1;		/* Memory Tag of Fmtstrs */

	GLOBALDEF PRS	*Ptr_prs_top = {0};	/* start of array of PRS structures */
	GLOBALDEF SORT	*Ptr_sort_top = {0};	/* start of sort attribute list */
	GLOBALDEF char	**Ptr_break_top = {0};	/* start of break attribute list */
	GLOBALDEF DCL	*Ptr_dcl_top = {0};	/* start of declared variable list */

	GLOBALDEF BRK	*Ptr_brk_top = {0};	/* top of breaks stack */
	GLOBALDEF BRK	*Ptr_last_brk = {0};	/* ptr to bottom of break stack */
	GLOBALDEF BRK	*Ptr_det_brk = {0};	/* ptr to detail break struct */
	GLOBALDEF BRK	*Ptr_pag_brk = {0};	/* ptr to page break structure */

	GLOBALDEF LAC	*Ptr_lac_top = {0};	/* top of lac list */
	GLOBALDEF PAR	*Ptr_par_top = {0};	/* start of list of parameters */
	GLOBALDEF ESTK	*Ptr_estk_top = {0};	/* start of environment stack */
	GLOBALDEF STK	*Ptr_stk_top = {0};	/* top of BEGIN/END stack */
	GLOBALDEF RNG	*Ptr_rng_top = {0};	/* start of range linked list */

	GLOBALDEF LN	*Ptr_rln_top = {0};	/* top of report line buffers */
	GLOBALDEF LN	*Ptr_pln_top = {0};	/* top of page break line buffers */
	GLOBALDEF LN	*Ptr_cln_top = {0};	/* top of centering line buffers */

	GLOBALDEF QUR	*Ptr_qur_arr = {0};	/* points to array of QUR structs */
	GLOBALDEF SC	*Ptr_set_top = {0};	/* points to list of setup structs */
	GLOBALDEF SC	*Ptr_clean_top = {0};	/* points to list of cleanup structs */

	GLOBALDEF STK	*Ptr_t_rbe = {0};	/* top of report B/E stack */
	GLOBALDEF STK	*Ptr_t_pbe = {0};	/* top of page B/E stack */
	GLOBALDEF STK	*Ptr_t_cbe = {0};	/* top of current B/E stack */
	GLOBALDEF i2	Cbe_tag = -1;		/* Stack element tag */

	GLOBALDEF i4	Sequence_num = 0;	/* current sequence number for action */
	GLOBALDEF TBLPTR *Ptr_tbl = {0};	/* start of table list */
						/* Used for joindef reports */




/*
**	Cact - flags etc which correspond to the current text action
**		being processed.
*/

	GLOBALDEF char	*Cact_tname = {0};	/* i4  name of current text type */
	GLOBALDEF BRK	*Cact_break = {0};	/* current BRK structure */
	GLOBALDEF bool	Cact_written = FALSE;	/* TRUE if a write made in
						** current break */
	GLOBALDEF char	*Cact_type = {0};	/* current text type (header,footer,or detail */
	GLOBALDEF char	*Cact_attribute = {0};	/* current attribute name */
	GLOBALDEF TCMD	*Cact_tcmd = {0};	/* current TCMD structure */
	GLOBALDEF i2	Tcmd_tag = -1;		/* TCMD tag id */
	GLOBALDEF char	*Cact_rtext = {0};	/* current RTEXT string */
	GLOBALDEF char	*Cact_command = {0};	/* current RTEXT command name */
	GLOBALDEF ATTRIB *Cact_bordinal = {0};	/* ordinal of current break */
	GLOBALDEF char	Oact_type[MAXRNAME+1] = {0};	/* previous text type (header,footer,or detail */
	GLOBALDEF char	Oact_attribute[FE_MAXNAME+1] = {0};	/* previous attribute name */



/*
**	Token - Variables used for Token generation.
*/

	GLOBALDEF char	*Tokchar = {0}; /* ptr to current postion in token line */


/*
**	Error Processing
*/

	GLOBALDEF i2	Err_count= 0;		/* count of number of non-fatal errors */
	GLOBALDEF ERRNUM Err_ingres = {0};	/* INGRES or EQUEL error number */
	GLOBALDEF i2	Warning_count= 0;	/* count of number of warning errors */

/*
**	Debugging Trace Information
*/

	GLOBALDEF i2	T_flags[T_SIZE] = {0};	/* table of trace flags */


/*
**	Global storage spaces.
*/

	GLOBALDEF char	Usercode[FE_MAXNAME+1] = {0}; /* temp storage of user code */
	GLOBALDEF char	Dbacode[FE_MAXNAME+1] = {0};	/* storage of dba code */

/*
**	Standard input and output file buffers
*/

	GLOBALDEF FILE	*Fl_stdin = 0;	/* standard input file number */
	GLOBALDEF FILE	*Fl_stdout = 0; /* standard output file number */



/*
**	Miscellaneous.
*/

	GLOBALDEF char	*Q_buf = {0};		/* buffer to hold query */
	GLOBALDEF i4	Lj_buf[2] = {0};	/* i4  jump buffer */
	GLOBALDEF i4	Err_type = NOABORT;	/* type of exit to take */
	GLOBALDEF i4    Seq_declare = 0;        /* Declare sequence # */
	GLOBALDEF bool	ing_deadlock = FALSE;	/* Deadlock occurred if TRUE */

/*
**	Global variables used by the r_exit routine, for
**		SREPORT and RBF.  Though these are not used by
**		the Report Writer, they are needed in the exit routines.
*/

   /*
   **	File buffers
   */

	GLOBALDEF FILE	*Fl_rcommand = {0};	/* Buffer for RCOMMAND file */
	GLOBALDEF FILE	*Fl_input = {0};	/* Buffer for input text file */
	GLOBALDEF char	En_rcommands[MAX_LOC+1];/* File name of Rcommands file */


   /*
   **	Additional Status Variables.  (See rglobi.h for more)
   */

	GLOBALDEF bool	St_rco_open = FALSE;	/* opened RCOMMAND file */
	GLOBALDEF bool	St_keep_files = FALSE;	/* keep temp files around */

   /*
   ** pipe names
   */
	GLOBALDEF char	*r_xpipe = NULL;
	GLOBALDEF char	*r_vpipe = NULL;

/*
**	general tags for "catch-all" memory allocation which is not
**	covered by tags associated with a specific structure.  These
**	are freed at the specified level in r_reset.  Generally corresponds
**	to pointers associated with the storage being NULL'ed.
*/
	GLOBALDEF i2	Rst5_tag = -1;	/* reset level 5 storage */
	GLOBALDEF i2	Rst4_tag = -1;	/* reset level 4 storage */

/* ADF session control block */

	GLOBALDEF ADF_CB	*Adf_scb;

/*
**	Declarations of REPORT, etc. Routines
*/

	char	*IIRWmct_make_ctrl();	/* q0 format related routines.	*/
	VOID	IIRWpct_put_ctrl();
	bool	deadlock();		/* Track DB deadlocks	*/
	i4	errproc();		/* Track DB errors	*/
	i4	*get_space();		/* allocate and clear memory */
	bool	IISRxs_XNSSet();	/* Enable .DELIMID command	*/
	ACC	*r_acc_get();		/* get an ACC structure */
	ACC	*r_acc_make();		/* construct an ACC structure */
	BRK	*r_brk_find();		/* get BRK structure, given acc */
	ACT	*r_ch_look();		/* look ahead at cached action */
	bool	r_chk_dat();		/* check for legal relation */
	bool	r_chk_fmt();		/* check for valid format */
	REN	*r_chk_rep();		/* check for valid report */
	VOID	r_clear_line();		/* Re-init LN struct and buffers */
	i4	r_cnt_lines();		/* count the number of lines
					** in a TCMD list */
	char	*r_crk_par();
	VOID	r_ctarg_order();	/* Sort ordinal clean_up routine */
	VOID	r_end_frms();		/* Cover for FEendforms()	*/
	bool	r_env_set();		/* set up report environment */
	f4	r_eval_pos();		/* evaluate position */
	i4	r_exit();		/* exit handler */
	STATUS	r_fclose();		/* close a file */
	STATUS	r_fdelete();		/* delete a file */
	BRK	*r_find_brk();		/* check for a control break */
	ATTRIB	r_fnd_sort();		/* find order of an attribute */
	STATUS	r_fopen();		/* open a file */
	i4	r_g_aexp();
	i4	r_g_arterm();
	i4	r_g_arfactor();
	i4	r_g_arprimary();
#ifndef CMS
	i4	r_g_aelement();
#endif
	i4	r_g_blprimary();
	i4	r_g_blelement();
	i4	r_g_double();		/* decode a number */
	i4	r_g_eskip();
	i4	r_g_expr();
	FMT	*r_g_format();		/* get a format specification */
	char	*r_g_ident();		/* get a name which may be a delimited id */
	i4	r_g_long();		/* get a i4  integer */
	ADI_OP_ID r_g_ltype();
	char	*r_g_name();		/* get a name */
	char	*r_g_nname();		/* get a numeric name */
	i4	r_g_skip();		/* skip white space and return
					** type of next token */
	char	*r_g_string();		/* get a string */
	ACT	*r_gch_act();		/* get cached action */
	i4	*r_ges_next(i2);
	i4	r_gord_targ();
	ATTRIB	r_grp_nxt();		/* find next column in group */
	bool	r_grp_set();		/* set up group commands */
	ATT	*r_gt_att();		/* get ATT, given ordinal */
	bool	r_gt_cagg();		/* find code for aggregate */
	ACTION	r_gt_code();		/* get code of command */
	bool	r_gt_ftype();
	ADI_OP_ID r_gt_funct();
	LN	*r_gt_ln();		/* get a new LN struct */
	i2	r_gt_pcode();		/* get code of program constant */
	i4	r_gt_vcode();
	i4	r_gt_wcode();
	VOID	r_gt_word();
	char	*r_gtarg_order();	/* Sort ordinal match routine	*/
	i4	r_ing_err();
	STATUS	r_init_frms();		/* Cover for FEforms()	*/
	VOID	r_ln_clr();		/*
					** Re-init LN struct, buffers, and
					** current/top pointers
					*/
	bool	r_m_dsort();		/* Establish default sort order	*/
	EXP	*r_mak_tree();
	ACC	*r_mk_agg();		/* set up an ACC structure */
	CUM	*r_mk_cum();		/* set up a CUM structure */
	ATTRIB	r_mtch_att();		/* find matching ordinal */
	FMT	*r_n_fmt();		/* make an FMT, given a type */
	TCMD	*r_new_tcmd();		/* get a new TCMD structure */
	ACC	*r_nxt_acc();		/* find next linked ACC */
	i4	r_p_agg();		/* parse an aggregate */
	i4	r_p_att();
	i4	r_p_funct();
	i4	r_p_param();
	i4	r_p_tparam();
	bool	r_p_var();
	bool	r_par_find();
	char	*r_par_get();		/* get or prompt for parameter */
	PAR	*r_par_put();
	i2	r_pc_set();		/* setup of program constant */
	i4	r_preferable();
	char	*r_prompt();		/* prompt user and get entry */
	PRS	*r_prs_get();		/* find a PRS structure */
	TCMD	*r_pop_be();		/* pop the B/E stack */
	ESTK	*r_pop_env();		/* pop the environment */
	STK	*r_psh_be();		/* push the B/E stack */
	ESTK	*r_push_env();		/* push the environment */
	bool	r_qur_do();		/* run the query */
	bool	r_qur_set();		/* set up a query */
	i4	r_readln();		/* read a line from a file */
	bool	r_rep_do();		/* run the report */

# ifndef FOR_SREPORT
	i4	r_report();		/* run the report writer */
# endif

	VOID	r_reset();
	ACC	*r_ret_acc();		/* find an ACC structure */
	i2	r_ret_wid();		/* find width of a column */
	bool	r_rng_add();		/* add a range variable */
	i4		r_scroll();		/* add a range variable */
	bool	r_srt_set();		/* set up sort variable array */
	i4	r_st_adflt();		/* maybe set default pos/format */
	VOID	r_starg_order();	/* Sort ordinal init routine */
	TCMD	*r_wi_begin();		/* set up for a .WITHIN block */
	TCMD	*r_wi_end();		/* end a .WITHIN block */
	i4	r_wrt();		/* write to a file */
	i4	r_wrt_eol();		/* write a line to a file */

	VOID	s_reset();

	FUNC_EXTERN	VOID	r_open_db();

