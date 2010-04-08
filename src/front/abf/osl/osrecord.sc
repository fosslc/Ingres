/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

#include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
#include	<st.h>
#include	<cm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<adf.h>
#include	<fe.h>
EXEC SQL INCLUDE <ui.sh>;
#include	<afe.h>
#include	<oodep.h>
#include	<oocat.h>
EXEC SQL INCLUDE <ooclass.sh>;
#include	<fdesc.h>
#include	<oslconf.h>
#include	<oserr.h>
#include	<osglobs.h>
#include	<ossym.h>
#include	<ostree.h>
# include     	<uigdata.h>

/**
** Name:	osrecord.sc -	OSL Translator Class Lookup.
**
** Description:
**
**	osreclook()	lookup a class definition.
**	osrecdeclare()	declare a class and it's attributes to the symboltable.
**	osldform()	load a form and its fields as a class definition.
**	osldtable()	load a table and its columns as a class definition.
**
**    statics:
**	_findrec()	find a class definition.
**	_mkrec()	make a class struct, put on the list.
**	_mkratt()	make a class attribute struct, put on class's list.
**	_mkalias()	make an internal alias for a datatype, for namespace.
**
** History:
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Cleaned up description and casting of return value
**		in osrecdeclare.
**	07-april--92 (kimman)
**		Making FormPname, TablePname, and TfldPname static variables
**		since they don't need to be global.
**	12-jun-92 (edf)
**            Fixed bug 36438 in osldtable() - Force all
**            attribute names to lowercase for special class definitions
**            (ie, type of table X).
**
**	Revision 6.5
**	23-feb-93 (davel)
**		Fix bug 49810: Added owner argument to the FEatt_open() call
**		is osldtable().
**	17-may-93 (davel)
**		Fix bugs 51700 & 51721 - add table owner argument to
**		osldtable().  Also fix delim id check to properly use
**		IIUGdlm_ChkdlmBEobject().
**	23-jul-93 (donc)
**		Fix bug 53593 - osgl_dep was being called with one too few
**		arguments (the owner wasn't being passed as per the last
**		two bug fixes.  In addition _findrec wasn't providing
**		object ownership information.  This was added. 
**	23-jul-93 (donc)
**		Fix bug 52458 - osgl_dep was (once again) being called with
**		the last two bug fixes.  This time the function modified was
**		osldform.
**	19-jan-94 (donc) Bug 57372
**		Modify _mkrec. Need to allocate space for r_owner in the record
**		and initialize the pointer to it.
**	27-jan-94 (donc) Bug 59024
**		Pull additions to _mkratt that caused SEP tests to fail for 
**		records based upon tablefields.
**      27-jan-64 (donc) Bug 58443
**              Alter record and atrribute names to lowercase via CVlower. If
**              this isn't done record and attribute lookup for FIPS uppercase
**              databases fail.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

typedef struct _att_struct {
	char		*a_name;
	DB_DATA_VALUE	a_dbdv;
	struct _att_struct	*a_next;
} RATT;

typedef struct _rec_struct {
	char		*r_name;
        char		*r_owner;
	char		*r_typename;
	i4		r_flags;
	OOID		r_deptype;
	OOID		r_ooid;
	struct _rec_struct	*r_next;
	struct _rec_struct	*r_tfld;	/* current tablefield. */
	struct _att_struct	*r_fields;
} REC;

/* for detecting circular class references. */
#define	R_VISITED	0x02

static REC	*Rlist = NULL;

/* Annoying, but FEtsalloc fails on a zero tag. */
static TAGID	Tag = 0;
#define RTAG	(Tag == 0 ? (Tag = FEgettag()) : Tag)

static REC	*_mkrec();
static RATT	*_mkratt();
static REC	*_findrec();
static char	*_mkalias();

/*
** We need buffers big enough to hold 
**	'table field <name>.<name>'
** where each <name> may be FE_MAXNAME long.
*/
#define	RC_MAXNAME	((FE_MAXNAME * 3) + 3) 

