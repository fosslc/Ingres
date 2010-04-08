/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ooclass.h>
#include	<oocat.h>
#include	<oocatlog.h>
# include	<uigdata.h>
#include	"eroo.h"

/**
** Name:	catclass.c -	Visual Object Catalog Class.
**
** Description:
**	Contains the visual object catalog class and its internal load method,
**	which is most likely to be redefined when a sub-class is used (and
**	hence, is placed together with the class itself, which will not be
**	referenced when a sub-class is defined.)  Defines:
**
**	IIooCatClass		visual object catalog class.
**	IIOOctxTFldLoad()	load visual object catalog table field.
**
** History:
**	Revision 6.2  89/01  wong
**	Initial revision.
**      03-Feb-96 (fanra01)
**          Extracted data for DLLs on Windows NT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Name:	IIooCatClass -	Visual Object Catalog Class.
**
** Description:
**	The class object for the visual object catalog class.
*/
GLOBALREF C_OO_CATALOG	IIooCatClass;

/*{
** Name:	IIOOctxTFldLoad() -	Load Visual Object Catalog Table Field.
**
** Description:
**	Loads the catalog table field with the description of the objects
**	(including sub-classes) selected by the pattern specified to match the
**	names for the current user.  The row corresponding to the last row on
**	which the cursor was positioned as specified by the name for that last
**	row is returned.
**
**	This method should only be called by the 'catLoad()' method and should
**	be redefined if necessary when additional selection criteria are
**	required.  ('catLoad' abstracts shared functionality.)
**
** Input:
**	self	{OO_CATALOG *}  The visual object catalog.
**	pattern	{char *}  Name pattern (either QUEL or SQL) with which
**				to load the catalog.
**	lastname {char *}  Name of object on which cursor was last positioned
**				in catalog table field.
**
** Output:
**	prevrow	{nat *}  The row number that contains the object with the name
**				'lastname'.
**
** History:
**	01/89 (jhw) - Written.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
*/
VOID
IIOOctxTFldLoad ( self, pattern, lastname, prevrow )
register OO_CATALOG	*self;
char			*pattern;
char			*lastname;
i4			*prevrow;
{

    IIOOctObjLoad(self, TRUE, pattern, IIUIdbdata()->user, lastname, prevrow);
}
