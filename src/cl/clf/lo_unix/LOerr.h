/*
**	/mnt/ingres/CL/release/src/hdr/LOerr.h -- built from LOerr.def
**
** History:
**      05-Jul-2005 (fanra01)
**          Bug 114804
**          Consolidate LO status codes into lo.h.
*/

/* static char	*Sccsid = "@(#)LOerr.h	1.6  6/29/83"; */


/*
**	/mnt/ingres/CL/release/src/hdr/SIerr.h -- built from SIerr.def
**
**	included so that LOerror.c can be called by SI routines.
*/


#ifndef	SI_EMFILE
# define	SI_EMFILE	3005	/* Can't open another file -- maximum number of files already opened. */
#endif

