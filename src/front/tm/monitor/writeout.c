# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"monitor.h"
# include	<er.h>
# include	"ermo.h"

/*
** Copyright (c) 2004 Ingres Corporation
**
**	WRITEOUT - write the buffer to a text file
**
**	History:
**	25-jan-90 (teresal)
**		Modified to comply with standards for calling LOfroms routine.
**	27-aug-90 (kathryn)
**		Check for NULL filename with \w command and return error 
**		message if no filename was given, for all platforms.
**		This was ifdefd for CMS - Removed the ifdef and changed the
**		error message not to give CMS File Name Specification syntax.
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
*/

writeout()
{
	char		errbuf[ER_MAX_LEN];
	char		*file;
	LOCATION	loc;
	char		locbuf[MAX_LOC+1];
	FILE		*dest;
	i4		status;
	i4		c;

	FUNC_EXTERN	char	*getfilenm();
	FUNC_EXTERN	i4	q_getc();

	errbuf[0] = EOS;
	file = getfilenm();

	/*
	**    CMS requires a non-null file name
	*/
	if (STlength(file)==0)
	{
		putprintf(ERget(F_MO0043_W_filename_required));
		return;
	}
	/*! BEGIN BUG */
	STcopy(file, locbuf);
	if (status = LOfroms(PATH & FILENAME, locbuf, &loc))
	{
		ERreport(status, errbuf);
		putprintf(ERget(F_MO003E_Bad_file_name), file, errbuf);
	}
	else if (status = SIopen(&loc, ERx("w"), &dest))
	{
		ERreport(status, errbuf);
		putprintf(ERget(F_MO003F_Cannot_open_file), file, errbuf);
	}
	/*! END BUG */
	else
	{
		if (!Nautoclear)
			Autoclear = 1;

		if (q_ropen(Qryiop, 'r') == (struct qbuf *)NULL)
			/* writeout: q_ropen */
			ipanic(E_MO006D_1501700);

		while ((c = q_getc(Qryiop)) > 0)
			SIputc((char)c, dest);

		SIclose(dest);

		if (q_ropen(Qryiop, 'a') == (struct qbuf *)NULL)
			/* writeout: q_ropen 2 */
			ipanic(E_MO006E_1501701);
	}
}
