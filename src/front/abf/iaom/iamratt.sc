/*
**	Copyright (c) 1987, 2004 Ingres Corporation
*/

#include	<compat.h>
#include	<er.h>
#include	<st.h>
#include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ui.h>
#include	<oocat.h>
#include	<oosymbol.h>
#include	<oodep.h>
EXEC SQL INCLUDE <abclass.sh>;
#include	<acatrec.h>
# include	<uigdata.h>
#include	"iamint.h"
#include	<oodefine.h>

/**
** Name:	iamratt.sc -	ABF Class Attribute Module.
**
** Description:
**	Contains routines to read and write ABF class attributes.
**
**	IIAMraReadAtts()	Read attributes from catalog.
**	IIAMwaWriteAtt()	Write an attribute to catalog.
**	IIAMdaDeleteAtt()	Delete an attribute from catalog.
**	IIAMccClearClass()	Delete class attributes from catalog.
**
**	IIAMgsrGetSpecialRecs() Read 'Special' Class Defs.
**			(doesn't quite belong here, but deserves a home.)
**
**  hack-methods:
**	iiamDbRattName()	Fetch class attribute by name.
**
** History:
**	8-89 (billc) first written.
**	03/11/91 (emerson)
**		Integrated another set of DESKTOP porting changes.
**    07-april--92 (kimman)
**            Making FormPname, TablePname, and TfldPname static variables
**            since they don't need to be global.
**	12-jun-92 (edf)
**		Fixed bug 36438 in IIAMgsrGetSpecialRecs() - Force all
**		attribute names to lowercase for special class definitions
**		(ie, type of table X).
**      3-aug-1992 (fraser)
**          Changed _flush0 to ii_flush0 because of conflict with
**          Microsoft library.
**	23-feb-93 (davel)
**		Fixed bug 49810: Added NULL owner name argument to 
**		FEatt_open() call.
**	18-may-93 (davel)
**		Add owner argument to ii_ldtable().
**      02-Sep-99 (hweho01)
**              Added function prototype of *iiamLkComp(). Without the 
**              declaration, the default int return value for a  
**              function will truncate a 64-bit address on ris_u64 
**              platform.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2005 (schka24)
**	    DB_DATA_VALUE changed, fix here.
**	28-Mar-2005 (lakvi01)
**	    Added oodefine.h for OO prototypes.
**	10-Jul-2007 (kibro01) b118702
**	    Remove IIAMdateAlias since its functionality is done in adi
*/

/*
** Cur_class - hack used to store currently-edited class definition.  This hack
**	is explained in greater detail with the hack-methods below.
*/

static RECDEF *Cur_class = NULL;

/*{
** Name:	IIAMraReadAtts() -	Read Class Attributes.
**
** Description:
**	Given a class definition object, read all the associated class
**	attributes.
**
** Inputs:
**	class	{RECDEF *} The class whose attributes we're reading.
**
** Returns:
**	{STATUS}	OK		success
**			FAIL		database read failure
**
** History:
**	8-89 (billc) first written.
**	9-90 (Mike S) Order attributes alphabetically
*/

APPL_COMP       *iiamLkComp();
static RECMEM *_makeobj();

STATUS
IIAMraReadAtts ( class )
RECDEF *class;
{
EXEC SQL BEGIN DECLARE SECTION;
	OOID	class_id;
	OOID	catt_id;
	char	name[FE_MAXNAME+1];
	char	owner[FE_MAXNAME+1];
	char	short_remark[OOSHORTREMSIZE+1];
	char	create_date[sizeof(OOCAT_DATE)];
	char	alter_date[sizeof(OOCAT_DATE)];
	char	altered_by[FE_MAXNAME+1];
	char	rdesc[FE_MAXNAME+1];
	DB_DATA_DESC	dbdesc;

	RECMEM	*catt;
EXEC SQL END DECLARE SECTION;
	TAGID tag = (TAGID) class->appl->data.tag;

	class->recmems = NULL;
	class_id = class->ooid;
	dbdesc.db_data = rdesc;

	Cur_class = class;

	EXEC SQL REPEATED SELECT
		r.class_id, o.object_id, o.object_name, 
		r.type_name, r.adf_length, r.adf_type, r.adf_scale,
		o.object_owner, o.short_remark, 
		o.create_date, o.alter_date, o.last_altered_by
	INTO :class_id, :catt_id, :name, 
		:dbdesc.db_data, :dbdesc.db_length, :dbdesc.db_datatype,
		:dbdesc.db_scale,
		:owner, :short_remark, 
		:create_date, :alter_date, :altered_by
	FROM ii_objects o, ii_abfclasses r
	WHERE r.class_id = :class_id AND o.object_id = r.catt_id
	ORDER BY o.object_name DESC; 
	EXEC SQL BEGIN;
	{
		catt = _makeobj( class, catt_id, tag);
		if (catt == NULL)
			return FAIL;

		catt->data.tag = tag;
		catt->class = OC_RECMEM;
		catt->ooid = catt_id;

		catt->name = FEtsalloc(tag, name);

		iiamStrAlloc(tag, short_remark, &(catt->short_remark));
		iiamStrAlloc(tag, create_date, &(catt->create_date));
		iiamStrAlloc(tag, altered_by, &(catt->altered_by));

		_VOID_ STtrmwhite(alter_date);
		iiamStrAlloc(tag, alter_date, &(catt->alter_date));
		iiamStrAlloc(tag, owner, &(catt->owner));

		catt->data_type.db_data = FEtsalloc(tag, dbdesc.db_data);
		catt->data_type.db_datatype = dbdesc.db_datatype;
		catt->data_type.db_length = dbdesc.db_length;
		catt->data_type.db_scale = dbdesc.db_scale;

		catt->att_order = 0;

		catt->parent = (APPL_COMP *) class;

		/* put this on the class's list of attributes */
		catt->next_mem = class->recmems;
		class->recmems = catt;
	}
	EXEC SQL END;

	return FEinqerr();
}

