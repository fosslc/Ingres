/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<st.h>
# include	<er.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<oocollec.h>
# include	<oodefine.h>
# include	<oosymbol.h>
# include	"eroo.h"

/**
** Name:	obencode.c	-  code for encoding/decoding objects.
**
** Description:
**	Objects can be translated to a encoded-string form and
**	decoded from that form into object data structures.  The
**	encoded string form is a machine-independent character
**	representation that can be efficiently stored in the database
**	as a sequence of tuples containing a lengthy text field
**	comprising all the attributes of an object.  The encoded
**	string is represented in the object schema as an attribute of
**	class OC_ENCODING, instances of which are stored in the table
**	"iiencoding".  Specific classes can further enhance database
**	storage/retrieval efficiency by incorporating component objects
**	into their own encoded string representation and storing,
**	retrieving and decoding the entire encoded object as a
**	single unit.
**
**	This file defines:
**
**	OBencode()	produce a encoded string representing an object.
**	OBencAtts()	outputs encoded representation of each attribute.
**
** History:
**
**      17-jan-93 (leighb)
**              Allocate "buf" instead of using stack.  (Stack overflowed                
**              on PC with buf on the stack.)
**      15-sep-93 (kchin)
**              Changed use of FORCE_ALIGN macro to ME_ALIGN_MACRO in
**		OBencAtts().  We should stick to one standard alignment
**		interface in the CL.
**      18-oct-1993 (kchin)
**          	Since the return type of OOsndSelf() has been changed to PTR,
**          	when being called in OBencode(), its return type
**          	needs to be cast to the proper datatype.
**      06-dec-93 (smc)
**		Bug #58882
** 	        Commented lint pointer truncation warning.
**      15-jan-1996 (toumi01; from 1.1 axp_osf port)
**              Added kchin's change (from 6.4) for axp_osf
**              31-aug-93 (kchin)
**              Need to align pointer: p to 8-byte boundary (if on 64-bit
**              pointer machine) in OBencAtts().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-sep-06 (gupsh01)
**		Added support for ANSI date/time types.
*/

PTR	OBencAtts();

OOID
OBencode(self, level, version)
OO_OBJECT	*self;
i4		level;
bool		version;	/* if TRUE output version stamp */
{
	i4		id = self->ooid;

	/* lint truncation warning if size of ptr > OOID, but code valid */
	id = (i4)OOsndSelf(self, _fetchAll);
	self = OOp(id);
	if (version)
	    P(ERx("V60\n"));
	P(ERx("*%d{"), id);
	_VOID_ OBencAtts(self, self->class, level);
	P(ERx("}"));
	return self->ooid;
}

PTR
OBencAtts(self, class, level)
OO_OBJECT	*self;
OOID	class;
i4	level;
{
	register OO_CLASS	*k = OOpclass(class);
	OO_CLASS		*ksup;
	register OO_aCOLLECTION	*ca;
	register i4		j;
	register char		*p = NULL;
	register i4		hasDetailRefs;
	register OOID		id;
	register char		*s;
	OOID			detailref();
	OO_ATTRIB		*f;
	char			*buf, *b;

	if (k->super != nil && k->level > level) {
	    p = (char *)OBencAtts(self, k->super, level);
	}
	else
	{
	    /* output class level elision markers for absent level(s) */
	    for (j = k->level; j--; )
		P(ERx("/"));
	}
	if (p == NULL)
	{
		if (class != OC_OBJECT)
		{
		    ksup = OOpclass(k->super);
		    p = (char *)self + ksup->memsize;
		}
		else
		    p = (char *)self + OOHDRSIZE;
	}
	if (OOsnd(k->attributes, _isEmpty))
		return (PTR)p;

	buf = MEreqmem((u_i4) 0, (u_i4) (10 * ESTRINGSIZE), FALSE, NULL);

	hasDetailRefs = (i4) !OOsnd(k->detailRefs, _isEmpty);
	ca = (OO_aCOLLECTION*)OOp(k->attributes);
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
	      case DB_DMY_TYPE:

                    /* need to align p to 8-byte boundary (if on 64-bit pointer
                       machine), since it will be used to access to pointers
                                                               -kchin */
                    p = ME_ALIGN_MACRO(p,sizeof(PTR));

		    if (s =  *(char **)p)
		    {
			b = buf;
			P(ERx("'"));
			/* escape any literal "'" characters */
			while (*b = *s++)
			{
			    if (*b == '\'' || *b == '\\')
			    {
				*b++ = '\\';
				*b = s[-1];
			    }
			    b++;
			}
			P(ERx("%s'"), buf);
		    }
		    else
			P(ERx("\""));
		    p += sizeof (char *);
		    break;

	      case DB_INT_TYPE:
		    if (hasDetailRefs && detailref(k->detailRefs, f) != nil)
		    {
			id = *((OOID *)p);
			switch (id)
			{
			    case	nil:
				P(ERx("N"));
				break;

			    case	UNKNOWN:
				P(ERx("U"));
				break;

			    default:
				P(ERx("O%d"), id);
			}
		    }
		    else
		    {
			P(ERx("I%d"), *(i4 *)p);
		    }
		    p += sizeof (i4);
		    break;

	      case DB_FLT_TYPE:
	      {
		    PTR		aligned_p;

		    aligned_p = (PTR)ME_ALIGN_MACRO(p,sizeof(ALIGN_RESTRICT));
		    STprintf(buf, ERx("%.6f"), *(double *)aligned_p, '.');
		    b = buf + STlength(buf);
		    while (*(--b) == '0')
			;
		    if (*b != '.') b++;
		    *b = '\0';
		    P(ERx("F%s"), buf);
		    p = (char *) aligned_p + sizeof (f8);
		    break;
	      }

	      default:
		    err(E_OO0023_Unexpected_attribute_, f->frmt);
	    }
	}

	/* master reference collections */
	if (!OOsnd(k->masterRefs, _isEmpty))
	{
	    register OO_COLLECTION	*co;

	    co = (OO_COLLECTION*)OOp(k->masterRefs);
	    for (j = 0; j < co->size; j++)
	    {
		OOID	res;
		OO_REFERENCE	*r;
		i4	i;

		if (*((OOID *)p) == (OOID)UNKNOWN)
		{
		    P(ERx("U"));
		}
		else
		{
		    r = (OO_REFERENCE*)OOp(co->array[j]);
		    res = OOcheckRef(self, co->array[j], r->moffset);
		    if (res != nil)
		    {
			OO_COLLECTION	*resp = (OO_COLLECTION*)OOp(res);
			P(ERx("["));
			for (i = 0; i < resp->size; i++)
			{
			    id = resp->array[i];
			    switch (id)
			    {
				case	nil:
				    P(ERx("N"));
				    break;

				case	UNKNOWN:
				    P(ERx("U"));
				    break;

				default:
				    P(ERx("O%d"), id);
			    }
			}
			P(ERx("]"));
		    }
		    else
			P(ERx("N"));
		}
		p += sizeof(OOID);
	    }
	}

	/* now for variable length objects print array[0]...*/
	/* NB: this won't actually work right ... */
	/* if (k->hasarray && ((OO_COLLECTION*)self)->size)
	** {
	**     P("O%d", ((OO_COLLECTION*)self)->array[0]);
	** }
	*/

	/* end class level with class level end marker ';' */
	P(ERx(";"));

	MEfree(buf);

	return (PTR)p;
}
