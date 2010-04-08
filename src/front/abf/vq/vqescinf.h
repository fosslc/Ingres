/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	vqescinf.h -	Types used by Visual Query escape editing	
**
** Description:
**	Define types for VQ Escape editing
**
** History:
**	8/4/89 (Mike S)	Initial version
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*}
** Name:	ESCINFO		- 	Data passed to escape editing routines
**
** Description:
**	Data needed by escape editing routines
**
** History:
**	8/4/89 (Mike S) Initial version
*/
typedef struct
{
	METAFRAME	*metaframe;	/* Metaframe being editied */
	i4		esctype;	/* Escape type */
	char		*item;		/* Field or menuitem name */
	i4		line;		/* Line to scroll tablefield to */
	u_i2		flags;		/* flag bits */
	TAGID		frmtag;		/* Tag frame's form was allocated on */

/* Flags */
# define VQESC_MS 	0x1		/* Menuitems sorted */
# define VQESC_FL	0x2		/* Form loaded into memory */
# define VQESC_LF	0x8		/* Field listpick form created */	
# define VQESC_EL	0x10		/* Field Entry or exit tblfld loaded */
# define VQESC_CL	0x20		/* Field Change tblfld loaded */
# define VQESC_TF_MASK (VQESC_EL | VQESC_CL)
} ESCINFO;

/*}
** Name:        ESCTYPES        - Escape code types
**
** Description:
**      Static information which defines the escape code types.
**	Instances of this type will be indexed by the escape type.
**
** History:
**      8/10/89 (Mike S)        Initial version
*/
typedef struct
{
        u_i4   frame_mask;     /* Mask of frame types which use escape type */
        ER_MSGID esc_name;      /* ID for escape type name */
        ER_MSGID hdr_name;      /* header line for this escape type */
} ESCTYPE;

/*
** The following DEFINEs are used when calling the routine 
** IIVQgffGetFormFields().  When this routine is called with a positive
** number for the type of form field processing to perform, the routine
** knows that it is dealing with Escape Code editing and builds listpick
** forms appropriately.  When a negative number is passed to the routine,
** the listpick building is altered to show only form fields, exclude the
** 'ALL' field, and excluding the extra column that shows what fields have
** escape code defined.
*/

/* Fetch all fields (simple and tablefields) that are defined on the form */
# define VQ_FIELD_FETCH	-1
