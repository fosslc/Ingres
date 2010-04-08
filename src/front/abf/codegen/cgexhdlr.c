/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<ex.h>
#include	<lo.h>
#include	<si.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<er.h>
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

#include	"cggvars.h"
#include	"cgilrf.h"
#include	"cgerror.h"

/**
** Name:    cgexhdlr.c -	Code Generarator Exception Handler.
**
** Description:
**	Contains handler for the code generator:
**
**	IICGehExitHandler()	Handler called before exiting.
**
** History:
**	Revision 6.0  87/02  arthur
**	Initial revision.
*/


/*{
** Name:    IICGehExitHandler() -	Handler called before exiting
**
** Description:
**	A routine called by the interrupt handler whenever there is
**	about to be an exit from the code generator caused by an exception.
**	IICGexhExitHandler() calls the ILRF clean-up routine, and then passes
**	control on to the next outer handler in line, presumably FEhandler().
**
** Inputs:
**	ex	The exception structure.
**
** History:
**	20-feb-1987 (agh)
**		Written.  Equivalent to IIOehExitHandler() in interp/ilexhdlr.c.
**
**	3-jun-1987 (agh)
**		Call general front-end handler FEhandler().
*/
EX
IICGexhExitHandler (ex)
EX_ARGS		*ex;
{
    EX	FEhandler();

    IIORscSessClose(&IICGirbIrblk);
    return FEhandler(ex);
}
