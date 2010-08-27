/*
**Copyright (c) 2004 Ingres Corporation
*/
/*	@(#)dextern.h	1.5	*/
/* This file has the location of the parser, and the size of the program
** desired.  It may also contain definitions to override various defaults.
**
** History:
**	05-oct-93 (swm)
**	    Bug #58886
**	    Increased MEMSIZE to avoid running out of state space on
**	    axp_osf.
**	14-oct-93 (swm)
**	    Bug #56448
**	    Altered ERROR calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to ERROR.
**	    This works because STprintf is a varargs function in the CL.
**	    Defined STBUFSIZE for local STprintf buffers required for
**	    this change.
**	23-sep-1996 (canor01)
**	    Add definition of NUMSPECS (moved from y2.c).
**	17-aug-98 (inkdo01)
**	    Increased sizes of LSETSIZE, NSTATES due to grammar additions.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	02-jul-99 (schte01)
**	    Changed i4  to long for LP64 to avoid unaligned access. This is a
**       cross-integration of  the ingres30 change 441406, modified for LP64 
**       and i4 ->nat. 
**	23-sep-99 (inkdo01)
**	    Change CNAMSZ from 12000 to 16000 to avoid "too many characters"
**	    error.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-oct-2001 (somsa01)
**	    Increased LSETSIZE slightly more so that it would be successful
**	    on WIN64.
**	31-dec-02 (inkdo01)
**	    Bring defines into line with 3.0 for "sequence" propagation.
**	07-dec-2004 (abbjo03)
**	    Update location of VMS parser driver file.
**	20-may-2008 (dougi)
**	    Increase NSTATES, TEMPSIZE for inexorable increase in size of grammar.
**	13-aug-2008 (dougi)
**	    Increase NPROD from 2000 to 2500 (oink, oink).
**	08-jul-2009 (frima01) SIR 122138
**	    Remove i4 to long macro as in front dextern.h
**	18-Mar-2010 (kiria01) b123438
**	    Upped the ACTSIZE to 16000
**	2-Aug-2010 (kschendel) b124170
**	    byacc has to generate "reentrant" and "functions", remove flags.
*/

/**
** Name: dextern.h - Parser file definitions for backend YACC.
**
** Description:
**	@(#)dextern.h	1.5
** This file has the location of the parser, and the size of the program
** desired.  It may also contain definitions to override various defaults.
**
** History:
**	24-jan-1994 (gmanning)
**	    Add comment block.  Add WIN32 #ifdef to get definitions for HUGE
**	    and PARSER in NT environment.
**/

/*
** on some systems, notably IBM, the names for the output files and tempfiles
** must also be changed
*/

/* location of the default parser text file */
#ifdef    VMS
#define	    PARSER  "ING_TOOLS:[lib]byacc.par"
#endif

#ifdef    UNIX
#define			HUGE
#define			PARSER	    "byacc.par"
#endif

#ifdef    WIN32
#define			HUGE
#define			PARSER	    "byacc.par"
#endif

#ifdef    OS2
#define			HUGE
#define			PARSER	    "byacc.par"
#endif

#ifdef	  CMS
#define			PARSER	    "byacc.par"
#endif

#ifdef	  MVS
#define			PARSER     "'betools.vj.h(byacc)'"
#endif

/*  MANIFEST CONSTANT DEFINITIONS */

/* base of nonterminal internal numbers */
#define			NTBASE		010000

/* internal codes for error and accept actions */

#define			ERRCODE		8190
#define			ACCEPTCODE	8191

/* sizes and limits */

/*
**  NOTE: Either HUGE or MEDIUM must be defined to the compiler.
**	HUGE is for systems with a lot of memory and 32-bit words.  MEDIUM is
**	for systems without much memory and/or 16-bit words.
*/
#ifdef    VMS
#define	  HUGE
#endif

#ifndef   HUGE
#define	  MEDIUM
#endif

#ifdef    HUGE
#define			ACTSIZE		16000
#define			MEMSIZE		48000
#define			NSTATES		3500
#define			NTERMS		600
#define			NPROD		3000
#define			NNONTERM	1000
#define			TEMPSIZE	3500
#define			CNAMSZ		16000
#define			LSETSIZE	2500
#define			WSETSIZE	1000
#endif

#ifdef    MEDIUM
#define			ACTSIZE		4000
#define			MEMSIZE		5200
#define			NSTATES		720
#define			NTERMS		127
#define			NPROD		300
#define			NNONTERM	200
#define			TEMPSIZE	1000
#define			CNAMSZ		4000
#define			LSETSIZE	1000
#define			WSETSIZE	250
#endif

#define			NAMESIZE	50
#define			NTYPES		63
#define			DEFCASESIZE	30

