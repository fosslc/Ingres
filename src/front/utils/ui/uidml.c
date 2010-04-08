/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<ci.h>
#include	<nm.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ui.h>
#include	<fedml.h>
#include	<uigdata.h>

/**
** Name:	uidml.c -	Front-End Utility Determine DML Type Module.
**
** Description:
**	Contains the routine that determines the default DML
**	for the front-ends.  Defines:
**
**	IIUIdml()	return value for default DML.
**
** History:
**	Revision 6.1  87/10/01  danielt
**	Initial revision.
**	08-jun-94 (jpk/kirke)
**	    Eliminated reference to II_AUTHORIZATION.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:	IIUIdml() -		Return Value for Default DML.
**
** Description:
**	Returns the default value for the query language to be used by a
**	front-end product.
**
**	Two factors must be considered by this routine:
**
**	It looks at the query language(s) that the DBMS can understand, as
**	indicated by the "ii_dbcapabilities" catalog, which can be QUEL, SQL
**	or GATEWAY SQL.
**
**	It also looks at the query language(s) that the installation is
**	authorized to use, determined by a call to 'FEdml()' (which calls
**	'CIcapability()' to examine the authorization string.)  This can
**	be SQL and/or QUEL, or neither, if there were some unforseen problem
**	with the authorization routines.  Note that in the case of a foreign
**	DBMS, authorization strings may not be implemented (DG for instance).
**	In this case 'CIcapability()' will always return OK for any capability.
**
**	This function uses the following priority in determining which DML to
**	use as the default:
**
**	If both SQL and QUEL are authorized, then the DML will be chosen in
**	this order:
**
**		1) RT/SQL (if there is a DBMS capability)
**		2) QUEL (if there is a DBMS capability)
**		3) Gateway SQL
**
**	If only one language is authorized, that language will be chosen as
**	long as it is understood by the DBMS.  It is assumed that the only
**	case of this would be when an INGRES installation was authorized to be
**	SQL-only.  In this case, RT/SQL would be chosen.
**
**	When no DBMS capabilities are defined for any language, the most
**	limited DML is returned (Gateway SQL.)
**
**	NOTE:  It is assumed that 'IIUIdci_initcap()' will be called before
**	any calls to 'IIUIdml()'.  This function will NOT indicate an error
**	if there is a problem accessing the "iidbcapabilities" catalog.
**
** Returns:
**	{nat}	UI_DML_QUEL	-Quel is default language
**		UI_DML_SQL	-RT/SQL is default language
**		UI_DML_GTWSQL   -Gateway SQL is default language
**
** History:
**	01-oct-1987 (danielt)	Written.
**	08/09/87 (jhw)  Changed to use 'FEdml()'.
**	18-jun-1991 (jhw)  Clarified logic.
*/

i4
IIUIdml()
{
	if ( IIUIdbdata()->dmldef == UI_DML_NOLEVEL )
	{
		i4	deflt,	/* default */
   			avail;  /* available DMLs */
		i4	level;

		/* See what languages are authorized */
		FEdml(&deflt, &avail);

		/*
		** Determine the default language.
		**
		**	Always check for SQL first, which will then be the
		**	default even if QUEL is available.  If neither DML
		**	is authorized, check the DBMS as well.  This is to
		**	take care of any potential non-fatal errors with the
		**	authorization string.
		*/

		if ( avail == FEDMLNONE  ||  avail == FEDMLBOTH )
		{ /* check DBMS */
			if ( (level = IIUIdcs_sqlLevel()) != UI_DML_NOLEVEL 
				|| (level = IIUIdcq_quelLevel())
						!= UI_DML_NOLEVEL )
			{
				return (IIUIdbdata()->dmldef = level);
			}
		}

		/*
		** At this point we assume that this is an INGRES DBMS
		** with only one language authorized.
		*/

		if ( ( avail == FEDMLSQL &&
			       (level = IIUIdcs_sqlLevel()) != UI_DML_NOLEVEL )
			|| ( avail == FEDMLQUEL &&
			      (level = IIUIdcq_quelLevel()) != UI_DML_NOLEVEL ))
		{
			return (IIUIdbdata()->dmldef = level);
		}

		/*
		** At this point, only a major error such as a screwed up
		** "iidbcapabilities" catalog could have prevented us from
		** finding a default DML, so just return the most limited DML
		** (gateway SQL.)
		*/

		IIUIdbdata()->dmldef = UI_DML_GTWSQL;
	}
	return IIUIdbdata()->dmldef;
}
