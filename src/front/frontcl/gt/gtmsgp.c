/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<st.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	<si.h>
# include	<cm.h>
# include	<te.h>
# include	<termdr.h>
# include	"ergt.h"
# include	<ug.h>

/**
** Name:	gtmsgp.c -	Graphics System Non-Forms Message & Pause
**				Routine.
**
** Description:
**	Contains the Graphics system message and pause routine.	 This version
**	does not use the Forms system (see 'GTfmsg_pause()' in "gtedit.c".)
**
**	GTmsg_pause()	print message and wait for return.
**
** History:
**	86/01/15  00:31:48  wong
**		Initial revision.
**
**	86/04/15  15:41:42  wong
**		Now outputs cursor motion and <ESC> sequence to GKS device.
**
**	86/04/18  14:30:50  wong
**		Do not output newline at end.
**
**	86/07/24  09:47:45  bobm
**		set raw mode while waiting for user input in rungraph
**
**	30-oct-86 (bruceb) Fix for bug 9688
**		changed from '||' to '&&' in 'if (X != NULL && *X != EOS)'
**	14-oct-87 (bruceb)
**		call TEopen with a dummy TEENV_INFO struct for new interface.
**	19-oct-87 (bobm)
**		use TE exclusively instead of SI for output so that
**		bell gets flushed properly on VMS
**	17-nov-88 (bruceb)
**		Added dummy arg in call on TEget.  (arg used in timeout.)
**	26-Aug-1993 (fredv) - Included <st.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

extern char	*G_tc_cm;	/* Graphics TERMCAP cursor motion */

extern i4	G_lines;	/* Graphics lines on device */

/*{
** Name:	GTmsg_pause() - Print message and wait for return.
**
**	Move to the bottom of screen and wait for a <CR>.  Prompt if message
**	string is set.	As a side effect, all characters typed up to the <CR>
**	are echoed to enable users to type printer escapes, etc.
**
**	This routine is almost identical with 'GTfmsg_pause()' in "gtedit.c"
**	except that it does not use the Forms system.
**
** Parameters:
**	prompt -- (input only)	Message string.
**
** History:
**	12/85 (jhw) -- Written.
*/

VOID
GTmsg_pause (prompt)
char	*prompt;
{
	register char	*gp;
	register char	ch;
	char		bufr[FE_PROMPTSIZE];
	char		*TDtgoto();
	TEENV_INFO	open_dummy;

	/* Get cursor motion; ignore possible pad */

	if (G_tc_cm != NULL && *G_tc_cm != EOS)
	{
		gp = TDtgoto(G_tc_cm, 0, G_lines - 1);
		while (CMdigit(gp))
			 CMnext(gp);

		GTputs(gp);	/* move cursor on GKS device */
	}

	TEopen(&open_dummy);
	TErestore(TE_FORMS);

	if (prompt != (char *)NULL)
	{
		IIUGfmt(bufr, FE_PROMPTSIZE,
				ERget(F_GT0014_r_RET_format), 1, prompt);
		TEwrite (bufr, STlength(bufr));
	}
	TEput(BELL);
	TEflush();

	/* Wait for a <cr> */

	for (ch = TEget((i4)0); ch != '\r' && ch != '\n'; ch = TEget((i4)0))
	{
		char	output[2];

		/* Pass through possible <ESC> sequence */
		output[0] = ch; output[1] = EOS;
		GTputs(output);		/* output character to GKS device */
	}

	TEclose();
}
