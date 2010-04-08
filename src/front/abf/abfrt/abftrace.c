/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<tr.h>		/* 6-x_PC_80x86 */
#include	<si.h>
#include	<lo.h>
#include	<st.h>
#include	<er.h> 
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<abfcnsts.h>
#include	<abftrace.h>

/*
** These data definitions are included here since nobody else
** needs to know about them.
*/
static i4	abTrace[ABTRACESIZE] = {0};
static FILE	*abtrcfile = NULL;



/*{
** Name:    abtrcinit() -	Initialize the trace vector.
**
** Description:
**	Initializes the trace vector by calling TRmaketrace.
**
** Inputs:
**	argv		-	The argv to use to initialize the trace
**				vector.
**
** History:
**	26-sep-1985 (joe)
**		Written
**	06/16/86 (a.dea) -- Initialize variables trcloc & trcbuf.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      13-oct-2008 (stegr01)
**          multiple changes for VMS itanium DECC compiler
**          this gets ACCVIOs using old ansi style function declarations
**          and doesn't like more/less args in the call than there should be
**          move data declarations to the top of the file
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/
VOID
abtrcinit(char	**argv)
{
	abtrcfile = stderr;
	TRmaketrace(argv, 'T', ABTRACESIZE, abTrace, TRUE);
}

/*{
** Name:	abtrcreset() -	Reset the trace flags
**
** Description:
**	Resets the trace flags from a string.
**
** Inputs:
**	buf	-	The string containing the new trace flags.
**
**
** Side Effects:
**	Will place nulls in the passed buffer.
**
** History:
**	26-sep-1985 (joe)
**		Written
*/
VOID
abtrcreset(char	*buf)
{
	char	*argv[20];
	i4	cnt;

	argv[0] = ERx("-T");
	argv[1] = NULL;
	TRmaketrace(argv, 'T', ABTRACESIZE, abTrace, FALSE);
	STgetwords(buf, &cnt, argv);
	argv[cnt] = NULL;
	TRmaketrace(argv, 'T', ABTRACESIZE, abTrace, TRUE);
}

/*{
** Name:	abtrcopen() -	Open a new file for the trace output.
**
** Description:
**	Opens a new file for the trace output.	If a file is already open
**	then it is closed first.  Note that by default the trace file
**	points to stderr.  If the abtrcfile is not NULL and points to
**	something other than stderr then another file is open.
**
** Inputs:
**	name		-	Name of file to open.
**
** Outputs:
**	Errors
**		ABTRCFILE -	Can't open trace file.
**
** History:
**	26-sep-1985
**		Written
*/
static LOCATION trcloc ZERO_FILL ;
static char	trcbuf[MAX_LOC] ZERO_FILL;

VOID
abtrcopen(char	*name)
{
	if (abtrcfile != NULL && abtrcfile != stderr)
		SIclose(abtrcfile);
	STcopy(name, trcbuf);
	LOfroms(PATH&FILENAME, trcbuf, &trcloc);
	if (SIopen(&trcloc, ERx("w"), &abtrcfile) != OK)
	{
		aberrlog(ERx("abtrcopen"), ABTRCFILE, name);
	}
}


VOID
abtrctofile(FILE *fp)
{
	char	buf[MAX_LOC];

	if (abtrcfile == stderr)
	{
		SIfprintf(fp, ERx("Trace file is stderr\n"));
	}
	else if (abtrcfile == NULL)
	{
		SIfprintf(fp, ERx("Trace file is NULL\n"));
	}
	SIclose(abtrcfile);
	if (SIopen(&trcloc, ERx("r"), &abtrcfile) != OK)
	{
		aberrlog(ERx("abtrcopen"), ABTRCFILE, trcbuf);
	}
	while (SIgetrec(buf, sizeof(buf) - 1, abtrcfile) == OK)
		SIfprintf(fp, buf);
	SIclose(abtrcfile);
	if (SIopen(&trcloc, ERx("w"), &abtrcfile) != OK)
	{
		aberrlog(ERx("abtrcopen"), ABTRCFILE, trcbuf);
	}
}


/* VARARGS2 */
VOID
abtrsrout(char *routine, char *fmt, char *a1, char *a2, char *a3, char *a4, char *a5, char *a6)
{
	char	buf[ABBUFSIZE];

	if (TRgettrace(ABTROUTENT, 0))
		abtrcprint(ERx("ROUTINE ENTRY -%s-"), routine, (char *) NULL,
                           NULL, NULL, NULL, NULL, NULL);
	if (TRgettrace(ABTROUTENT, 1))
		abtrctime((i4) ABTROUTENT);
	if (fmt != NULL)
	{
		STprintf(buf, ERx("ARGUMENT %s"), fmt);
		if (TRgettrace(ABTROUTENT, 2))
			abtrcprint(fmt, a1, a2, a3, a4, a5, a6, (char *) NULL);
	}
}

