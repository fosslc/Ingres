/*
** Copyright (c) 2004 Ingres Corporation
*/

/*	static char	Sccsid[] = "@(#)sglobi.h	1.1	1/24/85";	*/

# include	 <rglobi.h> 

/*
**	Global variables to the report formatter.
**	
**	History:
**	05-apr-90 (sylviap)
**		Added St_width_given. (#129, #588)
**      7/30/90 (elein)
**              Add global sequence number for .declares
**	23-aug-90 (sylviap)
**		Moved Seq_declare to rglobi.h. (#32780)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-aug-2004 (lazro01)
**	        BUG 112819
**	        Changed Line_num to accept values greater than 32K.
*/


/*
**	Additional Current Action stuff.  (See rglobi.h for more)
*/

	GLOBALDEF RSO	*Cact_rso = {0};	/* Current RSO structure */
	GLOBALDEF REN	*Cact_ren = {0};	/* current REN structure */
	GLOBALDEF SBR	*Cact_sbr = {0};	/* current SBR structure */


/*
**	Additional environment variables.
*/

	GLOBALDEF char	*En_sfile = "";		/* ptr to file name of text file */



/*
**	Additional Status Variables.  (See rglobi.h for more)
*/

	GLOBALDEF bool	St_q_started = FALSE;	/* started query for this report */
	GLOBALDEF bool	St_o_given = FALSE;	/* specified .output for this */
	GLOBALDEF bool	St_s_given = FALSE;	/* specified if .sort given */
	GLOBALDEF bool	St_b_given = FALSE;	/* specified if .break given */
	GLOBALDEF bool	St_sr_given = FALSE;	/* specified if .shortrem */
	GLOBALDEF bool	St_lr_given = FALSE;	/* specified if .longrem */
	GLOBALDEF bool	St_d_given = FALSE;	/* specified if .declare  */
	GLOBALDEF bool	St_setup_given = FALSE;	/* specified if .setup  */
	GLOBALDEF bool	St_clean_given = FALSE;	/* specified if .cleanup  */
	GLOBALDEF bool	St_width_given = FALSE;	/* specified if .pagewidth  */

/*
**	Additional Pointer variables.
*/

	GLOBALDEF REN	*Ptr_ren_top = {0};	/* top of REN linked list */


/*
**	Places to Hold commands, etc.
*/

	GLOBALDEF char	Line_buf[MAXLINE+1] = {0};	/* maximum input line */

	GLOBALDEF i4	Csequence = 0;			/* current rcosequence */
	GLOBALDEF char	Ctype[MAXRNAME+1] = {0};	/* name of type */
	GLOBALDEF char	Csection[MAXRNAME+1] = {0};	/* section name */
	GLOBALDEF char	Cattid[FE_MAXNAME+1] = {0};	/* attribute name */
	GLOBALDEF char	Ccommand[MAXRNAME+1] = {0};	/* command (for actions) */
	GLOBALDEF char	Ctext[MAXRTEXT+1] = {0};	/* maximum rcotext line */
	GLOBALDEF char	*Argvs[MAXEARG+1] = {0};	/* arguments to error routine */

/*
**	Special to the Report Specifier
*/

	GLOBALDEF i4	Line_num = 0;			/* line number in text file */
