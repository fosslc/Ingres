/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include "ioistd.h"
#include "ioifil.h"

/**
** Name:	ioiftch.c - fetch image file
**
** Description:
**	Fetch and initialize image file.
*/

/* address of allocated memory is placed here. If non-zero,
   this address is assumed to be freeable when the session
   is closed.                                             */
static char *Fnbuf = 0;

/*{
** Name:	IIOIifImgFetch - fetch image file
**
** Description:
**	Fetch image file data into memory (run time table, header).
**	File is left open, and a pointer to it returned if OSL frames
**	are contained in the image.
**
** Inputs:
**	fname	file name
**
** Outputs:
**	ohdr	in memory image file data.
**
**	return:
**		OK		success
**		ILE_FILOPEN	can't open file
**		ILE_IMGBAD	bad file format
**		ILE_NOMEM	can't allocate file name.
**
** History:
**	9/86 (rlm)	written
**	21-sep-88 (kenl)
**		Changed MEalloc() call to MEreqmem() call.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

STATUS
IIOIifImgFetch (fname,ohdr)
char *fname;
OLIMHDR *ohdr;
{
	FILE *fp;
	STATUS rc;
	ABRTSOBJ *aobj;
	i4 pos;

	/*
	** open file.  This is SI_RACC filetype so size can be backpatched
	** by DS (IS_RACC sh_type).  We do "lose" some storage here, but it
	** is not a great deal, and the current design of calling code means
	** that a process will only come through here once, anyway.  We
	** allocate it in the "zero" pool with other miscellaneous stuff.
	*/
	if ((Fnbuf = (char *)MEreqmem((u_i4)0, (u_i4)(MAX_LOC+1), FALSE,
		(STATUS *)NULL)) == NULL)
	{
		return (ILE_NOMEM);
	}

	STlcopy(fname, Fnbuf, MAX_LOC+1);
	if (LOfroms(FILENAME&PATH,Fnbuf,&ohdr->loc) != OK)
		return (ILE_FILOPEN);
	if (SIfopen(&ohdr->loc,"r",SI_RACC,IMRECLEN,&fp) != OK)
		return (ILE_FILOPEN);

	/*
	** get ohdr from trailer.  Also returns position of runtime table
	*/
	if ((rc = trail_get(fp,ohdr,&pos)) != OK)
		return (ILE_IMGBAD);

	/*
	** get runtime table.  First seek to appropriate position.
	*/
	SIfseek(fp,pos,SI_P_START);
	if ((rc = rtt_get(fp,&aobj)) != OK)
		return (ILE_IMGBAD);
	ohdr->rtt = aobj;
	return (OK);
}


/*{
** Name:	IIOIifImgFree - free memory allocated by IIOIifImgFetch
**
** Description:
**	This routine frees memory allocated by IIOIifImgFetch.
**
** Inputs:
**	None
**
** Outputs:
**	None.
**
**	return:
**		None.
**
** History:
**	July 30, '87 (steveh) - written
**
*/

void 
IIOIifImgFree()
{
	if(Fnbuf != NULL)
	{
		MEfree(Fnbuf);	
		Fnbuf = NULL;
	}
} /* End of IIOIifImgFree */

