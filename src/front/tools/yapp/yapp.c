/*
** Copyright (c) 1993, 2004 Ingres Corporation
**
*/
/*
**  yapp.c -- yet another (portable) pre-processor
** 
**  This general purpose preprocessor filters the contents of a text
**  file by interpreting the following C pre-processor directives:
**
**	#define, #undef, #if, #ifdef, #ifndef, #else, #endif, #include
**
**  Symbols may be defined either using the -D command line options or
**  the #define directive.  A string which identifies the beginning of
**  History comment lines (to be stripped) may be optionally specified
**  with using -H command line option.
**
**  Usage: yapp [ -C ] [ -Dsymbol[=value] ] [ -Hprefix ] [ -Iinclude ] [ file ]
**
**  History:
**	1-jun-93 (tyler)
**		Created.
**	26-jul-93 (tyler)
**		Fixed bugs in #define and #undef processing.  yapp will
**		now accept multiple history prefixes specified on the
**		command line and reads from standard input if no file
**		is named.
**	29-jul-93 (tyler)
**		Fixed bug introduced in the previous integration which
**		caused unquoted command-line arguments not to be
**		recognized on VMS.  Error messages now say 'yapp' instead
**		of displaying argv[ 0 ] which is too long on VMS.
**	06-aug-93 (tyler)
**		Commented.	
**	03-sep-93 (joplin)
**		Added symbol value substitution and #include processing.
**              Warning: yapp doesn't understand any C or sh grammar.  It
**              simply breaks things up into symbol tokens and other chars.
**              Symbol tokens are just sequences of characters which contain
**              alphanumeric text and the '$' and '_' characters.
**	18-Oct-93 (joplin)
**		Added preprocessor if directive handling.  This only supports
**              defined(), parenthetical expressions, !, &&, and || operators.
**		Added processing of C comments and -C command line flag.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      21-apr-1999 (hanch04)
**        Replace STrindex with STrchr
**	15-feb-2005 (abbjo03)
**	    On VMS, SIgetrec doesn't always return a newline character at the
**	    end of a line, in particular if reading from a pipe. For expediency
**	    when testing for line continuation, see if the backslash is followed
**	    by a null character (in addition to a \n).
*/

# include <compat.h>
# include <cm.h>
# include <er.h>
# include <lo.h>
# include <me.h>
# include <pc.h>
# include <si.h>
# include <st.h>

/*
PROGRAM =	yapp
**
NEEDLIBS =	COMPATLIB
**
UNDEFS =	II_copyright
*/



# define MAX_LINE	1000		/* maximum input line length */
# define MAX_ITERATION  10              /* maximum symbol iteration level */
# define MAX_INCLUDE    10              /* maximum include depth */
# define MAX_SYMLEN     128             /* maximum STlength of a symbol */

/*
** symbol table definition, contains the number of symbols
** defined in the table with the name/value pairs and history flag indicator.
*/

typedef struct symbol_struct {
	char 	*name;
	char	*value;
	bool	hist_flag;
	struct	symbol_struct *next;
} SYMBOL;

static struct symbol_table {
	i4 	nsym;
	SYMBOL	*head;
} symtab = { 0, NULL };


typedef struct stack_frame {
	bool display_mode;
	struct stack_frame *prev;
} STACK_FRAME; /* stores "display mode" for each #ifdef block. */

# define tEOF		0	/* end-of-file token */
# define tHISTORY	1	/* history comment line token */
# define tSOURCE	2	/* source line token */
# define tDIRECTIVE	3	/* pre-processor directive line token */ 

/*
** The file stack is an array of files which are open at any point in
** time.  Whenever we need to go to open a new file, in order to process
** an include directive, we just push a new file on top of the stack.
*/
 
static struct _file
{
        FILE	*fp;
        char	name[ MAX_LOC + 1 ];
	char	yytext[ MAX_LINE + 1 ];	/* input line buffer */
	i4	yylineno;		/* input line counter */
	bool	yyline_cache;      	/* TRUE if last input line cached */
	bool	yyopen;			/* TRUE if file successfully opened */
	bool	yycomment;		/* TRUE if processing a comment */
} file[ MAX_INCLUDE ];

static i4  filex = -1;             /* Index of topmost element of file stack */
static struct _file *infile;       /* Pointer to topmost opened file */


/*
** An include path list is built to allow for a series of include paths
** to be used for resolving include file names.  The first item in the
** list of paths will be the current directory.  
**
**    If an include file is specified with the include "filename" syntax, 
**    then search for the file beginning with the current directory.
**
**    If an include file is specified with the include <filename> syntax, 
**    then search for the file beginning the next directory in the list.
*/

typedef struct _include_path {
	struct _include_path *next;
	LOCATION *pathloc;
} IPATH;  

static IPATH ihead;               /* Head of the Include path list */
static LOCATION defdir;		  /* Default file search location */
static bool pass_comments =FALSE; /* Default C comment passing flag */

/*
**
** Function prototypes
**
*/

