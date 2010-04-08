/*
**	Copyright (c) 2004 Ingres Corporation 
*/


/**
** Name:	rcons.h -	Report Formatter Constant Definitions File.
**
**	Constants for Report Formatter
**	--------- --- ------ ---------
**  History:
** 	14-oct-88 (sylviap)
**		Deleted definitions of MAXFILE and LFRM_*.  They were not 
**		being used in the code.
**
**	6/19/89 (elein) garnet
**		Added new RCOTYPE NAM_SETUP and NAM_CLEANUP.
**	12/2/89 (martym) (garnet)
**		Added new report styles and define's needed for template 
**		style default report generation. Also added declared 
**		variables for labels style reports.
** 	06-feb-90 (sylviap)
**		Took out TRM_DFLT.  Not used.
** 	05-apr-90 (sylviap)
**		Added NAM_WIDTH.  (#129, #588)
** 	17-may-90 (sylviap)
**		Added page_width as a user variable. (#958)  
**		Changed MW_DFLT to be based on the maximum number of columns in 
**		   a table instead of 100.  Changed to be able to do a label 
**		   style report on 127 columns w/no errors - ran against max
**		   w/the -w flag. (#916)
**	08-jun-90 (sylviap)
**		Added TTLE_DFLT.  Used in rmblock.c and rfmtdflt.c.
**	25-jul-90 (elein)
**		Increased MQ_DFLT to 2048 from 1000.  Need more space
**		to gracefully accomodate joindef reports created by RBF
**      8/1/90 (elein) 30449
**              Use a set date format for current_date variable.
** 	06-aug-90 (sylviap)
**		Added LABST_IN_BLK. (#32085)
**	9/12/90 (elein) tandem porting bug
**		Change ERRNUM to i4  since we assign a i4  value to it.
** 	06-nov-90 (sylviap)
**		Added the defines WRD_* used in s_q_add. (#33894)
**	14-oct-91 (steveh)
**		Increased MQ_DFLT to 5000 from 2048.  Need more space
**		to gracefully accomodate joindef reports created by RBF.
**	22-may-92 (rdrane) 37360
**		Add default date formats for all recognized DB_***_DFMT values.
**		This extends the fixed date format correction made for bug
**		30449 to the international arena.  Added PRG_DELREP in
**		anticipation of new 6.5 utility.  Add defines for (S)REPORT
**		reset states in anticipation of 6.5 changes to reset
**		invocations (existing numeric constants are not very readable).
**		Altered sense of ifdef on .FORMFEEDS - the negative form was
**		potentially confusing.  Did same for page length.
**	10-jul-92 (rdrane)
**		Since we decided to add forms deletion to DELREP, we're
**		changing its name to DELOBJ, and so we're retiring PRG_DELREP
**		in favor of PRG_DELOBJ.
**	23-sep-1992 (rdrane)
**		Add name of expanded namespace command "group" to the NAM_*
**		#define's.  Extend RP_RESET_* constants to include RBF.
**	25-nov-92 (rdrane)
**		Renamed references to .ANSI92 to .DELIMID as per the LRC.
**		Removed PRG_DELOBJ since that will now be a bona fide
**		separate general utility.
**	17-may-1993 (rdrane)
**		Add support for hex literals (TK_HEXLIT).
**	21-jun-1993 (rdrane)
**		Add support for suppression of initial form feed (C_NO1STFF,
**		P_NO1STFF,RW_NO1STFF_FLAG, RW_1STFF_FLAG).
**	21-jul-1993 (rdrane)
**		Define max FMTSTR lengths for hdr/ftr date/time and
**		page # format variable declarations.
**	18-aug-1993 (rdrane)
**		Define "known" report levels with respect to specific
**		features.
**	07-oct-1998 (rodjo04) Bug 93450
**		Added support for MULTINATIONAL4.
**	23-oct-1998 (rodjo04) Bug 93785
**		Added support for YMD, DMY, and DMY.
**	24-Jan-1999 (shero03)
**	    Support ISO4 date.
**	07-jan-2000 (somsa01)
**	    Added conditional include logic.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**    28-apr-2004 (srisu02)
**          Add constants for new flag "numeric_overflow"
**	14-Nov-2008 (gupsh01)
**	    Added support for -noutf8align flag for report
**	    writer.
*/

