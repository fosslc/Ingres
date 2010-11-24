/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>

/**
**
**  Name: PSQDELRNG.C - Functions used to delete all range variables that
**		    refer to a given table
**
**  Description:
**      This file contains the functions necessary to delete the range 
**      variables that refer to a given table.
**
**          psq_delrng	    - Delete all range variables
**	    psq_clrcache    - Clear cache entry(ies) depending on the table id
**			      stored in psq_cb.
**
**
**  History:    $Log-for RCS$
**      02-oct-85 (jeff)    
**          written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	10-aug-93 (andre)
**	    removed declaration of rdf_call()
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psq_delrng(
	PSQ_CB *psq_cb,
	PSS_SESBLK *sess_cb);
i4 psq_clrcache(
	PSQ_CB *psq_cb,
	PSS_SESBLK *sess_cb);

/*{
** Name: psq_delrng	- Delete all range variables
**
**  INTERNAL PSF call format: status = psq_delrng(&psq_cb, &sess_cb);
**
**  EXTERNAL call format:     status = psq_call(PSQ_DELRNG, &psq_cb, &sess_cb);
**
** Description:
**      The psq_delrng function deletes all range variables from the user range 
**      table.  Both user and the auxiliary range table will be affected.
**
**	This function will be useful when range variables become obsolete (e.g.
**	the table is destroyed).  Using this function will make it impossible
**	for the parser to produce a query tree that uses the obsolete table id,
**	thus avoiding endless loops when re-parsing and re-executing a query
**	that failed because a table is no longer there.
**	psq_clrcache() will be called to invalidate the cache entry(ies).
**
** Inputs:
**      psq_cb				ptr to a request control block
**	sess_cb				Pointer to session control block.
**
** Outputs:
**	psq_cb
**	    .psq_error			Error information
**		.err_code		    What error occurred
**		    E_PS0000_OK			Success
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**		    E_PS0205_SRV_NOT_INIT	Server not initialized
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-oct-85 (jeff)
**          written
**	26-nov-86 (daved)
**	    deleting a range var should mean to get rid of existing
**	    table information, not to get rid of the range var itself.
**	05-mar-1987 (fred)
**	    Call rdf_invalidate to kill of the cached info
**	27-may-1987 (daved)
**	    call clrrange to clear both range tables
**	10-jun-88 (stec)
**	    On RDF_INVALIDATE that invalidates the whole cash make
**	    sure that RDR_PROCEDURE bit in the mask is turned off.
**	06-nov-89 (andre)
**	    broke off the portion responsible for cache invalidation into a
**	    separate function - psq_clrcache().
**	15-jun-92 (barbara)
**	    pass sess_cb to pst_clrrng so that PSF can tell RDF whether
**	    session is distrib or not.
**	    BB_FIX_ME: Now that we pass in the sess control block, might
**	    be better to pass in a flag indicating to pst_clrrng whether
**	    to clear user's or auxiliary range table.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/
DB_STATUS
psq_delrng(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		    status;

    /*
    ** Clear out the user's range table.
    */
    if (status = pst_clrrng(sess_cb, &sess_cb->pss_usrrange,
			&psq_cb->psq_error))
    {
	return (status);
    }
    /*
    ** Clear out the auxiliary range table.
    */
    if (status = pst_clrrng(sess_cb, &sess_cb->pss_auxrng, &psq_cb->psq_error))
    {
	return (status);
    }

    return(psq_clrcache(psq_cb, sess_cb));
}

/*{
** Name: psq_clrcache	- this function would call RDF to invalidate cache
**			  information.
** Description:
**      psq_clrcache is called to invalidate cache entry(ies).
**	Depending on values stored in psq_cb->psq_table, one entry or the entire
**	cache will be invalidated.
** Inputs:
**      psq_cb
**          .psq_table                  Id of table for which to delete range
**					variables. Zero means delete all range
**					variables.
**	sess_cb				Pointer to session control block.
**	    .pss_distrib		DD_3_DDB_SESS indicates distrib session
**
** Outputs:
**	psq_cb
**	    .psq_error			Error information
**		.err_code		    What error occurred
**		    E_PS0000_OK			Success
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**		    E_PS0205_SRV_NOT_INIT	Server not initialized
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**  31-jan-90 (andre)
**	broke off from psq_delrng() so it could be called from other places
**	without invalidating all the range vars.
**  12-mar-90 (andre)
**	set the newly added field, rdr_2types_mask, to 0;
**  21-may-90 (teg)
**	init rdr_instr to RDF_NO_INSTR.
**  15-jun-92 (barbara)
**	Sybil merge.  Set rdr_r1_distrib.
**  11-aug-93 (andre)
**	cast 2-nd arg to rdf_call() to avoid acc warnings
**	6-Jul-2006 (kschendel)
**	    Update unique dbid type (is i4).
*/
DB_STATUS
psq_clrcache(
	PSQ_CB             *psq_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS			status;
    RDF_CB			rdf_cb;
    extern PSF_SERVBLK		*Psf_srvblk;

    /* Set the RDF control structure address */
    rdf_cb.rdf_rb.rdr_fcb	    = Psf_srvblk->psf_rdfid;
    rdf_cb.rdf_info_blk		    = (RDR_INFO*) NULL;

    /* Indicate to RDF whether or not this is a distributed session */
    rdf_cb.rdf_rb.rdr_r1_distrib    = sess_cb->pss_distrib;

    /* To indicate that procedures are not involved. */ 
    rdf_cb.rdf_rb.rdr_types_mask    = (RDF_TYPES) 0;

    rdf_cb.rdf_rb.rdr_2types_mask   = (RDF_TYPES) 0;
    rdf_cb.rdf_rb.rdr_instr	    = RDF_NO_INSTR;

    /* invalidate cache entry */
    STRUCT_ASSIGN_MACRO(psq_cb->psq_table, rdf_cb.rdf_rb.rdr_tabid);
	
    if ((psq_cb->psq_table.db_tab_base == 0) && 
		(psq_cb->psq_table.db_tab_index == 0))
    {
	rdf_cb.rdf_rb.rdr_db_id = (PTR) NULL;
	rdf_cb.rdf_rb.rdr_unique_dbid = 0;
    }
    else
    {
	rdf_cb.rdf_rb.rdr_db_id	= sess_cb->pss_dbid;
	rdf_cb.rdf_rb.rdr_unique_dbid = sess_cb->pss_udbid;
    }
    if (status = rdf_call(RDF_INVALIDATE, (PTR) &rdf_cb))
    {
	(VOID) psf_rdf_error(RDF_INVALIDATE, &rdf_cb.rdf_error,
	    &psq_cb->psq_error);
	return(status);
    }
    return(E_DB_OK);
}
