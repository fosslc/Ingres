/*
** Copyright (c) 1993, 2008 Ingres Corporation
*/
/*	@(#)y2.c	1.8	*/
#include    <compat.h>
#include    <cm.h>
#include    <lo.h>
#include    <si.h>
#include    <st.h>
#include    <cv.h>

#include "dextern.h"

/*
**	y2.c - file for yacc 'grammar preprocessor'
**
** History:
**	27-may-93 (rblumer)
**	    add "nat" to printf for 'iftn' function declarations;
**	    this will eliminate some gcc warnings.
**	    Also, move bypass/nobypass output before lineno output,
**	    so that the debugger can correctly decipher what line you're on.
**	10-Jun-1993 (daveb)
**	    Add include of st.h
**	14-oct-93 (swm)
**	    Bug #56448
**	    Altered ERROR calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to ERROR.
**	    This works because STprintf is a varargs function in the CL.
**	23-sep-1996 (canor01)
**	    Move global data definitions to yaccdata.c
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-jan-06 (dougi)
**	    Change YYVARS* to YYVARS** in generated code to support 
**	    reentrant calls for derived tables (subselects in the FROM clause).
**  26-Jun-2006 (raddo01)
**		bug 116276: SEGV on double free() when $ING_SRC/lib/ doesn't exist.
**	28-feb-08 (smeke01) b120003
**	    On Windows escape all backslashes in the input filename so that the 
**	    emit #line directives option works without warnings.
**	29-feb-08 (smeke01) b120033
**	    The -l option should not be a toggle.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
*/

# define IDENTIFIER 257
# define MARK 258
# define TERM 259
# define LEFT 260
# define RIGHT 261
# define BINARY 262
# define PREC 263
# define LCURLY 264
# define C_IDENTIFIER 265	/* name followed by colon */
# define NUMBER 266
# define START 267
# define TYPEDEF 268
# define TYPENAME 269
# define UNION 270
# define NOBYPASS 271
# define YENDFILE 0

 /* communication variables between various I/O routines */

GLOBALREF   char   *infile;			/* input file name */
GLOBALREF   char   infile2[MAX_LOC + MAX_LOC];
GLOBALREF   i4      numbval;		/* value of an input number */
GLOBALREF   char    tokname[NAMESIZE];	/* input token name */

 /* storage of names */

/* place where token and nonterminal names are stored */
GLOBALREF   char    cnames[CNAMSZ];
GLOBALREF   i4      cnamsz;	/* size of cnames */
GLOBALREF   char   *cnamp; /* place where next name is to be put in */
GLOBALREF   i4      ndefout;    /* number of defined symbols output */

 /* storage of types */
GLOBALREF   i4      ntypes;		/* number of types defined */
GLOBALREF   char   *typeset[NTYPES];	/* pointers to type tags */

 /* symbol tables for tokens and nonterminals */

GLOBALREF   i4      ntokens;
GLOBALREF   struct toksymb  tokset[NTERMS];
GLOBALREF   i4      toklev[NTERMS];
GLOBALREF   i4      nnonter;
GLOBALREF   struct ntsymb   nontrst[NNONTERM];
GLOBALREF   i4      start;			/* start symbol */

 /* assigned token type values */
GLOBALREF   i4      extval;

 /* input and output file descriptors */

GLOBALREF   FILE *finput;		/* yacc input file */
GLOBALREF   FILE *faction;		/* file for saving actions */
GLOBALREF   FILE *fdefine;		/* file for # defines */
GLOBALREF   FILE *ftable;		/* ytab.c file */
GLOBALREF   FILE *ftemp;		/* tempfile to pass 2 */
/* where the strings for debugging are stored */
GLOBALREF   FILE *fdebug;
GLOBALREF   FILE *foutput;		/* y.output file */

 /* storage for grammar rules */

GLOBALREF   i4      mem0[MEMSIZE];		/* production storage */
GLOBALREF   i4     *mem;
GLOBALREF   i4      nprod;		/* number of productions */
GLOBALREF   i4	    ftname[NPROD];	/* function names for actions */
GLOBALREF   i4	    ftncnt;		/* number of functions generated */
GLOBALREF   i4	    casecnt;    /* number of case statements for actions */
/* pointers to descriptions of productions */
GLOBALREF   i4     *prdptr[NPROD];
/* precedence levels for the productions */
GLOBALREF   i4      levprd[NPROD];
/* set to 1 if the reduction has action code to do */
GLOBALREF   char    had_act[NPROD];
/* flag for generating the # line's default is yes */
GLOBALREF   i4      gen_lines;
/* flag for whether to include runtime debugging */
GLOBALREF   i4      gen_testing;
GLOBALREF   i4	    functions;
GLOBALREF   i4	    switchsize;
/* TRUE means make re-entrant parser */
GLOBALREF   i4	    reentrant;
/* Control block type for re-entrant parser */
GLOBALREF   char    argtype[50];
GLOBALREF   LOCATION	actloc;
GLOBALREF   char    actbuf[MAX_LOC];
GLOBALREF   LOCATION	temploc;
GLOBALREF   char    tempbuf[MAX_LOC];
GLOBALREF   LOCATION	debugloc;
GLOBALREF   char    dbgbuf[MAX_LOC];
GLOBALREF   char    prefix[50];	/* Prefix for table names */
/* Name of print function to use for debugging */
GLOBALREF   char    printfunc[50];
/* Name of structure in control block */
GLOBALREF   char    structname[50];

/*
**  This table is for controlling where the various yacc output tables will
**  be written.  For each table, there is a command line option that allows
**  the table to be written into a separate file.  For example,
**		yacc -yypgo abc.c grammar.y
**  would put the table yypgo[] into the file abc.c.
**
**  NOTE: THIS TABLE IS ORDER DEPENDENT.  DO NOT CHANGE THE ORDER OF THE ROWS
**	    HERE WITHOUT CHANGING THE CORRESPONDING CONSTANTS IN DEXTERN.H.
*/

GLOBALREF   struct outfiles Filespecs[];

/*
**	This table contains the names of the various parameters to the
**	parser.  These parameters exist outside of the session control
**	block used to control a re-entrant parser.
*/

#define		    MAXPARMS	10

GLOBALREF   PARMTYPE Params[MAXPARMS];
GLOBALREF   i4      Numparams;