#ifndef RCONS_INCLUDE

#define RCONS_INCLUDE

/*
**	Codes for En_program, the program which is being run.
*/

# define	PRG_REPORT	1	/* running REPORT */
# define	PRG_SREPORT	2	/* running SREPORT */
# define	PRG_RBF		3	/* running RBF */
# define	PRG_COPYREP	4	/* running COPYREP */
# define	PRG_DELOBJ	5	/* running DELETE REPORT/FORM (R6.5) */

/*
**	New Type Definitions
*/

# define	ATTRIB	i2	/* attribute numbers fit in i2 nat's now */
# define	ACTION	i2	/* action codes fit in i2 now */
# define	STRING	char	/* ptr to the start of a string */
# define	NUMBER	f8	/* all numbers are f8 precision, now */
# define	COLUMN	i2	/* column number on output page */
# define	ERRNUM	i4	/* error number. */
# define	TFILE	i4	/* unit number for text file access */

/*
**	Useful Constants
*/

# define	ABORT	-1	/* flag for aborting from Report */
# define	NOABORT 0	/* flag for no abort */
# define	FATAL	1	/* flag for fatal errors */
# define	NONFATAL 0	/* flag for non-fatal errors */

/*
**	Codes for attribute ordinals.
*/

# define	A_REPORT	0	/* code for overall report break */
# define	A_DETAIL	-1	/* code for detail break */
# define	A_PAGE		-2	/* code for page break */
# define	A_GROUP		-3	/* code for group of columns, as
					** as used with the .WITHIN com */
# define	A_ERROR		-4	/* code for error */

/*
**	Codes for RTEXT command names
*/

# define	C_ERROR		-1	/* error code */
# define	C_TEXT		0	/* code for print command */
# define	C_ETEXT		2	/* code for end of print */
# define	C_PRINTLN	5	/* code for printline command */
# define	C_NEWLINE	10	/* code for new line */
# define	C_TAB		20	/* code for tab to column n */
# define	C_LM		30	/* code for left margin */
# define	C_RM		40	/* code for right margin */
# define	C_PL		50	/* code for page length */
# define	C_NPAGE		60	/* code for new page */
# define	C_NEED		70	/* code for need */
# define	C_FORMAT	80	/* code for format */
# define	C_TFORMAT	90	/* code for tformat */
# define	C_POSITION	100	/* code for position */
# define	C_WIDTH		105	/* code for width */
# define	C_CENTER	110	/* code for centering */
# define	C_LEFT		120	/* code for right justification */
# define	C_RIGHT		130	/* code for left justification */
# define	C_UL		140	/* code for underline */
# define	C_NOUL		150	/* code for no underline */
# define	C_ULC		160	/* code for underline character */
# define	C_FF		170	/* code for formfeeds */
# define	C_NO1STFF	175	/* code for no initial formfeed */
# define	C_NOFF		180	/* code for no formfeeds */
# define	C_NULLSTR	190	/* code for null string */
# define	C_END		200	/* code for end command */
# define	C_BEGIN		205	/* code for begin command */
# define	C_WITHIN	210	/* begin WITHIN block */
# define	C_EWITHIN	211	/* end WITHIN block */
# define	C_BLOCK		220	/* begin BLOCK */
# define	C_EBLOCK	221	/* end BLOCK */
# define	C_TOP		225	/* go to top of block */
# define	C_BOTTOM	226	/* go to bottom of block */
# define	C_LINEEND	227	/* go to end of current line */
# define	C_LINESTART	228	/* go to start of current line */
# define	C_LET		230	/* code for let */
# define	C_ELET		235	/* code for end of let */
# define	C_RBFSETUP	300	/* start of report setup (RBF) */
# define	C_RBFTITLE	310	/* start of report title (RBF) */
# define	C_RBFHEAD	320	/* start of column head (RBF) */
# define	C_RBFPTOP	330	/* start of page top (RBF) */
# define	C_RBFPBOT	340	/* start of page bottom (RBF) */
# define	C_RBFTRIM	350	/* start of detail trim (RBF) */
# define	C_RBFFIELD	360	/* start of detail field (RBF) */
# define	C_RBFAGGS	370	/* start of aggregates (RBF) */
# define	C_IF		400	/* code for if */
# define	C_THEN		410	/* code for then */
# define	C_ELSE		420	/* code for else */
# define	C_ELSEIF	430	/* code for elseif */
# define	C_ENDIF		440	/* code for endif */


