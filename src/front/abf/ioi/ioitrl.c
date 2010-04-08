/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include "ioistd.h"

/**
** Name:	ioitrl.c - trailer utilities.
**
** Description:
**	IOI routines to deal with the image file trailer
**
** History:
**	Revision 6.3/03/00  90/09  wong
**	Include "ioitrl.h" in-line since it's only used here.
**
**	Revision 5.1  86/09  bobm
**	Initial revision.
**      03-jun-1999 (hanch04)
**          Change SIfseek to use OFFSET_TYPE and LARGEFILE64 for files > 2gig
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

typedef struct
{
	i4 version;	/* version number */
	i4 magic;	/* magic number */
	i4 iact;	/* ILRF interaction mode */
	i4 rttpos;	/* position of runtime table */
	i4 xpand[12];	/* future growth */
} IMG_TRAILER;

#define TRAIL_MAGIC 1017

/*{
** Name:	trail_write - write trailer
**
** Description:
**	Write trailer to file.
**
** Inputs:
**	pos	position of run time table
**	iact	interaction mode for ILRF
**	fp	file
**
** Outputs:
**
**	return:
**		OK		success
**
** History:
**	9/86 (rlm)	written
**	14-sep-1989 (wolf)
**		Add CMS-only code to write trailer into RACC file.
*/

STATUS
trail_write ( pos, iact, fp )
i4	pos;
i4	iact;
FILE	*fp;
{
	i4		dummy;
	IMG_TRAILER	trailer;

	/* seek to end of file, to be sure */
	SIfseek(fp, 0L, SI_P_END);

#ifdef CMS
	{
		OFFSET_TYPE bytes;
		char	filler[IMRECLEN];

		/* Pad the file before writing the trailer so that the trailer
		** is at the end of a record.  Otherwise, since CMS does not
		** support seeks except on fixed length record files, the
		** record will be padded and the last bytes in the file will
		** not be the trailer.
		*/

		SIftell(fp, &bytes);
		bytes = IMRECLEN - (bytes % IMRECLEN);	/* no. bytes left */
		if ( bytes != sizeof(IMG_TRAILER) )
		{ /* ... needs padding */
			bytes -= sizeof(IMG_TRAILER);	/* no. bytes to fill */
			if ( bytes < 0 )
			{ /* ... not enough room for trailer */
				bytes += IMRECLEN;	/* write extra record */
			}
			MEfill((u_i2)bytes, 'X', filler);
			SIwrite(bytes, filler, &dummy, fp);
		}
	}
#endif
	/*
	** Fill the trailer with consistent data for unused bytes -- try
	** to keep differences from appearing in identical image files.
	*/
	MEfill((u_i2)sizeof(IMG_TRAILER), '\0', (char *)&trailer);

	/* fill in trailer */
	trailer.rttpos = pos;
	trailer.iact = iact;
	trailer.version = IMAGE_VERSION;
	trailer.magic = TRAIL_MAGIC;

	/* write to file */
	return SIwrite((i4)sizeof(IMG_TRAILER), (char*)&trailer,&dummy,fp);
}

/*{
** Name:	trail_get - get trailer from file
**
** Description:
**	get trailer from file.  Return filled in OLIMHDR, except for
**	runtime table item, and file position to read run time table.
**	returns a fail code for bad read or bad magic.
**
** Inputs:
**	fp	file
**
** Outputs:
**	ohdr	data read from trailer
**	pos	returned position for runtime table
**
**	return:
**		OK		success
**		fail code	SIread failure or bad magic.
**
** History:
**	9/86 (rlm)	written
**
*/

STATUS
trail_get(fp,ohdr,pos)
FILE	*fp;
OLIMHDR	*ohdr;
i4	*pos;
{
	STATUS rc;
	i4 count;
	IMG_TRAILER trailer;

	/* seek to trailer */
	SIfseek(fp,(i4) - sizeof(IMG_TRAILER),SI_P_END);

	/* read trailer */
	if ((rc = SIread(fp, sizeof(IMG_TRAILER), &count, (char *) &trailer)) != OK)
		return (rc);

	/*
	** version check may expand to several values in the future -
	** version is externally known, magic is not.  Version may
	** be used to handle differences in image content.
	*/
	if (trailer.magic != TRAIL_MAGIC)
		return (FAIL);
	if (trailer.version > IMAGE_VERSION || trailer.version <= 0)
		return (FAIL);

	/* fill in ohdr and pos from trailer */
	ohdr->fp = fp;
	ohdr->iact = trailer.iact;
	ohdr->version = trailer.version;
	*pos = trailer.rttpos;

	return (OK);
}
