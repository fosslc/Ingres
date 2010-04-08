
/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<er.h>
#include "irstd.h"
#include "irmagic.h"

/**
** Name:	irfload.c - load form
**
** Description:
**	Calls IOI to load form from image file
*/

#ifdef ILRFDTRACE
extern FILE *Fp_debug;
#endif

/*{
** Name:	IIORflFormLoad - load form
**
** Description:
**	Loads form from clients open image file.
**
** Inputs:
**	irblk	ilrf client
**	fname	form name
**	fpos	form position in image file
**
**	tag	Tag to allocate form under. If -1 then this routine
**		will allocate its own tag.
**
** Outputs:
**	addr	returned form
**
**	return:
**		OK		success
**		IOR_CLIENT	not a legal client
**
** History:
**	10/86 (rlm)	written
**
*/

STATUS
IIORflFormLoad(irblk,fname,fpos,tag, addr)
IRBLK *irblk;
char *fname;
i4 fpos;
i2 tag;
PTR *addr;
{
	STATUS rc;

	CHECKMAGIC(irblk,ERx("fl"));

#ifdef ILRFTIMING
	IIAMtprintf(ERx("request retrieval of form %s"),fname);
#endif

	if (tag == -1)
		tag = FEgettag();

	rc = IIOIflFormLoad(&(irblk->image),fname,fpos,tag,addr);

#ifdef ILRFTIMING
	IIAMtprintf(ERx("%s retrieved"),fname);
#endif

#ifdef ILRFDTRACE
	fprintf(Fp_debug,ERx("IIOIflFormLoad returns %d\n"),rc);
	if (rc == OK)
		IIAMdump(Fp_debug,ERx("FORM"),*addr,50);
#endif

	return(rc);
}
