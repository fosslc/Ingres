/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/*
** Name:	abqual.h -	ABF RunTime Qualification Interface Definitions.
**
** Description:
**	Contains the declarations of the structures used in implementing
**	4GL qualification functions.  This file is referenced by the 4GL
**	translator, the code generator, and the ABF runtime system.
**
** History:
**	Revision 6.0  87/06/15  arthur
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*}
** Name:	ABRT_QFLD -	Associate Field with Column Attribute.
**
** Description:
**	Specifies an association between a DBMS column attribute and a field.
**	If the field contains a query, then this is the column attribute to
**	use with the query value.  An array of ABRT_QFLDs must always end with
**	an element containing NULL members.
**
**	For example:
**		ABRT_QFLD {"R.column", "field"}
**
**		The field 'fld' contains the following value: >10 or <2
**
**		The generated where clause is: R.column > 10 or R.column < 2
**
**	The ABRT_QUAL structure will contain an element that references the
**	array of ABRT_QFLD elements for a qualification.
**
** History:
**	15-jun-1987 (agh)
**		First written.
*/
typedef struct
{
    char	*qf_field;	/* The field name */
    char	*qf_expr;	/* The database expression (as a string) */
} ABRT_QFLD;

/*}
** Name:	ABRT_QUAL	-	Information on qualification function
**
** Description:
**	This structure is used to build a query from the fields of a form
**	when, for example, a user's OSL code contains a qualification
**	function.
**
** History:
**	15-jun-1987 (agh)
**		First written.
*/
typedef struct
{
    i4		qu_type;	/* Type of structure this is. */
#define	ABQ_FLD		0x01		/* qu_elms is array of ABRT_QFLDs */
    char	*qu_form;	/* The name of the form to query on. */
    i4		qu_count;	/* Number of elements in following array. */
    ABRT_QFLD	*qu_elms;	/* Array of sub-structures (e.g., ABRT_QFLDS) */
} ABRT_QUAL;
