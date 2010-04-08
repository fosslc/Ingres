/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	oocollec.h -	Object Module Collection Array Definition File.
**
** Description:
**	Contains the definition of the Object (OO) module collection array
**	class.  (See note below.)
**
**	OO_aCOLLECTION
**
** History:
**	Revision 6.2  89/03  wong
**	Moved here from "ooclass.qsc".
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes: add const since most of these structs
**	    are initialized using const char arrays.
**/

/*}
** Name:	OO_aCOLLECTION -	Collection Array Class Definition.
**
** Description:
**	Work-around for initialization of OO_ATTRIB Collections on VMS, where
**	forcibly casting (OO_ATTRIB *) to (OOID) is not permissible.  Instead
**	we will define a variant of OO_COLLECTION structure, OO_aCOLLECTION,
**	the last member of which is an (OO_ATTRIB *).  This will not have any
**	impact on the code that actually references these Collection or the
**	OO_ATTRIB pointers themselves as all such references use explicit
**	(legal) casts.
*/

typedef	struct OO_aCOLLECTION
{
 	OOID		ooid;

 	dataWord	data;

 	OOID		class;
 	const char	*name;
 	i4		env;
 	const char	*owner;
 	i4		is_current;
 	const char	*short_remark;
 	const char	*create_date;
 	const char	*alter_date;
 	const char	*long_remark;

 	i4		size;
 	i4		current;
 	i4		atend;
 	OO_ATTRIB	**array; /* special for OO_ATTRIB * Collections */
} OO_aCOLLECTION;