/*{
** Name:	osreclook() -	Look up a class definition.
**
** Description:
**	Given a class name, looks up the class definition.
**
** Inputs:
**	rname	{char *}	the name of the class.
**
** Returns:
**	{STATUS}	OK		success
**			FAIL		couldn't find the object
**
** History:
**	9-89 (billc) first written.
*/

STATUS 
osreclook(rname)
char	*rname;
{
	REC	*rec;
	char	buf[RC_MAXNAME + 1];

	_VOID_ STlcopy(rname, buf, sizeof(buf) - 1);
	CVlower(buf);
	if ((rec = _findrec(buf)) == NULL)
		return FAIL;

	/* 
	** If we haven't done it already,
	** create a dependency on this class definition 
	*/
	if (rec->r_deptype != 0)
	{
		osgl_dep(rec->r_name, rec->r_owner, OC_RECORD, rec->r_deptype);
		rec->r_deptype = 0;

	}
	return OK;
}

/*{
** Name:	osrecdeclare() -	declare class to symbol table.
**
** Description:
**	Given a complex object, declare all of its attributes to the symbol
**	table.
**
** Inputs:
**	parent	{OSSYM*}	The parent symbol.
**	rname	{char *}	Name of the object.
**	sym_type {nat}		The type (OSVAR, OSGLOB, etc.) of the symbol
**					we want to create.
**	dbdv	{DB_DATA_VALUE*} to pass to the symbol table.
**
** Returns:
**	Pointer to symbol table entry for new complex object (NULL on error).
**
** History:
**	11-89 (billc) first written.
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Cleaned up description and casting of return value.
*/

OSSYM * 
osrecdeclare(parent, rname, sym_type, dbdv)
OSSYM		*parent;
char		*rname;
i4		sym_type;
DB_DATA_VALUE 	*dbdv;
{
	register REC	*rec;
	register RATT	*ratt;
	OSSYM		*sym;
	i4 		flags = FDF_RECORD;
	char		rbuf[RC_MAXNAME + 1];

	if (ossympeek(rname, parent) != NULL)
	{
		/* multiply-defined symbol. */
		oscerr(OSEHIDDEN, 2, rname, parent->s_name);
		return (OSSYM *)NULL;
	}

	_VOID_ STlcopy((char *) dbdv->db_data, rbuf, sizeof(rbuf) - 1);
	CVlower(rbuf);

	/* 
	** Our caller has already determined that this is a legit class,
	** so this check is just pro-forma bullet-proofing.
	*/
	if ((rec = _findrec(rbuf)) == NULL)
	{
		oscerr(OSBADATTR, 2, rname, (char*) dbdv->db_data);
		return (OSSYM *)NULL;
	}

	/* check for recursively-defined class. */
	if (rec->r_flags & R_VISITED)
	{
		oscerr(OSRECURSIVE, 2, rname, rec->r_typename);
		return (OSSYM *)NULL;
	}

	if (dbdv->db_length != 0)
		flags |= FDF_ARRAY;

	/* create the symbol for the object. */
	if ((sym = ossymput(rname, sym_type, dbdv, parent, flags)) == NULL)
		osuerr(OSBUG, 1, ERx("osrecdeclare"));	/* exit! */

	/* Now create symbols for every attribute. */
	for (ratt = rec->r_fields; ratt != NULL; ratt = ratt->a_next)
	{
		i4	a_flags = 0;

		DB_DATA_VALUE *a_dbdv = &(ratt->a_dbdv);

        	if (a_dbdv->db_datatype == DB_DMY_TYPE)
		{
			/* We have a nested complex object type. */
			a_flags |= FDF_RECORD;

			if (a_dbdv->db_length != 0)
			{
				/* ...and it's an array */
				a_flags |= FDF_ARRAY;
			}
			/* Setup the recursion detector. */
			rec->r_flags |= R_VISITED;
			_VOID_ osrecdeclare(sym, ratt->a_name, OSRATTR, a_dbdv);
			rec->r_flags &= ~R_VISITED;
		}
		else
		{
			_VOID_ ossymput(ratt->a_name, OSRATTR, 
					a_dbdv, sym, a_flags
				);
		}
	}

	/* Setup symbols for _state and _record, if this is an array. */
	if (flags & FDF_ARRAY)
	{
		DB_DATA_VALUE tdbv;

		tdbv.db_datatype = DB_INT_TYPE;
		tdbv.db_prec = 0;
		tdbv.db_data = NULL;

		tdbv.db_length = 1;
		_VOID_ ossymput(ERx("_STATE"), OSRATTR, 
				&tdbv, sym, FDF_READONLY
			);

		tdbv.db_length = 4;
		_VOID_ ossymput(ERx("_RECORD"), OSRATTR, 
				&tdbv, sym, FDF_READONLY);
	}

	return sym;
}

