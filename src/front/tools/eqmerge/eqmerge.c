# include	<compat.h>
# include	<lo.h>
# include	<nm.h>
# include	<me.h>
# include	<st.h>
# include	<si.h>
# include	<cm.h>
# include	<ex.h>
# include	<cv.h>
# include	<pc.h>

/*
** EQMERGE - Equel grammar and token table merger.
**
** Old Usage:	eqmerge {grammar|tokens|language} G_input L_input output
** Usage:	eqmerge -{g|t|l} -I L_input_path G_input 
**
** Merges 
** 	1. Language dependent grammar (L) with the main Equel grammar (G).
**      2. L token table and G token table for main langauge.
**	3. G token table and L tokens table for language dependent test.
**
** Comment syntax:
**   1. Object begins with:
**		/* %{L|T} object [begin|end]
**   2. Use "L" when merging L data into the G grammar or tokens; 
**      use "T" when merging G data (tokens file) to build a language test.
**   3. Based on the context and what we are merging, we decide what we are
**      looking for.  See the G and L grammar and token files for C for 
**	examples.
**
** Notes:
**   1. To merge grammars the simple algorithm of 'text expansion' is used.
**      One file (the master) has the 'X' directive, while the other file
**      (the slave) has the block 'X begin' to 'X end' which is included
**	in the result.
**   2. Merging token tables is slightly more complex:  
**   2.1. When merging the token file for a main Equel language, one needs 
**	  to read the master file tokens, and all the language dependent 
**	  tokens that the slave wants to include.  Ie: The master will 
**	  have "message" and "menuitem" while the C slave may want to include 
**	  the "typedef" token. The L file (the slave) marks the ones it wants 
**	  to include. The  tokens are the sorted alphabetically for the 
**	  character set used.
**   2.2. When merging a token file for a language dependent test this tool
**	  first expands all the header and footer declarations from the G 
**	  (master) token file into a temp file (called II.00).  Then it reads
**	  the temp file, and just reads the token tables (tokens and operators)
**	  and does an insertion sort on them.  Its true, that we require that
**	  the files are ordered alphabetically according to the Ascii set, but
**	  this tool sorts them anyway (just in case).
**
** History:	25-jan-1985 - Written (ncg)
**		11-jan-1988 - Added ifdef generation for tokens (ncg)
**
** Copyright (c) 2004 Ingres Corporation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      10-Sep-2009 (frima01) 122490
**          Add include of cv.h and pc.h.
*/

/* Commands with "/* %L " , ie: "/* %L tokens " */
# define	MARK_LINE( b )	\
    		( b[0] == '/' && b[1] == '*' && b[2] == ' ' && b[3] == '%' && \
	 	  b[4] == CH && b[5] == ' ' )
# define	MARK_LEN	(i4)6
char		CH = '\0';

/* Reserved words for merging */
char	*kw_begin	= "begin"; 	/* Blocks controllers */
char	*kw_end		= "end";
char	*kw_fake	= "fake";	/* Fake L rules in G to allow Yaccing */
char	*kw_merge	= "merge";	/* file to merge */

/* Note: "tokens" and "ops" are reserved when merging token files */

/* Error exit status */
# define	E_NOERR		(i4)0	
# define	E_NOSYNC	(i4)1	/* Bad object name */
# define	E_BADBLOCK	(i4)2	/* Begin/end out of order */
# define	E_EOF		(i4)3	/* Premature Eof */
# define	E_BADOPEN	(i4)4	/* Cannot open file */
# define	E_BADLINE	(i4)5	/* Bad input line in token tables */
# define	E_NOMEM		(i4)6	/* Out of memory */
# define	E_NOTOKS	(i4)7	/* No tokens merged */
# define	E_BADTOK	(i4)8	/* Ambiguous tokens in G and L */
# define	E_USAGE		(i4)9	/* Usage error */
# define	E_NOMERGE	(i4)10	/* no merge-file */
# define	E_MSYNTAX	(i4)11	/* bad merge merge syntax */
# define	E_NOTFOUND	(i4)12	/* merge file not found */

/* Size of objects */
# define	IN_SIZE		255
# define	NAME_SIZE	30

/* When merging token tables whether to skip comments at start of line */
# define	NO_SKIP		(i4)0
# define	SKIP		(i4)1

