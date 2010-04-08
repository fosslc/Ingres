/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<pc.h>
#include	<cm.h>
#include	<si.h>
#include	<st.h>
#include	<lo.h>
#include	<nm.h>
#include	<me.h>
#include	<er.h>
#include	<ut.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>

/**
** Name:    feuta.c -	Front-End Command Parsing Module.
**
** Description:
**	This module provides an interface whereby command lines can be parsed
**	in a uniform manner.  The rules for the parse of the command line can
**	be found in the UTexe definition file for each command so defined. This
**	module contains:
**
** Sample Use:
**
**	See header of 'feutapar.c' for example of how to use
**	FEutaopen/get/close in conjunction with IIUGuapParse().
**	
**	FEutaopen()	parse command line using UTexe definition file
**	FEutaget()	return parameter for internal name.
**	FEutapget()	return a parameter value by position.
**	FEutaerr()	report command syntax or semantic error..
**	FEutaclose()	free storage for command line parser.
**	FEutapfx()	Return external prefix for internal id.
**
** History:
**	21-may-86 (bab)	- Use MAXSTRING for buffer sizes, both
**			  to fix bug 9285 (incompatible buffer
**			  sizes caused stack tromp) and for
**			  compatability.  MAXSTRING defined here
**			  since not defined in any globally used
**			  header file.
**	12-nov-1986 (yamamoto)  Modified for STindex.
**
**	Revision 4.0  86/05/07  12:25:00  bobm
**	fix for empty arguments
**	(86/03/11)  wong
**	  Extracted error messages into error file ("error.24").
**	(86/02/11)
**	  peter  Add SI.h for correct operation on VMS.
**	(86/02/11)  bobm
**	  add new format to skip output only lines.
**	(86/02/11)  bobm
**	  treat double quotes as delimeters
**	(86/01/21)  wong
**	  Fixed bug having to do with repetition count in format string.
**	(85/12/23)  wong
**	  Added 'FEutaerr()' to the interface.
**	(85/12/18)  wong
**	  Now prints error message with correct usage and returns when
**	  user enters invalid command syntax instead of causing a syserr.
**	(85/12/10)  wong
**	  Added new format "%-".
**
**	08/85 (rlm) -- Written.
**	29-aug-90 (pete)
**		Added "Sample Use" note above.
**	10-oct-91 (seg)
**		Changed "errno" to "errnum" to avoid naming conflict with
**		c runtime library
**	29-jul-1992 (rdrane)
**		Removed declaration of FEsalloc(), since it has been
**		placed in fe.h.
**	23-feb-93 (mohan)
**		added return type VOID to alloc_err, open_err, syn_err,
**		rarg_copy, size_err.  Added tag to CMDOPT structure.
**	16-mar-93 (fraser)
**		Changed structure tag name--it shouldn't begin with
**		underscore.
**	21-mar-96 (chech02)
**		Added function protypes for win 3.1 port.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	31-oct-2001 (gupsh01)
**	    Bumped up the value of UTE_BUFSIZE as Copydb
**	    parameters exceed length.
**      31-Jul-2003 (hanal04) SIR 110651 INGDBA 236
**          Bumped up the value of UTE_BUFSIZE as Copydb
**          parameters exceed length.
**      31-Oct-2006 (kiria01) b116944
**          Bumped up the value of UTE_BUFSIZE again as Copydb
**          parameters exceed length.
**       3-Dec-2008 (hanal04) SIR 116958
**          Bumped UTE_BUFSIZE to accomodate new -no_rep option to copydb.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

# define	UTE_MAX		50
# define	UTE_BUFSIZE	1500

# define	INFINITY	MAXI2
# define	NO_FORMAT	'_'
# define	SKIP_FORMAT	'?'
# define	ALL_FORMAT	'A'
# define	ALLZ_FORMAT	'a'
# define	ALT_AFORMAT	'-'
# define	MAXSTRING	255

/*
** Fatal Bugchecks.
*/
#define ExeOverflow	E_UG001B
#define BadExeName	E_UG001C
#define NoExeDef	E_UG001D
#define	BadExeOpen	E_UG001E
#define BadExeSyntax	E_UG001F

/* #define ARGDEBUG */

