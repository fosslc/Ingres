/*
** File:	fndefs.h
**
** Description:
**
**	This file contains all function declaration and their prototyped
** versions. There are two main sections, one for building with prototypes
** and one without. Within each of these there are subsections for each
** module.  Modules are identified by filename, with the dot changed to an
** underscore, i.e.: septool.c is defined as septool_c.
*/

/*
** History:
**
**	 2-jun-1993 (donj)
**		Created.
**	14-aug-1993 (donj)
**		Added SEP-alloc_Opt () function.
**      16-aug-1993 (donj)
**		Removed SEP_MEtrace_list(). reflect changes in
**		SEP_Vers_Init().
**	17-aug-1993 (donj)
**		Removed another SEP_MEtrace_list() declaration.
**	20-aug-1993 (donj)
**		Added the new function SEP_CMwhatcase().
**	 7-sep-1993 (donj)
**		Add parameter to open_io_comm_files ().
**	 9-sep-1993 (donj)
**		Added two new functions, undefine_env_var and unsetenv_cmmd.
**		These two were needed to make the handling of env vars a
**		bit more complete. Also, some people were trying to 'un'setenv
**		by using setenv incorrectly. So we should give them the
**		proper way to do it.
**	23-sep-1993 (donj)
**		Add parameter to SEPtranslate ().
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**      18-oct-1993 (donj)
**          Break functions out of SEP_exp_eval(), the SEPEVAL*()
**	    functions.
**      26-oct-1993 (donj)
**          Broke up SEP_TRACE functions to make them less complex.
**	14-dec-1993 (donj)
**	    Added functions, IS_ValueNotation() and Trans_SEPvar() to implement
**	    @VALUE() contruct (like @FILE()).
**	20-dec-1993 (donj)
**	    Added prototype for utility2.c function, SEP_STclassify() which
**	    classify's a string as a i4, or a f8 or as a char * and translates
**	    the string to i4  and/or f8.
**	30-dec-1993 (donj)
**	    Added a change Token Mask field to Trans_Token() prototype. Split out
**	    several functions from septool.c (thusly septool_c) into their own files:
**
**		Disp_Line()              now in displine.c (displine_c)
**		Disp_Line_no_refresh()   now in displine.c (displine_c)
**		get_command()            now in getcmd.c   (getcmd_c)
**		next_record()            now in nextrec.c  (nextrec_c)
**		set_sep_state()          now in setstate.c (setstate_c)
**		shell_cmmd()             now in shellcmd.c (shellcmd_c)
**		assemble_line()          now in assemble.c (assemble_c)
**		break_line()             now in breakln.c  (breakln_c)
**		classify_cmmd()          now in classify.c (classify_c)
**       4-mar-1994 (donj)
**          Had to change exp_bool from type bool to type i4, exp_bool was
**          exp_bool was carrying negative numbers - a no no on AIX. Changed
**          it's name from exp_bool to exp_result. Also had to change the
**          types of SEP_Eval_If(), SEP_exp_eval(), SEPEVAL_alnum(),
**          SEPEVAL_opr_expr(), SEPEVAL_expr_opr_expr(),
**          SEPEVAL_expr_plus_expr(), SEPEVAL_expr_mult_expr(),
**          SEPEVAL_expr_div_expr(), SEPEVAL_expr_equ_expr(),
**          SEPEVAL_expr_lt_expr() and SEPEVAL_expr_minus_expr() for the
**          sake of clarity.
**	13-apre-1994 (donj)
**	    Changed disp_res() from a i4  to a STATUS return type.
**	14-apr-1994 (donj)
**	    Added testcase module and function declarations.
**	20-apr-1994 (donj)
**	    Added somemore testcase modules and modified some.
**	10-may-1194 (donj)
**	    Split out several functions out of procmmd.c to their own files
**	    to make procmmd.c more readable. New files:
**
**	    	legalusr.c	crestf.c	opncomm.c
**		evalif.c	crecanon.c	diffing.c
**		isitif.c	readtc.c	stillcon.c
**		testinfo.sc
**
**	    Each of these files has their own section in fndefs.h.
**	    Also added section for executor.c. Reformatted alot for
**	    readability.
**	17-may-1194 (donj)
**	    VMS doesn't like #ifdefs that don't start in the first column.
**	24-may-1994 (donj)
**	    Add errmsg parameter to load_termcap() and load_commands().
**	25-may-1994 (donj)
**	    Reconcile get_answer(), in sep, and get_answer2(), in listexec,
**	    into one root function SEP_Get_Answer() in file sep/getanswer.c
**	27-may-1994 (donj)
**	    Change copy_hdr fm STATUS to i4  and add errmsg param.
**	22-jun-1994 (donj)
**	    Add a trace msg string to endOfQuery() function's parameter list.
**	28-Feb-1996 (somsa01)
**	    Added NT_GENERIC to "#ifdef UNIX"; these function definitions apply
**	    to NT as well.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	17-Oct-2008 (wanfr01)
**	    Bug 121090
**	    Added new parameter to get_command, next_record
**	24-Aug-2009 (kschendel) 121804
**	    eval-if is i4, not bool.  gcc 4.3 cares.
*/

/*
** disp_line macro used in certain files.
*/

#if defined(comments_c)|defined(crestf_c)|defined(diffing_c)|defined(evalif_c)|defined(opncomm_c)|defined(procmmd_c)|defined(sepostcl_c)|defined(sepparm_c)|defined(septool_c)

#ifndef septool_c
GLOBALREF    WINDOW       *mainW ;
GLOBALREF    i4            screenLine ;
GLOBALREF    bool          paging ;
#endif /* ndef septool_c */

#define disp_line(l,r,c) \
Disp_Line(mainW, batchMode, &screenLine, paging, l, r, c)
#define disp_line_no_refresh(l,r,c) \
Disp_Line_no_refresh(mainW, batchMode, &screenLine, l, r, c)

