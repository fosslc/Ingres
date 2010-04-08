/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<st.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<te.h>
# include	<termdr.h>
# include	<gtdef.h>
# include	"ergt.h"

/**
** Name:    gtstatus.c -	Graphics System Edit Status Line Module.
**
** Description:
**	This module handles setting of the edit status line for the Graphics
**	system in edit (interactive) mode.  This file defines:
**
**	GTstlupdt
**	GTstlinit
**	GTstloff
**	GTstlpush
**	GTstlpop
**	GTstlrec
**	GTstlempty
**	GTstlcat
**
** History:
**		Revision 40.102	 86/04/09  13:22:48  bobm
**		LINT changes
**
**		Revision 40.101	 86/03/24  18:00:58  bobm
**		GTstat -> GTstl
**
**		Revision 40.100	 86/02/26  17:45:36  sandyd
**		BETA-1 CHECKPOINT
**
**		Revision 40.11	86/02/05  16:52:55  bobm
**		update in GTstlcat.
**
**		Revision 40.10	86/01/28  10:44:17  bobm
**		declare TDtgoto char *
**
**		Revision 40.9  86/01/24	 12:48:43  bobm
**		ls
**		wipe out old reverse video band when updating non-reverse status line.
**
**		Revision 40.8  86/01/21	 12:45:39  bobm
**		remove spurious "rec_stat" call - replace with GTstlrec.
**
**		Revision 40.7  86/01/16	 14:48:17  bobm
**		Status line stack, formerly done at "vig" level.
**
**		Revision 40.6  86/01/13	 16:25:40  wong
**		Replaced 'G_srow' with reference to 'G_lines' ("G_SROW").
**
**		Revision 40.0  85/09/30	 15:08:35  bobm
**		Initial revision.
**	10/19/89 (dkh) - Added VOID return type to GTstlupdt().
**	26-Aug-1993 (fredv) - Included <st.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**	A stack of status lines is maintained.
**
**	NOTE:
**		Only pointers are stacked, so string argument passed
**		to "GTstlpush" must be valid between the call and the
**		closing "GTstlpop"
*/

extern i4	G_lines;	/* Graphics lines on device */
extern i4	G_cols;		/* Graphics columns on device */

/* Graphics TERMCAP */
extern char *G_tc_cm, *G_tc_rv, *G_tc_re, *G_tc_ce;

FUNC_EXTERN i4  GTchout ();

static char *Stat[48];
static i4  Sidx = -1;

static bool Stat_update = FALSE;
static bool Rev;
static i4  Curlen;
static bool Currev;

GTstloff ()
{
	Stat_update = FALSE;
}

GTstlinit ()
{
	Stat_update = TRUE;
	Rev = Currev = FALSE;
	Curlen = 0;
}

/*
**	called with force = TRUE if this is a refresh.	Refresh means
**	screen has been erased (no current length, line is not reversed)
*/
VOID
GTstlupdt (force)
bool force;
{
	char *TDtgoto();
	i4 i,lim,oldcur;
	char *statbuf;

	if (Sidx < 0)
		statbuf = ERx("");
	else
		statbuf = Stat[Sidx];

	if (force || Stat_update)
	{
# ifdef DGC_AOS
		GTlock(TRUE, 0, 0);
# endif
		if (force)
		{
			Curlen = 0;
			Currev = FALSE;
		}

		GTdmpstat (statbuf);
		Stat_update = FALSE;
		TDtputs(TDtgoto(G_tc_cm, 0, G_SROW), 1, GTchout);

		if (Rev)
			TDtputs (G_tc_rv,1,GTchout);
		TEput (' ');
		lim = G_cols - 1;
		for (i=0; i < lim && statbuf[i] != '\0'; ++i)
			TEput (statbuf[i]);
		oldcur = Curlen;
		Curlen = i;
		if (Rev || G_tc_ce == NULL || *G_tc_ce == '\0')
		{
			/*
			** if proper attribute already, only have to wipe
			** out old characters, otherwise produce band.
			** Down this leg to overprint in BOTH attributes
			** on terminals with no clear to end of line
			*/
			if ((Currev && Rev) || (!Currev && !Rev))
				lim = oldcur;
			for ( ; i < lim; ++i)
				TEput (' ');
			if (Rev)
				TDtputs (G_tc_re,1,GTchout);
		}
		else
		{
			/*
			** if shorter OR IN REVERSE VIDEO, zap.
			*/
			if (Curlen < oldcur || Currev)
				TDtputs (G_tc_ce,1,GTchout);
		}
		Currev = Rev;

# ifdef DGC_AOS
		GTcsunl();
# else
		GTcursync();
# endif
	}
}

GTstlrec(r)
bool r;
{
	Stat_update = TRUE;
	Rev = r;
}

GTstlpush(s,r)
char *s;
bool r;
{
	if (Sidx >= 48)
		GTsyserr (S_GT000F_Runaway_stack,0);
	++Sidx;
	Stat[Sidx] = s;
	Stat_update = TRUE;
	Rev = r;
}

GTstlpop(r)
bool r;
{
	if (Sidx < 0)
		GTsyserr(S_GT0010_stack_underflow,0);
	--Sidx;
	GTstlrec(r);
}

GTstlempty(r)
bool r;
{
	Sidx = -1;
	Stat_update = TRUE;
	Rev = r;
}

GTstlcat(s)
char *s;
{
	i4 len;
	i4 lim;
	i4 i;
	char *TDtgoto();

# ifdef DGC_AOS
	GTlock(TRUE, 0, 0);
# endif
	GTstlupdt(FALSE);
	if (Sidx >= 0)
		len = STlength(Stat[Sidx]) + 1;
	else
		len = 1;
	TDtputs(TDtgoto(G_tc_cm, len, G_SROW), 1, GTchout);
	if (Rev)
		TDtputs (G_tc_rv,1,GTchout);
	for (lim = G_cols - 1; len < lim && *s != '\0'; ++len)
	{
		TEput (*s);
		++s;
	}
	if (Rev || G_tc_ce == NULL || *G_tc_ce == '\0')
	{
		for (i = len; i < Curlen; ++i)
			TEput (' ');
		if (Rev)
			TDtputs (G_tc_re,1,GTchout);
	}
	else
	{
		if (len < Curlen)
			TDtputs (G_tc_ce,1,GTchout);
	}
	Currev = Rev;
	Curlen = len;
# ifdef DGC_AOS
	GTcsunl();
# else
	GTcursync();
# endif
}
