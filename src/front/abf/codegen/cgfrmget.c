/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<lo.h>
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<iltypes.h>
#include	<ilrffrm.h>
/* ILRF Block definition. */
#include	<abfrts.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>
#include        <er.h>

#include	"cgerror.h"
#include	"cggvars.h"
#include	"cgilrf.h"

/**
** Name:    cgfrmget.c -	Code Generator Get interpreted frame object
**
** Description:
**	Gets an interpreted frame object.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Use IIORfgFrmGet instead of the now-obsolete IIORfnFrmNoptrz.
**		(IIORfgFrmGet no longer does any "pointerization").
**
**	Revision 6.0  87/02  arthur
**	Initial revision.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
*/

/*{
** Name:    IICGfrgFrmGet() -	Get an interpreted frame object
**
** Description:
**	Gets an interpreted frame object, including its IL and
**	pseudo-symbol table (FDESC array).
**
** Inputs:
**	name	Name of the frame.
**	id	The frame identifier.
**
*/
VOID
IICGfrgFrmGet(name, id)
char		*name;
i4		id;
{
    FID	fid;

    fid.name = name;
    fid.id = id;
    if (IIORfgFrmGet(&IICGirbIrblk, &fid, &CG_frminfo, (bool)FALSE) != OK)
	IICGerrError(CG_OBJBAD, name, (char *) NULL);

    return;
}
