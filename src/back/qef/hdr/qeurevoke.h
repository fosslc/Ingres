/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: QEUREVOKE.H - QEF data structures and definitions used in processing of
**		       REVOKE statement
**
** Description:
**      This file contains QEF data structures and definitions used in
**	processing of REVOKE statement
**
** History:
**
**	05-aug-92 (andre)
**	    written
**	16-jul-93 (andre)
**	    defined QEU_DEPTH_LIST
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
*/

/*
**  Forward and/or External typedef/struct references.
*/
typedef struct _QEU_REVOKE_INFO		QEU_REVOKE_INFO;
typedef struct _QEU_MATCH_PRIV_INFO	QEU_MATCH_PRIV_INFO;
typedef struct _QEU_PERM_DEPTH_INFO	QEU_PERM_DEPTH_INFO;
typedef	struct _QEU_DEPTH_LIST		QEU_DEPTH_LIST;
typedef struct _QEU_PDI_TBL_DESCR	QEU_PDI_TBL_DESCR;

/*
** degree of the table which will be built to keep track of privileges being
** revoked; this must be kept in sync with struct _QEF_REVOKE_INFO
*/
#define	QEU_REVOKE_TBL_DEGREE    (DB_COL_WORDS + 6)

/*
** Name:    QEU_REVOKE_INFO - description of privileges being revoked
**
** Description:
**	This structure will be used to describe privileges being revoked as a
**	part of processing DROP PERMIT or REVOKE statement
**
** History:
**
**	05-aug-92 (andre)
**	    written
**	15-sep-93 (andre)
**	    defined QEU_INVALIDATE_QPS
**	19-sep-93 (andre)
**	    defined QEU_PRIV_LOST_BY_OWNER 
*/
struct _QEU_REVOKE_INFO
{
    DB_TAB_ID	    qeu_obj_id;		/* id of the object */
    i4		    qeu_priv;		/* privilege being revoked */
    i2		    qeu_flags;		/* informational flags */
#define		QEU_PRIV_LOST		    ((i2) 0x01)
#define		QEU_GRANT_OPTION_LOST	    ((i2) 0x02)
    /* 
    ** if set, QEU_GRANT_COMPATIBLE_ONLY indicates that only GRANT-compatible 
    ** privilege was lost, i.e. if the dependent object is a QUEL
    ** view, before claiming that it has become abandoned, we need to
    ** determine whether there are any non-GRANT-compatible privileges
    ** which would keep it from becoming abandoned
    */
#define		QEU_GRANT_COMPATIBLE_ONLY   ((i2) 0x04)
    /*
    ** set if we need force invalidation of query plans which could be affected
    ** by alteration/destruction of permits, destruction of other DBMS objects,
    ** or downgrading of status of a dbproc caused by revocation of privilege 
    ** described by the record
    */
#define		QEU_INVALIDATE_QPS	    ((i2) 0x08)
    /*
    ** set if the record describes [GRANT OPTION FOR] privilege lost by the
    ** object's owner.  Since no one can explicitly revoke a privilege from
    ** the owner of an object X (that is, one CAN issue a REVOKE statement
    ** revoking some privilege(s) from the object's owner, it will have no real
    ** effect and qeu_match_privs() will ensure that qeu_revoke_privs() is 
    ** never told that the owner of an object lost some privilege(s) on that 
    ** object), it means that he/she lost some privilege on some object on 
    ** which X depends.  
    **
    ** Knowing that a [GRANT OPTION FOR] privilege was lost by the object's 
    ** owner allows us to short-circuit processing in qeu_revoke_privs() and 
    ** remove that privilege from ALL permits regardless of the grantor which 
    ** also allows us to determine that we have removed/altered ALL permits 
    ** dependent on a given privilege, so that we can then force invalidation 
    ** of all QPs which could be affected by revocation of specified privilege
    */
#define		QEU_PRIV_LOST_BY_OWNER	    ((i2) 0x10)

    i2		    qeu_gtype;		/* type of grantee losing privilege */
    DB_OWN_NAME	    qeu_authid;		/* id of grantee losing privilege */
    i4		    qeu_attrmap[DB_COL_WORDS];	/*
                                                ** attributes on which privilege
						** is being lost
						*/
};

/*
** degree of the table which will be built to keep track of privileges being
** checked in the course of determining whether certain privileges are still
** possessed by a specified grantee (in qeu_match_privs());
** this must be kept in sync with struct _QEU_MATCH_PRIV_INFO
*/
#define	QEU_MATCH_PRIV_TBL_DEGREE    (DB_COL_WORDS + 6)

