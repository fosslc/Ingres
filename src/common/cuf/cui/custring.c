/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <cui.h>

/**
**
**  Name: CUSTRING.C - Common non-CL string functions
**
**  Description:
**	This file contains platform independent string functions that can
**	be used by front and back ends.
**
**          cus_trmwhite - Trim trailing white space.
**
**  History:
**	25-mar-93 (rickh)
**	    Created the file and populated it with one routine:  cus_trmwhite.
**	    I stole this routine from PSF (where it was called psf_trmwhite).
**
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name: cus_trmwhite 	- Trim trailing white space.
**
** Description:
**      Used by the parser facility for trimming trailing white space
**      in character texts residing in memory. The text may be null terminated
**	but doesn't have to be. This is the reason why the length argument
**	is passed along with the buffer pointer. This routine is currently
**	only used in calls to psf_error to make text presented to recipients
**	of error messages more user friendly.
**
** Inputs:
**      len				Max length to be scanned for white space
**	bufp				Pointer to the buf holding char data
**					(does not have to be null terminated)
** Outputs:
**	Returns:
**	    Length			Length of non-white part of the string
**
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**	11-apr-87 (stec)
**          written
**	16-apr-87 (stec)
**	    changed a little.
**	    Addressed performance and Kanji issues.
**	02-may-1990 (fred)
**	    Revamped to trim trailing white space from an arbitrary string.
**	    Previously, it was assumed that trailing white space started
**	    with the first bit o' white space from the beginning.  The routine
**	    will now trim from the end back.  This was necessitated for BLOB
**	    support which is the first case of a datatype name with an embedded
**	    blank (standard name - 'long varchar').
**	25-mar-93 (rickh)
**	    Moved here from PSFTRMWH.C and renamed with a cuf prefix.
*/
i4
cus_trmwhite(
	u_i4	len,
	char	*bufp)
{
    register char   *ep;    /* pointer to end of buffer (one beyond) */
    register char   *p;
    register char   *save_p;

    if (!bufp)
    {
	return (0);
    }

    p = bufp;
    save_p = p;
    ep = p + (int)len;
    for (; (p < ep) && (*p != EOS); )
    {
	if (!CMwhite(p))
	    save_p = CMnext(p);
	else
	    CMnext(p);
    }

    return ((i4)(save_p - bufp));
}
