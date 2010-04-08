/*
** Copyright (c) 1984, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<st.h>
# include	<er.h>
# include	<tm.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<oodep.h>
# include	<dmchecks.h>
# include	"iamdmtyp.h"

# if defined(i64_win)
# pragma optimize("s", on)
# endif

/*
** Name:	iamdm.c	- ABF/4GL dependency manager routines
**
** Description:
**	This file defines:
**
**	iiamDMIinitDM  		Initialize dependency manager
**	IIAMfctFindCompTable	Find component in hash table
**	iiamxDRdeleteRelation	Delete a relation
**	iiamxARaddRelation	Add a relation
**	iiamxGEgetEntity	Find (or create) an entity
**	iiamxDEdeleteEntity	Delete an entity
**	iiamxDGdeleteGraph	Delete an entity's relations
**	iiamxCVclearVisitedBit	Clear the "visited" bits for all entities
**
** History:
**	7/19/90 (Mike S) Initial version
**	03/09/91 (emerson)
**		Integrated DESKTOP porting changes.
**	27-aug-1993 (twai)
**		Change char *s in DM_hash() to unsigned char *s to accept
**		double-byte characters in frame name. (Bug no. 48204)
**	25-Oct-1993 (teresal)
**		Cast remainder expression in DM_hash() to shut up 
**		ANSI C compiler warning. 
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-dec-2001 (somsa01)
**	    Added LEVEL1_OPTIM for i64_win to prevent "Saving frame" in
**	    iiabf from looping forever in iiamxGEgetEntity().
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
*/

/* defines */
# define HASH_SIZE 255
# define NUM_ENTS 50	/* Number of entities to allocate at once */
# define NUM_RELS 50	/* Number of relations to allocate at once */

/* GLOBALDEF's */
/* 
** Has the graph been initialized.  Some clients of this module may never 
** initialize the graph.  Current examples are copyapp and OSL.  ABF 
** initializes the graph when it edits an application. 
*/
GLOBALDEF bool iiamxGIgraphInit = FALSE;
GLOBALDEF OOID  iiamxAIapplId = 0;	/* Application ID */

/* extern's */
FUNC_EXTERN     VOID 	IIAMactAddCompTable();
FUNC_EXTERN	DM_ENT 	*iiamxGEgetEntity();
FUNC_EXTERN     STATUS 	IIUGhdHtabDel();
FUNC_EXTERN     STATUS 	IIUGheHtabEnter();
FUNC_EXTERN     STATUS 	IIUGhfHtabFind();
FUNC_EXTERN     STATUS 	IIUGhiHtabInit();
FUNC_EXTERN     VOID 	IIUGhrHtabRelease();

/* static's */
static VOID	delete_rels();
static OOID	generic_class();
static DM_REL	*get_relation();
static i4	DM_hash();
static i4	DM_compare();

/* Previous primary entity for iiamxARaddRelation */
static	DM_ENT *saved_ent;	

static TAGID graphTag = 0;	/* Tag to allocate graph objects on */

/* Pointers to free entities and relations */
static DM_ENT 		*free_ents;
static DM_REL 		*free_rels;

static PTR DM_hash_id;

/*{
** Name:	iiamDMIinitDM  Initialize dependency manager
**
** Description:
**	Create or re-initialize hash tables for dependency and components.
**
** Inputs:
**	applid	OOID		Application id
**	appl	APPL		* Current application object.  May be NULL
**
** Outputs:
**	none
**
**	Returns:
**		none
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	7/19/90 (Mike S) Initial version
*/
VOID
iiamDMIinitDM(applid, appl)
OOID	applid;
APPL	*appl;
{
	APPL_COMP	*comp;

	if (iiamxGIgraphInit)
	{
		/* Get rid of the hash table */
		IIUGhrHtabRelease(DM_hash_id);

		/* Free all entities and relations */
		FEfree(graphTag);
	}

	/* Init hash table for components */
	_VOID_ IIUGhiHtabInit(HASH_SIZE, (VOID (*)())NULL, 
				DM_compare, DM_hash,
				&DM_hash_id);
	free_rels = NULL;
	free_ents = NULL;
	if (graphTag == 0)
		graphTag = FEgettag();

	/* Put all exisitng components into the hash table */
	if (appl != NULL)
		for (comp = (APPL_COMP *)appl->comps; 
		     comp != NULL; 
		     comp = comp->_next)
			IIAMactAddCompTable(comp);

	iiamxAIapplId = applid;
	iiamxGIgraphInit = TRUE;
}

