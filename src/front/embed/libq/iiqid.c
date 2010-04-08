/*
** Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<generr.h>
# include	<gca.h>
# include	<iicgca.h>
# include	<iisqlca.h>
# include	<iilibq.h>
# include	<erlq.h>
/**
+*  Name: iiqryid.c - Routines to manage query ids returning from DBMS.
**
**  Defines:
**	IIqid_find	- Find a query id
**	IIqid_send	- Send query id to DBMS
**	IIqid_read	- Read a query id from DBMS
**	IIqid_add	- Add query id to defined list
**  	IIqid_free	- Free memory of query id lists.
**
**  Notes:
**	This file manages query ids that are returned from the DBMS.
**	A query id is made up of two i4's and a GCA_MAXNAME
**	blank-padded name.  
**	
**	The following example demonstrates how a query id is saved and used
**	for a REPEAT query.  Note that the preprocessor generates a "static"
**	FE query id (that the DBMS treats as an extended name) and that this
**	query id is matched to a DB-query id:
**
**	IIexExec(FE-id)		- Execute passing the FE-id.  The first time
**				  there is no matching DB-id, so we just skip
**				  this phase.
**	IIexDefine(FE-id)	- Define a query using the FE-id.  The result
**				  of this is that a DB-query-id returns. LIBQ
**				  puts the FE-id and the DB-id into the
**				  "defined" id list.
**	IIexExec(FE-id)		- Execute passing the FE-id.  This time round
**				  there is a matching DB-id, so we use that one
**				  to execute the REPEAT query.
**	
**	Note that cursor routines may also use some of this module, but they
**	manage their own cursor descriptors (that include a query/cursor id).
**
**	Database procedures also store a query/procedure id, returned on
**	each invocation of a procedure.  In this case, the program passes
**	the name of the id, and an internal DBMS id is returned for use
**	in subsequent invocations.
-*
**  History:
**	27-oct-1986	- Written (ncg)
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	26-aug-1987 	- Modified to use GCA. (ncg)
**	03-may-1988	- Modified for database procedures as well. (ncg)
**	22-jun-1988	- GCA_MAXNAME is the new size of all ids. (ncg)
**	12-dec-1988	- Generic error processing. (ncg)
**	24-jan-1991 	- Added IIqid_free().  (kathryn)
**	4/91		- Use FE memory allocator for qid's. (Mike S)
**	18-Dec-97 (gordy)
**	    Added support for multi-threaded applications.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/


/*{
+*  Name: IIqid_find - Find a DB-query id.
**
**  Description:
**	Given an FE-query id or a procedure name, find the matching
**	DB-query id in the "defined" query id list.
**
**  Inputs:
**	IIlbqcb	Current Session control block.
**	rep	- Flag to indicate repeat query or procedure.
**	name	- Name only used for procedure ids - size is GCA_MAXNAME.
**	num1	- Statically generated first id.
**	num2	- Statically generated second id.
**
**  Outputs:
**	Returns:
**	    IIDB_QRY_ID * - Pointer to first DB-query id which matched the
**			    FE-id.  (By construction, this should be the only
**			    one.) If one is not found then return NULL.
**	Errors:
**	    None
-*
**  Side Effects:
**	None
**	
**  History:
**	27-oct-1986	- Written (ncg)
**	03-may-1988	- Added procedure id searching (ncg)
**	24-mar-92 (leighb) DeskTop Porting Change: 
**		Converted to new style declaration because func proto needed.
**	6-apr-1992 (fraser)
**		Because of a policy clarification (prototypes legal in Ingres
**		code, but one still can't use them without ifdef CL_HAS_PROTOS),
**		ifdef-ed new style declaration.
*/

IIDB_QRY_ID	*
IIqid_find( II_LBQ_CB *IIlbqcb, bool rep, char *name, i4  num1, i4  num2 )
{
    II_QRY_ELM		*qid;

    /* Search for the FE query id and return the matching DB query id */
    for (qid = IIlbqcb->ii_lq_qryids.ii_qi_defined; qid; qid = qid->ii_qi_next)
    {
	if (rep && qid->ii_qi_rep)		/* Searching for repeat */
	{
	    if (qid->ii_qi_1id == num1 && qid->ii_qi_2id == num2)
		return &qid->ii_qi_dbid;
	}
	else if (!rep && !qid->ii_qi_rep)	/* Searching for procedure */
	{
	    if (MEcmp(name, qid->ii_qi_dbid.gca_name, GCA_MAXNAME) == 0)
		return &qid->ii_qi_dbid;
	}
    }
    return (IIDB_QRY_ID *)0;
}