/*
**	Codes for the RTEXT type, which indicates, when printing
**	out the report from the database, the type of command.
*/

# define	RX_NOTPRIM	0	/* not the primary entry */
# define	RX_RBF		1	/* RBF command */
# define	RX_REG		2	/* regular entry, just print */
# define	RX_LRTAB	3	/* left tab when seen, then right tab after this */
# define	RX_RTAB		4	/* right tab after this */
# define	RX_LTAB		5	/* left tab when seen */
# define	RX_BEGIN	6
# define	RX_END		7
# define	RX_PRINT	8
# define	RX_EPRINT	9
# define	RX_IF		10
# define	RX_ELSEIF	11
# define	RX_THEN		12
# define	RX_LET		13
# define	RX_ELET		14



/*
**	Internal codes for RTEXT processing.  These are similar to the
**		RTEXT commands, but are somewhat more detailed for
**		use in the formatting loop.
*/

# define	P_ERROR		-1	/* code for error in command */
# define	P_NOOP		0	/* no operation (for endif) */
# define	P_PRINT		5	/* print */
# define	P_TAB		10	/* default tab (sends \t)   */
# define	P_NEWLINE	20	/* write end-of-line */
# define	P_RM		30	/* change right margin to "n" */
# define	P_LM		32	/* change left margin to "n" */
# define	P_PL		34	/* change page length to "n" */
# define	P_NPAGE		40	/* go to new page (absolute) */
# define	P_POSITION	45	/* set position for a column */
# define	P_WIDTH		47	/* set width for a column */
# define	P_NEED		50	/* need n lines, or page eject */
# define	P_CENTER	60	/* default centering */
# define	P_RIGHT		70	/* default right just */
# define	P_LEFT		80	/* left just to absolute position */
# define	P_UL		90	/* underline on */
# define	P_ULC		95	/* underline character */
# define	P_FF		97	/* formfeeds on */
# define	P_NO1STFF	98	/* suppress initial formfeed */
# define	P_LET		100	/* let command */
# define	P_FORMAT	112	/* set default format for att */
# define	P_TFORMAT	113	/* set temp format for att */
# define	P_AOP		140	/* perform aggregate operation */
# define	P_ACLEAR	150	/* clear out an aggregate accumulator */
# define	P_WITHIN	180	/* start within block */
# define	P_BLOCK		190	/* start keep block */
# define	P_TOP		195	/* go to top of keep block */
# define	P_BOTTOM	196	/* go to bottom of block */
# define	P_LINEEND	197	/* go to end of line */
# define	P_LINESTART	198	/* go to start of line */
# define	P_NULLSTR	199	/* null string */
# define	P_END		200	/* end of block of some sort */
# define	P_BEGIN		205	/* start of block of some sort */
# define	P_RBFSETUP	300	/* start of report setup (RBF) */
# define	P_RBFTITLE	310	/* start of report title (RBF) */
# define	P_RBFHEAD	320	/* start of column head (RBF) */
# define	P_RBFPTOP	330	/* start of page top (RBF) */
# define	P_RBFPBOT	340	/* start of page bottom (RBF) */
# define	P_RBFTRIM	350	/* start of detail trim (RBF) */
# define	P_RBFFIELD	360	/* start of detail field (RBF) */
# define	P_RBFAGGS	370	/* start of aggregates (RBF) */
# define	P_IF		400	/* start of if conditional statement */

/*
**	Codes for program constants
*/

# define	PC_ERROR	-1	/* error code for not found  */
# define	PC_DATE		1	/* current date */
# define	PC_DAY		2	/* current day */
# define	PC_TIME		3	/* current time */
# define	PC_CNAME	10	/* column name in .WI block */


/*
**	Codes for types of presetting
*/

# define	PT_CONSTANT	1	/* preset to a constant */
# define	PT_ATTRIBUTE	2	/* preset to the current value of
					** an attribute. */



/*
**	Codes for the type of newlines created.
*/