/* Structure describing content of token table - 2 names */
typedef struct tk
{
    char	*tk_name;			/* Name of keyword/operator */
    char	*tk_tok;			/* Yacc defined constant */
    char	*tk_who;			/* From G or L */
    char	*tk_ifdef;			/* Ifdef SYS word */
    struct tk	*tk_next;
} TOK_ELM;

FILE		*G_f = (FILE *)0;
FILE		*L_f = (FILE *)0;
FILE		*Outf = (FILE *)0;

#define PCOUNT 10
char *Path_names[PCOUNT];

FUNC_EXTERN	LOCATION	*open_file();
FUNC_EXTERN	void	get_name();
FUNC_EXTERN	void	get_object();
FUNC_EXTERN	void	get_block();
FUNC_EXTERN	void	get_fake();
FUNC_EXTERN	void	er_exit();
FUNC_EXTERN	FILE    *get_merge();
FUNC_EXTERN	FILE    *find_merge();
FUNC_EXTERN	void	file_expand();
FUNC_EXTERN	i4	merge_tok();
FUNC_EXTERN	void	add_token();
FUNC_EXTERN	void	print_tokens();
FUNC_EXTERN	i4	sc_compare();	/* From the Equel scanner + backslash */
FUNC_EXTERN	i4	ex_handle();

/*
** ming hints

PROGRAM =	eqmerge

NEEDLIBS =	COMPATLIB

UNDEFS =	II_copyright
*/

main( argc, argv )
i4	argc;
char	*argv[];
{
    char		mcode;
    EX_CONTEXT		context;
    int			argi = 1; 
    char		*outname = (char *) 0;
    char		*gname = (char *) 0;
    char		*lname = (char *) 0;
    char		*p;
    i4			pcnt = 0;

    if ( argc < 2 )
	er_exit( E_USAGE );

    /* backward compatibility: 1st arg may be magic action key. */
    if (*argv[1] != '-')
    {
	mcode = *argv[1];
	argi = 2;
    }

    for (; argi < argc; argi++)
    {
	if (argv[argi][0] != '-')
	    break;

	switch (argv[argi][1])
	{
	  case 'i':
	  case 'I':
	    if (pcnt < (PCOUNT - 1))
		Path_names[pcnt++] = &argv[argi][2];
	    Path_names[pcnt] = (char *) 0;
	    break;

	  case 'o':
	  case 'O':
	    if (argv[argi][2] != '\0')
		outname = &argv[argi][2];
	    else
		outname = argv[++argi];
	    break;

	  case 'l':
	  case 'L':
	    outname = "II.00";	/* Temp file */
	    /* fall through to ... */
	  case 'g':
	  case 'G':
	  case 't':
	  case 'T':
	    mcode = argv[argi][1];
	    break;

	  default:
	    er_exit( E_USAGE );
	}
    }
    if (argi >= argc || mcode == '\0')
	er_exit( E_USAGE );

    lname = argv[argi++];
    /* output defaults to stdout */
    Outf = stdout;

    if (argi < argc)
	gname = argv[argi++];
    if (argi < argc)
    {
	char *tmp;

	/* gross backward-compatibility hack. juggle names. */
	outname = argv[argi++];
	tmp = lname;
	lname = gname;
	gname = tmp;
    }

    _VOID_ open_file( "L input", lname, "r", &L_f );
    _VOID_ open_file( "G input", gname, "r", &G_f );
    _VOID_ open_file( "output", outname, "w", &Outf );

    SIfprintf( Outf, "/*\n** Generated by: eqmerge %s %s %s %s\n*/\n", 
	       argv[1], argv[2], argv[3], argv[4] );

    EXdeclare( ex_handle, &context );

    if ( mcode == 'g' )
    {
	CH = 'L';
	if (G_f == (FILE *) 0)
	    G_f = find_merge( lname, L_f, &gname, Path_names );
	file_expand( gname, G_f, lname, L_f );
    }
    else if ( mcode == 't' )
    {
	CH = 'L';
	if (G_f == (FILE *) 0)
	    G_f = find_merge( lname, L_f, &gname, Path_names );
	if (merge_tok( FALSE, gname, G_f, lname, L_f ) <= 0)
	    er_exit( E_NOTOKS );
    }
    else if ( mcode == 'l' )
    {
	LOCATION	*tmp_loc;

	CH = 'T';
	file_expand( lname, L_f, gname, G_f );
	/* Second pass */
	SIclose( G_f );
	SIclose( L_f );
	SIclose( Outf );
	Outf = G_f = L_f = (FILE *)0;
	_VOID_ open_file( "output", argv[4], "w", &Outf );
	tmp_loc = open_file( "Temp input", "II.00", "r", &L_f );
	CH = 'L';
	if (merge_tok( TRUE, NULL, NULL, lname, L_f ) <= 0)
	    er_exit( E_NOTOKS );
	LOdelete( tmp_loc );
    }

    er_exit( E_NOERR );
}