static RECMEM *
_makeobj ( class, id, tag )
RECDEF	*class;
OOID	id;
TAGID tag;
{
	register RECMEM	*obj = NULL;

	if ( class->appl->comps == NULL 
	  || (obj = (RECMEM *)iiamLkComp(class->appl, id)) == NULL)
	{ 
		/* allocate new object */
		register OO_CLASS	*cp;
		OO_CLASS *OOpclass();

		cp = OOpclass(OC_RECMEM);
		if ( cp == NULL  
		  || (obj = (RECMEM *)FEreqmem( tag, cp->memsize,
						TRUE, (STATUS *)NULL
					)) == NULL  
		  || OOhash(id, (OO_OBJECT *) obj) == 0 )
		{
			IIUGbmaBadMemoryAllocation(ERx("IIAMraReadAtts()"));
		}
		obj->data.dbObject = TRUE;
		obj->appl = class->appl;
		obj->data.levelbits = (1 | cp->level); /* set level 0 */
		IIAMinsComp(class->appl, obj);
	}
	obj->data.inDatabase = TRUE;
	return obj;
}

/*{
** Name:	IIAMwaWriteAtt() -	Write Class Attribute.
**
** Description:
**	Write a class attribute to the dbms.
**
** Inputs:
**	att	{RECMEM *} The attribute to write.
**
** Returns:
**	{STATUS}	OK		success
**			FAIL		database write failure
**
** History:
**	8-89 (billc) first written.
*/

static STATUS 	insertAtt();
static STATUS 	updateAtt();

STATUS
IIAMwaWriteAtt ( att )
EXEC SQL BEGIN DECLARE SECTION;
	RECMEM *att;
EXEC SQL END DECLARE SECTION;
{
	STATUS 	stat;

	if ( !IIUIinXaction() )
		iiuicw1_CatWriteOn();

	if (att->ooid == OC_UNDEFINED || !(att->data.dirty))
		return OK;

	if ( !att->data.inDatabase )
	{
		stat = insertAtt( att, att->appl->ooid, 
				att->parent->ooid, &(att->data_type)
			);
	}
	else
	{
		stat = updateAtt( att, att->appl->ooid, 
				att->parent->ooid, &(att->data_type)
			);
	}

	if (stat == OK)
	{
		att->data.inDatabase = TRUE;
		_VOID_ OOsndSelf( (OO_OBJECT*) att, ii_flush0 );	 
		att->data.dirty = FALSE;
	}

	if ( !IIUIinXaction() )
		iiuicw0_CatWriteOff();

	return stat;
}

/*{
** Name:	insertAtt() -	Insert New Class Attribute.
*/

static STATUS
insertAtt ( catt, applid, classid, desc )
EXEC SQL BEGIN DECLARE SECTION;
	RECMEM *catt;
	OOID applid;
	OOID classid;
	DB_DATA_DESC *desc;
EXEC SQL END DECLARE SECTION;
{
	EXEC SQL REPEATED INSERT INTO ii_abfclasses ( 
			appl_id, class_id, catt_id, class_order, 
			adf_type, type_name, adf_length, adf_scale
		)
	VALUES ( 
			:applid, :classid, :catt->ooid, 0,
			:desc->db_datatype, :desc->db_data, :desc->db_length,
			:desc->db_scale
		);
	return FEinqerr();
}

