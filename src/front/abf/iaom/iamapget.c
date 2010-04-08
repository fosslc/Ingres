/*
**	Copyright (c) 1989, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<me.h>
#include	<cv.h>
#include	<cm.h>
#include	<st.h>
#include	<ol.h>
#include	<nm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<abclass.h>
#include	<abfdbcat.h>
#include	<oocat.h>
#include	<oodep.h>
#include	<oodefine.h>
#include	<ilerror.h>
#include	<metafrm.h>
#include	"eram.h"

/**
** Name:	iamapget.c -	Application Object(s) Fetch Routine.
**
** Description:
**	Contains the routine used to fetch an application object from the DBMS.
**	This can either be an entire application or just one of its component
**	objects.  Defines:
**
**	IIAMapFetch()	fetch application object.
**
** History:
**	Revision 6.2  89/02  wong
**	Initial revision.  (Modified from 'IIAMcgCatGet()'.)
**
**	Abstracted out SQL fetch code into 'iiamCatRead()'.  89/05
**
**	Remove extension (which is system-dependent) from form reference
**		source-code name so applications can be ported to different
**		systems.  89/09
**
**	Revision 6.4
**	03/09/91 (emerson)
**		Integrated DESKTOP porting changes.
**	03/11/91 (emerson)
**		Integrated another set of DESKTOP porting changes.
**	02-dec-91 (leighb) DeskTop Porting Change:
**		Added routines to pass data across ABF/IAOM boundary.
**	04-mar-92 (leighb) DeskTop Porting Change:
**		Moved function declaration outside of calling function.
**		This is so function prototypes will work.
**
**	Revision 6.5
**	09-jan-92 (davel)
**		Change iiamFtConstVar() to store pre-6.5 constant language
**		values in value[lang] rather than in value[lang-1].
**	24-aug-92 (davel)
**		Set db_length for pre-6.5 global constant records (in
**		iiamFtConstVar() ).
**	20-oct-92 (davel)
**		Added special handling to upgrade global constants in the
**		catalogs, to phase out previous natural language scheme for
**		string constants.
**	08-dec-92 (davel)
**		Replaced IIolLang references with the official OL interface,
**		OLanguage().  Also made small addition to global
**		constant handling.
**	31-aug-1993 (mgw)
**		Casted a few pointers (tmpptr) correctly for MEcopy()
**		prototyping.
**	Revision  Ingres II 2.0 
**      11-jan-2001 (bolke01) 
**              Required change to avoid conflict with reserved word restrict.
**              Identified after latest compiler OSFCMPLRS440 installed on axp.osf
**              which includes support for this bases on the C9X review draft .
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

GLOBALREF STATUS	iiAMmemStat;
GLOBALREF bool		IIab_Vision;			 
	
FORM_REF	*iiamFoGetRef();
APPL_COMP	*iiamLkComp();
FORM_REF	*IIAMfaFormAlloc();
FUNC_EXTERN FORM_REF *IIAMformRef();

/*{
** Name:	IIAMapFetch() -	Fetch Application Object.
**
** Description:
**	Fetch catalog records and construct an ABF application object or
**	component object.  This passes the internal build function to
**	'iiamCatRead(),' which fetches the catalog records.
**
**	The application ID is provided as well as the objects own ID to allow
**	future use of the same object in multiple applications.  Also, if
**	OC_UNDEFINED is passed in for the object ID, the entire application
**	object including its components is returned.
**
**	Application components are kept as a sequenced list, which is part
**	of the application object.  This list is sorted by object ID.  (See
**	"iamlist.c".)
**
** Inputs:
**	app	{APPL *}  Application object structure.
**	objid	{OOID}  Object ID, == OC_UNDEFINED for entire application.
**	iirestrict {bool}	Whether to fetch a restricted set of dependencies
**
** Outputs:
**	app	{APPL *}  Reference to reference for object.
**
** Return:
**	{STATUS}	OK		success
**			ILE_NOOBJ	doesn't exist
**			ILE_NOMEM	allocation failure
**			ILE_FAIL	query failed
**
** History:
**	02/89 (jhw)  Written from 'IIAMcgCatGet()' for new application objects.
**	10/89 (Mike S)  Add fetch of compdate and flags where applicable
**	01/90 (jhw)  On check for form reference look-up call 'iiamLkComp()' to
**		get object rather than 'OOp()'.  This removes unnecessary DB
**		access when originally called to get a form reference anyway.
**	10/90 (Mike S) Update compdate and flags even if alter date hasn't 
**			changed.
**	20-oct-92 (davel)
**		Added special handling for upgrading global constants for 6.5.
**      11-jan-2001 (bolke01) 
**              Required change to avoid conflict with reserved word restrict.
**              Identified after latest compiler OSFCMPLRS440 installed on axp.osf
**              which includes support for this bases on the C9X review draft .
*/

static STATUS	process_row();
static STATUS	(*_send())();
static STATUS 	iiamFtAppl();
static STATUS 	iiamFtMetaFrame();
static STATUS 	iiamFtUserFrame();
static VOID 	userDeps();
static STATUS 	iiamFtQBFFrame();
static VOID 	QBFFrameDeps();
static STATUS 	iiamFtRepFrame();
static VOID 	reportDeps();
static STATUS 	iiamFtGraphFrame();
static VOID 	graphDeps();
static STATUS 	iiamFt4GLProc();
static VOID 	_4GLprocDeps();
static STATUS 	iiamFtHLProc();
static STATUS 	iiamFtDBProc();
static STATUS 	iiamFtForm();
static STATUS 	iiamFtConstVar();
static STATUS 	iiamFtGlobVar();
static STATUS 	iiamFtRecordDef();
static STATUS	fixup_oldoslframes();
static STATUS	wehaveoldosl();
static VOID 	release_comps();

static bool Entire_app;
static bool Has_meta;

static bool Any_str_constants;
static bool Any_default_str_constants;
static i4   Any_lang_code;