/*
** file_expand - Merge files. For each marked line in source 'file1' (owned 
**		 by 'who1') get the block in source 'file2' (owned by 'who2').
*/
void
file_expand( who1, file1, who2, file2 )
char	*who1;
FILE	*file1;
char	*who2;
FILE	*file2;
{
    char		inbuf[IN_SIZE +1];
    char		*inptr;				/* Input line ptr */
    char		name1[NAME_SIZE +1];		/* file1 object name */
    char		name2[NAME_SIZE +1];		/* file2 object name */
    char		tmp_name[NAME_SIZE +1];		/* Secondary names */
    bool		in_merge;

    while ( SIgetrec(inbuf, IN_SIZE, file1) == OK )
    {
	SIputrec( inbuf, Outf );
	if ( !MARK_LINE(inbuf) )
	    continue;

	/* Line begins with special marker - get L object name */
	get_name( inbuf+MARK_LEN, name1, &inptr );

	/* 
	** Check for "/* %L fake begin" 
	** If we see in file1 "fake begin" then strip all lines till "fake end".
	*/
	if ( STcompare(name1, kw_fake) == 0 )
	{
	    get_block( who1, name1, kw_begin, inptr+1 );
	    get_fake( who1, file1 );
	    continue;
	}

	/* 
	** name1 has the object we are merging. Look for corresponding
	** object in file2. Skip in file2 till we get to the "begin" of 
	** that object.
	*/
	in_merge = TRUE;
	while ( SIgetrec(inbuf, IN_SIZE, file2) == OK )
	{
	    if ( !MARK_LINE(inbuf) )
		continue;

	    SIputrec( inbuf, Outf );
	    get_name( inbuf+MARK_LEN, tmp_name, &inptr );
	    if ( STcompare(tmp_name, kw_fake) == 0 )
	    {
		get_block( who2, kw_fake, kw_begin, inptr+1 );
		get_fake( who2, file2 );
		continue;
	    }

	    /* ignore "merge" directive, we've already handled it */
	    if ( STcompare(tmp_name, kw_merge) == 0 )
		continue;

	    in_merge = FALSE;
	    break;
	}

	if ( in_merge )		/* Eof before finding object */
	    er_exit( E_EOF, who2, name1 );
	/* 
	** file2 object name and file1 object name better be the same and we 
	** must find the "begin" of that object.
	** For example:
	**   file1:	/* %L tokens 
	**   file2:	/* %L tokens begin
	**     		/* %L tokens end
	*/
	get_object( who2, name2, name1, inbuf+MARK_LEN, &inptr );
	get_block( who2, name2, kw_begin, inptr+1 );
	/* Copy all data inside the file2 block */
	in_merge = TRUE;
	while ( SIgetrec(inbuf, IN_SIZE, file2) == OK )
	{
	    SIputrec( inbuf, Outf );
	    if ( MARK_LINE(inbuf) )
	    {
		get_name( inbuf+MARK_LEN, tmp_name, &inptr );
		if ( STcompare(tmp_name, kw_fake) == 0 )
		{
		    get_block( who2, kw_fake, kw_begin, inptr+1 );
		    get_fake( who2, file2 );
		    continue;
		}
		in_merge = FALSE;
		break;
	    }
	}
	if ( in_merge )		/* Eof before merging name1 in L */
	    er_exit( E_EOF, who2, name1 );
	/* This line better be the closing "object end" */
	get_object( who2, name2, name1, inbuf+MARK_LEN, &inptr );
	get_block( who2, name2, kw_end, inptr+1 );
	in_merge = FALSE;
    }
}