setup (argc, argv)
i4   argc;
char   *argv[];
{
    i4		i,
		j,
		lev,
		t,
		ty;
    i4		c;
    i4		*p;
    char	actname[8];
    LOCATION	uloc,
		dloc,
		oloc,
		inloc;
    char	filename[MAX_LOC],
		oname[MAX_LOC];
    char	*err_buf;
    char	stbuf[STBUFSIZE];

    STcopy(OFILE, oname);
    foutput = NULL;
    fdefine = NULL;
    while (argc >= 2 && argv[1][0] == '-')
    {
	while (*++(argv[1]))
	{
	    switch (*argv[1])
	    {
		case 'v': 
		case 'V': 
		    if (*(argv[1] + 1) == '\0')
		    {
			STcopy(FILEU, filename);
		    }
		    else
		    {
			STcopy(argv[1] + 1, filename);
			/*
			** To terminate inner loop, set argv[1] pointing to last
			** char of string just processed.
			*/
			argv[1] += STlength(argv[1]) - 1;
		    }
		    if (LOfroms(PATH & FILENAME, filename, &uloc) != OK)
			ERROR(STprintf(stbuf,
			    "bad filename %s for output file", filename));
		    if (SIopen(&uloc, "w", &foutput) != OK)
			ERROR(STprintf(stbuf, "cannot open %s", filename));
		    continue;

		case 'D': 
		case 'd': 
		    if (*(argv[1] + 1) == '\0')
		    {
			STcopy(FILED, filename);
		    }
		    else
		    {
			STcopy(argv[1] + 1, filename);
			/*
			** To terminate inner loop, set argv[1] pointing to last
			** char of string just processed.
			*/
			argv[1] += STlength(argv[1]) - 1;
		    }
		    if (LOfroms(PATH & FILENAME, filename, &dloc) != OK)
			ERROR(STprintf(stbuf,
			    "bad filename %s for output file", filename));
		    if (SIopen(&dloc, "w", &fdefine) != OK)
			ERROR(STprintf(stbuf, "cannot open %s", filename));
		    continue;

		case 'l': 
		case 'L': 
		    gen_lines = 0;
		    continue;

		case 't': 
		case 'T': 
		    gen_testing = !gen_testing;
		    continue;

		case 'o': 
		case 'O': 
		    SIfprintf(stderr, "`o' flag now default in yacc\n");
		    continue;

		case 'r': 
		case 'R': 
		    reentrant = TRUE;
		    STcopy(argv[1] + 1, argtype);
		    /*
		    ** To terminate inner loop, set argv[1] pointing to last
		    ** char of string just processed.
		    */
		    argv[1] += STlength(argv[1]) - 1;
		    continue;

		case 'S':
		case 's':
		    functions = TRUE;
		    if (*(argv[1] + 1) != '\0')
		    {
			if (CVal(argv[1] + 1, &switchsize) != OK)
			    ERROR(STprintf(stbuf,
				"Illegal maximum switch size %s",argv[1] + 1));
		    }
		    /*
		    ** To terminate inner loop, set argv[1] pointing to last
		    ** char of string just processed.
		    */
		    argv[1] += STlength(argv[1]) - 1;
		    continue;

		case 'Y':
		case 'y':
		    if (argc < 3)
			ERROR(STprintf(stbuf, "Option %s needs a parameter",
			    argv[1] - 1));

		    /*
		    ** -y followed by a filename means to put all of the
		    ** tables in that file.  -yanything means to put anything[]
		    ** in the named file.
		    */
		    if (*(argv[1] + 1) == '\0')
		    {
			for (i = 0; i < NUMSPECS; i++)
			{
			    STcopy(argv[2], Filespecs[i].filename);
			    Filespecs[i].filegiven = TRUE;
			}
		    }
		    else
		    {
			for (i = 0; i < NUMSPECS; i++)
			{
			    if (!STbcompare(argv[1],0,Filespecs[i].optname,
				    0,TRUE))
				break;
			}
		        if (i == NUMSPECS)
			    ERROR(STprintf(stbuf,
				"Invalid option %s", argv[1] - 1));
			STcopy(argv[2], Filespecs[i].filename);
			Filespecs[i].filegiven = TRUE;
		    }
		    /*
		    ** To terminate inner loop, set argv[1] pointing to last
		    ** char of string just processed.
		    */
		    argv++;
		    argc--;
		    argv[1] += STlength(argv[1]) - 1;
		    continue;

		case 'c':
		case 'C':
		    STcopy(argv[1] + 1, oname);
		    /*
		    ** To terminate inner loop, set argv[1] pointing to last
		    ** char of string just processed.
		    */
		    argv[1] += STlength(argv[1]) - 1;
		    continue;

		case 'p':
		case 'P':
		    STcopy(argv[1] + 1, prefix);
		    /*
		    ** To terminate inner loop, set argv[1] pointing to last
		    ** char of string just processed.
		    */
		    argv[1] += STlength(argv[1]) - 1;
		    continue;

		case 'w':
		case 'W':
		    STcopy(argv[1] + 1, printfunc);
		    /*
		    ** To terminate inner loop, set argv[1] pointing to last
		    ** char of string just processed.
		    */
		    argv[1] += STlength(argv[1]) - 1;
		    continue;

		case 'n':
		case 'N':
		    STcopy(argv[1] + 1, structname);
		    /* note, the input for this flag will always be treated
		    ** as lower case. If this is a problem, remove this
		    ** line and get command line input from a file (not
		    ** commandline).
		    */
		    CVlower(structname);
		    /*
		    ** To terminate inner loop, set argv[1] pointing to last
		    ** char of string just processed.
		    */
		    argv[1] += STlength(argv[1]) - 1;
		    continue;

		case 'e':
		case 'E':
		    if (Numparams >= MAXPARMS - 1)
			ERROR("Too many -e options");
		    STcopy(argv[1] + 1, Params[Numparams].parmtype);
		    if (argc < 3)
			ERROR("The -e option requires a second parameter");
		    STcopy(argv[2], Params[Numparams].parmname);
		    /* note, the input for this flag will always be treated
		    ** as lower case. If this is a problem, remove this
		    ** line and get command line input from a file (not
		    ** commandline).
		    */
		    CVlower(Params[Numparams].parmname);
		    Numparams++;
		    /*
		    ** To terminate inner loop, set argv[1] pointing to last
		    ** char of string just processed.
		    */
		    argv++;
		    argc--;
		    argv[1] += STlength(argv[1]) - 1;
		    continue;

		default: 
		    ERROR(STprintf(stbuf, "illegal option: %c", *argv[1]));
	    }
	}
	argv++;
	argc--;
    }

    if (LOgt(dbgbuf, &debugloc) != OK)
	ERROR("cannot find current directory for debug file");
    if (LOuniq("dbg", "tmp", &debugloc) != OK)
	ERROR("cannot create name of debug file");
    if (SIopen(&debugloc, "w", &fdebug) != OK)
    {
	LOtos(&debugloc, &err_buf);
	ERROR(STprintf(stbuf, "cannot open debug file %s", err_buf));
    }
    if (LOfroms(PATH & FILENAME, oname, &oloc) != OK)
	ERROR(STprintf(stbuf, "bad filename %s for C file", oname));
    if (SIopen(&oloc, "w", &ftable) != OK)
	ERROR(STprintf(stbuf, "cannot open %s", oname));
    if (functions)
    {
	SIfprintf(ftable, "#define\tYYFUNCTIONS\n");
	SIfprintf(ftable, "#define\tYY_1HANDLER\t%s_1handle\n", prefix);
	SIfprintf(ftable, "#define\tYY_2HANDLER\t%s_2handle\n", prefix);
    }
    if (reentrant)
	SIfprintf(ftable, "#define\tYYREENTER\n");
    SIfprintf(ftable, "#define\tYYPRINTF\t%s\n", printfunc);

    for (i = 0; i < NUMSPECS; i++)
    {
	/* If no filename given for table, use output file */
	if (!Filespecs[i].filegiven)
	{
	    Filespecs[i].fileptr = ftable;
	}
	else
	{
	    /* Make a LOCATION */
	    if (LOfroms(PATH & FILENAME, Filespecs[i].filename,
		    &(Filespecs[i].fileloc)) != OK)
	    {
		ERROR(STprintf(stbuf, "bad filename %s for option %s",
			Filespecs[i].filename, Filespecs[i].optname));
	    }

	    /*
	    ** User gave a filename.  If two of the given filenames are the
	    ** same, only open the file once.  
	    **  BUG -- Compare the results of LOtos. (Mike S)
	    */
	    for (j = 0; j < i; j++)
	    {
		char	*file_i_buffer;
		char	*file_j_buffer;

		LOtos(&(Filespecs[i].fileloc), &file_i_buffer);
		LOtos(&(Filespecs[j].fileloc), &file_j_buffer);
  		if (STcompare(file_i_buffer, file_j_buffer)== 0)
		    break;
	    }

	    if (j != i)
	    {
		Filespecs[i].fileptr = Filespecs[j].fileptr;
	    }
	    else
	    {
		if (SIopen(&(Filespecs[i].fileloc), "w",
		    &(Filespecs[i].fileptr)) != OK)
		{
		    ERROR(STprintf(stbuf, "cannot open file %s for option %s",
			Filespecs[i].filename, Filespecs[i].optname));
		}
		SIfprintf(Filespecs[i].fileptr, "# include\t<compat.h>\n\n");
		SIfprintf(Filespecs[i].fileptr, "typedef\ti4\tyytabelem;\n\n");
	    }
	}
    }
    if (LOgt(tempbuf, &temploc) != OK)
	ERROR("cannot find current directory for temporary file");
    if (LOuniq("tmp", "tmp", &temploc) != OK)
	ERROR("cannot create name of temporary file");
    if (SIopen(&temploc, "w", &ftemp) != OK)
    {
	LOtos(&temploc, &err_buf);
	ERROR(STprintf(stbuf, "cannot open temp file %s", err_buf));
    }
    if (LOgt(actbuf, &actloc) != OK)
	ERROR("cannot find current directory for action file");
    if (LOuniq("act", "tmp", &actloc) != OK)
	ERROR("cannot create name of temporary action file");
    if (SIopen(&actloc, "w", &faction) != OK)
    {
	LOtos(&actloc, &err_buf);
	ERROR(STprintf(stbuf, "cannot open action file %s", err_buf));
    }

    if (argc < 2)
    {
	ERROR("no input file specified");
    }
    infile = argv[1];
    if (LOfroms(PATH & FILENAME, infile, &inloc) != OK)
	ERROR(STprintf(stbuf, "bad filename %s for input file", infile));
    if (SIopen(&inloc, "r", &finput) != OK)
	ERROR(STprintf(stbuf, "cannot open input file %s", infile));

    /* 
    ** If we have any backslash characters, then escape them so that the 
    ** filename is acceptable to the compiler.  This is for the benefit 
    ** of Windows of course, but we call this for all platforms to avoid 
    ** using a #ifdef here.
    */
    LOtoes(&inloc, "\\", infile2);
    infile = infile2;

    cnamp = cnames;
    defin(0, "$end");
    extval = 0400;
    defin(0, "error");
    defin(1, "$accept");
    mem = mem0;
    lev = 0;
    ty = 0;
    i = 0;
    beg_debug();		/* initialize fdebug file */

 /* sorry -- no yacc parser here..... we must bootstrap somehow... */

    for (t = gettok(); t != MARK && t != YENDFILE;)
    {
	switch (t)
	{

	    case ';': 
		t = gettok();
		break;

	    case START: 
		if ((t = gettok()) != IDENTIFIER)
		{
		    ERROR("bad %%start construction");
		}
		start = chfind(1, tokname);
		t = gettok();
		continue;

	    case TYPEDEF: 
		if ((t = gettok()) != TYPENAME)
		    ERROR("bad syntax in %%type");
		ty = numbval;
		for (;;)
		{
		    t = gettok();
		    switch (t)
		    {

			case IDENTIFIER: 
			    if ((t = chfind(1, tokname)) < NTBASE)
			    {
				j = TYPE(toklev[t]);
				if (j != 0 && j != ty)
				{
				    ERROR(STprintf(stbuf,
					    "type redeclaration of token %s",
					    tokset[t].name));
				}
				else
				    SETTYPE(toklev[t], ty);
			    }
			    else
			    {
				j = nontrst[t - NTBASE].tvalue;
				if (j != 0 && j != ty)
				{
				    ERROR(STprintf(stbuf,
					"type redeclaration of nonterminal %s",
					nontrst[t - NTBASE].name));
				}
				else
				    nontrst[t - NTBASE].tvalue = ty;
			    }
			case ',': 
			    continue;

			case ';': 
			    t = gettok();
			    break;

			default: 
			    break;
		    }
		    break;
		}
		continue;

	    case NOBYPASS: 
		for (;;)
		{
		    t = gettok();
		    switch (t)
		    {

			case IDENTIFIER: 
			    if ((t = chfind(1, tokname)) < NTBASE)
			    {
				ERROR(STprintf(stbuf,
					"NOBYPASS not allowed for token %s",
					tokset[t].name));
			    }
			    else
			    {
				nontrst[t - NTBASE].nobypass = TRUE;
			    }
			case ',': 
			    continue;

			case ';': 
			    t = gettok();
			    break;

			default: 
			    break;
		    }
		    break;
		}
		continue;

	    case UNION: 
	    /* copy the union declaration to the output */
		cpyunion();
		t = gettok();
		continue;

	    case LEFT: 
	    case BINARY: 
	    case RIGHT: 
		++i;
	    case TERM: 
		lev = t - TERM;	/* nonzero means new prec. and assoc. */
		ty = 0;

	    /* get identifiers so defined */

		t = gettok();
		if (t == TYPENAME)
		{		/* there is a type defined */
		    ty = numbval;
		    t = gettok();
		}

		for (;;)
		{
		    switch (t)
		    {

			case ',': 
			    t = gettok();
			    continue;

			case ';': 
			    break;

			case IDENTIFIER: 
			    j = chfind(0, tokname);
			    if (lev)
			    {
				if (ASSOC(toklev[j]))
		    ERROR(STprintf(stbuf, "redeclaration of precedence of %s",
			tokname));
				SETASC(toklev[j], lev);
				SETPLEV(toklev[j], i);
			    }
			    if (ty)
			    {
				if (TYPE(toklev[j]))
		    ERROR(STprintf(stbuf, "redeclaration of type of %s",
			tokname));
				SETTYPE(toklev[j], ty);
			    }
			    if ((t = gettok()) == NUMBER)
			    {
				tokset[j].value = numbval;
				if (j < ndefout && j > 2)
				{
			    ERROR(STprintf(stbuf,
				"please define type number of %s earlier",
				tokset[j].name));
				}
				t = gettok();
			    }
			    continue;

		    }

		    break;
		}

		continue;

	    case LCURLY: 
		defout();
		cpycode();
		t = gettok();
		continue;

	    default: 
		ERROR("syntax error");

	}

    }

    if (t == YENDFILE)
    {
	ERROR("unexpected EOF before %%");
    }

 /* t is MARK */

    defout();
    end_toks();		/* all tokens dumped - get ready for reductions
				   */

    SIfprintf(ftable, "#define yyclearin %schar = -1\n", prefix);
    SIfprintf(ftable, "#define yyerrok %serrflag = 0\n", prefix);
    if (!reentrant)
    {
	SIfprintf(ftable, "extern i4  %schar;\nextern i4  %serrflag;\n",
	    prefix, prefix);
    }
    SIfprintf(ftable, "#ifndef YYMAXDEPTH\n#define YYMAXDEPTH 150\n#endif\n");
    if (!ntypes)
	SIfprintf(ftable, "#ifndef YYSTDEF\n#define YYSTYPE i4\n#endif\n");
    if (!reentrant)
    {
	SIfprintf(ftable,
	    "GLOBALDEF YYSTYPE %slval ZERO_FILL, %sval ZERO_FILL;\n",
	    prefix, prefix);
    }
    SIfprintf(ftable, "typedef i4  yytabelem;\n");

    prdptr[0] = mem;
 /* added production */
    *mem++ = NTBASE;
    *mem++ = start;		/* if start is 0, we will overwrite with the
				   lhs of the first rule */
    *mem++ = 1;
    *mem++ = 0;
    prdptr[1] = mem;

    while ((t = gettok()) == LCURLY)
	cpycode();

    if (t != C_IDENTIFIER)
	ERROR("bad syntax on first rule");

    if (!start)
	prdptr[0][1] = chfind(1, tokname);

 /* read rules */

    while (t != MARK && t != YENDFILE)
    {

    /* process a rule */

	if (t == '|')
	{
	    rhsfill((char *) 0);/* restart fill of rhs */
	    *mem++ = *prdptr[nprod - 1];
	}
	else
	    if (t == C_IDENTIFIER)
	    {
		*mem = chfind(1, tokname);
		if (*mem < NTBASE)
		    ERROR("token illegal on LHS of grammar rule");
		++mem;
		lhsfill(tokname);/* new rule: restart strings */
	    }
	    else
		ERROR("illegal rule: missing semicolon or | ?");

    /* read rule body */


	t = gettok();
more_rule: 
	while (t == IDENTIFIER)
	{
	    *mem = chfind(1, tokname);
	    if (*mem < NTBASE)
		levprd[nprod] = toklev[*mem];
	    ++mem;
	    rhsfill(tokname);	/* add to rhs string */
	    t = gettok();
	}


	if (t == PREC)
	{
	    if (gettok() != IDENTIFIER)
		ERROR("illegal %%prec syntax");
	    j = chfind(2, tokname);
	    if (j >= NTBASE)
	ERROR(STprintf(stbuf, "nonterminal %s illegal after %%prec",
	    nontrst[j - NTBASE].name));
	    levprd[nprod] = toklev[j];
	    t = gettok();
	}

	if (t == '=')
	{
	    had_act[nprod] = 1;
	    levprd[nprod] |= ACTFLAG;
	    if (functions)	/* if actions go in functions */
	    {
		if ((++casecnt) % switchsize == 1)
		{
		    i4	    k;

		    ftncnt++;   /* new function name */
		    if (reentrant)
		    {
			SIfprintf(faction, "\ni4\n%s%diftn(yacc_cb, yyrule",
				  prefix, ftncnt); 
		    }
		    else
		    {
			SIfprintf(faction,
				  "\ni4\n%s%diftn(yypvt, pyyval, yyrule",
				  prefix, ftncnt); 
		    }
		    SIfprintf(faction, ", yystatusp, yyvarspp");
		    if (reentrant)
		    {
			SIfprintf(faction, ", cb");
		    }
		    for (k = 0; k < Numparams; k++)
		    {
			SIfprintf(faction, ", %s", Params[k].parmname);
		    }
		    if (reentrant)
		    {
			SIfprintf(faction, ")\nYACC_CB\t\t*yacc_cb;");
		    }
		    else
		    {
			SIfprintf(faction, ")\nYYSTYPE *yypvt, *pyyval;");
		    }
		    SIfprintf(faction, "\ni4\t\tyyrule;");
		    if (reentrant)
			SIfprintf(faction, "\n%s\t*cb;", argtype);
		    SIfprintf(faction, "\nDB_STATUS\t*yystatusp;"); 
		    SIfprintf(faction, "\nYYVARS\t\t**yyvarspp;"); 
		    for (k = 0; k < Numparams; k++)
		    {
			SIfprintf(faction, "\n%s\t%s;", Params[k].parmtype,
				Params[k].parmname);
		    }
		    SIfprintf(faction, "\n{\n    YYVARS\t*yyvarsp = *yyvarspp;\n\tswitch(yyrule)\n\t{");
		}
		ftname[nprod] = ftncnt;
	    }

	    SIfprintf(faction, "\ncase %d:", nprod);
	    cpyact(mem - prdptr[nprod] - 1);
	    SIfprintf(faction, " break;\n");

	    if (functions)	/* if actions go in functions */
	    {
		if ((casecnt % switchsize) == 0)
		{
		    /* close switch */
		    SIfprintf(faction, "\t}\n\treturn(0);\n}\n");
		}
	    }

	    if ((t = gettok()) == IDENTIFIER)
	    {
	    /* action within rule... */

		lrprnt();	/* dump lhs, rhs */
		STprintf(actname, "$$%d", nprod);
		j = chfind(1, actname);/* make it a nonterminal */

	    /* the current rule will become rule number nprod+1 */
	    /* move the contents down, and make room for the null */

		for (p = mem; p >= prdptr[nprod]; --p)
		    p[2] = *p;
		mem += 2;

	    /* enter null production for action */

		p = prdptr[nprod];

		*p++ = j;
		*p++ = -nprod;

	    /* update the production information */

		levprd[nprod + 1] = levprd[nprod] & ~ACTFLAG;
		levprd[nprod] = ACTFLAG;

		if (++nprod >= NPROD)
		    ERROR(STprintf(stbuf, "more than %d rules", NPROD));
		prdptr[nprod] = p;

	    /* make the action appear in the original rule */
		*mem++ = j;

	    /* get some more of the rule */

		goto more_rule;
	    }

	}

	while (t == ';')
	    t = gettok();

	*mem++ = -nprod;

    /* check that default action is reasonable */

	if (ntypes && !(levprd[nprod] & ACTFLAG) &&
	    nontrst[*prdptr[nprod] - NTBASE].tvalue)
	{
	/* no explicit action, LHS has value */
	    register i4     tempty;

	    tempty = prdptr[nprod][1];
	    if (tempty < 0)
		ERROR("must return a value, since LHS has a type");
	    else
		if (tempty >= NTBASE)
		    tempty = nontrst[tempty - NTBASE].tvalue;
		else
		    tempty = TYPE(toklev[tempty]);
	    if (tempty != nontrst[*prdptr[nprod] - NTBASE].tvalue)
	    {
		ERROR("default action causes potential type clash");
	    }
	}

	if (++nprod >= NPROD)
	    ERROR(STprintf(stbuf, "more than %d rules", NPROD));
	prdptr[nprod] = mem;
	levprd[nprod] = 0;

    }

 /* end of all rules */

    end_debug();		/* finish fdebug file's input */
    finact();
    if (t == MARK)
    {
	SIputc('\n', ftable);
	/*
	**  gen_line, if false, used to completely suppress the printing of
	**  #line directives.  Now, if it is false, it will enclose them in
	**  comments.
	*/
	if (!gen_lines)
	{
	    SIfprintf(ftable, "/* ");
	}
	SIfprintf(ftable, "# line %d \"%s\"", lineno, infile);
	if (!gen_lines)
	{
	    SIfprintf(ftable, " */");
	}
	SIputc('\n', ftable);

	while ((c = SIgetc(finput)) != EOF)
	    SIputc(c, ftable);
    }
    SIclose(finput);
	finput = (FILE *)NULL;
}

