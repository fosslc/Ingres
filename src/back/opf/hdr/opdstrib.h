/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPDSTRIB.H - distributed query optimization structures 
**
** Description:
**      This file contains structure definitions used during
**      distributed query optimization
**
** History:
**      17-mar-88 (seputis)
**          initial creation
**      26-mar-91 (fpang)
**          Renamed opdistributed.h to opdstrib.h
[@history_template@]...
**/

#define                 OPD_MAXSITE     64
/* Maximum number of sites which can be considered in a distributed
** query 
*/
#define                 OPD_TSITE	0
/* This is the destination site for the query
*/
#define                 OPD_CSITE	1
/* This is the coordinator site
*/
#define                 OPD_CREATE_COST     ((OPO_COST)16.)
/* Fixed Cost to create/delete a temporary relation used by
** distributed database
*/
#define                 OPD_FMIN        0.001
/* This is the minimum delta used to determine equality for site cost
** comparisons
*/
#define                 OPD_PRECISION	1000000.0
/* For large costs OPD_FMIN may be insignificant, so this value is used
** as a scale factor to avoid roundoff
*/

/*}
** Name: OPD_SCOST - Site cost information for an intermediate
**
** Description:
**      This structure contains the site cost information summary for 
**      CO node.  It is only used for distributed query optimization.
**      This will typically be a variable length structure, with one
**      element in the array per site being considered for the optimization.
**      The n'th element of the array represents the cost of having
**      the result of the CO node on the n'th site.
**
** History:
**      17-mar-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPD_SCOST
{
    OPO_COST        opd_intcosts[1];    /* costs of having intermediate
                                        ** results on each site */
}   OPD_SCOST;

/*}
** Name: OPD_BESTCO - Site query plans for an intermediate
**
** Description:
**      This structure contains the site query plans for 
**      subquery.  It is only used for distributed query optimization.
**      This will typically be a variable length structure, with one
**      element in the array per site being considered for the optimization.
**      The n'th element of the array represents the plan for having
**      the result of the CO node on the n'th site.
**
** History:
**      17-mar-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPD_BESTCO
{
    struct _OPO_CO  *opd_colist[1];    /* costs of having intermediate
                                        ** results on each site */
}   OPD_BESTCO;

/*}
** Name: OPD_ISITE - index into the site descriptor table
**
** Description:
**      This type will contain the site ID for optimization purposes.
**      It represents an index into the site descriptor table.
**
** History:
**      17-mar-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef i2 OPD_ISITE;
#define                 OPD_NOSITE      ((OPD_ISITE)-1)
/* used when a site has not been defined */

/*}
** Name: OPD_QCOMP - query compilation summary for CO node
**
** Description:
**      The query compilation phase should not need to deal with multiple
**      site cost arrays.  This structure summarizes the information
**      which query compilation will receive, and should be sufficient
**      to easily make determination about inter-site movement at this
**      CO node
[@comment_line@]...
**
** History:
**      17-mar-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPD_QCOMP
{
    OPD_ISITE       opo_jsite;          /* if this is a join node then this
                                        ** is the site upon which to
                                        ** join the result, OPD_NOSITE
                                        ** if this is not a join node */
    OPD_ISITE       opo_jresult;        /* this is the site upon which the
                                        ** result of this intermediate
                                        ** node should be placed */
}   OPD_QCOMP;

/*}
** Name: OPD_CAPABILITY - capabilities of the site
**
** Description:
**      This type describes the capabilities which can be performed upon a
**      site
**
** History:
**      7-aug-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef i4	OPD_CAPABILITY;
#define                 OPD_CTARGET     1
/* The site can only read tuples from other sites, but cannot perform
** any joins, create temporaries etc.  It is meant for the distributed
** server site upon which no underlying file system exists, but can
** read tuples from other sites and return the results to the user 
*/
#define			OPD_NODTID	2
/* This bit is set if the site does not support TID references for
** duplicate handling
*/
#define			OPD_NOUTID	4
/* This bit is set if the site does not support TID references for
** non-read queries, i.e. update, and delete
*/
#define                 OPD_MCLUSTER    8
/* set if this site is on the same clustered ingres installation as the 
** current distributed server */