/*
**	if there is no format on an argument (argfmt = NO_FORMAT),
**	it becomes all prefix (slen = 0).  There is a parse table
**	entry for SKIP_FORMAT lines to avoid a lot of special case
**	construction.  This entry won't be matched.
*/
typedef struct s_CMDOPT
{
	char *intname;		/* internal name of option */
	char *extpfx;		/* external format prefix */
	char *extsfx;		/* external format suffix */
	char *extname;		/* external name for prompting */
	i2 intid;		/* internal numeric id of option */
	i2 slen;		/* suffix length */
	i2 plen;		/* prefix length */
	i2 repcount;		/* repeat count allowed */
	char argfmt;		/* argument format */
} CMDOPT;

/*
**	NOTE: Oarg, Rarg may actually wind up shorter than
**	Argv if we have arguments separated from their prefixes
*/
static i4  Argc = 0;		/* number of arguments */
static char *Command = ERx("");	/* command */
static char *Cmdname = ERx("");	/* command name */
static char **Argv = NULL;	/* (non-command) arg vector */
static CMDOPT **Oarg = NULL;	/* option pointer vector parallel to argv */
static ARGRET *Rarg = NULL;	/* return structure vector parallel to argv */
static i2 Optag ZERO_FILL;	/* tag for storage allocation */
static CMDOPT *Ovec = NULL;	/* array of legal options */
static i4  Vnum = 0;		/* items in Ovec */
static i4  Pcount = 1000;	/* prompt counter */
static char *Reserved = NULL;	/* leading characters reserved for options */
static char *Syntax = NULL;	/* possible syntax */

/*
** Oempty is not used when scanning arguments.  It is a CMDOPT
** block for empty arguments to point to.  Only the internal name,
** and int. id items will matter.
*/
static CMDOPT Oempty =
{
	ERx(""),
	ERx(""),
	ERx(""),
	ERx(""),
	-1,
	0,
	0,
	0,
	EOS
};

#ifdef WIN16
static STATUS ov_set(char *, nat);
static STATUS argprompt(CMDOPT *, ARGRET *, i4  *, nat);
static STATUS cvt_arg(char *, char, ARGRET *, bool, ...);
static VOID size_err(i4);
static i4  tabwords(char *, i4, char **, bool, bool);
#endif /* WIN16 */

static i4  parse2();
static FILE *ov_open();
static STATUS ov_set();
static STATUS argprompt();
static STATUS cvt_arg();
static VOID alloc_err();
static VOID open_err();
static VOID syn_err();
static VOID rarg_copy();
static VOID size_err();
static i4  tabwords();

FUNC_EXTERN bool GetOpenRoadStyle();

/*{
** Name:	FEutaerr() -	Report command line syntax or semantic error.
**						(Non-fatal.)
** Description:
**	This routine prints the specified formatted error message and then
**	prints the correct syntax for the command line before returning.
**
** Parameters:
**	errnum -- (input only)  The error number.
**	argno -- (input only)  The number of format values.
**	a1, a2, ... -- (input only)  Values to be formatted into error message.
**
** History:
**	01/12/85 (jhw) -- Written.
**	20/07/88 (ncg) -- Uses stdout for errors.
**	10-oct-91 (seg)
**		Changed "errno" to "errnum" to avoid naming conflict with
**		c runtime library
**	1-dec-1997 (donc)
**	    Direct these errors to the OpenROAD trace window.
*/

VOID
FEutaerr(errnum, argno, a1, a2, a3, a4, a5)
ER_MSGID	errnum;
i4		argno;
PTR		a1,
		a2,
		a3,
		a4,
		a5;
{
    bool isOpenROADStyle;
    isOpenROADStyle = GetOpenRoadStyle();
    if (isOpenROADStyle)
    {
        IIUGfer(NULL, errnum, 0, argno, a1, a2, a3, a4, a5);
        IIUGfer(NULL, BADSYNTAX, 0, 2, (PTR)Cmdname, (PTR)Syntax);
    }
    else
    {
        IIUGfer(stdout, errnum, 0, argno, a1, a2, a3, a4, a5);
        IIUGfer(stdout, BADSYNTAX, 0, 2, (PTR)Cmdname, (PTR)Syntax);
    }
}

/*
**	FEutaopen - parse command line according to UTexe definition file
**		storing information for later FEutaget, FEutapget calls
**
**		syserr's bad UTEXE syntax, or illegal arguments.
**		illegal arguments are those matching no formats or
**		exceeding repetition counts.
**
**	Input Parameters:
**		argc,argv - command line
**		cname - command name in UTexe definition file
**
**	Returns
**		OK, FAIL
*/

