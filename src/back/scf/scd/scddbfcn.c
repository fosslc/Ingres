/*
**Copyright (c) 2004 Ingres Corporation
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <cs.h>
#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <id.h>
#include    <pc.h>
#include    <tm.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <duf.h>
#include    <dudbms.h>
#include    <gca.h>

/* added for scs.h prototypes, ugh! */
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <scfcontrol.h>
#include    <scserver.h>

/**
**
**  Name: SCDDBFCN.C - Server level functions from DMF
**
**  Description:
**      This file contains server level functions necessary from DMF. 
**      Included herein are such things as starting dmf, adding databases, 
**      deleting databases, etc.
**
**          scd_dbadd - Add a database to the server
**          scd_dbdelete - Delete a database from the server
[@func_list@]...
**
**
**  History:    $Log-for RCS$
**      24-Jun-1986 (fred)    
**          Created on Jupiter
**     12-sep-1988 (rogerk)
**	    Changed DMC_FASTCOMMIT to DMC_FSTCOMMIT,
**	    DMC_FORCE to DMC_MAKE_CONSISTENT.
**	10-feb-89 (rogerk)
**	    Pass DMC_SOLECACHE flag to dmf if fast commit, sole-server or
**	    sole-cache are specified.
**	13-jul-92 (rog)
**	    Included ddb.h and er.h.
**	12-Mar-1993 (daveb)
**	    Add include <me.h>
**	2-Jul-1993 (daveb)
**	    prototyped.
**	16-Jul-1993 (daveb)
**	    include <tr.h> for xDEBUG code.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	29-Jul-1993 (daveb)
**	    handle prototyped sc0e_put.
**	09-oct-93 (swm)
**	    Bug #56437
**	    Changed PTR casts of sc_pid to i4 when assigning to dmc_id
**	    as the dmc_id type has now changed.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in scd_dblist().
** 	13-may-1997 (wonst02)
** 	    Use du_access, from the iidatabase tuple, for readonly database.
**      15-Mar-1999 (hanal04)
**          - Partial cross integraion of change 276554. Allow E_DM0011
**          in scd_dbadd(), as may occur occasionally due to redo of 
**          scs_dbadd().
**          - Do not log E_DM004D in scd_dbadd(), as we will retry the 
**          add DB operation in scs_dbopen(). b55788. 
**      27-May-1999 (hanal04)
**          Remove and reworked the above change for bug 55788. If
**          DMC_CNF_LOCKED is set in scd_db_add() pass this information 
**          to DMF in the dmc->dmc_flags_mask. b97083.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Use new form sc0ePut().
[@history_template@]...
**/

/*
**  Forward and/or External function references.
*/

GLOBALREF SC_MAIN_CB	    *Sc_main_cb;    /* core struct of SCF */