finact()
{
 /* finish action routine */

    /* Close off last switch and function if actions being put in functions */
    if (functions && ((casecnt % switchsize) != 0))
    {
	SIfprintf(faction, "}\n\treturn(0);\n}\n");
    }
    SIclose(faction);
	faction = (FILE *)NULL;

    SIfprintf(ftable, "# define YYERRCODE %d\n", tokset[2].value);

}

/*	define s to be a terminal if t=0 or a nonterminal if t=1		*/
defin(t, s)
i4		t;
register char	*s;
{
    register i4     val;
    char	    stbuf[STBUFSIZE];

    if (t)
    {
	if (++nnonter >= NNONTERM)
	    ERROR (STprintf(stbuf, "too many nonterminals, limit %d",
		NNONTERM));
	nontrst[nnonter].name = cstash (s);
	return (NTBASE + nnonter);
    }
 /* must be a token */
    if (++ntokens >= NTERMS)
	ERROR (STprintf(stbuf, "too many terminals, limit %d", NTERMS));
    tokset[ntokens].name = cstash (s);

 /* establish value for token */

    if (s[0] == ' ' && s[2] == '\0')/* single character literal */
	val = s[1];
    else
	if (s[0] == ' ' && s[1] == '\\')
	{			/* escape sequence */
	    if (s[3] == '\0')
	    {			/* single character escape sequence */
		switch (s[2])
		{
		    /* character which is escaped */
		    case 'n': 
			val = '\n';
			break;
		    case 'r': 
			val = '\r';
			break;
		    case 'b': 
			val = '\b';
			break;
		    case 't': 
			val = '\t';
			break;
		    case 'f': 
			val = '\f';
			break;
		    case '\'': 
			val = '\'';
			break;
		    case '"': 
			val = '"';
			break;
		    case '\\': 
			val = '\\';
			break;
		    default: 
			ERROR("invalid escape");
		}
	    }
	    else
	    {
		if (s[2] <= '7' && s[2] >= '0')
		{
		    /* \nnn sequence */
		    if (s[3] < '0' || s[3] > '7' || s[4] < '0' ||
			    s[4] > '7' || s[5] != '\0')
			ERROR("illegal \\nnn construction");
		    val = 64 * s[2] + 8 * s[3] + s[4] - 73 * '0';
		    if (val == 0)
			ERROR("'\\000' is illegal");
		}
	    }
	}
	else
	{
	    val = extval++;
	}
    tokset[ntokens].value = val;
    toklev[ntokens] = 0;
    return (ntokens);
}