# define	LN_NONE		0	/* no line started yet */
# define	LN_WRAP		1	/* created by wrap around */
# define	LN_NEW		2	/* created with a newline */



/*
**	Miscellaneous other codes.
*/

# define	B_HEADER	"h"	/* header text */
# define	B_FOOTER	"f"	/* footer text */

# define	B_ABSOLUTE	0	/* absolute change (used with positioning) */
# define	B_RELATIVE	1	/* relative change (used with positioning) */
# define	B_DEFAULT	2	/* default (used with positioning) */
# define	B_COLUMN	3	/* column change (used in positioning) */




/*
**	Codes for Data values.
*/

# define	D_CURRENT	0	/* current data value */
# define	D_PREVIOUS	1	/* previous data value */

/*
**	Names of special breaks and types in report
*/

# define	NAM_REPORT	"report"	/* name of report RACTION tuples*/
# define	NAM_PAGE	"page"		/* name of page RACTION tuples */
# define	NAM_DETAIL	"detail"	/* name of detail RACTION tuples */

# define	NAM_HEADER	"header"	/* name of header text (used for errors) */
# define	NAM_FOOTER	"footer"	/* name of footer text (for errors) */

# define	NAM_TLIST	"targetlist"	/* name of rows in RQUERY table
						** which are part of target list */
# define	NAM_EPRINT	"endprint"	/* name of endprint command
						** generated by SREPORT to
						** indicate the end of a print
						** command. */

# define	NAM_ELET	"endlet"	/* name of nedlet command
						** generated by SREPORT to
						** indicate the end of a let
						** command. */
# define	NAM_SELECT	"select"	/* an SQL select statement */
# define	NAM_FROM	"from"		/* from clause in SQL select */

# define	NAM_REMAINDER	"remainder"	/* remainder of query after
						** target list such as where
						** clause and/or sort by clause
						** or order by clause. */

/*
**	Values of RCOTYPE field in the RCOMMAND table.
*/

# define	NAM_ACTION	"ac"		/* name of action type */
# define	NAM_QUERY	"qu"		/* name of query type */
# define	NAM_SQL		"sq"		/* name of sql query type */
# define	NAM_SORT	"so"		/* name of sort type */
# define	NAM_BREAK	"br"		/* name of break type */
# define	NAM_OUTPUT	"ou"		/* name of output type */
# define	NAM_TABLE	"ta"		/* name of table type */
# define	NAM_DECLARE	"de"		/* name of declare type */
# define	NAM_SETUP	"se"		/* name of setup type */
# define	NAM_CLEANUP	"cl"		/* name of cleanup type */
# define	NAM_WIDTH	"wd"		/* name of width type */
# define	NAM_DELIMID	"xn"		/*
						** name of expanded namespace
						** type
						*/

/*
**	Codes for order of sorting
*/

# define	O_ASCEND	"a"	/* ascending order */
# define	O_DESCEND	"d"	/* descending order */

/*
**	Codes for the types of tokens found.
*/

# define	TK_ENDFILE	-1	/* code for end of file */
# define	TK_ENDSTRING	0	/* end of string ('\0') found */
# define	TK_ALPHA	1	/* alphabetic char found */
# define	TK_NUMBER	2	/* digit next */
# define	TK_SIGN		3	/* + or - found */
# define	TK_OPAREN	4	/* opening parenthesis found */
# define	TK_CPAREN	5	/* closing parenthesis found */
# define	TK_QUOTE	6	/* quote mark found */
# define	TK_COMMA	7	/* comma found */
# define	TK_PERIOD	8	/* period found */
# define	TK_SEMICOLON	9	/* semicolon found */
# define	TK_COLON	10	/* colon found */
# define	TK_EQUALS	11	/* equal sign found */
# define	TK_DOLLAR	12	/* dollar sign found */
# define	TK_MULTOP	13	/* * or / found */
# define	TK_POWER	14	/* ** found */
# define	TK_RELOP	15	/* !=, <, <=, >, or >= found */
# define	TK_SQUOTE	16	/* single quote found */
# define	TK_LCURLY	17	/* left curly bracket found */
# define	TK_RCURLY	18	/* right curly bracket found */
# define	TK_HEXLIT	19	/*
					** hex constant (x|X followed by single
					** quote) found
					*/
