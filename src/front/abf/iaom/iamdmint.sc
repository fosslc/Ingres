/*
**	Copyright (c) 1990, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<oodep.h>
EXEC SQL INCLUDE 'iamdmtyp.sh';
EXEC SQL INCLUDE <metafrm.sh>;


/**
** Name:	iamdmint.qsc -- Dependency Manager interface routines
**
** Description:
**	This file defines:
**
**	iiamDMaAddDep		Add dependency to graph
**	iiamDMdDelDep		Delete dependencies from graph
**	IIAMxpdProcessDeps	Process the dependencies for a component
**	IIAMxrcRenameComp	Rename component
** 	IIAMactAddCompTable	Add component to table
**	IIAMdctDelCompTable	Delete component from table
**	IIAMxbtBuildTree	Built AFD
**	IIAMdmrReadDeps		Read dependencies from database
**
** History:
**	8/90 (Mike S) Initial version
**
**	02-dec-91 (leighb) DeskTop Porting Change:
**		Added routines to pass data across ABF/IAOM boundary.
**	18-jun-92 (davel)
**		changed variable name "restrict" to "restricted" in
**		IIAMdmrReadDeps(), as restrict is an ESQL reserved word now.
**	09-dec-1993 (daver)
**		Implemented owner.table support for "type of table x.y" for 
**		copyapp. Select abfdef_owner from ii_abfdependencies; pass it 
**		to do_deps() in iamcget.c. Its really passed via a callback 
**		function func(), but the only routine which calls this with 
**		a non-null callback function is IIAMcgCatGet(); hence
**		the only callback function to modify is do_deps().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN STATUS 	FEinqerr();
FUNC_EXTERN VOID 	iiamDMIinitDM();
FUNC_EXTERN DM_ENT * 	IIAMfctFindCompTable();
FUNC_EXTERN VOID 	iiamxARaddRelation();
FUNC_EXTERN VOID 	iiamxDEdeleteEntity();
FUNC_EXTERN DM_ENT * 	iiamxDGdeleteGraph();
FUNC_EXTERN VOID 	iiamxDRdeleteRelation();
FUNC_EXTERN DM_ENT * 	iiamxGEgetEntity();

GLOBALREF bool	iiamxGIgraphInit;	/* Is graph initialized */
GLOBALREF bool	IIab_Vision;		/* Is this Emerald ? */		 
/* static's */


/*{
** Name:        iiamDMaAddDep - Add dependency to graph
**
** Description:
**      Add a dependency to the graph
**
** Inputs:
**      comp            OO_OBJECT *     Depender component
**      dep_name        char *          dependee name
**      dep_class       OOID            dependee class
**      dep_type        OOID            dependency type
**      dep_link        char *          dependency link name
**      dep_order       i4              depemdemcy order
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
iiamDMaAddDep(comp, dep_name, dep_class, dep_type, dep_link, dep_order)
OO_OBJECT *comp;
char    *dep_name;
OOID    dep_class;
OOID    dep_type;
char    *dep_link;
i4      dep_order;
{
        DM_ENT p_ent;   /* Primary entity */
        DM_ENT s_ent;   /* Secondary enttiy */
        DM_REL rel;     /* Relation */
 
        if (!iiamxGIgraphInit)
                return;         /* No graph -- we're not in ABF */
 
        /* Fill in temporary entities and relation */
        STcopy(comp->name, p_ent.name);
        p_ent.class = comp->class;
 
        STcopy(dep_name, s_ent.name);
        s_ent.class = dep_class;
 
        rel.deptype = dep_type;
        rel.deporder = dep_order;
        STcopy(dep_link, rel.linkname);
 
        /* Add relation */
        iiamxARaddRelation(&p_ent, &s_ent, &rel, FALSE);
}
 
/*{
** Name:        iiamDMdDelDep - Delete dependencies from graph
**
** Description:
**      Delete the specified dependencies from the graph
**
** Inputs:
**      comp            {OO_OBJECT *}  The ABF application component object.
**      depty1,2,3      {OOID}  The dependency type to be deleted,
**                              all if depty1 = OC_UNDEFINED.
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
iiamDMdDelDep(comp, depty1, depty2, depty3 )
OO_OBJECT *comp;
OOID    depty1;
OOID    depty2;
OOID    depty3;
{
        DM_ENT *entity; /* Entity corresponding to component */
        DM_ENT ent_tmp; /* Temporary entity */
        DM_REL *rel;
        DM_REL *next_rel;
        bool all = (depty1 == OC_UNDEFINED);
 
        if (!iiamxGIgraphInit)
                return;         /* No graph -- we're not in ABF */
 
        if ((entity = IIAMfctFindCompTable(comp->name, comp->class)) == NULL)
                return;         /* Cannot find entity */
 
        /* Loop thorugh its relations, deleting any matches */
        for (rel = entity->p_first; rel != NULL; rel = next_rel)
        {
                next_rel = rel->p_next;
                if (all || rel->deptype == depty1 ||
                    rel->deptype == depty2 || rel->deptype == depty3)
                {
                        iiamxDRdeleteRelation(rel);
                }
        }

        return;
}