#endif /* comments_c, crestf_c, etc... */

#if defined(procmmd_c)|defined(opncomm_c)

GLOBALREF    char          buffer_1 [] ;
GLOBALREF    i4            testGrade ;

#define return_fail(e,f,g)  \
{ STcopy(e,buffer_1); testGrade = g; return(f); }

#endif
	/* ------------------------------------------------------- */
#ifdef executor_c
                     STATUS        EXEC_initialize      ( char *errmsg ) ;
                     STATUS        EXEC_get_args        ( i4  argc,
                                                          char *argv[],
                                                          char *message ) ;
                     STATUS        parse_list_line      ( char *theline,
                                                          char **theparts,
                                                          char *dirOfTest ) ;
                     STATUS        read_confile         ( char *message ) ;
                     STATUS        EXEC_cat_errmsg      ( i4  sonres,
                                                          char *repLine ) ;
                     STATUS        EXEC_print_rpt_header( char *buffer,
                                                          FILE *repPtr ) ;
                     i4            EXEC_was_sep_aborted ( i4  sonres,
                                                          i4  difflevel ) ;
                     STATUS        EXEC_open_files      ( char *errmsg,
                                                          FILE **tstPtr,
                                                          FILE **rptPtr,
                                                          LOCATION *gtLoc,
                                                          FILE **gtPtr,
                                                          LOCATION *bdLoc,
                                                          FILE **bdPtr ) ;
                     char         *EXEC_TMnow           () ;
#ifdef VMS
                     STATUS        EXEC_chk_listexecKey ( TCFILE **kptr,
                                                          char *errmsg ) ;
#endif
                     STATUS        EXEC_were_partsubs_selected ( char *errmsg ) ;
        
                     STATUS        check_filename       ( char *filenm,
                                                          char *newfilenm,
                                                          char *message,
                                                          bool *isfull,
                                                          bool isthere ) ;
                     STATUS        check_outdir         ( char *dirname,
                                                          char *newdirname,
                                                          char *message ) ;
                     STATUS        get_test_flags       ( char *theline,
                                                          char **theparts,
                                                          i4  *count ) ;
                     STATUS        get_test_dir         ( char *theline,
                                                          char *thedir ) ;
                     STATUS        get_test_class       ( char *theline,
                                                          char **theparts,
                                                          i4  *count ) ;
                     STATUS        read_subs            ( PART_NODE *part,
                                                          char *message,
                                                          FILE *aptr ) ;
#endif	/* executor_c */
	/* ------------------------------------------------------- */
#ifdef septool_c
                     VOID          get_answer           ( char *question,
							  char *ansbuff ) ;
                     STATUS        load_commands        ( char *pathname,
							  char *errmsg ) ;
                     STATUS        insert_node          ( CMMD_NODE **master,
							  CMMD_NODE *anode ) ;
                     bool          break_arg            ( char *arg,
							  char *savebuf,
							  char **param,
                                                          char **op_tion,
							  i4  *param_len,
							  i4  *op_tion_len ) ;
                     i4            copy_hdr             ( FILE *testFile,
							  char *errmsg ) ;
                     STATUS        define_env_var       ( char *name,
							  char *newval ) ;
                     STATUS        undefine_env_var     ( char *name ) ;
                     VOID          exiting              ( i4  value,
							  bool getout ) ;
#else
#ifndef peditor_c

        FUNC_EXTERN  VOID          get_answer           ( char *question,
							  char *ansbuff ) ;
#ifdef SI_INCLUDED
        FUNC_EXTERN  i4            copy_hdr             ( FILE *testFile,
							  char *errmsg ) ;
#endif /* SI_INCLUDED */
        FUNC_EXTERN  STATUS        load_commands        ( char *pathname,
							  char *errmsg ) ;
        FUNC_EXTERN  bool          break_arg            ( char *arg,
							  char *savebuf,
							  char **param,
                                                          char **op_tion,
							  i4  *param_len,
							  i4  *op_tion_len ) ;
        FUNC_EXTERN  STATUS        define_env_var       ( char *name,
							  char *newval ) ;
        FUNC_EXTERN  STATUS        undefine_env_var     ( char *name ) ;

#ifdef sepdefs_h
        FUNC_EXTERN  STATUS        insert_node          ( CMMD_NODE **master,
							  CMMD_NODE *anode ) ;
#endif /* sepdefs_h */
#endif /* peditor_c */
#endif /* septool_c */
	/* ------------------------------------------------------- */
#ifdef getanswer_c
                     i4            SEP_Get_Answer       ( WINDOW *stat_win,
							  i4  row, i4  col,
							  char *question,
							  char *ansbuff,
							  bool *batch_mode,
							  char **errmsg ) ;
#else
#ifdef WINDOW
        FUNC_EXTERN  i4            SEP_Get_Answer       ( WINDOW *stat_win,
							  i4  row, i4  col,
							  char *question,
							  char *ansbuff,
							  bool *batch_mode,
							  char **errmsg ) ;
#endif /* WINDOW */
#endif /* getanswer_c */
	/* ------------------------------------------------------- */
#ifdef legalusr_c
                     bool          legal_urange         ( char *uname,
							  char *uname_root,
							  i4  i_min,
							  i4  i_max ) ;
                     bool          Legal_User           ( char *User_name,
							  char *errmsg ) ;
#else
        FUNC_EXTERN  bool          Legal_User           ( char *User_name,
							  char *errmsg ) ;
#endif
	/* ------------------------------------------------------- */
#ifdef breakln_c
                     VOID          break_line           ( char *string,
							  i4  *tokens,
							  char **toklist ) ;
#else
        FUNC_EXTERN  VOID          break_line           ( char *string,
							  i4  *tokens,
							  char **toklist ) ;
#endif /* breakln_c */
	/* ------------------------------------------------------- */
