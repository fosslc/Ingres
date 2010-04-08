/*
**	Copyright (c) 1989, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<st.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
EXEC SQL INCLUDE	<ooclass.sh>;
EXEC SQL INCLUDE	<oosymbol.sh>;
# define OOSHORTREMSIZE 60
# define OOLONGREMSIZE 600
# define OODATESIZE 25
#include	<oocat.h>
EXEC SQL INCLUDE	<abfdbcat.sh>;
# include	<uigdata.h>
#include	<ilerror.h>

/**
** Name:	iamcread.sc -	Application Catalog Row Fetch Routines.
**
** Description:
**	Contains the routine used to fetch the catalog rows for an application
**	object from the DBMS.  This can either be an entire application or just
**	one sub-component of it.  Defines:
**
**	iiamCatRead()	fetch application catalog row.
**	iiamStrAlloc()	allocate a string.
**
** History:
**
**	7/90 (Mike S) Remove 3-way join; dependencies are now fetched separately
**
**	Revision 6.2  88/12  wong
**	Merged queries and added some attributes to the target list.
**	89/05  wong  Abstracted from 'IIAMapFetch()' and 'IIAMcgCatGet()'.
**	89/09  wong  Toggle readlock for concurrency on INGRES.
**
**	Revision 6.1  88/05  wong
**	Modified to use SQL and a structure to create object structures
**	(rather than statics.)
**	24-oct-88 (marian)
**		Modified column names for extended catalogs to allow them
**		to work with gateways.
**
**	Revision 6.0  87/05  bobm
**	Initial revision.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

GLOBALDEF STATUS	iiAMmemStat = OK;

/*{
** Name:	iiamCatRead() -	Fetch Application Catalog Rows.
**
** Description:
**	Fetch catalog rows with the essential information regarding an ABF known
**	object.  These rows are then processed by the input function.
**
**	The application ID is provided as well as the objects own ID to allow
**	future use of the same object in multiple applications.  Also, if
**	OC_UNDEFINED is passed in for the object ID, a list of catalog records
**	is returned for the entire application.
**
** Inputs:
**	applid	{OOID}  Application ID.
**	objid	{OOID}  Object ID, == OC_UNDEFINED for entire application.
**	func	{STATUS (*)()}  Function to apply to each record.
**	data	{PTR}  Data.
**
** Return:
**	{STATUS}	OK	success
**				otherwise error routine from 'FEinqerr()'.
**
** History:
**	5/87 (bobm)	written
**	12/88 (jhw)  Changed to use a single, repeated query, which should
**		be shared.
**	09/89 (jhw)  Toggle readlock for concurrency on INGRES DBMS.
**	28-feb-1991 (pete)
**		Changed "0,0,0" in select target list below to
**		"applid, applid, applid" and set those 3 columns to 0 inside
**		the select loop. This was done to get around
**		a bug in the RDB gateway. The DESCRIBE statement on RDB
**		gateways was returning "char" as the data type
**		for a numeric constant in a select list
**		(e.g. SELECT 0 from foo) this caused type conversion errors
**		in LIBQ. RDB Bug reported to DEC (by AlenM).
**	15-mar-93 (essi)
**		Fixed bug 50370. Need to use corelation name in the order by
**		clause since both ii_objects and ii_abfobjects have a common
**		field (object_id). 
**	17-mar-94 (rudyw)
**		Fix for 59810. Extract the constant comparison from a repeated
**		select where clause. Use the constant comparison to set up a 
**		conditional clause with two separate selects which each can be
**		handled by the optimizer in a way that is best for performance.
*/

