/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ooclass.h>
#include	<oodefine.h>
# include	"ooldef.h"
#include	<oosymbol.h>
#include	"eroo.h"

/**
** History:
**      22-oct-90 (sandyd)
**              Fixed #include of local ooldef.h to use "" instead of <>.
**      10-sep-93 (kchin)
**              Changed return type of OOsnd(), OOsndAt, OOsndSelf(), and
**		OOsndSuper() from OOID to PTR.  As these functions will 
**		occassionally return pointers.  If their return types are
**		OOID (ie. int), 64-bit addresses will be truncated.
**      18-oct-93 (kchin)
**              Since variable 'ret' in OOsndAt() is expecting a PTR, cast
**		the return type of the routine it calls to PTR, the default
**		int return type will result in 64-bit address being
**		truncated and compiler warning message on 64-bit platform.
**	15-Nov-1993 (tad)
**		Bug #56449
**		Replace %x with %p for pointer values where necessary.
**      03-Feb-96 (fanra01)
**              Extracted data for DLLs on Windows NT.
**      21-nov-96 (hanch04)
**          Problems linking for unix
**      23-jun-99 (hweho01)
**              Since variable 'ret' in OOsndAt() is expecting a PTR,
**              a pointer to function that will retrun a PTR must be  
**              declared and used on ris_u64 platform.  without the   
**              explicit definition, the default return value for a  
**              function call will truncate the 64-bit address.
**	18-May-2009 (kschendel) b122041
**	    Compiler warning fixes.  Change return types to SCALARP since
**	    these routines usually return an integer-ish thing (OOID), but
**	    can return pointer-sized things.
**	24-Aug-2009 (kschendel) 121804
**	    checkAt is bool, define for gcc 4.3
**/

# if defined(NT_GENERIC)
GLOBALREF	OO_METHOD	*_CurMeth;
GLOBALREF	OO_OBJECT	*Self;
# else
GLOBALDEF       OO_METHOD       *_CurMeth = (OO_METHOD*) iiOOnil;
GLOBALDEF       OO_OBJECT       *Self = iiOOnil;
# endif             /* NT_GENERIC  */

FUNC_EXTERN bool checkAt(OO_OBJECT *, OO_CLASS *, i4);

/* ...
OO_CLASS	*_AtClass;
char	*_Msg;
*/

/*{
** Name:	OOsnd() -	send a message to an object, i.e.,
**				invoke an OO method.
**
** Description:
**	OOsnd() is the principal interface to the ObjectUtility
**	message dispatcher.  The object referenced in 'obj' as
**	an OOID is resolved to an OO_OBJECT pointer.  The OO
**	kernel message dispatch routine, OOsndAt, is directed to
**	look up and invoke a method matching selector 'sym',
**	starting the search in the class of 'obj' and searching
**	up the superclass chain to class Object until a matching
**	method is found or the set of methods is exhausted.
**
**	The  first two parameters of the function are fixed, with
**	the remainder a varying number (up to 10) of varying type.
**
**	The matching method function is invoked with the resolved
**	object pointer (referred to as "self") as its first argument,
**	followed by arguments a0, a1, ... a9.
**
** Input params:
**	OOID	obj;		// object id
**	char	sym[];		// meethod selector
**	PTR	a0, a1, ..., a9;  // arbitrary arguments
**
** Output params:
**	<varying>
**
** Returns:
**	OOID		// nominal return value declaration
*/

/* VARARGS2 */
SCALARP
OOsnd(obj, sym, a1, a2, a3, a4, a5, a6,a7,a8,a9,a10,a11,a12)
OOID	obj;
char	sym[];
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12;
{
	/* Self=0;Selfid=obj;_AtClass=0;_Msg=sym;OOsndAt a; */
	return OOsndAt((OO_CLASS *)NULL, OOp(obj), sym,
			a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12
	);
}

/*{
** Name:	OOsndSelf() -	send a message to object "self"
**
** Description:
**	OOsndSelf() is a variant of OOsnd which is used within
**	OO method functions to send a message to "self" (the current
**	receiver object -- first parameter of any method function).
**	The "self" object is already resolved to a memory pointer,
**	thus conversion from an OOID is not required.  Otherwise
**	OOsndSelf() functions exactly like OOsnd.
**
** Input params:
**	OO_OBJECT	self;		// object "self"
**	char		sym[];		// method selector
**	PTR		a0, a1, ..., a9;  // arbitrary arguments
**
** Output params:
**	<varying>
**
** Returns:
**	OOID		// nominal return value declaration
**
*/