/*{
+*  Name: IIqid_send - Send a DB-query id to the DBMS.
**
**  Description:
**	Given a DB-query id (probably found via IIqid_find or cursor routine)
**	send the id to the DBMS in the order described in the definition of
**	IIDB_QRY_ID.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	dbid - IIDB_QRY_ID - DB-query id.
**
**  Outputs:
**	Returns:
**	    Nothing
**	Errors:
**	    None
-*
**  Side Effects:
**	None
**	
**  History:
**	27-oct-1986	- Written (ncg)
*/

VOID
IIqid_send( II_LBQ_CB *IIlbqcb, IIDB_QRY_ID *dbid )
{
    DB_DATA_VALUE	dbv;

    /* Now send the DB-query id */
    II_DB_SCAL_MACRO(dbv, 0, sizeof(*dbid), dbid);
    IIcgc_put_msg(IIlbqcb->ii_lq_gca, TRUE, IICGC_VID, &dbv);
}


/*
+*  Name: IIqid_read - Read a query id from DBMS.
**
**  Description:
**	After some queries (ie, DEFINE phase of a REPEAT query or an OPEN 
**	CURSOR) a query id returns.  This routine reads the query id.
**	The format of the query id is 2 unique i4 integers, followed
**	by a blank padded (up to GCA_MAXNAME) name.
**
**  Inputs:
**	IIlbqcb		Current session control block.
**	dbid	 - IIDB_QRY_ID - Pointer to DB query id.
**
**  Outputs:
**	dbid		 - Fields filled with data just read.
**
**	Returns:
**	    STATUS - OK or FAIL
**
**	Errors:
**	    None
-*
**  Side Effects:
**	None
**	
**  History:
**	27-oct-1986	- Written. (ncg)
*/

STATUS
IIqid_read( II_LBQ_CB *IIlbqcb, IIDB_QRY_ID *dbid )
{
    i4  		resval;			/* Returns bytes from GCA */
    DB_DATA_VALUE   	dbv;

    /* Beginning of query id - must be 2 unique i4's */
    II_DB_SCAL_MACRO(dbv, 0, sizeof(*dbid), dbid);
    resval = IIcgc_get_value(IIlbqcb->ii_lq_gca, GCA_QC_ID, IICGC_VID, &dbv);
    if (resval != sizeof(*dbid))
    {
	IIlocerr(GE_LOGICAL_ERROR, E_LQ0080_QRYID, II_ERR, 0, (char *)0);
	return FAIL;
    }
    return OK;
}


/*{
+*  Name: IIqid_add - Add a query id to the "defined" list.
**
**  Description:
**	This routine adds a new FE & DB-query id to the defined list of 
**	query ids.  This list can later be searched (IIqid_find) for the
**	new id. 
**
**	First see if we already have an FE-id with the same value.  If we
**	do then overwrite it with the new info and return.  This can happen
**	if (for some reason) a query id (or defined REPEAT query) became
**	invalid and the query must be redefined.  The static FE-id remains
**	the same, but the DBMS has chosen a new DB-id for it.
**
**	In the case of a procedure id, on the first invocation return we
**	should have to add a new one.  On subsequent invocations, we
**	should always find the procedure we were just executing in order
**	to replace its id.
**
**	If we do not have one, then get one from the query id "pool" and add
**	it to the "defined" query id list.
**
**  Inputs:
**	IIlbqcb			Current session control block.
**	rep	- bool	      - Repeat query or procedure id.
**	num1	- i4  	      - Statically generated first id.
**	num2	- i4  	      - Statically generated second id.
**	dbid	- IIDB_QRY_ID - Pointer to DB query id just read.
**
**  Outputs:
**	Returns:
**	    STATUS - OK or FAIL - FAIL happens currently because of memory
**				  alocation failure.
**	Errors:
**	    None
-*
**  Side Effects:
**	1. May update an existing node in the "defined" list.
** 	2. Optionally allocates a node pool, and add a node to the "defined"
**	   list.
**	
**  History:
**	27-oct-1986	- Written (ncg)
**	03-may-1988	- Added procedure ids (ncg)
*/

