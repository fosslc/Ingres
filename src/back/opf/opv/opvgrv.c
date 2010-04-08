/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <scf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#define        OPV_RGRV      TRUE
#include    <me.h>
#include    <bt.h>
#include    <opxlint.h>

/**
**
**  Name: OPVGRV.C - allocate new global range variable for implicit table
**
**  Description:
**      Routine to allocate a new global range variable element
**
**  History:    
**      1-jul-86 (seputis)    
**          initial creation
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
[@history_line@]...
**/

/*{
** Name: opv_agrv	- allocate new global range variable
**
** Description:
**	Find a free slot in the global range table for an 
**      aggregate function, or implicitly referenced index.  
**      If there are no free slots for the aggregate function, then the 
**      optimization is aborted and an error reported.  There will be
**      one global range table per optimization and there will be no
**      overlapping of range table assignments i.e. it is conceivable
**      the two temporary relations could use the same range table
**      entry since they do not exist at the same... this will not be
**      done.
**
** Inputs:
**      global				ptr to global state variable
**      name                            ptr to table name
**                                      NULL- indicates a temporary table
**      owner                           ptr to owner name
**      abort                           TRUE if optimization should be aborted
**                                      in case of error
**
** Outputs:
**	Returns:
**	    - index into global range table representing the allocated
**          variable
**	Exceptions:
**	    Will generate an internal exception if the global range table
**          is full.  This will abort the query and report an error.
**
** Side Effects:
**	    none
**
** History:
**	7-apr-86 (seputis)
**          initial creation
**	11-apr-91 (seputis)
**	    ask for RDF info if name, or table ID is given so
**	    that explicit secondary index substitution can get
**	    histograms
**	18-sep-92 (ed)
**	    bug 44850 - added parameter to allow multi-to-one mapping
**	    so a common aggregate temp can be used
**	17-Jan-2004 (schka24)
**	    Rename RDR_BLD_KEY to RDR_BLD_PHYS, gives us partition info too.
[@history_line@]...
*/
OPV_IGVARS
opv_agrv(
	OPS_STATE          *global,
	DB_TAB_NAME        *name,
	DB_OWN_NAME        *owner,
	DB_TAB_ID          *table_id,
	OPS_SQTYPE         sqtype,
	bool               abort,
	OPV_GBMVARS        (*gbmap),
	OPV_IGVARS	   gvarno)
{
    OPV_IGVARS		grv_index; /* index into global range table */
    OPV_IGVARS		empty_index; /* index into global range table of
				    ** free element */
    OPV_GRT             *gbase;    /* ptr to base of array of ptrs to global
				   ** range table elements */
    bool		lookup;    /* look for existing definition if
                                   ** names are available */

    lookup = name && owner;	   /* TRUE - if RDF table ID given */
    empty_index = OPV_NOGVAR;
    gbase = global->ops_rangetab.opv_base;
    for ( grv_index = 0; grv_index < OPV_MAXVAR; grv_index++)
    {
	OPV_GRV		*existing_var;
	if (!(existing_var = gbase->opv_grv[grv_index]))
	{
	    if (empty_index == OPV_NOGVAR)
	    {
		empty_index = grv_index;
		if (!lookup)
		    break;	    /* empty slot found and we do not need
				    ** to continue searching for an existing
                                    ** table entry of the same name */
	    }
	}
	else
	{
	    if (lookup
		&&
		existing_var->opv_relation
		&&
		existing_var->opv_relation->rdr_rel
		&&
		(   existing_var->opv_relation->rdr_rel->tbl_name.db_tab_name[0]
		    ==
		    name->db_tab_name[0]
		)
		&&
		(  existing_var->opv_relation->rdr_rel->tbl_owner.db_own_name[0]
		    ==
		    owner->db_own_name[0]
		)
		&&
		!MEcmp((PTR)&existing_var->opv_relation->rdr_rel->tbl_name,
		    (PTR)name, sizeof(*name))
		&&
		!MEcmp((PTR)&existing_var->opv_relation->rdr_rel->tbl_owner,
		    (PTR)owner, sizeof(*owner))
		&&
		(   !gbmap
		    ||
		    !BTtest((i4)grv_index, (char *)gbmap)  /* do not use the
						** same global range variable
                                                ** in the same subquery twice
                                                ** or OPC will complain */
		)
	       )
	    {	/* a match has been found */
		if (gbmap)
		    BTset((i4)grv_index, (char *)gbmap); /* set the global
						** bit map so that this
                                                ** range variable is not
                                                ** reused in the same subquery
                                                */
		return(grv_index);
	    }
	}
    }

    if (empty_index != OPV_NOGVAR)
    {
	if (gvarno == OPV_NOGVAR)
	{
	    /* empty slot found - allocate and initialize slot and return     */
	    OPV_GRV       *grv;     /* pointer to global range table element  */
	    RDF_CB	      *rdfcb;

	    rdfcb = &global->ops_rangetab.opv_rdfcb;
	    if (name || table_id)
	    {   /* if name is available then table ID might be available */
		if (table_id)
		{	/* use table ID if available */
		    STRUCT_ASSIGN_MACRO((*table_id), rdfcb->rdf_rb.rdr_tabid); /*
					** need table name */
		    rdfcb->rdf_rb.rdr_types_mask = RDR_RELATION | 
			RDR_ATTRIBUTES | RDR_BLD_PHYS; /*get relation info 
					    ** - The optimizer uses attribute
					    ** info in query tree directly 
					    ** but it is needed to be requested
					    ** since the RDF uses attr info to
					    ** build RDR_BLK_PHYS info.  The
					    ** attribute info does not need to
					    ** be requested if RDF is changed.*/
		}
		else
		{	/* table ID not available so use name */
		    MEfill( (i4)sizeof(DB_TAB_ID), (u_char)0, 
			(PTR)&rdfcb->rdf_rb.rdr_tabid);
		    rdfcb->rdf_rb.rdr_types_mask = RDR_RELATION | 
			RDR_ATTRIBUTES | RDR_BLD_PHYS | RDR_BY_NAME;
		    STRUCT_ASSIGN_MACRO((*name), rdfcb->rdf_rb.rdr_name.rdr_tabname);/* need
					    ** table name */
		    STRUCT_ASSIGN_MACRO((*owner), rdfcb->rdf_rb.rdr_owner);/* 
					    ** need table owner */
		}
	    }
	    else
		rdfcb->rdf_info_blk = NULL; /* get new ptr to info
					    ** associated with global var */

	    /* allocate and initialize global range table element */
	    if (opv_parser(global, empty_index, sqtype, 
		(name != NULL) || (table_id != NULL), /* TRUE - if rdf info needs to be retrieved */ 
		FALSE,		/* TRUE - if this is a parser range table element */
		abort)		/* TRUE - if error occurs then otherwise FALSE
				    ** means return varinit == TRUE */
		)
		return(OPV_NOGVAR);	/* ignore variable if error occurs */

	    grv = gbase->opv_grv[empty_index]; /* get ptr to element */
	    grv->opv_qrt = OPV_NOGVAR; /* indicates that this table was not
				    ** explicitly referenced in the query */
	    grv->opv_relation = rdfcb->rdf_info_blk; /* save ptr to RDF info */
	}
	else
	{
	    gbase->opv_grv[empty_index] = gbase->opv_grv[gvarno]; /* used
					** for aggregate temporaries which
					** require "2 cursors" */
	}
	if (gbmap)
	    BTset((i4)empty_index, (char *)gbmap); /* set the global
				    ** bit map so that this
				    ** range variable is not
				    ** reused in the same subquery
				    */
    }
    else if (abort)
	/* the entire table is full so report and error */
	opx_error(E_OP0005_GRANGETABLE);
    return (empty_index);	    /* return with no range table entry */
}
