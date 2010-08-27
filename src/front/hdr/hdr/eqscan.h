/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
** eqscan.h
**	- Header for all scanner modules in the EQUEL preprocessor
**	- Users must include <si.h> first.
**
** History:
**	20-nov-1984		Rewritten (ncg)
**	09-aug-1985		Modified to support ESQL (mrw)
**	15-jun-1990		Added decimal support (teresal)
**	13-aug-1992		Added tok_skeytab and tok_skynum to support
**				separate Master/Slave keyword tables in 
**				ESQL. EQUEL still uses only tok_keytab (larrym)
**	24-sep-1992		Added new routine for doing ANSI SQL2
**				keyword lookup. (teresal)
**	02-oct-1992 (larrym)
**		Added support for allowing host variables to be reserved
**		words.  Added globals sc_hostvar and sc_saveptr.
**	09-oct-1992 (teresal)
**				Removed sc_saveptr, it was causing esqyylex
**				to be included for EQUEL on VMS. For now,
**				this variable is only referenced in esqyylex.
**	20-nov-1992 (Mike S)
**		Add 4GL-specific keyword tables.
**	14-dec-1992 (lan)
**		Added TOK_DELIMID constant for delimited identifiers.
**	09-feb-1993 (teresal)	Add declaration for sc_moresql() function. 
**	10-jun-1993 (kathryn)
**	    Add prototype for sc_execstmt.
**	14-apr-1994 (teresal)
**	    	Add TOK_SLASH for ESQL/C++ so we can determine an C++ 
**	   	style comment.
**      12-Dec-95 (fanra01)
**              Changed all externs to GLOBALREFs.
**              Modified references for DLLs on NT.
**              Declare the keyword data as extern for data initialisation
**              purposes.
**              Declares SC_PTR for importing from DLL.
**	06-Feb-96 (fanra01)
**		Changed flag from __USE_LIB__ to IMPORT_DLL_DATA.
**	04-mar-1996 (thoda04)
**		Added function prototypes for include functions.
**	30-sep-1996 (canor01)
**		Remove GLOBALREF for sc_dbglang (static in sccmnt.c)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	21-aug-01 (inkdo01)
**	    Added 2nd left nest token to sc_tokeat call.
**	7-dec-05 (inkdo01)
**	    Added tUCONST for Unicode literal support.
**	24-Mar-2007 (toumi01)
**	    Added SC_ATASKBODY for scan of Ada TASK BODY for -multi support.
**	05-apr-2007 (toumi01)
**	    Add support for the Ada ' tick operator.
**	15-aug-2007 (dougi)
**	    Added tDTCONST for date/time literals.
**      23-aug-2010 (stial01) (SIR 121123)
**          Changes for Long IDs
*/

/* 
** To decommit special scanner features (Include files) of old versions
** before 4.0, delete this comment delimiter -->
# define  SC_EQ21		  /* end comment */

typedef struct 
{
    char	*key_term;
    i4		key_tok;		
} KEY_ELM;

/*
** The following two structures are used for double keyword scanning.
*/
typedef struct 
{
    char	*name;
    i4		lower;
    i4		upper;
} ESC_1DBLKEY;

typedef struct {
    char	*name;
    i4		tok;
    char	*dblname;
} ESC_2DBLKEY;

# define	KEYNULL		(KEY_ELM *)0

# if defined(NT_GENERIC) && defined(REFERENCE_IN_DLL)
FACILITYREF KEY_ELM		tok_keytab[];		/* Equel/SQL-Master keywords */
FACILITYREF KEY_ELM		tok_skeytab[];		/* SQL Slave keywords */
FACILITYREF KEY_ELM		tok_4glkeytab[];	/* EXEC 4GL keywords */
FACILITYREF i4		tok_kynum;		/* num of Equel/SQL keywords */
FACILITYREF i4		tok_skynum;		/* num of SQL Slave keywords */
FACILITYREF i4		tok_4glkynum;		/* num of EXEC 4GL keywords */
FACILITYREF KEY_ELM		tok_optab[];		/* Table of operators */
FACILITYREF i4		tok_opnum;		/* Total number of operators */
FACILITYREF ESC_1DBLKEY	tok_1st_doubk[];	/* Table of 1st of dbl words */
FACILITYREF ESC_2DBLKEY	tok_2nd_doubk[];	/* Table of 2nd of dbl words */
FACILITYREF char		*tok_sql2key[];		/* Table of SQL2 keywords */
FACILITYREF i4		tok_sql2num;		/* Total number SQL2 keywords */
# else              /* NT_GENERIC && REFERENCE_IN_DLL */
GLOBALREF KEY_ELM		tok_keytab[];		/* Equel/SQL-Master keywords */
GLOBALREF KEY_ELM		tok_skeytab[];		/* SQL Slave keywords */
GLOBALREF KEY_ELM		tok_4glkeytab[];	/* EXEC 4GL keywords */
GLOBALREF i4		tok_kynum;		/* num of Equel/SQL keywords */
GLOBALREF i4		tok_skynum;		/* num of SQL Slave keywords */
GLOBALREF i4		tok_4glkynum;		/* num of EXEC 4GL keywords */
GLOBALREF KEY_ELM		tok_optab[];		/* Table of operators */
GLOBALREF i4		tok_opnum;		/* Total number of operators */
GLOBALREF ESC_1DBLKEY	tok_1st_doubk[];	/* Table of 1st of dbl words */
GLOBALREF ESC_2DBLKEY	tok_2nd_doubk[];	/* Table of 2nd of dbl words */
GLOBALREF char		*tok_sql2key[];		/* Table of SQL2 keywords */
GLOBALREF i4		tok_sql2num;		/* Total number SQL2 keywords */
# endif             /* NT_GENERIC && REFERENCE_IN_DLL */