static bool pp_eval_boolexp( char **strptr, char *goal );
static void pp_file( STACK_FRAME *stack, char *path_name, bool default_dir );
static i4   pp_input( FILE *input, STACK_FRAME *stack, bool ifdef );

/*
** Error messages (Later conversion to .msg files?)
*/
#define E_YAPP000 \
    ERx("\nUsage: yapp [-C] [-Dsymbol[=value]] [-Hprefix] [-Iinclude] [file]\n")
#define E_YAPP001 ERx("\nSyntax error (in preprocessor if)\n")
#define E_YAPP002 ERx("\nIPATH Memory request failed\n")
#define E_YAPP003 ERx("\nLOCATION Memory request failed\n")
#define E_YAPP004 ERx("\nMAX_LOC Memory request failed\n")
#define E_YAPP005 ERx("\nError creating include LOCATION\n")
#define E_YAPP006 ERx("\nMaximum iteration level exceeded\n")
#define E_YAPP007 ERx("\nUnexpected #else directive encountered\n")
#define E_YAPP008 ERx("\nSYMBOL Memory request failed\n")
#define E_YAPP009 ERx("\nExpecting #endif directive\n")
#define E_YAPP00A ERx("\nMaximum include depth exceeded\n")
#define E_YAPP00B ERx("\nCould not open file, %s\n")
#define E_YAPP00C ERx("\nUnexpected #endif directive encountered\n")
#define E_YAPP00D ERx(">>File:%s, line:%d, %s")
#define E_YAPP00E ERx("\nToo many boolean expressions\n")
#define E_YAPP00F ERx("\nAND (&&) caused too many operators\n")
#define E_YAPP010 ERx("\nMissing comment terminator, */\n")
#define E_YAPP011 ERx("\nOR (||) caused too many operators\n")
#define E_YAPP012 ERx("\nunused\n")
#define E_YAPP013 ERx("\nInvalid or empty defined()\n")
#define E_YAPP014 ERx("\nUnknown or unsupported syntax\n")
#define E_YAPP015 ERx("\nFailed to completely evaluate preprocessor if\n")
#define E_YAPP016 ERx("\nFailed trying to parse empty boolean expression\n")
#define E_YAPP017 ERx("\nObject of a boolean expression operator is missing\n")

/*
** yydump() -- Dump the contents of the file stack and die a horrible
** death with a call to PCexit()
*/

static void
yydump( )
{
	i4 filenum;

	for (filenum = 0; filenum <= filex; filenum++)
	{
		if (filenum < MAX_INCLUDE && file[filenum].yyopen == TRUE)
			SIfprintf(stderr, E_YAPP00D, 
				file[filenum].name,
				file[filenum].yylineno,
				file[filenum].yytext);
	}
	SIfprintf(stderr, ERx("\n") );
	PCexit( FAIL );
}


/*
** define() -- add a symbol to the symbol table.
*/

static void
define(char *symbol, char *value, bool hist)
{
	SYMBOL **symptr_anchor;
	SYMBOL *symptr_new;
	STATUS sts;	
	char *buf;

	/* Locate the last defined symbol in the list */
	for (symptr_anchor = &symtab.head;
		*symptr_anchor != NULL; 
		symptr_anchor = &((*symptr_anchor)->next) )
		continue; 

	/* Allocate memory for the new symbol structure */
	symptr_new = (SYMBOL *)MEreqmem(0, sizeof(SYMBOL), FALSE, &sts);
	if (symptr_new == NULL || sts != OK)
	{
		SIfprintf( stderr, E_YAPP008 );
		yydump();
	}

	/* 
	** Anchor the new symbol structure and fill it in 
	*/

	*symptr_anchor = symptr_new;
	symptr_new->name = STalloc( symbol );
	symptr_new->value = STalloc( value );
	symptr_new->hist_flag = hist;
	symptr_new->next = NULL;
	symtab.nsym++;
}

/*
** undefine() -- remove symbol from specified symbol table.
*/

static void
undefine( char *symbol )
{
	SYMBOL **symptr_anchor;
	SYMBOL *symptr;

	/* Try and locate the symbol in the list */
	for (symptr_anchor = &symtab.head;
		*symptr_anchor != NULL; 
		symptr_anchor = &symptr->next )
	{
		symptr = *symptr_anchor;
		if( STequal( symbol, symptr->name ) != 0 ) 
		{
			*symptr_anchor = symptr->next;
			MEfree( symptr->name );
			MEfree( symptr->value );
			MEfree( (PTR)symptr );
			symtab.nsym--;
			break;
		}
	}
}

/*
** is_defined() -- Symbol lookup request
**
** If the symbol is found, 
**   if the value string is non-NULL, fill it in with the defined value
**   return OK
** If the symbol is not found
**   return FAIL
*/

static STATUS
is_defined(char *symbol, char *value)
{
	SYMBOL *symptr;

	/* Try and locate the symbol in the list */
	for (symptr = symtab.head;
		symptr != NULL; 
		symptr = symptr->next )
	{
		if( STequal( symbol, symptr->name ) != 0 &&
			symptr->hist_flag == FALSE )
		{
			if (value != NULL)
				STcopy(symptr->value, value);
			return( OK );
		}
	}

	return( FAIL );
}