/*{
** Name: scd_dbadd	- add a database to the server
**
** Description:
**      This function adds a database to the Ingres DBMS Server. 
**      The call is made to DMF to add the database.
** 
**      To add the database, the appropriate control block is set up, 
**      DMF is called to add the database.  The status of this operation
**      is posted in the kdbl entry, so that other sessions which may be 
**      waiting on this operation will note its success or failure. 
**      The operation is marked complete, and the semaphore is released.
**
** Inputs:
**      dbcb				dbcb which indicates db to be added
**      error                           DB_ERROR structure to be filled in
**	dmc				DMC_CB sent in from above since there is
**					    one there
**	scf_cb				SCF_CB as above
**
**  NOTE:   This routine assumes that the kdbl list into which this dbcb is to
**	    added is already locked exclusively. 
**
** Outputs:
**      *error                          filled in with error information
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-Jun-1986 (fred)
**          Created
**      15-May-1988 (rogerk)
**          Added Fast Commit option to DMC_ADD_DB call.
**      10-aug-88 (eric)
**          Excluded the DMC_MAKE_CONSISTENT and DMC_PRETEND bits from the
**	    flags_mask for the DMC_ADD_DB call.
**	10-feb-89 (rogerk)
**	    Pass DMC_SOLECACHE flag to dmf if fast commit, sole-server or
**	    sole-cache are specified.
**	24-Mar-89 (ac)
**	    Turned off the DMC_NLK_SESS bit from the flags_mask for the
**	    DMC_ADD_DB call.  
**	8-nov-92 (ed)
**	    remove DB_MAXNAME dependency
**	9-dec-92 (ed)
**	    fix 8-nov-92 change to use correct literal
**	12-Mar-1993 (daveb)
**	    Add include <me.h>
**	2-Jul-1993 (daveb)
**	    prototyped. removed unused var 'i'
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      20-apr-1995 (ICL phil.p)
**          Introduced support for the Distributed Multi-Cache Management 
**          (DMCM) protocol.
**          If we're running sole_server or sole_cache, then we cannot run
**          DMCM. If we are running DMCM, effectively fast_commit in a
**          multi-cache mode, then explicilty set those options.
** 	13-may-1997 (wonst02)
** 	    Use du_access, from the iidatabase tuple, for readonly database.
**      15-Mar-1999 (hanal04)
**          - Partial cross integraion of change 276554. Allow E_DM0011, as
**          may occur occasionally due to redo of scs_dbadd(). 
**          - Do not log E_DM004D as we will retry the operation in
**          scs_dbopen(). b55788. 
**      27-May-1999 (hanal04)
**          Remove and reworked the above change for bug 55788. If
**          DMC_CNF_LOCKED is set pass this information to DMF. b97083.
**          
[@history_template@]...
*/
DB_STATUS
scd_dbadd(SCV_DBCB *dbcb, 
	  DB_ERROR  *error,
	  DMC_CB *dmc,		/* sent in to help */
	  SCF_CB *scf_cb )	/*    ditto   */
{
    i4			status;

    for (;;)
    {
	dmc->type = DMC_CONTROL_CB;
	dmc->length = sizeof(DMC_CB);

	STRUCT_ASSIGN_MACRO(dbcb->db_dbname, dmc->dmc_db_name);
	STRUCT_ASSIGN_MACRO(dbcb->db_dbowner, dmc->dmc_db_owner);

	/* Set the flags for the ADD_DB call. Note that either of DMC_PRETEND
	** or DMC_MAKE_CONSISTENT may be set in dbcb for the DMC_OPEN_DB call.
	** DMC_ADD_DB doesn't like these flags, so we need to exclude them.
	*/
	dmc->dmc_flags_mask = 
	(dbcb->db_flags_mask & ~(DMC_MAKE_CONSISTENT | DMC_PRETEND | DMC_NLK_SESS));

	dmc->dmc_db_access_mode = dbcb->db_access_mode;
	if (dmc->dmc_db_access_mode == DMC_A_WRITE
	    && dbcb->db_dbtuple.du_access & DU_RDONLY)
	    dmc->dmc_db_access_mode = DMC_A_READ;
	dmc->dmc_db_location.data_address = (char *) 
	    &dbcb->db_location.loc_entry;
	dmc->dmc_db_location.data_in_size = 
	    dbcb->db_location.loc_l_loc_entry;	    /* nbr locations * sizeof */
	dmc->dmc_id = (i4) Sc_main_cb->sc_pid;  /* use pid for server id */
	dmc->dmc_op_type = DMC_DATABASE_OP;

	/*
	** Set database access type - soleserver or multiserver.
	** The IIDBDB database is always opened multiserver.
	*/
	if ((dbcb->db_dbname.db_db_name[0] == 'i')
		&& (dbcb->db_dbname.db_db_name[1] == 'i')
		&& (MEcmp(&dbcb->db_dbname,
			DB_DBDB_NAME,
			sizeof(dbcb->db_dbname)) == 0))
	{
	    dmc->dmc_s_type = DMC_S_MULTIPLE;
	}
	else
	{
	    dmc->dmc_s_type = Sc_main_cb->sc_soleserver;

	    /*
	    ** Set Fast Commit and Sole-cache flags.  Using Fast Commit
	    ** implies that the database can only be used by one cache.
	    ** Using Sole-server implies sole-cache.
	    */

            /*
            ** (ICL phil.p)
            ** Is sc_soleserver=DMC_S_SINGLE or sc_solecache=DMC_S_SINGLE
            ** then we switch off DMCM.
            ** DMCM is effectively running FastCommit and MultiCache, so
            ** if DMCM is set, we explicitly set FastCommit.
            */

            if (Sc_main_cb->sc_fastcommit)
                dmc->dmc_flags_mask |= (DMC_FSTCOMMIT | DMC_SOLECACHE);

            /*
            ** (ICL phil.p)
            */
            if (Sc_main_cb->sc_dmcm)
                dmc->dmc_flags_mask |= DMC_DMCM;

            if ((Sc_main_cb->sc_soleserver == DMC_S_SINGLE) ||
                (Sc_main_cb->sc_solecache == DMC_S_SINGLE))
            {
                dmc->dmc_flags_mask |= DMC_SOLECACHE;
                /*
                ** (ICL phil.p)
                ** Unset DMCM protocol.
                */
                dmc->dmc_flags_mask &= ~DMC_DMCM;
            }

            /*
            ** For DMCM explicitly set FastCommit & Multi-Cache
            */
            if (dmc->dmc_flags_mask & DMC_DMCM)
            {
                Sc_main_cb->sc_fastcommit = TRUE;
                dmc->dmc_flags_mask |= DMC_FSTCOMMIT;
                dmc->dmc_flags_mask &= ~DMC_SOLECACHE;
                dmc->dmc_s_type = DMC_S_MULTIPLE;
            }

	    /*
	    ** If server class affinity uses, then DMCM is not
	    ** required, and sole cache is implicitly enforced.
	    */
	    if (Sc_main_cb->sc_class_node_affinity)
	    {
		dmc->dmc_flags_mask |= DMC_SOLECACHE;
		dmc->dmc_flags_mask &= ~DMC_DMCM;
	    }
	}

	status = dmf_call(DMC_ADD_DB, dmc);
        /* b55788 */
        if (status) 
	    break;
        dbcb->db_addid = dmc->dmc_db_id;
	
	break;
    }

    STRUCT_ASSIGN_MACRO(dmc->error, *error);
    if (status)
    {
        sc0ePut(NULL, E_SC010C_DB_ADD, NULL, 4, 
            sizeof(dbcb->db_dbname), (PTR)&dbcb->db_dbname,
	    sizeof(dbcb->db_dbowner), (PTR)&dbcb->db_dbowner,
	    sizeof(dbcb->db_access_mode), (PTR)&dbcb->db_access_mode,
	    sizeof(dbcb->db_flags_mask), (PTR)&dbcb->db_flags_mask);

	sc0ePut(NULL, E_SC010D_DB_LOCATION, NULL, 3,
	    sizeof(dbcb->db_location.loc_entry.loc_name),
	    (PTR)&dbcb->db_location.loc_entry.loc_name,
	    dbcb->db_location.loc_entry.loc_l_extent,
	    (PTR)dbcb->db_location.loc_entry.loc_extent,
	    sizeof(dbcb->db_location.loc_entry.loc_flags),
	    (PTR)&dbcb->db_location.loc_entry.loc_flags);
        sc0ePut(&dmc->error, 0, NULL, 0);
	scd_note(status, DB_DMF_ID);
    }
    return(status);
}