/* Describe OLDOSL FRAMES in application */
# define INIT_IDS_SIZE 	50

static struct
{
	i4	num_frames;
	OOID	*ids;
	i4	ids_size;
} oldoslframes;


STATUS
IIAMapFetch ( app, objid, iirestrict )
register APPL	*app;
OOID		objid;
bool		iirestrict;
{
	register APPL_COMP	*obj;
	STATUS			stat = OK;

	Entire_app = (objid == OC_UNDEFINED);
	Has_meta = FALSE;

	if ( app->data.tag == 0 )
		app->data.tag = FEgettag();

	if ( (stat = IIAMdmrReadDeps( app->ooid, objid, iirestrict, 
				    (VOID (*)())NULL, app ))
	    != OK )
		return ILE_FAIL;

	oldoslframes.num_frames = 0;
	oldoslframes.ids = NULL;
	oldoslframes.ids_size = 0;
	Any_str_constants = FALSE;
	Any_default_str_constants = FALSE;
	Any_lang_code = 0;
	stat = iiamCatRead(app->ooid, objid, process_row, (PTR)app);
	if (oldoslframes.num_frames)
		fixup_oldoslframes();

	if (Any_str_constants && !Any_default_str_constants && Entire_app)
	{
		/* fix up the constants in the catalogs, and then re-fetch 
		** the components.
		*/
		char old_lang_string[ER_MAX_LANGSTR + 1];

		(void) ERlangstr( Any_lang_code, old_lang_string );
		iiuicw1_CatWriteOn();
		(void) IIAMugcUpdateGlobCons(app->ooid, 
						APC_CONSTANT_LANGUAGE,
						old_lang_string);
		iiuicw0_CatWriteOff();
	}
	if ( stat == ILE_NOMEM )
	{ /* allocation failed, release partial allocation */
		release_comps(app);
		return ILE_NOMEM;
	}
	else if ( stat != OK )
	{
		return ILE_FAIL;
	}
	else if ( FEinqrows() == 0 )
	{
		return ILE_NOOBJ;
	}

	/* Check for form references when called to fetch specific
	** component for application that might have a form reference.
	*/
	if ( objid != OC_UNDEFINED && app->ooid != objid &&
			(obj = iiamLkComp(app, objid)) != NULL &&
			( obj->class == OC_OSLFRAME ||
				obj->class == OC_RWFRAME ||
		  		obj->class == OC_MUFRAME ||
		  		obj->class == OC_APPFRAME ||
		  		obj->class == OC_UPDFRAME ||
		  		obj->class == OC_BRWFRAME ||
				obj->class == OC_OLDOSLFRAME ) )
	{
		register FORM_REF	*form;

		form = ( obj->class == OC_RWFRAME )
				? ((REPORT_FRAME *)obj)->form
				: ((USER_FRAME *)obj)->form;
		if ( form != NULL && form->ooid <= 0 )
			_VOID_ IIAMformRef(app, form->name);
	}

	/* If we're Emerald reading in the whole application, build the tree */
	if (Has_meta)
		IIAMxbtBuildTree(app);
	return stat;
}

static OOID	Applerr = OC_UNDEFINED;

IIAMuoUnkObj(appid,class)
OOID appid;
OOID class;
{
	i4 type;

	type = class;

	if (Applerr != appid)
	{
		IIUGerr(E_AM0016_badobj, 0, 1, (PTR) &type);
		Applerr = appid;
	}
}

static STATUS
process_row ( abf_obj, data )
register ABF_DBCAT	*abf_obj;
PTR			data;
{
	register APPL	*app = (APPL *)data;
	STATUS		(*fn)();

	fn = _send(abf_obj->class);
	if (fn == NULL)
	{
		IIAMuoUnkObj(app->ooid, abf_obj->class);
		return OK;
	}

	_VOID_ STtrmwhite(abf_obj->alter_date);

	if ( abf_obj->class != OC_APPL &&
		abf_obj->class != OC_AFORMREF &&
				abf_obj->class != OC_FORM &&
		( app->comps == NULL || 
			iiamLkComp(app, abf_obj->ooid) == NULL ) )
	{ /* allocate new object */
		register OO_CLASS	*cp;
		register APPL_COMP	*obj;

		cp = OOpclass(abf_obj->class);
		if ( cp == NULL  || (obj = (APPL_COMP *)FEreqmem( app->data.tag,
							cp->memsize,
							TRUE, (STATUS *)NULL
					)) == NULL  ||
				OOhash(abf_obj->ooid, obj) == 0 )
		{
			IIUGbmaBadMemoryAllocation(ERx("IIAMapFetch()"));
		}
		obj->data.dbObject = TRUE;

		obj->data.tag = app->data.tag;
		obj->class = cp->ooid;
		obj->ooid = abf_obj->ooid;
		obj->appl = app;

		obj->data.levelbits = 1;	/* set level 0 */
		BTsetF(cp->level, obj->data.levelbits);

		/* 
		** We have to set the name so that the component goes into the 
		** hash table correctly, then clear it, so iiamStrAlloc
		** will make a new copy.
		*/
		obj->name = abf_obj->name;
		IIAMinsComp(app, obj);
		obj->name = NULL;
	}
	return (*fn)( app, abf_obj, FALSE );
}

static void
release_comps (app)
APPL	*app;
{
	APPL_COMP *obj;

	for ( obj = (APPL_COMP *)app->comps ; obj != NULL ; obj = obj->_next )
	{
		IIOOforget((OO_OBJECT *)obj);
	}
	IIUGtagFree(app->data.tag);
	app->data.levelbits = 0;
}

DB_LANG	IIUGstrDml();		 

