/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

/**
** Name:	iamtbl.h - drive table definition for encoding/decoding
**
** Description:
**
** Drive table structure for coding oo strings.  Used to define a table
** for the items of the structure being coded.  What this is aimed at handling
** is a general structure consisting of one top-level object with variable
** length array items encoded as succesive sub-objects in the string.  The
** convention followed in the top level object is that an array is given by
** a pointer followed by a number-of-elements entry.
**
** Because the oo format has negative object id's pointing to objects which
** are encoded in the string AFTER we have completed the top-level object,
** we need to save id's and array information to read / write in one pass.
** These two things are included in this table, to be filled in during the
** encode / decode process.
**
** The "fixed" items are the control information which form a template matching
** the structure being decoded / encoded.
**
** This structure, together with the token-handling interface allows us to
** handle the decoding entirely inside a retrieve loop which obtains one
** buffer-full of string at a time.
**
** Encoding:
**
**	The table is walked through.  If an item is type AOO_array, the wproc
**	procedure is passed the array address, and the number of elements
**	It writes the string for encoding the array, using db_puts and
**	db_printf.  The string does not include the leading id, and the
**	curly braces, which are handled by the caller.
**
** Decoding:
**
**	The table is walked through while calling the token routines to decode.
**	The aproc procedure is called with arguments identical to FEtalloc to
**	allocate space for array items.  The number of elements is argument
**	2, and the element size is argument 3.  This is often reversed in
**	calls to FEtalloc, since it really doesn't matter which is which.
**	It may matter for a specialized allocation procedure.
**
**	After scan of the leading portion of the string is finished, the rest
**	of the string is scanned.  As leading object id's are encountered, the
**	table is searched for the id, recorded when discovered in the top-level
**	object, and the decode procedure is called.  The decode procedure is
**	passed the allocated array pointer, a pointer pointer, and the number
**	of elements.
**
**	The procedure repeatedly calls tok_next to parse the portion of the
**	string from the first array element to close curly.  The procedure
**	returns 0 for completion, negative for syntax error, and positive to
**	indicate end-of-string.  In that case, a pointer to the position to
**	resume decoding at is returned in the double pointer argument.  It gets
**	re-called after more string has been read from the database.  The number
**	of elements argument is zero on recalls.  This allows the procedure to
**	handle any needed initialization, such as state synchronization,
**	since "end-of-string" can happen anywhere.
**
**      It's possible to handle multiple versions of a structure.
**      To do this, the following guidelines must be adhered to:
**      (1) All versions of the structure must have a version number
**          of type i4 (type AOO_i4 in the corresponding drive table entry).
**      (2) The portion of the structure preceding this version number
**          must be identical in all versions, and must contain no arrays
**          (type AOO_array in the corresponding drive table entry).
**      (3) The vproc member of the drive table entry for the version number
**          must specify a procedure to analyze the version number.
**          This procedure takes the following arguments:
**          (a) The version number.
**          (b) A pointer to the address of the drive table.
**          (c) A pointer to the number (i4) of entries in the drive table.
**          The procedure may substitute a different drive table
**          by modifying the contents of (b) and (c), or it may (in effect)
**          discard entries from the end of the current drive table
**          by modifying the contents of (c) (to a smaller value).
**          [Note: the drive table entry for any i4 that's not a version number
**          must specify a no-op routine for vproc.]
**
** History:
**      22-jun-92 (davel)
**              Changed to allow decoding multiple versions of a structure:
**              Added vproc member to CODETABLE structure, and added
**              description of how to handle multiple versions.
**              Also corrected description of how re-calls of a decode
**              procedure are identified.  Note: these changes patterned
**		after changes developed by Emerson for W4GL.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

typedef struct
{
	/* these items are fixed */
	i4	type;		/* type of item */
	i4	size;		/* per-element size of array items */
	PTR	(*aproc)();	/* allocation procedure for array items */
	i4	(*dproc)();	/* decode procedure for array items */
	STATUS	(*wproc)();	/* write procedure for array items */
	VOID    (*vproc)();     /* procedure to analyze a version number
				** (relevant only for i4 items)
				*/
	/* these items are set during the encoding / decoding process */
	i4	*addr;		/* array address */
	i4	numel;		/* number of elements for array items */
	i4	id;		/* object id for array items */
} CODETABLE;

/*
** type item encodes
*/
#define AOO_f8 1
#define AOO_i4 2
#define AOO_array 3

/*
** size items
*/
#define ESTRLEN 1990	/* size of text field for encoded string */
#define TMAXLEN 2400	/* maximum token size */