STATUS
FEutaopen (argc, argv, cname)
i4  argc;
char **argv;
char *cname;
{
	i4 i, j;

	Command = *argv;
	Cmdname = cname;
	Argv = argv+1;
	argc -= 1;

	/* use j to hold allocation request - prevent 0 allocations */
	j = argc > 0 ? argc : 1;

	Optag = FEbegintag();

	if (((Oarg = (CMDOPT **)FEreqmem((u_i4)0,
		(u_i4)(j * sizeof(CMDOPT *)),
		FALSE, (STATUS *)NULL)) == NULL) ||
	    ((Rarg = (ARGRET *)FEreqmem((u_i4)0,
		(u_i4)(j * sizeof(ARGRET)),
		FALSE, (STATUS *)NULL)) == NULL) ||
	    ((Ovec = (CMDOPT *)FEreqmem((u_i4)0,
		(u_i4)(UTE_MAX * sizeof(CMDOPT)),
		FALSE, (STATUS *)NULL)) == NULL))
	{
		alloc_err();	/* aborts */
	}

	if (ov_set(cname, UTE_MAX) != OK)
		return (FAIL);

	FEendtag();

	/* increment as we parse, because some things will be two args */
	Argc = 0;

	/* parse arguments, using Ovec structure */
	for (i = 0; i < argc; i += j)
	{
		j = parse2(Argv[i], (i < argc - 1 ? Argv[i+1] : NULL));
		if (j <= 0)
			return (FAIL);
		++Argc;
	}
	return OK;
}

/*
**	check argument, and possibly one past it, for match.
**	return number of arguments used, -1 for failure.
**
**	History:
**		08/85 (rlm) -- Written.
**		12/85 (jhw) -- Modified to return input argument for "%-"
**				format.
**		12/85 (jhw) -- Modified to report user errors and return
**				instead of using 'syserr()'.
*/