/*
** merge_tok - Merge tokens tables.
**
** Get all the tokens in 'file1' (owned by 'who1') and the marked ones 
** in 'file2' (owned by 'who2').  Do this for the same named objects
** - probably "tokens" or "ops" - and store them.  Sort them, and print 
** them to the output file. The 'is_lang' argument tells if we are just
** merging for a language dependent test.
*/
i4
merge_tok( is_lang, who1, file1, who2, file2 )
i4	is_lang;		/* Language test on */
char	*who1;
FILE	*file1;
char	*who2;
FILE	*file2;
{
    char		inbuf[IN_SIZE +1];
    char		closebuf[IN_SIZE +1];		/* Closing file1 cmnt */
    char		*inptr;
    char		name1[NAME_SIZE +1];		/* file1 object name */
    char		name2[NAME_SIZE +1];		/* file2 object name */
    char		tmp_name[NAME_SIZE +1];		/* Secondary name */
    bool		in_merge;
    TOK_ELM		*out_toks;			/* Sorted table */
    static long		tcount = 0;

    for ( ; ; )
    {
	/* After the first time through we really should free up space */
	out_toks = NULL;

	if ( is_lang )		/* Simulate getting objects from file1 */
	{
	    static i4 first = 1;

	    if ( first )
	    {
		STcopy( "tokens", name1 );
		first = 0;
	    }
	    else
		STcopy( "ops", name1 );
	}
	else			/* Not lang test - get objects from file1 */
	{
	    in_merge = TRUE;
	    while ( SIgetrec(inbuf, IN_SIZE, file1) == OK )
	    {
		SIputrec( inbuf, Outf );
		if ( MARK_LINE(inbuf) )
		{
		    get_name( inbuf+MARK_LEN, name1, &inptr );
		    if ( STcompare(name1, kw_merge) == 0 )
			continue;

		    in_merge = FALSE;
		    break;
		}
	    }
	    if ( in_merge )			/* Reached last Eof */
		return (tcount);

	    /* Line begins with special marker - get L object name "begin" */
	    get_name( inbuf+MARK_LEN, name1, &inptr );

	    get_block( who1, name1, kw_begin, inptr+1 );
	    in_merge = TRUE;

	    /* Skip in G till closing marker storing all tokens */
	    while ( SIgetrec(inbuf, IN_SIZE, file1) == OK )
	    {
		if ( MARK_LINE(inbuf) )
		{
		    get_object( who1, tmp_name, name1, inbuf+MARK_LEN, &inptr );
		    get_block( who1, name1, kw_end, inptr+1 );
		    in_merge = FALSE;
		    STcopy( inbuf, closebuf );
		    break;
		}
		/* Comments must start at the beginning of the line */
		if ( inbuf[0] !='/' && inbuf[1] != '*' )
		{
		    add_token( NO_SKIP, who1, &out_toks, inbuf );
		    tcount++;
		}
	    }
	    if ( in_merge )
		er_exit( E_EOF, who1, name1 );

	}

	/* Skip in file2 till same "object begin" */
	in_merge = TRUE;
	while ( SIgetrec(inbuf, IN_SIZE, file2) == OK )
	{
	    if ( MARK_LINE(inbuf) )
	    {
		get_name( inbuf+MARK_LEN, name2, &inptr );
		if ( STcompare(name2, kw_merge) == 0 )
		    continue;

		in_merge = FALSE;
		break;
	    }
	    else if ( is_lang )		/* Dump the stuff */
		SIputrec( inbuf, Outf );
	}
	if ( in_merge )			/* Gotten to end of file */
	{
	    if ( is_lang )
		return (tcount);
	    else
		er_exit( E_EOF, who2, name1 );
	}

	if ( STcompare(name2, name1) != 0 )
	    er_exit( E_NOSYNC, who2, name1, name2 );

	get_block( who2, name2, kw_begin, inptr+1 );
	in_merge = TRUE;
	/* 
	** Continue in L till closing marker, and store marked tokens.
	** Example:
	**  /* % tokens begin 
	**     			"dummy", 	tDUMMY,	 -- do not add
	**  /* % tokens *]	"param",	tPARAM,	 -- add
	**  /* % tokens end
	*/
	while ( SIgetrec(inbuf, IN_SIZE, file2) == OK )
	{
	    if ( MARK_LINE(inbuf) )
	    {
		get_object( who2, tmp_name, name2, inbuf+MARK_LEN, &inptr );
		get_name( inptr+1, tmp_name, &inptr );
		if ( *tmp_name == '\0' )	/* An object to add */
		{
		    add_token( SKIP, who2, &out_toks, inptr );
		    tcount++;
		    continue;
		}

		/* Better be the closing object */
		if ( STcompare(tmp_name, kw_end) != 0 )
		    er_exit( E_BADBLOCK, who2, tmp_name, name2, kw_end );

		if ( is_lang )
		    STcopy( inbuf, closebuf );
		in_merge = FALSE;
		break;
	    }
	    else if ( is_lang )		/* Unmarked language test object */
	    {
		add_token( NO_SKIP, who2, &out_toks, inbuf );
		tcount++;
	    }
	}
	if ( in_merge )				/* Eof before closing object */
	    er_exit( E_EOF, who2, name1 );
	print_tokens( out_toks );
	SIputrec( closebuf, Outf );
    }
}