STATUS
IIqid_add( II_LBQ_CB *IIlbqcb, bool rep, i4  num1, i4  num2, IIDB_QRY_ID *dbid )
{
# define II_QID_ALLOC	20

    II_QRY_ELM		*qid;
    II_QRY_IDS		*idlist;
    i4			i;

    /*
    ** Try to find the original one - if already defined we can just replace
    ** that element.  Key off the FE-id or procedure name (depending on
    ** context).
    */
    idlist = &IIlbqcb->ii_lq_qryids;
    for (qid = idlist->ii_qi_defined; qid; qid = qid->ii_qi_next)
    {
	if (rep && qid->ii_qi_rep)		/* Repeat query */
	{
	    if (qid->ii_qi_1id == num1 && qid->ii_qi_2id == num2)
	    {
		MEcopy((PTR)dbid, sizeof(*dbid), (PTR)&qid->ii_qi_dbid);
		return OK;
	    }
	}
	else if (!rep && !qid->ii_qi_rep)	/* Procedure id */
	{
	    if (MEcmp(dbid->gca_name, qid->ii_qi_dbid.gca_name,
		      GCA_MAXNAME) == 0)
	    {
		qid->ii_qi_dbid.gca_index[0] = dbid->gca_index[0];
		qid->ii_qi_dbid.gca_index[1] = dbid->gca_index[1];
		return OK;
	    }
	}
    }

    /* Pick one off the free list */
    if (idlist->ii_qi_pool == (II_QRY_ELM *)0)
    {
	/* Refresh free list with reasonable number of ids */
	MUp_semaphore( &IIglbcb->ii_gl_sem );
	if ( ! idlist->ii_qi_tag )  idlist->ii_qi_tag = FEgettag();
	qid = (II_QRY_ELM *)FEreqmem( idlist->ii_qi_tag,
				      (u_i4)(II_QID_ALLOC * sizeof( *qid )),
				      TRUE, NULL );
	MUv_semaphore( &IIglbcb->ii_gl_sem );

	/* Cannot define query id (out of memory) */
	if ( ! qid )  return( FAIL );

	idlist->ii_qi_pool = qid;
	/* Make contiguous storage into linked list */
	for (i = 1; i < II_QID_ALLOC; i++, qid++)
	    qid->ii_qi_next = qid + 1;
	qid->ii_qi_next = (II_QRY_ELM *)0;	/* Last one */
    }

    /* Pick off first from free list and assign (as stack) to defined list */
    qid = idlist->ii_qi_pool;
    idlist->ii_qi_pool = qid->ii_qi_next;
    /* Switch onto defined list (and it's there for good) */
    qid->ii_qi_next = idlist->ii_qi_defined;
    idlist->ii_qi_defined = qid;
    qid->ii_qi_rep = rep;
    qid->ii_qi_1id = num1;
    qid->ii_qi_2id = num2;
    MEcopy((PTR)dbid, sizeof(*dbid), (PTR)&qid->ii_qi_dbid);
    return OK;
}


/* Name: IIqid_free - Free Memory of repeat query id lists.
**
** Description:
**	This routine frees the memory used to store repeat query id 
**	information for the current database session. It is called 
**	from IILQsdSessionDrop() when a session is dropped.
**
** Inputs:
**	IIlbqcb		Current session control block.
**
** Outputs:
**	None.
** Returns:
**	Void.
**
** Side Effects:
**	IIlbqcb->ii_lq_qryids.ii_qi_pool = NULL
**	IIlbqcb->ii_lq_qryids.ii_qi_defined = NULL
**	IIlbqcb->ii_lq_qryids.ii_qi_tag = 0
**
** History:
**	24-jan-1991 (kathryn) - Written.
**      06-jul-1993 (larrym)
**          added freeing of proc_id's as well.
*/

VOID
IIqid_free( II_LBQ_CB *IIlbqcb )
{
    MUp_semaphore( &IIglbcb->ii_gl_sem );

    if ( IIlbqcb->ii_lq_qryids.ii_qi_tag )
	    IIUGtagFree( IIlbqcb->ii_lq_qryids.ii_qi_tag );

    IIlbqcb->ii_lq_qryids.ii_qi_tag = 0;
    IIlbqcb->ii_lq_qryids.ii_qi_pool = NULL; 
    IIlbqcb->ii_lq_qryids.ii_qi_defined = NULL; 

    if ( IIlbqcb->ii_lq_procids.ii_pi_tag )
	    IIUGtagFree( IIlbqcb->ii_lq_procids.ii_pi_tag );

    IIlbqcb->ii_lq_procids.ii_pi_tag = 0;
    IIlbqcb->ii_lq_procids.ii_pi_pool = NULL;
    IIlbqcb->ii_lq_procids.ii_pi_defined = NULL;

    MUv_semaphore( &IIglbcb->ii_gl_sem );
    return;
}
