/*
** Copyright (c) 1984, 2000 Ingres Corporation
*/

/*
** 	static	char	*Sccsid = "%W% %G%";
*/

# include	<compat.h>
# include	<gl.h>
# include	<st.h>
# include	<si.h>
# include	<lo.h>
# include	<starlet.h>

/*
**  SIEQINIT - 	Open I/O stream for Equel programs.  
**
**	Equel C programs not starting at compatmain, or non-C Equel programs 
**	need to open the three standard channels for Libq and the Form system.
**	On VMS the sys$ channels need to be mapped to Standard in, Standard out
**	and Standard error. On UNIX this may not be needed at all and can just
**	be a noop.
**
**  Requires:
**		That input, output and error are opened in that order, so that
**  the corresponding slots in the file array map to stdin, stdout and stderr.
**
**  Parameters:
**		None.
**  Returns:
**		Error status or OK.
**  History:
**		7-jun-1984	Written (ncg)
**      16-jul-93 (ed)
**	    added gl.h
**      11-aug-93 (ed)
**          added missing includes
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
*/

STATUS
SIeqinit()
{
	LOCATION		loc;
	FILE			*fp;
	char			buf[MAX_LOC +1];
	/*
	** If stdin cannot be opened, attempt to open stdout, stderr for
	** possible error messages.
	*/
	i4			in_status, out_status, err_status;

	if (SIisopen(stdin))
		return (OK);

	STcopy("SYS$INPUT", buf);
	LOfroms(PATH & FILENAME, buf, &loc);
	in_status = SIopen(&loc, "r", &fp);

	STcopy("SYS$OUTPUT", buf);
	LOfroms(PATH & FILENAME, buf, &loc);
	out_status = SIopen(&loc, "w", &fp);

	STcopy("SYS$ERROR", buf);
	LOfroms(PATH & FILENAME, buf, &loc);
	err_status = SIopen(&loc, "w", &fp);

	if (in_status != OK)
		return (in_status);
	if (out_status != OK)
		return (out_status);
	return (err_status);			/* OK or a real status */
}
