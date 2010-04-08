/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
**
*/

# include	<compat.h>
# include	<me.h>
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ooclass.h>
# include	<oodefine.h>
# include	<oosymbol.h>
# include	<oocat.h>
# include	"eroo.h"
# include	<uigdata.h>

/**
** Name:	ob.c -	Object Methods.
**
** Description:
**	Contains basic object methods.  Defines:
**
**	iiobAuthorized()	is current user authorized for object?
**	iiobCheckName()		check name with respect to object namespace.
**
** History:
**	Revision 6.0  87/07/13  bruceb
**	Changed memory allocation to use [FM]Ereqmem.
**	Revision 6.2  89/01  wong
**	Added 'iiobAuthorized()'.
**	Revision 8.0  89/07  wong
**	Added 'iiobCheckName()'.  Modified to use 'iiooMemAlloc()', etc.
**	8-feb-93 (blaise)
**		Changed _flush, etc. to ii_flush because of conflict with
**		Microsoft C library.
**	27-jan-93 (blaise)
**		The previous change changed all tabs in this file to spaces;
**		changed back to tabs again.
**      18-oct-1993 (kchin)
**          	Since the return type of OOsnd(), OOsndSelf() and OOsndAt() 
**		have been changed to PTR, when being called in OBsetRefNull(), 
**		and iiobCheckName(), their return types need to be cast to 
**		the proper datatypes.
**      12-oct-93 (daver)
**              Casted parameters to MEcopy() to avoid compiler warnings 
**		when this CL routine became prototyped.
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**      06-dec-93 (smc)
**		Bug #58882
**		Commented lint pointer truncation warning.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

OO_OBJECT	*iiooMemAlloc();
char		*iiooStrAlloc();

/*{
** Name:	iiobAuthorized() -	Is Current User Authorized for Object?
**
** Description:
**	Determines whether the current user is authorized to affect the
**	object (generally, this means `edit' the object.)
**
** Input:
**	self	{OO_OBJECT *}  The object.
**
** Returns:
**	{bool}  TRUE if authorized.
**
** History:
**	01/89 (jhw) -- Written.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
*/
bool
iiobAuthorized ( self )
OO_OBJECT	*self;
{
	return (bool)( self->owner != NULL && *self->owner != EOS &&
				STequal(self->owner, IIUIdbdata()->user) );
}

OOID
OBclass(self)
OO_OBJECT	*self;
{
	return self->class;
}

OOID
OBcopy(self)
OO_OBJECT *self;
{
	register OO_OBJECT	*obj;
	OO_OBJECT		*OBcopyId();

	OOsndSelf(self, _fetchAll);
	obj = OBcopyId(self);
	obj->ooid = IIOOnewId();
	obj->data.inDatabase = FALSE;
	return ( OOhash(obj->ooid, obj) ) ? obj->ooid : nil;
}

/*
** History:
**	13-jul-87 (bab) Changed memory allocation to use [FM]Ereqmem.
**	07/89 (jhw)  Modified to use 'iiooMemAlloc()' and 'iiooStrAlloc()'.
*/
OO_OBJECT *
OBcopyId (self)
register OO_OBJECT *self;
{
	OO_OBJECT	*obj;
	OO_CLASS	*classp = OOpclass(self->class);
	STATUS		stat;

	if ( (obj = iiooMemAlloc( classp, OC_UNDEFINED,
				FEgettag(), (u_i4)classp->memsize))
			== NULL )
	{
	    IIUGerr( E_OO001F_MEreqmem_err_OBcopyId, UG_ERR_ERROR, 1, &stat );
	    return nil;
	}
	MEcopy((PTR)self, (u_i2)classp->memsize, (PTR)obj);

	if ( obj->long_remark == NULL || *obj->long_remark == EOS )
	{ /* check for long remarks as well */
		char	buf[OOLONGREMSIZE+1];

		OOrd_seqText(self->ooid, buf, _ii_longremarks, _long_remark);
		obj->long_remark = iiooStrAlloc(obj, buf);
	}

	return obj;
}

OOID
OBedit (self)
OO_OBJECT *self;
{
	OOsndSelf(self, _subResp, _edit);
}

bool
OBisNil(self)
register OO_OBJECT *self;
{
	return (bool)( self == NULL || self->ooid == nil );
}

bool
OBisEmpty(self)
OO_OBJECT *self;
{
	return OBisNil(self);
}

OOID
OBmarkChange (self)
register OO_OBJECT *self;
{
	register i4  old;
	old = self->is_current;
	self->is_current = 0;
	return(old);
}

char *
OBname(self)
OO_OBJECT	*self;
{
	return self->name;
}

OOID
OBinit(self, name, env, own, cur, srem, cre, alter, lrem)
register OO_OBJECT *self;
char	*name;
i4	env;
char	*own;
i4	cur;
char	*srem, *cre, *alter, *lrem;
{
	self->name = name;
	self->env = env;
	self->owner = own;
	self->is_current = cur;
	self->short_remark = srem == NULL? "": srem;
	if (cre != NULL)
	    self->create_date = cre;
	if (alter != NULL)
	    self->alter_date = alter;
	self->long_remark = lrem;
	return self->ooid;
}