/* VARARGS2 */
VOID
abtrerout(char *routine, char *fmt, char *a1, char *a2, char *a3, char *a4, char *a5, char *a6)
{
	char	buf[ABBUFSIZE];

	if (TRgettrace(ABTROUTEXIT, 0))
		abtrcprint(ERx("ROUTINE EXIT -%s-"), routine, (char *) NULL,
                           NULL, NULL, NULL, NULL, NULL);
	if (TRgettrace(ABTROUTEXIT, 1))
		abtrctime((i4) ABTROUTEXIT);
	if (fmt != NULL)
	{
		STprintf(buf, ERx("ARGUMENT %s"), fmt);
		if (TRgettrace(ABTROUTEXIT, 2))
			abtrcprint(fmt, a1, a2, a3, a4, a5, a6, (char *) NULL);
	}
}

VOID
abtrsdb(char *ret)
{
	if (TRgettrace(ABTDBOPS, 0))
		abtrcprint(ERx("DB OPS -%s-"), ret, (char *) NULL,
                           NULL, NULL, NULL, NULL, NULL);
	if (TRgettrace(ABTDBOPS, 1))
		abtrctime((i4) ABTDBOPS);
}

VOID
abtredb()
{
	if (TRgettrace(ABTDBOPS, 2))
		abtrcprint(ERx("DB OPS -number of tuples %d-"),
				IItuples(), (char *) NULL,
                           NULL, NULL, NULL, NULL, NULL);
	if (TRgettrace(ABTDBOPS, 1))
		abtrctime((i4) ABTDBOPS);
}

/* VARARGS1 */
VOID 
abtrcprint(char	*fmt, char *a1, char *a2, char *a3, char *a4, char *a5, char *a6, char *a7)
{
    if (abtrcfile == stdout)
	SIputc('\r', abtrcfile);
    SIfprintf(abtrcfile, fmt, a1, a2, a3, a4, a5, a6, a7);
    SIfprintf(abtrcfile, ERx("\n"));
    SIflush(abtrcfile);
}

/*{
** Name:    abtrcdfile() -	Dump a file to the trace file.
**
** Description:
**	This takes a file and dumps it to the trace file.
**
** Inputs
**	f	The file to dump
**
** History:
**	26-sep-1985 (joe)
**		Written
*/
VOID
abtrcdfile(LOCATION	*f)
{
	char	*cp;
	FILE	*fp;
	char	buf[256];

	if (SIopen(f, ERx("r"), &fp) != OK)
	{
		LOtos(f, &cp);
		abtrcprint(ERx("abtrcdfile: Can't open file %s to dump\n"), cp,
                           NULL, NULL, NULL, NULL, NULL, NULL);
		return;
	}
	while (SIgetrec(buf, 255, fp) == OK)
		abtrcprint(buf, 0, NULL, NULL, NULL, NULL, NULL, NULL);
	SIclose(fp);
}

/*
** For each trace value this is a string
** to print for it.
*/
static char *tstring[ABTRACESIZE] = {
	ERx("ROUTINE ENTRY"),
	ERx("ROUTINE EXIT"),
	ERx("DATABASE OPS"),
	ERx("COMPILATION"),
	ERx("IMAGE FILES"),
	ERx("APPL STRUCT"),
	ERx("FRM STRUCT"),
	ERx("LISTS"),
	ERx("SUB PROCS"),
	ERx("DEBUG FRM"),
	ERx("ERR LOG"),
	ERx("DEP INFO"),
	ERx("RUNTIME")
};

typedef struct
{
	i4	abtcum;
	i4	abtlast;
} ABTIME;

static ABTIME	abTime[ABTRACESIZE] = {0};
static i4	lasttime = 0;

VOID
abtrctime(i4	val)
{
	register ABTIME *tim = abTime;
	i4		curtime;
	i4		interval;

	curtime = TMcpu();
	if (val <= -1 || val >= ABTRACESIZE)
		return;
	tim += val;
	if (tim->abtlast == 0)
	{
		/*
		** start of time
		*/
		tim->abtlast = curtime;
	}
	interval = curtime - tim->abtlast;
	tim->abtcum +=	interval;
	abtrcprint(ERx("  TIME:%s  %d  %d  %d  %d"),
		tstring[val], curtime, curtime - lasttime,
		tim->abtcum,  interval,
		(char *) NULL, NULL);
	lasttime = curtime;
	tim->abtlast = curtime;
}
