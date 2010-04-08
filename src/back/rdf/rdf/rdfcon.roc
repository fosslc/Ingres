/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPRDFLIBDATA
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>

/**
**
**  Name: RDFCON.ROC - Global readonly variables.
**
**  Description:
**      THis file contains all the global readonly variables 
**	definitions. 
**
**  History:    
**      15-sep-88 (mings)
**	20-dec-91 (teresa)
**	    include ddb.h and change include of qefddb.h to qefrcb.h for SYBIL.
**	08-dec-92 (anitap)
**	    include qsf.h for CREATE SCHEMA.
**	15-dec-92 (wolf)
**	    include ulf.h ahead of qsf.h 
**	06-jul-93 (teresa)
**	    change include of dbms.h to include of gl.h, sl.h, iicommon.h &
**	    dbdbms.h.
**	14-sep-93 (swm)
**	    Included cs.h for definition of CS_SID which is used by qefrcb.h.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPRDFLIBDATA
**	    in the Jamfile.
**/

/**
**
**  Name: IIRD_INGRES - String containing "ingres";
**
**  Description:
**      String contains "ingres";
**
**  History:    
**      15-sep-88 (mings)
*/

GLOBALDEF char		       Iird_ingres[] = "INGRES"; 

/**
**
**  Name: IIRD_DINGRES - String containing "$ingres";
**
**  Description:
**      String contains "$ingres".
**
**  History:    
**      15-sep-88 (mings)
*/

GLOBALDEF char                Iird_dingres[] = "$ingres";

/**
**
**  Name: IIRD_table, IIRD_view, IIRD_index, IIRD_link - Strings define object types.
**
**  Description:
**      Strings contain object types.
**
**  History:    
**      15-sep-88 (mings)
*/
GLOBALDEF char                Iird_table[] = "T       ";
GLOBALDEF char                Iird_view[]  = "V       ";
GLOBALDEF char                Iird_index[] = "I	      ";       
GLOBALDEF char                Iird_link[]  = "L       ";

/**
**
**  Name: IIRD_YES, IIRD_NO, IIRD_QUOTED - Strings containing "YES",
**					   "NO", "QUOTED"		
**
**  Description:
**      Strings contain "YES", "NO" or "QUOTED".
**	They represent OWNER_NAME capability.
**
**  History:    
**      15-sep-88 (mings)
*/
GLOBALDEF char                Iird_yes[] = "YES";
GLOBALDEF char		      Iird_no[] = "NO";
GLOBALDEF char		      Iird_quoted[] = "QUOTED";

/*}
**
**  Name: IIRD_HEAP, IIRD_ISAM, IIRD_HASH, IIRD_BTREE - Storage types.
**
**  Description:
**      Strings contain storage types.
**
**  History:    
**      15-sep-88 (mings)
*/

GLOBALDEF char                Iird_heap[]  = "HEAP";
GLOBALDEF char                Iird_isam[]  = "ISAM";
GLOBALDEF char                Iird_hash[]  = "HASH";
GLOBALDEF char                Iird_btree[] = "BTREE";
GLOBALDEF char		      Iird_unknown[] = "                ";

/*}
**
**  Name: IIRD_LOWER, IIRD_MIXED, IIRD_UPPER - ldb name case capability.
**
**  Description:
**      Strings define ldb name case capability.
**
**  History:    
**      15-sep-88 (mings)
*/

GLOBALDEF char                Iird_lower[]  = "LOWER";
GLOBALDEF char                Iird_mixed[]  = "MIXED";
GLOBALDEF char                Iird_upper[]  = "UPPER";

/*}
**  Name: IIRD_CAPS - ldb capabilities 
**
**  Description:
**	ldb description definitions.
**	
**	Note that this file should be sync with supported capabilities. 
**	Currently, 14 capabilities are defined.
**
**  History:	
**	14-jan-89 (mings)
**	    initial creation
**	12-apr-89 (mings)
**	    added PHYSICAL_SOURCE
**	14-jun-91 (teresa)
**	    added _TID_LEVEL to fix bug 34684.
**	01-mar-93 (barbara)
**	    Added OPEN/SQL_LEVEL and DB_DELIMITED_CASE for Star support
**	    of delimited ids.
**	02-sep-93 (barbara)
**	    Added DB_REAL_USER_CASE for Star support of delimited ids.
*/

GLOBALDEF char		    *Iird_caps[] =
{
	"DISTRIBUTED",		/* RDD_DISTRIBUTED */
	"DB_NAME_CASE",		/* RDD_DB_NAME_CASE */
	"UNIQUE_KEY_REQ",	/* RDD_UNIQUE_KEY_REQ */
	"INGRES",		/* RDD_CAP_INGRES */
	"INGRES/QUEL_LEVEL",	/* RDD_QUEL_LEVEL */
	"INGRES/SQL_LEVEL",	/* RDD_SQL_LEVEL */
	"COMMON/SQL_LEVEL",	/* RDD_COMMON_SQL_LEVEL */
	"SAVEPOINTS",		/* RDD_SAVEPOINTS */
	"DBMS_TYPE",		/* RDD_DBMS_TYPE */
        "OWNER_NAME",		/* RDD_OWNER_NAME */
	"PHYSICAL_SOURCE",	/* PHYSICAL_SOURCE */
	"_TID_LEVEL",	    	/* RDD_TID_LEVEL */
	"OPEN/SQL_LEVEL",	/* RDD_OPENSQL_LEVEL */
	"DB_DELIMITED_CASE",	/* RDD_DELIMITED_NAME_CASE */
	"DB_REAL_USER_CASE"	/* RDD_REAL_USER_CASE */
};

/*}
**  Name: IIRD_DD_CAPS - default value for DD_CAPS structure. 
**
**  Description:
**	Initial value for DD_CAPS structure.
**	
**  History:	
**	14-jan-89 (mings)
**	    initial creation
*/
GLOBALDEF DD_CAPS	    Iird_dd_caps = {
					    0x0000L,
					    0,
				            600,
					    600,
					    600,
					    0,
					    "INGRES",
					    2};

/**
**
**  Name: IIRD_UPPER_SYSTAB, IIRD_LOWER_SYSTAB - Strings contain catalog names
**		    
**
**  Description:
**      Strings contain catalog name that we use to obtain catalog owner.
**
**  History:    
**      20-may-89 (mings)
**	    created
**	19-feb-91 (teresa)
**	    changed "II_OBJECTS" and "ii_objects" to "IITABLES" and 
**	    "iitables" since createdb has been changed and ii_objects does not
**	    exist when the distributed DB is created.  Unfortunately, RDF uses
**	    that catalog name to inquire for SYSTEM OWNER, so we can never
**	    create any tables.  By changing this to iitables, which is 
**	    guarenteed to exist, this paradox is eleminated."
*/

GLOBALDEF char		       Iird_upper_systab[] = "IITABLES"; 
GLOBALDEF char		       Iird_lower_systab[] = "iitables"; 
