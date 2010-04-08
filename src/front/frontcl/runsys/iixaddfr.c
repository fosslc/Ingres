/* 
** Copyright (c) 2004 Ingres Corporation  
*/ 
# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>

/*
** IIXADDFRM.C - Interface to IIaddform for COBOL which passes EXTERNALS
**		 by REFERENCE even if explicilty passed by VALUE.  The
**		 same applies to FORTRAN.
** History:
**		21-may-1985 - Written (ncg).
**		02-aug-1989 - shut up ranlib (GordonW)
**	16-sep-1993 (kathryn)
**	    Moved IIaddfrm() from the deleted front/frame/run30/ii30form.c 
**	    file to this file.  Although VMS ESQL/COBOL should have been 
**	    generating IIxaddfrm since release 4, it has been incorrectly 
**	    generating IIaddfrm which was an old 3.0 cover to IIaddform(). With
**	    release 6.5 and beyond the old 3.0 entry points were removed. 
**	    However, this one will need to stay around forever.
**			
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  11-Sep-2003 (fanra01)
**      Bug 110909
**      Add a version of IIxaddfrm for Windows platform
**	19-Sep-2003 (bonro01)
**	    Function IIaddfrm() was required for COBOL on i64_hpu
**	    Since this function is now required for VMS, Windows
**	    and UNIX, I removed the platform specific ifdef's
*/

void
IIxaddfrm( frm )
i4	**frm;			/* Really (FRAME **) */
{
    void IIaddform();

    IIaddform( *frm );
}
void
IIaddfrm( frmptr )
i4      *frmptr;
{
    void IIaddform();

    IIaddform(*frmptr);
}

