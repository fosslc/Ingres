/*
**Copyright (c) 1987, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<lo.h>
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
#include	<abqual.h>
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

/**
** Name:	ilpred.c	- Contains routine that build qualfication.
**
** Description:
**	This file defines:
**
**	IIITbqsBuilldQualStruct		Routine to build structure needed
**					by qualification function.
** History:
**	Revision 6.2  88/11/18  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*{
** Name:	IIITbqsBuildQualStruct - Build a structure for a qualification.
**
** Description:
**	This routine builds the structures needed to implement a qualification
**	function.  It is used by both ilquery and ildb to build the
**	ABRT_QUAL structure for a query.
**
**	The interface to this routine depends on the caller allocating
**	the memory to contain the ABRT_QUAL and any ABRT_QFLD structures
**	that the qualification function will need.  The routine is given
**	pointers to pointers for all its arguments.  It updates these
**	to point past the various arguments it has used up.  
**
**	This interface was designed for the needs of ilquery where in
**	an IL_QRY statement, it has several IL_QRYPRED statements at
**	the top of the file.  See the comments in IIObqBuildQryExec to
**	see how it wants to use memory.
**
**	After routine exits, the stmt pointer will be pointing at the
**	ENDLIST for the QRYPRED statement.
**
** IL Statements:
**
**	IL_QRYPRED intTypeOfPredicate in#OfElementsInPredicate
**	{
**	    IL_TL2ELM  strNameOfField	strDatabaseLeftHandSide
**	}
**	IL_ENDLIST
**
** Inputs:
**	tag		The tag to allocate any strings under.
**			If the tag is 0, then no dynamic memory is used.
**
** Outputs:
**	stmt		This is a pointer to a pointer to a IL_QRYPRED
**			statement.  The routine will build the RTS structures
**			needed for this IL_QRYPRED statement.  After the routine
**			ends, this argument will pointer (by indirection) to
**			the IL_ENDLIST statement for this IL_QRYPRED.
**
**	qual		A pointer to an array of ABRT_QUALs.  The ABRT_QUAL
**			currently pointed at will be filled in with the
**			the information.  The argument will be updated to
**			point at the next element in the array.
**
**	qflds		A pointer to a pointer to an array of ABRT_QFLDs.  This
**			routine will use as many of these as it needs.  It
**			assumes that there is enough.  The argument will be
**			updated to point at the next free element after
**			the ones that this routine has used.
**
** History:
**	18-nov-1988 (Joe)
**		First Written.
*/
STATUS
IIITbqsBuildQualStruct(tag, stmt, qual, qfld)
i2		tag;
IL		**stmt;
ABRT_QUAL	**qual;
ABRT_QFLD	**qfld;
{
    (*qual)->qu_type = (i4) ILgetOperand(*stmt, 1);
    if (tag == 0)
	(*qual)->qu_form = IIOFfnFormname();
    else
	(*qual)->qu_form = FEtsalloc(tag, IIOFfnFormname());
    (*qual)->qu_count = (i4) ILgetOperand(*stmt, 2);
    (*qual)->qu_elms = *qfld;
    (*qual)++;
    *stmt = IIOgiGetIl(*stmt);
    if (**stmt != IL_TL2ELM)
    {
	IIOerError(ILE_STMT, ILE_ILMISSING,
		   ERx("TL2ELM in IIITbqsBuildQueryStruct"),
		   (char *) NULL);
	return FAIL;
    }
    do
    {
	(*qfld)->qf_field = ILcharVal(ILgetOperand(*stmt, 1), NULL);
	(*qfld)->qf_expr = ILcharVal(ILgetOperand(*stmt, 2), NULL);
	if (tag != 0)
	{
	    (*qfld)->qf_field = FEtsalloc(tag, (*qfld)->qf_field);
	    (*qfld)->qf_expr = FEtsalloc(tag, (*qfld)->qf_expr);
	}
	(*qfld)++;
	*stmt = IIOgiGetIl(*stmt);
    } while (**stmt == IL_TL2ELM);
    if (**stmt != IL_ENDLIST)
    {
	IIOerError(ILE_STMT, ILE_ILMISSING,
		   ERx("IL_ENDLIST in IIITbqsBuildQueryStruct"),
		   (char *) NULL);
	return FAIL;
    }
    return OK;
}