extern char	*osDatabase;	/* database name */

extern i4	osCompat;	/* 5.0 text compatibility */

/* 
** Used in generating internal datatype aliases in order to partition 
** the datatype namespace.
*/
static char *FormPname = ERx("form");
static char *TablePname = ERx("table");
static char *TfldPname = ERx("table field");

/*
** Constants from the forms catalogs.
*/
#define		FREGULAR	0
#define		FTABLE		1
#define		FCOLUMN		2

/*{
** Name:	osldform() -	Load form into class-definition format.
**
** Description:
**	This routine loads a form from the dbms and creates a class for it. 
**
** Input:
**	formname	{char *}  The name of the form/procedure object
**			  for which to enter fields.
**	tfname		{char *}  Tablefield we're interested in, if any.
**			  This controls the datatype alias that we return.
**
** Returns:
**	{char *}	an internal datatype name, not exposed to user,
**			goobered up so multiple objects can occupy the same
**			namespace.
**
** History:
**	10/89 (billc) -- Stolen.
*/

static	STATUS	do_field();
static	char	*r_typename();

char *
osldform (formname, tfname)
char	*formname;
char	*tfname;
{
	REC	*rec;
	i4	frid;		/* form ID */
	char	nbuf[FE_MAXNAME + 1];
	char	*retname;
	char	tfbuf[FE_MAXNAME + 1];
	char	dummy[OODATESIZE+1];
	STATUS	IIAMfoiFormInfo();

	if (formname == NULL || *formname == EOS)
		osuerr(OSNOFRMNAME, 0);	/* exit! */

	if (osDatabase == NULL)
		osuerr(OSNODB, 0);	/* exit! */

	_VOID_ STlcopy(formname, nbuf, sizeof(nbuf) - 1);
	CVlower(nbuf);

	if (tfname != NULL)
	{
		_VOID_ STlcopy(tfname, tfbuf, sizeof(tfbuf) - 1);
		CVlower(tfbuf);
		tfname = tfbuf;
	}

	/* See if we've already created a class type for this form */
	retname = r_typename(nbuf, tfname);
	if (retname != NULL)
		return retname;

	/* Make sure the form exists. */
	if (IIAMfoiFormInfo(nbuf, dummy, &frid) != OK 
	   || frid == OC_UNDEFINED)
	{
		oscerr(OSNOFORM, 1, formname);
		return NULL;
	}

	/* We have a form. Make a class. */
	rec = _mkrec(FormPname, (char*)NULL, nbuf);

	/* and create a dependency. */
	osgl_dep(rec->r_name, rec->r_owner, OC_RECORD, OC_DTFORM_TYPE);

	/*
	** Step through the form retrieving all the fields
	** entering each into the class's attribute list.
	*/
	if (IIAMforFormRead(nbuf, frid, (PTR) rec, do_field) != OK)
		return NULL;

	return r_typename(nbuf, tfname);
}

/*
** r_typename	- get the datatype name associated with a 
**		formname/tablefieldname pair, if such a type has been declared
**		already.
*/

