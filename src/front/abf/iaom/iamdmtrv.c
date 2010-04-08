/*
**	Copyright (c) 1990, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ug.h>
# include	<ooclass.h>
# include	"iamdmtyp.h"
# include	"eram.h"

/**
** Name:	iamdmtrv.c - Traverse the DM graph
**
** Description:
**	These are the routines used to traverse the graph.  Given a starting
**	component (or entity) and a traversal table, the graph is traversed,
**	recursively if the table so specifies, until the end is reached, or one
**	of the traversal routines returns -1 (see iamtrvrtn.sc for further
**	details.)
**
**	We avoid infinite loops during a recursive traversal by setting a 
**	"visit number" in each entity.  If this visit number matches the number
**	of the current traversal, we won't visit the entity again. We don't
**	support multiple active recursive traversals.
**
**	This file defines:
**
**	IIAMxdsDepSearch 	- Search Dependency Graph
**	iiAMxsgSearchGraph 	- Search Dependency graph
**
** History:
**	8/90 (Mike S) Initial version
**	31-dec-90 (blaise)
**	    The visit number, given to each entity during a recursive
**	    traversal to avoid infinite loops, is only valid for recursive
**	    traversals. Stopped this number from being checked during a
**	    non-recursive traversal. Also when a recursive traversal
**	    produces an error, reset the boolean recursion_active to FALSE,
**	    since we've finished the traversal (Bug #34813).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
GLOBALREF	OOID	iiamxAIapplId;

FUNC_EXTERN	DM_ENT 	*IIAMfctFindCompTable();
FUNC_EXTERN	STATUS	iiAMxsgSearchGraph();

/* static's */
static STATUS	do_check();

static bool	recursion_active = FALSE;
static i4  tn_seq = 1;	/* Generate unique traversal numbers */

/*{
** Name:        IIAMxdsDepSearch - Search Dependency Graph
**
** Description:
**      This is the external entry point, whcih accept a component.
**      It's a cover routine for iiAMxsgSearchGraph.
**
** Inputs:
**      comp    APPL_COMP *     Component whose dependencies to search
**      table   PTR             Table to drive the search
**      arg     PTR             Argument to pass to traversal routines
**
** Outputs:
**      none
**
**      Returns:
**              none
**
**      Exceptions:
**
** Side Effects:
**
** History:
**      7/90 (Mike S) Initial version
*/
VOID
IIAMxdsDepSearch(comp, table, arg)
APPL_COMP       *comp;
PTR             table;
PTR             arg;
{
        DM_ENT *entity = IIAMfctFindCompTable(comp->name, comp->class);
 

        if (entity != NULL)
                _VOID_ iiAMxsgSearchGraph(entity, (DM_CHECK **)table,
                                          0, arg);
}
         
/*{
** Name:        iiAMxsgSearchGraph - Search Dependency graph
**
** Description:
**      Search an entity's dependency graph looking for things to do.
**      This is the internal entry point; it speciifes an entity.
**
** Inputs:
**      entity  DM_ENT *        Component whose dependencies to search
**      table   DM_CHECK **      Table to drive the search
**      level   i4              Recursion level of this call
**      arg     PTR             Argument to pass to traversal routines
**
** Outputs:
**      None
**
**      Returns:
**              STATUS          OK      Normally
**                              -1      No further traversal should be done
**
**      Exceptions:
**
** Side Effects:
**
** History:
**      7/90 (Mike S) Initial version
*/
STATUS
iiAMxsgSearchGraph(entity, table, level, arg)
DM_ENT          *entity;
DM_CHECK        **table;
i4              level;
PTR             arg;
{
        bool check_prime = FALSE;       /* Check dependencies for which entity
                                           is primary */
        bool check_second = FALSE;      /* Check dependencies for which entity
                                           is secondary */
	bool recurse = FALSE;		/* Are there recursive checks */
        STATUS status = OK;
	DM_REL	*rel;
        int i;
 
        /*
        ** See what sorts of checking there are to do
        */
        for (i = 0; i < NO_RC_TYPES; i++)
        {
                if (table[i] != NULL)
                {
                        if (table[i]->direction == DM_P_TRAVERSE)
                                check_prime = TRUE;
                        else 
                                check_second = TRUE;
			if (table[i]->recursive)
				recurse = TRUE;
                }
        }
 
        /*
        ** If this is a recursive search, mark the entity as visited.  If
	** it's level 0, get a new traversal number. Also check for multiply
	** active recursive searches.
        */
	if (recurse)
	{
		if (level == 0)
		{
			if (recursion_active)
				IIUGerr(E_AM0032_MultRecurseTrav, 
					UG_ERR_FATAL, 0);
			recursion_active = TRUE;
			tn_seq++;
		}
        	entity->visit_number = tn_seq;
	}
 
        /* Traverse primary->secondary */
        if (check_prime)
        {
                for (rel = entity->p_first; rel != NULL; rel = rel->p_next)
                {
                        status = do_check(table, rel, DM_P_TRAVERSE, 
					  level, arg, tn_seq);
			if (status == -1)
			{
				if (recursion_active)
					recursion_active = FALSE;
                                return status;
			}
                }
        }
 
        /* Traverse secondary->primary */
        if (check_second)
        {
                for (rel = entity->s_first; rel != NULL; rel = rel->s_next)
                {
                        status = do_check(table, rel, DM_S_TRAVERSE, 
					  level, arg, tn_seq);
			if (status == -1)
			{
				if (recursion_active)
					recursion_active = FALSE;
                                return status;
			}
                }
        }
 
	if (recurse && level == 0)
		recursion_active = FALSE;
        return status;
}
 
/*
** Do a traversal
*/
static STATUS
do_check(table, rel, direction, level, arg, tn)
DM_CHECK        **table;        /* Control table for traversal */
DM_REL          *rel;           /* Relation to traverse */
i4              direction;      /* Direction to traverse */
i4              level;          /* Recursion level */
PTR             arg;            /* Argument to pass to traversal routine */
i4		tn;		/* Traversal number */
{
        i4  class = rel->rel_class;	/* Relation class */
        DM_CHECK *check;                /* Traveral structure */
        DM_ENT *target = 		/* Target entity */
		 (direction == DM_P_TRAVERSE) ? rel->s_ent : rel->p_ent;
        STATUS status = OK;
 

	/*
	** Don't traverse if:
	**	1.	This isn't a relation class which gets traversed, or
	**	2.	There's no check for this traversal, or
	**	3.	The check is for a traversal the other way, or
	**	4.	This is a recursive search and we've already been to
	**		the target.
	*/
 
        if (class < 0				||
                (check = table[class]) == NULL 	||
                check->direction != direction 	||
	        (check->recursive && target->visit_number == tn)
	)
		return OK;

	/* Call the traversal function (if there is one) */
	if (check->check_rtn != NULL)
	{
		status = (*check->check_rtn)
                                        (iiamxAIapplId, rel, check->arg, arg);
		/*
		** -1 is a special value; it means that we've done
		** what we needed to do; no more traversal is needed.
		*/
		if (status == -1)
			return status;
	}
 
	/*
	** If need be, do a recursive check.  -2 is a special value;
	** it means that we should continue traversing, but not recurse
	** here.  If the target already has the current traversal
	** number, we've already been there.
	*/
	if (check->recursive && status != -2)
	{
		status = iiAMxsgSearchGraph(target, table, level+1, arg);
        }
        return status;
}