/* write out the defines (at the end of the declaration section) */
defout()
{
    register i4     i;
    i4              c;
    register char  *cp;

    for (i = ndefout; i <= ntokens; ++i)
    {

	cp = tokset[i].name;
	if (*cp == ' ')		/* literals */
	{
	    SIfprintf(fdebug, "\t\"%s\",\t%d,\n",
		    tokset[i].name + 1, tokset[i].value);
	    cp++;		/* in my opinion, this should be continue */
	}

	for (; (c = *cp) != '\0'; ++cp)
	{
	    if (!(CMlower (&c) || CMupper (&c) || CMdigit (&c) || c == '_'))
		goto nodef;
	}

	if (tokset[i].name[0] != ' ')
	    SIfprintf(fdebug, "\t\"%s\",\t%d,\n",
		tokset[i].name, tokset[i].value);
	if (fdefine == NULL)
	{
	    SIfprintf(ftable,
		"# define %s %d\n", tokset[i].name, tokset[i].value);
	}
	else
	{
	    SIfprintf(fdefine, "# define %s %d\n", tokset[i].name,
		tokset[i].value);
	}

nodef: 	;
    }

    ndefout = ntokens + 1;

}

char   *
cstash(s)
register char   *s;
{
    char   *temp;

    temp = cnamp;
    do
    {
	if (cnamp >= &cnames[cnamsz])
	    ERROR("too many characters in id's and literals");
	else
	    *cnamp++ = *s;
    } while (*s++);
    return (temp);
}