#ifdef classify_c
                     i4            classify_cmmd        ( CMMD_NODE *master,
							  char *acmmd ) ;
#else
#ifdef sepdefs_h
        FUNC_EXTERN  i4            classify_cmmd        ( CMMD_NODE *master,
							  char *acmmd ) ;
#endif /* sepdefs_h */
#endif /* classify_c */
	/* ------------------------------------------------------- */
#ifdef setstate_c
                     STATUS        set_sep_state        ( i4  tokens ) ;
#else
        FUNC_EXTERN  STATUS        set_sep_state        ( i4  tokens ) ;
#endif /* setstate_c */
	/* ------------------------------------------------------- */
#ifdef nextrec_c
                     STATUS        next_record          ( FILE *fdesc, char print ) ;
#else
#ifdef SI_INCLUDED
        FUNC_EXTERN  STATUS        next_record          ( FILE *fdesc, char print ) ;
#endif /* SI_INCLUDED */
#endif /* nextrec_c */
	/* ------------------------------------------------------- */
#ifdef assemble_c
                     i4            assemble_line        ( i4  tokens,
							  char *buffer ) ;
#else
        FUNC_EXTERN  i4            assemble_line        ( i4  tokens,
							  char *buffer ) ;
#endif /* assemble_c */
	/* ------------------------------------------------------- */
#ifdef testcase_c
                     STATUS        testcase_start       ( char **argv ) ;
		     STATUS        testcase_end         ( i4  noname_flag ) ;
		     STATUS        testcase_init        ( TEST_CASE **tc,
							  char *tc_name ) ;
		     STATUS        testcase_trace       ( FILE *fptr,
							  TEST_CASE *tcroot,
							  char *suffix ) ;
                     STATUS        testcase_startup     () ;
		     STATUS        testcase_logdiff     ( i4  nr_of_diffs ) ;
		     i4            testcase_sumdiffs    () ;
		     TEST_CASE    *testcase_find        ( TEST_CASE *tcroot,
							  char *tc_name,
							  STATUS *ret_status ) ;
		     TEST_CASE    *testcase_last        () ;
		     STATUS        testcase_writefile   () ;
		     STATUS        testcase_tos         ( TEST_CASE *tcptr,
							  char *str_ptr ) ;
		     STATUS        testcase_froms       ( char *str_ptr,
							  TEST_CASE *tcptr ) ;
		     STATUS        testcase_merge       ( TEST_CASE **tc_root,
							  TEST_CASE **tc_new ) ;
#else
        FUNC_EXTERN  STATUS        testcase_start       ( char **argv ) ;
	FUNC_EXTERN  STATUS        testcase_end         ( i4  noname_flag ) ;
#ifdef sepdefs_h
	FUNC_EXTERN  STATUS        testcase_init        ( TEST_CASE **tc,
                                                          char *tc_name ) ;
	FUNC_EXTERN  STATUS        testcase_trace       ( FILE *fptr,
							  TEST_CASE *tcroot,
							  char *suffix ) ;
	FUNC_EXTERN  TEST_CASE    *testcase_find        ( TEST_CASE *tcroot,
							  char *tc_name,
							  STATUS *ret_status ) ;
	FUNC_EXTERN  TEST_CASE    *testcase_last        () ;
	FUNC_EXTERN  STATUS        testcase_tos         ( TEST_CASE *tcptr,
							  char *str_ptr ) ;
	FUNC_EXTERN  STATUS        testcase_froms       ( char *str_ptr,
							  TEST_CASE *tcptr ) ;
	FUNC_EXTERN  STATUS        testcase_merge       ( TEST_CASE **tc_root,
							  TEST_CASE **tc_new ) ;
#endif /* sepdefs_h */
	FUNC_EXTERN  STATUS        testcase_startup     () ;
	FUNC_EXTERN  STATUS        testcase_logdiff     ( i4  nr_of_diffs ) ;
	FUNC_EXTERN  i4            testcase_sumdiffs    () ;
	FUNC_EXTERN  STATUS        testcase_writefile   () ;
#endif /* testcase_c */
	/* ------------------------------------------------------- */
#ifdef shellcmd_c
                     STATUS        shell_cmmd           ( char *cmmd ) ;
#else
        FUNC_EXTERN  STATUS        shell_cmmd           ( char *cmmd ) ;
#endif /* shellcmd_c */
	/* ------------------------------------------------------- */
#ifdef getcmd_c
                     STATUS        get_command          ( FILE *fdesc,
							  char *cmmdkey,
							  char print ) ;
#else
#ifdef SI_INCLUDED
        FUNC_EXTERN  STATUS        get_command          ( FILE *fdesc,
							  char *cmmdkey,
							  char print ) ;
#endif /* SI_INCLUDED */
#endif /* getcmd_c */
        /* ------------------------------------------------------- */
#ifdef displine_c
                     VOID          Disp_Line            ( WINDOW *mainW,
							  bool batch_mode,
							  i4  *scr_line,
                                                          bool paging,
							  char *lineptr,
							  i4  row,
							  i4  column ) ;
                     VOID          Disp_Line_no_refresh ( WINDOW *mainW,
							  bool batch_mode,
							  i4  *scr_line,
							  char *lineptr,
							  i4  row,
							  i4  column ) ;
#else
#ifdef WINDOW
	FUNC_EXTERN  VOID          Disp_Line            ( WINDOW *mainW,
							  bool batch_mode,
							  i4  *scr_line,
							  bool paging,
							  char *lineptr,
							  i4  row,
							  i4  column ) ;
	FUNC_EXTERN  VOID          Disp_Line_no_refresh ( WINDOW *mainW,
							  bool batch_mode,
							  i4  *scr_line,
							  char *lineptr,
							  i4  row,
							  i4  column ) ;
#endif /* WINDOW */
#endif /* displine_c */
	/* ------------------------------------------------------- */