/*{
** Name: scd_dbdelete	- delete a database from the server
**
** Description:
**      This routine deletes a database from the server (via DMF). 
**      It is called from scs_dbclose() when a database is found 
**      to be unused.
**
**	It is assumed that the dbcb to be deleted is already locked
**	exclusively (via scf semaphores).  This routine will delete
**	the dbcb, unlink it from the que, and deallocate the memory.
**
** Inputs:
**      dbcb                            dbcb to delete
**      error                           standard area for errors
**	dmc				dmc_cb to use
**	scf_cb				scf_cb to use
**
** Outputs:
**      *error                          filled in if appropriate
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-Jun-1986 (fred)
**          Created on Jupiter
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/
DB_STATUS
scd_dbdelete( SCV_DBCB *dbcb,
	     DB_ERROR *error,
	     DMC_CB *dmc,	
	     SCF_CB *scf_cb )
{
    DB_STATUS           status;

    dmc->type = DMC_CONTROL_CB;
    dmc->length = sizeof(*dmc);
    dmc->dmc_op_type = DMC_DATABASE_OP;
    dmc->dmc_id = (i4) Sc_main_cb->sc_pid;
    dmc->dmc_db_id = dbcb->db_addid;
    status = dmf_call(DMC_DEL_DB, dmc);

    if (status)
    {
	sc0ePut(NULL, E_SC010E_DB_DELETE, NULL, 4, 
		 sizeof(dbcb->db_dbname), (PTR)&dbcb->db_dbname,
		 sizeof(dbcb->db_dbowner), (PTR)&dbcb->db_dbowner,
		 sizeof(dbcb->db_addid), (PTR)&dbcb->db_addid,
		 sizeof(dbcb->db_flags_mask), (PTR)&dbcb->db_flags_mask);
	sc0ePut(&dmc->error, 0, NULL, 0);
	scd_note(status, DB_SCF_ID);
	status = E_DB_FATAL;
    }
    return(status);
}

/*{
** Name: scd_dblist	- list databases which are known to the server
**
** Description:
**      This routine lists all databases which are known to the server. 
**      For each database, all information known is printed. 
**      This routines intended usage is for debugging and possible usage 
**      from the monitor session.  Note:  monitor session usage 
**      will require changes, since this routine currently uses TRdisplay() 
**      to dump data.
**
** Inputs:
**      none                            this routine is driven entirely from the
**                                      server control block
**
** Outputs:
**      (via TRdisplay())
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-jun-1986 (fred)
**          Created on Jupiter.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**      27-Mar-2009 (hanal04) Bug 121857
**          Resolve syntax error in sc0ePut() call. Found with xDEBUG defined.
[@history_template@]...
*/

