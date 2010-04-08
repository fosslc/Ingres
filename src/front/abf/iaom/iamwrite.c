/*
**	Copyright (c) 1989, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<me.h>		 
#include	<st.h>
#include	<ol.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ui.h>
#include	<abclass.h>
#include	<abfdbcat.h>
#include	<oocat.h>
#include	<oosymbol.h>
#include	<oodep.h>
#include	<oodefine.h>
#include	<metafrm.h>
#include	"eram.h"

/**
** Name:	iamwrite.c -	ABF Object Write Module.
**
** Description:
**	Contains routines used to write ABF application component objects
**	to the database.  Except for 'IIAMwrFormRef()', these routines write
**	only the ABF catalogs.  The object catalog entries for these objects
**	should be written through the OO module.  Defines:
**
**	IIAMwrApp()		write an application base object.
**	IIAMwrUserFrame()	write an application user-defined frame object.
**	IIAMwrRepFrame()	write an application report frame object.
**	IIAMwrQBFFrame()	write an application QBF frame object.
**	IIAMwrGraphFrame()	write an application graph frame object.
**	IIAMwr4GLProc()		write an application 4GL procedure object.
**	IIAMwrHLProc()		write an application host-language proc. object.
**	IIAMwrDBProc()		write an application database procedure object.
**	IIAMwrFormRef()		write an application form reference.
**	IIAMwrConst()		write an application constant.
**	IIAMwrGlobal()		write an application variable.
**	IIAMwrRecord()		write an application record definition.
**
** History:
**	Revision 6.2  89/01  wong
**	Initial revision.
**
**	7/90 (Mike S) Dependency manager changes
**
**	Revision 6.5
**	
**	14-jan-92 (davel)
**		Modified IIAMwrConst to reflect simplified handling
**		of global constants. Moved special logic for updating 
**		constants to _write_it().
**      3-aug-1992 (fraser)
**          Changed _flush0 to ii_flush0 because of conflict with
**          Microsoft library.
**	25-aug-92 (davel)
**		Modified IIAMwrConst to move the version number from the
**		CONSTANT structure into ABF_DBCAT structure, as it may have
**		been modified.
**	16-oct-92 (davel)
**		Remove special case for Constant inserts; keep the special
**		case for Constant updates.
**	08-dec-92 (davel)
**		Replaced IIolLang references with the official OL interface,
**		OLanguage().
**	18-may-93 (davel)
**		Added argument to iiamAddDep() calls throughout the module.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**	26-Aug-2009 (kschendel) 121804
**	    Need ui.h and oodefine.h to satisfy gcc 4.3.
*/

char	*IIUGdmlStr();
static i4	_compare();


/*{
** Name:	_write_it() - Write an application object to database.
**
** Description:
**	Writes an application base object (not including any component objects)
**	to the database.
**
**	This routine was constructed originally to service the writing of
**	form and constant application objects since they
**	all have exactly the same requirements.
**
** Input:
**	record	{APPL_COMP *}  The application component to be written.
**	catalog	{ABF_DBCAT *}  The database catalog record.
**
** History:
**	09/89 (jfried)  Cloned from inline instantiations.
**	14-jan-92 (davel)
**		incorporated special update/insert rules for global constants.
*/
static STATUS
_write_it ( record, catalog )
APPL_COMP	*record;
ABF_DBCAT	*catalog;
{
    STATUS	stat;

    if ( record->data.inDatabase )
    {
	stat = (record->class == OC_CONST && (STtrmwhite(catalog->arg0) > 0) )
		 ? iiamucUpdateConst(catalog) : iiamUpdateCat(catalog);
    }
    else 
    {
	stat = iiamInsertCat(catalog);
	if ( stat == OK )
		stat = iiamAddDep(record, catalog->applid, 
				ERx("DUMMY"), 0, 0, (char *)NULL, _, 0);
    }
    
    if ( stat == OK )
    {
	/* APPL's don't have an appl item */
	if (record->class != OC_APPL)
		STcopy(record->alter_date, record->appl->modified);
	record->data.dirty = FALSE;
    }

    return stat;
}