/*{
** Name:	updateAtt() -	update New Class Attribute.
*/

static STATUS
updateAtt ( catt, applid, classid, desc )
EXEC SQL BEGIN DECLARE SECTION;
	RECMEM *catt;
	OOID applid;
	OOID classid;
	DB_DATA_DESC *desc;
EXEC SQL END DECLARE SECTION;
{
	EXEC SQL REPEATED UPDATE ii_abfclasses SET
			class_order	=	0,
			adf_type	=	:desc->db_datatype, 
			type_name	=	:desc->db_data, 
			adf_length	=	:desc->db_length, 
			adf_scale	=	:desc->db_scale
		WHERE appl_id = :applid 
			AND class_id = :classid
			AND catt_id = :catt->ooid
		;
	return FEinqerr();
}

/*{
** Name:	IIAMdaDeleteAtt() -	Delete a single Class Attribute.
**
** Description:
**	Delete an attribute from a class.
**
** Inputs:
**	attr	{RECMEM *} The attribute we're deleting.
**
** Returns:
**	{STATUS}	OK		success
**			FAIL		database write failure
**
** History:
**	8-89 (billc) first written.
*/

STATUS
IIAMdaDeleteAtt ( attr )
EXEC SQL BEGIN DECLARE SECTION;
RECMEM *attr;
EXEC SQL END DECLARE SECTION;
{
	STATUS stat;

	if ( !IIUIinXaction() )
		iiuicw1_CatWriteOn();

	EXEC SQL REPEATED 
	DELETE FROM ii_abfclasses r
	WHERE r.catt_id = :attr->ooid;

	stat = FEinqerr();

	if ( !IIUIinXaction() )
		iiuicw0_CatWriteOff();

	return stat;
}

/*{
** Name:	IIAMccClearClass() -	Delete Class Attributes.
**
** Description:
**	Given a class definition object, delete all the associated class
**	attributes.
**
** Inputs:
**	class	{RECDEF *} The class whose attributes we're deleting.
**
** Returns:
**	{STATUS}	OK		success
**			FAIL		database write failure
**
** History:
**	8-89 (billc) first written.
*/

STATUS
IIAMccClearClass ( class )
EXEC SQL BEGIN DECLARE SECTION;
RECDEF *class;
EXEC SQL END DECLARE SECTION;
{
	STATUS	stat;

	if ( !IIUIinXaction() )
		iiuicw1_CatWriteOn();

	EXEC SQL REPEATED 
	DELETE FROM ii_abfclasses r
	WHERE r.class_id = :class->ooid;

	stat = FEinqerr();

	if ( !IIUIinXaction() )
		iiuicw0_CatWriteOff();

	return stat;
}

/*
** Hacked Methods.
**
** 	These methods employ a gruesome hack to overcome the fact that these
**	methods don't pass any parent information.  class attributes must
**	be uniquely named only within a class, so we just don't get enough
**	information to check the namespace.  So, we have to stash a pointer
**	to the currently-edited class definition.  This hack relies on the
**	fact that these methods are only called while a class is being edited,
**	and are called only on that class's attributes.
**
**	The right way to fix this would be to override the OO rename and
**	checkname methods, but this would create a lot of redundant code.
*/

/*{
** Name:        iiamDbRattName() -  Fetch Class Attribute by Name.
**
** Description:
**      Return ID of named class attribute.
**
** Input:
**      class   {OO_CLASS *}  	The class object for the class attribute.
**      name    {char *}  	The class attribute name.
**	appid	{OOID}		The application ID.
**
** Returns:
**      {OOID}  ID of class attribute if it exists.
**              nil otherwise.
**
** History:
**      08/89 (billc) -- Written.
*/

OOID
iiamDbRattName ( class, name, appid )
OO_CLASS	*class;
char		*name;
OOID		appid;
{
	if (Cur_class != NULL)
	{
		register RECMEM *rap;

		for (rap = Cur_class->recmems; rap != NULL; rap = rap->next_mem)
		{
			if (STequal(rap->name, name))
				return rap->ooid;
		}
	}
	return nil;
}

/*{
** Name:	IIAMgsrGetSpecialRecs() -	Read 'Special' Class Defs.
**
** Description:
**	This routine creates class definitions (RECDEF structs) for classes
**	that were defined in OSL "CLASS OF" declarations.  OSL wrote 
**	dependencies for these.  We'll read the dependencies and create 
**	the classes.
**
** Inputs:
**	appl	{APPL*}	The application.
**	tag	{TAGID}	The tag to allocate from.
**
** Outputs:
**	count	{nat*}	The number of classes created.
**	stat	{STATUS}	OK or FAIL.
**
** Returns:
**	{RECDEF*}	The list of classes created.
**
** History:
**	10-89 (billc) first written.
*/

