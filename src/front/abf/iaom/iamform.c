/*
**	Copyright (c) 1989, 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<st.h>
#include	<me.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<abclass.h>
#include	<uigdata.h>

/**
** Name:	iamform.c -	Form Reference Module.
**
** Description:
**	Contains routines used to get and remove form references.  Defines:
**
** Defines
**	IIAMfaFormAlloc()	return new form reference.
**	iiamFoGetRef()		return a shared form reference for a form.
**	IIAMformRef()		return form reference for a form.
**	IIAMfrRemoveRef()	remove a form reference for a form.
**	IIAMfrChangeRef()	change a form reference for a different form.
**	IIAMcrCountRef()	count the number of objects using a given form.
**
** History
**	Revision 6.2  89/02  wong
**	Initial revision.
*/

OOID	IIOOtmpId();

GLOBALREF char	_iiOOempty[];

static FORM_REF	*_lookupRef();

/*{
** Name:	IIAMfaFormAlloc() -	Allocate a Form Reference for a Form.
**
** Description:
**	Allocates a new form reference for a form from storage.  The structure
**	will be initialized with a temporary ID (which may be overwritten when
**	the form reference is fetched from the database.)
**
**	This routine does not share form references or search the database.  It
**	will be called by routines that implement sharing of references, and
**	will be called directly for QBF frames, which do not share form
**	references.
**
** Inputs:
**	app	{APPL *}  The application object to which the form belongs.
**	name	{char *}  The name of the form.
**
** Returns:
**	{FORM_REF *}  The application form reference.
**
** History:
**	02/89 (jhw)  Written.
**	9/89 (bobm) add fid argument to IIAMcdCatDel
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
*/
FORM_REF *
IIAMfaFormAlloc ( app, name )
APPL	*app;
char	*name;
{
	register FORM_REF	*fp;

	if ( (fp = (FORM_REF *)FEreqmem( app->data.tag, sizeof(*fp),
				TRUE, (STATUS *)NULL )) == NULL )
	{
		IIUGerr( E_UG0009_Bad_Memory_Allocation, UG_ERR_ERROR,
				1, ERx("IIAMformAlloc") 
		);
		return NULL;
	}
	fp->ooid = IIOOtmpId();
	fp->class = OC_AFORMREF;
	fp->name = FEtsalloc(app->data.tag, name);
	fp->owner = IIUIdbdata()->dba;
	fp->short_remark = _iiOOempty;
	fp->alter_date = fp->create_date = _iiOOempty;
	fp->symbol = _iiOOempty;
	fp->source = _iiOOempty;
	fp->refs = 0;
	fp->data.inDatabase = FALSE;
	fp->data.dirty = fp->data.dbObject = TRUE;
	fp->data.levelbits = 1;	/* level 0 */
	fp->data.tag = app->data.tag;

	return fp;
}

/*{
** Name:	iiamFoGetRef() -	Gets a Shared Form Reference for a Form.
**
** Description:
**	Gets a form reference for a form by searching the components list of an
**	application specified by the name of the form.  Otherwise, if one
**	is not found, then allocate and insert one into the components list.
**
**	The database catalog will NOT be searched if the reference is new.
**
**	Note also that this routine does not increment the reference count
**	for the form reference; the callers of this routine will take care
**	of it.
**
** Inputs:
**	app	{APPL *}  The application object to which the form belongs.
**	name	{char *}  The name of the form.
**
** Called by:
**	'IIAMapFetch()' (and hence, cannot access database.)
**
** Returns:
**	{FORM_REF *}  The application form reference.
**
** History:
**	02/89 (jhw)  Written.
*/
FORM_REF *
iiamFoGetRef ( app, name )
APPL	*app;
char	*name;
{
	register FORM_REF	*fp;

	if ( (fp = _lookupRef(app, name)) == NULL )
	{
		if ( (fp = IIAMfaFormAlloc(app, name)) != NULL )
			IIAMinsComp(app, fp);
	}
	return fp;
}

