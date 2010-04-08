/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<abclass.h>
#include	<abfdbcat.h>
#include	<acatrec.h>
#include	<ilerror.h>
#include	"iamint.h"

/**
** Name:	iamcget.c -	Application Catalog Records Fetch Routine.
**
** Description:
**	Contains the routine used to fetch the catalog records for an
**	application object from the DBMS.  This can either be an entire
**	application or just one sub-component.  Defines:
**
**	IIAMcgCatGet()	fetch application catalog records.
**
** History:
**	Revision 6.4  90/9 (Mike S) 
**	Redo for IIAMdmrReadDeps interface.
**		
**	Revision 6.2  88/12  wong
**	Merged queries and added some attributes to the target list.
**	Abstracted out SQL fetch code into 'iiamCatRead()'.  89/05
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
**	09-dec-1993 (daver)
**		Implemented owner.table for copyapp table dependencies. Added 
**		parameter dep_owner to do_deps() callback function. Loaded 
**		owner into the ACATREC structure so copyapp has something 
**		to print to its file. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# define HASH_SIZE 127

FUNC_EXTERN     STATUS  IIUGheHtabEnter();
FUNC_EXTERN     STATUS  IIUGhfHtabFind();
FUNC_EXTERN     STATUS  IIUGhiHtabInit();
FUNC_EXTERN     VOID    IIUGhrHtabRelease();

GLOBALREF STATUS	iiAMmemStat;

OOID fid_unname();

static i4  hashfunc();
static i4  hashcompare();
static PTR hash_id;

/*{
** Name:	IIAMcgCatGet() -	Fetch Application Catalog Records.
**
** Description:
**	Fetch catalog record(s) with the essential information regarding an
**	ABF known object.  Record(s) allocated by this routine use tagged
**	memory for easy removal.  The application ID is provided as well as
**	the objects own ID to allow future use of the same object in multiple
**	applications.  Also, if UNKNOWN is passed in for the object ID, a list
**	of catalog records is returned for the entire application.
**
** Inputs:
**	apid	{OOID}  Application ID.
**	objid	{OOID}  Object ID, == UNKNOWN for entire application.
**
** Outputs:
**	rec	{ACATREC **}  Reference to reference for object.
**
** Return:
**	{STATUS}	OK		success
**			ILE_NOOBJ	doesn't exist
**			ILE_NOMEM	allocation failure
**			ILE_FAIL	query failed
**
** History:
**	5/87 (bobm)	written
**	12/88 (jhw)  Changed to use a single, repeated query, which should
**		be shared.
**	8/90 (Mike S) Fetch objects and dependencies separately
*/

static STATUS	do_row();
static VOID	do_deps();

static i2	Tag;

STATUS
IIAMcgCatGet ( apid, objid, rec )
OOID	apid;
OOID	objid;
ACATREC	**rec;
{
	STATUS	stat;

	Tag = FEgettag();

	*rec = NULL;

	/* Create hash table */
	_VOID_ IIUGhiHtabInit(HASH_SIZE, (VOID (*)())NULL, 
			      hashcompare, hashfunc, &hash_id);

	if ( (stat = iiamCatRead(apid, objid, do_row, (PTR)rec)) == ILE_NOMEM )
	{ /* allocation failed, release partial allocation */
		IIUGtagFree(Tag);
		stat = ILE_NOMEM;
	}
	else if ( stat != OK )
	{
		stat = ILE_FAIL;
	}
	else if ( *rec == NULL )
	{
		stat = ILE_NOOBJ;
	}
	else
	{
		stat = OK;
	}
	
	if (stat == OK)
		stat = IIAMdmrReadDeps(apid, objid, FALSE, 
				       do_deps, (APPL *)NULL);

	/* Release hash table */
	IIUGhrHtabRelease(hash_id);

	return stat;
}

#define _alloc(size) (iiAMmemStat == ILE_NOMEM ? (PTR)NULL : FEreqmem(Tag, (size), TRUE, (STATUS *)NULL))

