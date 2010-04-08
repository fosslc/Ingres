/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<oocollec.h>
# include	<oodefine.h>
# include	<oosymbol.h>
# include	<er.h>
# include	<me.h>

/**
** Name:    obprint.qc	-  code for printing objects.
**
** Description:
**	Print a representation of the attributes of an object
**	one line per attribute, giving attribute names and values.
**	Attributes containing object id's as values are so
**	indicated.
**
**	This file defines:
**
**	OBprint()	print out a representation of an object.
**	OBprAtts()	outputs printed representation of each attribute.
**
** History:
**		Revision 40.103	 86/04/09  15:56:19  peterk
**		cast arg to Psetfile to placate lint.
**
**		Revision 40.101	 86/03/10  14:34:42  peterk
**		change #include "defines.h" to <oodefine.h>
**
**		Revision 40.1  86/02/03	 11:49:00  peterk
**		initial version
**
**      15-sep-93 (kchin)
**              Changed use of FORCE_ALIGN macro to ME_ALIGN_MACRO in
**		OBprAtts().  We should stick to one standard alignment
**		interface in the CL.
**	5-oct-93 (deannaw)
**		Added "include <me.h>" to define ME_ALIGN_MACRO which was
**		changed by kchin (see change above).
**      18-oct-1993 (kchin)
**          	Since the return type of OOsndSelf() has been changed to PTR,
**          	when being called in OBprint(), its return
**          	type needs to be cast to the proper datatype.
**      06-dec-93 (smc)
**		Bug #58882
**              Commented lint pointer truncation warning.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-sep-2006 (gupsh01)
**	    Added support for ANSI date/time types.
**/

OBprint(self)
register OO_OBJECT	*self;
{
	OOID	id;
        /* lint truncation warning if size of ptr > OOID, but code valid */
	id = (OOID)OOsndSelf(self, _fetchAll, 0);
	self = OOp(id);
	Psetfile((char *)1);
	P(ERx("{ /* %s	    */"), OOsnd(self->class, _name, 0));
	P(ERx("	 /* id\t*/ %d"), self->ooid);
	OBprAtts(self, self->class);
	P(ERx("}"));
}

OBprAtts(self, class)
register OO_OBJECT	*self;
register OOID	class;
{
	register OO_CLASS	*k = OOpclass(class);
	register OO_aCOLLECTION	*ca;
	register i4	j;
	OOID		detailref();
	register i4	hasDetailRefs;
	register char	*p = (char *)self;
	register OO_ATTRIB	*f;

	p += (k->super != nil) ? OBprAtts(self, k->super) : OOHDRSIZE;

	if (OOsnd(k->attributes, _isEmpty, 0))
		return k->memsize;

	hasDetailRefs = (i4) !OOsnd(k->detailRefs, _isEmpty, 0);
	ca = (OO_aCOLLECTION*) OOp(k->attributes);
	for (j = 0; j < ca->size; j++) {
	    f = (OO_ATTRIB*) ca->array[j];
	    switch (f->frmt) {
	      case DB_CHR_TYPE:
	      case DB_TXT_TYPE:
	      case DB_DTE_TYPE:
	      case DB_ADTE_TYPE:
	      case DB_TMWO_TYPE:
	      case DB_TMW_TYPE:
	      case DB_TME_TYPE:
	      case DB_TSWO_TYPE:
	      case DB_TSW_TYPE:
	      case DB_TSTMP_TYPE:
	      case DB_INDS_TYPE:
	      case DB_INYM_TYPE:
		    P(ERx("  /* %s\t*/ \"%s\","), f->name, *(char **)p);
		    p += sizeof(char *);
		    break;

	      case DB_INT_TYPE:
		    if (hasDetailRefs && detailref(k->detailRefs, f) != nil)
		    {
			P(ERx("	 /* %s (ID) */ %d,"), f->name, *((OO_OBJECT **)p));
		    }
		    else {
			P(ERx("	 /* %s\t*/ %d,"), f->name, *(i4 *)p);
		    }
		    p += sizeof(i4);
		    break;

	      case DB_FLT_TYPE:
	      {
		    PTR		aligned_p;

		    aligned_p = (PTR)ME_ALIGN_MACRO(p,sizeof(ALIGN_RESTRICT));
		    P(ERx("  /* %s\t*/ %12.6f,"), f->name, *(double *)aligned_p,
			       '.');
		    p = (char *) aligned_p + sizeof (double);
	      }
	    }
	}

	/* just print out master reference collection IDs for now... */
	if (!OOsnd(( k->masterRefs), _isEmpty, 0))
	{
	    register OO_COLLECTION	*co;

	    co = (OO_COLLECTION*) OOp(k->masterRefs);
	    for (j = 0; j < co->size; j++) {
		P(ERx("	 /* %s (ID)\t*/ %d,"),
		    OOsnd(co->array[j], _name, 0), *((OO_OBJECT **)p));
		p += sizeof(OO_OBJECT*);
	    }
	}
	/* now for variable length objects print array[0]...*/
	if (k->hasarray && ((OO_COLLECTION*)self)->size) {
	    P(ERx("  /* array[0] (ID)	    */ %d,"), ((OO_COLLECTION*)self)->array[0]);
	}

	return k->memsize;
}