OOID
OBnoMethod(self, methSym)
OO_OBJECT	*self;
char	methSym[];
{
	char	*classnam;
	classnam = (char *)OOsnd(self->class, _name);
	IIUGerr( E_OO0020_does_not_understand, UG_ERR_ERROR, 3,
		classnam, self->name, methSym
	);
	return(self->ooid);
}

char *
OBrename(self, newname)
register OO_OBJECT	*self;
char	*newname;
{
	register char	*old;
	old = self->name;
	self->name = newname;
	if (OOsndSelf(self, ii_flush0) == nil)
	{
		self->name = old;
		return NULL;
	}
	return(old);
}

OOID
OBsetPermit ()
{
	return(0);
}

OOID
OBsetRefNull(self, ref)
register OO_OBJECT	*self;
register OOID	ref;
{
	register OO_CLASS	*class, *super;
	register char		*p = (char *) self;
	register OO_REFERENCE	*refp = (OO_REFERENCE*)OOp(ref);

#ifdef DDEBUG
	D(ERx("setRefNull(x%p (%d), x%p) offset = %d"), obj, obj->ooid, refp, refp->doffset);
#endif /* DDEBUG */

	MEfill(sizeof(OO_OBJECT*), 0, p + refp->doffset);

	class = OOpclass(self->class);
	super = OOpclass(class->super);

	/* find appropriate level to flush (assuming already in db) */
	for (; class->level; class = super, super = OOpclass(super->super))
	{
	
	    if (refp->doffset > super->memsize)
		 /* 2 lint truncation warnings if size of ptr > OOID, 
		    but code valid */	 
		 return ( OOisInDb(self) ) 
		      ? (OOID)OOsndAt(class, self, ii_flush, class)
			   : (OOID)OOsndSelf(self, ii_flushAll);
	}
	/* lint truncation warning if size of ptr > OOID, but code valid */
	return (OOID)OOsndAt(class, self, ii_flush);
}

OOID
OBsubResp(self, sym)
OO_OBJECT	*self;
char	sym[];
{
	IIUGerr( E_OO0021_Subclass_method, UG_ERR_ERROR, 1, sym );
	return nil;
}

/*{
** Name:	iiobCheckName() -	Check Name with Respect to Name Space.
**
** Description:
**	The legality of the name under which the object will be saved if new
**	is checked, and the database is checked for any name conflict with any
**	accessible object of the same type.  This is reported as an error.  If
**	the saved object will be a new one based on an edit of an existing one,
**	the new object is instantiated here, with the new ID being returned to
**	the caller.
**
**	If we're checking for a rename, we're checking any special requirements
**	on the name.  There aren't any at this generic level, so we'll
**	just return OC_UNDEFINED, which is success, since it isn't nil.
**
**	Notes:  Assumes that the name is a legal INGRES name.
**
** Input params:
**	obj		{OBJECT	*}	// object being created or saved.
**	savename	{char *}	// name under which to save object.
**	rename		{nat};		// Are we checking validity for a rename
**
** Returns:
**	{OOID}	ID of object to be saved
**		nil if error.
**
** Side Effects:
**	Existing object of same name may be retrieved from database;
**
** History:
**	07/89 (jhw)  Written as non-FRS version from 'iiobConfirmName()' that
**			does not prompt for overwrite.
**	10/90 (Mike S) Add argument, so we know if it's a Rename check.
*/
OOID
iiobCheckName ( obj, savename, rename )
register OO_OBJECT	*obj;
char			*savename;
i4			rename;
{
	if (rename)
		return OC_UNDEFINED;	/* No special check to make */

	if ( !STequal(obj->name, savename) || !obj->data.inDatabase )
	{ /* new name or new object */
	    OOID	id2 = OC_UNDEFINED;
	    OOID	class = obj->class;

	    /* lint truncation warning if size of ptr > OOID, but code valid */
	    if ( (id2 = (OOID)OOsnd(class, _withName, savename, (char*)NULL)) != nil )
	    { /* already exists in DB ... */
		OO_OBJECT	*obj2;

		obj2 = OOp(id2);
		/* lint truncation warning if size of ptr > OOID, 
		   but code valid */
		if ( ! (bool)OOsndSelf(obj2, _authorized) )
		{ /* ... cannot change it (likely owned by the DBA) */
			IIUGerr( S_OO002C_OwnedByDBA, UG_ERR_ERROR,
					2, OOpclass(class)->name, savename
			);
		}
		else
		{
			IIUGerr( S_OO002E_ExistsInDatabase, UG_ERR_ERROR,
					2, OOpclass(class)->name, savename
			);
		}
		return nil;
	    }
	    if ( obj->data.inDatabase )
	    { /* requires new object in DB ... */
		/*
		** A new name here means to save as a different object.
		** If ID already represents an existing object (i.e., is
		** a permanent ID,) then we have to create a different
		** installed object structure other than that currently
		** pointed to by 'obj'.
		*/
		/* lint truncation warning if size of ptr > OOID, 
		   but code valid */
		id2 = (OOID)OOsnd(obj->ooid, _copy);
		obj = OOp(id2);
		obj->create_date = NULL;
	    }
	    obj->name = iiooStrAlloc(obj, savename);
	}
	return obj->ooid;
}
