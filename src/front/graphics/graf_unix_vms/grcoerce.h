/*
**	Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	qrcoerce.h	-	definitions for GRcoerce_type
**
** History:	$Log - for RCS$
**	<automatically entered by code control>
**/

#define GRC_SLEN 80	/* character storage needed for conversion to string */

/*
** conversion from month / day / year to VE date format.
** We reject any date whose converted value is less than VE_MINDATE due
** to plotting problems.   Actually, all of 1901 except Jan. 1 works, but
** it makes it easier to document if we cut off at 1902.  Any value less
** than GRC_INTERVAL is an interval rather than an absolute date.  The
** interval could actually be read off in the appropriate parts of the integer.
*/
#define VE_DATE(M,D,Y)	((Y-1900)*((i4)10000)+M*100+D)
#define VE_MINDATE	VE_DATE(1,1,1902)
#define GRC_INTERVAL	VE_DATE(1,1,1582)

/*
** type to declare which represents enough storage for coercions which will
** be done by GRcoerce_type.  The intent is that target DB_DATA_VALUES to
** this routine will point at one of these.
**
** NOTE:  to be strictly correct, code will never actually use these
**	items by name.  They will coerce the address into the appropriate
**	type for the DB_DATA_VALUE.  The union is simply a mechanism for
**	declaring enough storage for all the types.
*/
typedef union
{
	f8	fstore;
	i4	istore;
	AFE_DCL_TXT_MACRO(GRC_SLEN)	sstore;
} GRC_STORE;
