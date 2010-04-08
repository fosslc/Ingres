/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	feds.h -	Front-End Data Structure Definitions File.
**
**  These constants used to be defined in "feds.h".  Put here due
**  to nesting problems for includes on DEC-C. (dkh)
**
** History:
**	xx-xxx-xx (xxx)
**		Initial version.
**	14-apr-93 (davel)
**		Added DS_OIDCONS for ABF/4GL decimal constants.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**	Data structure Id's
*/

/*
**	Data structure (template) identfiers (dsid's)
**	are integers small enough to be represented as i2.
**	Dsid's for standard system-wide data structures are assigned as
**	#defines in clhdr file DS.h.  These data structures include
**	standard basic data types such as i2, i4, (char *), f8, etc.,
**	as well as common data structures used in interfacing to
**	the CL such as LOCATIONs.  Dsid's in DS.h are assigned in
**	the range 0-999.
**	
**	Front-end data structure id's are assigned as #defines
**	here in front-end header file feds.h.
**	Data structures general to all the front-ends
**	(such as FRAME, etc) are assigned in the range 1000-1999.
**	Each current front-end subsystem is allocated a range
**	within which to assign dsid's for data structures specific
**	to itself.
**	
**	Allocation is as follows:
**	
**		    0 -   999	system-wide, CL
**		 1000 -  1999	FE general
**		 2000 -  2999	ABF/OSL
**		 3000 -  3999	Equel
**		 4000 -  4999	GBF
**		 5000 -  5999	Ingres/Menu (switcher, relshell)
**		 6000 -  6999	QBF
**		 7000 -  7999	RBF
**		 8000 -  8999	Report
**		 9000 -  9999	Sreport
**		10000 - 10999	Vifred
**		11000 - 11999	Miscellaneous
**					Ingcntrl
**					Printform
**					Copyform
**					Copyrep
**		12000 - 16535	(unused)
**	
*/

/*
**		    0 -    99	System-wide; CL
*/
/*
** 0 and 1 reserved for DS_PREGFLD and DS_PTBLFLD (which would
** otherwise be within FE general) in order to be consistent
** with already existing compiled forms.
*/
# define	DS_PREGFLD	   0
# define	DS_PTBLFLD	   1

/*
**		 1000 -  1999	FE general
*/
# define	DS_VTREE	1000
# define	DS_VALROOT	1001
# define	DS_VALLIST	1002
# define	DS_FRAME	1003
# define	DS_FIELD	1004
/*
** Pointers to REGFLD and TBLFLD structures must be given the
** id's 0 and 1 in order to be consistent with compiled forms
** that already exist.
*/
# define	DS_REGFLD	1005
# define	DS_TBLFLD	1006
# define	DS_FLDHDR	1007
# define	DS_FLDTYPE	1008
# define	DS_FLDVAL	1009
# define	DS_FLDCOL	1010
# define	DS_TRIM		1011
# define	DS_FTINTRN	1012
# define	DS_FMT		1013
# define	DS_LIST		1014
# define	DS_PTRIM	1015
# define	DS_PFIELD	1016
# define	DS_TABLE	1017
# define	DS_SPECIAL	1018
# define	DS_PVTREE	1019
# define	DS_PVALROOT	1020

/*
**		 2000 -  2999	ABF/OSL
**
**	NOTE: OC_ classes for frame types are used as DS id's when handling
**	the ABRTSVAR structure because this allows the handling of the
**	structure as a DSUNION with no special massaging before and after
**	relocation.  These codes are in the 2200-2300 range, and are not
**	likely to change since they appear in catalogs.  The DS_ codes below
**	must not conflict with them.
*/
# define	DS_AROBJ	2000
# define	DS_ARPRO	2001
# define	DS_ARFRM	2002
# define	DS_ARVAR	2003
# define	DS_OIFRM	2004
# define	DS_OIDBDV	2005
# define	DS_OISYMTAB	2006
# define	DS_OIFCONS	2007
# define	DS_OIDBTEXT	2008
# define	DS_ARFORM	2009
# define	DS_ARVSNUSER	2010
# define	DS_ACUFRM	2011
# define	DS_AC4PROC	2012
# define	DS_OIDTXTPTR	2013
# define	DS_OIDCONS	2014

/*
**		 3000 -  3999	Equel
*/

/*
**		 4000 -  4999	GBF
*/

/*
**		 5000 -  5999	Ingres/Menu (switcher, relshell)
*/

/*
**		 6000 -  6999	QBF
*/
# define	DS_MQQDEF	6001
# define	DS_RELINFO	6002
# define	DS_ATTRIBINFO	6003
# define	DS_JOININFO	6004
# define	DS_QDEFINFO	6005

/*
**		 7000 -  7999	RBF
*/

# define	DS_SHDS		7000
# define	DS_COPT		7001
# define	DS_SEC		7002
# define	DS_CS		7003

/*
**		 8000 -  8999	Report
*/

# define	DS_ATT		8000
# define	DS_PRS		8001
# define	DS_ACC		8002

/*
**		 9000 -  9999	Sreport
*/

/*
**		10000 - 10999	Vifred
*/

# define	DS_POS		10000
# define	DS_VFNODE	10001
# define	DS_WRITEFRM	10002
# define	DS_PPOS		10003
# define	DS_PVFNODE	10004

/*
**		11000 - 11999	Miscellaneous
*/

/*---------------------------- +++ ---------------------------------*/