/*{
** Name:	IIAMwrApp() -	Write an Application Object to the Database.
**
** Description:
**	Writes an application base object (not including any component objects)
**	to the database.  The argument values 'AppExecutable', 'AppStartName',
**	'AppLinkFile', 'AppDML' and 'AppRole' are filled in; the other 
**	argument values and
**	the 'symbol' and return type fields (except for the 'retlength') are
**	cleared.  The return type length is used to contain the special
**	application flags.
**
**	A single DUMMY dependency is written to the catalogs when the
**	application is created.
**
** Input:
**	app	{APPL *}  The application object.
**
** History:
**	01/89 (jhw)  Written.
*/
STATUS
IIAMwrApp ( app )
register APPL	*app;
{
	STATUS		stat = OK;

	if ( app->ooid > OC_UNDEFINED && app->data.dirty )
	{
		ABF_DBCAT	cata;

		cata.applid = app->ooid;
		cata.ooid = app->ooid;
		cata.source = app->source;
		cata.symbol = _;
		cata.AppExecutable = app->executable;
		cata.AppStartName = app->start_name;
		cata.AppLinkFile = app->link_file;
		cata.AppDML = IIUGdmlStr(app->dml);
		cata.AppRole = app->roleid;
		cata.arg5 = _;
		cata.retadf_type = 0;
		cata.retlength = app->batch ? APPL_BATCH : app->start_proc;
		cata.retscale = 0;
		cata.rettype = _;
		cata.flags = (i4)app->flags;

		stat = _write_it( (APPL_COMP *)app, &cata );
	}
	return stat;
}

/*{
** Name:	IIAMwrUserFrame() - Write an Application User Frame
**					Component Object to the Database.
** Description:
**	Writes an application user frame component object to the database.
**	Except for the argument values, all fields are set.  The form
**	reference will be written a well if necessary.
**
**	Two dependency records are written to the catalogs; one for the form,
**	and the other for the 4GL IL code.
**
**	Note:  2.0 Version user frames are also written with this routine.
**
** Input:
**	frm	{USER_FRAME *}  The application user frame component object.
**
** History:
**	01/89 (jhw)  Written.
*/
static VOID	user_fill();

STATUS
IIAMwrUserFrame ( frm )
register USER_FRAME	*frm;
{
	STATUS	stat = OK;
	METAFRAME *mf;
	OOID d2, d3;

	/*
	** set mf NULL if this is either not a metframe type, OR
	** we never fetched the menu dependencies.  If there are no menu
	** dependencies, we don't want to update them.
	*/
	mf = frm->mf;
	if (mf != NULL && (mf->flags & MF_ABFONLY) != 0)
		mf = NULL;
	if (mf == NULL)
		d2 = d3 = OC_DTDBREF;
	else
		d2 = d3 = OC_DTMREF;

	if ( frm->ooid > OC_UNDEFINED && frm->data.dirty )
	{
		ABF_DBCAT	cata;
		char		arg5[48];

		user_fill(&cata,frm,arg5);

		if ( !frm->data.inDatabase )
			stat = iiamInsertCat(&cata);
		else
		{
			if ( (stat = iiamUpdateCat(&cata)) == OK )
				stat = IIAMdpDeleteDeps((OO_OBJECT *)frm,
						OC_DTDBREF, d2, d3);
		}
		if ( stat == OK )
		{
			/* Both Form and ILCode are required */
			if ( (stat = iiamAddDep( frm, frm->appl->ooid,
						  frm->form->name,
						 ( frm->form->symbol != NULL &&
						    *frm->form->symbol != EOS )
							? OC_AFORMREF : OC_FORM,
						 OC_DTDBREF, (char *)NULL, _, 0
					)) == OK )
			{
				char	buf[11];

				stat = iiamAddDep( frm, frm->appl->ooid,
					STprintf(buf, ERx("%d"), frm->fid),
					OC_ILCODE, OC_DTDBREF, 
					(char *)NULL, _, 0
				);
			}
			if ( stat == OK  &&  ( frm->form->data.dirty ||
						!frm->form->data.inDatabase ) )
				stat = IIAMwrFormRef( frm->form );
			if ( stat == OK && mf != NULL )
				stat = IIAMwmdWriteMenuDep( mf, FALSE );
			if ( stat == OK )
			{
				STcopy(frm->alter_date, frm->appl->modified);
				frm->data.dirty = FALSE;
			}
		}
	}
	return stat;
}

/*{
** Name:	IIAMufqUserFrameQuick() - Write an Application
**					Component to the Database.
** Description:
**	Writes an application object to the database,
**	but ONLY the abfobjects part, avoiding the update of ii_objects
**	and the abfdependencies information.  This is somewhat abusive,
**	as this means that the alter date in ii_objects will not be changed.
**	This call is used only for internal changes not visible to the
**	user.  This is primarily for the purposes of updating VISION
**	state information on a frame without incurring unwanted overhead.
**
**	NOTE:
**		If the object is not already in the database, this
**		routine will append to all the tables, and doesn't
**		provide any savings.
**
** Input:
**	obj	Application object.
**
** History:
**	8/89 (bobm) written
**	11/89 (bobm) modified to accept all types rather than just
**		user frames, making the name somewhat misleading.
*/

static VOID	report_fill();
static VOID	qbf_fill();
static VOID	graph_fill();
static VOID	hl_fill();
static VOID	db_fill();
static VOID	_4gp_fill();