/*
** is_history() -- Returns OK if the text has a history prefix, FAIL otherwise
*/

static STATUS
is_history(char *text)
{
	SYMBOL *symptr;
	i4 len = STlength( text );

	/* Try and locate the symbol in the list */
	for (symptr = symtab.head;
		symptr != NULL; 
		symptr = symptr->next )
	{
		if( STbcompare(text, len, symptr->name, 0, FALSE ) == 0 &&
			symptr->hist_flag == TRUE )
			return( OK );
	}
	return( FAIL );
}



/*
** yyadd_path() -- Append an include path to the list of include directories.
*/

static void
yyadd_path( char *idirectory )
{
	STATUS sts;	
	IPATH *ipath_cur;
	IPATH *ipath_new;
	char *buf;

	/* Locate the last directory in the list of include paths */
	for (ipath_cur = &ihead; 
		ipath_cur->next != NULL; 
		ipath_cur = ipath_cur->next)
		continue; 

	/* Allocate memory for the new path structure */
	ipath_new = (IPATH *)MEreqmem(0, sizeof(IPATH), FALSE, &sts);
	if (ipath_new == NULL || sts != OK)
	{
		SIfprintf( stderr, E_YAPP002 );
		yydump();
	}

	/* 
	** Anchor the new structure and fill it in 
	*/

	ipath_cur->next = ipath_new;
	ipath_new->pathloc = 
		(LOCATION *)MEreqmem(0, sizeof(LOCATION), FALSE, &sts);
	ipath_new->next = NULL;

	if (ipath_new->pathloc == NULL || sts != OK )
	{
		SIfprintf( stderr, E_YAPP003 );
		yydump( );
	}

	buf = (char *)MEreqmem(0, MAX_LOC + 1, FALSE, &sts);
	if (buf == NULL || sts != OK )
	{
		SIfprintf( stderr, E_YAPP004 );
		yydump();
	}

	STcopy(idirectory, buf);
	if (LOfroms(PATH, buf, ipath_new->pathloc) != OK)
	{
		SIfprintf( stderr, E_YAPP005 );
		yydump();
	}
}


/*
** yygetline() -- store the next input line in yytext[]
*/ 

static STATUS
yygetline( FILE *input )
{
	char *read_buf = infile->yytext, *p, *psave;
	i4  linelen = 0;
	STATUS rtn;
	bool still_reading = TRUE;

/*
** If a line exists in the "unget" cache, just return success.
*/
	if( infile->yyline_cache )
	{
		infile->yyline_cache = FALSE;
		return( OK );
	}

/*
** Read lines from the input file until a non-continuation line is hit
** or the EOF is reached.
*/
	else while (still_reading == TRUE)
	{
		/* Assume the line is not a continuation and read a line */
		still_reading = FALSE;
		rtn = SIgetrec(read_buf, (i4)(MAX_LINE - linelen), input);

		/* Check for a continuation line */
		if (rtn == OK) 
		{
			++infile->yylineno; /* Increment files line counter */

			linelen = STlength(read_buf);

			/* Was the last character a continuation indicator? */
			p = STrchr(read_buf, '\\');
			if (p != NULL)
			{
				psave = p;
				CMnext(p);
#ifdef VMS
				if (*p == EOS || CMcmpcase(p, ERx("\n")) == 0)
#else
				if (CMcmpcase(p, ERx("\n")) == 0)
#endif
				{
					/* At this point we determined the */
					/* line is continued. */
					still_reading = TRUE;
					read_buf = psave;
					STcopy(ERx(""), psave);
				}
			}
		}
	}

	/* Special condition exception: If the EOF was hit after an some */
	/* data has already been read and we were still looking for      */
	/* non-continuation lines, just return back the data we have.    */
	if (OK != rtn && linelen > 0)
		return (OK);
	else
		return (rtn);
}

/*
** yyungetline() -- cache the current input line.
*/

static void
yyungetline( void )
{
	infile->yyline_cache = TRUE;
}

/*
** yylex() -- get the next line of input and return its token id.
*/

static i4
yylex( FILE *input )
{
	char *pscan;

	if( yygetline( input ) != OK )
		return( tEOF );

	/* Unless the -C flag was specified, blank out all of the comments */
	pscan = infile->yytext;
	while (pass_comments == FALSE && *pscan != EOS)
	{
		if (infile->yycomment == TRUE)
		{
			/* Look for the 'end comment' identifier */
			if (STbcompare(pscan,2,ERx("*/"),2,FALSE) == 0)
			{
				/* Blank out the 'end comment' characters */
				CMcpychar(ERx(" "), pscan);
				CMnext(pscan);
				CMcpychar(ERx(" "), pscan);

				/* Remember that the comment has ended */
				infile->yycomment = FALSE;
			}
			/* Blank out non-whitespace characters of the comment */
			else if (!CMwhite(pscan))
			{
				CMcpychar(ERx(" "), pscan);
			}
			/* Whitespace chars are skipped, this lets \n thru */
		}
		/* Look for the 'begin comment' identifier */
		else if (STbcompare(pscan,2,ERx("/*"),2,FALSE) == 0)
		{
			/* Blank out the 'begin comment' characters */
			CMcpychar(ERx(" "), pscan);
			CMnext(pscan);
			CMcpychar(ERx(" "), pscan);
			infile->yycomment = TRUE;
		}
		CMnext(pscan);	/* Continue the scan with the next character */
	}

	if( is_history(infile->yytext ) == OK)
	{
		return( tHISTORY ); 
	}

	if( *infile->yytext == '#' ) 
		return( tDIRECTIVE );

	return( tSOURCE );
}

