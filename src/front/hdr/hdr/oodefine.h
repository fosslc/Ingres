/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# ifndef OODEFINE_INCLUDE
# define OODEFINE_INCLUDE
/**
** Name:	oodefine.h -	miscellaneous macros and declarations for
**				Object Utility applications.
**
** Description:
**	This file contains miscellaneous macros, typedefs and extern
**	declarations for the Object Utility (OO) and for client applications.
**	It is only necessary to include this header file if items defined
**	in this file are directly referenced in client code.  (I.e. other
**	OO header files do not require this file.)
**
** History:
**	1986 (peterk) - created
**	4/87 (peterk) - changes for 6.0
**	11/89 (Mike S) - Remove IIUIuser and IIUIdba
**	12/19/89 (dkh) - VMS shared lib changes - GLOBALREF declarations
**			 moved to "ooldef.h" in OO directory.
**	9/15/93 (kchin) - Removed the FORCE_ALIGN macro, since we already
**			  have a standard alignment interface in CL 
**			  (ME_ALIGN_MACRO).  Other routines that call this
**		    	  macro in front are modified to called the 
**			  ME_ALIGN_MACRO.
**			  Also changed the return type of OOsnd(), OOsndAt(),
**			  OOsndSelf(), and OOsndSuper() to PTR.  As these
**			  functions will occassionally return pointers.  If
**			  their return types are OOID (ie. 32-bit int), 64-bit 
**			  addresses will be truncated.
**	6/21/93 (DonC) - Create the OOHASHCONST definition. It is a number
**			 unique to the OOTABLESIZE (which I bumped to 4096)
**      9/30/93 (DonC) - Made hash table size reflect what was documented (4096)
**	10/25/93 (donc)- Change OOsnd, OOsndAt, OOsndSelf, OOsndSuper,
**			 Return datatype to reflect that of send.c (kchin
**			 change)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-mar-2009 (stephenb)
**	    Add prototype for OOisTempId, without it the gcc 4.3 compiler
**	    makes bad code.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.  Make OOsnd and friends SCALARP, since
**	    that fits their most common usage (ie OOID) better.
**	24-Feb-2010 (frima01) Bug 122490
**	    Add function prototypes as neccessary
**	    to eliminate gcc 4.3 warnings.
**/

/* general object manager constants */
# define UNKNOWN	(-1)
# define OOTABLESIZE    4096	
# define OOHASHCONST	1813  /* For a OOTABLESIZE of 4096 */
# define OOHDRSIZE	(sizeof(OOID) + sizeof(dataWord))
# define MAXSYSOBJECT	10000
# define MAXPERMOBJECT	0x7fffffff
# define MAXTEMPOBJECT	0xfffffffe

/* constants pertaining to object encodings */
# define ESTRINGSIZE	1990
# define MAXTEMPS	500 
# define MAXRELOC	200

/* bit field macros (cum BT routines) */
# define BTsetF(pos, field) 	field |= (1<<(pos))
# define BTclearF(pos, field) 	field &= ~(1<<(pos))
# define BTtestF(pos, field) 	((field)&(1<<(pos)))

/* check if reference attribute is fetched */
# define refOff(o,r)	((i4)&o->r - (i4)o)
# define refVal(oop, refmember) OOcheckRef(oop, masterOff(oop->class, refOff(oop,refmember)),refOff(oop,refmember))

/* metaclass test */
# define OOisMeta(x)	((x)==OC_CLASS)

/* object equality test */
# define OOisEq(x, y)	((x)==(y))

/* delete rule constants */
# define	RESTRICTED	(-1)
# define	NULLIFIES	0
# define	CASCADES	1



/* function externs */

bool	OOisTempId(OOID id);

/*
** 	OOp() -	convert id to a ptr to instance.
**	
**	used to convert an Object id to a pointer to the
**	instance.  
*/

FUNC_EXTERN OO_OBJECT 	*OOp();
FUNC_EXTERN OO_CLASS	*OOpclass();
FUNC_EXTERN OOID	OOcheckRef();

/*
** 	OOsnd, OOsndSelf, OOsndSuper - message sending functions 
**
**	used to "polymorphically" invoke object methods
**	instead of directly calling method functions.
**
**	Generically declared to return SCALARP's, because sometimes
**	these things return pointers or pointer-sized data;
**	cast return values explicitly to desired type.
**
*/
FUNC_EXTERN SCALARP	OOsnd(), OOsndAt();
FUNC_EXTERN SCALARP	OOsndSelf(), OOsndSuper();

#ifndef NT_GENERIC
FUNC_EXTERN i4		OOhash();
#endif

# endif /* OODEFINE_INCLUDE */
