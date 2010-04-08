/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	 <fstm.h>

/*
** Name:    rglob.h -	Report Writer Global Declarations.
**
** Description:
**	Contains declarations for all the global variables of the report writer.
**
** History:
**	Revision 6.0  87/07  grant
**	Removed 'rf_mes_func' (it's function has been replaced by 'FEmsg()' jhw)
**	changed type returned from expression routines.
**	added .DECLARE and .LET commands.
**	added stuff for .BREAK command and calling OO library
**	changed for handling native-SQL in RW for 6.0
**	fixed up VMS declaration conflicts over type returned by routines.
**	changed types of procedures to match that of actual declarations.
**	added .NULLSTRING
**
**	Revision 5.0  86/04/18  19:00:55  wong
**	Moved declaration of 'rf_mes_func' here from "rattdflt.qc"
**	since both SREPORT and REPORT use it to avoid linking in the
**	Forms system.
**
**	6/19/89 (elein) garnet
**		Added declaration for setup/cleanup structure top pointers.
**	26-jun-89 (sylviap)
**		Added En_printer, En_copies, and St_print for sending the
**		report directly to the printer.
**	28-jul-89 (sylviap)
**		Added En_buffer, En_bcb and En_rows_added for scrollable output.
**              Changed #include from si.h to fstm.h.  fstm.h already includes 
**              si.h.
**	9/5/89 (elein)
**		Added St_cache--boolean for lookaheads
**	08-sep-89 (sylviap)
**		Added St_compatible60 for the -6 flag.
**	28-sep-89 (sylviap)
**		Added En_ctrl for control sequences (Q format).
**	10/27/89 (elein)
**		Added St_setup_continue for continue on error in setup
**      04-dec-89 (sylviap)
**              Added St_copyright for copyright suppression and St_batch for 
**		batch reporting.
**	12/12/89 (elein)
**		Added En_SrcTyp to represent the data source of a report.
**	12/14/89 (martym)
**		Added LabelInfo for labels style reports.
**      1/24/90 (elein)
**              Change to decl of En_file_name & En_rcommands to ensure that
**              LOfroms sequence has correct scope.
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
**		Added St_one_name (US #347)
**      25-jul-90 (sylviap)
**		Added En_tbl for joindef reports.
**      17-aug-90 (sylviap)
**		Added En_lmax_orig for label reports.  Needs to know what the
**		En_lmax was originally set, but En_lmax changes while setting
**		up the report.
**	8/20/90 (elein) 32395
**		Added Warning_count for initialization errors.  These really
**		are warnings and are only noted so that we can allow hit return
**		for them to be viewed.
**      23-aug-90 (sylviap)
**		Added Seq_declare for label reports.  (#32708)
**      31-oct-90 (sylviap)
**		Added St_pgwdth_cmd.  (#33894)
**	26-aug-1992 (rdrane)
**		Add ing_deadlock global.  Add function declarations for
**			i4	errproc()
**			bool	deadlock()
**			STATUS	r_fopen()
**			i4	r_ing_err()
**		Alphabetize the list of function declarations.
**		Remove the obsolete CFEVMSTAT define.
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
**		used in a global context.  Explicitly declare r_clear_line()
**		and r_ln_clr() as returning VOID.  Add declarations of
**		IIRWmct_make_ctrl() and IIRWpct_put_ctrl()
**	17-mar-1993 (rdrane)
**		Eliminate references to Bs_buf (eliminated in fix for
**		b49991) and C_buf (just unused).
**	19-mar-1993 (rdrane)
**		Clean up some of the function declarations.
**	05-Apr-93 (fredb)
**		Removed "St_txt_open" as it is preferred to use "Fl_input".
**	21-jun-1993 (rdrane)
**		Add St_no1stff_on and St_no1stff_set support suppression of
**		the initial formfeed when formfeeds are enabled.
**	12-jul-1993 (rdrane)
**		Add declrations of r_init_frms() and r_end_frms.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Sep-2002 (hanje04)
**	    Change prototype for r_ges_next so that it matches the updated
**	    style in gt.c the prototype in rglob.h which was causing 
**	    compilation problems on Linux.
**      28-apr-2004 (srisu02)
**           Declared num_oflag for new flag "numeric_overflow=ignore|fail|warn"
**	28-Mar-2005 (lakvi01)
**	    Added prototypes for r_m_*() functions.
**	09-Apr-2005 (lakvi01)
**	    Moved some function declarations into NT_GENERIC and reverted back
**	    to old declerations since they were causing compilation problems on
**	    non-windows compilers.
**	26-Aug-2005 (hanje04)
**	    Fix up prototypes after bad cross
**	14-Nov-2008 (gupsh01)
**		Added support for -noutf8align flag for report writer.
**	28-Aug-2009 (kschendel) b121804
**	    FEadfcb decl is now in fe.h.
**	24-Feb-2010 (frima01) Bug 122490
**	    Moved s_reset to sglob.h and added function prototypes as
**	    neccessary to eliminate gcc 4.3 warnings.
*/

