/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <dbms.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <me.h>
# include    <cs.h>
# include    <pc.h>
# include    <lk.h>
# include    <st.h>
# include    <tm.h>
# include    <tr.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <ulh.h>
# include    <sxf.h>
# include    <sxfint.h>

/*
** Name: SXCDB.C - SXF routines for managing database info
**
** Description:
**	The file contains the routines used to manage SXF database information
**
**	    sxc_getdbcb  - Lookup database info, possibly initializing
**
**	    sxf_freedbcb - Free database info when not needed
**
** History:
**	10-may-1993 (robf)
**	    Initial creation.
**	13-oct-93 (robf)
**          Add st.h
**	04-Aug-1997 (jenjo02)
**	    Removed sxf_incr(). It's gross overkill to semaphore-protect a
**	    simple statistic.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* Forward/static declarations 

/*
** Name: SXC_GETDBCB - Get a database control block, optionally allocating if
**	 necessary.
**
** Description:
** 	This routine is used to find a database control block by name. It
**	returns a pointer to the CB if found, or NULL if not. If the create
**	parameter is set to true and the CB isn't found it will try to
**	allocate one then return that.
**
**	Overview of algorithm:-
**
**	Walk through all DBCBs checking the name to see if it matches
**	If found, then return it.
**	If not found and not create then return NULL
** 	If not found and create try to create the CB and return it.
**
** Inputs:
**
**	db_name	  - database to  lookup
**	create_cb - create if not found
**
** Outputs:
**	err_code  - error code if something went wrong.
**
** Returns:
**	DBCB if found.
**
** History:
**	10-may-1993 (robf)
**	    Initial creation.
**	26-may-94 (robf)
**          Add protection check on err_code so we don't
**	    dereference null on error.
*/
SXF_DBCB*
sxc_getdbcb(
	SXF_SCB	   *scb,
	DB_DB_NAME *db_name,
	bool	   create_cb,
	i4	   *err_code
)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    ULM_RCB		ulm_rcb;
    SXF_DBCB		*sxf_dbcb=NULL;
    SXF_DBCB		*dbcb;
    char		*func_name="sxc_getdbcb";

    /*
    ** Check parameters
    */
    if (!db_name || !err_code)
    {
	/*
	** Parameter error, fail
	*/
	_VOID_ ule_format(E_SX1076_PARM_ERROR, NULL, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_err, 2,
			STlength(func_name),func_name);
	if(err_code)
		*err_code = E_SX0004_BAD_PARAMETER;
	return NULL;
    }
    /*
    ** Get db semaphore to stop list changing under us
    */
    CSp_semaphore(TRUE, &(Sxf_svcb->sxf_db_sem));
    *err_code=E_SX0000_OK;
    for (;;)
    {
	/*
	** Walk through dbcbs in the Svcb
	*/
	dbcb=Sxf_svcb->sxf_db_info;
	while (dbcb)
	{
		if(dbcb->sxf_usage_cnt>0 &&
		   MEcmp((PTR)db_name, (PTR)&dbcb->sxf_db_name, sizeof(*db_name))==0)
			break;
		dbcb=dbcb->sxf_next;
	}
	/*
	** if found a dbcb, stop here
	*/
	if (dbcb)
	{
		sxf_dbcb=dbcb;
		break;
	}
	/*
	** Not found in list, check create flag and decide how to handle
	*/
	if (create_cb==FALSE)
	{
		/*
		** Don't create so return empty pointer
		*/
		sxf_dbcb=NULL;
		break;
	}
	/*
	** Need to create a new database control block. We walk along
	** looking for unused ones we can access.
	*/
	dbcb=Sxf_svcb->sxf_db_info;
	while (dbcb)
	{
		if(dbcb->sxf_usage_cnt== -1)
			break;
		dbcb=dbcb->sxf_next;
	}
	if (!dbcb)
	{
		/*
		** No unused cbs found so allocate one.
		*/
		/* Build and initialize the DB CB */
		ulm_rcb.ulm_facility = DB_SXF_ID;
		ulm_rcb.ulm_streamid_p = &Sxf_svcb->sxf_svcb_mem;
		ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
		ulm_rcb.ulm_psize = sizeof (SXF_DBCB);
		status = ulm_palloc(&ulm_rcb);
		if (status != E_DB_OK)
		{
		    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
			*err_code = E_SX1002_NO_MEMORY;
		    else
			*err_code = E_SX106B_MEMORY_ERROR;

		    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, 
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		    _VOID_ ule_format(*err_code, NULL, 
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}
	        if(Sxf_svcb->sxf_pool_sz < Sxf_svcb->sxf_stats->mem_low_avail)
		    Sxf_svcb->sxf_stats->mem_low_avail=Sxf_svcb->sxf_pool_sz;
		dbcb = (SXF_DBCB *) ulm_rcb.ulm_pptr;
		MEfill(sizeof(SXF_DBCB), 0, (PTR) dbcb);
		/*
		** Hook into database cb list, put at start
		*/
		dbcb->sxf_next=Sxf_svcb->sxf_db_info;
		Sxf_svcb->sxf_db_info=dbcb;
		Sxf_svcb->sxf_stats->db_build++;
	}
	/*
	** Mark this cb as being not free
	*/
	dbcb->sxf_usage_cnt=0;
	/*
	** Copy over the database name
	*/
	STRUCT_ASSIGN_MACRO(*db_name, dbcb->sxf_db_name);
	/*
	** Return pointer
	*/
	sxf_dbcb=dbcb;
	break;

    }
    CSv_semaphore(&Sxf_svcb->sxf_db_sem);
    /* Handle any errors and return to the caller */
    if (*err_code != E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX1077_SXC_GETDBCB;

    }
    return sxf_dbcb;
}

/*
** Name: SXC_FREEDBCB - Release a database control block.
**
** Description:
** 	This routine is used to free an unneeded DBCB. This decrements the
**	usage counter, then free in no users. Currently we just
**	mark the cb as unused (by setting the usage count to -1) and leaving
**	it alone.
**
**	Overview of algorithm:-
**
** Inputs:
**
**	sxf_dbcb  - database control block to free.
**
** Outputs:
**	None
**
** Returns:
**	None
**
** History:
**	10-may-1993 (robf)
**	    Initial creation.
*/
VOID
sxc_freedbcb(
	SXF_SCB	*scb,
	SXF_DBCB *sxf_dbcb
)
{
    DB_STATUS		status = E_DB_OK;
    ULH_RCB		ulhrcb;
    SXF_DBCB		*dbcb;
    char		*func_name="sxc_getdbcb";

    /*
    ** Get svcb semaphore to stop list changing under us
    */
    CSp_semaphore(TRUE, &(Sxf_svcb->sxf_db_sem));

    if (sxf_dbcb->sxf_usage_cnt>0)
	sxf_dbcb->sxf_usage_cnt--;

    if(sxf_dbcb->sxf_usage_cnt==0)
    {
	    /*
	    ** Mark this DBCB as not used
	    */
	    sxf_dbcb->sxf_usage_cnt= -1;
    }
    /*
    ** Release the semaphore
    */
    CSv_semaphore( &(Sxf_svcb->sxf_db_sem));
}