# define	TK_OTHER	100	/* other character found */

/*
**	Names of the internal variables in the report formatter
**		which are available to the user.
*/

# define	V_PAGENUM	"page_number"	/* page number */
# define	V_LINENUM	"line_number"	/* line number */
# define	V_POSNUM	"position_number"	/* position number */
# define	V_LEFTMAR	"left_margin"	/* left margin */
# define	V_RIGHTMAR	"right_margin"	/* right margin */
# define	V_PAGELEN	"page_length"	/* page length */
# define	V_PAGEWDTH	"page_width"	/* page width */


/*
**	Codes for variables.
*/

# define	VC_NONE		0	/* error or none */
# define	VC_PAGENUM	1	/* page_number */
# define	VC_LINENUM	2	/* line_number */
# define	VC_POSNUM	3	/* position_number */
# define	VC_LEFTMAR	4	/* left_margin */
# define	VC_RIGHTMAR	5	/* right_margin */
# define	VC_PAGELEN	6	/* page_length */
# define	VC_PAGEWDTH	7	/* page_width */




/*
**	Codes for status of expression parsing.
**
*/

# define	NULL_EXP	-1	/* "null" found */
# define	GOOD_EXP	0	/* a legal expression was found */
# define	NO_EXP		1	/* no expression found */
# define	BAD_EXP		2	/* bad expression */


/*
**	Codes for items in expressions
*/

# define	I_NONE		0	/* no item */
# define	I_EXP		1	/* expression */
# define	I_CON		2	/* data constant */
# define	I_PC		3	/* program constant */
# define	I_PV		4	/* program variable */
# define	I_ATT		5	/* attribute */
# define	I_PAR		6	/* parameter */
# define	I_ACC		7	/* aggregate */
# define	I_CUM		8	/* cumulative aggregate */






/*
**	Codes for boolean operators (not in ADF)
*/

# define	OP_NONE		-1	/* no operator */
# define	OP_NOT		-2	/* not */
# define	OP_AND		-3	/* and */
# define	OP_OR		-4	/* or */
# define	OP_BREAK	-5	/* change_in_value function */





/*
**	Constants particular to the Report Formatter
*/

# define	T_SIZE	300		/* number of trace flag words */
# define	ERRBASE 7000		/* all Report errors in 7000 range */
# define	MAXEARG 10		/* maximum number of arguments to
					** the error file processor.
					*/
# define	T_LST_SIZE	29	/* max length in characters of the
					** target list for one attribute.
					** Bug 10657 - up'd by 4 to allocate
					** proper storage in rdtplset.c
					*/
# define	PREFIX		"rrrr"	/* prefix for temporary relation name */
# define	REPMODE		1	/* mode of access to report file (write) */
# define	REPRSIZE	0	/* record size for report file */


/*
**	Maximums for the report formatter
*/

# define	MAXRTEXT	100	/* maximum length of RTEXT parameters */
# define	MAXQLINE	100	/* maximum length of RQUERY lines */
# define	MAXERR		30	/* maximum number of error messages */
# define	MAXRNAME	12	/* maximum name size in report writer */
# define	MAXPNAME	36	/* maximum name size of parameters */

/*
**	Constants for INGRES
*/

# define	MAXSTRING 255	/* maximum length of char strings */
# define	MAXLINE 255	/* maximum size for various buffers */
# define	MAXCMD	12	/* maximum length of RTEXT commands */
# define	MAXFNAME 50	/* maximum length of a file name */
# define	NAM_RANGE	"range"		/* name of range statement */
# define	NAM_RETR	"retrieve"	/* name of retrieve statement */
# define	NAM_OF		"of"		/* used in range statement */
# define	NAM_IS		"is"		/* used in range statement */
# define	NAM_WHERE	"where"		/* name of where clause */
# define	NAM_AND		"and"		/* name of and */
# define	NAM_SQLQUOTE	"'"		/* SQL quote character */
# define	NAM_QUELQUOTE	"\""		/* QUEL quote character */
/*
**	Defaults for Report Formatter
*/

# define	MAX_CF	35		/* maximum size of character formats
					** before a folding format is used */