#ifdef funckeys_c
                     STATUS        getNewFKey           ( FUNCKEYS **newfk,
							  char *label,
							  char *message ) ;
#else
        FUNC_EXTERN  STATUS        readMapFile          ( char *message ) ;
#ifdef sepdefs_h
        FUNC_EXTERN  STATUS        getNewFKey           ( FUNCKEYS **newfk,
							  char *label,
							  char *message ) ;
#endif /* sepdefs_h */
#endif /* funckeys_c */
        /* ------------------------------------------------------- */
#ifdef sepfiles_c
                     STATUS        SEPopen              ( LOCATION *fileloc,
							  i4  buffSize,
							  SEPFILE  **fileptr ) ;
                     STATUS        SEPclose             ( SEPFILE *fileptr ) ;
                     VOID          SEPtranslate         ( char *buffer,
							  i4  buff_len ) ;
                     i4            SEPgetc              ( SEPFILE *fileptr ) ;
#else
#ifdef sepdefs_h
        FUNC_EXTERN  STATUS        SEPopen              ( LOCATION *fileloc,
							  i4  buffSize,
							  SEPFILE  **fileptr ) ;
        FUNC_EXTERN  STATUS        SEPclose             ( SEPFILE *fileptr ) ;
        FUNC_EXTERN  STATUS        SEPrewind            ( SEPFILE *fileptr,
							  bool flushing ) ;
        FUNC_EXTERN  VOID          SEPtranslate         ( char *buffer,
							  i4  buff_len ) ;
        FUNC_EXTERN  STATUS        SEPputrec            ( char *buffer,
							  SEPFILE *fileptr ) ;
        FUNC_EXTERN  STATUS        SEPputc              ( i4  achar,
							  SEPFILE *fileptr ) ;
        FUNC_EXTERN  STATUS        SEPgetrec            ( char *buffer,
							  SEPFILE *fileptr ) ;
        FUNC_EXTERN  i4            SEPgetc              ( SEPFILE *fileptr ) ;
        FUNC_EXTERN  STATUS        SEPfilesInit         ( char *pidstring,
							  char *message ) ;
#endif /* sepdefs_h */

        FUNC_EXTERN  VOID          SEPtranslate         ( char *buffer,
							  i4  buff_len ) ;
        FUNC_EXTERN  STATUS        SEPfilesInit         ( char *pidstring,
							  char *message ) ;
#endif /* sepfiles_c */
        /* ------------------------------------------------------- */
#ifdef utility2_c
                     VOID          SEP_TRACE_exp_eval ( expression_node *exp ) ;
		     VOID          SEP_TRACE_operand  ( char *prefix,
							expression_node *exp ) ;
                     VOID          SEP_TRACE_exp_result ( char *prefix,
							  i4  exp_result ) ;
                     VOID          SEP_exp_init       ( expression_node *exp ) ;
                     VOID          SEP_TRACE_exp_oper   ( char *prefix,
							  char *exp_operator,
                                                          i4  exp_oper_code ) ;
                     VOID          SEP_Vers_Init        ( char *suffix,
							  char **vers_ion,
                                                          char **plat_form ) ;
                     i4            SEPEVAL_alnum      ( expression_node *exp ) ;
                     i4            SEPEVAL_opr_expr   ( expression_node *exp ) ;
                     i4            SEPEVAL_expr_opr_expr( expression_node *exp ) ;
                     i4            SEPEVAL_expr_plus_expr( expression_node *exp ) ; 
                     i4            SEPEVAL_expr_minus_expr( expression_node *exp ) ;
                     i4            SEPEVAL_expr_mult_expr( expression_node *exp ) ;
                     i4            SEPEVAL_expr_div_expr( expression_node *exp ) ;
                     i4            SEPEVAL_expr_equ_expr( expression_node *exp ) ;
                     i4            SEPEVAL_expr_lt_expr ( expression_node *exp ) ;
                     i4            SEP_exp_eval       ( expression_node *exp ) ;
                     i4            SEP_cmp_oper         ( char *op1,
							  char *op2 ) ;
                     i4            SEP_opr_what         ( char *oper,
							  i4  *grp ) ;
                     STATUS        SEP_STclassify       ( char *str,
							  i4  *result_ptr,
							  i4  *nat_ptr,
							  f8 *f8_ptr ) ;

#define SEPEVAL_flip_it(e) \
{ if (e->exp_result == TRUE) e->exp_result = FALSE;	\
  else if (e->exp_result == FALSE) e->exp_result = TRUE; }

#else
#ifdef sepdefs_h
        FUNC_EXTERN  STATUS        SEP_cmd_line         ( i4  *argc,
							  char **argv,
							  OPTION_LIST *opt,
							  char *opt_prefix) ;
        FUNC_EXTERN  VOID          SEP_alloc_Opt        ( OPTION_LIST **opthead,
							  char *list,
							  char *retstr,
							  bool *active,
                                                          i4  *retnat,
							  f8 *retfloat,
							  bool arg_req ) ;
#endif /* sepdefs_h */

        FUNC_EXTERN  VOID          SEP_Vers_Init        ( char *suffix,
							  char **vers_ion,
							  char **plat_form ) ;
        FUNC_EXTERN  i4		   SEP_Eval_If          ( char *Tokenstr,
							  STATUS *err_code,
							  char *err_msg ) ;
        FUNC_EXTERN  i4            SEP_cmp_oper         ( char *op1,
							  char *op2 ) ;
        FUNC_EXTERN  i4            SEP_opr_what         ( char *oper,
							  i4  *grp ) ;
        FUNC_EXTERN  bool          IS_ValueNotation     ( char *varnm ) ;
        FUNC_EXTERN  STATUS        Trans_SEPvar         ( char **Token,
							  bool Dont_free,
							  i2 ME_tag ) ;
        FUNC_EXTERN  STATUS        SEP_STclassify       ( char *str,
							  bool *bool_ptr,
							  i4  *nat_ptr,
							  f8 *f8_ptr ) ;