STATUS
IIAMufqUserFrameQuick ( obj )
register OO_OBJECT	*obj;
{
	STATUS		rc = OK;
	ABF_DBCAT	cata;
	char		arg5[ABFARG_SIZE+1];

	if ( obj->ooid > OC_UNDEFINED)
	{
		iiuicw1_CatWriteOn();

		switch(obj->class)
		{
		case OC_MUFRAME:
		case OC_APPFRAME:
		case OC_UPDFRAME:
		case OC_BRWFRAME:
		case OC_OLDOSLFRAME:
		case OC_OSLFRAME:
			if ( !obj->data.inDatabase )
				rc = IIAMwrUserFrame((USER_FRAME *)obj);
			else
			{
				user_fill(&cata, (USER_FRAME *)obj, arg5);
				rc = iiamUpdateCat(&cata);
			}
			break;
		case OC_HLPROC:
			if ( !obj->data.inDatabase )
				rc = IIAMwrHLProc((HLPROC *)obj);
			else
			{
				hl_fill(&cata,(HLPROC *)obj);
				rc = iiamUpdateCat(&cata);
			}
			break;
		case OC_OSLPROC:
			if ( !obj->data.inDatabase )
				rc = IIAMwr4GLProc((_4GLPROC *)obj);
			else
			{
				_4gp_fill(&cata,(_4GLPROC *)obj);
				rc = iiamUpdateCat(&cata);
			}
			break;
		case OC_DBPROC:
			if ( !obj->data.inDatabase )
				rc = IIAMwrDBProc((DBPROC *)obj);
			else
			{
				db_fill(&cata,(DBPROC *)obj);
				rc = iiamUpdateCat(&cata);
			}
			break;
		case OC_RWFRAME:
			if ( !obj->data.inDatabase )
				rc = IIAMwrRepFrame((REPORT_FRAME *)obj);
			else
			{
				report_fill(&cata,(REPORT_FRAME *)obj);
				rc = iiamUpdateCat(&cata);
			}
			break;
		case OC_QBFFRAME:
			if ( !obj->data.inDatabase )
				rc = IIAMwrQBFFrame((QBF_FRAME *)obj);
			else
			{
				qbf_fill(&cata,(QBF_FRAME *)obj);
				rc = iiamUpdateCat(&cata);
			}
			break;
		case OC_GRFRAME:
		case OC_GBFFRAME:
			if ( !obj->data.inDatabase )
				rc = IIAMwrGraphFrame((GRAPH_FRAME *)obj);
			else
			{
				graph_fill(&cata,(GRAPH_FRAME *)obj);
				rc = iiamUpdateCat(&cata);
			}
			break;

		/*
		** these remaining types don't require dependency updates
		** anyway - simply call the appropriate update routine.
		*/
		case OC_AFORMREF:
			obj->data.dirty = TRUE;
			rc = IIAMwrFormRef((FORM_REF *) obj);
			break;
		case OC_GLOBAL:
			obj->data.dirty = TRUE;
			rc = IIAMwrGlobal((GLOBVAR *) obj);
			break;
		case OC_CONST:
			obj->data.dirty = TRUE;
			IIAMwrConst((CONSTANT *)obj);
			break;
		case OC_RECORD:
			obj->data.dirty = TRUE;
			IIAMwrRecord((RECDEF *)obj);
			break;
		default:
			rc = OK;
			break;
		}

		iiuicw0_CatWriteOff();
	}

	return rc;
}

static VOID
user_fill ( cat, frm, arg5 )
register ABF_DBCAT	*cat;
register USER_FRAME	*frm;
char			*arg5;
{
	cat->applid = frm->appl->ooid;
	cat->ooid = frm->ooid;
	cat->source = frm->source;
	cat->symbol = frm->symbol;
	cat->retadf_type = frm->return_type.db_datatype;
	cat->retlength = frm->return_type.db_length;
	cat->retscale = frm->return_type.db_scale;
	cat->rettype = frm->return_type.db_data;
	cat->UserStatic = ( frm->is_static ? ERx("static") : ERx("dynamic") );
	cat->arg1 = cat->arg2 = cat->arg4 = _;

	cat->arg3 = frm->compdate;
	if (cat->arg3 == NULL)
		cat->arg3 = _;

	cat->arg5 = arg5;
	if (frm->class != OC_OSLFRAME && frm->mf != NULL)
	{ /* meta-frames */
		cat->arg4 = ( frm->mf->formgendate != NULL )
				? frm->mf->formgendate : _;
		STprintf(arg5,"%d@%s", frm->mf->state, frm->mf->gendate);
	}
	else
	{
		arg5[0] = EOS;
	}
	cat->flags = (i4)frm->flags;
}