/* 
** open_file - Open input/output files 
*/
LOCATION *
open_file( err, name, mode, fp )
char	*err;
char	*name;
char	*mode;
FILE	**fp;
{
    static char		locbuf[MAX_LOC];
    static LOCATION		loc;

    if (name == (char *) 0)
	return((LOCATION *) 0);

    STcopy( name, locbuf );
    LOfroms( PATH & FILENAME, locbuf, &loc );
    if ( SIopen(&loc, mode, fp) != OK )
	er_exit( E_BADOPEN, err, name );
    return &loc;
}

/* 
** er_exit - Exit on any error 
*/
void
er_exit( ernum, a1, a2, a3, a4 )
i4	ernum;
char	*a1, *a2, *a3, *a4;
{
    i4	stat = -1;

    switch( ernum )
    {
      case E_NOERR:
	stat = OK;
	break;

      case E_NOSYNC: 
	SIprintf( "Unsynchronized object name found in \"%s\".\n", a1 );
	SIprintf( "   Found \"%s\" when looking for \"%s\".\n",a2, a3 );
	break;
    
      case E_BADBLOCK: 
	SIprintf( "Unsynchronized block found in \"%s\".\n", a1 );
	SIprintf( "   Found \"%s %s\" when looking for \"%s %s\".\n",
		  a3, a2, a3, a4 );
	break;

      case E_EOF:
	SIprintf( "Found EOF in \"%s\" when merging for \"%s\".\n", a1, a2 );
	break;

      case E_BADOPEN:
	SIprintf( "Cannot open %s file \"%s\".\n", a1, a2 );
	break;

      case E_BADLINE:
	SIprintf( "Illegal input line in \"%s\": %s in \"%s\"\n", a2, a1, a3 );
	break;

      case E_NOMEM:
	SIprintf( "No memory for \"%s\".\n", a1 );
	break;

      case E_NOTOKS:
	SIprintf( "No Tokens or Operators while merging.\n" );
	break;

      case E_NOMERGE:
	SIprintf( "Found eqmerge directive before merge file specified.\n" );
	break;

      case E_MSYNTAX:
	SIprintf( "bad merge syntax.\n" );
	break;

      case E_NOTFOUND:
	SIprintf( "merge file not found: \"%s\"\n", a1 );
	break;

      case E_BADTOK:
	SIprintf( "Ambiguity between 2 token representations (ignore case)\n" );
	SIprintf( "  First  : name = \"%s\",\ttoken integer = %s\n", a1, a2 );
	SIprintf( "  Second :        \"%s\",\t              = %s\n", a3, a4 );
	break;

      case E_USAGE:
	SIprintf( "Usage: eqmerge -{g|t|l} [-I<path>] [-o <output>] L_input\n" );
	SIprintf( "  '-g' to merge a grammar;\n" );
	SIprintf( "  '-t' to merge a token file;\n" );
	SIprintf( "  '-l' to merge a language dependent token test file.\n" );
	SIprintf( "  <path> is search path for master merge-file.\n" );
	break;
    }
    if ( G_f )
	SIclose( G_f );
    if ( L_f )
	SIclose( L_f );
    if ( Outf )
	SIclose( Outf );
    PCexit( stat );
}