/*
**	Global Variables in Report Formatter
**	------ --------- -- ------ ---------
**		(Declarations only)
*/


/*
**	En - contains overall report environment information.
**		Many of these variables come from the RENVIR relation.
*/

	extern	char	*En_database;		/* database name */
	extern	char	*En_report;		/* report name */
	extern	char	*En_rep_owner;		/* user code for report owner */
	extern	FE_RSLV_NAME En_ferslv;	/*
						** FE_RSLV_NAME structure to
						** input and resolved table
						** name and owner name for
						** data relation
						*/
	extern	char	*En_pre_tar;		/* 'unique' or 'distinct/all'
						** before the target list */
	extern	char	*En_tar_list;		/* target list if query */
	extern	char	*En_srt_list;		/* list of sort keys */
	extern	char	*En_q_qual;		/* qualification clause if query */
	extern	char	*En_sql_from;		/* select from list if sql query */
	extern	char	*En_remainder;		/* replaces qual and sort.
						** contains remainder of query
						** after target or from list */
	extern	char	En_file_name[MAX_LOC+1];/* file to write report to  */
	extern	char	*En_fspec;		/* file specified on call to
					        ** report.  Overrides default */
	extern	char	*En_printer;		/* printer to write report to  */
	extern	i4	En_copies;		/* number of copies to print */
	extern	i4	En_num_oflag;		/* "-numeric_overflow" flag  */

	extern	char	*En_uflag;		/* -u flag, if specified */
	extern	char	*En_gidflag;		/* -G flag, if specified */
	extern	char	*En_passflag;		/* -P flag, if specified */
	extern	ATTRIB	En_n_attribs;	/* number of attributes in data relation */
	extern	ATTRIB	En_n_sorted;		/* number of attributes in sort list */
	extern	ATTRIB	En_n_breaks;		/* number of breaks in report */
	extern	FILE	*En_rf_unit;		/* unit number of report file */
	extern	REN	*En_ren;		/* REN for report */
	extern	i4	En_lc_bottom;		/* number of lines in page footer */
	extern	i4	En_lc_top;		/* number of lines in page header */
	extern	i4	En_qmax;		/* maximum query size */
	extern	i4	En_lmax;		/* maximum line size */
	extern	i4	En_lmax_orig;		/* maximum line size */
	extern	i4	En_wmax;		/* maximum # lines to wrap */
	extern	i4	En_scount;		/* number of sort vars in REPORTS */
	extern	i4	En_bcount;		/* number of break vars in REPORTS */
	extern	i4	En_qcount;		/* number of query lines in REPORTS */
	extern	i4	En_acount;		/* number of action lines in REPORTS */
	extern	i2	En_program;		/* Program which is running */
	extern	char	En_rtype;		/* type of report, either 's'
						** for SREPORT, or 'f' for RBF */
	extern	i4	En_cols;		/* # of columns on terminal */
	extern	i4	En_lines;		/* # of lines on terminal */
	extern	char	En_termbuf[80];		/* buffer for TDtermsize */
	extern	char	*En_initstr;		/* initialization string for terminal */
	extern	i4	En_amax;		/* maximum action cache size */
	extern	i4	En_qlang;		/* report query language */
	extern	OOID	En_rid;			/* object-oriented id for report */

        /* For scrollable output */
	extern char  	*En_buffer;       	/* buffer for one line of the
						** report */
	extern i4    	En_rows_added;      	/* number of rows added to
						** scrolling buffer */
	extern BCB  	*En_bcb;       		/* buffer control block for 
						** scrolling */
        extern i4    En_SrcTyp;              	/* the type of the data
	                                       	** source for the report */
	extern LABST_INFO LabelInfo;		/* Contains the dimensions 
						** of a labels style report */
	extern	char	En_pg_width[];		/* Maximum page width */
	extern	bool	En_need_utf8_adj;	/*  Is UTF8 adjustment needed */
	extern	bool	En_need_utf8_adj_frs;	/*  Is UTF8 adjustment needed */
	extern	i4	En_utf8_adj_cnt;	/*  num of spaces for utf8 adjustment */
	extern	i4	En_utf8_lmax;		/*  Maximum line buffer in UTF8  installation */
	extern	bool	En_utf8_set_in_within;	/*  UTF8 adj done in r_x_within*/