/*ARGSUSED*/
static STATUS
iiamFtAppl ( app, abfobj, dummy )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			dummy;
{
	char	*opt;

	if ( !app->data.inDatabase || app->alter_date == NULL ||
			!STequal(app->alter_date, abfobj->alter_date) )
	{
		app->version = abfobj->version;
		iiamStrAlloc( app->data.tag, abfobj->name, &app->name );
		app->ooid = abfobj->ooid;
		if ( app->owner == NULL )
			iiamStrAlloc(app->data.tag, abfobj->owner, &app->owner);
		iiamStrAlloc( app->data.tag, abfobj->short_remark,
				&app->short_remark
		);
		app->long_remark = NULL;
		iiamStrAlloc( app->data.tag, abfobj->altered_by,
				&app->altered_by
		);
		if ( app->create_date == NULL )
			iiamStrAlloc( app->data.tag, abfobj->create_date,
					&app->create_date
			);
		iiamStrAlloc( app->data.tag, abfobj->alter_date,
				&app->alter_date
		);
		iiamStrAlloc( app->data.tag, abfobj->source, &app->source );
		iiamStrAlloc( app->data.tag, abfobj->AppExecutable,
				&app->executable
		);
		iiamStrAlloc( app->data.tag, abfobj->AppStartName,
				&app->start_name
		);
		iiamStrAlloc( app->data.tag, abfobj->AppLinkFile,
				&app->link_file
		);
		iiamStrAlloc( app->data.tag, abfobj->AppRole,
				&app->roleid
		);
		app->password = NULL;
		NMgtAt(ERx("ING_ABFOPT1"), &opt);
		if ( ( app->link_file == NULL || *app->link_file == EOS ) &&
				opt != NULL && *opt != EOS )
		{
			app->link_file = FEtsalloc(app->data.tag, opt);
		}
		app->dml = DB_NOLANG;
		if ( !CMdigit(abfobj->AppDML) )
			app->dml = IIUGstrDml(abfobj->AppDML);
		else
		{ /* convert integer from converted 5.0 application */
			i4	dml;

			if ( STtrmwhite(abfobj->AppDML) > 0 &&
					CVan(abfobj->AppDML, &dml) == OK )
			{
				app->dml = dml;
			}
		}
		if (app->dml == DB_NOLANG)
			app->dml = DB_QUEL;
		app->start_proc = (app->batch =
			( (abfobj->retlength & APPL_BATCH) == APPL_BATCH )) ||
				( (abfobj->retlength & START_PROC) != 0 );

		app->data.inDatabase = TRUE;
		app->data.dirty = FALSE;

		app->data.levelbits = 1;	/* set level 0 */
		app->flags = (i4)abfobj->flags;
		BTsetF(OOpclass(OC_APPL)->level, app->data.levelbits);
	}

	return iiAMmemStat;
}

/*ARGSUSED*/
static STATUS
iiamFtMetaFrame( app, abfobj, dummy  )
APPL		*app;
ABF_DBCAT	*abfobj;
bool		dummy;
{
	register USER_FRAME	*frm;
	STATUS			rc;

	struct _metaframe	*savemf;

	savemf = ( (frm = (USER_FRAME *) OOp(abfobj->ooid)) != NULL )
			? frm->mf : NULL;

	/* 
	** If we're in Emerald, and we're reading in the whole application,
	** rebuild the tree.
	*/
	Has_meta = IIab_Vision && Entire_app;		 
	rc = iiamFtUserFrame( app, abfobj, Has_meta);
	
#ifndef NOVISION
	/*
	** If we are in ABF only (not Emerald), we must still attach a 
	** metaframe, but don't pay attention to menu dependencies.
	*/
	if (rc == OK)
	{
		if (!IIab_Vision)		 
		{
			if (savemf != NULL)
				frm->mf = savemf;
			else
			{
				IIAMamAttachMeta(frm);
				(frm->mf)->flags |= MF_ABFONLY;
			}	
		}
	}
	IIAMigdInitGenDates(frm, abfobj->arg4, abfobj->arg5);
#endif
	return rc;
}

static STATUS
iiamFtUserFrame ( app, abfobj, isMeta )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			isMeta;	/* We're Emerald, rebuilding the tree */
{
	register USER_FRAME	*frm = (USER_FRAME *)OOp(abfobj->ooid);

	if ( !frm->data.inDatabase || frm->alter_date == NULL ||
			!STequal(frm->alter_date, abfobj->alter_date) )
	{
		frm->version = abfobj->version;
		iiamStrAlloc( frm->data.tag, abfobj->name, &frm->name );
		frm->ooid = abfobj->ooid;
		if ( frm->owner == NULL )
			iiamStrAlloc(frm->data.tag, abfobj->owner, &frm->owner);
		iiamStrAlloc( frm->data.tag, abfobj->short_remark,
				&frm->short_remark
		);
		frm->long_remark = NULL;
		iiamStrAlloc( frm->data.tag, abfobj->altered_by,
				&frm->altered_by
		);
		if ( frm->create_date == NULL )
			iiamStrAlloc( frm->data.tag, abfobj->create_date,
					&frm->create_date
			);
		iiamStrAlloc( frm->data.tag, abfobj->alter_date,
				&frm->alter_date
		);
		iiamStrAlloc( frm->data.tag, abfobj->source, &frm->source );
		iiamStrAlloc( frm->data.tag, abfobj->symbol, &frm->symbol );
		frm->return_type.db_datatype = abfobj->retadf_type;
		frm->return_type.db_length = abfobj->retlength;
		frm->return_type.db_scale = abfobj->retscale;
		iiamStrAlloc( frm->data.tag, abfobj->rettype,
				&frm->return_type.db_data
		);

		frm->is_static = STequal(abfobj->UserStatic, ERx("static"));

		frm->data.inDatabase = TRUE;
		frm->data.dirty = FALSE;
	
	}

	/* Set flags and compdate, even if alter_date hasn't changed */
	iiamStrAlloc( frm->data.tag, abfobj->arg3, &frm->compdate );
	frm->flags = (i4)abfobj->flags;

	/* Attach metaframe */
	if (isMeta)
	{
		IIAMamAttachMeta(frm);
		(frm->mf)->flags &= ~MF_ABFONLY;
	}

	/* Get the dependencies */
	IIAMxpdProcessDeps((APPL_COMP *)frm, OC_DTDBREF, OC_DTDBREF, 
			  (isMeta ? OC_DTMREF : OC_DTDBREF),
			  userDeps, (PTR)NULL);

	return iiAMmemStat;
}

