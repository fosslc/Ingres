/*
** Copyright 1985, 2008 Ingres Corporation
*/
# include	<compat.h> 
# include	<efndef.h>
# include	<iledef.h>
# include	<iosbdef.h>
# include	<jpidef.h>
# include	<gl.h>
# include	<id.h> 
# include	<starlet.h>

/*{
** Name: IDuseric
**
** Description: 
**	set the passed in argument 'uic' to contain the user ic
**	of the user who is executing this process.
**
** Inputs: 
**	uic		user ic
**
** Outputs:
**
** History:
**		8/8/85 (ac) -- created. For VMS 4.* to get the process's
**			       group and member UIC.
**		
**      16-jul-93 (ed)
**	    added gl.h
**      17-oct-97 (rosga02)
**        added a null item terminator for getjpi() call item list
**        to avoid potential accvio
**	19-jul-2000 (kinte01)
**	   Correct prototype definitions by adding missing includes
**	02-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	24-oct-2008 (joea)
**	    Use EFN$C_ENF and IOSB when calling a synchronous system service.
*/

VOID
IDuseric(uic)
MID	*uic;
{
	ILE3	itmlst[2] = {{4, JPI$_UIC, uic, 0},{0,0,0,0}};
	IOSB	iosb;

	sys$getjpiw(EFN$C_ENF, 0, 0, &itmlst, &iosb, 0, 0);
}