#endif /* utility2_c */
        /* ------------------------------------------------------- */
#ifdef utility_c
                     VOID          disp_d_line          ( char *lineptr,
							  int row,
							  int column ) ;
                     STATUS        page_down            ( i4  update ) ;
                     STATUS        edit_file            ( char *fname ) ;
                     VOID          get_d_answer         ( char *question,
							  char *ansbuff ) ;
                     STATUS        append_file          ( char *filename ) ;
                     STATUS        append_sepfile       ( SEPFILE  *sepptr ) ;
                     STATUS        append_line          ( char *aline,
							  i4  newline ) ;
                     STATUS        disp_prompt          ( char *buffer,
							  char *achar,
							  char *range ) ;
                     bool          ask_for_conditional  ( WINDOW *askW,
							  char edCAns ) ;
                     VOID          put_message          ( WINDOW *wndow,
							  char *aMessage ) ;
                     VOID          get_string           ( WINDOW *wndow,
							  char *question,
							  char *ansbuff ) ;
                     STATUS        SEPprintw            ( WINDOW *win,
							  i4  y, i4  x,
							  char *bufr ) ;

#else
        FUNC_EXTERN  STATUS        disp_diff            ( STATUS acmmt,
							  i4  canons ) ;
        FUNC_EXTERN  VOID          disp_d_line          ( char *lineptr,
							  int row,
							  int column ) ;
        FUNC_EXTERN  STATUS        page_down            ( i4  update ) ;
        FUNC_EXTERN  STATUS        edit_file            ( char *fname ) ;
        FUNC_EXTERN  VOID          put_d_message        ( char *aMessage ) ;
        FUNC_EXTERN  STATUS        open_log             ( char *testPrefix,
							  char *testSufix,
							  char *username,
							  char *errbuff ) ;
        FUNC_EXTERN  VOID          history_time         ( char *timeStr ) ;
        FUNC_EXTERN  STATUS        append_file          ( char *filename ) ;

#ifdef sepdefs_h
        FUNC_EXTERN  STATUS        append_sepfile       ( SEPFILE  *sepptr ) ;
#endif /* sepdefs_h */

        FUNC_EXTERN  STATUS        append_line          ( char *aline,
							  i4  newline ) ;
        FUNC_EXTERN  STATUS        disp_prompt          ( char *buffer,
							  char *achar,
							  char *range ) ;
#ifdef WINDOW
        FUNC_EXTERN  bool          ask_for_conditional  ( WINDOW *askW,
							  char edCAns ) ;
        FUNC_EXTERN  VOID          put_message          ( WINDOW *wndow,
							  char *aMessage ) ;
        FUNC_EXTERN  VOID          get_string           ( WINDOW *wndow,
							  char *question,
							  char *ansbuff ) ;
        FUNC_EXTERN  STATUS        SEPprintw            ( WINDOW *win,
							  i4  y, i4  x,
							  char *bufr ) ;
#endif /* WINDOW */
#endif /* utility_c */
        /* ------------------------------------------------------- */
#ifdef comments_c
                     STATUS        bld_cmtName          ( char *errmsg ) ;
		     STATUS        skip_comment         ( FILE *aFile,
							  char *cbuff, 
							  char preprint ) ;
                     STATUS        form_comment         ( FILE *aFile ) ;
#else
	FUNC_EXTERN  STATUS        bld_cmtName          ( char *errmsg ) ;
#ifdef SI_INCLUDED
        FUNC_EXTERN  STATUS        skip_comment         ( FILE *aFile,
							  char *cbuff,
							  char preprint ) ;
        FUNC_EXTERN  STATUS        form_comment         ( FILE *aFile ) ;
#endif /* SI_INCLUDED */
#endif /* comments_c */
        /* ------------------------------------------------------- */
#ifdef procmmd_c
                     STATUS        process_if           ( FILE *testFile,
							  char *commands[],
							  i4  cmmdID ) ;
                     VOID          endOfQuery           ( TCFILE *tcinptr,
							  char *errprefx ) ;
                     STATUS        disp_res             ( SEPFILE *aFile ) ;
                     STATUS        form_canon           ( FILE *aFile,
							  i4  *canons,
							  char **open_args,
							  char **close_args ) ;
                     STATUS        next_canon           ( FILE *aFile,
							  bool logit,
							  bool *err_or ) ;
                     STATUS        skip_canons          ( FILE *aFile,
							  bool logit ) ;
                     STATUS        diff_canon           ( i4  *diffound ) ;
                     STATUS        append_obj           ( char *banner,
							  char *diffout,
							  SEPFILE *filptr,
							  char *ebanner ) ;
                     STATUS        del_file             ( char *afile ) ;
		     STATUS        del_floc             ( LOCATION *floc ) ;
                     STATUS        disconnectTClink     ( char *pidstring ) ;
                     STATUS        getnxtkey            ( char *inbuf,
							  i4  *inchars ) ;
#else
#ifdef SI_INCLUDED
        FUNC_EXTERN  STATUS        process_ostcl        ( FILE *testFile,
							  char *command ) ;
        FUNC_EXTERN  STATUS        process_tm           ( FILE *testFile,
							  char *command ) ;
        FUNC_EXTERN  STATUS        process_if           ( FILE *testFile,
							  char *commands[],
							  i4  cmmdID ) ;
        FUNC_EXTERN  STATUS        process_fe           ( FILE *testFile,
							  char *command ) ;
#endif /* SI_INCLUDED */

#ifdef TC_BAD_SYNTAX
        FUNC_EXTERN  VOID          endOfQuery           ( TCFILE *tcinptr,
							  char *errprefx ) ;
#endif /* TC_BAD_SYNTAX */