/*ARGSUSED*/
static VOID
userDeps(comp, data, name, class, deptype, linkname, deporder)
APPL_COMP *comp;         /* Component which has dependency */
PTR     data;           /* Passed data */
char    *name;          /* Dependee name */
OOID    class;          /* Dependee class */
OOID    deptype;        /* Dependency type */
char *  linkname;       /* Dependency name */
i4      deporder;       /* Dependency order */
{
	/* OC_AFORMREF and OC_ILCODE dependencies are required. */

	USER_FRAME *frm = (USER_FRAME *)comp;

	/* 
	** We can't build the AFD tree here.  The components aren't all in
	** memory yet.  We'll do it in IIAMxbtBuild Tree, called above.
	*/
	if (deptype == OC_DTMREF)
		return;

	/* Form and IL dependencies here */
	switch(class)
	{
	  case OC_AFORMREF:
	  case OC_FORM:
	  {
		register FORM_REF	*form;

		form = iiamFoGetRef( frm->appl, name );
		if (frm->form != form)
		{
			if ( frm->form != NULL )
				--(frm->form->refs);
			frm->form = form;
			++form->refs;
		}
		break;
	  }
	  case OC_ILCODE:
	  {
		OOID	fid;
		char	*dnarg;

		dnarg = (*(name) == 'f')
			? (name + 3) : name;
		frm->fid = CVal(dnarg,&fid) != OK ? 0 : fid;
		break;
	  }
	  default:
		break;
	}
}

/*ARGSUSED*/
static STATUS
iiamFtQBFFrame ( app, abfobj, dummy  )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			dummy;
{
	register QBF_FRAME	*frm = (QBF_FRAME *)OOp(abfobj->ooid);

	if ( !frm->data.inDatabase || frm->alter_date == NULL ||
			!STequal(frm->alter_date, abfobj->alter_date) )
	{
		frm->version = abfobj->version;
		iiamStrAlloc( frm->data.tag, abfobj->name, &frm->name );
		frm->ooid = abfobj->ooid;
		if ( frm->owner == NULL )
			iiamStrAlloc(frm->data.tag, abfobj->owner, &frm->owner);
		iiamStrAlloc( frm->data.tag, abfobj->short_remark,
				&frm->short_remark
		);
		iiamStrAlloc( frm->data.tag, abfobj->altered_by,
				&frm->altered_by
		);
		frm->long_remark = NULL;
		if ( frm->create_date == NULL )
			iiamStrAlloc( frm->data.tag, abfobj->create_date,
					&frm->create_date
			);
		iiamStrAlloc( frm->data.tag, abfobj->alter_date,
				&frm->alter_date
		);
		iiamStrAlloc( frm->data.tag, abfobj->QBFCmd, &frm->cmd_flags );
		frm->qry_type = CMcmpnocase(abfobj->QBFQryType, ERx("j")) == 0
					? OC_JOINDEF : 0;
		frm->flags = (i4)abfobj->flags;
		frm->form = NULL;	/* cleared since it's not required. */

		frm->data.inDatabase = TRUE;
		frm->data.dirty = FALSE;
	}

	/* Get the dependencies */
	IIAMxpdProcessDeps((APPL_COMP *)frm, OC_DTDBREF, OC_DTDBREF, OC_DTDBREF,
			  QBFFrameDeps, (PTR)NULL);

	return iiAMmemStat;
}

/*ARGSUSED*/
static VOID
QBFFrameDeps(comp, data, name, class, deptype, linkname, deporder)
APPL_COMP *comp;         /* Component which has dependency */
PTR     data;           /* Passed data */
char    *name;          /* Dependee name */
OOID    class;          /* Dependee class */
OOID    deptype;        /* Dependency type */
char *  linkname;       /* Dependency name */
i4      deporder;       /* Dependency order */
{
	QBF_FRAME *frm = (QBF_FRAME *)comp;

	if ( class == OC_AFORMREF || class == OC_FORM )
		frm->form = IIAMfaFormAlloc( frm->appl, name );
	else if ( class == OC_JOINDEF )
	{
		iiamStrAlloc( frm->data.tag, name, &frm->qry_object );
	}
}

/*ARGSUSED*/
static STATUS
iiamFtRepFrame ( app, abfobj, dummy  )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			dummy;
{
	register REPORT_FRAME	*frm = (REPORT_FRAME *)OOp(abfobj->ooid);

	if ( !frm->data.inDatabase || frm->alter_date == NULL ||
			!STequal(frm->alter_date, abfobj->alter_date) )
	{
		frm->version = abfobj->version;
		iiamStrAlloc( frm->data.tag, abfobj->name, &frm->name );
		frm->ooid = abfobj->ooid;
		if ( frm->owner == NULL )
			iiamStrAlloc(frm->data.tag, abfobj->owner, &frm->owner);
		iiamStrAlloc( frm->data.tag, abfobj->short_remark,
				&frm->short_remark
		);
		frm->long_remark = NULL;
		iiamStrAlloc( frm->data.tag, abfobj->altered_by,
				&frm->altered_by
		);
		if ( frm->create_date == NULL )
			iiamStrAlloc( frm->data.tag, abfobj->create_date,
					&frm->create_date
			);
		iiamStrAlloc( frm->data.tag, abfobj->alter_date,
				&frm->alter_date
		);
		iiamStrAlloc( frm->data.tag, abfobj->source, &frm->source );
		iiamStrAlloc( frm->data.tag, abfobj->RepCmd, &frm->cmd_flags );
		iiamStrAlloc( frm->data.tag, abfobj->RepOutput, &frm->output );
		frm->flags = (i4)abfobj->flags;
		if ( frm->form != NULL )
			--(frm->form->refs);
		frm->form = NULL;	/* cleared since it's not required. */

		frm->data.inDatabase = TRUE;
		frm->data.dirty = FALSE;
	}

	/* Get the dependencies */
	IIAMxpdProcessDeps((APPL_COMP *)frm, OC_DTDBREF, OC_DTDBREF, OC_DTDBREF,
			  reportDeps, (PTR)NULL);

	return iiAMmemStat;
}