#ifdef    HUGE
#define			TBITSET		((32 + NTERMS) / 32)

/* bit packing macros (may be machine dependent) */
#define			BIT(a,i)	((a)[(i) >> 5] & (1 << ((i) & 037)))
#define			SETBIT(a,i)	((a)[(i) >> 5] |= (1<<((i)&037)))

/* number of words needed to hold n+1 bits */
#define			NWORDS(n)	(((n) + 32) / 32)

#else

#define			TBITSET		((16 + NTERMS) / 16)

/* bit packing macros (may be machine dependent) */
#define			BIT(a,i)	((a)[(i) >> 4] & (1 << ((i) & 017)))
#define			SETBIT(a,i)	((a)[(i) >> 4] |= (1 << ((i) & 017)))

/* number of words needed to hold n+1 bits */
#define			NWORDS(n)	(((n) + 16) / 16)
#endif

/*
** relationships which must hold:
**	TBITSET nats must hold NTERMS+1 bits...
**	WSETSIZE >= NNONTERM
**	LSETSIZE >= NNONTERM
**	TEMPSIZE >= NTERMS + NNONTERMs + 1
**	TEMPSIZE >= NSTATES
*/

/* associativities */

#define                 NOASC           0   /* no assoc. */
#define			LASC		1   /* left assoc. */
#define			RASC		2   /* right assoc. */
#define			BASC		3   /* binary assoc. */

/* flags for state generation */

#define			DONE		0
#define			MUSTDO		1
#define			MUSTLOOKAHEAD	2

/* flags for a rule having an action, and being reduced */

#define			ACTFLAG		04
#define			REDFLAG		010

/* output parser flags */
#define			YYFLAG1		(-1000)

/* macros for getting associativity and precedence levels */

#define			ASSOC(i)	((i) & 03)
#define			PLEVEL(i)	(((i) >> 4) & 077)
#define			TYPE(i)		((i >> 10) & 077)

/* macros for setting associativity and precedence levels */

#define			SETASC(i,j)	i |= j
#define			SETPLEV(i,j)	i |= (j << 4)
#define			SETTYPE(i,j)	i |= (j << 10)

/* looping macros */

#define			TLOOP(i)	for(i = 1; i <= ntokens; ++i)
#define			NTLOOP(i)	for(i = 0; i <= nnonter; ++i)
#define			PLOOP(s,i)	for(i = s; i < nprod; ++i)
#define			SLOOP(i)	for(i = 0; i < nstate; ++i)
#define			WSBUMP(x)	++x
#define			WSLOOP(s,j)	for(j = s; j < cwp; ++j)
#define			ITMLOOP(i,p,q)	q=pstate[i+1];for(p=pstate[i];p<q;++p)
#define			SETLOOP(i)	for(i = 0; i < tbitset; ++i)

/* I/O descriptors */

GLOBALREF FILE *finput;		/* input file */
GLOBALREF FILE *faction;		/* file for saving actions */
GLOBALREF FILE *fdefine;		/* file for # defines */
GLOBALREF FILE *ftable;		/* ytab.c file */
GLOBALREF FILE *ftemp;		/* tempfile to pass 2 */
GLOBALREF FILE *fdebug;		/* tempfile for two debugging info arrays */
GLOBALREF FILE *foutput;		/* yacc.out file */

/* other I/O stuff */

GLOBALREF	LOCATION    actloc;
GLOBALREF	LOCATION    temploc;
GLOBALREF	LOCATION    debugloc;

#define	STBUFSIZE	132

/* structure declarations */

struct looksets
{
	i4		    lset[TBITSET];
};

struct item
{
	i4		    *pitem;
	struct looksets	    *look;
};

struct toksymb
{
	char		    *name;
	i4		    value;
};

struct ntsymb
{
	char		    *name;
	i4		    tvalue;
	bool		    nobypass;
};

struct wset
{
	i4		    *pitem;
	i4		    flag;
	struct looksets	    ws;
};

/* token information */

GLOBALREF i4  ntokens ;	/* number of tokens */
GLOBALREF struct toksymb tokset[];
GLOBALREF i4  toklev[];	/* vector with the precedence of the terminals */

/* nonterminal information */

GLOBALREF i4  nnonter ;	/* the number of nonterminals */
GLOBALREF struct ntsymb nontrst[];

/* grammar rule information */

GLOBALREF i4  nprod ;	/* number of productions */
GLOBALREF i4  *prdptr[];	/* pointers to descriptions of productions */
GLOBALREF i4  levprd[] ;	/* contains production levels to break conflicts */
GLOBALREF char had_act[];	/* set if reduction has associated action code */

/* state information */