/*
** Name:    QEU_MATCH_PRIV_INFO - description of privileges being checked in the
**				  course of determining whether certain
**				  privileges are still possessed by a specified
**				  grantee
**
** Description:
**	To determine whether an <auth id> still possesses a privilege, it is not
**	sufficient to find some permit granting that privilege to the <auth id>
**	or to PUBLIC.  Consider an example:
**
**	    U1: create table U1.T (i i);
**		grant select on U1.T to PUBLIC with grant option;
**	    U2: grant select on U1.T to PUBLIC with grant option;
**	    U1: revoke select on U1.T from public cascade;
**	    
**	If we were to claim that an <auth id> still possesses a privilege as
**	long as there is a permit granting it that privilege, we would
**	mistakenly fail to destroy the permit created by U2.
**
**	To avoid cases like this as well as possible cycles in the privilege
**	graph, we need to keep track of privileges checked so far and discard
**	those whose existence is not supported (directly or indirectly) by a
**	privilege granted by the object owner.
**
**	This can be reduced to searching for a path in a privilege graph defined
**	as follows:
**	    for a DBMS object X owned by U let G(X) be a directed graph
**	    consisting of
**	      case:
**	        (1) if X is a table or a view, then nodes labelled (U,P,C) and
**		    (U,P',C) for every legal privilege P on every column of X
**		    and nodes labelled (A,P,C) for every <auth id> A s.t. there
**		    is a permit granting P WGO on column C of X to A or to
**		    PUBLIC
**		    and nodes labelled (A,P',C) for every <auth id> A s.t. there
**		    is a permit granting P [WGO] on column C of X to A or to
**		    PUBLIC.
**	        (2) if X is a dbproc or a dbevent, then then nodes labelled
**		    (U,P) and (U,P') for every legal privilege P on X
**		    and nodes labelled (A,P) for every <auth id> A s.t. there is
**		    a permit granting P WGO on X to A or to PUBLIC,
**		    and nodes labelled (A,P') for every <auth id> A s.t. there
**		    is a permit granting P [WGO] on X to A or to PUBLIC.
**	    and
**	      case:
**	        (1) if privilege(s) apply to a table or a view X, then edges
**		    from node (A,P,C) to node (B,P,C) existing iff B granted
**		    privilege P WGO on column C of X to A or to PUBLIC
**		    and edges from node (A,P',C) to node (B,P',C) existing iff B
**		    granted privilege P [WGO] on column C of X to A or to
**		    PUBLIC.
**	        (2) if privilege(s) apply to a dbproc or a dbevent X, then edges
**	            (A,P) to node (B,P) existing iff B granted privilege P WGO
**		    on X to A or to PUBLIC
**		    and edges from node (A,P') to node (B,P') existing iff B
**		    granted privilege P [WGO] on X to A or to PUBLIC.
**
**	If X is a table or a view owned by U2, we will say that an <auth id> U1
**	possesses a privilege P WGO on a set of attributes S of X if in the
**	privilege graph G(X) described above, for every attribute s in S there
**	is a path from node (U1,P,s) to node (U2,P,s).
**
**	If X is a table or a view, we will say that an <auth id> U1 possesses a
**	privilege P on a set of attributes S of X if in the privilege graph G(X)
**	described above, for every attribute s in S there exists an edge from
**	(U1,P',s) to (U2,P',s) or there exists some other <auth id> U3 such that
**	there is an edge from (U1,P',s) to (U3,P',s) and there is a path from
**	node (U3,P,s) to node (U2,P,s).
**	
**	If X is a dbevent or a dbproc, we will say that an <auth id> U1
**	possesses a privilege P WGO on X if in the privilege graph G(X)
**	described above, there is a path from node (U1,P) to node (U2,P).
**
**	If X is a dbevent or a dbproc, we will say that an <auth id> U1
**	possesses a privilege Pon X if in the privilege graph G(X) described
**	above, there is an edge from (U1,P') to (U2,P') or there exists some
**	other <auth id> U3 such that there is an edge from (U1,P') to (U3,P')
**	and there is a path from node (U3,P) to node (U2,P).
**	
**	The privilege graph may contain cycles and in order to avoid them, we
**	will store representation of nodes already visited in a table with
**	tuples of type QEU_MATCH_PRIV_INFO.
**
** History:
**
**	18-aug-92 (andre)
**	    written
*/
struct _QEU_MATCH_PRIV_INFO
{
    i4		    qeu_priv;		/*
					** privilege being checked; if grantable
					** privilege is required,
					** DB_GRANT_OPTION bit will be set
					*/
    i2		    qeu_depth;		/*
					** number of the traversal of IIPROTECT
					** during which this row was added;
					** during n-th traversal we will match
					** IIPROTECT tuples to records of depth
					** n-1
					*/
    i2		    qeu_gtype;		/*
					** type of grantee thought to possess a
					** privilege
					*/
    DB_OWN_NAME	    qeu_authid;		/*
					** id of grantee thought to possess a
					** privilege
					*/
    i2		    qeu_offset;		/*
					** offset into the array of descriptions
					** of privileges to be checked passed by
					** caller of qeu_match_privs()
					*/
    i2		    qeu_flags;		/* additional info */