static i4
parse2 (arg, arg2)
register char	*arg,
		*arg2;
{
    i4	j;
    char		buf[MAXSTRING + 1];
    i4			ridx;

#ifdef ARGDEBUG
    SIfprintf(stderr, ERx("\narg: %s %s"), arg, arg2);
#endif

    /*
    ** special case empty arguments by pointing them
    ** to Oempty, and setting Rarg to "no data".
    */
    while (*arg == ' ')
	++arg;
    if (*arg == EOS)
    {
#ifdef ARGDEBUG
	SIfprintf(stderr, ERx(" - EMPTY\n"));
#endif
	Rarg[Argc].type = 0;
	Oarg[Argc] = &Oempty;
	return 1;
    }

    /*
    **	search for an option whose repcount hasn't been
    **	exhausted yet, with prefix matching argument
    **	we break as soon as we find one.
    */
    for (j = 0 ; j < Vnum ; ++j)
    {
	register CMDOPT	*ovp = &Ovec[j];
	i4	len;

#ifdef ARGDEBUG
	SIfprintf(stderr, ERx("\n\tcompare: %s %d %s(%d) %c %s(%d) rep"),
				ovp->intname, ovp->repcount,
				ovp->extpfx, ovp->plen, ovp->argfmt,
				ovp->extsfx, ovp->slen
	);
#endif
	if (ovp->argfmt == SKIP_FORMAT)
	    continue;

	ridx = 1;
	STcopy(arg, buf);

	if (ovp->slen == 0 && ovp->plen == 0)
	{	/* No suffix or prefix */
#ifdef ARGDEBUG
	    SIfprintf(stderr, ERx(" Ns"));
#endif
	    if (ovp->repcount > 0 && STindex(Reserved, buf, 0) == NULL &&
		    cvt_arg(buf, ovp->argfmt, &Rarg[Argc], FALSE) == OK)
		break;

	    continue;
	}
#ifdef ARGDEBUG
	SIfprintf(stderr, ERx(" S/P"));
#endif
	len = STlength(buf);
	if (len < ovp->plen)
	    continue;
#ifdef ARGDEBUG
	SIfprintf(stderr, ERx(" pfx"));
#endif
	if (ovp->plen > 0)
	{ /* Compare for prefix */
	    buf[ovp->plen] = EOS;
	    if (!STequal(ovp->extpfx, buf))
		continue;

	    /*
	    ** Now, figure out what to compare for suffix
	    ** If the argument is entirely prefix, we have to decide wether we
	    ** look ahead to the next argument for a string:
	    **
	    **	1) the NO_FORMAT case we don't - this was simply intended as
	    **	    a flag argument in the first place.
	    **
	    **  2) If the format is lower case, we don't because we are allowing
	    **	    empty strings and thus requiring attached arguments.
	    **
	    ** ALL_FORMAT and ALLZ_FORMAT imply prefix returned as part of the
	    ** converted string.  If ALL_FORMAT and we used 2 arguments, we
	    ** rejoin into one string.
	    */
	    if ( len > ovp->plen && ovp->argfmt == NO_FORMAT )
	    {
		FEutaerr(BADARG, 1, arg);
		return -1;
	    }
	    else if (len == ovp->plen &&
			ovp->argfmt != NO_FORMAT && !CMlower(&(ovp->argfmt)))
	    {
		ridx += 1;
		if (arg2 == NULL)
		{
		    FEutaerr(NOARGVAL, 1, arg);
		    return -1;
		}
		if (ovp->argfmt == ALL_FORMAT)
		    _VOID_ STcat(buf, arg2);
		else
		    STcopy(arg2, buf);
		len = STlength(buf);
		if (STindex(Reserved, buf, 0) != (char *)NULL)
		{
		    FEutaerr(NOARGVAL, 1, arg);
		    return -1;
		}
	    }
	    else
	    {
		if (ovp->argfmt == ALL_FORMAT || ovp->argfmt == ALLZ_FORMAT)
		    STcopy(arg, buf);
		else
		    STcopy(&arg[ovp->plen], buf);
		len -= ovp->plen;
	    }
	}
#ifdef ARGDEBUG
	SIfprintf(stderr, ERx(" sfx"));
#endif
	if (ovp->slen > 0)
	{	/* Compare for suffix */
	    register char	*ptr;

	    if (len < ovp->slen)
		continue;

	    ptr = &buf[len - ovp->slen];
	    if (!STequal(ovp->extsfx, ptr))
		continue;
	    *ptr = EOS;
	    len -= ovp->slen;
	}

#ifdef ARGDEBUG
	SIfprintf(stderr, ERx(" fmt"));
#endif
	/* buf contains variable portion of argument */
	if (ovp->argfmt != NO_FORMAT && ovp->repcount > 0)
	{
	    if (cvt_arg(buf, ovp->argfmt, &Rarg[Argc], FALSE) != OK)
		continue;
	}
	break;
    }	/* end for */

    /* should have gotten it, now */
    if (j >= Vnum || Ovec[j].repcount <= 0)
    {
	STATUS	err;

	err = j >= Vnum && STindex(Reserved, buf, 0) != (char *)NULL
				? BADARG : TOOMANYARGS;
	FEutaerr(err, 1, (j >= Vnum ? arg : Ovec[j].extpfx));
	return -1;
    }

#ifdef ARGDEBUG
    SIfprintf(stderr, ERx(" ACCEPTED = "));
    rarg_dump(Rarg+Argc);
#endif

    --(Ovec[j].repcount);
    Oarg[Argc] = &Ovec[j];
    return ridx;
}

/*
**	FEutaname - return command name recorded at FEutaopen
**		may be called after FEutaclose.
*/

char *
FEutaname ()
{
	return Command;
}

/*
**	FEutaget - return a command line parameter by its internal
**		name.  Argument strings returned in the "ret" argument
**		will still be valid after FEutaclose.
**
**		Requests to prompt for an option which does not have a
**		format string results in a "yes" or "no" expected on
**		the prompt, and TRUE or FALSE returned in ret.
**
**	Input Parameters:
**		iname - internal name
**		rep - desired repetition of parameter
**		fmode - action to take on failure: prompt, return error, abort
**
**	Output Parameters:
**		ret - ARGRET structure containing returned argument
**		rpos - position of parameter
**
**		returns:
**			OK, FAIL if parameter not present and fmode is
**			FARG_FAIL.
*/