static char *
r_typename(fname, tfname)
char	*fname;
char	*tfname;
{
	char	*retname;
	REC	*rec;

	retname = _mkalias(FormPname, (char*)NULL, fname);

	if ((rec = _findrec(retname)) == NULL)
		return NULL;

	if (tfname != NULL)
	{
		RATT	*r;

		/* look for the class-type for the tablefield. */
		for (r = rec->r_fields; r != NULL; r = r->a_next)
		{
			if (STequal(r->a_name, tfname))
				break;
		}

		if (r == NULL)
		{
			/* whoa. form exists in our cache, but doesn't
			** have any such tablefield.
			*/
			oscerr(OSNOTBLFLD, 2, tfname, fname);
			retname = NULL;
		}
		else
		{
			retname = _mkalias(TfldPname, fname, tfname);
		}
	}
	return retname;
}

static STATUS
do_field(fname, fldname, p, kind, dbv)
char	*fname;
char	*fldname;
PTR	p;
i4	kind;
DB_DATA_VALUE	*dbv;
{
	REC	*rec = (REC *) p;
	REC	*tab;

	switch (kind)
	{
	  case FTABLE:
		/* tablefield.  This, also, is a class. */
		tab = _mkrec(TfldPname, fname, fldname);
		rec->r_tfld = tab;

		/* and it's also an attribute of the forms class. */
		_VOID_ _mkratt(rec, fldname, tab->r_typename,
				DB_DMY_TYPE, (i4) 1, (i2) 0
		);
		break;

	  case FCOLUMN:
		/*  This is an attribute of the tablefield's class. */
		if (_mkratt(rec->r_tfld, fldname, (char*)NULL, 
			dbv->db_datatype, 
			dbv->db_length, dbv->db_prec) == NULL)
		{
			oscerr(OSE2COL, 2, fldname, rec->r_tfld->r_name);
			return FAIL;
		}
		break;

		/*  This is an attribute of the form's class. */
	  case FREGULAR:
		if (_mkratt(rec, fldname, (char*)NULL,
			dbv->db_datatype, 
			dbv->db_length, dbv->db_prec) == NULL)
		{
			oscerr(OSE2FLD, 2, fldname, fname);
			return FAIL;
		}
		break;
	}	/* end switch */
	
	return OK;
}

/*{
** Name:	osldtable() -	Load DBMS table into class-definition format.
**
** Description:
**	This routine loads a table definition from the dbms and creates a
**	class for it. 
**
** Input:
**	tablename	{char *}  The name of the table to enter.
**	parentname	{char *}  The owner of the table to enter; NULL
**				  if no explicit owner was specified.
**
** Returns:
**	{char *}	an internal datatype name, not exposed to user,
**			goobered up so multiple objects can occupy the same
**			namespace.
**
** History:
**	10/89 (billc) -- written.
**	23-feb-93 (davel)
**		Added owner argument to the FEatt_open() call. (bug 49810)
**	17-may-93 (davel)
**		Added parentname argument, and fixed delim ID checking logic.
**		This fixes bugs 51700 & 51721.  Also check for non-FE datatypes,
**		and omit from the record definition - this fixes bug 51814.
*/

char *
osldtable (tablename, parentname)
char	*tablename;
char	*parentname;
{
        FE_ATT_QBLK     qblk;
	char		*retname;
	REC		*rec;
	FE_ATT_INFO     ainfo;
	DB_DATA_VALUE   dbv;
	char		*pname = parentname;
	char		tblbuf[FE_MAXNAME + 1];
	char		pbuf[FE_MAXNAME + 1];

	if (tablename == NULL || *tablename == EOS) 
		osuerr(OSBUG, 1, ERx("osldtable(no tablename)"));/* exit! */

	_VOID_ STlcopy(tablename, tblbuf, sizeof(tblbuf) - 1);
	(void) IIUGdlm_ChkdlmBEobject(tblbuf, tblbuf, FALSE);

	if (pname != NULL)
	{
		_VOID_ STlcopy(pname, pbuf, sizeof(pbuf) - 1);
		(void) IIUGdlm_ChkdlmBEobject(pbuf, pbuf, FALSE);
		pname = pbuf;
	}

	/* See if we've already created a class type for this table */
	retname = _mkalias(TablePname, pname, tblbuf);
	if (_findrec(retname) != NULL)
		return retname;

	if ( FEatt_open(&qblk, tblbuf, pname) != OK 
	  || qblk.status == STATUS_DONE
	   )
	{
		oscerr(OSNOTAB, 1, tablename);
		return NULL;
	}

	rec = _mkrec(TablePname, pname, tblbuf);

	osgl_dep(tblbuf, pname, OC_RECORD, OC_DTTABLE_TYPE);

	while(FEatt_fetch(&qblk,&ainfo) == OK)
	{
		bool warning_raised = FALSE;

		FEatt_dbdata(&ainfo, &dbv);

		/* check to make sure that this is a valid FE datatype - if not
		** then leave this attribute out of the declaration.
		*/
		if (!IIAFfedatatype(&dbv))
		{
			if (!warning_raised)
			{
			    oswarn(E_OS025B_DatatypeUnsupp, 1, retname);
			    warning_raised = TRUE;
			}
			continue;
		}

		CVlower(ainfo.column_name);
		_VOID_ _mkratt(rec, ainfo.column_name, (char*)NULL,
			dbv.db_datatype, dbv.db_length, dbv.db_prec
			);
	}
	FEatt_close(&qblk);

	return retname;
}

