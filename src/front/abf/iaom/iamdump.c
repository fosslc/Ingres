/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include "iamstd.h"
#include <si.h>
#include <fdesc.h>

static FILE *Debug_fp = NULL;

/*
** debug routines.  All calls normally ifdef'ed out
** May be called from ILRF with file pointer other than Debug_fp.
** NULL is used from within AOM to indicate use of Debug_fp.
**
** Debug_fp is defined here so that the symbol only turns up
** in one place, but doesn't force any unwanted linkages, and will
** always be defined.
**
** aomprintf is also called only when debug tracing is compiled in.
** it handles opening the trace file on the initial call.
**
** NOTE: all this code only works on UNIX / DOS.  We call fopen / fprintf
** directly, without benefit of compatlib.  Basically avoiding hassling
** with the NM / LO stuff for the debug output.
**
** History:
**	03/09/91 (emerson)
**		Integrated DESKTOP porting changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
*/

IIAMdump(fp,title,addr,num)
FILE *fp;
char *title;
i4 *addr;
i4  num;
{
#ifdef AOMDTRACE					/* 6-x_PC_80x86 */
	i4 i;

	if (fp == NULL)
	{
		if (Debug_fp == NULL)
			Debug_fp = fopen(ERx("aom.out"),ERx("w"));
		fp = Debug_fp;
	}

	num /= sizeof(i4);

	fprintf(fp,ERx("\n	    %s (X%lx)"),title,(i4) addr);
	for (i=0; i < num; ++i)
	{
		if ((i%8) == 0)
			fprintf(fp,ERx("\n: %8lx"),*addr);
		else
			fprintf(fp,ERx(" %8lx"),*addr);
		++addr;
	}
	fprintf(fp,ERx("\n"));
#endif							/* 6-x_PC_80x86 */
}

IIAMstrdump(fp,addr,num)
FILE *fp;
char **addr;
i4  num;
{
#ifdef AOMDTRACE					/* 6-x_PC_80x86 */
	if (fp == NULL)
	{
		if (Debug_fp == NULL)
			Debug_fp = fopen(ERx("aom.out"),ERx("w"));
		fp = Debug_fp;
	}

	fprintf(fp,ERx("\n	    STRINGS\n"));
	for (; num > 0; num--,addr++)
		fprintf(fp,ERx("\"%s\"\n"),*addr);
#endif							/* 6-x_PC_80x86 */
}

IIAMsymdump(fp,addr,num)
FILE *fp;
FDESCV2 *addr;
i4  num;
{
#ifdef AOMDTRACE					/* 6-x_PC_80x86 */
	char *nptr,*tptr;

	if (fp == NULL)
	{
		if (Debug_fp == NULL)
			Debug_fp = fopen(ERx("aom.out"),ERx("w"));
		fp = Debug_fp;
	}

	fprintf(fp,ERx("\n	    SYMBOLS\n"));
	for (; num > 0; num--,addr++)
	{
		nptr = addr->fdv2_name;
		tptr = addr->fdv2_tblname;
		if (nptr == NULL)
			nptr = ERx("<NULL>");
		if (tptr == NULL)
			tptr = ERx("<NULL>");
		fprintf(fp,ERx("F\"%s\" T\"%s\" v%c %d\n"),
			nptr, tptr, addr->fdv2_visible, addr->fdv2_dbdvoff);
	}
#endif							/* 6-x_PC_80x86 */
}

aomprintf(s,a,b,c,d,e,f)
char *s;
i4 a,b,c,d,e,f;
{
#ifdef AOMDTRACE					/* 6-x_PC_80x86 */
	if (Debug_fp == NULL)
		Debug_fp = fopen(ERx("aom.out"),ERx("w"));
	fprintf(Debug_fp,s,a,b,c,d,e,f);
#endif							/* 6-x_PC_80x86 */
}

aomflush()
{
#ifdef AOMDTRACE					/* 6-x_PC_80x86 */
	if (Debug_fp != NULL)
		fflush(Debug_fp);
#endif                                                  /* 6-x_PC_80x86 */
}

aomclose()
{
	if (Debug_fp != NULL)
		fclose(Debug_fp);
	Debug_fp = NULL;
}
