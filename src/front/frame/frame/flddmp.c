/*
**	flddmp.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	flddmp.c - Dump a field structure.
**
** Description:
**	Dump a field structure to "stdout", used in debugging.
**
** History:
**	2/16/82 (ps)	written.
**	02/15/87 (dkh) - Added header.
**	08/14/87 (dkh) - ER changes.
**	12-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	"fdfuncs.h"
# include	<si.h>
# include	<er.h>




/*{
** Name:	FDflddmp - Dump a field structure.
**
** Description:
**	Given a field pointer, dump the structure to "stdout"
**	for debugging purposes.
**
** Inputs:
**	f	Pointer to FIELD structure to dump.
**
** Outputs:
**	Returns:
**		None.
**	Exceptions:
**		None.
**
** Side Effects:
**	None.
**
** History:
**	02/16/82 (ps) - Initial version.
**	02/15/87 (dkh) - Added procedure header.
*/
VOID
FDflddmp(f)
register FIELD	*f;
{
	SIprintf(ERx("\tAddress of FIELD structure:%p\r\n"), f);
	if (f==NULL || f->fltag == FTABLE)
		return;

	if (f->fldname!=NULL)
	{
		SIprintf(ERx("\t\tfldname:%s\r\n"), f->fldname);
	}
	SIprintf(ERx("\t\tflseq:%d\r\n"), f->flseq);

	if (f->flbufr!=NULL)
	{
		SIprintf(ERx("\t\tflbufr:%s\r\n"), f->flbufr);
	}

	if (f->fldefault!=NULL)
	{
		SIprintf(ERx("\t\tfldefault:%s\r\n"), f->fldefault);
	}

	SIprintf(ERx("\t\tfllength:%d\r\n"), f->fllength);
	SIprintf(ERx("\t\tflscr add:%p\r\n"), f->flscr);
	SIprintf(ERx("\t\tflposx:%d"), f->flposx);
	SIprintf(ERx("\t\tflposy:%d\r\n"), f->flposy);
	SIprintf(ERx("\t\tflmaxx:%d"), f->flmaxx);
	SIprintf(ERx("\t\tflmaxy:%d\r\n"), f->flmaxy);
	SIprintf(ERx("\t\tfltitle add:%p\r\n"), f->fltitle);

	if (f->fltitle!=NULL)
	{
		SIprintf(ERx("\t\tfltitle:%s\r\n"), f->fltitle);
	}

	SIprintf(ERx("\t\tfltitx:%d"), f->fltitx);
	SIprintf(ERx("\t\tfltity:%d\r\n"), f->fltity);
	SIprintf(ERx("\t\tflwidth:%d\r\n"), f->flwidth);
	SIprintf(ERx("\t\tfldatawin add:%p\r\n"), f->fldatawin);
	SIprintf(ERx("\t\tfldatax:%d"), f->fldatax);
	SIprintf(ERx("\t\tfldatay:%d\r\n"), f->fldatay);
	SIprintf(ERx("\t\tfldataln:%d\r\n"), f->fldataln);
	SIprintf(ERx("\t\tfldatatype:%c\r\n"), f->fldatatype);
	SIprintf(ERx("\t\tfldflags:%d"), f->fldflags);
	SIprintf(ERx("\t\tfloper:%d"), f->floper);
	SIprintf(ERx("\t\tflintrp:%d\r\n"), f->flintrp);
	SIprintf(ERx("\t\tflvalstr add:%p\r\n"), f->flvalstr);

	if (f->flvalstr!=NULL)
	{
		SIprintf(ERx("\t\tflvalstr:%s\r\n"), f->flvalstr);
	}

	SIprintf(ERx("\t\tflvalmsg add:%p\r\n"), f->flvalmsg);

	if (f->flvalmsg!=NULL)
	{
		SIprintf(ERx("\t\tflvalmsg:%s\r\n"), f->flvalmsg);
	}

	SIprintf(ERx("\t\tflfmtstr add:%p\r\n"), f->flfmtstr);

	if (f->flfmtstr!=NULL)
	{
		SIprintf(ERx("\t\tflfmtstr:%s\r\n"), f->flfmtstr);
	}

	SIflush(stdout);
	return;
}
