# include	<compat.h>
# include	<gl.h>
# include	<si.h>
# include	<lo.h>

#ifdef VMS
    globalvalue	iicl__siopen;	
#endif

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
**		22-mar-1988	Kevinm - modified code so that a more specific
**				status was returned if si_open returned a bad
**				status.
**		06-Feb-1989	(anton) - Fix UNIX logic to return proper error
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	19-jun-95 (emmag)
**	    NT tags along to the UNIX ifdef.
*/

/* the following defines are channel open errors found in clerror. */
#define INCHAN_ERR  3010	/* can't open standard input channel */
#define OUTCHAN_ERR 3011	/* can't open standard output channel */
#define ERRCHAN_ERR 3012	/* can't open standar error channel */

STATUS
SIeqinit()
{
# if defined (UNIX) || defined (NT_GENERIC)
	
	/* Just verify the environment is normal */
	if (!SIisopen(stdin))
	    return(INCHAN_ERR);
	if (!SIisopen(stdout))
	    return(OUTCHAN_ERR);
	if (!SIisopen(stderr))
	    return(ERRCHAN_ERR);
	return(OK);

# endif
# ifdef VMS
	LOCATION		loc;
	FILE			*fp;
	char			buf[MAX_LOC +1];
	/*
	** If stdin cannot be opened, attempt to open stdout, stderr for
	** possible error messages.
	*/
	STATUS			in_status, out_status, err_status;

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

	if (in_status != OK){
		if (in_status == iicl__siopen)
			return(INCHAN_ERR);
		else
			return (in_status);
	}
	if (out_status != OK){
		if (out_status == iicl__siopen)
			return(OUTCHAN_ERR);
		else
			return (out_status);
	}
	if (err_status == iicl__siopen)
		return(ERRCHAN_ERR);
	else
		return (err_status);		/* OK or a real status */
# endif
}