# define	MAX_FRM	(MAXRTEXT - 4)	/*
					** Maximum length of a format string
					** so it will fit within RTEXT
					*/
# define	FRM_VN		"f6"	/* default format for runtime
					** variables */
# define	FRM_CDATE "d' 3-FEB-1901'"
					/* dflt format for current_date R5*/
					/*
					** dflt format for current_date R6
					** (DB_US_DFMT)
					*/
# define	FRM_CDATE6 "d'3-feb-1901'"
					/*
					** dflt formats for current_date R6
					** (International)
					*/
# define	FRM_CDATE_MLTI	"d'03/02/01'"	/* Multinational	*/
# define	FRM_CDATE_ISO	"d'010203'"	/* ISO			*/
# define	FRM_CDATE_ISO4	"d'19010203'"	/* ISO4			*/
# define	FRM_CDATE_FIN	"d'1901-02-03'"	/* Sweden/Finland	*/
# define	FRM_CDATE_GERM	"d'03.02.1901'"	/* German		*/
# define	FRM_CDATE_MLT4	"d'03/02/1901'"	 /* Multinational4       */
# define	FRM_CDATE_YMD	"d'1901-feb-03'" /* YMD                  */
# define	FRM_CDATE_MDY	"d'feb-03-1901'" /* MDY                  */
# define	FRM_CDATE_DMY	"d'03-feb-1901'" /* DMY                  */


# define	FRM_CTIME "d'16:05:06'"
					/* dflt format for current_time */

# define	TAB_DFLT	1	/* default number of spaces to tab */
# define	NL_DFLT		1	/* default number of newlines */
#ifdef FT3270
# define	PL_DFLT		60	/* page size (lines) default */
#else
# define	PL_DFLT		61	/* page size (lines) default */
#endif
# define	RM_DFLT		100	/* right margin (column) default */
# define	NP_DFLT		1	/* dflt add 1 to page number */
# define	NEED_DFLT	1	/* dflt for need command */
# define	LM_DFLT		0	/* left margin (column) default */
# define	SPC_DFLT	3	/* three spaces between columns */
# define	TTLE_DFLT	2	/* two spaces (colon, space) for 
					** block style title */
# define	UL_DFLT		' '	/* default character for underlining */
# define	MQ_DFLT		5000	/* default maximum query size */
# define	ML_DFLT		150	/* default maximum output line size */
# define	FO_DFLT		132	/* default maximum output line size
					** for default reports to files */
# define	MW_DFLT		DB_GW2_MAX_COLS + 10
					/* default maximum number of lines
					** to wrap around. This is an arbitrary
					** number, a bit larger than the max # 
					** of columns/table */
# define	ACH_DFLT	32000	/* default maximum action cache size */
# define	MAX_RETRIES	7	/* max # of retries of deadlocked
					** operation. */

# define	MAX_OPANDS	2	/* maximum number of operands for
					** operators and functions */



/*
** Defines needed for REPORT/SREPORT reset routine, invocation. The notation
** used in the comments details the former value in the reset parameter list:
** (r) - r_reset; (s) - s_reset; (rbf) - rF_reset.
*/
#define RP_RESET_LIST_END       0	/* End of list of reset options	*/
#define RP_RESET_PGM            1   	/*
					** Init invocation of program
					** (r1/s1/rbf1)
					*/
#define RP_RESET_DB             2   	/* Init database (r2)		*/
#define RP_RESET_SRC_FILE       3   	/* Init report source file (s2)	*/
#define RP_RESET_CALL           4   	/* Init call from appl. (r3)	*/
#define RP_RESET_REPORT         5   	/* init report spec (s3/rbf2)	*/
#define RP_RESET_RELMEM4        6   	/* Init report spec memory (r4)	*/
#define RP_RESET_RELMEM5        7   	/* Init report run memory (r5)	*/
#define RP_RESET_OUTPUT_FILE    8   	/* Init output file (r6)	*/
#define RP_RESET_CLEANUP        9   	/* Memory cleanup after load (r7)*/
#define RP_RESET_TCMD        	10   	/* Memory cleanup of TCMD (r8)	*/

/*
** Defines needed for template style default report generation:
*/

# define	RS_NULL		0	/* none specified */
# define	RS_DEFAULT	1	/* RW will choose appropriate default,
					** either RS_COLUMN or RS_BLOCK */