/*{
** Name:	IIAMwrReportFrame() - Write an Application Report Frame
**					Component Object to the Database.
** Description:
**	Writes an application report frame component object to the database.
**	The argument values 'RepCmd' and 'RepOutput' are filled in; all the
**	other fields are cleared.
**
**	One or two dependency records are written to the catalogs; one for
**	the report, and optionally, one for the form.
**
** Input:
**	frm	{REPORT_FRAME *}  The application report frame component object.
**
** History:
**	01/89 (jhw)  Written.
**
**	02/26/93 (DonC)	Do NOT create a dependancy between the Report Frame and
**			a Report Parameter Form if the Form Name is Blank.  
**			otherwise the application will try to compile and link
**			a blank form name into the application (Bug 40140).
*/
STATUS
IIAMwrRepFrame ( frm )
register REPORT_FRAME	*frm;
{
	STATUS	stat = OK;

	if ( frm->ooid > OC_UNDEFINED && frm->data.dirty )
	{
		ABF_DBCAT	cata;

		report_fill(&cata, frm);

		if ( !frm->data.inDatabase )
			stat = iiamInsertCat(&cata);
		else
		{
			if ( (stat = iiamUpdateCat(&cata)) == OK )
				stat = IIAMdpDeleteDeps((OO_OBJECT *)frm,
					 OC_DTDBREF, OC_DTDBREF, OC_DTDBREF);
		}
		if ( stat == OK )
		{
			if ( ( stat = iiamAddDep( frm, frm->appl->ooid,
						frm->report,
						OC_REPORT, OC_DTDBREF, 
						(char *)NULL, _, 0
					)) == OK  &&  frm->form != NULL 
						  && *frm->form->name != EOS
							)
			{ /* Process form reference */
				stat = iiamAddDep( frm, frm->appl->ooid,
						   frm->form->name,
						( frm->form->symbol != NULL &&
						    *frm->form->symbol != EOS )
							? OC_AFORMREF : OC_FORM,
						OC_DTDBREF, (char *)NULL, _, 0
				);
				if ( stat == OK  &&  ( frm->form->data.dirty ||
						!frm->form->data.inDatabase ) )
					stat = IIAMwrFormRef( frm->form );
			}
			if ( stat == OK )
			{
				STcopy(frm->alter_date, frm->appl->modified);
				frm->data.dirty = FALSE;
			}
		}
	}
	return stat;
}

static VOID
report_fill ( cata, frm )
register ABF_DBCAT	*cata;
register REPORT_FRAME	*frm;
{
	cata->applid = frm->appl->ooid;
	cata->ooid = frm->ooid;
	cata->source = frm->source;
	cata->symbol = _;
	cata->retadf_type = 0;
	cata->retlength = 0;
	cata->retscale = 0;
	cata->rettype = _;
	cata->RepCmd = frm->cmd_flags;
	cata->RepOutput = frm->output;
	cata->arg2 = cata->arg3 = cata->arg4 = cata->arg5 = _;
	cata->flags = (i4)frm->flags;
}

/*{
** Name:	IIAMwrQBFFrame() - Write an Application QBF Frame
**					Component Object to the Database.
** Description:
**	Writes an application QBF frame component object to the database.
**	The argument values 'QBFCmd' and 'QBFQryType' are filled in; all the
**	other fields are cleared.
**
**	One or two dependency records are written to the catalogs; one for
**	the query object (either a JoinDef or a Table,) and optionally, one
**	for the form.
**
** Input:
**	frm	{QBF_FRAME *}  The application QBF frame component object.
**
** History:
**	02/89 (jhw)  Written.
*/
STATUS
IIAMwrQBFFrame ( frm )
register QBF_FRAME	*frm;
{
	STATUS	stat = OK;

	if ( frm->ooid > OC_UNDEFINED && frm->data.dirty )
	{
		ABF_DBCAT	cata;

		qbf_fill(&cata,frm);

		if ( !frm->data.inDatabase )
			stat = iiamInsertCat(&cata);
		else
		{
			if ( (stat = iiamUpdateCat(&cata)) == OK )
				stat = IIAMdpDeleteDeps((OO_OBJECT *)frm,
					OC_DTDBREF, OC_DTDBREF, OC_DTDBREF);
		}
		if ( stat == OK )
		{
			if ( (stat = iiamAddDep( frm, frm->appl->ooid,
						frm->qry_object,
						OC_JOINDEF, OC_DTDBREF, 
						(char *)NULL, _, 0
					)) == OK  &&  frm->form != NULL )
			{
				stat = iiamAddDep( frm, frm->appl->ooid,
						frm->form->name,
						OC_FORM, OC_DTDBREF, 
						(char *)NULL, _, 0
				);
			}
			if ( stat == OK )
			{
				STcopy(frm->alter_date, frm->appl->modified);
				frm->data.dirty = FALSE;
			}
		}
	}
	return stat;
}