gettok()
{
    register i4     i,
		    base;
    static int	    peekline;	/* number of '\n' seen in lookahead */
    register i4     match,
		    reserve;
    int		    c;
    char	    stbuf[STBUFSIZE];

begin: 
    reserve = 0;
    lineno += peekline;
    peekline = 0;
    c = SIgetc(finput);
    while (c == ' ' || c == '\n' || c == '\t' || c == '\f')
    {
	if (c == '\n')
	    ++lineno;
	c = SIgetc(finput);
    }
    if (c == '/')
    {				/* skip comment */
	lineno += skipcom();
	goto begin;
    }

    switch (c)
    {

	case EOF: 
	    return (YENDFILE);
	case '{': 
	    SIungetc(c, finput);
	    return ('=');	/* action ... */
	case '<': 		/* get, and look up, a type name (union member
				   name) */
	    i = 0;
	    while ((c = SIgetc(finput)) != '>' && c >= 0 && c != '\n')
	    {
		tokname[i] = c;
		if (++i >= NAMESIZE)
		    --i;
	    }
	    if (c != '>')
		ERROR("unterminated < ... > clause");
	    tokname[i] = '\0';
	    for (i = 1; i <= ntypes; ++i)
	    {
		if (!STcompare(typeset[i], tokname))
		{
		    numbval = i;
		    return (TYPENAME);
		}
	    }
	    typeset[numbval = ++ntypes] = cstash(tokname);
	    return (TYPENAME);

	case '"': 
	case '\'': 
	    match = c;
	    tokname[0] = ' ';
	    i = 1;
	    for (;;)
	    {
		c = SIgetc(finput);
		if (c == '\n' || c == EOF)
		    ERROR("illegal or missing ' or \"");
		if (c == '\\')
		{
		    c = SIgetc(finput);
		    tokname[i] = '\\';
		    if (++i >= NAMESIZE)
			--i;
		}
		else
		    if (c == match)
			break;
		tokname[i] = c;
		if (++i >= NAMESIZE)
		    --i;
	    }
	    break;

	case '%': 
	case '\\': 

	    switch (c = SIgetc(finput))
	    {

		case '0': 
		    return (TERM);
		case '<': 
		    return (LEFT);
		case '2': 
		    return (BINARY);
		case '>': 
		    return (RIGHT);
		case '%': 
		case '\\': 
		    return (MARK);
		case '=': 
		    return (PREC);
		case '{': 
		    return (LCURLY);
		default: 
		    reserve = 1;
	    }

	default: 

	    if (CMdigit(&c))
	    {			/* number */
		numbval = c - '0';
		base = (c == '0') ? 8 : 10;
		for (c = SIgetc(finput); CMdigit(&c); c = SIgetc(finput))
		{
		    numbval = numbval * base + c - '0';
		}
		SIungetc(c, finput);
		return (NUMBER);
	    }
	    else
	    {
		if (CMlower(&c) || CMupper(&c) || c == '_' || c == '.'
		    || c == '$')
		{
		    i = 0;
		    while (CMlower(&c) || CMupper(&c) || CMdigit(&c)
			|| c == '_' || c == '.' || c == '$')
		    {
			tokname[i] = c;
			if (reserve && CMupper(&c))
			    tokname[i] += 'a' - 'A';
			if (++i >= NAMESIZE)
			    --i;
			c = SIgetc(finput);
		    }
		}
		else
		{
		    return (c);
		}
	    }

	    SIungetc(c, finput);
    }

    tokname[i] = '\0';

    if (reserve)
    {				/* find a reserved word */
	if (!STcompare(tokname, "term"))
	    return (TERM);
	if (!STcompare(tokname, "token"))
	    return (TERM);
	if (!STcompare(tokname, "left"))
	    return (LEFT);
	if (!STcompare(tokname, "nonassoc"))
	    return (BINARY);
	if (!STcompare(tokname, "binary"))
	    return (BINARY);
	if (!STcompare(tokname, "right"))
	    return (RIGHT);
	if (!STcompare(tokname, "prec"))
	    return (PREC);
	if (!STcompare(tokname, "start"))
	    return (START);
	if (!STcompare(tokname, "type"))
	    return (TYPEDEF);
	if (!STcompare(tokname, "union"))
	    return (UNION);
	if (!STcompare(tokname, "nobypass"))
	    return (NOBYPASS);
	ERROR(STprintf(stbuf, "invalid escape, or illegal reserved word: %s",
	    tokname));
    }

 /* look ahead to distinguish IDENTIFIER from C_IDENTIFIER */

    c = SIgetc(finput);
    while (c == ' ' || c == '\t' || c == '\n' || c == '\f' || c == '/')
    {
	if (c == '\n')
	    ++peekline;
	else
	    if (c == '/')
	    {			/* look for comments */
		peekline += skipcom ();
	    }
	c = SIgetc(finput);
    }
    if (c == ':')
	return (C_IDENTIFIER);
    SIungetc(c, finput);
    return (IDENTIFIER);
}