STATUS
FEutaget (iname, rep, fmode, ret, rpos)
char *iname;
i4  rep;
i4  fmode;
ARGRET *ret;
i4  *rpos;
{
    i4	lidx,
	idx,
	count;

#ifdef ARGDEBUG
    SIfprintf(stderr, ERx("Request %s ",iname)); 
#endif

    lidx = count = -1;
    for (idx = 0; idx < Argc; ++idx)
    {
	if (STequal((Oarg[idx])->intname, iname))
	{
	    lidx = idx;
	    if (rep < 0 || (++count) < rep)
		continue;
	    rarg_copy(Rarg+idx, ret);
#ifdef ARGDEBUG
	    SIfprintf(stderr, ERx("Return %d =", idx));
	    rarg_dump(ret);
#endif

	    *rpos = idx + 1;
	    return OK;
	}
    }
    if (rep < 0 && lidx >= 0)
    {
	rarg_copy(Rarg+lidx, ret);
#ifdef ARGDEBUG
	SIfprintf(stderr, ERx("Return %d =", lidx));
	rarg_dump(ret);
#endif

	*rpos = lidx + 1;
	return OK;
    }

#ifdef ARGDEBUG
    SIfprintf(stderr, ERx("Not found\n"));
#endif

    for (idx = 0; idx < Vnum; ++idx)
    {
	if (STequal(Ovec[idx].intname, iname))
	    break;
    }

    if (idx >= Vnum)
	IIUGfer(stderr, BadExeName, UG_ERR_FATAL, 1, iname);

    switch (fmode)
    {
      case FARG_FAIL:
	return (FAIL);
      case FARG_ABORT:
	FEutaerr(NOARGUMENT, 1, Ovec[idx].extname);
	PCexit(FAIL);
      default:
	return argprompt(&Ovec[idx], ret, rpos, fmode);
    }
}

/*
**	FEutapget - return an argument by position
**
**	Input parameters:
**		num - desired position
**
**	Output parameters:
**		name - internal name of argument
**		id - internal id number corresponding to name
**		ret - returned argument data
*/

STATUS
FEutapget(num,name,id,ret)
i4  num;
char **name;
i4  *id;
ARGRET *ret;
{
	if ((--num) >= Argc || num < 0)
		return (FAIL);

	MEcopy((char *)Rarg+num, (u_i2)sizeof(ARGRET), (char *)ret);
	*name = (Oarg[num])->intname;
	*id = (Oarg[num])->intid;
	return OK;
}

/*
**	FEutaclose - free storage for command parsing structure reserved
**		by FEutaopen.  No further calls may be made to FEutaget or
**		FEutapget, although strings returned through ARGRET
**		structures by them will remain valid.
*/

VOID
FEutaclose()
{
	FEfree(Optag);
	Vnum = Argc = 0;
	Pcount = 1000;
	Reserved = NULL;
}

/*
**	FEutapfx - return an external parameter prefix string.
**
**		Given an internal id this routine will match it to an external
**		parameter string.  This allows callers to generate errors
**		with external system dependent arguments known by FEuta.
**
**	Input Parameters:
**		id - internal integer id
**
**	Output Parameters:
**
**		returns:
**			pointer to name external prefix name.  if the id
**			was not legal then it return an empty string.
**	History:
**		25-jul-1988 - Written (ncg)
*/

char *
FEutapfx(id)
i4	id;
{
    i4		idx;

    for (idx = 0; idx < Vnum; ++idx)
    {
	if (Ovec[idx].intid == id)
	    return Ovec[idx].extpfx;
    }
    return ERx("");
}

/*
**	open UTexe definition file, advance pointer to beginning of command
**	return NULL for failure
**
**	History:
**		08/85 (rlm) -- Written.
**		12/85 (jhw) -- Modified to get acceptable command syntax
**				from UTexe definition file
**		4/91 (Mike S) Use UTdeffile for unbundling
**		29-jul-1992 (rdrane)
**			Removed declaration of FEsalloc(), since it has been
**			placed in fe.h.
**	17-Jun-2004 (schka24)
**	    Safer env var handling.
*/