/*{
** Name:	IIAMformRef() -	Return a Form Reference for a Form.
**
** Description:
**	Gets a form reference for a form by searching the components list of an
**	application for it specified by the name of the form.  Otherwise, if one
**	is not found, then allocate and insert one into the components list.
**
**	The database catalog will also be searched if the reference is new, and
**	if not found in the database, a new object ID will be allocated.
**
**	This routine is called when an object containing a form reference is
**	created or fetched from the database.
**
**	Note also that this routine does not increment the reference count
**	for the form reference; when a new component that contains a form
**	reference is created, then it will be incremented.
**
** Inputs:
**	app	{APPL *}  The application object to which the form belongs.
**	name	{char *}  The name of the form.
**
** Returns:
**	{FORM_REF *}  The application form reference.
**
** History:
**	02/89 (jhw)  Written.
**	01/90 (jhw)  Moved reference count increment to relevant create routines
**		in "abf!abf!abcreate.qsc".  Bug #9533.
**	07/90 (jhw)  Remember to move temporary form references in application
**		components list when they are assigned a "real" ID.  Bug #31027.
*/
FORM_REF *
IIAMformRef ( app, name )
APPL	*app;
char	*name;
{
	register FORM_REF	*fp;

	if ( (fp = _lookupRef(app, name)) == NULL )
	{ /* completely new reference */
		if ( (fp = IIAMfaFormAlloc(app, name)) != NULL )
		{
			IIAMinsComp(app, fp);
		}
	}

	if ( fp != NULL )
	{
		if ( fp->ooid <= OC_UNDEFINED )
		{ /* look up in database */
			OOID	class;

			if ( IIAMoiObjId( name, OC_AFORMREF, app->ooid,
						&fp->ooid, &class ) == OK  )
			{
				/* note:  'IIAMapFetch()' will move the
				** temporary form reference.
				*/
				_VOID_ IIAMapFetch(app, fp->ooid, TRUE);
			}
			else
			{ /* allocate ID */
				/* remember!  move form reference in list */
				iiamMvComp(app, fp, IIOOnewId());
			}
		}
	}

	return fp;
}

/*
** Name:	_lookupRef() -	Lookup a Form Reference.
**
** Description:
**	Search the components list of an application for a form reference
**	specified by the name of the form.
**
** Inputs:
**	app	{APPL *}  The application object to which the form belongs.
**	name	{char *}  The name of the form.
**
** Returns:
**	{FORM_REF *}  The application form reference.
**
** History:
**	02/89 (jhw)  Written.
*/
static FORM_REF *
_lookupRef ( app, name )
APPL		*app;
register char	*name;
{
	register APPL_COMP	*cp;
	register FORM_REF	*fp;

	for ( cp = (APPL_COMP *)app->comps ; cp != NULL ; cp = cp->_next )
	{
		if ( ( cp->class == OC_AFORMREF || cp->class == OC_FORM )  &&
				STequal(cp->name, name) )
		{
			fp = (FORM_REF *)cp;
			break;
		}
	}
	return cp == NULL ? NULL : fp;
}

