/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	scroll.h -	Define the data structure for scrolling fields
**
** Description:
**	This file defines:
**
**	SCROLL_DATA	Data structure for scrolling fields.
**
** History:
**	09-nov-87 (bruceb)	Initial implementation.
**	06-mar-90 (bruceb)
**		Added scr_fmt structure member; points to a FMT for the
**		full scrolling width of this field.  Initially NULL, set
**		by first call on IIFDssfSetScrollFmt().
**/

/*}
** Name:	SCROLL_DATA	- Scrolling field data structure
**
** Description:
**	Scrolling field data structure.  Pointed to by a field's
**	fvdatawin structure member for scrolling fields.
**
**	The scrolling field structure hangs off of a fldval's fvdatawin
**	pointer.  fvdatawin is being used by IBM for their own internal
**	purposes; since scrolling fields will not be available in the
**	synchronous world this overlap is not a problem.
**
**	All char *'s in the scroll structure point at a buffer whose size
**	is identical to the byte size underlying a scrolling field (as
**	specified in VIFRED), and which is dynamically allocated at the
**	same time as the display dbv.
**
**	+--------------------------------------------------+
**	|                    buffer                        |
**	+--------------------------------------------------+
**	^        ^                  ^                      ^
**	|        |                  |                      |
**	left     scr_start          cur_pos                right
**
**	left and right have the obvious meanings.  scr_start indicates the
**	character found in the first position of a field's window on the
**	terminal screen.  cur_pos indicates the character found at a field
**	window's cur[yx].  The buffer is always padded with blanks.
**
**	The buffer is left-to-right or right-to-left depending on the
**	reversed status of the field, so cur_pos could be to the left of
**	scr_start for a reversed field.
**
** History:
**	09-nov-87 (bab)	Initial description.
*/
typedef struct
{
	char	*left;		/* Point to left end of buffer */
	char	*right;		/* Point to right end of buffer */
	char	*scr_start;	/* Point to start of visible portion of buf */
	char	*cur_pos;	/* Point to location of cursor in buf */
	PTR	scr_fmt;	/* Point to full-width scroll FMT */
} SCROLL_DATA;

FUNC_EXTERN	SCROLL_DATA	*IIFTgssGetScrollStruct();