					/*
					** record was added in the course of
					** checking whether an <auth id>
					** posseses a privilege
					*/
#define	    QEU_PRIV_SOUGHT	    ((i2) 0x01)
					/*
					** record was added in the course of
					** checking whether an <auth id>
					** posseses GRANT OPTION FOR a privilege
					*/
#define	    QEU_PRIV_WGO_SOUGHT	    ((i2) 0x02)
    i4		    qeu_attrmap[DB_COL_WORDS];	/*
                                                ** attributes on which a grantee
						** is thought to possess a
						** privilege
						*/
};

/*
** degree of the table which will be built to keep track of permits for which
** depth is being recomputed in the course of destroying a permit;
** this must be kept in sync with struct _QEU_PERM_DEPTH_INFO
*/
#define	QEU_PERM_DEPTH_TBL_DEGREE    (DB_COL_WORDS + 6)

/*
** Name:    QEU_PERM_DEPTH_INFO - description of a privilege represented by
**				  a permit whose depth was recomputed due to
**				  destruction of a permit on which it depended
**
** Description:
**	When we destroy a permit specifying grantable privilege, we may need to
**	recompute depth of other permits on the same object which depended on
**	the newly desctroyed permit.  This structure describes a tuple in the
**	table which will be used to keep track of privileges represented by
**	permits whose depth was recomputed.
**
** History:
**
**	21-aug-92 (andre)
**	    written
**	16-jul-93 (andre)
**	    renamed qeu_permno to qeu_pdi_flags
**	    we used to insist that depth of a permit may be recomputed at most
**	    once during any invocation of qeu_depth.  As we are changing the
**	    conditions under which permit depth gets recomputed and doing away
**	    with the "at most one recomputation" requirement, permit number is
**	    no longer needed
*/
struct _QEU_PERM_DEPTH_INFO
{
    i2		    qeu_pdi_flags;	/*
					** misc bits of useful info
					** (none defined so far)
					*/

    i2		    qeu_old_depth;	/*
					** depth of permit before it was
					** recomputed
					*/
#define	    QEU_OLD_DEPTH_INVALID	((i2) -1)

    i2		    qeu_new_depth;	/*
					** recomputed depth of the permit
					*/
#define     QEU_NEW_DEPTH_INVALID       ((i2) -1)

    i2		    qeu_gtype;		/*
					** type of grantee specified in the
					** permit
					*/
    DB_OWN_NAME	    qeu_authid;		/*
					** id of grantee specified in the permit
					*/
    i4		    qeu_priv;	        /*
					** privilege specified by the permit
					*/
    i4		    qeu_attrmap[DB_COL_WORDS];	/*
                                                ** attributes on which a grantee
						** is thought to possess a
						** privilege
						*/
};

/*
** Name:    QEU_DEPTH_LIST - structure which will be used as we try to determine
**			     depth of a privilege based on depths of permits on
**			     which it depends
**
** Description:
**	When recomputing depth of a privilege (possibly represented by a
**	permit), we need to keep track of depths of permits on which the
**	abovementioned privilege depends.
**	
**	In case of privileges which can be granted only on object-wide basis
**	(e.g. EXECUTE, RAISE), this is pretty simple: we just look for a
**	lowest-depth permit granted to the specified grantee or to PUBLIC.
**
**	In case of GRANT-compatible privileges that can be granted on a per 
**	column basis (so far it means UPDATE or REFERENCES) things get more
**	compicated: privilege on various columns may depend on permits with
**	different depths.  If that is the case, then for each column we need to
**	keep track of the permit with lowest depth on which privilege on that
**	column depends; however, the depth of the privilege will depend on the
**	highest minimum depth value among all columns.  This requires that we
**	maintain a list of minimum depths on per column basis which is
**	precisely what this structure will provide.
**
** History:
**
**	16-jul-93 (andre)
**	    written
*/
struct _QEU_DEPTH_LIST
{
				    /*
				    ** if we have determined depth value for 
				    ** each of the columns of interest, we can 
				    ** determine aggregate depth value - this 
				    ** will enable us to avoid considering 
				    ** permits which will not help us minimize 
				    ** the aggregate depth value (i.e. permits 
				    ** whose depth is greater than the 
				    ** "current aggregate depth" value)
				    */
    i4			qeu_cur_aggr_depth;
#define		QEU_AGGR_DEPTH_UNKNOWN		-1

				    /*
				    ** if a privilege can be granted on a per
				    ** column basis, we need to keep track of 
				    ** depth of permits for every column; 
				    ** qeu_col_depth_list_len will contain
				    ** number of entries in the list
				    */
    i4			qeu_col_depth_list_len;

