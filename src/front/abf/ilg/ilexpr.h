/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

/**
** Name:    ilexpr.h -	OSL Interpreted Frame Object Generator
**				Internal Expression Description Definition.
** Description:
**	Contains the structure definition for the internal ILG expression
**	description passed between the expression and statement generation
**	modules of the ILG ("igexpr.c" and "igstmt.c".)
**
** History:
**	Revision 6.0  87/07  wong
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*}
** Name:    ILEXPR -	Intermediate Language Expression Description Structure.
**
** Description:
**	Defines the structure of the internal ILG expression description.
*/
typedef struct {
    IL		*il_code;	/* code for expression */
    i4		il_size;	/* size of code */
    i4		il_last;	/* offsets to last instruction */
} ILEXPR;

/* Function declaration */

ILEXPR	*IGendExpr();