/* determine the type of a symbol */
fdtype(t)
i4	t;
{
    register i4     v;
    char    stbuf[STBUFSIZE];

    if (t >= NTBASE)
	v = nontrst[t - NTBASE].tvalue;
    else
	v = TYPE(toklev[t]);
    if (v <= 0)
	ERROR(STprintf(stbuf, "must specify type for %s",
	    (t >= NTBASE) ? nontrst[t - NTBASE].name : tokset[t].name));
    return (v);
}

/*@RH@*/
/* determine if nobypass was specified for a symbol */
bool
fdnobypass(t)
i4	t;
{
    char stbuf[STBUFSIZE];

    if (t < NTBASE)
	ERROR(STprintf(stbuf, "NOBYPASS checked for token %s", tokset[t].name));

    return(nontrst[t - NTBASE].nobypass);
}

chfind(t, s)
i4		t;
register char	*s;
{
    i4      i;
    char    stbuf[STBUFSIZE];

    if (s[0] == ' ')
	t = 0;
    TLOOP(i)
    {
	if (!STcompare(s, tokset[i].name))
	{
	    return (i);
	}
    }
    NTLOOP(i)
    {
	if (!STcompare(s, nontrst[i].name))
	{
	    return (i + NTBASE);
	}
    }
 /* cannot find name */
    if (t > 1)
	ERROR(STprintf(stbuf, "%s should have been defined earlier", s));
    return (defin(t, s));
}

