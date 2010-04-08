/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ooldef.h - Local/private counterpart of oodefine.h
**
** Description:
**	This header file contains declarations previously in
**	oodefine.h that are local to OO.
**
** History:
**	12/14/89 (dkh) - Initial version.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


GLOBALREF OO_OBJECT	*IIooObjects[];	/* objects present at start-up */
GLOBALREF OOID		TempId;		/* next available temporary Object ID
					** (grows downward.)
					*/
GLOBALREF OOID		KernelTempId;	/* Temp objects with id's above this
					** value are essential to the OO Kernel
					** and must not be deleted or
					** dynamically overwritten.  Programs
					** (like classout )which dynamically
					** redefine the class hierarchy or
					** directly manipulate the OO object
					** id space are safe re-assigning
					** temp id's only at or below this
					** value.
					*/

GLOBALREF i4		IIuserIndex;


/*
** externs -- class object structures directly declared extern
*/
GLOBALREF OO_OBJECT	iiOOnil[];
GLOBALREF OO_CLASS	O1[], O2[], O3[], O4[];