/*
**	St  - contains the current status flags during the
**		running of the report formatter.  Whereas the
**		ENV variables represent global constants, the
**		STA variables represent current status information
**		in the running of the report.
*/

	extern	bool	St_db_open;		/* true if database open */
	extern	bool	St_in_retrieve; /* true when in data retrieve loop */
	extern	bool	St_before_report;	/* true if no data read */
	extern	bool	St_no_write;		/* TRUE until first writing
						** to the report */
	extern	bool	St_do_phead;		/* true when page headers
						** are to be done */
	extern	bool	St_rf_open;		/* true if report file open */
	extern	bool	St_tf_set;		/* true if any "tformat" are set */
	extern	bool	St_in_pbreak;		/* true when printing page breaks */
	extern	bool	St_rep_break;		/* true when end of data reached */
	extern	char	*St_h_tname;		/* text name of current break
						** when page break occurs */
	extern	char	*St_h_attribute;	/* name of current attribute
						** when page break occurs */
	extern	bool	St_h_written;		/* TRUE if current break had
						** write when page break occurs */
	extern	bool	St_lspec;		/* TRUE if -ln specified */
	extern	bool	St_parms;		/* TRUE if report has params */
	extern	bool	St_repspec;		/* TRUE if -r flag set */
	extern	bool	St_silent;		/* TRUE if -s flag set */
	extern	bool	St_truncate;		/* TRUE if +t flag set, FALSE if -t flag set */
	extern	bool	St_tflag_set;		/* TRUE if -t or +t flag set */
	extern	bool	St_ispec;		/* TRUE if -i flag set; if so,
						** En_sfile (in SREPORT) is
						** set to file name */
	extern	bool	St_ing_error;		/* True if INGRES error occurs */
	extern	bool	St_hflag_on;		/* TRUE if -h flag set */

	extern	bool	St_err_on;		/* TRUE if errors on */
	extern	bool	St_to_term;		/* TRUE if report going to terminal */
	extern	bool	St_p_term;		/* TRUE if prompting when going
						** to the terminal */
	extern	bool	St_underline;		/* TRUE in underlining mode */
	extern	bool	St_called;		/* TRUE if report called from
						** program */
	extern	bool	St_ff_on;		/* TRUE if formfeed command set */
	extern	bool	St_ff_set;		/* TRUE if formfeed set from command line */
	extern	bool	St_no1stff_on;		/* TRUE if no initial formfeed command set */
	extern	bool	St_no1stff_set;		/* TRUE if no initial formfeed set from command line */
	extern	ATTRIB	St_brk_ordinal;	/* break order of current break. */
	extern	BRK	*St_break;		/* current BRK structure found
						** while reading data.
						*/
	extern	i2	St_style;		/* style of report to use, if
						** one is to be set up */
	extern	i4	St_p_length;		/* current page length */
	extern	bool	St_pl_set;		/* TRUE if page length set from command line */
	extern	i4	St_right_margin;	/* current right margin */
	extern	i4	St_left_margin; /* current left margin */
	extern	i4	St_p_rm;		/* page break right margin */
	extern	i4	St_r_rm;		/* report right margin */
	extern	i4	St_p_lm;		/* page break left margin */
	extern	i4	St_r_lm;		/* report left margin */
	extern	bool	St_rm_set;		/* TRUE when right margin set */
	extern	bool	St_lm_set;		/* TRUE when left margin set */
	extern	char	St_ulchar;		/* default character for underlining */
	extern	i4	St_l_number;		/* current line number */
	extern	i4	St_top_l_number;	/* line number of block top */
	extern	i4	St_p_number;		/* current page number */
	extern	i4	St_pos_number;		/* current line position number */
	extern	DB_DATA_VALUE	St_cdate;	/* current_date */
	extern	DB_DATA_VALUE	St_ctime;	/* current_time */
	extern	DB_DATA_VALUE	St_cday;	/* current_day */
	extern	i4	St_nxt_page;		/* next page number, if set
						** by ".newpage" command.
						*/

	extern	bool	St_adjusting;		/* TRUE when centering, or justifying */
	extern	TCMD	*St_a_tcmd;		/* current TCMD structure used
						** in justifying */

	extern	bool	St_prline;		/* TRUE if last print command
						** was a print line command */

	extern	LN	*St_cline;		/* current line structure for report */
	extern	LN	*St_c_lntop;		/* top fo current LN linked list */
	extern	ESTK	*St_c_estk;		/* current spot in environment stack */

	extern	STK	*St_rbe;		/* current report B/E stack */
	extern	STK	*St_pbe;		/* current page B/E stack */
	extern	STK	*St_cbe;		/* current B/E stack */

	extern	i4	St_c_rblk;		/* report level of nesting of .BLK */
	extern	i4	St_c_pblk;		/* page level of nesting of .BLK */
	extern	i4	St_c_cblk;		/* current level of nesting of .BLK */

	extern	TCMD	*St_rwithin;		/* current .WITHIN command for report */
	extern	TCMD	*St_pwithin;		/* current .WITHIN command for page */
	extern	TCMD	*St_cwithin;		/* current .WITHIN command */

	extern	TCMD	*St_rcoms;		/* report first format command for .WI */
	extern	TCMD	*St_pcoms;		/* page first format command for .WI */
	extern	TCMD	*St_ccoms;		/* first format command for .WI */
	extern	bool	St_compatible;		/* TRUE if -5 flag set */
	extern	bool	St_compatible60;	/* TRUE if -6 flag set */
	extern	bool	St_print;		/* TRUE if -o flag set 
						** send report to the printer*/
	extern bool	St_cache;		/* TRUE if action look ahead is ok
						** set to F and set back to T in ractset
						** checked in rgeskip()*/

	extern bool	St_setup_continue;	/* Continue on error in setup
						** if true, Default is false
						*/
	extern bool  	St_in_setup;    	/* TRUE if in setup or cleanup sections
						*/
        extern bool     St_copyright;           /* TRUE if copyright banner
						** should be suppressed */
	extern bool     St_batch;               /* TRUE if RW is being run
						** in the background, so
						** suppress all prompts */
	extern bool     St_one_name;            /* TRUE if RW has read in one
						** .NAME command */
	extern bool     St_pgwdth_cmd;          /* TRUE if RW has PAGEWIDTH
						** command */
	extern bool	St_xns_given;		/* TRUE if report has an
						** .DELIMID (eXpanded Name
						** Space) Command
						*/