/*}
** Name: OPD_SITE - site descriptor
**
** Description:
**      This summarizes the information about a site which the optimizer
**      needs.  There is one descriptor, per site, per database, i.e. if there
**	are multiple databases on a site then there will be one entry for each
**	data base.
[@comment_line@]...
**
** History:
**      17-mar-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
typedef struct _OPD_SITE
{
    OPD_SCOST	    *opd_factor;        /* ptr to an array of factors which
					** define the cost of moving a byte from
                                        ** from this site to other sites, i.e.
					** there is one entry in this array
					** for each site */
    DD_LDB_DESC	    *opd_ldbdesc;       /* ptr to LDB descriptor */
    bool            opd_local;          /* TRUE if this is a local site
                                        ** i.e. direct keying can be done */
    DD_0LDB_PLUS    *opd_dbcap;		/* ptr to RDF's DBA and capability 
					** descriptor for this site */
    OPD_CAPABILITY  opd_capability;     /* set of masks describing the capability
					** of the site */
    DD_COSTS	    *opd_nodecosts;	/* ptr to tuple which provides defaults
					** to use for LDB/machine relative
					** power */
    OPO_COST	    opd_tosite;		/* cost per byte of moving data from
					** the star server site to this 
					** remote site */
    OPO_COST	    opd_fromsite;	/* cost per byte of moving data from
					** the remote site to the star server 
					** site */
}   OPD_SITE;

/*}
** Name: OPD_DT - site descriptor array
**
** Description:
**      This array describes the sites which will be considered in this
**      optimization
[@comment_line@]...
**
** History:
**      17-mar-88 (seputis)
**          initial creation
[@history_line@]...
[@history_template@]...
*/
typedef struct _OPD_DT
{
    OPD_SITE        *opd_dtable[OPD_MAXSITE]; /* array of ptrs to site
                                          ** descriptors */
}   OPD_DT;

/*}
** Name: OPD_ITEMP - temporary table identifier
**
** Description:
**      This type is used to describe/identify a distributed temporary table
[@comment_line@]...
**
** History:
**      15-aug-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef i4  OPD_ITEMP;
#define                 OPD_NOTEMP      -1
/* used when no temporary table has be defined */

/*}
** Name: OPD_SUBDIST - distributed optimization structure for a subquery
**
** Description:
**      This structure summarizes the distributed site descriptions
**      used in this subquery.
**
** History:
**      17-mar-88 (seputis)
**          initial creation
**	26-dec-90 (seputis)
**	    add info for distributed OPC
[@history_template@]...
*/
typedef struct _OPD_SUBDIST
{
    OPD_ISITE       opd_local;		/* site ID of local site or OPD_NOSITE
                                        ** if it is not involved in the query */
    OPD_SCOST       *opd_cost;          /* ptr to an array of best costs for
                                        ** each site */
    OPD_BESTCO      *opd_bestco;        /* an array of ptrs to best CO trees
                                        ** - one for each of the sites
                                        ** considered in the subquery
                                        ** - not used for the main query since
                                        ** only the best cost for the result
					** site is needed */
    OPD_ISITE	    opd_target;		/* target site for the query, for a
                                        ** function aggregate temporary this
                                        ** will start out as OPD_NOSITE and the
                                        ** first parent which gets optimized will
                                        ** set this, then all subsequent parents
                                        ** will need to use this copy */
    OPV_IVARS	    opd_updtsj;		/* this variable will not be OPV_NOVAR
					** only in the special situation of a
					** gateway table being updated by a
					** multi-variable query, in which
					** a semi-join may be useful */
    OPS_PNUM	    opd_dv_cnt;		/* number of parameters to the sent to
					** the LDB in order to process this query
					** - if this subquery contains a ~Q then
					** OPC should use the ~Q<no> form of the
					** subquery, and indicate that the ~Q 
					** parameters are to be found at the end
					** of the query
					*/
    OPS_PNUM	    opd_sagg_cnt;	/* number of simple aggregate operators
					** which will be computed for this query
					*/
    OPS_PNUM	    opd_dv_base;	/* parameter number at which the first
					** simple aggregate result will be placed
					*/
    i4		    opd_smask;		/* mask of boolean values to describe
					** variables characteristics of this subquery
					*/
#define                 OPD_DUPS        1
/* TRUE if duplicates need to be removed for this subquery by creating a temporary
** by using a SELECT DISTINCT to remove duplicates
** and selecting only the printing resdoms in the target list from this temporary */
#define                 OPD_FABOOLFACT  2
/* TRUE - some of the boolean factors in this node need to be handled specially
** due to SQL syntactic restrictions, e.g. the left hand operand of a LIKE
** must be a column name, but querymod may make this an expression after
** view substition */
    OPD_ITEMP	    opd_create;		/* used by OPC to deal with internal
					** create as select processing */
    i4		    opd_expansion;	/* for expansion */
}   OPD_SUBDIST;