/*{
** Name:        IIAMxpdProcessDeps - Process the dependencies for a component
**
** Description:  
**      This processes the dependency list for a component; that is, it calls
**      a function for the relations for which the component is the primary
**      entity.  The dependencies are correctly ordered with respect to
**      deporder.  Only relations of class RC_OTHER or RC_MENU are searched.
**
** Inputs:
**       
**      comp            APPL_COMP *     The ABF component to search
**                                      dependencies for
**      depty1,2,3      OOID            Dependency types to match.
**      func            VOID (*)()      Function to call when a match occurs.
**      data            PTR             Data to pass to "func"
**
**      func is called as
**              func(comp, data, name, class, deptype, linkname, deporder)
**              APPL_COMP comp;         /* Component which has dependency
**              PTR     data;           /* Passed data
**              char    *name;          /* Dependender name 
**              OOID    class;          /* Dependender class
**              OOID    deptype;        /* Dependency type 
**              char *  linkname;       /* Dependency name
**              i4      deporder;       /* Dependency order
**       
**
** Outputs:
**      none
**
**      Returns:
**              none
**      Exceptions:
**
** Side Effects:
**
** History:
**      7/90 (Mike S) Initial version
*/
IIAMxpdProcessDeps(comp, depty1, depty2, depty3, func, data)
APPL_COMP *comp;
OOID depty1, depty2, depty3;
VOID (*func)();
PTR data;
{
        DM_ENT *entity;
        DM_REL *rel;
 
        if ((entity = IIAMfctFindCompTable(comp->name, comp->class)) == NULL)
                return;         /* No entity, ergo no relations */
 
        for (rel = entity->p_first; rel != NULL; rel = rel->p_next)
        {
                if (rel->rel_class == RC_OTHER || rel->rel_class == RC_MENU)
		{
                    if (rel->deptype == depty1 ||
                        rel->deptype == depty2 ||
                        rel->deptype == depty3
                    )
                        (*func)(comp, data, rel->s_ent->name, rel->s_ent->class,                                rel->deptype, rel->linkname, rel->deporder);
		}
        }
}

/*{
** Name:        IIAMxrcRenameComp - Rename component
**
** Description:
**      Called when a component is renamed.  We delete the old entity,
**      and create a new one with the dependencies from the database.
**	If we're called from ABF, the component might not have been
**	instantiated yet; we can't assume its APPL_COMP-specific fields
**	are valid.
**
** Inputs:
**      appl            APPL *     	Current application
**      comp            APPL_COMP *     Component which was renamed
**      oldname         char *          Former name
**
** Outputs:
**      none
**
**      Returns:
**              none
**
**      Exceptions:
**              none
**
** Side Effects:
**
** History:
**      8/90 (Mike S) Initial version
**      9/90 (Mike S) Change "comp" to OO_OBJECT *. bug 33525.
*/
VOID
IIAMxrcRenameComp(appl, comp, oldname)
APPL		*appl;
OO_OBJECT       *comp;
char            *oldname;
{
        DM_ENT *entity;
 
        /* Delete the existing entity */
        if ((entity = IIAMfctFindCompTable(oldname, comp->class)) != NULL)
	{
                iiamxDEdeleteEntity(entity);
        
		/* Create a new one, with its current dependencies */
		IIAMdmrReadDeps(appl->ooid, comp->ooid, TRUE, 
				(VOID (*)())NULL, appl);
	}
}

/*{
** Name:        IIAMactAddCompTable - Add component to table
** Name:        IIAMdctDelCompTable - Delete component from table
**
** Description:
**      Add/Delete a component to/from the hash table
**
** Inputs:
**      APPL_COMP *comp         component
**
** Outputs:
**      none
**
**      Returns:
**      status          OK if delete succeeded
**                      FAIL if delete failed
**
**      Exceptions:
**
** Side Effects:
**
** History:
**      7/19/90 (Mike S) Initial version
*/
VOID
IIAMactAddCompTable(comp)
APPL_COMP *comp;
{
        DM_ENT  tmp_ent;
	DM_ENT	*entity; 

        /* Put it into the table */
	if (comp->name != NULL && *comp->name != EOS)
	{
		STcopy(comp->name, tmp_ent.name);
		tmp_ent.class = comp->class;
		entity = iiamxGEgetEntity(&tmp_ent);
		entity->comp = comp;
	}
}
 