/*ARGSUSED*/
static VOID
reportDeps(comp, data, name, class, deptype, linkname, deporder)
APPL_COMP *comp;         /* Component which has dependency */
PTR     data;           /* Passed data */
char    *name;          /* Dependee name */
OOID    class;          /* Dependee class */
OOID    deptype;        /* Dependency type */
char *  linkname;       /* Dependency name */
i4      deporder;       /* Dependency order */
{
	/* Only the OC_REPORT dependency is required. */
	REPORT_FRAME *frm = (REPORT_FRAME *)comp;

	if ( class == OC_AFORMREF || class == OC_FORM )
	{
		frm->form = iiamFoGetRef( frm->appl, name );
		++(frm->form->refs);
	}
	else if ( class == OC_REPORT ||
			class == OC_RWREP ||
				class == OC_RBFREP )
	{
		iiamStrAlloc( frm->data.tag, name, &frm->report );
	}
}

/*ARGSUSED*/
static STATUS
iiamFtGraphFrame ( app, abfobj, dummy  )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			dummy;
{
	register GRAPH_FRAME	*frm = (GRAPH_FRAME *)OOp(abfobj->ooid);

	if ( !frm->data.inDatabase || frm->alter_date == NULL ||
			!STequal(frm->alter_date, abfobj->alter_date) )
	{
		frm->version = abfobj->version;
		iiamStrAlloc( frm->data.tag, abfobj->name, &frm->name );
		frm->ooid = abfobj->ooid;
		if ( frm->owner == NULL )
			iiamStrAlloc(frm->data.tag, abfobj->owner, &frm->owner);
		iiamStrAlloc( frm->data.tag, abfobj->short_remark,
				&frm->short_remark
		);
		frm->long_remark = NULL;
		iiamStrAlloc( frm->data.tag, abfobj->altered_by,
				&frm->altered_by
		);
		if ( frm->create_date == NULL )
			iiamStrAlloc( frm->data.tag, abfobj->create_date,
					&frm->create_date
			);
		iiamStrAlloc( frm->data.tag, abfobj->alter_date,
				&frm->alter_date
		);
		frm->flags = (i4)abfobj->flags;
		iiamStrAlloc(frm->data.tag, abfobj->GraphCmd, &frm->cmd_flags);
		iiamStrAlloc(frm->data.tag, abfobj->GraphOutput, &frm->output);

		frm->data.inDatabase = TRUE;
		frm->data.dirty = FALSE;
	}

	/* Get the dependencies */
	IIAMxpdProcessDeps((APPL_COMP *)frm, OC_DTDBREF, OC_DTDBREF, OC_DTDBREF,
			  graphDeps, (PTR)NULL);

	return iiAMmemStat;
}

/*ARGSUSED*/
static VOID
graphDeps(comp, data, name, class, deptype, linkname, deporder)
APPL_COMP *comp;         /* Component which has dependency */
PTR     data;           /* Passed data */
char    *name;          /* Dependee name */
OOID    class;          /* Dependee class */
OOID    deptype;        /* Dependency type */
char *  linkname;       /* Dependency name */
i4      deporder;       /* Dependency order */
{
	GRAPH_FRAME *frm = (GRAPH_FRAME *)comp;

	if ( class == OC_GRAPH )
	{
		iiamStrAlloc(frm->data.tag, name, &frm->graph );
	}
}

/*ARGSUSED*/
static STATUS
iiamFt4GLProc ( app, abfobj, dummy  )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			dummy;
{
	register _4GLPROC	*proc = (_4GLPROC *)OOp(abfobj->ooid);

	if ( !proc->data.inDatabase || proc->alter_date == NULL ||
			!STequal(proc->alter_date, abfobj->alter_date) )
	{
		proc->version = abfobj->version;
		iiamStrAlloc( proc->data.tag, abfobj->name, &proc->name );
		proc->ooid = abfobj->ooid;
		if ( proc->owner == NULL )
			iiamStrAlloc( proc->data.tag, abfobj->owner,
					&proc->owner
			);
		iiamStrAlloc( proc->data.tag, abfobj->short_remark,
				&proc->short_remark
		);
		proc->long_remark = NULL;
		iiamStrAlloc( proc->data.tag, abfobj->altered_by,
				&proc->altered_by
		);
		if ( proc->create_date == NULL )
			iiamStrAlloc( proc->data.tag, abfobj->create_date,
					&proc->create_date
			);
		iiamStrAlloc( proc->data.tag, abfobj->alter_date,
				&proc->alter_date
		);
		iiamStrAlloc( proc->data.tag, abfobj->source, &proc->source );
		iiamStrAlloc( proc->data.tag, abfobj->symbol, &proc->symbol );
		proc->return_type.db_datatype = abfobj->retadf_type;
		proc->return_type.db_length = abfobj->retlength;
		proc->return_type.db_scale = abfobj->retscale;
		iiamStrAlloc( proc->data.tag, abfobj->rettype,
				&proc->return_type.db_data
		);

		proc->data.inDatabase = TRUE;
		proc->data.dirty = FALSE;
	}

	/* Set flags and compdate, even if alter_date hasn't changed */
	iiamStrAlloc( proc->data.tag, abfobj->arg3, &proc->compdate );
	proc->flags = (i4)abfobj->flags;

	/* Get the ILCODE dependency */
	IIAMxpdProcessDeps((APPL_COMP *)proc, OC_DTDBREF, OC_DTDBREF, OC_DTDBREF,
			  _4GLprocDeps, (PTR)NULL);

	return iiAMmemStat;
}

