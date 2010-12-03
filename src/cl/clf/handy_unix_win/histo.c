/*
**Copyright (c) 2004 Ingres Corporation
*/
#include	<compat.h>
#include	<gl.h>
#include	<systypes.h>
#include	<stdio.h>
#include	<pc.h>
#include	<st.h>
#include	<cv.h>

/**
** histo
**
** A simple histogram or plotting tool
**
** The caller supplies a unique name and the sample value
**
** The user must set the following:
**	II_{name}_LOG	{filename}		for triggering logging
**	II_{name}_DUMP	{count}			for periodic dumping
**	II_{name}_BREAK {nn}			call the function
**	II_{name}_BREAK	{nn-xx}
**							MEhistoBreak when
**							when that sample value 
**							is hit. Used for a
**							debugger.
**
** History:
**	28-Apr-89 (GordonW)
**		initial release.
**	03-May-89 (GordonW)
**		avoid any usage of CL routines causes
**		re-entrancy problems.
**	21-may-91 (seng)
**		Add <systypes.h> to clear <types.h> on AIX 3.1.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-Jan-2010 (wanfr01) Bug 123139
**	    Include cv.h for function defintions
**	29-Nov-2010 (frima01) SIR 124685
**	    Removed FUNC_EXTERN commands.
**/

#define	HISTO_SIZE	1024		/* default histo size */
#define	HISTO_GRANDU	1		/* default grandularity */
#define	HISTO_HWMARK	50		/* high water mark */
#define	HISTO_MAX	10		/* max number of running histos */

/*
** local data
*/
struct HISTO
{
	i4	histo[HISTO_SIZE];	/* histogram */
	i4	size;		/* size of above */
	i4	grandu;		/* grandularity */
	i4	hwmark;		/* high water mark */
	char	name[64];	/* caller's name */
	char	log[1024];	/* logfile name */
	i4	dump;		/* dump flag */
	i4	count;		/* count until dump */
	i4	samples;	/* total num of samples */
	i4	flag;		/* Histo flag -1=off/0=check/1=on */
	i4	breakpt;	/* break: -1=off/0=absolute/1=range */
	i4	start;		/* low break value */
	i4	end;		/* high break value */
} ;

/*
** Histo tables
*/
static	struct HISTO	histo[HISTO_MAX] ZERO_FILL;

extern	char	*getenv();

/*
** externals/forwards
*/

/* TABLE OF CONTENTS */
bool iiHisto(
	char *name,
	i4 sample);
VOID iiHistoDump(
	struct HISTO *histop);
VOID iiHistoExit(void);
VOID iiHistoBreak(
	char *name,
	i4 n);