GLOBALREF i4  nstate ;		/* number of states */
GLOBALREF struct item *pstate[];	/* pointers to the descriptions of the states */
GLOBALREF i4  tystate[];	/* contains type information about the states */
GLOBALREF i4  defact[];	/* the default action of the state */
GLOBALREF i4  tstates[];	/* the states deriving each token */
GLOBALREF i4  ntstates[];	/* the states deriving each nonterminal */
/* the continuation of the chains begun in tstates and ntstates */
GLOBALREF i4  mstates[];

/* lookahead set information */

GLOBALREF struct looksets lkst[];
GLOBALREF i4  nolook;  /* flag to turn off lookahead computations */

/* working set information */

GLOBALREF struct wset wsets[];
GLOBALREF struct wset *cwp;

/* storage for productions */

GLOBALREF i4  mem0[];
GLOBALREF i4  *mem;

/* storage for action table */

GLOBALREF i4  amem[];  /* action table storage */
GLOBALREF i4  *memp ;		/* next free action table position */
GLOBALREF i4  indgo[];		/* index to the stored goto table */

/* temporary vector, indexable by states, terms, or ntokens */

GLOBALREF i4  temp1[];
GLOBALREF i4  lineno; /* current line number */

/* statistics collection variables */

GLOBALREF i4  zzgoent ;
GLOBALREF i4  zzgobest ;
GLOBALREF i4  zzacent ;
GLOBALREF i4  zzexcp ;
GLOBALREF i4  zzclose ;
GLOBALREF i4  zzrrconf ;
GLOBALREF i4  zzsrconf ;

/* code production information */

/* maximum number of cases per switch, to be used if above is TRUE */
GLOBALREF i4   switchsize;
/* type of the control block passed to the re-entrant parser */
GLOBALREF char argtype[];
/* prefix for table names */
GLOBALREF char prefix[];
/* name of structure in control block */
GLOBALREF char structname[];
/* struct for storing info about output files for tables */
struct outfiles
{
    char	optname[20];	    /* command-line option to set filename */
    char	filename[MAX_LOC];  /* name of file if given on command line */
    bool	filegiven;	    /* tells whether filename was given */
    LOCATION	fileloc;	    /* LOCATION of output file, if given */
    FILE	*fileptr;	    /* Where to write the table */
};
GLOBALREF struct outfiles	Filespecs[];

/*
**  The following define the positions of the various table info]
**  in Filespecs[].  For example, Filespecs[YYPACT] contains the info
**  about where the yypact[] table will be written.
*/

#define                 YYEXCA          0
#define			YYACT		1
#define			YYPACT		2
#define			YYPGO		3
#define			YYR1		4
#define			YYR2		5
#define			YYCHK		6
#define			YYDEF		7
#define			YYFUNC		8

#define			NUMSPECS	9   /* total of above */

/* Info about parameters to parser function */
typedef struct
{
    char    parmtype[20];	/* Parameter type */
    char    parmname[20];       /* Parameter name */
} PARMTYPE;

GLOBALREF  PARMTYPE	Params[];	/* Parameter types and names */
GLOBALREF	i4 		Numparams;	/* Number of parameters */

/* define functions with strange types... */

FUNC_EXTERN	char *cstash();
FUNC_EXTERN	struct looksets *flset();
FUNC_EXTERN	char *symnam();
FUNC_EXTERN	char *writem();

/* default settings for a number of macros */

/* name of yacc tempfiles */

/*
**  NOTE: TEMPFILES NO LONGER HAVE FIXED NAMES.  THE NAMES ARE GENERATED
**	ON THE FLY AND ARE GUARANTEED TO BE UNIQUE.  THIS SHOULD MAKE IT
**	POSSIBLE FOR MORE THAN ONE YACC TO RUN IN A DIRECTORY AT ONE TIME.
*/

/* default output file name */

#ifndef    OFILE
#ifdef MVS
#define	    OFILE   "dd:yacctabc"
#else
#define			OFILE		"ytab.c"
#endif
#endif

/* default user output file name */

#ifndef			FILEU
#ifdef MVS
#define			FILEU           "dd:yaccout"
#else
#define			FILEU		"yacc.out"
#endif
#endif

/* default output file for # defines */

#ifndef	   FILED
#ifdef MVS
#define			FILED           "dd:yacctabh"
#else
#define			FILED		"ytab.h"
#endif
#endif

/* command to clobber tempfiles after use */

#ifndef    ZAPFILE
#define			ZAPFILE(x)	LOdelete(x)
#endif

/*
** Following define defines the name of the error handling proc.
** This change was introduced on 11/17/87 to resolve `error multiply defined'
** conflict.
**  17-nov-87 (stec)
**	Added.
*/
#define ERROR yacc_error