STATUS
IIAMdctDelCompTable(comp)
APPL_COMP *comp;
{
        DM_ENT *entity;
 
        /* Find it in the table */
	if (comp->name == NULL || *comp->name == EOS)
		return OK;

        if ((entity = IIAMfctFindCompTable(comp->name, comp->class)) == NULL)
                return FAIL;    /* It's not there */
 
        /* Delete it and its relations */
        iiamxDEdeleteEntity(entity);
	return OK;
}

/*{
** Name:	IIAMxbtBuildTree	- Build AFD
**
** Description:
**	We've already made the metaframes.  Now that we've read in the whole 
**	application, we can make the menu structures too.
**
** Inputs:
**	app		APPL*	Application object
**
** Outputs:
**	none
**
**	Returns:
**		none
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	8/90 (Mike S)	Initial version
*/
VOID
IIAMxbtBuildTree(app)
APPL	*app;
{
    	APPL_COMP 	*comp;	/* Parent component */
    	METAFRAME 	*mf;	/* Parent metaframe */
    	DM_ENT		*entity; /* Parent entity */
	DM_REL		*rel;	/* Parent-to-child relation */

    	/* Loop through all components */
    	for (comp = (APPL_COMP *)app->comps; comp != NULL; comp = comp->_next)
    	{
    		switch (comp->class)
    		{
    	  	    case OC_MUFRAME:
    	  	    case OC_APPFRAME:
    	  	    case OC_BRWFRAME:
    	  	    case OC_UPDFRAME:
    	    	    	if ((entity = 
				IIAMfctFindCompTable(comp->name, comp->class)) 
    	      		    == NULL)
    				break;	/* Can't find entity */

    	    		if ((mf = ((USER_FRAME *)comp)->mf) == NULL)
    				break;	/* No metaframe */

	    		/* Loop through relations, looking for menu refs */
	    		for (rel = entity->p_first;
			     rel != NULL; 
			     rel = rel->p_next)
	    		{
				register MFMENU *m;
				APPL_COMP *child;

				/* 
				** Skip if it isn't a menu ref, we're full, or 
				** there's no secondary entity.
				*/
				if (rel->deptype != OC_DTMREF || 
		    		    mf->nummenus >= MF_MAXMENUS || 
		    		    (child = rel->s_ent->comp) == NULL)
		    			continue;

				/* Create and fill in menu struct */
				m = (MFMENU *)FEreqmem(comp->data.tag, 
						       sizeof(MFMENU),
						       FALSE, 
						       (STATUS *)NULL);
				if (m == NULL)
			    		IIUGbmaBadMemoryAllocation(
					    ERx("IIAMxbtBuildTree()"));
				m->args = NULL;
				m->numargs = 0;
				m->apobj = (OO_OBJECT *)child;
				m->text = STtalloc(comp->data.tag, 
						   rel->linkname);
				(mf->menus)[mf->nummenus++] = m;
			}
	    	}
	}
}