/*
** local routine to handle the rows retrieved from query
**
** returns 0 normally, -1 if retrieve should be aborted
** This is only done for alloc failures.
**
** These are rows from ii_abfobjects, not joined to ii_abfdependencies,
** so each row is a new object.
*/
static STATUS
do_row ( abfobj, data )
register ABF_DBCAT	*abfobj;
PTR			data;
{
	register ACATREC	**rec = (ACATREC **)data;
	register ACATREC	*New;

	if ( (New = (ACATREC *) _alloc(sizeof(*New))) == NULL )
	{
		return iiAMmemStat = ILE_NOMEM;
	}

	/*
	** Push onto the current list -- '*rec' always points to
	** the last, and the objects are returned in reverse order
	** on the list.
	*/
	New->_next = *rec;
	*rec = New;

	New->dep_on = NULL;

	New->tag = Tag;
	New->version = abfobj->version;
	iiamStrAlloc( Tag, abfobj->name, &New->name );
	New->ooid = abfobj->ooid;
	New->class = abfobj->class;
	iiamStrAlloc( Tag, abfobj->create_date, &New->create_date );
	iiamStrAlloc( Tag, abfobj->alter_date, &New->alter_date );
	iiamStrAlloc( Tag, abfobj->owner, &New->owner );
	iiamStrAlloc( Tag, abfobj->source, &New->source );
	iiamStrAlloc( Tag, abfobj->symbol, &New->symbol );
	iiamStrAlloc( Tag, abfobj->short_remark, &New->short_remark );
	iiamStrAlloc( Tag, abfobj->arg0, &(New->arg)[0] );
	iiamStrAlloc( Tag, abfobj->arg1, &(New->arg)[1] );
	iiamStrAlloc( Tag, abfobj->arg2, &(New->arg)[2] );
	iiamStrAlloc( Tag, abfobj->arg3, &(New->arg)[3] );
	iiamStrAlloc( Tag, abfobj->arg4, &(New->arg)[4] );
	iiamStrAlloc( Tag, abfobj->arg5, &(New->arg)[5] );
	iiamStrAlloc( Tag, abfobj->rettype, &New->rettype );
	New->retadf_type = abfobj->retadf_type;
	New->retlength = abfobj->retlength;
	New->retscale = abfobj->retscale;
	New->flags = abfobj->flags;

	/* Add object to hash table */
	_VOID_ IIUGheHtabEnter(hash_id, (char *)&New->ooid, (PTR)New);

 	return iiAMmemStat;
}	

/*
** called for each dependency row.  We attach it to its depender by
** looking up the depender in our hash table
** 
** History:
**	xx-xxx-xxxx(xxx) Written.
**	09-dec-1993 (daver)
**		Implemented owner.table for copyapp dependencies. Added 
**		parameter dep_owner, loaded owner into the ACATREC
**		structure so copyapp has something to print to its file. 
*/
static VOID
do_deps(oid, dep_name, dep_class, dep_owner, dep_type, dep_link, dep_order)
OOID	oid;
char    *dep_name;
OOID    dep_class;
char	*dep_owner;
OOID    dep_type;
char    *dep_link;
i4      dep_order;
{
	ACATREC *catrec;
	PTR	ptr;
	register ACATDEP	*newd;

	/*
	** if not a dummy dependency, handle dependency
	*/
	if ( dep_class != 0 )
	{
		/* Find object it attaches to */
		if (IIUGhfHtabFind(hash_id, (char *)&oid, &ptr) != OK)
			return;	/* No object to tie dependency to */

		catrec = (ACATREC *)ptr;

		if ( (newd = (ACATDEP *) _alloc(sizeof(*newd))) == NULL )
		{
			iiAMmemStat = ILE_NOMEM;
			return;
		}

		/*
		** for OC_ILCODE, we pass back the id.  The id is part
		** of the name, which saves us retrieving anything else
		*/
		if ( dep_class == OC_ILCODE )
			newd->id = fid_unname( dep_name );
	
		newd->class =  dep_class ;
		newd->deptype = dep_type;
		newd->deporder = dep_order;
		iiamStrAlloc( Tag, dep_name, &newd->name );
		iiamStrAlloc( Tag, dep_owner, &newd->owner );
		iiamStrAlloc( Tag, dep_link, &newd->deplink);
	
		/* add to list */
		newd->_next = catrec->dep_on;
		catrec->dep_on = newd;
	}
}

/*
** Hash routines.  We just hash object ID module HASH_SIZE
*/
static i4
hashfunc(key, size)
char *key;	/* Pointer to object id (hash key) */
i4   size;	/* Size of hash table */
{
	OOID oid = *(OOID *)key;

	return oid % size;
}

static i4  
hashcompare(ij1, ij2)
char *ij1;
char *ij2;
{
	return *(OOID *)ij1 - *(OOID *)ij2;
}
