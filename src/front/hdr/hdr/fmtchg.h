/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	fmtchg.h -	Define the data structures for dynamically
**				changing field format structures.
**
** Description:
**	This file defines:
**
**	FMT_INFO	Data structure containing fmt information for a given
**			fmt on a given field.
**	FMT_MAX		Data structure containing maximum area covered by a
**			fmt for a given field.
**
** History:
**	02-feb-89 (bruceb)	Initial implementation.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*}
** Name:	FMT_INFO - Data structure containing basic fmt information.
**
** Description:
**	Contains information pertaining to the display format for a field.
**	Correspond to information in struct members of the same name in
**	the FLDTYPE struct.
**
** History:
**	02-feb-89 (bruceb)	Initial implementation.
*/
typedef struct fmt_info
{
    i4			ftwidth;
    i4			ftdataln;
    char		*ftfmtstr;
    FMT			*ftfmt;
    struct fmt_info	*next;
} FMT_INFO;

/*}
** Name:	FMT_MAX	- Data structure containing maximum area that may
**			  be convered by a fmt for a specific field.
**
** Description:
**	Contains the maximum number of rows and columns that may be
**	used by a display format for a given field.
**
** History:
**	02-feb-89 (bruceb)	Initial implementation.
*/
typedef struct
{
    i4			maxrows;
    i4			maxcols;
    struct fmt_info	*firstfmt;
} FMT_MAX;