#ifdef sepdefs_h
        FUNC_EXTERN  STATUS        disp_res             ( SEPFILE *aFile ) ;
        FUNC_EXTERN  STATUS        append_obj           ( char *banner,
							  char *diffout,
							  SEPFILE *filptr,
							  char *ebanner ) ;
#endif /* sepdefs_h */

#ifdef SI_INCLUDED
        FUNC_EXTERN  STATUS        form_canon           ( FILE *aFile,
							  i4  *canons,
							  char **open_args,
							  char **close_args ) ;
        FUNC_EXTERN  STATUS        next_canon           ( FILE *aFile,
							  bool logit,
							  bool *err_or ) ;
        FUNC_EXTERN  STATUS        skip_canons          ( FILE *aFile,
							  bool logit ) ;
#endif /* SI_INCLUDED */
        FUNC_EXTERN  STATUS        diff_canon           ( i4  *diffound ) ;
        FUNC_EXTERN  STATUS        del_file             ( char *afile ) ;
	FUNC_EXTERN  STATUS        del_floc             ( LOCATION *floc ) ;
        FUNC_EXTERN  STATUS        disconnectTClink     ( char *pidstring ) ;
        FUNC_EXTERN  STATUS        getnxtkey            ( char *inbuf,
							  i4  *inchars ) ;
#endif /* procmmd_c */
	/* ------------------------------------------------------- */
#ifdef isitif_c
                     i4            Is_it_If_statement   ( FILE *testFile,
							  i4  *counter,
							  bool Ignore_it ) ;
#else
        FUNC_EXTERN  i4            Is_it_If_statement   ( FILE *testFile,
							  i4  *counter,
							  bool Ignore_it ) ;
#endif
	/* ------------------------------------------------------- */
#ifdef crestf_c
		     STATUS        Create_stf_file      ( i2 ME_tag,
							  char *prefix,
							  char **fname,
							  LOCATION **floc,
							  char *errmsg ) ;
#else
	FUNC_EXTERN  STATUS        Create_stf_file      ( i2 ME_tag,
							  char *prefix,
							  char **fname,
							  LOCATION **floc,
							  char *errmsg ) ;
#endif /* crestf_c */
        /* ------------------------------------------------------- */
#ifdef readtc_c
                     i4            Read_TCresults       ( TCFILE *tcoutptr,
							  TCFILE *tcinptr,
							  i4  waiting,
							  bool *fromBOS,
                                                          i4  *sendEOQ_ptr,
							  bool waitforit,
							  STATUS *tcerr ) ;
#else
#ifdef TC_BAD_SYNTAX
        FUNC_EXTERN  i4            Read_TCresults       ( TCFILE *tcoutptr,
							  TCFILE *tcinptr,
							  i4  waiting,
							  bool *fromBOS,
                                                          i4  *sendEOQ_ptr,
							  bool waitforit,
							  STATUS *tcerr ) ;
#endif /* TC_BAD_SYNTAX */
#endif /* readtc_c */
	/* ------------------------------------------------------- */
#ifdef evalif_c
                     bool          Eval_If              ( char *commands[] ) ;
#else
        FUNC_EXTERN  bool          Eval_If              ( char *commands[] ) ;
#endif /* evalif_c */
	/* ------------------------------------------------------- */
#ifdef diffing_c
                     STATUS        diffing              ( FILE *testFile,
							  i4  prtRes,
							  i4  getprompt ) ;
                     STATUS        Prt_Diff_msg         ( bool wrong, char *exp,
							  i4  cnr, char *buf,
							  i4  prt, i4  getp ) ;
#else
#ifdef SI_INCLUDED
        FUNC_EXTERN  STATUS        diffing              ( FILE *testFile,
							  i4  prtRes,
							  i4  getprompt ) ;
#endif /* SI_INCLUDED */
#endif /* diffing_c */
	/* ------------------------------------------------------- */
#ifdef stillcon_c
                     STATUS        Still_want_have_connection
							( char *cmd,
							  STATUS *ierr ) ;
#else
        FUNC_EXTERN  STATUS        Still_want_have_connection
							( char *cmd,
							  STATUS *ierr ) ;
#endif /* stillcon_c */
	/* ------------------------------------------------------- */
#ifdef opncomm_c
                     STATUS        open_io_comm_files   ( bool display_it,
							  STATUS tclink ) ;
#else
        FUNC_EXTERN  STATUS        open_io_comm_files   ( bool display_it,
							  STATUS tclink ) ;
#endif /* opncomm_c */
	/* ------------------------------------------------------- */
#ifdef crecanon_c
                     STATUS        create_canon         ( SEPFILE *res_ptr ) ;
                     STATUS        create_canon_fe      ( SEPFILE *res_ptr ) ;
#else
#ifdef sepdefs_h
        FUNC_EXTERN  STATUS        create_canon         ( SEPFILE *res_ptr ) ;
        FUNC_EXTERN  STATUS        create_canon_fe      ( SEPFILE *res_ptr ) ;
#endif /* sepdefs_h */
#endif /* crecanon_c */
	/* ------------------------------------------------------- */
#ifdef sepostcl_c
                     STATUS        do_delete_cmmd       ( char *filenm ) ;
                     STATUS        exists_cmmd          ( char *filenm ) ;
                     STATUS        cd_cmmd              ( char *directory ) ;
                     STATUS        setenv_cmmd          ( char *logical,
							  char *value,
							  bool bflag ) ;
		     STATUS        unsetenv_cmmd        ( char *logical,
							  bool bflag ) ;
                     STATUS        type_cmmd            ( char *filenm ) ;
                     STATUS        SEP_SET_read_envvar  ( char *envvar ) ;
                     STATUS        sepset_cmmd          ( char *argv,
							  char *argv2,
							  char *argv3 ) ;
                     STATUS        unsepset_cmmd        ( char *argv,
							  char *argv2,
							  char *argv3 ) ;
                     STATUS        fill_cmmd            ( FILE *testFile,
							  char *filenm ) ;
