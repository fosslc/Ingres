/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	cglabel.h - Code Generator Label Module Interface Definition.
**
** Description:
**	Contains the definition of the interface to label module of the OSL
**	code generator.  This module is implemented in "cglabel.c".
**
** History:
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL):
**		Add new function IICGesEvalSid.
**	Revision 6.1  88/09  wong
**	Added CGLM_FORM0 for RETFRM and EXIT.  Also, moved structure
**	typedefs and local defines to "cglabel.c".
**
**	Revision 6.0  87/06  arthur
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name:	LBL_MODE -	Modes of labels.
**
**	Three possible modes exists -- for the base form, and for
**	forms and activations.
*/

#define		CGLM_FORM0	(-1)	/* base form display */
#define		CGLM_FORM	0	/* form display */
#define		CGLM_ACTIV	1	/* activation */

/*
** Name:	LBL_TYPE -	Label Types.
**
**	Label types used as indexes into a string table.  These types are passed
**	into the label generation routines and apply also to the 'stk_mode'
**	member of the CGL_STACK structure.
*/
# define	CGL_0		0

# define 	CGL_FDINIT 	1
# define 	CGL_FDBEG 	2
# define 	CGL_FDFINAL 	3
# define 	CGL_FDEND 	4

# define 	CGL_MUINIT 	5

# define 	CGL_OSL 	6

/*
** Name:	LBL_END -	Label End Mode.
**
**	The following modes apply to the 'cgl_mode' member of the
**	CGL_LBL_STRUCT structure, and are passed into 'IICGlitLabelInsert().'
*/
# define	CGLM_NOBLK	0
# define	CGLM_BLKEND	1
# define	CGLM_MDBLKEND	2

/*
** Function Declarations.
*/
VOID	IICGlbiLblInit();
VOID	IICGlebLblEnterBlk();
VOID	IICGlarLblActReset();
VOID	IICGlexLblExitBlk();
i4	IICGlnxLblNext(/* LBL_MODE */);
VOID	IICGpxlPrefixLabel(/* LBL_TYPE, LBL_MODE */);
VOID	IICGpxgPrefixGoto(/* LBL_TYPE, LBL_MODE */);
VOID	IICGlblLabelGen(/* IL * */);
VOID	IICGgtsGotoStmt(/* IL *, i4  */);
VOID	IICGlitLabelInsert(/* IL *, LBL_END */);
bool	IICGlrqLabelRequired(/* IL * */);
IL	*IICGesEvalSid(/* IL *, IL */);
