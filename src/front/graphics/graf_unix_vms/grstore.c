/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ergr.h>
# include	<er.h>
# include	<ug.h>
# include	<graf.h>

/**
** Name:	grstore.c -	Graphics System Memory Storage Module.
**
** Description:
**	Contains routines that allocate and free storage for objects in the
**	Graphics System.  Defines:
**
**	GRstinit()	storage initialization.
**	GRstfree()	storage free.
**	GRtxtfree()	free storage for text strings.
**	GRtxtstore()	allocate storage for text string.
**	GRretobj()	return object to storage.
**	GRgetobj()	allocate object from storage.
**
** History:
**	86/03/03  19:06:24  wong
**		Removed tag alloction routines; not used.
**
**	86/03/11  15:45:34  bobm
**		bomb-proof legend pointers by assigning at create time.
**
**	86/03/12  14:48:04  wong
**		Name changes.
**
**	86/04/15  18:08:30  bobm
**		NULL pointer handling in GRtxtstore
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# define	STR_BLOCK 600

/*
** Name:	ATTR_UNION -	Union of Attributes for Types
**					Other Than GR_TEXT.
** Description:
**	A union defined for storage allocation from a pool.  (So that
**	"sizeof(ATTR_UNION)" can be used to allocate a list pool.)
*/
typedef union
{
	BARCHART bar;
	LINECHART line;
	SCATCHART scat;
	PIECHART pie;
	LEGEND leg;
} ATTR_UNION;

/*
** Name:	storage_pool -	Storage Structures.
**
** Description:
**	Three list pools for objects:
**
**	One for GR_OBJ nodes which are created and deleted with great
**	abandon, and quite often with no attribute data attached
**	(see erase/gallery routines).
**
**	One for GR_TEXT attribute nodes, which are likely to be more dynamic
**	than other types, are smaller than other attribute nodes, and are
**	created as phony objects temporarily for font choice displays.
**
**	One for other attribute types, which we aren't liable to have
**	many of at once.
*/

static FREELIST *Attr_pool;	/* pool of types in ATTR_UNION */
static FREELIST *Obj_pool;	/* pool of GR_OBJ nodes */
static FREELIST *Text_pool;	/* pool of GR_TEXT nodes */

static i2	 Str_tag;	/* tag for text storage */

/*{
** Name:	GRstinit() -	Memory Storage Initialization.
**
** Description:
**	Initializes memory storage for Graphics System objects.
**
** Exceptions:
**	fatal error on allocation failure.
**
** History:
**	09/85 (rlm) -- Written.
*/

VOID
GRstinit ()
{
	if ((Attr_pool = FElpcreate(sizeof(ATTR_UNION))) == NULL)
		IIUGbmaBadMemoryAllocation(ERx("GRstinit - Attr"));
	if ((Obj_pool = FElpcreate(sizeof(GR_OBJ))) == NULL)
		IIUGbmaBadMemoryAllocation(ERx("GRstinit - Obj"));
	if ((Text_pool = FElpcreate(sizeof(GR_TEXT))) == NULL)
		IIUGbmaBadMemoryAllocation(ERx("GRstinit - Text"));
	Str_tag = FEgettag();
}

/*{
** Name:	GRstfree() -	Memory Storage Free.
**
** Description:
**	Frees allocated list pools for Graphics System objects.
**
** Side Effects:
**	Resets 'Attr_pool', 'Obj_pool', 'Text_pool' and 'Str_tag'.
**
** History:
**	02/86 (jhw) -- Written.
*/

VOID
GRstfree ()
{
	_VOID_ FElpdestroy(Attr_pool);
	_VOID_ FElpdestroy(Obj_pool);
	_VOID_ FElpdestroy(Text_pool);
	FEfree(Str_tag);
}

/*{
** Name:	GRtxtfree() -	Memory Free Storage for Text Strings.
**
** Description:
**	Frees tagged, allocated storage for text strings and resets tag for
**	re-allocation.
**
** Side Effects:
**	Resets 'Str_tag' with new value.
**
** History:
**	09/85 (rlm) -- Written.
*/

VOID
GRtxtfree ()
{
	FEfree(Str_tag);
	Str_tag = FEgettag();
}

char *
GRtxtstore (s)
char *s;
{
	register char	*rs;

	/* FEtsalloc blows up on NULL pointers */
	if (s == NULL)
		s = ERx("");

	if ((rs = FEtsalloc(Str_tag, s)) == NULL)
		IIUGbmaBadMemoryAllocation(ERx("GRtxtstore"));
	return (rs);
}

GRretobj (obj)
GR_OBJ *obj;
{
	FREELIST *fptr;
	i4 erarg;

	/* get appropriate list pool for attribute pointer */
	switch (obj->type)
	{
	case GROB_BAR:
	case GROB_LINE:
	case GROB_SCAT:
	case GROB_PIE:
	case GROB_LEG:
		fptr = Attr_pool;
		break;
	case GROB_TRIM:
		fptr = Text_pool;
		break;
	default:
		if (obj->type == GROB_NULL)
		{
			fptr = NULL;
			break;
		}
		erarg = obj->type;
		IIUGerr(S_GR0010_bad_class, UG_ERR_FATAL, 1, &erarg);
	}

	/* return to pool(s) */
	if (fptr != NULL)
		FElpret (fptr,obj->attr);
	FElpret (Obj_pool,obj);
}

GR_OBJ *
GRgetobj (class)
OOID	class;
{
	FREELIST	*fptr;
	register GR_OBJ *rptr;	/* returned pointer */
	i4		erarg;

	/* get appropriate list pool for attribute pointer */
	switch (class)
	{
	case GROB_BAR:
	case GROB_LINE:
	case GROB_SCAT:
	case GROB_PIE:
	case GROB_LEG:
		fptr = Attr_pool;
		break;
	case GROB_TRIM:
		fptr = Text_pool;
		break;
	default:
		if (class == GROB_NULL)
		{
			fptr = NULL;
			break;
		}
		erarg = class;
		IIUGerr(S_GR0023_GRgetobj_bad_class, UG_ERR_FATAL, 1, &erarg);
	}	/* end switch */

	/* get object node */
	if ((rptr = (GR_OBJ *) FElpget(Obj_pool)) == NULL)
		IIUGbmaBadMemoryAllocation(ERx("GRgetobj - obj"));

	/*
	**	Fill in with defaults, most of which will be overwritten.
	**	id and class are important.
	*/
	rptr->type = class;
	rptr->goflgs = 0;
	rptr->bck = BLACK;	/* background color */
	rptr->prev = rptr->next = NULL;
	rptr->scratch = rptr->layer = rptr->order = -1;
	rptr->ext.xlow = rptr->ext.ylow = 0.0;
	rptr->ext.xhi = rptr->ext.yhi = 1.0;
	rptr->id = -1;

	/* if not a null class, get attribute node */
	if (fptr != NULL)
	{
		if ((rptr->attr = (i4 *)FElpget(fptr)) == NULL)
			IIUGbmaBadMemoryAllocation(ERx("GRgetobj - attr"));
		/*
		** if a legend pointer, set up initial cross - object
		** association pointer - should be overwritten by
		** 'GRlgchk()' - this is bomb-proofing.
		*/
		if (class == GROB_LEG)
		{
			((LEGEND *) rptr->attr)->obj = rptr;
			((LEGEND *) rptr->attr)->assoc = NULL;
		}
	}

	return (rptr);
}