cpyunion()
{
 /* copy the union declaration to the output, and the define file if present */

    i4      level,
            c;

    /*
    ** gen_lines option used to completely suppress #line directives.  Now it
    ** just puts them in comments.
    */
    SIputc('\n', ftable);
    if (!gen_lines)
    {
	SIfprintf(ftable, "/* ");
    }
    SIfprintf(ftable, "# line %d \"%s\"", lineno, infile);
    if (!gen_lines)
    {
	SIfprintf(ftable, " */");
    }
    SIputc('\n', ftable);

    if (fdefine)
	SIfprintf(fdefine, "\n#define YYSTDEF\ntypedef union ");
    else
    	SIfprintf(ftable, "\n#define YYSTDEF\ntypedef union ");

    level = 0;
    for (;;)
    {
	if ((c = SIgetc(finput)) < 0)
	    ERROR("EOF encountered while processing %%union");
	if (fdefine)
	    SIputc(c, fdefine);
	else
	    SIputc(c, ftable);

	switch (c)
	{

	    case '\n': 
		++lineno;
		break;

	    case '{': 
		++level;
		break;

	    case '}': 
		--level;
		if (level == 0)
		{		/* we are finished copying */
		    if (fdefine)
			SIfprintf(fdefine, " YYSTYPE;\n");
		    else
			SIfprintf(ftable, " YYSTYPE;\n");
		    if (!reentrant && fdefine)
			SIfprintf(fdefine, "extern YYSTYPE %slval;\n", prefix);
		    return;
		}
	}
    }
}

cpycode()
{				/* copies code between \{ and \} */

    i4      c;

    c = SIgetc(finput);
    if (c == '\n')
    {
	c = SIgetc(finput);
	lineno++;
    }
    /*
    ** gen_lines option used to completely suppress #line directives.  Now it
    ** just puts them in comments.
    */
    SIputc('\n', ftable);
    if (!gen_lines)
    {
	SIfprintf(ftable, "/* ");
    }
    SIfprintf(ftable, "# line %d \"%s\"", lineno, infile);
    if (!gen_lines)
    {
	SIfprintf(ftable, " */");
    }
    SIputc('\n', ftable);

    while (c >= 0)
    {
	if (c == '\\')
	    if ((c = SIgetc(finput)) == '}')
		return;
	    else
		SIputc('\\', ftable);
	if (c == '%')
	    if ((c = SIgetc(finput)) == '}')
		return;
	    else
		SIputc('%', ftable);
	SIputc(c, ftable);
	if (c == '\n')
	    ++lineno;
	c = SIgetc(finput);
    }
    ERROR("eof before %%}");
}

skipcom()
{				/* skip over comments */
    register i4     c,
                    i = 0;		/* i is the number of lines skipped */

 /* skipcom is called after reading a / */

    if (SIgetc(finput) != '*')
	ERROR("illegal comment");
    c = SIgetc(finput);
    while (c != EOF)
    {
	while (c == '*')
	{
	    if ((c = SIgetc(finput)) == '/')
		return (i);
	}
	if (c == '\n')
	    ++i;
	c = SIgetc(finput);
    }
    ERROR("EOF inside comment");
 /* NOTREACHED */
}