/*
** Name:	IIAMcrCountRef() - Count the number of objects 
**		that reference a particular form.
**
** Description:
**	Search the components list of an application for a form reference
**	specified by the name of the form.  For each occurance of a particular
**	form, add one to the form`s refernce count.
**
** Inputs:
**	app	{APPL *}  The application object to which the form belongs.
**      frm	{USER_FRAME *}  The frame object.
**
** Returns:
**	VOID.
**
** History:
** 	25-june-1993	(DonC) - Initial version.	
**	4-aug-1994 (mgw) Bug #64735
**		Don't compare form names for frames without forms thus
**		avoiding reference through a null pointer and AV.
*/
VOID 
IIAMcrCountRef ( app, frm )
APPL		*app;
USER_FRAME	*frm;
{
	register APPL_COMP	*cp;
	register USER_FRAME     *uf;
	register REPORT_FRAME   *rf;

	frm->form->refs = 1; /* Start at one becuase this search excludes
				the reference by the current frm object */

	for ( cp = (APPL_COMP *)app->comps ; cp != NULL ; cp = cp->_next )
	{
		if ( ( cp->class == OC_OSLFRAME || 
		       cp->class == OC_APPFRAME ||
		       cp->class == OC_APPFRAME ||
		       cp->class == OC_UPDFRAME ||
		       cp->class == OC_BRWFRAME ||
		       cp->class == OC_MUFRAME ))
		{
		    uf = (USER_FRAME *)cp;
		    if ( uf->form == (FORM_REF *)NULL )
			continue;
		    if ( STequal( uf->form->name, frm->form->name) ) 
		        frm->form->refs++;
                }
		else if ( cp->class == OC_RWFRAME ) 
		{	
		    rf = (REPORT_FRAME *)cp;
		    if ( rf->form == (FORM_REF *)NULL )
			continue;
		    if ( STequal( rf->form->name, frm->form->name) ) 
			frm->form->refs++;
		}
        }
        return;
} 
	
/*{
** Name:	IIAMfrRemoveRef() -	Remove a Form Reference.
**
** Description:
**	Remove a reference to a form.
**
** Inputs:
**	form	{FORM_REF *}  The form reference to be removed.
**
** History:
**	02/89 (jhw)  Written.
**	01/90 (jhw)  Do not pass in application, which was NULL.  Instead,
**		access application through the form reference.  Bug #9533.
*/
VOID
IIAMfrRemoveRef ( form )
FORM_REF	*form;
{

	if ( form == NULL || --form->refs > 0 )
		return;

	/* form is removable */
	if ( form->data.inDatabase && form->ooid >= 0 )
		IIAMcdCatDel( form->appl->ooid, form->ooid,
					OC_UNDEFINED, OC_AFORMREF );
	iiamRmComp(form->appl, form);
}

/*{
** Name:	IIAMfrChangeRef() -	Change a Form Reference for a New Form.
**
** Description:
**	Modify a reference to a form for a new form name by returning a
**	reference to a different form.  If the form reference would be
**	completely derefrenced by the change, simply rename.  Otherwise,
**	return a different form reference.
**
** Inputs:
**	app	{APPL *}  The application object to which the reference belongs.
**	form	{FORM_REF *}  The form reference to be changed.
**	name	{char *}  The name for the new form reference.
**
** Returns:
**	{FORM_REF *}  The application form reference.
**
** History:
**	02/89 (jhw)  Written.
*/
FORM_REF *
IIAMfrChangeRef ( app, form, name )
APPL			*app;
register FORM_REF	*form;
register char		*name;
{
	register FORM_REF	*tmp;
	OOID			frmid;
	OOID			class;

	/* assert:  form != NULL || ( name != NULL && *name != EOS ) */

	if ( form == NULL || --form->refs > 0 )
	{
		return ( name == NULL || *name == EOS )
				? NULL : IIAMformRef( app, name );
	}
	else if ( (tmp = _lookupRef(app, name)) != NULL  ||
			IIAMoiObjId( name, OC_AFORMREF, app->ooid,
					&frmid, &class ) == OK )
	{ /* reference found */
		if ( form->data.inDatabase && form->ooid >= 0 )
			IIAMcdCatDel( app->ooid, form->ooid,
						OC_UNDEFINED, OC_AFORMREF );
		iiamRmComp(app, form);

		if ( tmp != NULL  ||
				(tmp = IIAMfaFormAlloc(app, name)) != NULL )
		{
			if ( tmp->ooid <= OC_UNDEFINED )
			{ /* fetch from DB */
				tmp->ooid = frmid;
				IIAMinsComp(app, tmp);
				_VOID_ IIAMapFetch(app, tmp->ooid, TRUE);
			}
			++tmp->refs;	/* a(nother) reference */
		}

		return tmp;
	}
	else
	{
		form->name = FEtsalloc(form->data.tag, name);
		form->data.dirty = TRUE;
		form->refs = 1;

		return form;
	}
	/*NOT REACHED*/
}