/*{
** Name:	IIAMfctFindCompTable
**
** Description:
**	Find a component's entity in the hash table.  The class parameter 
**	needn't be an exact match right; we genricize it before looking it up.
**
** Inputs:
**	name		char *	Component name
**	class		OOID	Object class
**
** Outputs:
**	none
**
**	Returns:
**		DM_ENT * Object entity.  NULL if not found
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	7/19/90 (MIke S)	Initial version
*/
DM_ENT *
IIAMfctFindCompTable(name, class)
char	*name;
OOID	class;
{
	DM_ENT tmp_ent;
	PTR ptr;

	STcopy(name, tmp_ent.name);
	tmp_ent.class = generic_class(class);

	if (IIUGhfHtabFind(DM_hash_id, (char *)&tmp_ent, &ptr) == OK)
		return (DM_ENT *)ptr;
	else
		return (DM_ENT *)NULL;
}


/*
**	iiamxDRdeleteRelation
**
**	delete a relation.  Relink everything it points to.
*/
VOID
iiamxDRdeleteRelation(rel)
DM_REL *rel;	/* Relation to remove */
{
	/* Unlink it from primary entity */
	if (rel->p_ent->p_first == rel)
		rel->p_ent->p_first = rel->p_next;
	if (rel->p_ent->p_last == rel)
		rel->p_ent->p_last = rel->p_prev;

	/* Unlink it from primary chain */
	if (rel->p_next != NULL)
		rel->p_next->p_prev = rel->p_prev;
	if (rel->p_prev != NULL)
		rel->p_prev->p_next = rel->p_next; 

	/* Unlink it from secondary entity */
	if (rel->s_ent->s_first == rel)
		rel->s_ent->s_first = rel->s_next;
	if (rel->s_ent->s_last == rel)
		rel->s_ent->s_last = rel->s_prev;

	/* Unlink it from secondary chain */
	if (rel->s_next != NULL)
		rel->s_next->s_prev = rel->s_prev;
	if (rel->s_prev != NULL)
		rel->s_prev->s_next = rel->s_next; 

	/* Free it */
	rel->p_next = free_rels;
	free_rels = rel;
}

/*
**	iiamxARaddRelation
**
** Add a relation into the relation/entity structure.
*/
VOID
iiamxARaddRelation(ent1, ent2, depend, p_same)
DM_ENT *ent1;		/* first entity description */
DM_ENT *ent2;		/* Second entity description */
DM_REL *depend;		/* Dependency descriptor */
bool p_same;		/* same primary entity  as last time */
{
	DM_ENT *primary;
	DM_ENT *secondary;
	DM_REL	*relation;

	if (p_same)
	{
		primary = saved_ent;
	}
	else
	{
		primary = iiamxGEgetEntity(ent1);
		saved_ent = primary;
	}
	secondary = iiamxGEgetEntity(ent2);

	/* Get the new relation */
	relation = get_relation();

	/* Link it to the primary entity's chain */
	relation->p_ent = primary;
	relation->p_prev = primary->p_last;
	relation->p_next = NULL;
	if (primary->p_first == NULL)
		primary->p_first = relation;
	if (primary->p_last != NULL)
		primary->p_last->p_next = relation;
	primary->p_last = relation;

	/* Link it to the secondary entity's chain */
	relation->s_ent = secondary;
	relation->s_prev = secondary->s_last;
	relation->s_next = NULL;
	if (secondary->s_first == NULL)
		secondary->s_first = relation;
	if (secondary->s_last != NULL)
		secondary->s_last->s_next = relation;
	secondary->s_last = relation;

	/* Fill in  the details*/
	STcopy(depend->linkname, relation->linkname);
	relation->deptype = depend->deptype;
	relation->deporder = depend->deporder;

	/* Determine relation class */
	relation->rel_class = RC_OTHER;	/* Assume this */	
	switch (relation->deptype)
	{
	    case OC_DTCALL:
		relation->rel_class = RC_CALL;
		break;

	    case OC_DTRCALL:
		relation->rel_class = RC_RCALL;
		break;

	    case OC_DTMREF:
		relation->rel_class = RC_MENU;
		break;

	    case OC_DTGLOB:
		relation->rel_class = RC_GLOB;
		break;

	    case OC_DTTABLE_TYPE:
		relation->rel_class = RC_TABTYPE;
		break;

	    case OC_DTFORM_TYPE:
		relation->rel_class = RC_FORMTYPE;
		break;

	    case OC_DTTYPE:
		switch (primary->class)
		{
		    case OC_GLOBAL:
			relation->rel_class = RC_GLOBREC;
			break;

		    case OC_RECORD:
			relation->rel_class = RC_RECREC;
			break;

		    case OC_APPLFRAME:
		    case OC_APPLPROC:
			relation->rel_class = RC_REC;
			break;
		}
		break;
	}
}