cpyact(offset)
{				/* copy C action to the next ; or closing } */
    i4      brac,
            c,
            match,
            j,
            s,
            tok;
    char    stbuf[STBUFSIZE];

    SIputc('\n', faction);
    /*@RH@*/
    if (fdnobypass(*prdptr[nprod]))
	SIfprintf(faction, "if (TRUE)\t/* NOBYPASS */\n");
    else
    {
    	if (functions)
    	{
	    SIfprintf(faction, "if (!((*yyvarspp)->bypass_actions))\n");
    	}
    	else
    	{
	    SIfprintf(faction, "if (!(yyvars.bypass_actions))\n");
    	}
    }

    /*
    ** gen_lines option used to completely suppress #line directives.  Now it
    ** just puts them in comments.
    */
    if (!gen_lines)
    {
	SIfprintf(faction, "/* ");
    }
    SIfprintf(faction, "# line %d \"%s\"", lineno, infile);
    if (!gen_lines)
    {
	SIfprintf(faction, " */");
    }
    SIputc('\n', faction);

    brac = 0;

loop: 
    c = SIgetc(finput);
swt: 
    switch (c)
    {

	case ';': 
	    if (brac == 0)
	    {
		SIputc(c, faction);
		return;
	    }
	    goto lcopy;

	case '{': 
	    brac++;
	    goto lcopy;

	case '$': 
	    s = 1;
	    tok = -1;
	    c = SIgetc(finput);
	    switch (c)
	    {
		case '<':
		    /* type description */
		    SIungetc(c, finput);
		    if (gettok() != TYPENAME)
			ERROR("bad syntax on $<ident> clause");
		    tok = numbval;
		    c = SIgetc(finput);
		    break;
		case '$':
		    /*
		    **  If actions are being put in functions, then the functions
		    **  take a pointer to yyval which must be dereferenced.  If
		    **  not, we can just use yyval.
		    */
		    if (functions)
		    {
			if (reentrant)
			    SIfprintf(faction, "yacc_cb->yyval");
			else
			    SIfprintf(faction, "*pyyval");
		    }
		    else
		    {
			if (reentrant)
			    SIfprintf(faction, "yacc_cb->yyval");
			else
			    SIfprintf(faction, "%sval", prefix);
		    }
		    if (ntypes)
		    {		/* put out the proper tag... */
			if (tok < 0)
			    tok = fdtype(*prdptr[nprod]);
			SIfprintf(faction, ".%s", typeset[tok]);
		    }
		    goto loop;
		case 'Y':		    
		    if (functions)
		    {
			SIfprintf(faction, "yyvarsp->");
		    }
		    else
		    {
			SIfprintf(faction, "yyvars.");
		    }
		    goto loop;
		case '-':
		    s = -s;
		    c = SIgetc(finput);
		    break;
		default:
		    if (CMdigit(&c))
		    {
			j = 0;
			while (CMdigit(&c))
			{
			    j = j * 10 + c - '0';
			    c = SIgetc(finput);
			}

			j = j * s - offset;
			if (j > 0)
			{
			    ERROR(STprintf(stbuf, "Illegal use of $%d",
				j + offset));
			}

			if (reentrant)
			{
			    SIfprintf(faction, "yacc_cb->");
			}
			SIfprintf(faction, "yypvt[-%d]", -j);
			if (ntypes)
			{		/* put out the proper tag */
			    if (j + offset <= 0 && tok < 0)
				ERROR(STprintf(stbuf,
				    "must specify type of $%d", j + offset));
			    if (tok < 0)
				tok = fdtype(prdptr[nprod][j + offset]);
			    SIfprintf(faction, ".%s", typeset[tok]);
			}
			goto swt;
		    }
	    }
	    SIputc('$', faction);
	    if (s < 0)
		SIputc('-', faction);
	    goto swt;

	case '}': 
	    if (--brac)
		goto lcopy;
	    SIputc(c, faction);
	    return;


	case '/': 		/* look for comments */
	    SIputc(c, faction);
	    c = SIgetc(finput);
	    if (c != '*')
		goto swt;

	/* it really is a comment */

	    SIputc(c, faction);
	    c = SIgetc(finput);
	    while (c != EOF)
	    {
		while (c == '*')
		{
		    SIputc(c, faction);
		    if ((c = SIgetc(finput)) == '/')
			goto lcopy;
		}
		SIputc(c, faction);
		if (c == '\n')
		    ++lineno;
		c = SIgetc(finput);
	    }
	    ERROR ("EOF inside comment");

	case '\'': 		/* character constant */
	    match = '\'';
	    goto string;

	case '"': 		/* character string */
	    match = '"';

    string: 

	    SIputc(c, faction);
	    while (c = SIgetc(finput))
	    {

		if (c == '\\')
		{
		    SIputc(c, faction);
		    c = SIgetc(finput);
		    if (c == '\n')
			++lineno;
		}
		else
		    if (c == match)
			goto lcopy;
		    else
			if (c == '\n')
			    ERROR("newline in string or char. const.");
		SIputc(c, faction);
	    }
	    ERROR("EOF in string or character constant");

	case EOF: 
	    ERROR("action does not terminate");

	case '\n': 
	    ++lineno;
	    goto lcopy;

    }

lcopy: 
    SIputc(c, faction);
    goto loop;

}


#define RHS_TEXT_LEN		( BUFSIZ * 4 )/* length of rhstext */

/* store current lhs (non-terminal) name */
GLOBALREF char    lhstext[BUFSIZ];
GLOBALREF char    rhstext[RHS_TEXT_LEN]; /* store current rhs list */

/* new rule, dump old (if exists), restart strings */
lhsfill(s)
char   *s;
{
    rhsfill((char *) 0);
    STcopy(s, lhstext);	/* don't worry about too long of a name */
}


rhsfill(s)
char   *s;			/* either name or 0 */
{
    static char *loc = rhstext;	/* next free location in rhstext */
    register char  *p;

    if (!s)			/* print out and erase old text */
    {
	if (*lhstext)		/* there was an old rule - dump it */
	    lrprnt();
	(loc = rhstext)[0] = '\0';
	return;
    }
 /* add to stuff in rhstext */
    p = s;
    *loc++ = ' ';
    if (*s == ' ')		/* special quoted symbol */
    {
	*loc++ = '\'';		/* add first quote */
	p++;
    }
    while (*loc = *p++)
	if (loc++ > &rhstext[RHS_TEXT_LEN] - 2)
	    break;
    if (*s == ' ')
	*loc++ = '\'';
    *loc = '\0';		/* terminate the string */
}


lrprnt()			/* print out the left and right hand sides */
{
    char   *rhs;

    if (!*rhstext)		/* empty rhs - print usual comment */
	rhs = " /* empty */";
    else
	rhs = rhstext;
    SIfprintf(fdebug, "	\"%s :%s\",\n", lhstext, rhs);
}


beg_debug()			/* dump initial sequence for fdebug file */
{
    SIfprintf(fdebug,
	    "typedef struct { char *t_name; int t_val; } yytoktype;\n");
    SIfprintf(fdebug,
	    "#ifndef YYDEBUG\n#\tdefine YYDEBUG\t%d", gen_testing);
    SIfprintf(fdebug, "\t/*%sallow debugging */\n#endif\n\n",
	    gen_testing ? " " : " don't ");
    SIfprintf(fdebug,
	    "#if YYDEBUG\n\nGLOBALDEF const\tyytoktype %stoks[] =\n{\n",
	    prefix);
}


end_toks()			/* finish yytoks array, get ready for yyred's
				   strings */
{
    SIfprintf(fdebug, "\t\"-unknown-\",\t-1\t/* ends search */\n");
    SIfprintf(fdebug, "};\n\nGLOBALDEF const\tchar * %sreds[] =\n{\n",
	prefix);
    SIfprintf(fdebug, "\t\"-no such reduction-\",\n");
}


end_debug()			/* finish yyred array, close file */
{
    lrprnt();			/* dump last lhs, rhs */
    SIfprintf(fdebug, "};\n#endif /* YYDEBUG */\n");
    SIclose(fdebug);
	fdebug = (FILE *)NULL;
}

/* inform caller of function name, if any, for this action */
funcid(ndx)
i4	ndx;
{
    return (ftname[ndx]);
}