static FILE *
ov_open (cmd)
char *cmd;
{
	FILE *fptr;
	char *alt;
	i4 count;
	LOCATION loc;
	char buf[UTE_BUFSIZE];
	char *wd[5];
	CL_ERR_DESC clerror;


	NMgtAt(ERx("II_UTEXE_DEF"), &alt);

	if (alt == (char *)NULL || *alt == EOS)
	{
		/* If UTdeffile fails, it couldn't open utexe.def */
		if (UTdeffile(cmd, buf, &loc, &clerror) != OK)
			open_err(ERx("utexe.def"));
	}
	else
	{
		STlcopy(alt, buf, sizeof(buf)-1);
		LOfroms(PATH&FILENAME, buf, &loc);
	}

	if (SIopen (&loc, ERx("r"), &fptr) != OK)
	{
		char *filename;

		LOtos(&loc, &filename);
		open_err(filename);	/* aborts */
	}

	while (SIgetrec(buf, sizeof buf, fptr) == OK)
	{
		/* quick throwaway if 1st char shows not a candidate. */
		if (CMwhite(buf) || *cmd != *buf)
			continue;
		count = tabwords(buf, 5, wd, FALSE, TRUE);
		if (count > 0 && STequal(wd[0], cmd))
		{
			if (count >= 4 && Reserved == NULL)
			{
				if ((Reserved = FEsalloc(wd[3])) == (char *)NULL)
					alloc_err();	/* aborts */
#ifdef ARGDEBUG
				SIfprintf(stderr, ERx("Reserved chars: %s\n"),Reserved);
#endif
			}
			if (count >= 5 && Syntax == (char *)NULL)
			{ /* save copy of syntax */
				if ((Syntax = STalloc(wd[4])) == (char *)NULL)
					alloc_err();	/* aborts */
			}
			return fptr;
		}
	}

	IIUGfer(stdout, NoExeDef, UG_ERR_FATAL, 1, cmd);
	return (NULL);
}

/*
**	set up the argument structure.  We assume sufficient storage has
**	been defined.
**
**	History:
**		08/85 (rlm) -- Written.
**		12/85 (jhw) -- Modified to accept less strict argument
**				specification syntax and to use reasonable
**				defaults in that case.
*/

static STATUS
ov_set (cmd, count)
char *cmd;
i4  count;
{
	register FILE	*fptr;
	char		buf[UTE_BUFSIZE];

	if ((fptr = ov_open(cmd)) == NULL)
		return (FAIL);

	for (Vnum = 0; SIgetrec(buf, sizeof buf, fptr) == OK && CMwhite(buf); ++Vnum)
	{
		register CMDOPT	*ovp = &Ovec[Vnum];
		register char	*ptr;
		i4		num;
		char		*tok[5];

		if (Vnum >= count)
		{
			size_err(count);
			/* NOTREACHED */
		}

		if ((num = tabwords(buf, 5, tok, TRUE, FALSE)) < 2)
			syn_err();	/* aborts */

#ifdef ARGDEBUG
		SIfprintf(stderr, ERx("parse: \"%s\" \"%s\" \"%s\" \"%s\" \"%s\"\n"),
				tok[0], tok[1], tok[2], tok[3], tok[4]);
#endif

		/* strings already in tagged storage (by tabwords) */
		ovp->intname = tok[0];
		ovp->extname = tok[num >= 4 ? 3 : 0];
		ovp->extpfx = tok[num >= 5 ? 4 : 1];
		if (ovp->intname == NULL ||
				ovp->extname == NULL || ovp->extpfx == NULL)
			alloc_err();	/* aborts */

		if (num < 3)
			num = 0;
		else if (CVan(tok[2], &num) != OK)
			syn_err();	/* aborts */
		ovp->intid = num;

		/* external format is in prefix - parse into correct parts */
		for (ptr = ovp->extpfx; *ptr != EOS && *ptr != '%'; ++ptr)
			;
		if (*ptr == '%')
		{
			register char	*nstr;
			ARGRET		trash;

			/* terminate prefix string by overwriting % */
			*ptr = EOS;
			++ptr;

			/* mark leading numeric from format */
			for (nstr = ptr; CMdigit(ptr); ++ptr)
				;

			/* set suffix to point to stuff following format */
			if (*ptr == EOS)
				syn_err();	/* aborts */
			ovp->extsfx = ptr + 1;

			/* copy and check format char, terminate num string */
			ovp->argfmt = *ptr;

			/* use of '-' for 'A' for backwards compatibility */
			if (ovp->argfmt == ALT_AFORMAT)
				ovp->argfmt = ALL_FORMAT;

			_VOID_ cvt_arg((char *)NULL, ovp->argfmt, &trash, FALSE);
			*ptr = EOS;

			/* convert number for repetition count */
			if (*nstr == EOS)
				num = 1;
			else if (CVan(nstr, &num) != OK)
				syn_err();
			ovp->repcount = num != 0 ? num : INFINITY;
		}
		else
		{
			/* no format string, entire thing remains in prefix */
			ovp->extsfx = ptr;
			ovp->argfmt = NO_FORMAT;
			ovp->repcount = INFINITY;
		}

		ovp->plen = STlength(ovp->extpfx);
		ovp->slen = STlength(ovp->extsfx);

		if (ovp->argfmt == NO_FORMAT && ovp->plen == 0)
			syn_err();	/* aborts */

#ifdef ARGDEBUG
		SIfprintf(stderr, ERx("	%d: int %s(%d), ext %s, rep %d\n"), Vnum,
				ovp->intname, ovp->intid,
				ovp->extname, ovp->repcount);
		SIfprintf(stderr, ERx("	format %s(%d) %c %s(%d)\n"), ovp->extpfx,
				ovp->plen,ovp->argfmt,
				ovp->extsfx,ovp->slen);
#endif
	}	/* end for */

	SIclose(fptr);
	return OK;
}

