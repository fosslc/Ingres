/*
**Copyright (c) 1986, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<tr.h>		/* 6-x_PC_80x86 */
#include	<lo.h>
#include	<nm.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<iltypes.h>
#include	<ilops.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>
#include	<ilrffrm.h>
#include	<ilerror.h>
#include	"il.h"
#include	"if.h"
#include	"ilgvars.h"
#include	"iltrace.h"

/**
** Name:	ifdump.c -	Dump routines for interpreter
**
** Description:
**	Dump routines for interpreter.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Add #undef IIOtxDebug to turn these debugging routines into
**		no-ops.  Further work will be required to make them useful again
**
**	Revision 5.1  86/06/19  arthurh
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# undef IIOtxDebug

/*{
** Name:	IIOdnDumpNoop	-	Dummy routine to pull in ildump module
**
** Description:
**	This is a dummy routine intended to pull in this module
**	which, on the PC, is in an overlay.
**	This call is required since the entry point to this module,
**	IIOduDumpExec(), is actually called through a table at runtime.
**
** History:
**	6-nov-1986	Written (agh).
*/
IIOdnDumpNoop()
{
	return;
}

/*{
** Name:	IIOduDumpExec	-	Execute IL 'dump' statement
**
** Description:
**	Executes the IL 'dump' statement.
**	This statement is used internally to aid in testing
**	and debugging modules of the OSL interpreter.
**
** IL statement:
**	DUMP INT VALUE
**
**	The INT (integer constant) is the constant for the type
**	of dump desired (for example, of the contents of the frame
**	stack).  The constants are defined in iltrace.h.
**	The VALUE is a character string providing information about
**	the dump.
**
** Inputs:
**	stmt	The IL statement to execute.
**
*/
IIOduDumpExec(stmt)
IL	*stmt;
{
	if (ILgetOperand(stmt, 1) == IIOTRSTK)		/* dump frame stack */
	{
# ifdef IIOtxDebug
# ifdef IIOtxStk
		if (TRgettrace(IIOTRSTK, 0))
		{
			IIOsdStkDmp(IIOframeptr,
			    IIOgvGetVal(ILgetOperand(stmt, 2), DB_CHR_TYPE));
		}
# endif /* IIOtxStk */
# endif /* IIOtxDebug */
	}
}


# ifdef IIOtxDebug
# ifdef IIOtxStk

/*{
** Name:	IIOsdStkDmp	-	Dump the frame stack
**
** Description:
**	Dump the frame stack.
**
** Inputs:
**	stk	The frame stack.
**	str	A string to identify this dump.
**
** Outputs:
**	Errors
**		ILE_TRFILE 	-	Can't open trace file.
**
** History:
**	19-jun-1986 (agh)
**		First written
*/
IIOsdStkDmp(stk, str)
IFCONTROL	*stk;
char		*str;
{
	char		*cp;
	FILE		*fp;
	IFCONTROL	*frm;
	LOCATION	tloc;
	LOCATION	loc;
	char		locbuf[MAX_LOC];

	/*
	** Open a temp file to use.
	*/
	if ( NMloc(TEMP, PATH, (char *) NULL, &tloc) != OK  ||
		(LOcopy(&tloc, locbuf, &loc),
			LOuniq(ERx("osl"), ERx("dmp"), &tloc)) != OK  ||
				SIopen(&loc, ERx("a"), &fp) != OK )
	{
		LOtos(&loc, &cp);
		IIOerError(ILE_STMT, ILE_TRFILE, cp, (char *) NULL);
		return;
	}

	SIfprintf(fp, ERx("Snapshot of Stack at `%s':\n"), str);
	if (stk == NULL)
	{
		SIfprintf(fp, ERx("Empty stack\n"));
		return;
	}

	/*
	** Dump all the frames on the stack, starting at the current frame.
	*/
	for (frm = stk; frm != NULL; frm = frm->if_nextfrm)
	{
		IIOfdFrmDmp(frm, fp);
	}

	SIclose(fp);
	return;
}

/*{
** Name:	IIOfdFrmDmp	-	Dump stack frame
**
** Description:
**	Dump a stack frame.
**
** Inputs:
**	frm	A frame on the frame stack.
**
** History:
**	19-jun-1986 (agh)
**		First written
*/
IIOfdFrmDmp(frm, fp)
IFCONTROL	*frm;
FILE		*fp;
{
	PTR		ptr;
	DB_DATA_VALUE	*base;
	i4		i = 0;

	SIfprintf(fp, ERx("FRAME:\n"));
	SIfprintf(fp, ERx("Address of stack frame: %x\n"), (i4) frm);
	SIfprintf(fp, ERx("Control Section:\n"));
	SIfprintf(fp, ERx("\tif_retaddr.fid.name: %s\n"),
			frm->if_retaddr.fid.name);
	SIfprintf(fp, ERx("\tif_retaddr.fid.id: %d\n"),
			frm->if_retaddr.fid.id);
	SIfprintf(fp, ERx("\tif_retaddr.magic: %d\n"),
			frm->if_retaddr.magic);
	SIfprintf(fp, ERx("\tif_retaddr.statement: %x\n"),
			frm->if_retaddr.statement);
	SIfprintf(fp, ERx("\tif_jump: %x\n"), frm->if_jump);
	SIfprintf(fp, ERx("\tif_framename: %s\n"), frm->if_framename);
	SIfprintf(fp, ERx("\tif_formname: %s\n"), frm->if_formname);
	SIfprintf(fp, ERx("\tif_fdesc: %x\n"), frm->if_fdesc);
	SIfprintf(fp, ERx("\tif_param: %x\n"), frm->if_param);
	SIfprintf(fp, ERx("\tif_nextfrm: %x\n\n"), frm->if_nextfrm);

	base = frm->if_data;
	SIfprintf(fp, ERx("Base Array and Data:\n"));
	/*
	** Dump the actual elements in the base array, and the data
	** they point to.
	*/
	for (i = 0; IIORcdCheckDbdv(&il_irblk, i) == OK; i++, base++)
	{
		SIfprintf(fp, ERx("\tType: %d\n\tLength: %d\n"), base->db_datatype,
				base->db_length);
		SIfprintf(fp, ERx("\tAddress of data: %x\n\tValue of data: "),
				(i4) base->db_data);
		IIOddDataDump(base->db_datatype, base->db_data, fp);
		SIfprintf(fp, ERx("\n\n"));
	}

	return;
}

/*{
** Name:	IIOddDataDump	-	Dump a data value in a stack frame
**
** Description:
**	Dump a data value in a stack frame.
**
** Inputs:
**	type	Data type.
**	value	Data value.
**	fp	File to write to.
**
** History:
**	19-jun-1986 (agh)
**		First written
*/
IIOddDataDump(type, value, fp)
i4	type;
PTR	value;
FILE	*fp;
{
	switch (type)
	{
	  case DB_CHR_TYPE:
		SIfprintf(fp, ERx("%s"), value);
		break;

	  case DB_FLT_TYPE:
		SIfprintf(fp, ERx("%f"), *((f8 *) value));
		break;

	  case DB_INT_TYPE:
		SIfprintf(fp, ERx("%d"), *((i4 *) value));
		break;

	  case DB_DYC_TYPE:
		SIfprintf(fp, ERx("dynamic string"));
		break;

	  case DB_QUE_TYPE:
		SIfprintf(fp, ERx("QRY structure"));
		break;

	  default:
		SIfprintf(fp, ERx("bad type"));
		break;
	}
	return;
}

# endif /* IIOtxStk */
# endif /* IIOtxDebug */