/*
**	Ptr - contains pointers to the start of many of
**		the dynamically allocated linked lists and arrays used by
**		the report formatter.
*/

	extern	ATT	*Ptr_att_top;		/* start of attribute list */

	extern	PRS	*Ptr_prs_top;		/* start of array of PRS structures */
	extern	SORT	*Ptr_sort_top;		/* start of sort attribute list */
	extern	char	**Ptr_break_top;	/* start of break attribute list */
	extern	DCL	*Ptr_dcl_top;		/* start of declared variable list */

	extern	BRK	*Ptr_brk_top;		/* top of breaks stack */
	extern	BRK	*Ptr_last_brk;		/* ptr to bottom of break stack */
	extern	BRK	*Ptr_det_brk;		/* ptr to detail break struct */
	extern	BRK	*Ptr_pag_brk;		/* ptr to page break structure */

	extern	LAC	*Ptr_lac_top;		/* top of lac list */
	extern	PAR	*Ptr_par_top;		/* start of list of parameters */
	extern	ESTK	*Ptr_estk_top;		/* start of environment stack */
	extern	STK	*Ptr_stk_top;		/* top of BEGIN/END stack */
	extern	RNG	*Ptr_rng_top;		/* start of range linked list */

	extern	LN	*Ptr_rln_top;		/* top of report line buffers */
	extern	LN	*Ptr_pln_top;		/* top of page break line buffers */
	extern	LN	*Ptr_cln_top;		/* top of centering line buffers */

	extern	QUR	*Ptr_qur_arr;		/* points to array of QUR structs */
	extern	SC	*Ptr_set_top;		/* points to list of setup structs */
	extern	SC	*Ptr_clean_top;		/* points to list of cleanup structs */

	extern	STK	*Ptr_t_rbe;		/* top of report B/E stack */
	extern	STK	*Ptr_t_pbe;		/* top of page B/E stack */
	extern	STK	*Ptr_t_cbe;		/* top of current B/E stack */

	extern	i4	Sequence_num;		/* sequence # for current action */
	extern  TBLPTR  *Ptr_tbl;        	/* start of table list */
						/* Used for joindef reports */