/*ARGSUSED*/
static VOID
_4GLprocDeps(comp, data, name, class, deptype, linkname, deporder)
APPL_COMP *comp;         /* Component which has dependency */
PTR     data;           /* Passed data */
char    *name;          /* Dependee name */
OOID    class;          /* Dependee class */
OOID    deptype;        /* Dependency type */
char *  linkname;       /* Dependency name */
i4      deporder;       /* Dependency order */
{
	_4GLPROC *proc = (_4GLPROC *)comp;

	if ( class == OC_ILCODE )
	{
		OOID	fid;

		proc->fid = CVal( *name == 'f'
					? name + 3 : name,
				  &fid ) != OK ? 0 : fid;
	}
}

/*ARGSUSED*/
static STATUS
iiamFtHLProc ( app, abfobj, dummy  )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			dummy;
{
	register HLPROC	*proc = (HLPROC *)OOp(abfobj->ooid);

	if ( !proc->data.inDatabase || proc->alter_date == NULL ||
			!STequal(proc->alter_date, abfobj->alter_date) )
	{
		proc->version = abfobj->version;
		iiamStrAlloc( proc->data.tag, abfobj->name, &proc->name );
		proc->ooid = abfobj->ooid;
		if ( proc->owner == NULL )
			iiamStrAlloc( proc->data.tag, abfobj->owner,
					&proc->owner
			);
		iiamStrAlloc( proc->data.tag, abfobj->short_remark,
				&proc->short_remark
		);
		proc->long_remark = NULL;
		iiamStrAlloc( proc->data.tag, abfobj->altered_by,
				&proc->altered_by
		);
		if ( proc->create_date == NULL )
			iiamStrAlloc( proc->data.tag, abfobj->create_date,
					&proc->create_date
			);
		iiamStrAlloc( proc->data.tag, abfobj->alter_date,
				&proc->alter_date
		);
		iiamStrAlloc( proc->data.tag, abfobj->source, &proc->source );
		iiamStrAlloc( proc->data.tag, abfobj->symbol, &proc->symbol );
		proc->return_type.db_datatype = abfobj->retadf_type;
		proc->return_type.db_length = abfobj->retlength;
		proc->return_type.db_scale = abfobj->retscale;
		iiamStrAlloc( proc->data.tag, abfobj->rettype,
				&proc->return_type.db_data
		);
		if ( proc->ol_lang <= 0 )
		{ /* get language type */
			register i4	i;
			register char	*lang_str;

			for ( i = OLOSL ; --i >= OLC ; )
			{
				lang_str = OLanguage(i);
				if ( (lang_str != (char *)NULL)
					&& STbcompare( lang_str, 0,
							abfobj->HostLanguage, 0,
							TRUE )
						== 0 )
				{
					break;
				}
			}
			proc->ol_lang = i; /* note if unsupported, i == -1 */
		}

		proc->data.inDatabase = TRUE;
		proc->data.dirty = FALSE;
	}

	/* Set flags and compdate, even if alter_date hasn't changed */
	iiamStrAlloc( proc->data.tag, abfobj->arg3, &proc->compdate );
	proc->flags = (i4)abfobj->flags;

	return iiAMmemStat;
}

/*ARGSUSED*/
static STATUS
iiamFtDBProc ( app, abfobj )
register APPL		*app;
register ABF_DBCAT	*abfobj;
{
	register DBPROC	*proc = (DBPROC *)OOp(abfobj->ooid);

	if ( !proc->data.inDatabase || proc->alter_date == NULL ||
			!STequal(proc->alter_date, abfobj->alter_date) )
	{
		proc->version = abfobj->version;
		iiamStrAlloc( proc->data.tag, abfobj->name, &proc->name );
		proc->ooid = abfobj->ooid;
		if ( proc->owner == NULL )
			iiamStrAlloc( proc->data.tag, abfobj->owner,
					&proc->owner
			);
		iiamStrAlloc( proc->data.tag, abfobj->short_remark,
				&proc->short_remark
		);
		proc->long_remark = NULL;
		iiamStrAlloc( proc->data.tag, abfobj->altered_by,
				&proc->altered_by
		);
		if ( proc->create_date == NULL )
			iiamStrAlloc( proc->data.tag, abfobj->create_date,
					&proc->create_date
			);
		iiamStrAlloc( proc->data.tag, abfobj->alter_date,
				&proc->alter_date
		);
		proc->return_type.db_datatype = abfobj->retadf_type;
		proc->return_type.db_length = abfobj->retlength;
		proc->return_type.db_scale = abfobj->retscale;
		iiamStrAlloc( proc->data.tag, abfobj->rettype,
				&proc->return_type.db_data
		);
		proc->flags = (i4)abfobj->flags;

		proc->data.inDatabase = TRUE;
		proc->data.dirty = FALSE;
	}

	return iiAMmemStat;
}