				    /*
				    ** this array will contain lowest depths of
				    ** permits on which privilege on a given 
				    ** column will depend; for N-th column of a
				    ** table or view, the lowest depth value 
				    ** will be found in qeu_col_depth_list[N]
				    **
				    ** NOTE: this is a variable length array 
				    **       which must be the last element of
				    **	     the structure
				    */
    i2			qeu_col_depths[1];
				    /*
				    ** we are yet to find a permit conveying 
				    ** grantable privilege on this column
				    */
#define		QEU_DEPTH_UNKNOWN	((i2) -1)
				    
				    /*
				    ** we don't care if we never find a permit
				    ** conveying grantable privilege on this 
				    ** column
				    */
#define		QEU_DEPTH_IRRELEVANT	((i2) -2)

};

/*
** Name: QEU_PDI_TBL_DESCR - (PDI stands for Privilege Depth_info) 
**			     Structure containing control blocks and various 
**			     pointers needed to create, open, and destroy the 
**			     temp table which will contain descriptions of 
**			     privileges whose depths have changed as well as 
**			     fetch/insert tuples from/into that table
** Description:
**	Destruction/alteration of permits in the course of processing 
**	DROP PERMIT and REVOKE statements may force us to recompute depths of
**	some permits.  In the past, we were trying to recompute permit depths 
**	by calling qeu_depth() whenever some permit was dropped or altered.
**	Because this would generally happen BEFORE we had a chance to check for
**	abandoned permits, we had to insist that qeu_depth() may update depth 
**	of a given permit at most once during one invocation (this kept us from
**	getting stuck in cycles formed by permits conveying abandoned 
**	privileges which would eventually get cleaned up by qeu_priv_revoke()).
**
**	Unfortunately, there are plenty of cases where this restriction 
**	prevented us from correctly recomputing permit depths.  In order to 
**	solve this problem, will will remove the restrictions on number of 
**	times qeu_depth() may recompute depth of a given permit and qeu_depth()
**	will only be called when qeu_match_privs() determines that some 
**	privilege is not abandoned and the aggregate depth of permits conveying
**	that privilege is greater than that of the permit that was dropped or 
**	altered.  
**
**	While this will work for privileges that can be granted only on per 
**	object basis, complications may arise with revocation of 
**	column-specific privileges.  In particular, it is possible for a single
**	permit to convey privilege which is abandones one a subset of attributes
**	named in the permit.  As a result, qeu_depth() may find itself stuck
**	in a cycle formed by 2 or more permits s.t. privilege they convey on 
**	some of the attributes is abandoned.  
**
**	A sure way to avoid getting caught in cycles of abandones privileges is
**	to complete performing revocation of the specified privilege(s) before
**	attempting to recompute depths of remaining permits.  Originally, 
**	I was contemplating saving only a description of privilege(s) 
**	explicitly revoked by the user, but this would not be sufficient 
**	since this information alone would not always enable us to determine 
**	all permits whose depths need to be recomputed.  So I opted to save 
**	descriptions of all privileges such that the aggregate depth of 
**	permits conveying them has increased as a result of revoking specified 
**	privilege(s) in a temp table which will be handed to qeu_depth() once 
**	we completed destruction/alteration of permits affected by revocation 
**	of specified privilege(s).  
**
**	This structure will contain pointers to structures which will be used 
**	to create, open, read, write, and close the temp table which will 
**	contain descriptions of privileges s.t. depth of permits conveying 
**	them has increased as a result of revoking specified privilege(s).  
**	Function initiating the revocation (qeu_dprot() or qeu_revoke() will 
**	create and open the table, qeu_match_privs() will populate it with 
**	descriptions of privileges and qeu_depth() will recompute depths of 
**	permits dependent on privileges stored in the table
*/
struct _QEU_PDI_TBL_DESCR
{
    QEU_CB		    qeu_pdi_qeucb;
    QEF_DATA		    qeu_pdi_read_qefdata;
    QEF_DATA		    qeu_pdi_write_qefdata;
    DB_LOC_NAME		    qeu_pdi_locname;
    QEU_PERM_DEPTH_INFO	    qeu_pdi_read_row;
    QEU_PERM_DEPTH_INFO	    qeu_pdi_write_row;
    DMF_ATTR_ENTRY	    qeu_pdi_attr_descr[QEU_PERM_DEPTH_TBL_DEGREE];
    DMF_ATTR_ENTRY	    *qeu_pdi_attr_descr_p[QEU_PERM_DEPTH_TBL_DEGREE];
    DMT_CB		    qeu_pdi_dmtcb;
    DB_TAB_ID		    qeu_pdi_obj_id;
    i4			    qeu_pdi_num_rows;
    i4			    qeu_pdi_flags;
#define	    QEU_PDI_TBL_CREATED		0x01
#define	    QEU_PDI_TBL_OPENED		0x02
};