/*
** prompt user for argument.  If the user enters an empty string or something
** illegal three times, FAIL.  If mode was FARG_OPROMPT, an empty buffer is
** accepted and converted according to the argument format.
**
**	opt - CMDOPT structure for argument being prompted for.
**	ret - return structure to fill in for caller.
**	rpos - returned prompt counter to use for argument position.
**	mode - caller's FARG_XXX mode
**
** History:
**
**	25-aug-95 (tutto01)
**		Give the user one chance.  Three chances meant three dialog
**		boxes would have to be closed if the user decides he does not
**		want to run the app (on Windows NT).
*/
static STATUS
argprompt (opt, ret, rpos, mode)
CMDOPT *opt;
ARGRET *ret;
i4  *rpos;
i4  mode;
{
    i4		attempts;
    char	buf[FE_PROMPTSIZE + 1];

    for (attempts = 1; TRUE; ++attempts)
    {
	FEprompt(opt->extname, FALSE, FE_PROMPTSIZE, buf);

	if (buf[0] == EOS)
	{
	    if (mode == FARG_OPROMPT)
	    {
		/* empty strings assumed to convert properly */
		_VOID_ cvt_arg(buf, opt->argfmt, ret, FALSE, opt->extname);
		break;
	    }
	}
	else
	{
	    if (cvt_arg(buf, opt->argfmt, ret, TRUE, opt->extname) == OK)
		break;
	}

# ifdef NT_GENERIC
	/* Just one chance given on NT */
	attempts = 3;
# endif /* NT_GENERIC */

	if (attempts >= 3)
	{
	    FEutaerr(NOARGPROMPT, 1,
		(opt->plen != 0 ? opt->extpfx : opt->extname)
	    );
	    return FAIL;
	}
    } /* end for */

    *rpos = Pcount;
    ++Pcount;
    return OK;
}

/*
**	cvt_arg with a null string means to simply check the format
**	character, and abort if bad.  SKIP_FORMAT returns legal for
**	this reason.  Should an application force a prompt for this
**	format, any string the user types will be accepted.
**
**	NOTE: null string means NULL, not an empty buffer.  Empty buffers
**	actually get converted, and if the caller specified FARG_OPROMPT,
**	or an optional-argument format was used this may actually happen.
**
**	History:
**		08/85 (rlm) -- Written.
**		12/85 (jhw) -- Modified for new '%-' format.
**		02/86 (rlm) -- Modified for SKIP_FORMAT
*/

/*VARARGS4*/
static STATUS
cvt_arg (str, fmt, ret, msg, name)
char	*str;
char	fmt;
ARGRET	*ret;
bool	msg;
char	*name;
{
    STATUS	status = OK;

    switch (fmt)
    {
      case 'N':
      case 'n':
	ret->type = 2;
	if (str == (char *)NULL || CVan(str, &(ret->dat.num)) == OK)
	    return OK;
	status = ARGINTREQ;
	break;
      case 'F':
      case 'f':
	ret->type = 3;
	if (str == NULL || CVaf(str, '.', &(ret->dat.val)) == OK)
	    return OK;
	status = ARGFLTREQ;
	break;
      case 'S':
      case 's':
      case ALL_FORMAT:
      case ALLZ_FORMAT:
	ret->type = 1;
	if (str == NULL)
	    return OK;
	/* make a permanent copy of the string */
	if ((ret->dat.name = STalloc(str)) == NULL)
	{
	    alloc_err();
	}
	return OK;
      case NO_FORMAT:
	ret->type = 4;
	if (str == NULL)
	    return OK;
	if (IIUGyn(str, &status))
	{
	    ret->dat.flag = TRUE;
	    return OK;
	}
	else if (status == E_UG0005_No_Response)
	{
	    ret->dat.flag = FALSE;
	    return OK;
	}
	break;
      case SKIP_FORMAT:
	ret->type = 1;
	return OK;
      default:
	syn_err();	/* aborts */
	/*NOTREACHED*/
    } /* end switch */
    if (msg && status != OK)
	IIUGfer(stdout, status, 0, 1, (PTR)name);
    return FAIL;
}

