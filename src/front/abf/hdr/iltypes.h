/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#ifndef ILTYPES_H_INCLUDED
#define ILTYPES_H_INCLUDED

/**
** Name:	iltypes.h -	OSL Interpreted Frame Object
**				    Intermediate Language Types Definition File.
** Description:
**	Contains the type definitions for types used in referencing the
**	intermediate language for an OSL interpreted frame object.
**
** History:
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Changed IGSID to be PTR, and added some function declarations
**		(for bug 34846).
**
**	Revision 6.4/02
**	11/07/91 (emerson)
**		Created macro ILsetOperand; removed declarations
**		for deleted functions iiIGgetOffset and iiIGupdStmt
**		(for bugs 39581, 41013, and 41014).
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Changed comments pertaining to MAX_IL: with the advent
**		of support for long SIDs, MAX_IL no longer represents
**		the maximum SID.
**
**	Revision 6.2  89/07  wong
**	Changed IGSID to be i4 for robustness; maximum is still limited
**	to MAX_IL, which was also added.  JupBug #6367.
**
**	Revision 6.0  87/02  wong
**	Added statement label type enumerations.
**
**	Revision 5.1  86/10/17  16:34:07  wong
**	Added types for IL operator look-up table.  (86/09/17)  wong.
**	Initial revision.  (86/09/12)  wong.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       4-Jan-2007 (hanal04) Bug 118967
**          IL_EXPR and IL_LEXPR now make use of spare operands in order
**          to store an i4 size and i4 offset. Added ILgetI4Operand.
**      25-Mar-2008 (hanal04) Bug 118967
**          Adjust ILgetI4Operand and moved it to ilpackge.c
**	25-Aug-2009 (kschendel) b121804
**	    Add multi-inclusion protection.
*/

typedef i2	IL;
typedef i2	ILOP;
typedef i2	ILREF;
typedef i2	ILALLOC;

typedef PTR	IGSID;		/* points to a structure private to igstmt.c */

/*
** Name:	MAX_2IL -	Maximum int value from 2 elements of type IL.
*/
#define MAX_2IL	MAXI4

IGSID	*IGinitSID();
VOID	IGsetSID();
IGSID	*IGpopSID();
IGSID	*IGtopSID();
VOID	IGpushSID();

VOID	iiIGstmtInit();
VOID	iiIGofOpenFragment();
PTR	iiIGcfCloseFragment();
VOID	iiIGrfReopenFragment();
VOID	iiIGifIncludeFragment();
VOID	IGstartStmt();
VOID	IGgenStmt();
VOID	IGgenExpr();
VOID	IGgenRefs();
IL	*iiIGnsaNextStmtAddr();

ILREF   IGsetConst();
VOID    IGgenConsts();
i4	ILgetI4Operand();
VOID    IGoutStmts();

char	*iiIG_string();
char	*iiIG_vstr();

/*}
** Name:	IL_OPTAB -	Intermediate Language Operator Look-up Table
**					Type Definition.
** Description:
**	This defines the type of the entries for the IL operator look-up table.
*/
typedef struct {
	i4	il_token;	/* tag */
	char	*il_name;	/* keyword */
	i4	il_stmtsize;	/* size in i2's of this IL statement */
	i4	(*il_func)();	/* routine to execute */
} IL_OPTAB;

/*}
** Name:	IL_DB_VDESC - IL DBDV description
**
** Description:
**	This is actually just a DB_DATA_VALUE with an offset instead of
**	a pointer.  Used to keep track of relocatable DB_DATA_VALUE's.
*/
typedef struct {
	i4		db_offset;
	i4		db_length;
	DB_DT_ID	db_datatype;
	i2		db_prec;
} IL_DB_VDESC;

/*::
** Name:	ILgetOperand() -	Return Operand of an IL Statement.
**
** An IL statement is simply a series of i2's.
**
** Inputs to this macro:
**	IL	*stmt;	** a pointer to the beginning of the IL statement **
**	i4	num;	** the relative number of the operand **
**
** Returns:
**	an IL (that is, an i2)
*/
# define ILgetOperand(stmt, num)	((IL) *((stmt) + (num)))

/*::
** Name:	ILsetOperand() -	Set Operand of an IL Statement.
**
** Inputs to this macro:
**	IL	*stmt;	** a pointer to the beginning of the IL statement **
**	i4	num;	** the relative number of the operand **
**	IL	val;	** the value to which the operand is to be set **
**
** Returns:
**	val (usually discarded).
**
** History:
**	11/07/91 (emerson)
**		Created (for bugs 39581, 41013, and 41014).
*/
# define ILsetOperand(stmt, num, val)	((stmt)[num] = (val))

/*
** Maximum size of IL string.
*/
# define IL_MAX_STRING	2048

/*
** Name:	ILLABELS -	Labelled Statement Header Type Enumerations.
**
*/
#define		IL_LB_NONE 0	/* no label */
#define	 	IL_LB_MENU 1	/* menu label */
#define		IL_LB_WHILE 2	/* while label */
#define		IL_LB_UNLD 3	/* unloadtable label */

#endif /* ILTYPES_H_INCLUDED */
