/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<tr.h>		 
# include	<st.h>		 
#include	<lo.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<ilerror.h>
#include	"iltrace.h"
#include	"il.h"

/**
** Name:	iltrace.c -	Trace routines for interpreter
**
** Description:
**	Trace routines for interpreter.
**	Adapted from the file abftrace.c in the abf directory. (agh)
**
** History:
**	03/09/91 (emerson)
**		Integrated DESKTOP porting changes.
**	4./91 (Mike S)
**		Move initialization of iiotrcfile to IIOtiTrcInit..
**	16-aug-91 (leighb)
**		Remove DESKTOP porting changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/


/*
** Data definitions for statics used only within this file.
*/
static i4	iioTrace[IIOTRSIZE] = {0};
static FILE	*iiotrcfile;
static LOCATION	iiotrcloc;
static char	iiotrcbuf[MAX_LOC+1];

/*
** This array could provide a string to print for each type of trace value.
**
** static char *tstring[IIOTRSIZE] = {
** 	"ROUTINE ENTRY",
** 	"STACK FRAME"
** };
*/

/*{
** Name:	IIOtiTrcInit	-	Initialize the trace vector.
**
** Description:
**	Initializes the trace vector by calling TRmaketrace.
**
** Inputs:
**	argv	The argv to use to initialize the trace vector.
**
** History:
**	19-jun-1986 (agh)
**		First written
*/
IIOtiTrcInit(argv)
char	**argv;
{
	TRmaketrace(argv, 'R', IIOTRSIZE, iioTrace, TRUE);
	iiotrcfile = stderr;		 
}

/*{
** Name:	IIOtrTrcReset	-	Reset the trace flags
**
** Description:
**	Resets the trace flags from a string.
**
** Inputs:
**	buf	The string containing the new trace flags.
**
**
** Side Effects:
**	Will place NULLs in the passed buffer.
**
** History:
**	19-jun-1986 (agh)
**		First written
*/
IIOtrTrcReset(buf)
char	*buf;
{
	char	*argv[20];
	i4	cnt;

	argv[0] = ERx("-R");
	argv[1] = NULL;
	TRmaketrace(argv, 'R', IIOTRSIZE, iioTrace, FALSE);
	STgetwords(buf, &cnt, argv);
	argv[cnt] = NULL;
	TRmaketrace(argv, 'R', IIOTRSIZE, iioTrace, TRUE);
}

/*{
** Name:	IIOtoTrcOpen	-	Open a new file for the trace output.
**
** Description:
**	Opens a new file for the trace output.  If a file is already open,
**	then it is closed first.  Note that by default the trace file
**	points to stderr.  If the iiotrcfile is not NULL and points to 
**	something other than stderr, then another file is open.
**
** Inputs:
**	name		Name of file to open.
**
** Outputs:
**	Errors
**		ILE_TRFILE 	-	Can't open trace file.
**
** History:
**	19-jun-1986 (agh)
**		First written
*/
IIOtoTrcOpen(name)
char	*name;
{
	if (iiotrcfile != NULL && iiotrcfile != stderr)
		SIclose(iiotrcfile);
	STlcopy(name, iiotrcbuf, sizeof(iiotrcbuf));
	if ( LOfroms(PATH & FILENAME, iiotrcbuf, &iiotrcloc) != OK
			|| SIopen(&iiotrcloc, ERx("w"), &iiotrcfile) != OK )
		IIOerError(ILE_STMT, ILE_TRFILE, name, (char *) NULL);
}

