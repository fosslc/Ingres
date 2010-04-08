/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	mqtf.h
**
** Description:
**	Define MQTF structure
**
** History:
**	3-mar-1990 (Mike S) 
**		Initial version.  This was moved to a separate file so
**		uf!frmfmt.c could include it wothout dragging in OO refrences.
**	
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**	MQTF -- intermediate data used while formatting a tablefield column.
*/
typedef struct mqtf
{
        char    *title;		/* Column title */
        char    *format;	/* Column format string */
        char    *name;		/* Column name */
        i4      length;		/* Column data length */
	i4	attno;		/* attribute index */
	i4	flags;		/* Column flags */
	i4	flags2;		/* Column flags */
} MQTF;
