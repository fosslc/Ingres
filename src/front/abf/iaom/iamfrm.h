
/*
**	Copyright (c) 1986, 2004 Ingres Corporation
*/

/**
** Name:	iamfrm.h - AOM internal frame structure
**
** Description:
**	IAOM local structure to represent a frame.   This corresponds to
**	the object manager template.  It is mapped to an IFRMOBJ before
**	being handed off to the outside world.
**
**	THIS IS THE DB TEMPLATE!!!!!
**
**	VERSION NOTE:
**
**	The FE version number appears as the first item of this structure,
**	so that we can manage to switch decoding tables based on version
**	number if we ever have to.
**
**	The "align" array is fixed length, and we relate its items to
**	the individual items of the frame structure afterwards.  This
**	setup uses the already available integer constant decoding routine
**	to simply obtain an array of numbers - we can add a few later on
**	without modifying the encoding / decoding tables, although you
**	could eventually tack on too many things having nothing to do
**	with alignment.
**
**	RESTRICTIONS:
**
**		ONLY pointers and i4's may be members, assuring no item padding.
**
**		Array pointers must appear immediately preceding their i4 size
**		items.  These pairs of items will correspond to -A's in the oo
**		encoded strings.
**
**		Structure MUST match the tables given in iam[ed]tbl.c
**		which controls decoding / encoding of objects.
** History:
**	??? (???)
**		Initial version.
**
**	Revision 6.5
**	18-jun-92 (davel)
**		added array members for Decimal constants.
*/

typedef struct
{
	i4 feversion;		/* fe version number */
	i4 stacksize;		/* stack size */
	i4 *align;		/* ADF versions & alignment info */
	i4 num_align;		/* number of elements in align */
	IL_DB_VDESC *indir;	/* stack indirection section */
	i4 num_indir;		/* number of indir elements */
	IL *il;			/* intermediate language */
	i4 num_il;		/* number of il elements */
	FDESCV2 *symtab;	/* symbol table */
	i4 num_sym;		/* symbol table elements */
	char **c_const;		/* character constants */
	i4 num_cc;		/* number of c_const elements */
	char **f_const;		/* floating constants */
	i4 num_fc;		/* number of f_const elements */
	i4 *i_const;		/* integer constants */
	i4 num_ic;		/* number of i_const elements */
	DB_TEXT_STRING **x_const;/* hexadecimal constants */
	i4 num_xc;		/* number of x_const elements */
	char **d_const;         /* decimal constants */
	i4 num_dc;              /* number of d_const elements */
} AOMFRAME;