/*
** Find an entity; if it doesn't exist, make it
*/
DM_ENT *
iiamxGEgetEntity(ent_tmp)
DM_ENT *ent_tmp;	/* Data to look for/insert into entity */
{
	DM_ENT *entity;

	/* Search for it */
	if ((entity = IIAMfctFindCompTable(ent_tmp->name, ent_tmp->class))
	    != NULL)
		return entity;	/* Found it ! */

	/* Get a free one */
	if (free_ents == NULL)
	{
		DM_ENT	*group;
		register DM_ENT	*ptr1, *ptr2;

		/* We have to get more */
		group = (DM_ENT*)FEreqmem((u_i4)graphTag, 
						NUM_ENTS * sizeof(DM_ENT),
						FALSE, NULL);
		if (group == NULL)
			IIUGbmaBadMemoryAllocation( ERx("iiamxGEgetEntity"));
		for (ptr1 = group; ptr1 < group + NUM_ENTS - 1; ptr1 = ptr2)
		{
			ptr2 = ptr1 + 1;
			ptr1->p_first = (DM_REL *)ptr2;
		}
		ptr1->p_first = NULL;
		free_ents = group;
	}
	
	entity = free_ents;
	free_ents = (DM_ENT *)free_ents->p_first;

	/* Fill it in, and put it into the hash table */
	STcopy(ent_tmp->name, entity->name);
	entity->class = generic_class(ent_tmp->class);
	entity->comp = NULL;
	entity->p_first = entity->s_first = NULL;
	entity->p_last = entity->s_last = NULL;
	entity->visit_number = 0;

	_VOID_ IIUGheHtabEnter(DM_hash_id, (char *)entity, (PTR)entity);
	return entity;
}

/*
** get a relation structure off the free list.
*/
static DM_REL *
get_relation()
{
	DM_REL *relation;
	i4 i;

	/* Get a free one */
	if (free_rels == NULL)
	{
		DM_REL	*group;
		register DM_REL	*ptr1, *ptr2;

		/* We have to get more */
		group = (DM_REL*)FEreqmem((u_i4)graphTag, 
						NUM_RELS * sizeof(DM_REL),
						FALSE, NULL);
		if (group == NULL)
			IIUGbmaBadMemoryAllocation( ERx("get_relation"));
		for (ptr1 = group; ptr1 < group + NUM_RELS - 1; ptr1 = ptr2)
		{
			ptr2 = ptr1 + 1;
			ptr1->p_next = ptr2;
		}
		ptr1->p_next = NULL;
		free_rels = group;
	}
	
	relation = free_rels;
	free_rels = free_rels->p_next;
	return relation;
}

/*
**	iiamxDEdeleteEntity
**
**	Delete an entity from the DM graph.
*/
VOID
iiamxDEdeleteEntity(entity)
DM_ENT *entity;
{
	/* 
	** First, see what, if anything, its deletion triggers.  Currently,
	** if it's not a frame, procedure, or global, it can't trigger anything.
	*/
	switch (entity->class)
	{
	    case OC_APPLFRAME: 
	    case OC_APPLPROC: 
	    case OC_RECORD:
	    case OC_GLOBAL:

		if (entity->comp != NULL)
			iiAMxsgSearchGraph(entity, 
					   (DM_CHECK **)IIAMzccCompChange, 0, 
					   (PTR)NULL);
	}

	/* Next, delete the relations */
	delete_rels(entity, TRUE);

	/* 
	** Now try to delete the entity itself.  If it has any relations left, 
	** we'll leave it as a placeholder, i.e. with a NULL component pointer.
	** This won't currently happen, but it may be useful in the future.
	*/
	_VOID_ IIUGhdHtabDel(DM_hash_id, (char *)entity);
	if (entity->p_first != NULL || entity->s_first != NULL)
	{
		entity->comp = NULL;
	}
	else
	{
		/* Free it. */
		entity->p_first = (DM_REL *)free_ents;
		free_ents = entity;
	}

}

/*
**	Delete the graph for an entity.
*/
DM_ENT *
iiamxDGdeleteGraph(name, class, all)
char	*name;
OOID	class;
bool	all;	
{
	DM_ENT *entity;

	if ((entity = IIAMfctFindCompTable(name, class)) != NULL)
		delete_rels(entity, all);

	return entity;
}