/*
** add_token - Got an input line [with a marker comment in front], strip off
**	       the token name, the Yacc token constant, and store in the passed
** 	       output token table in order.
** Format:     "name",		tTOKEN,		[/ * =IFDEFNAME * /]
*/
void
add_token( skipflag, who, otoks, inbuf )
i4	skipflag;				/* Comment to skip */
char	*who;					/* G or L */
TOK_ELM	**otoks;				/* Output token table */
char	*inbuf;					/* Input buffer */
{
    register	char	*cp1, *cp2;
    TOK_ELM		*t;
    register    TOK_ELM	*sortt;
    TOK_ELM		*prevt;
    char		namebuf[NAME_SIZE +3];
    char		tokbuf[NAME_SIZE +1];
    char		ifdefbuf[NAME_SIZE +1];
    char		*ifdefptr = (char *)0;

    cp1 = inbuf;
    if ( skipflag == SKIP )			/* Skip to end of comment */
    {
	for ( ; *cp1; cp1++ )
	{
	    if ( *cp1 == '*' && *(cp1+1) == '/' )
	    {
		cp1 += 2;
		break;
	    }
	}
	if ( *cp1 == '\0' )
	    er_exit( E_BADLINE, "premature EOS", who, inbuf );
    }
    while ( *cp1 == ' ' || *cp1 == '\t' )	/* Skip blanks */
	cp1++;

    if ( *cp1 != '"' )			/* Better be the name */
    {
	/* not a token.  Just output it. */
	SIputrec( cp1, Outf );
	return;
    }

    cp1++;

    for ( cp2 = namebuf; *cp1 && *cp1 != '"' && cp2 < &namebuf[NAME_SIZE]; )
    {
	if ( *cp1 == '\\' )
	    *cp2++ = *cp1++;
	*cp2++ = *cp1++;
    }
    *cp2 = '\0';				/* Got the name */

    if ( *cp1 != '"' )				/* Closing quote */
	er_exit( E_BADLINE, "no closing quote", who, inbuf );

    cp1 += 2;					/* Skip the quote and comma */
    while ( *cp1 == ' ' || *cp1 == '\t' )	/* Skip blanks */
	cp1++;

    /* Allow negative Yacc constants for flagging */
    if ( !CMalpha(cp1) && *cp1 != '-' )
	er_exit( E_BADLINE, "non-numeric token value", who, inbuf );

    for ( cp2 = tokbuf; *cp1 && cp2 < &tokbuf[NAME_SIZE]; )
    {
	if ( !CMnmchar(cp1) && *cp1 != '_' && *cp1 != '-' )
	    break;
	*cp2++ = *cp1++;
    }
    *cp2 = '\0';				/* Got the constant */
    if (cp1 = STindex(cp1, "=", 0))		/* Look for =IFDEFNAME */
    {
	cp1++;					/* Skip = */
	while (*cp1 == ' ' || *cp1 == '\t')	/* Skip blanks */
	    cp1++;
	for (cp2=ifdefbuf; *cp1 && CMalpha(cp1) && cp2 < &ifdefbuf[NAME_SIZE];)
	{
	    *cp2++ = *cp1++;
	}
	*cp2 = '\0';				/* Got ifdef name */
	ifdefptr = STalloc(ifdefbuf);
    }
    if ((t = (TOK_ELM *)MEreqmem((u_i4)0, (u_i4)sizeof(TOK_ELM), TRUE, 
		(STATUS *)NULL)) == NULL)
	er_exit( E_NOMEM, "Token element" );

    t->tk_name = STalloc(namebuf);
    t->tk_tok = STalloc(tokbuf);
    t->tk_ifdef = ifdefptr;
    t->tk_who = who;

    /* Insert in sorted order - insertion sort */
    for ( prevt = NULL, sortt = *otoks; sortt;  
	  prevt = sortt, sortt = sortt->tk_next )
    {
	/* 
	** Use a similar routine to sc_compare() because sc_getkey() will use 
	** it too. This routine must ignore '\' characters put into the tables
	** for the C compiler.
	*/
	switch ( sc_compare(t->tk_name, sortt->tk_name) )
	{
	  case -1:
	    break;

	  case 0:
	    /* 
	    ** If the names are the same then  the Yacc token constants better 
	    ** be the same too.
	    */
	    if ( STcompare(t->tk_tok, sortt->tk_tok) != 0 )
	    {
		print_tokens( *otoks );		/* See where we are */
		er_exit( E_BADTOK, sortt->tk_name, sortt->tk_tok, t->tk_name, 
			 t->tk_tok ); 
	    }
	    /*
	    ** Remove the existing one as L tokens override
	    ** If "ifdefptr" is set then allow multiple ones so we can
	    ** have cases like:
	    **		"ref",		tPVAR,		/ * =MVS * /
	    **		"ref",		tPVAR,		/ * =CMS * /
	    ** generating both with an ifdef around them.
	    */
	    if (ifdefptr == (char *)0)
	    {
		prevt->tk_next = sortt->tk_next;	
		sortt->tk_next = NULL;
		break;
	    }
	    else
	    {
		/* Continue as though this was not found */
		continue;
	    }

	  case 1:
	    continue;
	}
	break;
    }
    if ( prevt == NULL )
    {
	t->tk_next = *otoks;
	*otoks = t;
    }
    else
    {
	t->tk_next = prevt->tk_next;
	prevt->tk_next = t;
    }
}

