/*
**  wrhdr.c
**  write out a FLDHDR
**  the heading has already been written.
**
**  Copyright (c) 2004 Ingres Corporation
**
** History:
**
 * Revision 60.2.1.1  87/03/03  18:37:18  dave
 * Branch taken by dave.
 * Revision 60.3  87/04/07  23:35:57  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.2  86/11/18  20:57:27  dave
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/18  20:57:18  dave
 * *** empty log message ***
 * 
 * Revision 1.2  86/05/06  23:08:42  roger
 * *** empty log message ***
 * 
**		Revision 30.2  86/01/29  10:49:09  seiwald
**		Now calls DSwrite with sizeof() the argument instead of
**		the (wrong) magic cookie 13.  (Bug #6630)
**	03/03/87 (dkh) - Added support for ADTs.
**	06/01/87 (dkh) - Sync up with frame changes.
**	03/01/93 (dkh) - Fixed bug 49951.  Updated calls to DSwrite to
**			 handle interface change for type DSD_STR.
**	03/01/93 (dkh) - Fixed problem where a garbage value for frmode
**			 was being written out if the form was retrieved
**			 from the ii_encoded_forms catalog.
**	08/12/93 (dkh) - Fixed bug 53856.  DSwrite handles strings differently
**			 depending on whether the language is C or macro.
**			 So we pass different lengths, depending on the
**			 language, to accomodate the difference.
**      22-jan-1998 (fucch01)
**          Casted 2nd arg of DSwrite calls as type PTR to get rid of sgi_us5
**          compiler warnings...
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"decls.h"
# include	<st.h>

fcwrHdr(hdr)
	register FLDHDR	*hdr;
{
	i4	str_size;

	/*
	**  Clear out hdr->fhdfont to avoid picking up runtime
	**  garbage if the form is from the ii_encoded_forms catalog.
	*/
	hdr->fhdfont = 0;

	if (fclang == DSL_C)
	{
		str_size = STlength(hdr->fhdname) + 1;
	}
	else
	{
		str_size = sizeof(hdr->fhdname);
	}
	DSwrite(DSD_STR, hdr->fhdname, str_size);
	DSwrite(DSD_I4, (PTR)hdr->fhseq, 0);
	DSwrite(DSD_I4, 0, 0);				/* fhscr */
	DSwrite(DSD_SPTR, hdr->fhtitle, 0);
	DSwrite(DSD_I4, (PTR)hdr->fhtitx, 0);
	DSwrite(DSD_I4, (PTR)hdr->fhtity, 0);
	DSwrite(DSD_I4, (PTR)hdr->fhposx, 0);
	DSwrite(DSD_I4, (PTR)hdr->fhposy, 0);
	DSwrite(DSD_I4, (PTR)hdr->fhmaxx, 0);
	DSwrite(DSD_I4, (PTR)hdr->fhmaxy, 0);
	DSwrite(DSD_I4, 0, 0);				/* fhintrp */
	DSwrite(DSD_I4, (PTR)hdr->fhdflags, 0);
	DSwrite(DSD_I4, (PTR)hdr->fhd2flags, 0);
	DSwrite(DSD_I4, (PTR)hdr->fhdfont, 0);
	DSwrite(DSD_I4, (PTR)hdr->fhdptsz, 0);
	DSwrite(DSD_I4, (PTR)hdr->fhenint, 0);
	DSwrite(DSD_I4, 0, 0);				/* fhpar */
	DSwrite(DSD_I4, 0, 0);				/* fhsib */
	DSwrite(DSD_I4, 0, 0);				/* fhdfuture */
	DSwrite(DSD_I4, 0, 0);				/* fhd2fu */
}