/*
**	Cact - flags etc which correspond to the current text action
**		being processed.
*/

	extern	char	*Cact_tname;	/* i4  name of current text type */
	extern	BRK	*Cact_break;	/* current BRK structure */
	extern	bool	Cact_written;	/* TRUE if current break
					** had write */
	extern	char	*Cact_type;	/* current text type (header,footer,or
					** detail */
	extern	char	*Cact_attribute; /* current attribute name */
	extern	TCMD	*Cact_tcmd;	/* current TCMD structure */
	extern	i2	Tcmd_tag;	/* TCMD tag id */
	extern	char	*Cact_rtext;	/* current RTEXT string */
	extern	char	*Cact_command;	/* current RTEXT command name */
	extern	ATTRIB	*Cact_bordinal;	/* ordinal of current break */
	extern	char	Oact_type[];	/* current text type (header,footer,or
					** detail */
	extern	char	Oact_attribute[]; /* current attribute name */


/*
**	Token - Variables used for Token generation.
*/

	extern	char	*Tokchar;	/* ptr to current postion in
					** token line */


/*
**	Error Processing
*/

	extern	i2	Err_count;		/* count of number of non-fatal errors */
	extern	ERRNUM	Err_ingres;		/* INGRES or EQUEL error number */
	extern	i2	Warning_count;		/* count of number of warning errors */

/*
**	Debugging Trace Information
*/

	extern	i2	T_flags[T_SIZE];	/* table of trace flags */


/*
**	Global storage spaces.
*/

	extern	char	Usercode[FE_MAXNAME+1]; /* temp storage of user code */
	extern	char	Dbacode[FE_MAXNAME+1]; /* storage of dba code */

/*
**	Standard input and output file buffers
*/

	extern	FILE	*Fl_stdin;	/* standard input file number */
	extern	FILE	*Fl_stdout;	/* standard output file number */



/*
**	Miscellaneous.
*/

	extern	char	*Q_buf;		/* buffer to hold query */
	extern	i4	Lj_buf[2];	/* i4  jump buffer */
	extern	i4	Err_type;	/* type of exit to take */
	extern  i4      Seq_declare;    /* Declare sequence # */

	extern  bool	ing_deadlock;	/* Deadlock occurred if TRUE */




/*
**	Global variables used by the r_exit routine, for
**		SREPORT and RBF.  Though these are not used by
**		the Report Writer, they are needed in the exit routines.
*/

   /*
   **	File buffers
   */

	extern	FILE	*Fl_rcommand;		/* Buffer for RCOMMAND file */
	extern	FILE	*Fl_input;		/* Buffer for input text file */
	extern	char	En_rcommands[MAX_LOC+1]; /* Name of RCOMMANDS file */


   /*
   **	Additional Status Variables.  (See rglobi.h for more)
   */

	extern	bool	St_rco_open;		/* opened RCOMMAND file */
	extern	bool	St_keep_files;		/* keep temp files around */


