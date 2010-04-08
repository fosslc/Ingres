/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	rts.h - ABF RTS Local Definitions File.
**
** History:
**	Revision 6.3/03/00  90/06  wong
**	Removed #include's and changed to be definitions file.
**	24-Aug-2009 (kschendel) 121804
**	    Add a few function prototypes for gcc 4.3.
*/

/*}
** Name:	AB_TYPENAME -	ABF RTS Type Name Buffer.
**
** Description:
**	Buffer large enough to hold largest type name, which is
**
**		array of table field <name>.<name>
*/
typedef char	AB_TYPENAME[22+2*FE_MAXNAME+1];

/* Function prototypes */
FUNC_EXTERN void	iiarCcnClassName(DB_DATA_VALUE *,AB_TYPENAME,bool);
FUNC_EXTERN DB_DT_ID	iiarDbvType(i4);
FUNC_EXTERN bool	iiarIarIsArray( DB_DATA_VALUE * );
FUNC_EXTERN char *	iiarTypeName(DB_DATA_VALUE *, AB_TYPENAME);