static RECDEF*	ii_ldform();
static RECDEF*	ii_ldtable();
static RECDEF*	ii_mkclass();
static char*	ii_mkalias();

RECDEF *
IIAMgsrGetSpecialRecs ( appl, tag, count, stat )
APPL	*appl;
TAGID	tag;
i4	*count;
STATUS	*stat;
{
	ACATDEP *depList;
	ACATDEP	*iiamRrdReadRecordDeps();
	ACATDEP *dep;
	RECDEF 	*classlist = NULL;
	i4	lcount = 0;

	/* read the dependencies OSL created for 'special' classes. */
	depList = iiamRrdReadRecordDeps(appl->ooid, tag, stat);
	for (dep = depList; dep != NULL; dep = dep->_next)
	{
		RECDEF 	*class = NULL;

		if (dep->deptype == OC_DTFORM_TYPE)
		{
			/* CLASS OF FORM or CLASS OF TABLE FIELD */
			class = ii_ldform(appl, dep->name, tag);
		}
		else if (dep->deptype == OC_DTTABLE_TYPE)
		{
			/* CLASS OF TABLE */
			class = ii_ldtable(appl, dep->name, dep->owner, tag);
		}

		while (class != NULL)
		{
			RECDEF *tmp;

			tmp = (RECDEF*) class->_next;
			class->_next = (APPL_COMP*) classlist;
			classlist = class;
			class = tmp;
			lcount++;
		}
	}
	*count = lcount;
	return classlist;
}


/*{
** Name:	ii_mkclass() -- make a class and initialize
*/

static RECDEF *
ii_mkclass(appl, tag, cname, pname, name)
APPL	*appl;
TAGID	tag;
char	*cname;
char	*pname;
char	*name;
{
	RECDEF *class;

	class = (RECDEF *)FEreqmem(tag, sizeof(*class), TRUE, (STATUS *)NULL);
	if (class == NULL)
		return NULL;

	class->ooid = OC_UNDEFINED;
	class->class = OC_RECORD;
	class->appl = appl;
	class->recmems = NULL;
	class->_next = NULL;
	class->data.tag = tag;

	/* 'cname' is used for aliasing - so we can preserve namespaces. */
	class->name = ii_mkalias(tag, cname, pname, name);

	return class;
}

/*{
** Name:	ii_mkcatt() -- make an atttribute, put it on the class list.
*/

ii_mkcatt(class, name, dbv)
RECDEF	*class;
char	*name;
DB_DATA_VALUE	*dbv;
{
	RECMEM *catt;

	catt = (RECMEM *)FEreqmem((TAGID)class->data.tag, sizeof(*catt), 
			TRUE, (STATUS *)NULL);
	if (catt == NULL)
		return;

	catt->name = FEtsalloc(class->data.tag, name);

	if (dbv->db_data != NULL)
	{
		catt->data_type.db_data = 
				FEtsalloc(class->data.tag, dbv->db_data);
	}
	else
	{
		catt->data_type.db_data = NULL;
	}
			
	catt->data_type.db_datatype = dbv->db_datatype;
	catt->data_type.db_length = dbv->db_length;
	catt->data_type.db_scale = dbv->db_prec;

	/* put this on the attribute list of the class */
	catt->next_mem = class->recmems;
	class->recmems = catt;
}
/*{
** Name:	ii_mkalias() -- make an internal datatype name, for partitioning
**				namespace.
**
** WARNING: these aliases must match the ones produced by the OSL translator.
**	(These are generated in the osl/osrecord.sc module.)
*/

static char *
ii_mkalias(tag, classname, parentname, objname)
TAGID	tag;
char	*classname;
char	*parentname;
char	*objname;
{
	/* need a big buffer. */
	char	buf[(FE_MAXNAME * 6) + 3];

	if (classname == NULL)
		return objname;

	if (parentname == NULL)
	{
		_VOID_ STprintf(buf, ERx("%s %s"), classname, objname);
	}
	else
	{
		_VOID_ STprintf(buf, ERx("%s %s.%s"), classname, 
					parentname, objname);
	}
	return FEtsalloc(tag, buf);
}


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
** Name:	ii_ldform() -	Load form into class-definition format.
**
** Description:
**	Read a form and create class definitions, for CLASS OF FORM declaration
**	in OSL.
**
** Input:
**	appl	{APPL*}	the application.
**	formname	{char*}	the name of the form.
**	tag	{TAGID}	the tag to allocate from.
**
** Returns:
**	{RECDEF *}	The classes created.  A class is created for the
**			form itself, and one class is created for each
**			tablefield.
**
** History:
**	10/89 (billc) -- Stolen.
*/