STATUS
iiamCatRead ( applid, objid, func, data )
EXEC SQL BEGIN DECLARE SECTION;
OOID	applid;
OOID	objid;
EXEC SQL END DECLARE SECTION;
STATUS	(*func)();
PTR	data;
{
	STATUS		stat = OK;
EXEC SQL BEGIN DECLARE SECTION;
	ABF_DBCAT	abf_obj;
	i2		nullinds[32];
EXEC SQL END DECLARE SECTION;
	char		name[FE_MAXNAME+1];
	char		owner[FE_MAXNAME+1];
	char		short_remark[OOSHORTREMSIZE+1];
	char		create_date[sizeof(OOCAT_DATE)];
	char		alter_date[sizeof(OOCAT_DATE)];
	char		long_remark[1];
	char		altered_by[FE_MAXNAME+1];
	char		source[ABFSRC_SIZE+1];
	char		symbol[FE_MAXNAME+1];
	char		rdesc[FE_MAXNAME+1];
	char		args[ABFARGS][ABFARG_SIZE+1];
	char		depname[FE_MAXNAME+1];
 	char		deplnk[FE_MAXNAME+1];

	iiAMmemStat = OK;

	abf_obj.name = name;
	abf_obj.owner = owner;
	abf_obj.short_remark = short_remark;
	abf_obj.create_date = create_date;
	abf_obj.alter_date = alter_date;
	abf_obj.long_remark = long_remark;
	abf_obj.altered_by = altered_by;
	abf_obj.source = source;
	abf_obj.symbol = symbol;
	abf_obj.rettype = rdesc;
	abf_obj.dname = depname;
 	abf_obj.dlink = deplnk;
	abf_obj.arg0 = args[0];
	abf_obj.arg1 = args[1];
	abf_obj.arg2 = args[2];
	abf_obj.arg3 = args[3];
	abf_obj.arg4 = args[4];
	abf_obj.arg5 = args[5];

	/* Retrieve application object
	**
	** Note:  When the object ID is OC_UNDEFINED retrieve all objects for
	** an application, otherwise just retrieve a single application object.
	** (ABF uses the former and the 4GL compilers use the latter.)  Since
	** this test is made as part of the WHERE clause, and the query is a
	** a repeated SQL select, it is shared across each server.
	**
	** Fix for 59810.  Required pulling constant comparison out of the
	** where clause and creating a conditional with separate queries.
	** Must make any modification to query or select loop in both.
	*/
	if (objid == -1)
	{
		exec sql repeated select
			ii_objects.object_id, ii_objects.object_class,
			object_name, object_owner, 
			object_env, is_current,
			short_remark, create_date, 
			alter_date, '',
			alter_count, last_altered_by,
			ii_abfobjects.applid, abf_source, 
			abf_symbol, retadf_type, 
			rettype, retlength, 
			retscale, abf_version, 
			abf_arg1, abf_arg2, 
			abf_arg3, abf_arg4, 
			abf_arg5, abf_arg6, 
			'', '',
 			abf_flags,
			applid, applid, applid	/* changed from "0,0,0" */
			into :abf_obj:nullinds
			from ii_objects, ii_abfobjects
		where applid = :applid  and
			ii_objects.object_id = ii_abfobjects.object_id 
		order by object_name, ii_objects.object_id;
		exec sql begin;
		{
			abf_obj.deptype = 0;
			abf_obj.deporder = 0;
			abf_obj.dclass = 0;

			if (nullinds[28] == DB_EMB_NULL)
				abf_obj.flags = 0;
			if ( (stat = (*func)( &abf_obj, data )) != OK )
				exec sql endselect;
		}
		exec sql end;
	}
	else
	{
		exec sql repeated select
			ii_objects.object_id, ii_objects.object_class,
			object_name, object_owner, 
			object_env, is_current,
			short_remark, create_date, 
			alter_date, '',
			alter_count, last_altered_by,
			ii_abfobjects.applid, abf_source, 
			abf_symbol, retadf_type, 
			rettype, retlength, 
			retscale, abf_version, 
			abf_arg1, abf_arg2, 
			abf_arg3, abf_arg4, 
			abf_arg5, abf_arg6, 
			'', '',
 			abf_flags,
			applid, applid, applid	/* changed from "0,0,0" */
			into :abf_obj:nullinds
		from ii_objects, ii_abfobjects
		where applid = :applid  and
			ii_abfobjects.object_id = :objid  and
			ii_objects.object_id = ii_abfobjects.object_id 
		order by object_name, ii_objects.object_id;
		exec sql begin;
		{
			abf_obj.deptype = 0;
			abf_obj.deporder = 0;
			abf_obj.dclass = 0;

			if (nullinds[28] == DB_EMB_NULL)
				abf_obj.flags = 0;
			if ( (stat = (*func)( &abf_obj, data )) != OK )
				exec sql endselect;
		}
		exec sql end;
	}

	return stat != OK ? stat : FEinqerr();
}

/*{
** Name:	iiamStrAlloc() -	Allocate a New String Value.
**
** Description:
**	Allocates a new string value and sets the string reference to it.
**	The value is checked to see that it is different, and then it is
**	allocated and set.  (Several common values have shared allocation.)
**
**	This checks the global 'iiAMmemStat' to determine that memory allocation
**	can proceed.  Also, this will be set if allocation fails.  The global
**	avoids having to error check every blasted allocation call.  Allocation
**	calls stop allocating once iiAMmemStat == ILE_NOMEM.  (bobm)
**
** Input:
**	tag	{u_i4}  Memory tag for allocation.
**	str	{char *}  The string value to be allocated.
**	value	{char **}  Reference to old string value.
**
** Output:
**	value	{char **}  New string value if allocated.
**
** History:
**	05/87 (bobm) -- Written.
**	02/89 (jhw) -- Modified to set passed in value and check for new value.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
**	04-mar-92 (leighb) DeskTop Porting Change:
**		Moved function declaration outside of calling function.
**		This is so function prototypes will work.
*/

UIDBDATA	*uidbdata;		 

VOID
iiamStrAlloc ( tag, str, value )
u_i4	tag;
char	*str;
char	**value;
{
	uidbdata = IIUIdbdata();
	if ( STtrmwhite(str) <= 0 || *str == EOS )
		*value = _;
	else if ( *value == NULL || !STequal(str, *value) )
	{ /* new or different value */
		if ( STequal(str, uidbdata->user) )
			*value = uidbdata->user;
		else if ( STequal(str, uidbdata->dba) )
			*value = uidbdata->dba;
		else if ( iiAMmemStat != ILE_NOMEM )
		{
			if ( (str = FEtsalloc(tag, str)) != NULL )
				*value = str;
			else
				iiAMmemStat = ILE_NOMEM;
		}
	}
}