static VOID
qbf_fill ( cata, frm )
register ABF_DBCAT	*cata;
register QBF_FRAME	*frm;
{
	cata->applid = frm->appl->ooid;
	cata->ooid = frm->ooid;
	cata->source = _;
	cata->symbol = _;
	cata->retadf_type = 0;
	cata->retlength = 0;
	cata->retscale = 0;
	cata->rettype = _;
	cata->QBFCmd = frm->cmd_flags;
	cata->QBFQryType = frm->qry_type == OC_JOINDEF ? ERx("J") : ERx("T");
	cata->arg2 = cata->arg3 = cata->arg4 = cata->arg5 = _;
	cata->flags = (i4)frm->flags;
}


/*{
** Name:	IIAMwrGraphFrame() - Write an Application Graph Frame
**					Component Object to the Database.
** Description:
**	Writes an application graph frame component object to the database.
**	The argument values 'GraphCmd' and 'GraphOutput' are filled in; all
**	the other fields are cleared.
**
**	One dependency record is written to the catalogs for the graph object.
**
** Input:
**	frm	{GRAPH_FRAME *}  The application graph frame component object.
**
** History:
**	02/89 (jhw)  Written.
*/
STATUS
IIAMwrGraphFrame ( frm )
register GRAPH_FRAME	*frm;
{
	STATUS	stat = OK;

	if ( frm->ooid > OC_UNDEFINED && frm->data.dirty )
	{
		ABF_DBCAT	cata;

		graph_fill(&cata,frm);

		if ( !frm->data.inDatabase )
			stat = iiamInsertCat(&cata);
		else
		{
			if ( (stat = iiamUpdateCat(&cata)) == OK )
				stat = IIAMdpDeleteDeps((OO_OBJECT *)frm,
					OC_DTDBREF, OC_DTDBREF, OC_DTDBREF);
		}
		if ( stat == OK )
		{
			stat = iiamAddDep( frm, frm->appl->ooid, 
						frm->graph, OC_GRAPH, 
						OC_DTDBREF, (char *)NULL, _, 0
			);
			if ( stat == OK )
			{
				STcopy(frm->alter_date, frm->appl->modified);
				frm->data.dirty = FALSE;
			}
		}
	}
	return stat;
}

static VOID
graph_fill ( cata, frm )
register ABF_DBCAT	*cata;
register GRAPH_FRAME	*frm;
{
	cata->applid = frm->appl->ooid;
	cata->ooid = frm->ooid;
	cata->source = _;
	cata->symbol = _;
	cata->retadf_type = 0;
	cata->retlength = 0;
	cata->retscale = 0;
	cata->rettype = _;
	cata->GraphCmd = frm->cmd_flags;
	cata->GraphOutput = frm->output;
	cata->arg2 = cata->arg3 = cata->arg4 = cata->arg5 = _;
	cata->flags = (i4)frm->flags;
}

/*{
** Name:	IIAMwrHLProc() - Write an Application Host-Language
**				Procedure Component Object to the Database.
** Description:
**	Writes an application host-language procedure component object to the
**	database.  Only the 'HostLanguage' argument value is set along with all
**	the other fields.  (The 'source' field may be empty, though.)
**
**	A single DUMMY dependency is written to the catalogs when the object
**	is created.
**
** Input:
**	proc	{HLPROC *}  The application host-language procedure component
**				object.
** History:
**	01/89 (jhw)  Written.
*/

STATUS
IIAMwrHLProc ( proc )
register HLPROC	*proc;
{
	STATUS	stat = OK;

	if ( proc->ooid > OC_UNDEFINED && proc->data.dirty )
	{
		ABF_DBCAT	cata;

		hl_fill(&cata,proc);

		if ( proc->data.inDatabase )
			stat = iiamUpdateCat(&cata);
		else
		{
			if ( (stat = iiamInsertCat(&cata)) == OK )
				stat = iiamAddDep(proc, proc->appl->ooid, 
						ERx("DUMMY"), 0,
						0, (char *)NULL, _, 0);
		}
		if ( stat == OK )
		{
			STcopy(proc->alter_date, proc->appl->modified);
			proc->data.dirty = FALSE;
		}
	}
	return stat;
}

static VOID
hl_fill ( cata, proc )
register ABF_DBCAT	*cata;
register HLPROC		*proc;
{
	cata->applid = proc->appl->ooid;
	cata->ooid = proc->ooid;
	cata->source = proc->source;
	cata->symbol = proc->symbol;
	cata->retadf_type = proc->return_type.db_datatype;
	cata->retlength = proc->return_type.db_length;
	cata->retscale = proc->return_type.db_scale;
	cata->rettype = proc->return_type.db_data;
	cata->HostLanguage = OLanguage(proc->ol_lang);
	cata->arg3 = ( proc->compdate != NULL ) ? proc->compdate : _;
	cata->arg1 = cata->arg2 = cata->arg4 = cata->arg5 = _;
	cata->flags = (i4)proc->flags;
}

