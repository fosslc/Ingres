
/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	vqloc.h -	vq local definitions
**
** Description:
**	This file contains local definitions for visual query
**	and application flow diagram.
**
** History:
**	09/29/89 - (tom) created
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/* status bits for function returns */

# define VQ_CONT 0x01
# define VQ_DONE 0x02
# define VQ_SAVE 0x04
# define VQ_QUIT 0x08
# define VQ_NONE 0x10 
# define VQ_ERRM 0x20
# define VQ_ERRF 0x40


/*}
** Name:	FRMINFO	- frame type info struct
**
** Description:
**	Information about the frame types.
**
** History:
**	03/31/89 (tom) - created
*/
typedef struct
{
	char	*name;		/* name of the frame type */
	char	*desc;		/* description of the frame type */
	OOID	class;		/* class of the object */
	i4	tabtype;	/* table type for primary vq tables */
	i4	flags;		/* flags of the frame type */

# define HAS_MENU 0x01	/* frame type can have a menu attached to it */
# define HAS_VQ   0x02	/* frame type can have a menu attached to it */
# define ABF_TYPE 0x04	/* restricted frame type in Vision only product */
# define HAS_FORM 0x08	/* frame type has a form associated with it */
# define HAS_SRC  0x10	/* frame type has source file associated with it */
# define HAS_QBF  0x20	/* frame type has a qbf definition */
# define HAS_RBF  0x40	/* frame type has a rbf definition */
# define HAS_GRF  0x80	/* frame type has a graph definition */

} FRMINFO;



/*}
** Name:	EDINFO	- edit options info struct
**
** Description:
**	Information about the various objects that can be edited.
**	This stucture is used when a list must be put up for the user
**	to select what they want to edit, currently this occurs in
**	both the application flow diagram and the visual query.
**
** History:
**	10/01/89 (tom) - created
**	25-aug-92 (blaise)
**		Added local procedures option.
*/
typedef struct
{
	ER_MSGID mitem;		/* message number of menu item to display */
	i4	type;		/* type of edit this is */

# define EOP_VISQRY 1	/* visual query */
# define EOP_GENFRM 2	/* generated form */
# define EOP_MNUTXT 3	/* menu text */
# define EOP_FRPARM 4	/* frame parameters */
# define EOP_LOCVAR 5	/* local variables */
# define EOP_GLOBOB 6	/* global objects */
# define EOP_ESCCOD 7	/* escape code */
# define EOP_SRCFIL 8	/* source code file */
# define EOP_ERRORS 9	/* errors */
# define EOP_FRBEHA 10	/* frame behavior */
# define EOP_QBFDEF 11	/* qbf definition */
# define EOP_RBFDEF 12	/* rbf definition */
# define EOP_GRFDEF 13	/* graph definition */
# define EOP_LOCPRC 14	/* local procedures */

# define VQMAXEOPS  14	/* the maximum possible number of edit options */

	i4	flags;		/* these flags are and'ed with the flags in 
				   the FRM_INFO struct to see if this option 
				   is compatible with this frame type */
} EDINFO;


/*}
** Name:	FF_STATE	- state of frame flow diagraming 
**
** Description:
**	This structure is used to keep track of the current state
**	of the frame flow edit session, specifically, the stack of
**	frames currently being displayed is stored in this structure.
**
** History:
**	03/26/89 (tom) - created
*/
typedef struct
{
	OO_OBJECT *ao;		/* ptr to the current application component
				   in this row */
	char	disp;		/* index into the menuitems of the first 
				   display frame in this row */
	char	col;		/* position of the "current" frame in
				   this row relative to the display start..
				   this means that disp + col == the index 
				   into the menuitems of the current 
				   menuitem. */
} FF_STATE;



FUNC_EXTERN VOID IIVQer0();
FUNC_EXTERN VOID IIVQer1();
FUNC_EXTERN VOID IIVQer2();
FUNC_EXTERN VOID IIVQer3();
FUNC_EXTERN VOID IIVQer4();