/*}
** Name: OPD_DOPT - global distributed optimization structure for a query
**
** Description:
**      This structure summarizes the distributed site descriptions
**      used in this query.  This structure is global to all subqueries
**      which means all sites will be considered for all subqueries.
**
** History:
**      17-mar-88 (seputis)
**          initial creation
**	26-dec-90 (seputis)
**	    add info for distributed OPC
**      21-oct-2004 (rigka01, crossed from unsubm 2.5 IP by hayke02)
**        Added opd_gmask flag OPD_NOFROM to indicate that we have an update
**        query with no from clause, and that temporary table names should
**        not be used in the target list. This change fixes bug 108409.
[@history_template@]...
*/
typedef struct _OPD_DOPT
{
    OPD_DT          *opd_base;          /* base of distributed site info */
    OPD_ISITE       opd_dv;             /* number of distributed sites defined 
                                        ** in opd_base */
    OPD_SCOST       *opd_tcost;         /* temporary used for garbage collection
                                        ** of CO nodes when a new CO subtree is
                                        ** being added to the saved work area */
    OPD_BESTCO      *opd_tbestco;       /* temporary used for garbage collection
                                        */
    bool	    *opd_copied;        /* array of booleans, TRUE if the
                                        ** query plan associated with a site
                                        ** has been copied out of enumeration
                                        ** memory */
    bool	    opd_scopied;        /* TRUE - if distributed query plan has been
                                        ** copied out of memory, used for main query
                                        ** or simple aggregates which do not have an
				        ** array of plans associated with them */
    OPD_ISITE       opd_coordinator;	/* this is the site of the coordinator DB
					** which may or may not be the same as the
					** target ldb for the query, note that this
					** LDB may be on the same site but this does
					** not  necessarily mean the same entry in the
					** site array if a different LDB is referenced */
    OPZ_BMATTS	    *opd_repeat;        /* ptr to bit map of repeat query parameters
					** which represent the attributes in the
					** query which are correlated, NULL
					** if no correlated parameters are defined */
    OPV_BMVARS	    *opd_byposition;	/* bitmap of the target relation to be
					** updated and all of the indexes defined
					** on it */
    i4		    opd_gmask;		/* set of booleans which are precalculated
					** to describe various conditions for
					** this distributed optimization */
#define                 OPD_GUPDATE     1
/* TRUE - if the main query is an update query in which no TIDs are available
** for any semi-join operation with the target relation, this will cause no
** site movement for any intermediate which contains the target relation,
** and will cause a new range variable to be defined on the target relation
** to take care of that part of the search space which can benifit from
** semijoins on the target relation */
#define                 OPD_TOUSER      2
/* TRUE - if the results are to be passed back to the user, i.e. a retrieve
** or a read only cursor, without any updates, thus there are no site
** movement constraints which require the target relation to be on the
** final site */
#define                 OPD_DISTINCT	4
/* TRUE - indicates that OPC should generate a SELECT DISTINCT as opposed
** to a SELECT, for this query, this was introduced since deferred semantics
** for DEFERRED update produced sort nodes in query plans and cause SELECT
** DISTINCT to be generated when it really should not be,... OPC uses this
** flag to generate the DISTINCT and ignores the SORT node */
#define                 OPD_EXISTS      8
/* TRUE - indicates that the query mode has syntactic constraints so that
** a correlated EXISTS should be used for text generation */
#define                 OPD_NOFIXEDSITE 2
/* TRUE - if the results are to be passed back to the user, i.e. a retrieve
** or a read only cursor, without any updates, thus there are no site
** movement constraints which require the target relation to be on the
** final site */
#define                       OPD_NOFROM      16
/* TRUE - if we have an EXISTS or single relation update/replace query. Used
** in opc_nat() so that we use the base table name not a temporary table name */
    i4		    opd_user_parameters; /* number of user parameters supplied
					** with the query, this set of parameters
					** will be sent to each LDB */
    i4		    opd_expansion;
}   OPD_DOPT;


/*}
** Name: OPD_CSTATE - distributed OPC global state variable
**
** Description:
**      This structure contains variables which are global to
**      all subqueries, and are used only within OPC processing
**
** History:
**      15-nov-88 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPD_CSTATE
{
    OPD_ITEMP       opd_temp;           /* current number of temp
					** IDs defined for distributed
					** optimization */
    i4		    pad;
}   OPD_CSTATE;

/*}
** Name: OPG_SGATEWAY - gateway information associated with a subquery
**
** Description:
**      This structure defines gateway info which is to be used within
**      the scope of the subquery structure
**
** History:
**      29-mar-89 (seputis)
**          initial creation
[@history_template@]...
*/
typedef struct _OPG_SGATEWAY
{
    i4		opg_smask;                     /* set of boolean masks */
#define                 OPG_INDEX       1
/* set to TRUE if there is at least one table in this subquery which has
** an corresponding index included, but secondary index TID joins are not
** allowed... OPF must only used index substitution for tables like this */
}   OPG_SGATEWAY;