/*{
** Name:	IIAMwrDBProc() - Write an Application Database Procedure
**					Component Object to the Database.
** Description:
**	Writes an application database procedure component object to the
**	database.  Execpt for the return type fields, all fields are cleared.
**	(The 'source' field may be settable at some point in the future.)
**
**	A single dependency is written to the catalogs for the database
**	procedure itself when the object is created.
**
** Input:
**	proc	{DBPROC *}  The application database procedure component object.
**
** History:
**	01/89 (jhw)  Written.
*/
STATUS
IIAMwrDBProc ( proc )
register DBPROC	*proc;
{
	STATUS	stat = OK;

	if ( proc->ooid > OC_UNDEFINED && proc->data.dirty )
	{
		ABF_DBCAT	cata;

		db_fill(&cata,proc);

		if ( proc->data.inDatabase )
			stat = iiamUpdateCat(&cata);
		else
                {
                        if ( (stat = iiamInsertCat(&cata)) == OK )
                                stat = iiamAddDep( proc, proc->appl->ooid,
						   proc->name, OC_DBPROC, 
						   OC_DTDBREF, 
						   (char *)NULL, _, 0
                                );
                }
		if ( stat == OK )
		{
			STcopy(proc->alter_date, proc->appl->modified);
			proc->data.dirty = FALSE;
		}
	}
	return stat;
}

static VOID
db_fill ( cata, proc )
register ABF_DBCAT	*cata;
register DBPROC		*proc;
{
	cata->applid = proc->appl->ooid;
	cata->ooid = proc->ooid;
	cata->source = _;
	cata->symbol = _;
	cata->retadf_type = proc->return_type.db_datatype;
	cata->retlength = proc->return_type.db_length;
	cata->retscale = proc->return_type.db_scale;
	cata->rettype = proc->return_type.db_data;
	cata->arg0 = cata->arg1 = cata->arg2 =
			cata->arg3 = cata->arg4 = cata->arg5 = _;
	cata->flags = (i4)proc->flags;
}

/*{
** Name:	IIAMwr4GLProc() - Write an Application 4GL Procedure
**					Component Object to the Database.
** Description:
**	Writes an application 4Gl procedure component object to the database.
**	Except for the argument values, all fields are set.
**
**	One dependency record is written to the catalogs for the 4GL IL code.
**
** Input:
**	frm	{_4GLPROC *}  The application 4GL procedure component object.
**
** History:
**	01/89 (jhw)  Written.
*/
STATUS
IIAMwr4GLProc ( proc )
register _4GLPROC	*proc;
{
	STATUS	stat = OK;

	if ( proc->ooid > OC_UNDEFINED && proc->data.dirty )
	{
		ABF_DBCAT	cata;

		_4gp_fill(&cata,proc);

		if ( ! proc->data.inDatabase )
			stat = iiamInsertCat(&cata);
		else
		{
			if ( (stat = iiamUpdateCat(&cata)) == OK )
				stat = IIAMdpDeleteDeps((OO_OBJECT *)proc,
					OC_DTDBREF, OC_DTDBREF, OC_DTDBREF);
		}
		if ( stat == OK )
		{
			char	buf[11];

			stat = iiamAddDep( proc, proc->appl->ooid,
					STprintf(buf, ERx("%d"), proc->fid),
					OC_ILCODE, OC_DTDBREF, 
					(char *)NULL, _, 0
			);
		}
		if ( stat == OK )
		{
			STcopy(proc->alter_date, proc->appl->modified);
			proc->data.dirty = FALSE;
		}
	}
	return stat;
}

static VOID
_4gp_fill ( cata, proc )
register ABF_DBCAT	*cata;
register _4GLPROC	*proc;
{
	cata->applid = proc->appl->ooid;
	cata->ooid = proc->ooid;
	cata->source = proc->source;
	cata->symbol = proc->symbol;
	cata->retadf_type = proc->return_type.db_datatype;
	cata->retlength = proc->return_type.db_length;
	cata->retscale = proc->return_type.db_scale;
	cata->rettype = proc->return_type.db_data;
	cata->arg3 = ( proc->compdate != NULL ) ? proc->compdate : _;
	cata->arg0 = cata->arg1 = cata->arg2 = cata->arg4 = cata->arg5 = _;
	cata->flags = (i4)proc->flags;
}