/*
**	Delete all the relations for an entity.  We may invalidate frames
** 	as we go.
*/
static VOID
delete_rels(entity, all)
DM_ENT *entity;
bool	all;	/* If TRUE, delete relations where entity is secondary, also */
{
	DM_REL *rel;
	DM_REL *last_rel;

	/* First we'll chase the primary list */
	for (rel = entity->p_first; 
	     rel != NULL; 
	     rel = rel->p_next)
	{
		/* Remove it from secondary list */
		if (rel->s_prev != NULL)
			rel->s_prev->s_next = rel->s_next;
		if (rel->s_next != NULL)
			rel->s_next->s_prev = rel->s_prev;

		/* 
		** If it was the first or last entry in the secondary entity's 
		** list, fix the secondary entity's pointers.
		*/
		if (rel->s_ent->s_first == rel)
			rel->s_ent->s_first = rel->s_next;
		if (rel->s_ent->s_last == rel)
			rel->s_ent->s_last = rel->s_prev;
	}

	if (entity->p_first != NULL)
	{
		/* Free them en masse */
		entity->p_last->p_next = free_rels;
		free_rels = entity->p_first;
		entity->p_first = entity->p_last = NULL;
	}

	if (!all)
		return;

	/* Now we'll chase the secondary chain. */
	for (last_rel = NULL, rel = entity->s_first;
	     rel != NULL;
	     last_rel = rel, rel = rel->s_next)
	{
		/* Remove it from primary list */
		if (rel->p_prev != NULL)
			rel->p_prev->p_next = rel->p_next;
		if (rel->p_next != NULL)
			rel->p_next->p_prev = rel->p_prev;

		/* 
		** If it was the first or last entry in the primary entity's 
		** list, fix the primary entity's pointers.
		*/
		if (rel->p_ent->p_first == rel)
			rel->p_ent->p_first = rel->p_next;
		if (rel->p_ent->p_last == rel)
			rel->p_ent->p_last = rel->p_prev;

		if (last_rel != NULL)
			last_rel->p_next = rel;
	}

	if (last_rel != NULL) 
	{ 
		/* Free them en masse */ 
		last_rel->p_next = free_rels; 
		free_rels = entity->s_first; 
		entity->s_first = entity->s_last = NULL;    
	}
}

/*
**	Hash routines -- these are the routines which hash entites on both
**	name and generic class.
*/

static i4  
DM_hash(key ,size)
char *key;	/* Pointer to entity */
i4  size;	/* Size of hash table */
{
    	register i4 rem;
	DM_ENT *entity = (DM_ENT *)key;
	register char *s = entity->name;

	for (rem = entity->class % size; *s != EOS; s++)
	/* Change char *s to unsigned char *s for double-byte character (twai)
	** Bug no. 48204
	*/
		rem = (i4)(rem * 128 + *(unsigned char *)s) % size;
    	return (i4)rem;
}

static i4
DM_compare(e1, e2)
char *e1;	/* Entity 1 */
char *e2;	/* Entity 2 */
{
	DM_ENT *ent1 = (DM_ENT *)e1;
	DM_ENT *ent2 = (DM_ENT *)e2;

	/* We compare generic classes */
	if (generic_class(ent1->class) != generic_class(ent2->class))
		return -1;
	else 
		return (STcompare(ent1->name, ent2->name));
}

/*
** generic_class
** 
** Translate an object class to a generic class.  Basically, we confound all
** types of frame or procedure which involve code or a return value, and we
** also confound globals and constants.  In addition, we confound forms and
** formrefs, as who does not?
*/ 
static OOID
generic_class(class)
OOID    class;  /* Non-generic class */
{
        OOID genclass;

        switch (class)
        {
            case OC_OSLFRAME:
            case OC_OLDOSLFRAME:
            case OC_MUFRAME:
            case OC_APPFRAME:
            case OC_UPDFRAME:
                genclass = OC_APPLFRAME;
		break;
 
            case OC_HLPROC:
            case OC_OSLPROC:
            case OC_DBPROC:
                genclass = OC_APPLPROC;
		break;
 
            case OC_FORM:
            case OC_AFORMREF:
                genclass = OC_AFORMREF;
		break;
 
            case OC_GLOBAL:
            case OC_CONST:
                genclass = OC_GLOBAL;
		break;
 
            default:
                genclass = class;
		break;
        }
 
        return genclass;
}