/* 
** print_tokens - Print all tokens, anf from whom 
*/
void
print_tokens( outtoks )
TOK_ELM	*outtoks;
{
    register	TOK_ELM		*t;
    register	char		*cp;
    i4				i;

    if ( outtoks == NULL )
	return;
    for ( t = outtoks; t; t = t->tk_next )
    {
	if (t->tk_ifdef)
	    SIfprintf(Outf, "#ifdef %s\n", t->tk_ifdef);

	SIfprintf( Outf, "/* From %s */\t\t\"", t->tk_who );
	SIfprintf( Outf, "%s", t->tk_name );
	SIfprintf( Outf, "\",\t" );
	if ( (i = STlength(t->tk_name)) <= 4 )		/* Align tabbing */
	    cp = "\t\t";
	else if ( i <= 12 )
	    cp = "\t";
	else
	    cp = "";
	SIfprintf( Outf, "%s%s", cp, t->tk_tok );
	if ( t->tk_next )
	    SIfprintf( Outf, "," );
	SIfprintf( Outf, "\n" );

	if (t->tk_ifdef)
	    SIfprintf(Outf, "#endif /* %s */\n", t->tk_ifdef);
    }
    SIflush( Outf );
}

/* 
** get_name - Strip off name from 'str' into 'buf', and update passed 'inptr' 
*/
void
get_name( str, buf, inptr )
register char	*str;
char		buf[];
char		**inptr;
{
    register	char    *cp;

    for ( cp = buf; CMalpha(str) && cp < &buf[NAME_SIZE]; )
	*cp++ = *str++;
    *cp = '\0';
    CVlower( buf );
    *inptr = str;		/* Non-alpha or space */
}

/* 
** get_block - 'who' assumes we will read a 'beg_end' from 'str'; if not error
*/
void
get_block( who, object, beg_end, str )
char	*who;
char	*object;
char	*beg_end;
char	*str;
{
    char	buf[NAME_SIZE +1];

    get_name( str, buf, &str );
    if ( STcompare(buf, beg_end) != 0 )
	er_exit( E_BADBLOCK, who, buf, object, beg_end );
}

/* 
** get_object - 'who' wants to read from 'str' the 'object' into 'buf'. If not
**		not 'object' then error.  Update the passed 'inptr' on reading.
*/
void
get_object( who, buf, object, str, inptr )
char	*who;
char	*buf;
char	*object;
char	*str;
char	**inptr;
{
    get_name( str, buf, inptr );
    if ( STcompare(buf, object) != 0 )
	er_exit( E_NOSYNC, who, buf, object );
}

/*
** get_fake - Remove fake data.
*/
void
get_fake( who, f )
char	*who;
FILE	*f;
{
    char		inbuf[IN_SIZE +1];
    char		*inptr;				/* Input line ptr */
    char		name[NAME_SIZE +1];		/* file1 object name */
    bool		in_merge;

    in_merge = TRUE;
    while ( SIgetrec(inbuf, IN_SIZE, f) == OK )
    {
	if ( MARK_LINE(inbuf) )
	{
	    in_merge = FALSE;
	    break;
	}
    }
    if ( in_merge )		/* Eof before finding "fake" */
	er_exit( E_EOF, who, kw_fake );

    SIputrec( inbuf, Outf );

    /* This line better be the closing "fake end" */
    get_object( who, name, kw_fake, inbuf+MARK_LEN, &inptr );
    get_block( who, name, kw_end, inptr+1 );
}