/*{
** Name:	IIAMwrFormRef() -	Write an Application Form Reference
**						to the Database.
** Description:
**	Writes an application form reference object to the database.  The
**	'symbol' and 'source' fields are set; all other values are cleared.
**	This routine uses the OO level 0 flush method to create the object
**	catalog entry for the form reference.
**
**	A single DUMMY dependency is written to the catalogs when the
**	form reference is created.
**
** Input:
**	form	{FORM_REF *}  The application form reference object.
**
** History:
**	02/89 (jhw)  Written.
**	02/90 (Mike S) Flush FormRef 
*/
STATUS
IIAMwrFormRef ( form )
register FORM_REF	*form;
{
    STATUS		stat = OK;
    
    if ( form->ooid > OC_UNDEFINED && form->data.dirty )
    {
	ABF_DBCAT	cata;

	cata.applid = form->appl->ooid;
	cata.ooid = form->ooid;
	cata.source = form->source;
	cata.symbol = form->symbol;
	cata.arg0 = cata.arg1 = cata.arg2 =
	    cata.arg3 = cata.arg4 = cata.arg5 = _;
	cata.retadf_type = 0;
	cata.retlength = 0;
	cata.retscale = 0;
	cata.rettype = _;
	cata.flags = (i4)form->flags;

	if ( !IIUIinXaction() )
		iiuicw1_CatWriteOn();
	stat = _write_it( (APPL_COMP *)form, &cata );
	if (OK == stat)
		_VOID_ OOsndSelf((OO_OBJECT *)form, ii_flush0);		 
	if ( !IIUIinXaction() )
		iiuicw0_CatWriteOff();
    }
    return stat;
}

/*{
** Name:	IIAMwrConst() -	Write an Application Constant
**						to the Database.
** Description:
**	Writes an application Constant object to the database.  
**
**	A single DUMMY dependency is written to the catalogs when the
**	Constant is created.
**
** Input:
**	gvar	{CONSTANT *}  The application Constant object.
**
** History:
**	09/89 (jfried)  Cloned from IIAMwrGlobal.
**	01/90 (jhw)  Modified to use 'iiamCnInsertCat()'.
**	14-jan-92 (davel)  
**		Modify to reflect simplified handling of multi-language 
**		global constants: always moved the value of constant->language
**		into cata.arg0, and always use constant->value[0] as the
**		constants value. Moved special update/insert logic to 
**		_write_it().
**	25-aug-92 (davel)
**		Move the potentially modified version number back into ABF_DBCAT
**		structure.
*/

STATUS
IIAMwrConst ( constant )
register CONSTANT	*constant;
{
    STATUS	stat = OK;

    if ( constant->ooid > OC_UNDEFINED && constant->data.dirty )
    {
	ABF_DBCAT   cata;

	cata.applid = constant->appl->ooid;
	cata.ooid = constant->ooid;
	cata.version = constant->version;

	cata.symbol = _;
	cata.arg1 = cata.arg2 = cata.arg3 = cata.arg4 = cata.arg5 = _;

	cata.retadf_type = constant->data_type.db_datatype;
	cata.retlength = constant->data_type.db_length;
	cata.retscale = constant->data_type.db_scale;
	cata.rettype = constant->data_type.db_data;
	cata.flags = (i4)constant->flags;

	/* cata.arg0 (aka cata.ConstLanguage) contains the value "default"
	** for string constants and the value "" for numeric constants.  This
	** column previously contained the natural language (e.g. "english",
	** "french", etc.) associated with this value of the constant. For
	** backward compatibility reasons, we still need to have non-blank
	** values stored into cata.arg0 for string constants, and blank
	** value for numeric constants.  abf!abf!editglo.qsc and 
	** abf!abf!abcoedit.qsc ensure that the value of constant->language
	** is set to either "default" or "" based on the datatype of the 
	** constant.
	*/
	cata.arg0 = constant->language;
	cata.source = constant->value[0];

	stat = _write_it( (APPL_COMP *)constant, &cata );
    }

    return stat;
}

/*{
** Name:	IIAMwrGlobal() -	Write an Application Variable
**						to the Database.
** Description:
**	Writes an application Variable object to the database.  
**	This routine uses the OO level 0 flush method to create the object
**	catalog entry for the Global Variable.
**
** Input:
**	gvar	{GLOBVAR *}  The application global Variable object.
**
**	If the global is of record type, a dependency is written to the
**	database.
**
** History:
**	06/89 (billc)  Written.
**	7/91 (Mike S) Add dependency when a global of record type is created
*/
STATUS
IIAMwrGlobal ( gvar )
register GLOBVAR	*gvar;
{
    STATUS		stat = OK;
    
    if ( gvar->ooid > OC_UNDEFINED && gvar->data.dirty )
    {
	ABF_DBCAT	cata;

	cata.applid = gvar->appl->ooid;
	cata.ooid = gvar->ooid;

	cata.source = cata.symbol = _;
	cata.arg0 = cata.arg1 = cata.arg2 =
	    cata.arg3 = cata.arg4 = cata.arg5 = _;

	cata.retadf_type = gvar->data_type.db_datatype;
	cata.retlength = gvar->data_type.db_length;
	cata.retscale = gvar->data_type.db_scale;
	cata.rettype = gvar->data_type.db_data;
	cata.flags = (i4)gvar->flags;

    	if ( !gvar->data.inDatabase )
	{
		stat = iiamInsertCat(&cata);
	}
	else
	{
        	if ( (stat = iiamUpdateCat(&cata)) == OK)
			stat = IIAMdpDeleteDeps((OO_OBJECT *)gvar,
					OC_DTTYPE, OC_DTTYPE, OC_DTTYPE);
	}
	if (stat == OK)
	{
		/* Add a dependency if the global is of Record type */
		if (gvar->data_type.db_datatype == DB_DMY_TYPE)
			stat = iiamAddDep(gvar, gvar->appl->ooid,
				   (char *)(gvar->data_type.db_data), 
				   OC_RECORD, OC_DTTYPE, (char *)NULL, _, 0);
	}

    	if ( stat == OK )
    	{
                STcopy(gvar->alter_date, gvar->appl->modified);
        	gvar->data.dirty = FALSE;
    	}
    }
    return stat;
}