/*{
** Name:        IIAMdmrReadDeps- Read dependencies from database
**
** Description:
**      Read the dependencies from the database and either enter them into the
**      Entity-relationship graph or call a caller-specified function to
**	process them. We read dependencies for the entire application if 
**	ooid == applid or ooid = OC_UNDEFINED (-1).  If we're calling the
**	function, we read all dependencies; none of the following discussion
**	applies.
**
**	We don't read dummy dependencies.  In addition, we exclude the 
**	following types of dependencies in the following situations: 
**	(the starrred situations do not occur)
**	
**				ABF			Emerald
**				---			-------
**	When "restricted" is TRUE:
**	A1. Application object:	all			all
**	A2. Other object:	OC_DTCALL, OC_DTMREF	OC_DTCALL, OC_DTMREF
**	A3. Whole application:	OC_DTCALL, OC_DTMREF,	OC_DTCALL
**
**	When "restricted" is FALSE:
**	B1. Application object:	OC_DTCALL, OC_DTMREF,	*all
**				OC_DTDBREF
**	B2. Other object:	none			none
**	B3. Whole application:	none			none
**
**	Explanations of the situations:
**	The OC_DTCALL dependencies are only used to determine the call graph
**	for a test build; this is aleays situation B3.  The OC_DTMREF
**	dependencies are used both to build the Emerald AFD and to determine
**	the call graph; these are situations A3(Emerald) and B3 respectively.
**	The OC_DTDBREF dependencies are needed only to create APPL_COMP
**	structures; this is not done in B1(ABF).
**
**	In A1, we're just reading the application object for the defaults
**	screen; no dependencies are needed.  In B1(ABF), we're begining to edit
**	an application; we need to build most of the entity-relation graph.
**	
**	In A2, we're creating an APPL_COMP structure; we don't need 
**	call dependencies.  In B2, we're specifically getting the 
**	call dependencies after a component has been recompiled.
**
** Inputs:
**      applid OOID             ID of Application
**      ooid OOID               ID of component to read. 
**      restricted bool         TRUE to restrict the dependencies read
**	func VOID (*)()		Call this routine for each dependency.
**				Otherwise, add dependency to graph.
**				This is currently non-NULL when we're called 
**				from IIAMcgCatGet, which is used by executables
**				which don't use the graph (copyapp, osl, oslsql)
**				It's always NULL for ABF/Emerald.
**	appl  APPL *		Application object.  May be NULL.
**
**	func is called as:
**	VOID func(oid, dep_name, dep_class, dep_owner, dep_type, dep_link, 
** 								dep_order)
**	OOID    oid;
**	char    *dep_name;
**	OOID    dep_class;
**	char	*dep_owner;
**	OOID    dep_type;
**	char    *dep_link;
**	i4     dep_order;
**
** Outputs:
**      none
**
**      Returns:
**              STATUS status
**
**      Exceptions:
**
** Side Effects:
**
** History:
**      9/20/90 (Mike S) Initial version
**	09-dec-1993 (daver)
**		Implemented owner.table support for "type of table x.y" for 
**		copyapp. Select abfdef_owner out of the database and pass 
**		it on to func; add the dep_owner parameter to the
**		do_deps() routine in iamcget.c. The only routine which 
**		calls this with a non-null func is IIAMcgCatGet(); hence
**		the only callback function is do_deps().
**	17-mar-94 (rudyw)
**		Fix for 59810. Extract the constant comparison from a repeated
**		select where clause. Use the constant comparison to set up a 
**		conditional clause with two separate selects which each can be
**		handled by the optimizer in a way that is best for performance.
*/
STATUS
IIAMdmrReadDeps(applid, ooid, restricted, func, appl)
EXEC SQL BEGIN DECLARE SECTION;
OOID    applid;
OOID    ooid;
bool    restricted;
EXEC SQL END DECLARE SECTION;
VOID	(*func)();
APPL	*appl;
{
        EXEC SQL BEGIN DECLARE SECTION;
        DM_ENT ent1, ent2;
        DM_REL relation;
	OOID exclude1, exclude2, exclude3;
        OOID objid;     /* id of current row */
        OOID lastid;    /* id of previous row */
	i2   linkname_ind, deporder_ind;
			/* Null indicators */

	char owner_name[FE_MAXNAME + 1];	/* holds abfdef_owner */

        EXEC SQL END DECLARE SECTION;
	i4	dummy;

	exclude1 = exclude2 = exclude3 = OC_UNDEFINED;
						/* Assume we exclude nothing */
	if (ooid == applid)
	{
		if (func != NULL)
		{
			/* 
			** We're called from IIAMcgCatGet and reading only the 
			** application object; there are no dependencies.
			*/
			return OK;
		}
		else if (!IIab_Vision && !restricted)		 
		{
			exclude1 = OC_DTCALL;
			exclude2 = OC_DTMREF;
			exclude3 = OC_DTDBREF;
		}
		else
			return OK;      /* Nothing to do */
	}
	else if (restricted)
	{
		exclude1 = OC_DTCALL;

		if (ooid != OC_UNDEFINED)
			exclude2 = OC_DTMREF;
		else if (!IIab_Vision)				 
			exclude2 = OC_DTMREF;
	}

	if (ooid == applid)
		ooid = OC_UNDEFINED;	/* Whole application */
        if (OC_UNDEFINED == ooid)
        {
                /*
                ** If we're reading the whole application,
                ** we can init (or re-init) the graph.  We'll also check here
		** for 6.3 frontends having saved dependency records with NULL
		** application ID's.
                */
		if (func == NULL)
			iiamDMIinitDM(applid, appl);
		IIUIadaAddDepAppl(applid, TRUE, FALSE, &dummy);
        }
 
        /* Get the dependencies */


	/* Fix for 59810.  Required pulling constant comparison out of the
	** where clause and creating a conditional with separate queries.
	** Must make any modification to query or select loop in both.
	*/
        lastid = -1;
	if ( ooid == -1 )
	{
            EXEC SQL REPEATED SELECT
                o.object_id as objid,
                o.object_name as objname, o.object_class as objclass,
                a.abfdef_name, a.object_class,
		a.abfdef_owner,
                a.abfdef_deptype as deptype, 
		a.abf_linkname,
                a.abf_deporder as depord
                INTO
                :objid,
                :ent1.name, :ent1.class,
                :ent2.name, :ent2.class,
		:owner_name,
                :relation.deptype, :relation.linkname:linkname_ind,
                :relation.deporder:deporder_ind 
                FROM ii_objects o, ii_abfdependencies a
                WHERE o.object_id = a.object_id
                        AND a.abfdef_applid = :applid
                        AND a.abfdef_deptype <> 0
                        AND a.abfdef_deptype <> :exclude1
                        AND a.abfdef_deptype <> :exclude2
                        AND a.abfdef_deptype <> :exclude3
                ORDER BY objname, objclass, deptype, depord;
            EXEC SQL BEGIN;
            {
		if (linkname_ind == DB_EMB_NULL)
			*relation.linkname = EOS;
		if (deporder_ind == DB_EMB_NULL)
			relation.deporder = 0;
		if (func != NULL)
		{
			/*
			** changed to pass owner_name from abfdef_owner as 
			** well. required for table dependencies, since we now
			** need to know the owner of the table as well as
			** its name. only IIAMcgCatGet() passes in a func,
			** other callers use "the graph". copyapp fetches
			** the whole application.
			*/
			(*func)(objid, ent2.name, ent2.class, owner_name,
				relation.deptype, relation.linkname, 
				relation.deporder);
		}
		else
		{
			/*
			** If we're not fetching the whole application, delete 
			** the graph for the object the first time through.
			** Only delete the dependencies for which the object
			** is the depender.
			*/
			if (OC_UNDEFINED != ooid && -1 == lastid)
				iiamxDGdeleteGraph(ent1.name, ent1.class, 
						   FALSE);
 
			/* Add relation to graph */
			iiamxARaddRelation(&ent1, &ent2, &relation,
				(bool)(objid == lastid));
			lastid = objid;
		}
            }
            EXEC SQL END;
	}
	else
	{
            EXEC SQL REPEATED SELECT
                o.object_id as objid,
                o.object_name as objname, o.object_class as objclass,
                a.abfdef_name, a.object_class,
		a.abfdef_owner,
                a.abfdef_deptype as deptype, 
		a.abf_linkname,
                a.abf_deporder as depord
                INTO
                :objid,
                :ent1.name, :ent1.class,
                :ent2.name, :ent2.class,
		:owner_name,
                :relation.deptype, :relation.linkname:linkname_ind,
                :relation.deporder:deporder_ind 
                FROM ii_objects o, ii_abfdependencies a
                WHERE o.object_id = a.object_id
                        AND a.abfdef_applid = :applid
                        AND o.object_id = :ooid
                        AND a.abfdef_deptype <> 0
                        AND a.abfdef_deptype <> :exclude1
                        AND a.abfdef_deptype <> :exclude2
                        AND a.abfdef_deptype <> :exclude3
                ORDER BY objname, objclass, deptype, depord;
            EXEC SQL BEGIN;
            {
		if (linkname_ind == DB_EMB_NULL)
			*relation.linkname = EOS;
		if (deporder_ind == DB_EMB_NULL)
			relation.deporder = 0;
		if (func != NULL)
		{
			/*
			** changed to pass owner_name from abfdef_owner as 
			** well. required for table dependencies, since we now
			** need to know the owner of the table as well as
			** its name. only IIAMcgCatGet() passes in a func,
			** other callers use "the graph". copyapp fetches
			** the whole application.
			*/
			(*func)(objid, ent2.name, ent2.class, owner_name,
				relation.deptype, relation.linkname, 
				relation.deporder);
		}
		else
		{
			/*
			** If we're not fetching the whole application, delete 
			** the graph for the object the first time through.
			** Only delete the dependencies for which the object
			** is the depender.
			*/
			if (OC_UNDEFINED != ooid && -1 == lastid)
				iiamxDGdeleteGraph(ent1.name, ent1.class, 
						   FALSE);
 
			/* Add relation to graph */
			iiamxARaddRelation(&ent1, &ent2, &relation,
				(bool)(objid == lastid));
			lastid = objid;
		}
            }
            EXEC SQL END;
	}
 
        return FEinqerr();
}