/*
** Error reporting routines.
**
**	Including:
**		alloc_err()	Reports storage allocation failure.  Fatal.
**		open_err()	Reports open error on UTexe definition file.  Fatal.
**		syn_err()	Reports syntax error in UTexe definition file.  Fatal.
**
**	All these routines abort.
*/

static
VOID
alloc_err()
{
    IIUGbmaBadMemoryAllocation(ERx("FEuta"));
}

static
VOID
open_err (name)
char	*name;
{
    IIUGfer(stdout, BadExeOpen, UG_ERR_FATAL, 1, (PTR)name);
}

static
VOID
syn_err ()
{
    IIUGfer(stdout, BadExeSyntax, UG_ERR_FATAL, 1, (PTR)&Vnum);
}

static
VOID
size_err (size_max)
i4  size_max;
{
    IIUGfer(stdout, ExeOverflow, UG_ERR_FATAL, 1, (PTR)&size_max);
}

static
VOID
rarg_copy (source, dest)
ARGRET *source, *dest;
{
    switch (dest->type = source->type)
    {
      case 1:
	dest->dat.name = source->dat.name;
	break;
      case 2:
	dest->dat.num = source->dat.num;
	break;
      case 3:
	dest->dat.val = source->dat.val;
	break;
      case 4:
	dest->dat.flag = source->dat.flag;
	break;
      default:
	break;
    }
}

/*
** tabwords() -- parse string into words
**
**	'tabwords()' is like 'STgetwords()' but only recognizes tab as white-
**	space aand recognizes newline as a terminator.  Unlike 'STgetwords()',
**	it uses the input value, 'num', only as a limit on the token array and
**	returns the actual count.
**
**	Double quote marks (") are also scanned across for VMS.  VMS uses
**	quotes in output format strings to force case sensitivity.  When
**	reading such a format string for input, we want to ignore them.
**	Caution - quotes are just treated as delimiters here, we don't
**	look for pairs of them.
**
**	History:
**		08/85 (rlm) -- Written.
**		12/85 (jhw) -- Modified to return count.
**		2/86 (rlm)  -- quote mark treatment
**		10/90 (Mike S) -- Add fifth argument
**		29-jul-1992 (rdrane)
**			Removed declaration of FEsalloc(), since it has been
**			placed in fe.h.
*/

static i4
tabwords (buf, num, tok, alloc, noparselast)
register char	*buf;
i4		num;
register char	**tok;
bool 		alloc;
bool		noparselast;	/* Don't parse the last (syntax) argument */
{
    register i4	i;

    if (alloc)
    {

	buf = FEsalloc(buf);
    }

    for (i = 0 ; i < num ; ++i)
    {
	while (*buf == '\t' || *buf == '"')
	    ++buf;
	if (*buf == EOS || *buf == '\n')
	    break;
	tok[i] = buf;
    	if (noparselast && i == (num - 1))
    	{
    	    /* This is the last (syntax) token in the header line; use it all */
	    i++;
	    break;
	}
	while (*buf != '"' && *buf != '\t' && *buf != EOS && *buf != '\n')
	    ++buf;
	if (*buf != EOS)
	{
	    *buf++ = EOS;
	}
    } /* end for */

    return i;
}

#ifdef ARGDEBUG
rarg_dump (r)
ARGRET *r;
{
    switch (r->type)
    {
      case 1:
	SIfprintf(stderr, ERx("%s\n"), r->dat.name);
	break;
      case 2:
	SIfprintf(stderr, ERx("%d\n"), r->dat.num);
	break;
      case 3:
	SIfprintf(stderr, ERx("%d\n"), r->dat.val);
	break;
      case 4:
	SIfprintf(stderr, ERx("%d\n"), r->dat.flag);
	break;
      default:
	SIfprintf(stderr, ERx("?\n"));
	break;
    }
}
#endif