/*
** There are a number of tokens that are returned by the scanner that do
** not have corresponding keywords - for example, a string constant.  These
** are returned explicitly via indirection through tok_special[].  This would
** not be necessary if the Yacc definition file was included in all the files
** that want to return these values.  However, this is a very 'un-maintainable'
** idea considering the number of files, extended across each different 
** language. The problem is solved once here, and does not require 
** recompilation of anything but the file that defines this array.
*/

GLOBALREF i4		tok_special[];

/* Indices into this array of Yacc constants */

# define	TOK_NAME	0		/* User defined name */
# define	TOK_STRING	1		/* String constant */
# define	TOK_I4NUM	2
# define	TOK_F8NUM	3
# define	TOK_QUOTE	4		/* Begin string constant */
# define	TOK_COMMENT	5		/* Begin comment */
# define	TOK_CODE	6		/* Host language code */
# define	TOK_TERMINATE	7		/* Language terminator */
# define	TOK_INCLUDE	8		/* Non-inline ## include */
# define	TOK_DEREF	9		/* Derefenced use name */
# define	TOK_EOFINC	10		/* Eof of ESQL include */
# define	TOK_HEXCONST	11		/* Hex constant */
# define	TOK_WHITESPACE	12		/* whitespace */
# define	TOK_DECIMAL	13		/* Decimal number */
# define	TOK_DELIMID	14		/* Delimited Ids */
# define	TOK_SLASH	15		/* Forward slash '/' */
# define	TOK_UCONST	16		/* Unicode literal */
# define	TOK_ATICK	17		/* Ada ' tick operator */
# define	TOK_DTCONST	18		/* Date/time literal */

/* 
** Special values used by the character inputter, sc_readline(), to cause 
** changes in input control.  The special case of SC_EOFCH, which must be 
** the same as SC_EOF is reserved.
*/

# define	SC_EOFCH	'\0'		/* Same as SC_EOF, but char */

/*
** General states returned by parts of the scanner to cause changes in control.
*/

# define	SC_EOF		0		/* Eof token */
# define	SC_CONTINUE	1		/* Continue scanning */

/* Maximum sizes of data scanned off */

# define	SC_NUMMAX	100
# define	SC_WORDMAX	256
# define	SC_STRMAX	512
# define	SC_LINEMAX	512

/* 
** Tags to allow additions into a union of token types - yylval.
** Currently this is all faked as we only have (char *) types coming from
** the scanner.  If anymore are added then define here, and add to
** sc_addtok().
*/

# define	SC_YYSTR	0
# define	SC_YYSYM	1
# define	SC_YYINT	2

/* 
** Return values from Include file when openning and closing. These
** return values reflect whether there is an Inline inclusion or not.
*/

# define	SC_NOINC	0
# define	SC_INC		1
# define	SC_INLINE	2
# define	SC_BADINC	3

/*
** Return values from sc_ishost() for SQL (also SC_EOF)
*/

# define	SC_SQL		4
# define	SC_FRS		5
# define	SC_HOST		6
# define	SC_DEBUG	7	/* debug command line -- ignore */
# define	SC_TERM		8	/* terminator detected */
# define	SC_CONT		9	/* continue statement */
# define	SC_EQUEL	10	/* EQUEL statement */
# define	SC_4GL		11
# define	SC_ATASKBODY	12	/* added for scan of Ada TASK BODY */

/* Flags for sc_eat */

# define	SC_NOEAT	00
# define	SC_NEST		01
# define	SC_STRIP	02
# define	SC_BACK		04
# define	SC_SEEN		010

/* arguments to the scanner to indicate the scanner level of "rawness". */
# define	SC_RAW		(i4)0
# define	SC_RARE		(i4)1
# define	SC_COOKED	(i4)2

/* 
** Debugging table of functions to call - triggerred by $_debug in comment.
** Defined here so that different langauges can use it to define their own
** debug tables.
*/
typedef struct {
    i4		dbg_vis;
    char	*dbg_name;
    i4		(*dbg_func)();
    i4		dbg_arg;
    char	*dbg_help;
} SC_DBGCOMMENT;

# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF	char		*SC_PTR;	/* Global scanner pointer */
GLOBALDLLREF i4    	sc_hostvar;	/* are we scanning a host var? */
# else              /* NT_GENERIC && IMPORT_DLL_DATA */
GLOBALREF	char		*SC_PTR;	/* Global scanner pointer */
GLOBALREF i4    	sc_hostvar;	/* are we scanning a host var? */
# endif             /* NT_GENERIC && IMPORT_DLL_DATA */

/*
** Scanner function prototypes
*/
i4  	sc_comment(char	*boc);  	/* Eat comment */
i4  	sc_number(i4 mode);    	/* Eat number */
i4  	sc_operator(i4	mode);  	/* Eat operator */
i4  	sc_string(KEY_ELM *quotekey, i4  mode); 	/* Eat string constant */
i4  	sc_hexconst(void);      	/* Eat hex constant */
i4  	sc_uconst(void);      		/* Eat Unicode constant */
i4  	sc_dtconst(char *wbuf, i4 qoff); /* Eat date/time constant */
i4  	sc_word(i4 mode);      	/* Eat word */
i4  	sc_deref(void);         	/* Dereference word */
void	sc_addtok(i4 tag, char *value);	/* Add a token for Yacc */
void	sc_reset(void);     	/* Reset all input character buffers */
KEY_ELM	*sc_getkey(KEY_ELM tab[], i4  tabnum, char *usrkey);
                            	/* Get keyword token */
void	sc_eat(void (*storfunc)(), i4  flag, char *term_str, 
                           	       char l_nest_ch, char r_nest_ch);
                             	/* Eat up tokens - lower level */
STATUS	sc_ansikey(char *tab[], i4  tabnum, char *usrkey);
                        		/* Determine if token is ANSI SQL2 keyword */	
void	sc_popscptr(void);  	/* reset SC_PTR after hostvar scan */
void	sc_moresql(void);   	/* Set look for more SQL code flag */
void	sc_readline(void);  	/* Read a new input line */
void	sc_tokeat(i4 flag, i4  term_tok, i4  l1_nest_tok, i4 l2_nest_tok, 
					i4  r_nest_tok);
				/* Eat up tokens */
void	sc_dbgcomment(void);	/* Call the specified debugging function */

i4  	yylex(void);        	/* Scanner called by Yacc (yyparse) */
void	yyerror(char *emsg);	/* Error routine called by yyparse */
i4  	yyequlex(i4 mode); 	/* Scanner for Yacc */
i4  	yyesqlex(i4 mode); 	/* Scanner for Yacc */

bool	lex_isempty(i4 lex_hostcode); /* Is the input line empty */

/* Prototypes for language dependent modules (e.g. csqscan.c, cbsqscan.c).
** Some argument types vary depending on the language.
*/
i4  	sc_newline(/*args vary with the language*/);
                         		/* Process end-of-line (language dependent) */
i4  	sc_skipmargin(void);    	/* Skip the margin on a new line */
# ifdef EQSCAN_COMMENT_C_VERSION
i4  	sc_iscomment(char *cp); 	/* Determine if a line is a comment line */
i4  	scStrIsCont(void);      	/* Is current line a continuation line? */
# endif
# ifdef EQSCAN_COMMENT_COBOL_VERSION
bool	sc_iscomment(void);     	/* Determine if a line is a comment line */
bool	scStrIsCont(char ch);   	/* Is current line a continuation line? */
# endif
char *	sc_label(char **lbl);   	/* Scan for a label */
bool	sc_scanhost(i4 scan_term);	/* Do some crude parsing of host code */
bool	sc_eatcomment(char **cp, bool eatall); /* Do we have a host comment? */
bool	sc_inhostsc(void);    /* Is input line in a multi-line host comment? */

/* Prototypes for esqyylex.c
*/

FUNC_EXTERN  i4  sc_execstmt(char **cptr);     /* scan for EXEC SQL|FRS|4GL */	

/* Prototypes for include processing
*/
i4  	inc_tst_include();  
    	        /* See if the newly read line has an Include directive in it */
    	        /*  */
i4  	inc_parse_name(char *fname, i4  is_global);
    	        /* Parse a passed-in full file name (for SQL) */
void	inc_push_file();
    	        /* Change I/O files for a file inclusion after ## include */
void	inc_pop_file();
    	        /* Restore previous file environ (called by grammar on EOF) */
void	inc_abort_file();
    	        /* Set inc_stop and inc_pop_file */
i4  	inc_is_nested();
    	        /* Tell whether we are currently inside an include file */
void	inc_add_inc_dir(char *str);
    	        /* Add a directory name to the include list */
i4  	inc_expand_include( /* LOCATION *f_locp, 
    	                       char *f_locbufp, LOCATION **glob_loc */ ); 
    	        /* Expand include filenames with directory prefixes */
void	inc_save(  /* LOCATION *loc */);
    	        /* Save parent location for remote include file processing */