#else
#ifdef SI_INCLUDED
        FUNC_EXTERN  STATUS        sep_ostcl            ( FILE  *testFile,
							  char **argv,
							  i4  cmmdID ) ;
        FUNC_EXTERN  STATUS        fill_cmmd            ( FILE *testFile,
							  char *filenm ) ;
#endif /* SI_INCLUDED */
        FUNC_EXTERN  STATUS        do_delete_cmmd       ( char *filenm ) ;
        FUNC_EXTERN  STATUS        exists_cmmd          ( char *filenm ) ;
        FUNC_EXTERN  STATUS        cd_cmmd              ( char *directory ) ;
        FUNC_EXTERN  STATUS        setenv_cmmd          ( char *logical,
							  char *value,
							  bool bflag ) ;
	FUNC_EXTERN  STATUS        unsetenv_cmmd        ( char *logical,
							  bool bflag ) ;
        FUNC_EXTERN  STATUS        type_cmmd            ( char *filenm ) ;
        FUNC_EXTERN  STATUS        SEP_SET_read_envvar  ( char *envvar ) ;
        FUNC_EXTERN  STATUS        sepset_cmmd          ( char *argv,
							  char *argv2,
							  char *argv3 ) ;
        FUNC_EXTERN  STATUS        unsepset_cmmd        ( char *argv,
							  char *argv2,
							  char *argv3 ) ;
#endif /* sepostcl_c */
        /* ------------------------------------------------------- */
#ifdef sepparm_c
                     char         *FIND_SEPparam        ( char *token ) ;
#else
        FUNC_EXTERN  STATUS        Trans_SEPparam       ( char **Token,
							  bool Dont_free,
							  i2 ME_tag ) ;
        FUNC_EXTERN  bool          IS_SEPparam          ( char *token ) ;
        FUNC_EXTERN  char         *FIND_SEPparam        ( char *token ) ;
#endif /* sepparm_c */
        /* ------------------------------------------------------- */
#ifdef termcap_c
                     STATUS        insert_key_node      ( KEY_NODE **master,
							  KEY_NODE *anode ) ;
                     STATUS        decode               ( char *source,
							  char *dest ) ;
                     i4            comp_seqs            ( char *seq1,
							  char *seq2 ) ;
                     STATUS        get_key_value        ( KEY_NODE *master,
							  char *label ) ;
                     STATUS        get_key_label        ( KEY_NODE *master,
							  char **value,
							  char *label ) ;
#else
        FUNC_EXTERN  STATUS        load_termcap         ( char *pathname,
							  char *errmsg ) ;
        FUNC_EXTERN  STATUS        decode               ( char *source,
							  char *dest ) ;
        FUNC_EXTERN  i4            comp_seqs            ( char *seq1,
							  char *seq2 ) ;
        FUNC_EXTERN  VOID          decode_seqs          ( char *source,
							  char *dest ) ;
        FUNC_EXTERN  VOID          encode_seqs          ( char *source,
							  char *dest ) ;

#ifdef sepdefs_h
        FUNC_EXTERN  STATUS        insert_key_node      ( KEY_NODE **master,
							  KEY_NODE *anode ) ;
        FUNC_EXTERN  STATUS        get_key_value        ( KEY_NODE *master,
							  char *label ) ;
        FUNC_EXTERN  STATUS        get_key_label        ( KEY_NODE *master,
							  char **value,
							  char *label ) ;
#endif /* sepdefs_h */
#endif /* termcap_c */
        /* ------------------------------------------------------- */
#ifdef trantok_c
                     STATUS        Trans_Token          ( i2 ME_tag,
							  char **Token,
							  bool *chg_token,
							  i4  chg_mask ) ;
#else
        FUNC_EXTERN  STATUS        Trans_Tokens         ( i2 ME_tag,
							  char *Tokens[],
							  bool *chg_tokens ) ;
        FUNC_EXTERN  STATUS        Trans_Token          ( i2 ME_tag,
							  char **Token,
							  bool *chg_token,
							  i4  chg_mask ) ;
#endif /* trantok_c */
        /* ------------------------------------------------------- */
#ifdef getloc_c
                     STATUS        getLocation          ( char *string,
							  char *newstr,
							  LOCTYPE *typeOfLoc ) ;
#else
        FUNC_EXTERN  STATUS        getLocation          ( char *string,
							  char *newstr,
							  LOCTYPE *typeOfLoc ) ;
        FUNC_EXTERN  bool          IS_FileNotation      ( char *filenm ) ;
        FUNC_EXTERN  STATUS        Trans_SEPfile        ( char **Token,
							  LOCTYPE *typeOfLoc,
							  bool Dont_free,
							  i2 ME_tag ) ;
#endif /* getloc_c */
        /* ------------------------------------------------------- */
#ifndef sepcm_c
        FUNC_EXTERN  i4            SEP_CMstlen          ( char *str,
							  i4  maxlen ) ;
        FUNC_EXTERN  char         *SEP_CMlastchar       ( char *str,
							  i4  maxlen ) ;
        FUNC_EXTERN  char         *SEP_CMpackwords      ( i2 ME_tag,
							  i4  max_aray,
							  char **aray ) ;
        FUNC_EXTERN  i4            SEP_CMgetwords       ( i2 ME_tag,
							  char *src,
							  i4  max_aray,
							  char **aray ) ;
	FUNC_EXTERN  i4            SEP_CMwhatcase       ( char *str ) ;
#endif /* sepcm_c */
        /* ------------------------------------------------------- */
#ifdef seplo_c
                     bool          SEP_LOexists         ( LOCATION *aloc ) ;
                     STATUS        SEP_LOcompose        ( char *SEPLOdev,
							  char *SEPLOpath,
							  char *SEPLOfpre,
							  char *SEPLOfsuf,
                                                          char *SEPLOfver,
							  LOCATION *locp ) ;
                     LOCTYPE       SEP_LOwhatisit       ( char *f_string ) ;
                     bool          SEP_LOisitaPath      ( char *f_string ) ;