/*{
** Name:	_mkrec() -- make a class and put it on the list.
**
** Inputs:
**	cname	- optional name of class.
**	pname	- optional name of parent object.
**	name	- name of object.
**	ooid	- OOID of the class (for looking up attributes.)
**
**	These extra names are used to create isolated namespaces.  That is,
**	we could have seen
**		x1 = class of form X
**		x2 = class of table X
**		x3 = class of table field y.X
**		x4 = class of table field z.X
**		x5 = X
**	That's 5 instances of the name X, all legal and distinct.
*/

static REC *
_mkrec(cname, pname, name)
char	*cname;
char	*pname;
char	*name;
{
	REC	*rec;
	rec = (REC *)FEreqmem(RTAG, sizeof(*rec), TRUE, (STATUS *)NULL);
	if (rec == NULL)
		return NULL;

	rec->r_name = FEtsalloc(RTAG, name);
        rec->r_owner = (char *)FEreqmem(RTAG, FE_MAXNAME+1, TRUE,
                (STATUS *)NULL);


	/* 'cname' is used for aliasing - so we can preserve namespaces. */
	rec->r_typename = _mkalias(cname, pname, rec->r_name);
	rec->r_fields = NULL;
	rec->r_tfld = NULL;
	rec->r_ooid = OC_UNDEFINED;

	/* put this on the class list */
	rec->r_next = Rlist;
	Rlist = rec;

	return rec;
}
/*{
** Name:	_mkratt() -- make an atttribute and put it on the class' list.
**
** Inputs:
**	rec	{REC *}	The class that will have this attribute.
**	name	{char*}	Name of the attribute
**	desc	{char*}	Name of the datatype of the attribute.
**	< standard dbdv information. >
*/

static RATT *
_mkratt(rec, name, desc, type, length, prec)
REC	*rec;
char	*name;
char	*desc;
i2	type;
i4	length;
i2	prec;
{
	RATT	*ratt;

	ratt = (RATT *)FEreqmem(RTAG, sizeof(*ratt), TRUE, (STATUS *)NULL);
	if (ratt == NULL)
		return NULL;

	ratt->a_name = FEtsalloc(RTAG, name);

	if (desc != NULL)
		ratt->a_dbdv.db_data = (PTR) FEtsalloc(RTAG, desc);
	ratt->a_dbdv.db_datatype = type;
	ratt->a_dbdv.db_length = length;
	ratt->a_dbdv.db_prec = prec;

	/* put this on the attribute list of the class */
	ratt->a_next = rec->r_fields;
	rec->r_fields = ratt;

	return ratt;
}
/*{
** Name:	_mkalias() -- make an internal datatype name, for partitioning
**				namespace.
**	We make these aliases as a trick to partition the namespace.
**	We could have seen
**		x1 = class of form X
**		x2 = class of table X
**		x3 = class of table field y.X
**		x4 = class of table field z.X
**		x5 = X
**	That's 5 instances of the name X, all legal and distinct.
**	So internally we create datatypes:
**		class of form X			==> "form X"
**		class of table X		==> "table X"
**		class of table field y.X	==> "table field y.X"
**		class of table field z.X	==> "table field z.X"
**		X				==> "X"
**	This makes error messages look better at runtime, too.
*/