# define	RS_COLUMN	2	/* first choice of default.  This
					** prints columns accross to end */
# define	RS_WRAP		3	/* like RS_COLUMN, but wraps at
					** a certain point */
# define	RS_BLOCK	4	/* second choice of default.  This
					** prints titles to fields, ala QBF */
# define	RS_LABELS	5	/* special style for printting mailing 
					** labels */
# define    	RS_TABULAR  	6   	/* same as column */
# define    	RS_INDENTED 	7   	/* control break report */
# define    	RS_MASTER_DETAIL 8 	/* control break with multiple levels 
					** of break headers possible */
/*
** List Pick Selections--These must correspond
** to the order in which the items are displayed
** in the listpick list.  The selections from
** the listpick lists are in errf.msg.
**/
/* ListPick List for Data Source Selection*/
#define         NoRepSrc        -1      /* No Source */
#define        DupRepSrc        0     /* Duplicating report */
#define        TabRepSrc        1     /* From table */
#define        JDRepSrc         2     /* From joindef */
#define        VQRepSrc         3     /* From VQ--not implemented */
                    
                    
/*
** Defines needed for template style default report generation:
*/
# define 	R_SEGMENT_SIZE  3
# define 	R_TRIM_ONLY     1
# define 	R_FIELD_ONLY    2
# define 	R_TRIM_FIELD    3
# define 	R_X_POS_ONLY    1
# define 	R_Y_POS_ONLY    2
# define 	R_X_Y_POS       3



/*
**	Codes for report type, as stored in the REPORTS table.
*/

# define	RT_SREPORT	's'	/* report created by SREPORT */
# define	RT_RBF		'f'	/* report created by RBF (created
					   previous to below two types,
					   and kept around for compatability)
					 */
# define	RT_QUEL_RBF	'q'	/* QUEL report created by RBF */
# define	RT_SQL_RBF	'l'	/* SQL report created by RBF */
# define	RT_DEFAULT	'd'	/* default report */



/*
** Formfeed default for RW and RBF depends on operating system.
** So far, only UNIX has formfeed as default.  VMS, MSDOS, and CMS
** have no formfeed as default.
*/

# ifdef UNIX
# define	FFDFLTB		TRUE
# define	FFDFLTS		"y"
# else
# define	FFDFLTB		FALSE
# define	FFDFLTS		"n"
# endif

/*
** Define SQL Interface
*/
# ifndef SEINGRES
# define SQL
# endif


/*
** Declared variables needed for labels style report generation:
*/

#define 	LABST_LINECNT		"linecnt"
#define 	LABST_MAXFLDSIZ		"maxfldsiz"
#define 	LABST_MAXPERLIN		"maxperlin"
#define 	LABST_IN_BLK		"in_block"
#define     	LABST_VERT_DEFLT	2
#define		LABST_HORZ_DEFLT	3


/*
** Constants for s_q_add
*/
#define 	WRD_WIDTH		ERx("WIDTH")
#define 	WRD_BLANK		ERx(" ")
#define 	WRD_WHERE		ERx("where")
#define 	WRD_GROUP		ERx("group")
#define 	WRD_HAVING		ERx("having")
#define 	WRD_UNION		ERx("union")
#define 	WRD_ORDER		ERx("order")
#define 	WRD_SORT		ERx("sort")

/*
** Text of "new style" command line flags ...
*/
# define	RW_NO1STFF_FLAG		ERx("nofirstff")
# define	RW_1STFF_FLAG		ERx("firstff")
# define	RW_UTF8_NOALIGNFLAG	ERx("noutf8align") 

/*
** Report level identifiers
*/
#define		LVL_BASE_REPORT	0	/*
					** Base level report
					*/
#define		LVL_RBF_PG_FMT	1	/*
					** Supports user inclusion/format
					** specification for date/timestamp
					** and page number (RBF)
					*/
#define		LVL_MAX_REPORT	LVL_RBF_PG_FMT

/*
** Constants for numeric_overflow flag
*/
#define NUM_O_FAIL 0
#define NUM_O_WARN 1
#define NUM_O_IGNORE 2

#endif /* RCONS_INCLUDE */