/* VARARGS2 */
SCALARP
OOsndSelf(self, sym, a1, a2, a3, a4, a5, a6,a7,a8,a9,a10,a11,a12)
register OO_OBJECT	*self;
char	sym[];
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12;
{
	/* _Msg=sym;OOsndAt a; */
	return OOsndAt(self->class == OC_CLASS? (OO_CLASS*)self: OOpclass(self->class),
		    self,sym,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12);
}

/*{
** Name:	OOsndSuper() -	send a message to object "self", but
**				start method search in superclass of
**				the class to which the current method
**				function belongs.
**
** Description:
**	OOsndSuper() is a variant on OOsnd() and OOsndSelf().
**	It is only used withing the code of an OO method function.
**	It send a message to "self", similarly to OOsndSelf(),
**	except that the search for the matching method starts in
**	the superclass of the class to which the current method
**	function belongs.
**
** Input params:
**	OO_OBJECT	*self;		// object "self"
**	char		sym[];		// method selector
**	PTR		a0, a1, ..., a9;  // arbitrary arguments
**
** Output params:
**	<varying>
**
** Returns:
**	OOID		// nominal return value declaration
**
*/

/* VARARGS2 */
SCALARP
OOsndSuper(self, sym, a1, a2, a3, a4, a5, a6,a7,a8,a9,a10,a11,a12)
OO_OBJECT	*self;
char	sym[];
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12;
{
	register OO_CLASS	*msup;
	/* Self=self;_AtClass=_CurMeth->mclass->super=OOpclass(_CurMeth->mclass->super);_Msg=sym;OOsndAt a; */

	/* check that _CurMeth->mclass->super is fetched at level 1 */
	msup = OOpclass((OOpclass(_CurMeth->mclass))->super);

	return OOsndAt(msup,self,sym,
			a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12
	);
}


/* VARARGS3 */
SCALARP
OOsndAt ( class, self, sym, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12 )
register OO_CLASS	*class;
register OO_OBJECT	*self;
char			sym[];
PTR	a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12;
{
	register OO_OBJECT *prevSelf;
	SCALARP		ret;
	OOID		methid;
	register OO_METHOD	*meth, *prevMeth;
	bool		classmeth;

	OOID		iiooClook();
	OOID		(*Mproc())();


	if ( self == NULL )
	{
		IIUGerr(E_OO003C_NULL_receiver, UG_ERR_ERROR, 1, sym);
		return nil;
	}

	if ( class == NULL )
	{ /* check that self->class is fetched at level 1 */
		class = OOisMeta(self->class)?
			(OO_CLASS*) self: OOpclass(self->class);
	}

	/* class and self assumed to be fetched at this point */

	/* save previous Self object */
	prevSelf = Self;
	Self = self;

	/* look up method starting at class */
	if ( (methid = iiooClook(class, sym, OOisMeta(self->class))) == nil )
	{ /* ran out of supers (i.e. at OC_OBJECT) */
		OOsndSelf(self, _noMethod, sym);
		return nil;
	}
	meth = (OO_METHOD*)OOp(methid);

	/* found method at some level */
#ifdef DDEBUG
	D(ERx("\tfound at %s.%s"), meth->mclass->name, sym);
#endif /* DDEBUG */

	/* save meth for possible OOsndSupers in this method */
	prevMeth = _CurMeth;
	_CurMeth = meth;
	if ( meth->entrypt == NULL )
	{ /* find procedure in proctab and save */
	    meth->entrypt = Mproc(meth->ooid);
	}

	/* check object is fetched at appropriate level(s) */
	/* (level 0 already guaranteed fetched) */
	if (meth->fetchlevel)
	    checkAt(self, class, meth->fetchlevel);

	ret = (SCALARP)(*meth->entrypt)(self, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
        
#ifdef DDEBUG
	D(ERx("\t%s.%s returning: x%p"), class->name, sym, ret);
#endif /* DDEBUG */

	/* restore meths for OOsndSupers */
	_CurMeth = prevMeth;
	Self = prevSelf;

	/* log the method invocation if indicated by BOTH	*/
	/* the object's class AND the method.			*/
	if (class->keepstatus && meth->keepstatus)
		putstatus(self, meth);

	return(ret);
}
