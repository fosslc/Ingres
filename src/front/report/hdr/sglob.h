/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	sglob.h -	Report Specifier Global Declarations.
**
** History:
** 	05-apr-90 (sylviap)
**		Added St_width_given. (#129, #588)
**	7/30/90 (elein)
**		Add global sequence number for .declares
** 	23-aug-90 (sylviap)
**		Moved Seq_declare to rglob.h (#32780)
**	15-oct-90 (steveh)
**		Declared new function make_En_rcommands (#32954)
**	26-aug-1992 (rdrane)
**		Add function declarations for
**			STATUS	s_w_row()
**		Delete function declarations for
**			DB_DT_ID s_g_blprimary()
**			DB_DT_ID s_g_blelement()
**			DB_DT_ID s_g_aexp()
**			DB_DT_ID s_g_arterm()
**			DB_DT_ID s_g_arfactor()
**			DB_DT_ID s_g_arprimary()
**			DB_DT_ID s_g_aelement()
**		(all static to sgexpr.c).
**		Alphabetize the list of function declarations.
**	16-sep-1992 (rdrane)
**		Add declarations for s_g_ident() and s_g_tbl().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-aug-2004 (lazro01)
**	        BUG 112819
**	        Changed Line_num to accept values greater than 32K.
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# define	FOR_SREPORT	1

# include	<rglob.h>

/*
**	Global variables to the report formatter.
*/


/*
**	Additional Current Action stuff.  (See rglobi.h for more)
*/

	extern	RSO	*Cact_rso;		/* Current RSO structure */
	extern	REN	*Cact_ren;		/* current REN structure */
	extern	SBR	*Cact_sbr;		/* current SBR structure */


/*
**	Additional environment variables.
*/

	extern	char	*En_sfile;		/* ptr to file name of text file */



/*
**	Additional Status Variables.  (See rglobi.h for more)
*/

	extern	bool	St_q_started;		/* started query for this report */
	extern	bool	St_o_given;		/* specified OUTPUT for this */
	extern	bool	St_s_given;		/* specified if Sort given */
	extern	bool	St_b_given;		/* specified if Break given */
	extern	bool	St_sr_given;		/* specified if .shortrem */
	extern	bool	St_lr_given;		/* specified if .longrem */
	extern	bool	St_d_given;		/* specified if .declare */
	extern	bool	St_setup_given;		/* specified if .setup */
	extern	bool	St_clean_given;		/* specified if .cleanup */
	extern	bool	St_width_given;		/* specified if .pagelength */
/*
**	Additional Pointer variables.
*/

	extern	REN	*Ptr_ren_top;		/* top of REN linked list */


/*
**	Places to Hold commands, etc.
*/

	extern	char	Line_buf[MAXLINE+1];	/* maximum input line */

	extern	i4	Csequence;			/* current rcosequence */
	extern	char	Ctype[MAXRNAME+1];		/* name of type */
	extern	char	Csection[MAXRNAME+1];		/* section name */
	extern	char	Cattid[FE_MAXNAME+1];		/* attribute name */
	extern	char	Ccommand[MAXRNAME+1];		/* command (for actions) */
	extern	char	Ctext[MAXRTEXT+1];		/* maximum rcotext line */
	extern	char	*Argvs[MAXEARG+1];		/* arguments to error routine */

/*
**	Special to the Report Specifier
*/

	extern	i4	Line_num;			/* line number in text file */

/*
**	Declarations of Routines.
*/

	VOID	make_En_rcommands();	/* construct En_rcommands file name */
	REN	*s_chk_rep();		/* check for a report */
	char	*s_chk_file();		/* check a file name */
	DB_DT_ID s_g_expr();
	char	*s_g_ident();		/* get a (delimited) identifier */
	ADI_OP_ID s_g_logop();
	char	*s_g_name();		/* get a name */
	char	*s_g_number();
	char	*s_g_op();
	char	*s_g_paren();		/* get parenthesized list */
	i4	s_g_skip();		/* skip white space */
	char	*s_g_string();		/* get a string */
	char	*s_g_tbl();		/* get an owner.tablename */
	char	*s_g_token();		/* get a token */
	i4	s_get_scode();		/* get command code */
	VOID	s_nam_set();		/* read in the name */
	i4	s_q_add();		/* add rows to RCOMMAND */
	i4	s_quer_set();		/* set up query */
	RSO	*s_rso_add();		/* add a sort att */
	RSO	*s_rso_find();		/* find a SORT att */
	RSO	*s_rso_set();		/* set to a SORT att */
	SBR	*s_sbr_add();		/* add a break att */
	SBR	*s_sbr_find();		/* find a break att */
	SBR	*s_sbr_set();		/* set to a break att */
	i4	s_srt_set();		/* set up sort command */
	i4	s_exit();		/* exit routine */
	STATUS	s_w_row();

FUNC_EXTERN	STATUS	IISRsr_saveRcommands();
FUNC_EXTERN	VOID	IISRwds_WidthSet();
FUNC_EXTERN	VOID	crcrack_cmd();
FUNC_EXTERN	VOID	save_em();
FUNC_EXTERN	VOID	get_em();
FUNC_EXTERN	STATUS	s_lrem_set();
FUNC_EXTERN	STATUS	s_srem_set();
FUNC_EXTERN	VOID	s_cmd_skip();
FUNC_EXTERN	VOID	s_error();
FUNC_EXTERN	VOID	s_cat();
FUNC_EXTERN	VOID	s_ren_set();
FUNC_EXTERN	VOID	s_set_cmd();
FUNC_EXTERN	VOID	s_tok_add();
FUNC_EXTERN	VOID	s_include();
FUNC_EXTERN	VOID	s_dcl_set();
FUNC_EXTERN	VOID	s_out_set();
FUNC_EXTERN	VOID	s_dat_set();
FUNC_EXTERN	VOID	s_break();
FUNC_EXTERN	VOID	s_process();
FUNC_EXTERN	VOID	s_brk_set();
FUNC_EXTERN	VOID	s_reset();
FUNC_EXTERN	VOID	s_app_reports();
FUNC_EXTERN	i4	s_next_line();
FUNC_EXTERN	i4	s_setclean();

/*
**	Declaration of routines in COPYREP.
*/

	bool	cr_dmp_rep();		/* dump a set of reports */
	bool	cr_wrt_rep();		/* write out one report */