/*{
** Name:	IIOttTrcTofile	-	Put trace information in a file
**
** Description:
**	Puts trace information in a file.
**
** Inputs:
**	fp		File to use
**
** Outputs:
**	Errors
**		ILE_TRFILE 	-	Can't open trace file.
**
** History:
**	19-jun-1986 (agh)
**		First written
*/
IIOttTrcTofile(fp)
FILE	*fp;
{
	char	buf[IL_BUFSIZE];

	if (iiotrcfile == stderr)
	{
		SIfprintf(fp, ERx("Trace file is stderr\n"));
	}
	else if (iiotrcfile == NULL)
	{
		SIfprintf(fp, ERx("Trace file is NULL\n"));
	}
	SIclose(iiotrcfile);
	if (SIopen(&iiotrcloc, ERx("r"), &iiotrcfile) != OK)
	{
		IIOerError(ILE_STMT, ILE_TRFILE, iiotrcbuf, (char *) NULL);
		return;
	}
	while (SIgetrec(buf, (IL_BUFSIZE - 1), iiotrcfile) == OK)
		SIfprintf(fp, buf);
	SIclose(iiotrcfile);
	if (SIopen(&iiotrcloc, ERx("w"), &iiotrcfile) != OK)
		IIOerError(ILE_STMT, ILE_TRFILE, iiotrcbuf, (char *) NULL);
	return;
}

/*{
** Name:	IIOtuTrcRout	-	Print trace info on routine entry
**
** Description:
**	Prints trace information on routine entry.
**
** Inputs:
**	routine		Routine that was entered.
**	fmt		Format string.
**	a1 - a6		Arguments to be inserted into format string.
**
** Outputs:
**	None.
**
** History:
**	19-jun-1986 (agh)
**		First written
*/
/* VARARGS2 */
IIOtuTrcRout(routine, fmt, a1, a2, a3, a4, a5, a6)
char	*routine;
char	*fmt;
char	*a1, *a2, *a3, *a4, *a5, *a6;
{
	char	buf[IL_BUFSIZE];

	if (TRgettrace(IIOTRROUTENT, 0))
		IIOtpTrcPrint(ERx("ROUTINE ENTRY -%s-"), routine, (char *)NULL);
	if (fmt != NULL)
	{
		STprintf(buf, ERx("ARGUMENT %s"), fmt);
		if (TRgettrace(IIOTRROUTENT, 2))
		{
			IIOtpTrcPrint(fmt, a1, a2, a3, a4, a5, a6,
					(char *) NULL);
		}
	}
}

/*{
** Name:	IIOtpTrcPrint	-	Print trace info
**
** Description:
**	Prints trace information.
**
** Inputs:
**	fmt		Format string.
**	a1 - a7		Arguments to be inserted into format string.
**
** Outputs:
**	None.
**
** History:
**	19-jun-1986 (agh)
**		First written
*/
/* VARARGS1 */
IIOtpTrcPrint(fmt, a1, a2, a3, a4, a5, a6, a7)
char	*fmt;
char	*a1, *a2, *a3, *a4, *a5, *a6, *a7;
{
	if (iiotrcfile == stdout)
		SIfprintf(iiotrcfile, ERx("\r"));
	SIfprintf(iiotrcfile, fmt, a1, a2, a3, a4, a5, a6, a7);
	SIfprintf(iiotrcfile, ERx("\n"));
	SIflush(iiotrcfile);
}

/*{
** Name:	IIOtdTrcDfile	-	Dump a file to the trace file
**
** Description:
**	Take a file and dump it to the trace file.
**
** Inputs
**	loc	The file to dump
**
** History:
**	19-jun-1986 (agh)
**		First written
*/
IIOtdTrcDfile(loc)
LOCATION	*loc;
{
	char	*cp;
	FILE	*fp;
	char	buf[IL_BUFSIZE];

	if (SIopen(loc, ERx("r"), &fp) != OK)
	{
		LOtos(loc, &cp);
		IIOtpTrcPrint(ERx("IIOtdTrcDfile: Can't open file %s to dump\n"),cp);
		return;
	}
	while (SIgetrec(buf, (IL_BUFSIZE - 1), fp) == OK)
		IIOtpTrcPrint(buf, 0);
	SIclose(fp);
}