static char *
_mkalias(classname, parentname, objname)
char	*classname;
char	*parentname;
char	*objname;
{
	/* need a big buffer. */
	char	buf[RC_MAXNAME + 1];
	char	parent[FE_MAXNAME + 1];
	char	obj[FE_MAXNAME + 1];
	if (parentname != NULL)
	{
	    STcopy( parentname, parent);
	    CVlower( parent);
	}
	STcopy( objname, obj);
	CVlower( obj );

	if (classname == NULL)
		return objname;

	if (parentname == NULL)
	{
		_VOID_ STprintf(buf, ERx("%s %s"), classname, obj);
	}
	else
	{
		_VOID_ STprintf(buf, ERx("%s %s.%s"), classname, 
					parent, obj);
	}
	return FEtsalloc(RTAG, buf);
}

/*{
** Name:	_findrec() -	find a class definition in our list.
**
** Description:
**
** Inputs:
**
** Returns:
**	{STATUS}	OK		success
**			FAIL		couldn't find the object
**
** History:
**	9-89 (billc) first written.
*/

static bool	Loaded = FALSE;

static REC * 
_findrec(rname)
char	*rname;
{
EXEC SQL BEGIN DECLARE SECTION;
	char	recname[FE_MAXNAME + 1];
        char    recowner[FE_MAXNAME + 1];
	i4	ooid;
	i4	recid;
	i4	applid;
	char	attname[FE_MAXNAME + 1];
	char	db_desc[FE_MAXNAME + 1];
	i4	db_length;
	i2	db_prec;
	i2	db_type;
	i2	rec_order;
EXEC SQL END DECLARE SECTION;

	REC	*rec;
	/* 
	** We look in the list first, since we might have a number of 
	** CLASS OF declarations and no ABF declarations.  We might never
	** need to go to the DBMS.
	*/

	for (rec = Rlist; rec != NULL; rec = rec->r_next)
	{
		if (STequal(rec->r_typename, rname))
			break;
	}

	/* If we didn't find it in our list, what about the DBMS? */

	if (rec == NULL && !Loaded)
	{
		applid = osAppid;
		Loaded = TRUE;
		recid = OC_UNDEFINED;

		EXEC SQL REPEATED SELECT
			o.object_name, o.object_owner, o.object_id
		INTO :recname, :recowner, :ooid
		FROM ii_objects o, ii_abfobjects a
		WHERE a.applid = :applid 
			AND o.object_id = a.object_id 
			AND o.object_class = :OC_RECORD;
		EXEC SQL BEGIN;
		{
			REC *r;
	
			r = _mkrec((char*)NULL, (char *)NULL, recname);
			if (r == NULL)
				return NULL;
			r->r_deptype = OC_DTTYPE;
			r->r_ooid = ooid;
                        STcopy( recowner, r->r_owner );
			if (STequal(r->r_typename, rname))
				rec = r;
		}
		EXEC SQL END;

		if (FEinqerr() != OK)
			return NULL;
	}

	if (rec != NULL && rec->r_fields == NULL && rec->r_ooid != OC_UNDEFINED)
	{
		/* We haven't loaded the attributes, yet. */

		recid = rec->r_ooid;

		EXEC SQL REPEATED SELECT
			o.object_name, r.class_order,
			r.type_name, r.adf_length, r.adf_type, r.adf_scale
		INTO :attname, :rec_order, 
			:db_desc, :db_length, :db_type, :db_prec
		FROM ii_objects o, ii_abfclasses r
		WHERE r.class_id = :recid AND o.object_id = r.catt_id
		ORDER BY r.class_order DESC;
		EXEC SQL BEGIN;
		{
			_VOID_ _mkratt(rec, attname, db_desc, db_type, 
					db_length, db_prec
				);
		}
		EXEC SQL END;
	}

	return rec;
}