/*
**
** Description:
**	Load form reference application component with values from the database.
**
**	Form references are referenced by other application components in
**	addition to being components in their own right.  Since a reference may
**	be made before it is fetched from the database (and because it cannot be
**	fetched concurrently) it may be placed in the application components
**	using a temporary ID as a place holder.  This is handled by the
**	'iiamFoGetRef()' routine.
**
** History:
**	03/90 (jhw) -- Check for temporary form reference being filled in with
**		true values (old ID != new ID); this requires that the form
**		reference be moved in the components list.  JupBug #8516.
*/
/*ARGSUSED*/
static STATUS
iiamFtForm ( app, abfobj, dummy  )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			dummy;
{
	register FORM_REF	*form = iiamFoGetRef(app, abfobj->name);

	if ( !form->data.inDatabase || form->alter_date == NULL ||
			!STequal(form->alter_date, abfobj->alter_date) )
	{
		register char	*cp;

		form->version = abfobj->version;
		if ( form->name == NULL || *form->name == EOS )
			iiamStrAlloc(form->data.tag, abfobj->name, &form->name);
		if ( form->ooid != abfobj->ooid )
		{ /* move form reference in list */
			iiamMvComp(app, (APPL_COMP *)form, abfobj->ooid);
		}
		form->owner = _;
		form->short_remark = _;
		form->long_remark = NULL;
		form->altered_by = _;
		form->create_date = _;
		iiamStrAlloc( form->data.tag, abfobj->alter_date,
				&form->alter_date
		);
		if ( (cp = STindex(abfobj->source, ".", 0)) != NULL )
			*cp = EOS;	/* remove any extension */
		iiamStrAlloc( form->data.tag, abfobj->source, &form->source );
		iiamStrAlloc( form->data.tag, abfobj->symbol, &form->symbol );
		form->flags = (i4)abfobj->flags;

		form->data.inDatabase = TRUE;
		form->data.dirty = FALSE;
	}

	return iiAMmemStat;
}

static i4  Prefered_lang_code = 0;

/*ARGSUSED*/
static STATUS
iiamFtConstVar ( app, abfobj, dummy  )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			dummy;
{
    register CONSTANT	*constant = (CONSTANT *)OOp(abfobj->ooid);
    i4			lang;
    
    if ( !constant->data.inDatabase || constant->alter_date == NULL
		|| !STequal(constant->alter_date, abfobj->alter_date) )
    { /* new object */
	constant->version = abfobj->version;
        iiamStrAlloc( constant->data.tag, abfobj->name, &constant->name );
        constant->ooid = abfobj->ooid;
        if ( constant->owner == NULL )
            iiamStrAlloc( constant->data.tag, abfobj->owner, &constant->owner );
        iiamStrAlloc( constant->data.tag, abfobj->short_remark,
                     &constant->short_remark
        );
        constant->long_remark = NULL;
	iiamStrAlloc( constant->data.tag, abfobj->altered_by,
			&constant->altered_by
	);
        if ( constant->create_date == NULL )
            iiamStrAlloc( constant->data.tag, abfobj->create_date,
                         &constant->create_date
            );
        iiamStrAlloc( constant->data.tag, abfobj->alter_date,
			&constant->alter_date
	);
		
        constant->data_type.db_datatype = abfobj->retadf_type;
        constant->data_type.db_scale = abfobj->retscale;
        iiamStrAlloc( constant->data.tag, abfobj->rettype,
                     &constant->data_type.db_data
        );

	constant->flags = (i4)abfobj->flags;
        
        constant->data.inDatabase = TRUE;
        constant->data.dirty = FALSE;
    }

    /* Constant value
    **
    **	Constant values are a special case since multiple-rows can be returned
    **	from the database, so they must be allocated everytime this routine is
    **	called. Note only constants created before 6.5 will have rows
    **	that succeed on the ERlangcode check below.
    */
    if ( STtrmwhite(abfobj->arg0) <= 0 )
    {
	lang = 0;
    }
    else if ( ERlangcode(abfobj->arg0, &lang) != OK )
    {
	lang = 0;
	Any_str_constants = TRUE;
	Any_default_str_constants = TRUE;
    }
    iiamStrAlloc( constant->data.tag, abfobj->source, &constant->value[lang] );

    /* if we haven't encountered any of the "default" string constants yet,
    ** and this constant is an old lenguage-specific one, then set value[0],
    ** and update the value of Any_lang_code.
    */
    if (!Any_default_str_constants && lang > 0)
    {
	Any_str_constants = TRUE;
	if (Prefered_lang_code == 0)
	{
		/* only do this once */
		(VOID) ERlangcode( (char *) NULL, &Prefered_lang_code );
	}
	if (Any_lang_code != Prefered_lang_code || lang == Prefered_lang_code)
	{
		Any_lang_code = lang;
		constant->value[0] = constant->value[lang];

		/* set lang=0, so that if the version check below is executed,
		** the dbv length will be set to this new value in value[0].
		*/
		lang = 0;
	}
    }

    /* only update the length if this one of the default values - this
    ** may also get overridden if it was a pre-6.5 constant.
    */
    if (lang == 0)
    {
        constant->data_type.db_length = abfobj->retlength;
    }

    if (constant->version < ACAT_VERSION_REL_65 && lang == 0)
    {
	/* db_length is not set correctly for pre-6.5 constant objects - set
	** the length using STlength (this is okay, as nulls cannot be 
	** defined for global constants).
	*/
	i4 len = 0;
	switch(constant->data_type.db_datatype)
	{
		case DB_VCH_TYPE:
		case DB_TXT_TYPE:
			len = DB_CNTSIZE;
			/* fall through ... */
		case DB_CHR_TYPE:
		case DB_CHA_TYPE:
			len += STlength(constant->value[lang]);
        		constant->data_type.db_length = len;
			break;
		default:
			break;
	} /* end switch */
    }
    
    return iiAMmemStat;
}

/*ARGSUSED*/
static STATUS
iiamFtGlobVar ( app, abfobj, dummy  )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			dummy;
{
	register GLOBVAR	*glb = (GLOBVAR *)OOp(abfobj->ooid);

	if ( !glb->data.inDatabase || glb->alter_date == NULL ||
			!STequal(glb->alter_date, abfobj->alter_date) )
	{
		glb->version = abfobj->version;
		iiamStrAlloc( glb->data.tag, abfobj->name, &glb->name );
		glb->ooid = abfobj->ooid;
		if ( glb->owner == NULL )
			iiamStrAlloc(glb->data.tag, abfobj->owner, &glb->owner);
		iiamStrAlloc( glb->data.tag, abfobj->short_remark,
				&glb->short_remark
		);
		glb->long_remark = NULL;
		iiamStrAlloc( glb->data.tag, abfobj->altered_by,
				&glb->altered_by
		);
		if ( glb->create_date == NULL )
			iiamStrAlloc( glb->data.tag, abfobj->create_date,
					&glb->create_date
			);
		iiamStrAlloc( glb->data.tag, abfobj->alter_date,
				&glb->alter_date
		);
		glb->data_type.db_datatype = abfobj->retadf_type;
		glb->data_type.db_length = abfobj->retlength;
		glb->data_type.db_scale = abfobj->retscale;
		iiamStrAlloc( glb->data.tag, abfobj->rettype,
				&glb->data_type.db_data
		);
		glb->flags = (i4)abfobj->flags;

		glb->data.inDatabase = TRUE;
		glb->data.dirty = FALSE;
	}

	return iiAMmemStat;
}