#else
        FUNC_EXTERN  STATUS        SEP_LOrebuild        ( LOCATION *locptr ) ;
        FUNC_EXTERN  bool          SEP_LOexists         ( LOCATION *aloc ) ;
        FUNC_EXTERN  STATUS        SEP_LOffind          ( char *path[],
							  char *filename,
							  char *fileBuffer,
							  LOCATION *fileloc ) ;
        FUNC_EXTERN  STATUS        SEP_LOfroms          ( LOCTYPE flag,
							  char *string,
							  LOCATION *loc,
							  i2 ME_tag ) ;
        FUNC_EXTERN  STATUS        SEP_LOcompose        ( char *SEPLOdev,
							  char *SEPLOpath,
							  char *SEPLOfpre,
							  char *SEPLOfsuf,
                                                          char *SEPLOfver,
							  LOCATION *locp ) ;
        FUNC_EXTERN  LOCTYPE       SEP_LOwhatisit       ( char *f_string ) ;
        FUNC_EXTERN  bool          SEP_LOisitaPath      ( char *f_string ) ;
#endif /* seplo_c */

        FUNC_EXTERN  VOID          SEP_NMgtAt           ( char *name,
							  char **ret_val,
							  i4  mtag ) ;

        /* ------------------------------------------------------- */
#ifndef sepst_c
        FUNC_EXTERN  char         *FIND_SEPstring       ( char *token,
							  char *string ) ;
#endif /* sepst_c */
        /* ------------------------------------------------------- */
#ifdef septrace_c
                     STATUS        SEP_Trace_Set        ( char *argv2 ) ;
#else
        FUNC_EXTERN  STATUS        SEP_Trace_Set        ( char *argv2 ) ;
        FUNC_EXTERN  STATUS        SEP_Trace_Clr        ( char *argv2 ) ;
#ifdef TC_BAD_SYNTAX
        FUNC_EXTERN  i4            SEP_TCgetc_trace     ( TCFILE *tcoutptr,
							  i4  waiting ) ;
#endif /* TC_BAD_SYNTAX */
#endif /* septrace_c */
        /* ======================================================= */
#if defined(UNIX)||defined(NT_GENERIC)
        /* ------------------------------------------------------- */
#ifndef nmsetenv_c
        FUNC_EXTERN  STATUS        NMsetenv             ( char *name,
							  char *value ) ;
        FUNC_EXTERN  STATUS        NMdelenv             ( char *name ) ;
#endif /* nmsetenv_c */
        /* ------------------------------------------------------- */
#ifdef sepalloc_c
                     VOID          SEP_MEprint_tagname  ( i2 tag ) ;
#else
        FUNC_EXTERN  char         *SEP_MEalloc          ( u_i4 tag,
							  u_i4 size,
							  bool clear,
							  STATUS *ME_status ) ;
#endif /* sepalloc_c */
        /* ------------------------------------------------------- */
#ifndef sepspawn_c
#ifdef PID
        FUNC_EXTERN  STATUS        SEPspawn             ( i4  argc,
							  char **argv,
							  bool wait_for_child,
                                                          LOCATION *in_loc,
							  LOCATION *out_loc,
							  PID *pid,
                                                          i4  *exitstat,
							  i4  stderr_action ) ;
#endif /* PID */
#endif /* sepspawn_c */
        /* ------------------------------------------------------- */
#endif /* UNIX */
        /* ======================================================= */
#ifdef VMS
        /* ------------------------------------------------------- */
#ifdef sepalloc_c
                     VOID          SEP_MEprint_tagname  ( i2 tag ) ;
#else
        FUNC_EXTERN  char         *SEP_MEalloc          ( u_i4 tag,
							  u_i4 size,
							  bool clear,
							  STATUS *ME_status ) ;
#endif /* sepalloc_c */
#endif /* VMS */
        /* ======================================================= */

/*
** *****************************************************************
** These functions have no arguments
** *****************************************************************
*/

#ifndef sepfiles_c
FUNC_EXTERN  STATUS        SEPfilesTerminate    () ;
#endif

#ifdef utility_c
             STATUS        page_up              () ;
             STATUS        display_lines        () ;
#else
FUNC_EXTERN  STATUS        page_up              () ;
FUNC_EXTERN  STATUS        display_lines        () ;
FUNC_EXTERN  STATUS        close_log            () ;
FUNC_EXTERN  STATUS        del_log              () ;
FUNC_EXTERN  STATUS        fix_cursor           () ;
FUNC_EXTERN  STATUS        show_cursor          () ;
FUNC_EXTERN  STATUS        hide_cursor          () ;
#endif

#ifdef comments_c
             STATUS        create_comment       () ;
             STATUS        insert_comment       () ;
             char          do_comments          () ;
#else
FUNC_EXTERN  STATUS        create_comment       () ;
FUNC_EXTERN  STATUS        insert_comment       () ;
FUNC_EXTERN  char          do_comments          () ;
#endif

#ifdef procmmd_c
             VOID          Encode_kfe_into_buffer () ;
#else
FUNC_EXTERN  VOID          Encode_kfe_into_buffer () ;
#endif

#ifndef sepostcl_c
FUNC_EXTERN  VOID          del_fillfiles        () ;
#endif

#ifdef septool_c
             STATUS        undo_env_chain       () ;
#else
FUNC_EXTERN  STATUS        undo_env_chain       () ;
#endif

#ifdef sepalloc_c
             VOID          SEP_MEtrace_Table    () ;
#else
FUNC_EXTERN  VOID          SEP_MEtrace_Table    () ;
#endif

#ifndef septrace_c
FUNC_EXTERN  STATUS        SEP_Trace_Open       () ;
#endif

/*
** *****************************************************************
** The End
** *****************************************************************
*/