/*{
** Name:	IIAMwrRecord() -	Write an Application record Definition
**						to the Database.
** Description:
**	Writes an application record Definition object to the database.  
**	This routine uses the OO level 0 flush method to create the object
**	catalog entry for the Record Definition.
**
**	If the record contains other records, it has dependencies
**	pointing to the contained records.  They are processed in 
**	IIAMxrdRecordDependencies, below, when the new record definition 
**	is saved.
**
** Input:
**	rdef	{RECDEF *}  The application record defitition object.
**
** History:
**	08/89 (billc)  Written.
*/
STATUS
IIAMwrRecord ( rdef )
register RECDEF	*rdef;
{
    STATUS		stat = OK;
    
    if ( rdef->ooid > OC_UNDEFINED && rdef->data.dirty )
    {
	ABF_DBCAT	cata;

	cata.applid = rdef->appl->ooid;
	cata.ooid = rdef->ooid;

	cata.source = cata.symbol = _;
	cata.arg0 = cata.arg1 = cata.arg2 =
	    cata.arg3 = cata.arg4 = cata.arg5 = _;

	cata.retadf_type = DB_NODT;
	cata.retlength = 0;
	cata.retscale = 0;
	cata.rettype = _;
	cata.flags = (i4)rdef->flags;

	cata.short_remark = rdef->short_remark;

    	if ( !rdef->data.inDatabase )
	{
	    stat = iiamInsertCat(&cata);
	}
	else
	{
       	    stat = iiamUpdateCat(&cata);
	}
    }

    return stat;
}

STATUS
IIAMxrdRecordDependencies( rdef )
RECDEF *rdef;
{
    /* Add dependencies if the record's members are also records */
    RECMEM 	*recmem;
    char 	*typename;
    STATUS 	stat = OK;
    char 	**record_names;
    i4		count;	
    i4		i;

    /* Delete any current dependencies */
    iiuicw1_CatWriteOn();
    stat = IIAMdpDeleteDeps((OO_OBJECT *)rdef, OC_DTTYPE, OC_DTTYPE, OC_DTTYPE);

    /* First count the number of members which are themselves records */
    count = 0;
    for (recmem = rdef->recmems; recmem != NULL; recmem = recmem->next_mem)
    {
    	if (recmem->data_type.db_datatype == DB_DMY_TYPE)
    		count++;
    }
    if (count == 0)
    {
    	iiuicw0_CatWriteOff();
    	return OK;	/* Nothing to do */
    }

    /* Make a list of names to sort */
    record_names = (char **)MEreqmem(0, count * sizeof(char *), FALSE,NULL);
    if (record_names == NULL)
    {
    	iiuicw0_CatWriteOff();
    	return FAIL;
    }

    count = 0;
    for (recmem = rdef->recmems; recmem != NULL; recmem = recmem->next_mem)
    {
    	if (recmem->data_type.db_datatype == DB_DMY_TYPE)
    	    record_names[count++] = (char *)(recmem->data_type.db_data);
    }

    /* Sort the names */
    IIUGqsort((char *)record_names, count, sizeof(char *), _compare);

    /* Add a dependency for each new name */
    typename = ERx("");
    for (i = 0; i < count; i++)
    {
	if (!STequal(typename, record_names[i]))
	{
	    typename = record_names[i];
            /* 
            ** There shouldn't be any self-referential records.
            ** We should catch it higher up, but just in case, 
            ** let's not add the dependency.
            */
            if (STequal(typename, rdef->name))
    	       continue;

	    stat = iiamAddDep(rdef, rdef->appl->ooid, typename,
				    OC_RECORD, OC_DTTYPE, (char *)NULL, _, 0);
	}
    }	

    iiuicw0_CatWriteOff();
    MEfree(record_names);
    return stat;
}

static i4
_compare(st1, st2)
char *st1;
char *st2;
{
	/* We actually have pointers to character pointers */
	return STcompare(*(char **)st1, *(char **)st2);
}