static STATUS	read_field();

static RECDEF *
ii_ldform (appl, formname, tag)
APPL	*appl;
char	*formname;
TAGID	tag;
{
	RECDEF	*class;
	i4	frid;		/* form ID */
	char	dummy[256];
	STATUS	IIAMfoiFormInfo();
	STATUS	IIAMforFormRead();

	if (formname == NULL || *formname == EOS)
		return NULL;

	if (IIAMfoiFormInfo(formname, dummy, &frid) != OK
	  || frid == OC_UNDEFINED)
	{
		return NULL;
	}

	/* we have a form class. */
	class = ii_mkclass(appl, tag, FormPname, (char*)NULL, formname);

	/*
	** Step through the form retrieving all the fields
	** entering each into the class's attribute list.
	*/
	_VOID_ IIAMforFormRead(formname, frid, (PTR) class, read_field);
	return class;
}

static STATUS
read_field(fname, fldname, p, kind, dbv)
char 	*fname;
char 	*fldname;
PTR	p;
i4	kind;
DB_DATA_VALUE	*dbv;
{
	RECDEF	*class = (RECDEF *) p;
	RECDEF	*tab;
	DB_DATA_VALUE	ldbv;

	switch (kind)
	{
	  case FTABLE:
		/*
		** Tablefield.  For this we create a new class.  Also, the
		** tablefield is an attribute of the form's class.
		*/
		tab = ii_mkclass(class->appl, class->data.tag, 
				TfldPname, fname, fldname);
		ldbv.db_data = tab->name;
		ldbv.db_datatype = DB_DMY_TYPE;
		ldbv.db_length = 1;
		ldbv.db_prec = 0;
		ii_mkcatt(class, fldname, &ldbv);

		tab->_next = class->_next;
		class->_next = (APPL_COMP*) tab;
		break;

	  case FCOLUMN:
		/* This assumes that the most-recently read tablefield contains
		** this column.  We make a list of table fields, attached to 
		** the class definition, in the order in which we read them.
		*/
		ii_mkcatt((RECDEF*)class->_next, fldname, dbv);
		break;

	  case FREGULAR:
		ii_mkcatt(class, fldname, dbv);
		break;
	}	/* end switch */
	
	return OK;
}

/*{
** Name:	ii_ldtable() -	Load DBMS table into class-definition format.
**
** Description:
**
** Input:
**
** Returns:
**
** History:
**	10/89 (billc) -- written.
*/
/*{
** Name:	ii_ldtable() -	Load DBMS table into class-definition format.
**
** Description:
**	Read a DBMS table and create class definition, for a
**	"CLASS OF TABLE" declaration in OSL.
**
** Input:
**	appl	{APPL*}	the application.
**	tname	{char*}	the name of the table.
**	oname	{char*}	the owner of the table.
**	tag	{TAGID}	the tag to allocate from.
**
** Returns:
**	{RECDEF *}	The class created.  
**
** History:
**	10/89 (billc) -- Stolen.
**	23-feb-93 (davel)
**		Added NULL owner name argument to FEatt_open call (bug 49810).
*/

static RECDEF *
ii_ldtable (appl, tname, oname, tag)
APPL	*appl;
char	*tname;
char	*oname;
TAGID	tag;
{
        FE_ATT_QBLK     qblk;
	RECDEF		*class;
	FE_ATT_INFO     ainfo;
	DB_DATA_VALUE   dbv;

	/* the owner name could be NULL or empty, which meeans that there is
	** no explicit owner.  Take some insurance by passing NULL to the
	** FExxx routines.
	*/
	if (*oname == EOS)
		oname = NULL;

	if (FEatt_open(&qblk, tname, oname) != OK || qblk.status == STATUS_DONE)
		return NULL;

	class = ii_mkclass(appl, tag, TablePname, oname, tname);

	while(FEatt_fetch(&qblk,&ainfo) == OK)
	{
		FEatt_dbdata(&ainfo, &dbv);

		/* Weird.  FEatt_dbdata screws up the db_data pointer. */
		dbv.db_data = NULL;
		CVlower(ainfo.column_name);
		ii_mkcatt(class, ainfo.column_name, &dbv);
	}
	FEatt_close(&qblk);

	return class;
}