#ifdef xDEBUG
DB_STATUS
scd_dblist(void)
{
    SCV_DBCB		*dbcb;
    i4			i;
    i4			errors;
    DB_STATUS		status;
    SCF_CB		scf_cb;

    status = E_DB_OK;
    CLRDBERR(&scf_cb.scf_error);

    CSget_sid(&scf_cb.scf_session);

    for (;;)
    {
	if (status = CSp_semaphore(1, &Sc_main_cb->sc_kdbl.kdb_semaphore))
	{
	    SETDBERR(&scf_cb.scf_error, 0, status);
	    break;
	}

	for ( dbcb = Sc_main_cb->sc_kdbl.kdb_next, i = 0, errors = 0;
		dbcb->db_next != Sc_main_cb->sc_kdbl.kdb_next;
		dbcb = dbcb->db_next)
	{
	    i++;
	    TRdisplay("\tDatabase Control Block at %p:\n", dbcb);
	    TRdisplay("\t\tdb_dbname %t\tdb_dbowner %t\n",
		    sizeof(dbcb->db_dbname), dbcb->db_dbname,
		    sizeof(dbcb->db_owner),  dbcb->db_dbowner);
	    TRdisplay("\t\tdb_next %p\t\tdb_ucount %d.\n", dbcb->db_next,
		    dbcb->db_ucount);
	    TRdisplay("\t\tdb_prev %p\t\tdb_state %w %<(%x)\n", dbcb->db_prev,
		    "invalid,SCV_IN_PROGRESS,SCV_ADDED", dbcb->db_state);
	    TRdisplay("\t\tdb_flags_mask %x\t\tdb_access_mode %x\n",
		    dbcb->db_flags_mask, dbcb->db_access_mode);
	    TRdisplay("\t\tdb_addid %p\tnumber of locations %d.\n",
		    dbcb->db_addid, dbcb->db_location.loc_l_loc_entry);
 
	    if (dbcb->db_type != DBCB_TYPE)
	    {
		errors++;
		TRdisplay("\t>>>INVALID CB TYPE -- is %x, should be %x\n",
			dbcb->db_type, DBCB_TYPE);
	    }
	    if (dbcb->db_length != sizeof(SCV_DBCB))
	    {
		errors++;
		TRdisplay("\t>>>INVALID LENGTH -- is %d., should be %d.\n",
		    dbcb->db_length, sizeof(SCV_DBCB));
	    }
	}
	TRdisplay("Total of %d. known databases found -- %d. errors.\n", i);
	CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore);
	break;
    }
    if (status)
    {
	sc0ePut(&scf_cb.scf_error, 0, NULL, 0);
	sc0ePut(NULL, E_SC0120_DBLIST_ERROR, NULL, 0);
    }
    if (errors)
	return(E_DB_ERROR);
    else
	return(E_DB_OK);
}
#endif
/*
[@function_definition@]...*/
