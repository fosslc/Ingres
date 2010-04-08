
/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
# include	"irstd.h"
# include	"irmagic.h"

/**
** Name:	irfwrite.c - frame write
**
** Description:
**	write frame to file
**
*/

#ifdef ILRFDTRACE
extern FILE *Fp_debug;
#endif

/*{
** Name:	IIORfwFrmWrite - write frame to file
**
** Description:
**	writes current frame to a client's file.  caller has the
**	responsibility of seeking to the correct file position, and
**	any recording of file position which must take place before
**	or after write.
**
** Inputs:
**		irblk	ilrf client
**		ohdr	image file
**
** Outputs:
**	pos	written position.
**
**	return:
**		OK		success
**		ILE_CLIENT	not a legal client
**		ILE_CFRAME	no current frame to write
**		ILE_FRDWR	write failure
**
** History:
**	8/86 (rlm)	written
**
*/

STATUS
IIORfwFrmWrite (irblk,ohdr,pos)
IRBLK *irblk;
OLIMHDR *ohdr;
i4 *pos;
{
	IFRMOBJ *frm;
	IR_F_CTL *frinf;
	STATUS rc;

	/* see that we have a legal client, current frame */
	CHECKMAGIC(irblk,ERx("fw"));
	CHECKCURRENT(irblk);

	/* get frame */
	frinf = (IR_F_CTL *)(irblk->curframe);
	frm = frinf->d.m.frame;

	rc = IIOIfwFrmWrite(ohdr,frm,pos);
#ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("IIOIfwFrmWrite return: %d\n"),rc);
#endif

	return (rc);
}