/*
**	general tags for "catch-all" memory allocation which is not
**	covered by tags associated with a specific structure.  These
**	are freed at the specified level in r_reset.  Generally corresponds
**	to pointers associated with the storage being NULL'ed.
*/
	extern	i2	Rst5_tag;	/* reset level 5 storage */
	extern	i2	Rst4_tag;	/* reset level 4 storage */

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
	STATUS	r_fopen();		/* open a file */
	BRK	*r_find_brk();		/* check for a control break */
	ATTRIB	r_fnd_sort();		/* find order of an attribute */
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
	#ifdef NT_GENERIC
	VOID	r_m_action(char *, ...);
	VOID	r_m_chead(i2, i2);
	VOID	r_m_pbot(i4, i4);
	VOID	r_m_ptop(i4, i4, i4);
	VOID	r_m_rtitle(i4, i4);
	VOID	r_m_sort(i2, ATTRIB, char *, char *);
	VOID    r_open_db();
	VOID    rFreset(i2, ...);
	#else
	VOID    rFreset();
	#endif /* NT_GENERIC */

	bool	r_m_dsort();		/* Establish default sort order	*/
	VOID	r_open_db();
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
	bool	r_srt_set();		/* set up sort variable array */
	i4	r_st_adflt();		/* maybe set default pos/format */
	VOID	r_starg_order();	/* Sort ordinal init routine */
	TCMD	*r_wi_begin();		/* set up for a .WITHIN block */
	TCMD	*r_wi_end();		/* end a .WITHIN block */
	i4	r_wrt();		/* write to a file */
	i4	r_wrt_eol();		/* write a line to a file */
	i4	rFrbf(i4, char *[]);
	i2	r_gt_rword(char *);


	FUNC_EXTERN	VOID	r_syserr();
	FUNC_EXTERN	i4	r_fcreate();
	FUNC_EXTERN	i4	r_g_hexlit();
	FUNC_EXTERN	VOID	r_g_set();
	FUNC_EXTERN	VOID	r_p_let();
	FUNC_EXTERN	VOID	r_p_if();
	FUNC_EXTERN	VOID	r_p_str();
	FUNC_EXTERN	VOID	r_p_begin();
	FUNC_EXTERN	VOID	r_p_end();
	FUNC_EXTERN	VOID	r_p_ewithin();
	FUNC_EXTERN	VOID	r_p_within();
	FUNC_EXTERN	VOID	r_p_eflag();
	FUNC_EXTERN	VOID	r_p_flag();
	FUNC_EXTERN	VOID	r_p_width();
	FUNC_EXTERN	VOID	r_p_pos();
	FUNC_EXTERN	VOID	r_p_format();
	FUNC_EXTERN	VOID	r_p_snum();
	FUNC_EXTERN	VOID	r_p_vpnum();
	FUNC_EXTERN	VOID	r_p_usnum();
	FUNC_EXTERN	VOID	r_p_eprint();
	FUNC_EXTERN	VOID	r_p_text();
	FUNC_EXTERN	VOID	r_p_pos();
	FUNC_EXTERN	VOID	r_par_clip();
	FUNC_EXTERN	VOID	r_strpslsh();
	FUNC_EXTERN	VOID	r_fix_parl();
	FUNC_EXTERN	VOID	r_x_tcmd();
	FUNC_EXTERN	VOID	r_stp_adjust();
	FUNC_EXTERN	VOID	r_x_print();
	FUNC_EXTERN	VOID	r_x_newline();
	FUNC_EXTERN	VOID	r_x_tab();
	FUNC_EXTERN	VOID	r_switch();
	FUNC_EXTERN	VOID	r_set_dflt();
	FUNC_EXTERN	VOID	r_e_item();
	FUNC_EXTERN	VOID	r_nullbool();
	FUNC_EXTERN	VOID	r_a_cum();
	FUNC_EXTERN	VOID	r_a_acc();
	FUNC_EXTERN	VOID	r_e_par();
	FUNC_EXTERN	VOID	r_x_att();
	FUNC_EXTERN	VOID	r_x_pc();
	FUNC_EXTERN	VOID	r_runerr();
	FUNC_EXTERN	VOID	r_x_sformat();
	FUNC_EXTERN	VOID	r_par_type();
	FUNC_EXTERN	VOID	r_wild_card();
	FUNC_EXTERN	VOID	r_frc_etype();
	FUNC_EXTERN	VOID	r_clr_add();
	FUNC_EXTERN	VOID	r_op_add();
	FUNC_EXTERN	VOID	r_nxt_set();
	FUNC_EXTERN	VOID	r_m_pbot();
	FUNC_EXTERN	VOID	r_m_rtitle();
	FUNC_EXTERN	VOID	r_m_ptop();
	FUNC_EXTERN	VOID	r_m_rhead();
	FUNC_EXTERN	VOID	rF_bstrcpy();
	FUNC_EXTERN	VOID	r_m_action();
	FUNC_EXTERN	VOID	r_m_sort();
	FUNC_EXTERN	VOID	r_m_labels();
	FUNC_EXTERN	VOID	r_m_block();
	FUNC_EXTERN	VOID	r_m_column();
	FUNC_EXTERN	VOID	r_fmt_dflt();
	FUNC_EXTERN	VOID	r_mov_dat();
	FUNC_EXTERN	VOID	r_do_footer();
	FUNC_EXTERN	VOID	r_brkdisp();
	FUNC_EXTERN	VOID	r_trunc();
	FUNC_EXTERN	VOID	r_do_header();
	FUNC_EXTERN     VOID    r_a_caccs();
	FUNC_EXTERN     VOID    r_ach_del();
	FUNC_EXTERN     VOID    r_act_set();
	FUNC_EXTERN     VOID    r_advance();
	FUNC_EXTERN     VOID    r_a_ops();
	FUNC_EXTERN     bool    r_brk_set();
	FUNC_EXTERN     VOID    r_cmd_skp();
	FUNC_EXTERN     STATUS  r_convdbv();
	FUNC_EXTERN     VOID    r_crack_cmd();
	FUNC_EXTERN     VOID    r_dcl_set();
	FUNC_EXTERN     VOID    r_do_page();
	FUNC_EXTERN     VOID    r_e_blexpr();
	FUNC_EXTERN     VOID    r_end_type();
	FUNC_EXTERN     VOID    r_fix_parl();
	FUNC_EXTERN     VOID    r_ges_reset();
	FUNC_EXTERN     VOID    r_ges_set();
	FUNC_EXTERN     STATUS  r_getrow();
	FUNC_EXTERN     VOID    r_init();
	FUNC_EXTERN     VOID    r_lnk_agg();
	FUNC_EXTERN     VOID    r_m_chead();
	FUNC_EXTERN     VOID    r_m_rprt();
	FUNC_EXTERN     VOID    r_nxt_row();
	FUNC_EXTERN     VOID    r_nxt_within();
	FUNC_EXTERN     VOID    r_rco_set();
	FUNC_EXTERN     VOID    r_rep_load();
	FUNC_EXTERN     VOID    r_rep_set();
	FUNC_EXTERN     VOID    r_scn_tcmd();
	FUNC_EXTERN     i4      r_sc_qry();
	FUNC_EXTERN     i4      r_sendqry();
	FUNC_EXTERN     VOID    r_tcmd_set();
	FUNC_EXTERN     VOID    r_x_adjust();
	FUNC_EXTERN     VOID    r_x_bpos();
	FUNC_EXTERN     i4      r_x_chconv();
	FUNC_EXTERN     i4      r_x_colconv();
	FUNC_EXTERN     i4      r_x_conv();
	FUNC_EXTERN     VOID    r_x_eblock();
	FUNC_EXTERN     VOID    r_x_end();
	FUNC_EXTERN     VOID    r_x_ewithin();
	FUNC_EXTERN     i4      r_x_iconv();
	FUNC_EXTERN     VOID    r_x_lm();
	FUNC_EXTERN     VOID    r_x_need();
	FUNC_EXTERN     VOID    r_x_npage();
	FUNC_EXTERN     VOID    r_x_pl();
	FUNC_EXTERN     VOID    r_x_prelement();
	FUNC_EXTERN     VOID    r_x_rm();
	FUNC_EXTERN     VOID    r_x_spos();
	FUNC_EXTERN     VOID    r_x_swidth();
	FUNC_EXTERN     VOID    r_x_tformat();
	FUNC_EXTERN     VOID    r_x_within();
	FUNC_EXTERN     VOID    scan_loop();
	FUNC_EXTERN     VOID    strt_line();
	FUNC_EXTERN     VOID    rF_unbstrcpy();
	FUNC_EXTERN     VOID    IIVFlninit();
	FUNC_EXTERN     VOID    IIVFstrinit();
	FUNC_EXTERN     VOID    IIVFgetmu();
	FUNC_EXTERN     VOID    r_m_InitLabs();
	FUNC_EXTERN     VOID    sec_list_remove();

/*
** GLOBALS for subprocess calls of the report writer
**
** 9/8/82 (jrc)
*/
	extern	char	*r_xpipe;		/* transmit pipe */
	extern	char	*r_vpipe;		/* recieVe pipe */

/* ADF session control block */

	extern		ADF_CB	*Adf_scb;