/*
** sc_compare - Compare strings, but ignore embedded backslashes that escape.
*/
i4
sc_compare( ap, bp )
register char	*ap;
char		*bp;
{
    char	a[2];
    char	b[2];

    for ( ;; )
    {
	/* Do inequality tests ignoring case - and skipping escapes */
	CMtolower(ap, a);
	if ( a[0] == '\\' )
	{
	    ap++;
	    CMtolower(ap, a);
	}
	CMtolower(bp, b);
	if ( b[0] == '\\' )
	{
	    bp++;
	    CMtolower(bp, b);
	}
	if ( a[0] < b[0] )
	    return  (i4)-1;
	if ( a[0] > b[0] )
	    return (i4)1;
	if ( a[0] == '\0' )
	    return (i4)0;
	ap++; 
	bp++;
    }
}

/*
** ex_handle - Exception handler.
*/

i4
ex_handle( ex )
EX_ARGS	*ex;
{
    if ( ex->exarg_num == EXINTR )
    {
	if ( Outf )
	    SIfprintf( Outf, "\n" );
	SIprintf( "\nAborting...\n" );
	er_exit( E_NOERR );
    }
    if ( G_f ) SIclose( G_f );
    if ( L_f ) SIclose( L_f );
    if ( Outf ) SIclose( Outf );
    return( EXRESIGNAL );
}

FILE *
get_merge(nstr, ilist, name)
char *nstr;
char *ilist[];
char **name;
{
    char match[2];
    char *p;
    static LOCATION	fname;
    i4  i;
    static char nbuf[MAX_LOC+1];
    static char nmsave[MAX_LOC+1];

    if (nstr == (char *) 0)
	return ((FILE *) 0);
    
    while (*nstr == ' ' || *nstr == '\t')
	nstr++;

    if (*nstr == '\'' || *nstr == '"')
	*match = *nstr;
    else if (*nstr == '<')
	*match = '>';
    else
	er_exit( E_MSYNTAX );

    nstr++;
    match[1] = '\0';
    p = STindex(nstr, match, 0);
    if (p == (char *) 0)
	er_exit( E_MSYNTAX );
    
    *p = '\0';

    /* now nstr points to the filename. */
    STlcopy(nstr, nbuf, sizeof(nbuf) -1);
    STlcopy(nstr, nmsave, sizeof(nmsave) -1);
    *name = nmsave;

    LOfroms( FILENAME, nbuf, &fname );

    for (i = 0; ilist[i] != (char *) 0; i++)
    {
	char	    pathbuf[MAX_LOC+1];
	char	    *logname;
	FILE 	    *fp;
	LOCATION    path;

	logname = (char *)0;
	NMgtAt(ilist[i], &logname );			/* Maybe a logical */
	if (logname != (char *)0 && *logname != '\0')
	    STlcopy(logname, pathbuf, sizeof(pathbuf)-1);
	else
	    STlcopy(ilist[i], pathbuf, sizeof(pathbuf)-1);
	LOfroms( PATH , pathbuf, &path );

	LOstfile(&fname, &path);

	_VOID_ SIopen(&path, "r", &fp);

	if (fp != (FILE *)0)
	    return (fp);
    }

    er_exit( E_NOTFOUND, nstr );
    /* NOTREACHED */
}

FILE *
find_merge( who1, file1, who2, ilist )
char	*who1;
FILE	*file1;
char	**who2;
char 	*ilist[];
{
    char		inbuf[IN_SIZE +1];
    char		*inptr;				/* Input line ptr */
    char		name1[NAME_SIZE +1];		/* file1 object name */
    FILE		*file2;

    while ( SIgetrec(inbuf, IN_SIZE, file1) == OK )
    {
	if ( !MARK_LINE(inbuf) )
	    continue;

	/* Line begins with special marker - get L object name */
	get_name( inbuf+MARK_LEN, name1, &inptr );

	/* Check for %L merge "fname" */
	if ( STcompare(name1, kw_merge) == 0 )
	{
	    /* find and open the master file */
	    file2 = get_merge(inptr, ilist, who2);

	    /* reset the slave file */
	    SIfseek(file1, (long) 0, SI_P_START);
	    return (file2);
	}

	/* uh oh, got a directive but no "merge" */
	break;
    }
    er_exit( E_NOMERGE );
    /* NOTREACHED */
}