/*ARGSUSED*/
static STATUS
iiamFtRecordDef ( app, abfobj, dummy  )
register APPL		*app;
register ABF_DBCAT	*abfobj;
bool			dummy;
{
	register RECDEF	*rec = (RECDEF *)OOp(abfobj->ooid);

	if ( !rec->data.inDatabase || rec->alter_date == NULL ||
			!STequal(rec->alter_date, abfobj->alter_date) )
	{
		rec->version = abfobj->version;
		iiamStrAlloc( rec->data.tag, abfobj->name, &rec->name );
		rec->ooid = abfobj->ooid;
		if ( rec->owner == NULL )
			iiamStrAlloc(rec->data.tag, abfobj->owner, &rec->owner);
		iiamStrAlloc( rec->data.tag, abfobj->short_remark,
				&rec->short_remark
		);
		rec->long_remark = NULL;
		iiamStrAlloc( rec->data.tag, abfobj->altered_by,
				&rec->altered_by
		);
		if ( rec->create_date == NULL )
			iiamStrAlloc( rec->data.tag, abfobj->create_date,
					&rec->create_date
			);
		iiamStrAlloc(rec->data.tag, abfobj->alter_date, 
					&rec->alter_date
		);
		rec->flags = (i4)abfobj->flags;
		rec->data.inDatabase = TRUE;
		rec->data.dirty = FALSE;
	}

	return iiAMmemStat;
}



STATUS	IIOOdummy();

static STATUS (*_send(class))()
OOID	class;
{
	switch ( class )
	{
	  case OC_APPL:
		return iiamFtAppl;
	  case OC_MUFRAME:
	  case OC_APPFRAME:
	  case OC_UPDFRAME:
	  case OC_BRWFRAME:
		return iiamFtMetaFrame;
	  case OC_OSLFRAME:
		return iiamFtUserFrame;
	  case OC_RWFRAME:
		return iiamFtRepFrame;
	  case OC_QBFFRAME:
		return iiamFtQBFFrame;
	  case OC_GRFRAME:
		return iiamFtGraphFrame;
	  case OC_OSLPROC:
		return iiamFt4GLProc;
	  case OC_HLPROC:
		return iiamFtHLProc;
	  case OC_DBPROC:
		return iiamFtDBProc;
	  case OC_AFORMREF:
	  case OC_FORM:
		return iiamFtForm;
	  case OC_GLOBAL:
		return iiamFtGlobVar;
	  case OC_CONST:
		return iiamFtConstVar;
	  case OC_RECORD:
		return iiamFtRecordDef;
	  case OC_OLDOSLFRAME:
		return wehaveoldosl;
	  default:
		return NULL;
	}
	/*NOT REACHED*/
}

/*
** Notice that we have OLDOSL frames 
*/
static STATUS 
wehaveoldosl(app, abfobj, dummy)
APPL		*app;
ABF_DBCAT	*abfobj;
bool		dummy;
{
	OOID *tmpptr;

	/* Another oldosl frame; see if there's room in the array */
	if (++oldoslframes.num_frames > oldoslframes.ids_size)
	{
		i4 space = oldoslframes.ids_size*sizeof(OOID);
		i4 newspace = (space == 0) ? 
				INIT_IDS_SIZE*sizeof(OOID) : 2*space;

		/* No. Get more space */
		tmpptr = (OOID *)MEreqmem((u_i4)0, (u_i4) newspace,
					  FALSE, NULL);
		if (tmpptr == NULL)
			IIUGbmaBadMemoryAllocation(ERx("wehaveoldosl"));
		if (space > 0)
		{
			MEcopy((PTR)oldoslframes.ids, (u_i2)space, (PTR)tmpptr);
			_VOID_ MEfree((PTR)oldoslframes.ids);
		}
		oldoslframes.ids = tmpptr;
		oldoslframes.ids_size = newspace / sizeof(OOID);
	}
	oldoslframes.ids[oldoslframes.num_frames-1] = abfobj->ooid;
	return iiamFtUserFrame(app, abfobj, dummy);
}

static STATUS
fixup_oldoslframes()
{
	i4 i;
	STATUS status;

	/* 
	** For each oldosl frame:
	**	1. Give it an IL dependency, if it doesn't already have one 
	**	2. Change the object class to OC_OSLFRAME
	*/
	for (i = 0; i < oldoslframes.num_frames; i++)
	{
		USER_FRAME *frm = (USER_FRAME *)OOp(oldoslframes.ids[i]);

		if (frm == NULL)
			continue;	/* Didn't load this one */

		/* Add an IL dependency */
		status = IIAMfaFidAdd(frm->name, _, _, &frm->fid);
		if (status == OK)
		{
			iiuicw1_CatWriteOn();
			status = IIAMwrUserFrame(frm);
			if (status == OK)
				status = iiamUOFupdateOldOsl(frm->ooid);
			iiuicw0_CatWriteOff();
		}
		if (status != OK)
			IIUGerr(E_AM0033_OLDOSL_UpgradeFailure,0, 1, frm->name);
		else
			frm->class = OC_OSLFRAME;
	}
	_VOID_ MEfree((PTR)oldoslframes.ids);
}
