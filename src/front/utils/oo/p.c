/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<er.h>
# include	<lo.h>
# include	<si.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

/*
** History:
**
**		(lost in antiquity ...)
**
**              4/12/90 (Mike S)
**              Psetbuf now allocates a dynamic buffer
**              Jupbug 21023
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

/*
** We'll get a bigger buffer when we're within this margin of the end.
*/
# define MARGIN_SIZE 1024

static FILE	*Pfile = NULL;
static char	*pB = NULL;
static char	*printBuf = NULL;
static char	*pEnd;
static u_i4 pSize;
static char	**pHandle;
static u_i4	pTag;

/*

**	P -- print and flush formatted output
**		on file or into buffer;
*/

static VOID	Fprint();
static VOID	(*Pprint)() = Fprint;
static VOID	Sprint(); 

/* VARARGS1 */
P(s, a1, a2, a3, a4, a5, a6, a7, a8)
char *s, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
{
	(*Pprint)(s, a1, a2, a3, a4, a5, a6, a7, a8);
}

/*
**	Psetbuf -- make a char buffer the output buffer for P()
*/
Psetbuf(size, tag, handle)
u_i4 size;		/* Initial size of buffer */
u_i4 tag;		/* Memory allocation tag */
char	**handle;	/* Returned pointer to buffer */
{

	/* Insure a big enough buffer that we don't have to reallocate soon */
	size = max(size, 2*MARGIN_SIZE);

	printBuf = (char *)FEreqlng(tag, size, FALSE, NULL);
	if ((char *)NULL == printBuf)
		IIUGbmaBadMemoryAllocation(ERx("Psetbuf"));

	/* Save buffer information */
	pB = *handle = printBuf;
	pHandle = handle;
	pEnd = (pSize = size) + printBuf;
	pTag = tag;

	Pprint = Sprint;
}

/*
**	Psetfile -- open a file and make it output file for P()
**		pass stdout, stderr or char * file name.
*/
Psetfile(f)
char	*f;
{
	LOCATION	loc;
	Pprint = Fprint;
	if (f == (char *)stdout)
	    Pfile = stdout;
	else if (f == (char *)stderr)
	    Pfile = stderr;
	else 
	{
	    if (!Pfile) Pfile = stdout;
	    LOfroms(FILENAME, f, &loc);
	    SIopen(&loc, "w", &Pfile);
	}
}

/* VARARGS1 */
static VOID
Fprint(s, a1, a2, a3, a4, a5, a6, a7, a8)
char *s, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
{
	if (!Pfile) Pfile = stdout;
	SIfprintf(Pfile, s, a1, a2, a3, a4, a5, a6, a7, a8);
	SIfprintf(Pfile, "\n");
	SIflush(Pfile);
}

/* VARARGS1 */
static VOID
Sprint(s, a1, a2, a3, a4, a5, a6, a7, a8)
char *s, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
{
	STprintf(pB, s, a1, a2, a3, a4, a5, a6, a7, a8);
	pB = &pB[STlength(pB)];

	/* 
	** If we're within a MARGIN_SIZE of the end, double the 
	** buffer allocation.
	*/
	if ((pEnd - pB) < MARGIN_SIZE)
	{
		char *tptr;

		pSize *= 2;
		tptr = (char *)FEreqlng(pTag, pSize, FALSE, NULL);
		if ((char *)NULL == tptr)
			IIUGbmaBadMemoryAllocation(ERx("Sprint"));

		/* Copy what we've got and free the old buffer */
		STcopy(printBuf, tptr);
		
		/* Redo the buffer information */
		printBuf = tptr;
		pB = printBuf + STlength(printBuf);
		pEnd = printBuf + pSize;
		*pHandle = printBuf;
	}
}