/*
** yyputline() -- Process a string of text to be output.  If any token
** within the string evaluates to a symbol, then substitute the symbol.
** Following a substitution re-invoke this routine to recursively 
** substitute possible symbols within the newly evaluated symbol.
** This is done to handle the resolution of the following:
**    #define ONE 1
**    #define TWO ONE+1
**    
*/

static void
yyputline( char *outtext)
{
	char token_buf[MAX_LINE+1];
	char token_val[MAX_LINE+1];
	char *p = token_buf;
	static char *symc = ERx( 
	   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_$");
	i4 symc_l = STlength(symc);
	i4 defidx;
	static i4  iterate_level = 0;

	/* Check to make sure we don't have runaway recursion */
	if (iterate_level++ > MAX_ITERATION)
	{
		SIfprintf( stderr, E_YAPP006 );
		yydump();
	}

	/* Break up the string into tokens and try to resolve symbols */
	while (*outtext != EOS)
	{
		p = token_buf;
		while (STindex(symc, outtext, symc_l) != NULL) {
			STlcopy(outtext, p, 1);
			CMnext(outtext);
			CMnext(p);
		}

		/* Found a token that may be defined as a symbol */
		if (p != token_buf)
		{
			/* If the token is defined then translate the */
			/* value of the substituted text, otherwise just */
			/* output the untranslated text. */
			STcopy(ERx(""), p);
			if (OK == is_defined(token_buf, token_val)) 
				yyputline( token_val );
			else 
				SIprintf("%s", token_buf);
		}

		/* The text being processed at this point is whitespace, */
		/* operators, or some other characters that are not */
		/* valid for tokens. */
		else 
		{
			SIputc(*outtext, stdout);
			CMnext(outtext);
		}
	}
	iterate_level--;
}

/*
** active() -- TRUE if display_mode is TRUE in the specified stack frame. 
*/

static bool
active( STACK_FRAME *stack )
{
	for( ; stack != NULL; stack = stack->prev )
	{
		if( !stack->display_mode )
			return( FALSE );
	}
	return( TRUE );
}

/* values returned by pp_directive() */ 

# define NONE		0
# define DEFINE		1
# define UNDEF		2
# define IFDEF		3
# define IFNDEF		4
# define ELSE		5
# define ENDIF		6
# define IF             7


/*
** pp_directive() -- recursively called (indirectly via pp_input()) to
**	process input lines which contain preprocessor directives.  Calls
**	pp_input() before returning if the directive detected is #if, #ifdef
**	or #ifndef.  Returns an id number for the directive (see above).
*/

static u_i4 
pp_directive( FILE *input, STACK_FRAME *stack , bool ifdef )
{
	i4 n, len;
	char *words[ MAX_LINE / 2 ], *temp, *p;
	char *parse_val, *parse_end;
	STACK_FRAME stack_frame;
	bool def_dir;
	u_i4 rtn = NONE;

	stack_frame.prev = stack;
	stack_frame.display_mode = TRUE;

	p = temp = STalloc( infile->yytext );
	CMnext( p );
	n = sizeof( words );
	STgetwords( p, &n, words ); 

	/* process the directive, watch out for the empty directive */
	if (n == 0)
	{
		;       /* empty directive */
	}
	else if( STequal( words[ 0 ], ERx( "define" ) ) != 0 )
	{
		/* If a symbol was specified look for the value to give it */
		if (n > 1) 
		{
			/* Scan for the 'define' keyword */
			parse_val = infile->yytext;
			CMnext(parse_val);         /* Skip over the '#' */
			while (CMwhite(parse_val))
				CMnext(parse_val);

			/* Scan past the 'define' keyword */
			while (!CMwhite(parse_val))
				CMnext(parse_val);

			/* Scan over white space separating 'define' */
			/* keyword and the specified symbol name */
			while (CMwhite(parse_val))
				CMnext(parse_val);

			/* Skip over the symbol name */
			while (!CMwhite(parse_val))
				CMnext(parse_val);

			/* Skip over white space after the symbol name */
			while (CMwhite(parse_val))
				CMnext(parse_val);

			/* At this point we have scanned to the beginning */
			/* of the defined value for the symbol, Trim off */
			/* any trailing white space */
			STtrmwhite(parse_val);

			/* Define value found, could be the empty string, "" */
			if( active( &stack_frame ) )
			{
				define( words[ 1 ], parse_val, FALSE );
			}
		}
		rtn = DEFINE;
	}

	else if( active( &stack_frame ) &&
		STequal( words[ 0 ], ERx( "undef" ) ) != 0 )
	{
		if (n > 1)
		{
			undefine( words[ 1 ] );
		}
		rtn = UNDEF;
	}

	else if( STequal( words[ 0 ], ERx( "if" ) ) != 0 )
	{
		/* If an expression was specified look for its evaluation */
		if (n > 1) 
		{
			/* Scan for the 'if' keyword */
			parse_val = infile->yytext;
			CMnext(parse_val);         /* Skip over the '#' */
			while (CMwhite(parse_val))
				CMnext(parse_val);

			/* Scan past the 'if' keyword */
			while (!CMwhite(parse_val))
				CMnext(parse_val);

			/* Scan over white space separating 'if' */
			/* keyword and the expression */
			while (CMwhite(parse_val))
				CMnext(parse_val);

			/* At this point we have scanned to the beginning */
			/* of the expression.*/
			if( active( &stack_frame ) )
			{
				/* Evaluate boolean expression found */
				stack_frame.display_mode = 
					pp_eval_boolexp( &parse_val, ERx("") );
			}
			(void) pp_input( input, &stack_frame, TRUE );
		}
		rtn = IF;
	}

	else if( STequal( words[ 0 ], ERx( "ifdef" ) ) != 0 )
	{
		if( active(&stack_frame) && is_defined(words[1], NULL) == OK)
		{
			stack_frame.display_mode = TRUE;
		}
		else
			stack_frame.display_mode = FALSE;
		(void) pp_input( input, &stack_frame, TRUE );
		rtn = IFDEF;
	}

	else if( STequal( words[ 0 ], ERx( "ifndef" ) ) != 0 )
	{
		if( active(&stack_frame) && is_defined(words[1], NULL) != OK)
		{
			stack_frame.display_mode = TRUE;
		}
		else
			stack_frame.display_mode = FALSE;
		(void) pp_input( input, &stack_frame, TRUE );
		rtn = IFNDEF;
	}

	else if( STequal( words[ 0 ], ERx( "else" ) ) != 0 )
	{
		if( !ifdef )
		{
			SIfprintf( stderr, E_YAPP007 );
			yydump();
		}
		stack_frame.prev->display_mode =
			( stack_frame.prev->display_mode == TRUE ) ?
			FALSE : TRUE;
		rtn = ELSE;
	}

	else if( STequal( words[ 0 ], ERx( "endif" ) ) != 0 )
	{
		rtn = ENDIF;
	}

	else if( STequal( words[ 0 ], ERx( "include" ) ) != 0 )
	{
		/* Look for the include filename */
		if (n > 1) 
		{
			/* Scan for the 'include' keyword */
			parse_val = infile->yytext;
			CMnext(parse_val);         /* Skip over the '#' */
			while (CMwhite(parse_val))
				CMnext(parse_val);

			/* Scan past the include keyword */
			while (!CMwhite(parse_val))
				CMnext(parse_val);

			/* Scan over white space separating 'include' */
			/* keyword and the specified filename */
			while (CMwhite(parse_val))
				CMnext(parse_val);

			/* At this point were expecting "file" or <file> */
			/* remember the character which ends the filename */
			def_dir = TRUE;
			if (CMcmpcase(parse_val, ERx("\"")) == 0)
			{
				parse_end = ERx("\"");
			}
			else if (CMcmpcase(parse_val, ERx("<")) == 0)
			{
				parse_end = ERx(">");
				def_dir = FALSE;
			}
			else
			{
				parse_end = ERx("");
			}

			/* Save the include file name in the temp string. */
			/* Note, this overwrites the parsed words of the  */
			/* record since but these are no longer needed.   */
			p = temp;
			CMnext(parse_val);
			while (*parse_val != EOS)
			{
				if (CMcmpcase(parse_val, parse_end) == 0) 
				{
					/* Terminate the file name and call */
					/* pp_file to process the file. */
					STcopy(ERx(""), p);
					pp_file(stack, temp, def_dir);
					break;
				}
				CMcpychar(parse_val, p);
				CMnext(parse_val);
				CMnext(p);
			}
		}
		rtn = DEFINE;
	}

	/* display everthing but legal directives */ 
	if (rtn == NONE && active( &stack_frame ) )
		yyputline( infile->yytext );

	MEfree( temp );
	return( rtn );
}


/*
** pp_eval_boolexp() -- evalute an ANSI C style preprocessor if 
**      statement to a boolean expression.
**
**      This simply tries to evaluate an expression of the form:
**
**          boolval [ operator boolval ]
**
**      The catch is that boolval itself can actually evaluate to an
**      expression of the form    boolval [ operator ] boolval
**      so a simple recursive descent parsing algorithm is used.
**
**      The recursion is ended when a boolval is found of the form
**         [!] defined( symname ) 
**     
**      NOTE: Trickier boolean expressions are not yet supported, i.e.
**         if sizeof(FILE) < sizeof(FILE *)/sizeof(i4) || defined (OPT ## VAL)
**      This is left as an exercise for the reader.
**
*/

#define OPER_NONE 0
#define OPER_OR   1
#define OPER_AND  2

static bool
pp_eval_boolexp( char **strptr, char *goal )
{
	bool boolval[2];
	i4  notval[2];
	i4  oper = OPER_NONE;
	i4  curval = 0;
	i4  maxval;
	i4  goalfound = FALSE;
	char *parser = *strptr;
	bool rtn;

	notval[0] = notval[1] = 0;
	maxval = (sizeof(boolval) / sizeof(boolval[0])) - 1;
	while (goalfound == FALSE)
	{

		/* Are we through yet? */
		if( CMcmpcase( parser, goal ) == 0 )
		{
			goalfound = TRUE;
			CMnext(parser);
		}

		/* If the string is empty then bail out */
		else if( STlength( parser ) == 0)
			break;

		/* Have we exhausted our parsing capabilities? */
		else if (curval > maxval)
			break;

		/* If this is white space then just skip over it */
		else if( CMwhite( parser ))
			CMnext( parser );

		/* Is this an end to a parenthetical expression? */
		/* its not our goal, but it could be someones */
		else if( CMcmpcase( parser, ERx(")") ) == 0 &&
			*goal != EOS)
		{
			goalfound = TRUE;
			break;
		}

		/* Is this a NOT (!) to reverse the value of next boolean? */
		else if( CMcmpcase( parser, ERx("!") ) == 0 )
		{
			/* Use the xor operator to toggle, allows for !(!val) */
			notval[curval] ^= 1;
			CMnext(parser);
		}

		/* Is this a parenthetical expression? */
		else if( CMcmpcase( parser,ERx("(") ) == 0)
		{
			CMnext(parser);
			boolval[curval++] = pp_eval_boolexp(&parser, ERx(")"));
		}

		/* Is this an AND (&&) operator? */
 		else if( STbcompare( parser, 2, ERx("&&"), 2, 0 ) == 0) 
		{
			if (oper != OPER_NONE)
			{
				SIfprintf( stderr, E_YAPP001 );
				SIfprintf( stderr, E_YAPP00F );
				yydump();
			}
			oper = OPER_AND;

			CMnext(parser);
			CMnext(parser);

			boolval[curval++] = pp_eval_boolexp(&parser, ERx("\n"));
		}

		/* Is this an OR (||) operator? */
 		else if( STbcompare( parser, 2, ERx("||"), 2, 0 ) == 0) 
		{
			if (oper != OPER_NONE)
			{
				SIfprintf( stderr, E_YAPP001 );
				SIfprintf( stderr, E_YAPP011 );
				yydump();
			}
			oper = OPER_OR;

			CMnext(parser);
			CMnext(parser);

			boolval[curval++] = pp_eval_boolexp(&parser, ERx("\n"));
		}

		/* Is this the defined() preprocessor macro? */
 		else if( STbcompare( parser, 7, ERx("defined"), 7, 0 ) == 0 ) 
		{
			char defsym[MAX_SYMLEN + 1];
			char *defptr;

			/* Initialize the symbol name */
			STcopy(ERx(""), defsym);

			/* Try and find the beginning '(' */
			while (STlength(parser) > 0 &&
				CMcmpcase(parser, ERx("(") ) != 0)
				CMnext(parser);

			/* Skip over the open parenthesis, ( */
			if (STlength(parser) != 0)
				CMnext(parser);

			/* Skip over any whitespace following '(' */
			while (STlength(parser) > 0 && CMwhite(parser))
				CMnext(parser);

			/* Save the name of the symbol to look up */
			defptr = defsym;
			while (STlength(parser) > 0 &&
				CMcmpcase(parser, ERx(")") ) != 0 &&
				!CMwhite(parser))
			{
				CMcpychar(parser, defptr);
				CMnext(parser);
				CMnext(defptr);
			}
			*defptr = EOS;

			/* Try and find the ending ')' */
			while (STlength(parser) > 0 &&
				CMcmpcase(parser, ERx(")") ) != 0)
				CMnext(parser);

			/* Skip over the closed parenthesis, ) */
			if (STlength(parser) != 0)
				CMnext(parser);

			/* Make sure there was something in the parenthesis */
			if (STlength(defsym) == 0)
			{
				SIfprintf( stderr, E_YAPP001 );
				SIfprintf( stderr, E_YAPP013 );
				yydump();
			}
			boolval[curval++] = (is_defined(defsym, NULL) == OK);
		}

		/* Thats all we know how to parse, gotta bail out */
		else
		{
			SIfprintf( stderr, E_YAPP001 );
			SIfprintf( stderr, E_YAPP014 );
			yydump();
		}
	}

	/* Did we ultimately fail to evaluate the expression? */
	if (*goal == EOS && goalfound == FALSE)
	{
		SIfprintf( stderr, E_YAPP001 );
		SIfprintf( stderr, E_YAPP015 );
		yydump();
	}

	/* Now make sure something was specified in the current expression */
	if (curval == 0)
	{
		SIfprintf( stderr, E_YAPP001 );
		SIfprintf( stderr, E_YAPP016 );
		yydump();

	}

	/* If an operator is found make sure it had an object to operate on */
	if (oper != OPER_NONE && curval <= maxval)	
	{
		SIfprintf( stderr, E_YAPP001 );
		SIfprintf( stderr, E_YAPP017 );
		yydump();
	}

	/* Save the current location in parse string */
	*strptr = parser;

	/* Resolve the highest precedence NOT operators */
	if (curval > 0 && notval[0] == 1)
		boolval[0] = !boolval[0];
	if (curval > 1 && notval[1] == 1)
		boolval[1] = !boolval[1];

	/* Resolve the final expression */
	switch (oper)
	{
		case OPER_OR:
			rtn = boolval[0] || boolval[1];
			break;

		case OPER_AND:
			rtn = boolval[0] && boolval[1];
			break;

		default: 
			rtn = boolval[0]; 
			break;
	}
	return rtn;
}

/*
** pp_open() -- open a file for reading.  The use of the include path list
**      is used to resolve files that may not be in the current default
**      location.  
*/
static STATUS
pp_open( FILE **input, char *filepath, bool def_dir )
{
	LOCATION loc;
	LOCATION loctest;
	char buf[ MAX_LOC + 1 ];
	char bufdev[ MAX_LOC + 1 ];
	char bufpath[ MAX_LOC + 1 ];
	char bufsave[ MAX_LOC + 1 ];
	char buftest[ MAX_LOC + 1 ];
	STATUS sts;
	IPATH *ipath;
	bool found = FALSE;
	bool file_has_path, dir_has_path;

	/* 
	** Check to see if we should first look in the default directory,
	** this directory will be the first entry in the Include list.
	*/
	if (def_dir == TRUE) 
		ipath = &ihead;
	else
		ipath = ihead.next;

	/* Convert the filename to open to a LOCATION */
	STcopy(filepath, bufsave);
	if (OK != (sts = LOfroms( PATH & FILENAME, bufsave, &loc )) )
		return (sts);

	/*
	** Search for the file in each directory in the include list.
	** No expansion possible if there are no include directories.
	*/
	while (ipath != NULL && found == FALSE) 
	{
		/* Initialize loctest so LOaddpath will work */
		LOcopy(&loc, buftest, &loctest);
			
		/* Is there a device (VMS) and/or a path for the directory? */
		STcopy(ERx(""), bufdev);
		STcopy(ERx(""), bufpath);
		sts = LOdetail(ipath->pathloc, bufdev, bufpath, buf, buf, buf);
		if (sts != OK)
			break;
		if (STlength(bufdev) + STlength(bufpath) > 0 )
			dir_has_path = TRUE;
		else
			dir_has_path = FALSE;

		/* Don't mess with the filename if it was an absolute path */
		/* or if this include directory path is empty */
		if (LOisfull(&loctest) == FALSE && dir_has_path == TRUE )
		{
			/* Is there a path for the file? */
			sts = LOdetail(&loctest, 
				buf, bufpath, buf, buf, buf);
			if (sts != OK)
				break;
			file_has_path = (STlength(bufpath) > 0 ? TRUE : FALSE);

			/*
			** If there's no path component then copy in the 
			** next prefix, else append it.
			** If LOaddpath succeeds, the result will also have 
			** the original filename.  If it fails then drop out.
			** LOaddpath can fail if loctest is an absolute path,
			** but we already checked for that.
			*/
			if ( file_has_path )
			{
				sts =LOaddpath(ipath->pathloc, &loc, &loctest);
			}
			else
			{
				LOcopy(ipath->pathloc, buftest, &loctest);

				/*
				** If we can't set the filename from loc, 
				** then something is wrong, and will continue 
				** to be wrong.  Get out of the loop.
				*/
				sts = LOstfile(&loc, &loctest);
			}
		}

		/*
		** If all of the filename manipulations have succeeded
		** then check for the file's existance.  
		*/
		if (sts == OK && LOexist(&loctest) == OK) {
			found = TRUE;
			LOcopy(&loctest, buf, &loc);
		}

		/* Prepare for the next path in the include list */
		ipath = ipath->next;
	}

	/* 
	** If the file existed then return back the result from the open,
	** otherwise all of our efforts were a bust, return back a failure.
	*/ 
	if (found)
		return ( SIfopen(&loc, ERx("r"), SI_TXT, MAX_LINE, input) );
	else
		return (FAIL);
}

/*
** pp_input() -- recursively called (indirectly) to continue preprocessing
**	a specified input file.
**
**	The inputs to this function are the input file pointer (could be
**	used to support #include), a pointer to the current stack frame,
**	and the ifdef flag, which if TRUE indicates that the parser is in
**	the middle of parsing an #ifdef block.
*/

static i4
pp_input( FILE *input, STACK_FRAME *stack, bool ifdef )
{
	i4 token;
	u_i4 directive = NONE;

	while( directive != ENDIF && (token = yylex( input )) != tEOF )
	{
		switch( token )
		{
			case tSOURCE:
				if( active( stack ) )
					yyputline( infile->yytext );
				break;

			case tHISTORY:
				/* don't display */
				break;

			case tDIRECTIVE:
				directive = pp_directive( input, stack,
					ifdef );
				break;
		}
	}
	if( token == tEOF && ifdef )
	{
		SIfprintf( stderr, E_YAPP009 );
		yydump();
	}
	return( token );
}

/*
** pp_file() -- called to preprocess an input file.
*/

static void
pp_file( STACK_FRAME *stack, char *path_name, bool default_dir )
{
	LOCATION loc;
	char locbuf[ MAX_LOC + 1 ];

/*
** Get the next file on the input stack.  Check to see if there are
** any more available file positions.
*/
	if (++filex >= MAX_INCLUDE)
	{
		SIfprintf( stderr, E_YAPP00A );
		yydump();
	}

	/* Initialize the file entry */
	infile = &file[filex];
	if (path_name != NULL)                  /* Remember the file name */
        	STcopy(path_name,infile->name); 
	else
        	STcopy(ERx(""),infile->name);
	STcopy(ERx(""), infile->yytext);        /* Clear the input line */
	infile->yylineno = 0;			/* clear input line counter */
	infile->yyline_cache = FALSE;		/* clear line cache flag */
	infile->yyopen = FALSE;			/* Not yet opened */
	infile->yycomment = FALSE;		/* Haven't seen a comment yet */

	/* Try and open up the file, if no filename was specified */
	/* then open up the stdin */
	if( path_name == NULL )
	{
		infile->fp = stdin;
	}
	else if (pp_open(&infile->fp, path_name, default_dir) != OK)
	{
		SIfprintf( stderr, E_YAPP00B, path_name );
		yydump();
	}
	
	/* Mark the file as open, and pre-process its contents */ 
	infile->yyopen = TRUE;
	if( pp_input( infile->fp, stack, FALSE ) != tEOF )
	{
		SIfprintf( stderr, E_YAPP00C );
		yydump();
	}

	/* Check to see if the comments were left open in a file */
	if (infile->yycomment == TRUE)
	{
		SIfprintf( stderr, E_YAPP010 );
		yydump();
	}

	/* Close the file, mark the file as closed & reset the file index */ 
	SIclose( infile->fp );
	infile->yyopen = FALSE;
	if (--filex >= 0)
		infile = &file[filex];
}

/*
** main() for yapp.
*/

main( i4  argc, char **argv )
{
	bool usage = FALSE;
	STACK_FRAME stack_frame;
	char *filename = NULL;
	char buf[ MAX_LOC + 1];
	i4 i;
		
	stack_frame.prev = NULL;
	stack_frame.display_mode = TRUE;

	/* Initialize the Include directory list, make sure the resulting */
        /* path is not NULL by calling LOfaddpath with an empty path string */
	ihead.next = NULL;
	ihead.pathloc = &defdir;
	STcopy(ERx(""), buf);
	LOfroms(PATH, buf, ihead.pathloc);        /* Clear the default path */

	/* Process the command line */
	for( i = 1; i < argc; i++ )
	{
		char *p;
		char *v;

		/* process -C options */
 		if( STbcompare( argv[ i ], 0, ERx( "-C" ), 0, TRUE ) == 0)
		{
			pass_comments = TRUE;
			continue;
		}

		/* process -D options */
		if( STbcompare( argv[ i ], 2, ERx( "-D" ), 2, TRUE )
			== 0 && STlength( argv[ i ] ) > 2 )
		{
			p = argv[ i ];
			CMnext( p ); CMnext( p );

			/* Check for a value specified, -Dsymbol=value */
			if (NULL == (v = STindex(p, ERx("="), STlength(p)) ) )
				v = ERx("");
			else
				CMnext(v);
			define( p, v, FALSE );
			continue;
		}

		/* process -H option */
 		if( STbcompare( argv[ i ], 2, ERx( "-H" ), 2, TRUE )
			== 0 && STlength( argv[ i ] ) > 2 )
		{
			p = argv[ i ];
			CMnext( p ); CMnext( p );
			define( p, ERx(""), TRUE );
			continue;
		}

		/* process -I option */
 		if( STbcompare( argv[ i ], 2, ERx( "-I" ), 2, TRUE )
			== 0 && STlength( argv[ i ] ) > 2 )
		{
			p = argv[ i ];
			CMnext( p ); CMnext( p );
			yyadd_path( p );
			continue;
		}

		if( *argv[ i ] != '-' )
		{
			/* file name must be final argument */

 			if( i != argc - 1 )
			{
				usage = TRUE;
				break;
			}
			filename = argv[ i ];
		}
		else
			usage = TRUE;
		break;
	}

	if( usage )
	{
		SIfprintf( stderr, E_YAPP000 );
		PCexit( FAIL );
	}

	pp_file( &stack_frame, filename, TRUE );

	PCexit( OK );
}