bool
iiHisto(name, sample)
char	*name;
i4	sample;
{
	char	buf[1024], *ptr, *ptr0;
	struct HISTO	*histop;
	i4	n;

	/*
	** check if initializing a new histo
	*/
	for(n=0; n<HISTO_MAX; n++)
	{
		/*
		** find this entry
		*/
		if(strcmp(name, histo[n].name) == 0)
		{
			histop = &histo[n];
			break;
		}

		/*
		** empty slot ?
		*/
		if(strlen(histo[n].name) == 0)
		{
			histop = &histo[n];
			break;
		}
	}

	/*
	** did we find one ?
	*/
	if(histop == NULL)
		return(  FALSE );

	/*
	** are we running ?
	*/
	if(histop->flag == -1)
		return( FALSE );

	/*
	** leading edge call ?
	*/
	if(histop->flag == 0)
	{
		/*
		** log running ?
		*/
		STprintf(buf, "II_%s_LOG", name);
		CVupper(buf);
		if((ptr=getenv(buf)) == NULL)
		{
			histop->flag = -1;
			return(FALSE);
		}

		/*
		** save the log file name and envoking name
		*/
		strcpy(histop->log, ptr);
		strcpy(histop->name, name);
		PCatexit(iiHistoExit);
		histop->flag = 1;

		/*
		** set the size of the histo table
		*/
		histop->size = HISTO_SIZE;
		histop->grandu = HISTO_GRANDU;
		histop->dump = -1;
		histop->breakpt = -1;
		histop->hwmark = HISTO_HWMARK;

		/*
		** dumping running ?
		*/
		STprintf(buf, "II_%s_DUMP", name);
		CVupper(buf);
		if((ptr=getenv(buf)) != NULL)
			histop->dump = atoi(ptr);

		/*
		** test break flags
		**	can be:
		**
		** II_{NAME}_BREAK nn
		** II_{NAME}_BREAK nn-xx
		*/
		STprintf(buf, "II_%s_BREAK", name);
		CVupper(buf);
		if((ptr=getenv(buf)) != NULL)
		{
			histop->breakpt = 0;
			if((ptr0=STrchr(ptr, '=')) != NULL)
			{
				ptr--;
				*ptr0 = '\0';
				ptr0 += 2;
				histop->end = atoi(ptr0);
				histop->breakpt = 1;
			}
			histop->start = atoi(ptr);
			histop->end = histop->size * histop->grandu;
		}
	}		

	/*
	** do we want to call a break point routine
	*/
	switch (histop->breakpt)
	{
	case -1: break;	/* NO breakpoint */
	case 0:  if(histop->start == sample)
			iiHistoBreak(name, sample);
		break;
	default:
		if((sample >= histop->start) && (sample <= histop->end))
			iiHistoBreak(name, sample);
		break;
	}

	/* 
	** scale it and plug into table
	*/
	histop->samples++;
	sample /= histop->grandu;
	if(sample < histop->size)
		histop->histo[sample]++;
	else
		histop->histo[histop->size-1]++;

	/*
	** time to dump ?
	*/
	if(histop->dump != -1)
	{
		if(++histop->count >= histop->dump)
		{
			iiHistoDump(histop);
			histop->count = 0;
		}
	}
	return(TRUE);
}

VOID
iiHistoDump(histop)
struct HISTO	*histop;
{
	char	buf[65];
	i4	pid, x, low, high, flag = 0, omask;
	FILE	*fp;

	/*
	** generate a specific logfile name
	*/
	omask = umask(0);
	PCpid(&pid);
	STprintf(buf, "%s.%d", histop->log, pid);
	fp = fopen(buf, "w");
	umask(omask);
	if(fp == NULL)
		return;

	/*
	** write out header
	*/
	STprintf(buf,
		"%s: Histogram\nSize: %d\nGrandularity: %d\nSamples: %d\n",
		histop->name, histop->size, histop->grandu, histop->samples);
	write(fileno(fp), buf, STlength(buf));

	/*
	** Now write out to the high water mark
	*/
	for(x=0; x<histop->hwmark; x++)
	{
		STprintf(buf, "%d: %d\n", x, histop->histo[x]);
		write(fileno(fp), buf, STlength(buf));
	}

	/*
	** now compress and write out the rest of the table
	*/
	low = x;
	high = low + (histop->grandu - 1);
	for(; x<histop->size; x++)
	{
		/*
		** If not zero write out the samples
		*/
		if((histop->histo[x] != 0) && (x < histop->size -1))
		{
			STprintf(buf, "%d-%d: %d\n", low, high, histop->histo[x]);
			write(fileno(fp), buf, STlength(buf));
			low = x + 1;
			high = low + (histop->grandu - 1);
			flag = 0;
		}
		else
		{
			high += histop->grandu;
			flag = 1;
		}
	}

	/*
	** display any left over
	*/
	if(flag)
	{
		STprintf(buf, "%d-?: %d\n", low, histop->histo[x-1]);
		write(fileno(fp), buf, STlength(buf));
	}

	/*
	** close it
	*/
	fclose(fp);
}

VOID
iiHistoExit()
{
	i4	n;

	for(n=0; n<HISTO_MAX; n++)
	{
		if(strlen(histo[n].name))
			iiHistoDump(&histo[n]);
	}
}

VOID
iiHistoBreak(name, n)
char	*name;
i4	n;
{
	register i4  sample = n;
	register char *nam = name;
	return;
}

