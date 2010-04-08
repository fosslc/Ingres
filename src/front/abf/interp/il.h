/*
**Copyright (c) 1986, 2004 Ingres Corporation
** All rights reserved.
*/

/**
** Name:	IL.h
**	Header file with definitions used by the OSL interpreter.
**	See also cgil.h in codegen.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL):
**		Add macro ILNULLSID to support long SIDs
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* extern declarations */
VOID	IIOerError();
VOID	IIOpeProErr();
VOID	IIOexExitInterp();
VOID	IIOslSkipIl();
VOID	IIOdeDbErr();
VOID	IIOtop();
PTR	IIOgvGetVal();
PTR	IIOrfRunFrame();
PTR	IIOrpRunProc();

DB_DATA_VALUE	*IIOFdgDbdvGet();

VOID	IIITsniSetNextIl();
VOID	IIITeilExecuteIL();
VOID	IIITiteIlTabEdit();
VOID	IIITtrmTestRunMode();

# define	IL_BUFSIZE	256

/*
** Macro which returns TRUE if a VALUE is NULL.
*/
# define ILNULLVAL(val)		((val) == 0)

/*::
** Name:	ILNULLSID() -   Return TRUE if a SID is null.
**
** Description:
**	Returns TRUE if the SID is null, FALSE otherwise.
**	(A SID represents the offset of a new IL statement,
**	relative to the current statement.  A null SID indicates
**	that the "new" statement is simply the current statement).
**	
**	THIS IS A MACRO
**
** Inputs:
**	stmt		A pointer to the opcode of a IL statement.
**	sid		An IL operand from that statement, representing a SID.
**
** Outputs:
**	Returns:
**		TRUE	if the SID is null.
**		FALSE   otherwise.
** History:
**	01/09/91 (emerson)
**		First Written
**
** Discussion:
**	This macro was developed as part of support for long SIDs.
**	Currently, we don't need to look at (*stmt & ILM_LONGSID),
**	because if ILM_LONGSID is set, then operand 2 (sid) is the negation of
**	the 1-relative index of an integer constant, and thus cannot be zero.
**	But if it ever becomes possible for an IL operand of 0 to represent
**	an integer constant, then we'll have to check (*stmt & ILM_LONGSID):
**	If ILM_LONGSID is set, then the SID can't be null, even if ((sid) == 0).
*/
# define ILNULLSID(stmt, sid)	((sid) == 0)

/*::
** Name:	ILcharVal() -	Return Character String Value or Default Value.
*/
# define ILcharVal(val, default)	((ILNULLVAL(val)) ? (default) : ((char *) IIOgvGetVal((val), DB_CHR_TYPE)))

/*::
** Name:	ILisConst() -   Return TRUE if the value is a constant.
**
** Description:
**	Returns TRUE if the value is a constant, FALSE otherwise.
**	
**	THIS IS A MACRO
**
** Inputs:
**	val		An IL operand.
**
** Outputs:
**	Returns:
**		TRUE	if the value is a constant
**		FALSE   otherwise.
** History:
** 	26-oct-1988 (Joe)
**		First Written
*/
# define ILisConst(val)		(((i4) (val)) < 0)

/*
** Structure with information on jump tables for current frame.
** In each jump table, the zero-th element is a pointer to the next
** table in the list.  The index for all other elements comes from
** the value of IIretval().
**
** The ilj_offsets field will really be a larger array than 1 element.
** The field ilj_numele shows how many are really allocated.  Note that
** later it might make sense to add a count that shows how many are actually
** used, since it can be less if different kinds of activations are
** attached to the same operation.
**
** History:
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
*/
typedef struct iljump {
	struct iljump	*ilj_next;	/* Next jump table for frame */
	i4	ilj_numele;	/* Number of elements in ilj_offsets */
	i4	ilj_offsets[1]; /* Array of offsets */
} ILJUMP;
