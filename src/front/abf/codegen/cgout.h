/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	cgout.h -    Code Generator Output Module Interface Definitions.
**
** Description:
**	Contains the interface definition for the output module of code
**	generator.
**
** History:
**	Revision 6.3/03/01
**      09/12/91 (kevinm)
**              Changed iicgFormVar to a GLOBALREF instead of a GLOBALCONSTREF.
**              It is is declared as a globaldef in cginit.c.
**	03/04/91 (emerson)
**		Change GLOBALREF READONLY to GLOBALCONSTREF.
**		Also change GLOBALREF to GLOBALCONSTREF
**		in C Control Statement Names.
**
**	Revision 6.1  88/05/16  wong
**	Added shared strings.
**
**	Revision 6.0  87/06  arthur
**	Intial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

GLOBALCONSTREF
	char	iicgCharNull[],
		iicgShrtNull[],
		iicgEmptyStr[],
		iicgZero[],
		iicgOne[];
GLOBALREF
	char	iicgFormVar[];

#define _NullPtr	iicgCharNull
#define _ShrtNull	iicgShrtNull
#define _EmptyStr	iicgEmptyStr
#define	_Zero		iicgZero
#define _One		iicgOne
#define _FormVar	iicgFormVar

/*
** Globals used by outputting routines.
*/
extern i4	IICGotlTotLen;		/* length of the output line so far */
extern char	IICGoiaIndArray[];	/* array of tabs for indenting */


# define	CG_NOTLAST	0	/* not the last element in a list */
# define	CG_LAST		1	/* last element in a list */

# define	CG_NOINIT	0	/* variable not to be initialized */
# define	CG_INIT		1	/* variable being initialized */
# define	CG_ZEROINIT	2	/* array declared with [] */

/*
** Relational operators
*/
# define	CGRO_NOP	0
# define	CGRO_EQ		1
# define	CGRO_NE		2
# define	CGRO_LT		3
# define	CGRO_GT		4
# define	CGRO_LE		5
# define	CGRO_GE		6

/*
** Constants
*/
#define	CGBUFSIZE	256	/* size of buffers for strings, etc. */
#define	CGSHORTBUF	64	/* small buffer size */

/*
** C Control Statement Names.
*/
GLOBALCONSTREF char	_IICGif[];
GLOBALCONSTREF char	_IICGelseif[];
GLOBALCONSTREF char	_IICGfor[];
GLOBALCONSTREF char	_IICGwhile[];

/*{
** Name:    IICGoibIfBeg() -	Begin a C IF Statement.
**
** Description:
**	Generate C code to begin an IF statement.
*/
#define IICGoibIfBeg()	iicgControlBegin(_IICGif)

/*{
** Name:    IICGoeiElseIf() -	Begin a C ELSE IF Statement.
**
** Description:
**	Generate C code to begin the ELSE IF portion of a IF statement.
**
*/
#define IICGoeiElseIf() iicgControlBegin(_IICGelseif)

/*{
** Name:    IICGoikIfBlock() -	Begin the Statement list within an 'if' statement
**
** Description:
**	Generate code to begin the statement list within an 'if' statement.
**
** Inputs:
**	None.
**
*/
#define IICGoikIfBlock() iicgCntrlBlock()

/*{
** Name:    IICGoixIfEnd() -	End the Statement Block for a C IF Statement.
**
** Description:
**	Ends the statement block for a IF statement.
*/
#define IICGoixIfEnd() IICGobeBlockEnd()

/*{
** Name:    IICGofbForBeg() -	Begin a C FOR Statement.
**
** Description:
**	Generate C code to begin a FOR statement.
*/
#define IICGofbForBeg()	iicgControlBegin(_IICGfor)

/*{
** Name:    IICGofkForBlock() -	Begin the stmt block within a 'for' stmt
**
** Description:
**	Generates code to 
**	Begins the statement block within a 'for' statement.
*/
#define IICGofkForBlock()	iicgCntrlBlock()

/*{
** Name:    IICGofxForEnd() -	End the stmt block for a C FOR stmt
**
** Description:
**	Ends the statement block for a C FOR statement.
*/
#define IICGofxForEnd() IICGobeBlockEnd()

/*{
** Name:    IICGowbWhileBeg() -	Begin a 'while' loop
**
** Description:
**	Begins a C 'while' loop.
**
** Inputs:
**	None.
**
*/
#define IICGowbWhileBeg() iicgControlBegin(_IICGwhile)

/*{
** Name:    IICGowkWhileBlock() -	Begin the Statement Block For 'while' loop
**
** Description:
**	Begins the statement block within a 'while' loop.
**
** Inputs:
**	None.
**
*/
#define IICGowkWhileBlock() iicgCntrlBlock()

/*{
** Name:    IICGowxWhileEnd() -	End the Statement Block for a C WHILE Loop.
**
** Description:
**	Ends the statement block for a 'while' loop.
*/
#define IICGowxWhileEnd() IICGobeBlockEnd()
