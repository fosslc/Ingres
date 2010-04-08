
/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	frminfo.h -	vq frame formatting information
**
** Description:
**	This file contains structure definitions for 
**	visual query frame formatting.
**
** History:
**	05/08/89 - extracted from vqmakfrm.qsc 
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


# define MAXCOLTYPES 5  /* maximum column types in a visual query
			   table field. */


/*}
** Name:	TCINFO	- table column formatting info
**
** Description:
**	This structure holds formatting information on the various table
**	columns for the table types which are part of a visual query.
**
** History:
**	3/1/89  (tom) - created
*/
typedef struct
{
	char	*name;		/* name of column */
	i2	width;		/* width of column on screen */
	i2	dlen;		/* max string length of column data */
	i4	flags;		/* flags for the field */

} TCINFO;


/*}
** Name:	TABINFO	- formatting info on the various table types
**
** Description:
**	This structure holds formatting information on the various table
**	types which are part of a visual query.
**
**	The only instance of this structure is an initialized
**	data structure containing formatting info on the
**	various table types. The text fields are then filled in
**	from the message file.
**
** History:
**	2/16/89  (tom) - created
*/
typedef struct
{
	char	*name;		/* name of table, from msg file */
	i4	numcols;		/* number of columns in the table */
	TCINFO	cols[MAXCOLTYPES];	/* array of structures defining
					   the formatting of the columns */
} TABINFO;


GLOBALREF TABINFO tab_info[];
