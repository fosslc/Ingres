/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <qu.h>
#include    <st.h>
#include    <sl.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <scf.h>

/**
**
**  Name: PSQSTRT.C - Functions used to start a parser server.
**
**  Description:
**      This file contains the functions necessary to start up the parser 
**      for the duration of a server.  This file defines: 
**
**          psq_startup() - Routine to start up the parser for a server.
**
**
**  History:    $Log-for RCS$
**      01-oct-85 (jeff)    
**          written
**      20-jul-89 (jennifer)
**          Added server capabilites to startup.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	03-aug-92 (barbara)
**	    copy sq_cb->psq_distrib to rdf_cb.rdf_distrib to initialize RDF.
**	18-feb-93 (walt)
**		Removed (the now incorrect) prototype of adg_add_fexi because ADF
**		has been prototyped in <adf.h>.
**	29-jun-93 (andre)
**	    #included ME.H and TR.H
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**	18-oct-93 (rogerk)
**	    Added initialization of the PSF_NO_JNL_DEFAULT flag.
**	24-june-1997(angusm)
**	    added handling of PSQ_AMBREP_64COMPAT for redo
**	    of ambiguous replace handling
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	10-Jan-2001 (jenjo02)
**	    Save offset to session's PSS_SESBLK in server's psf_psscb_offset.
**	18-Oct-2004 (shaha03)
**	    SIR #112918, Added configurable default cursor open mode support.
**      03-nov-2004 (thaju02)
**          Change memleft to SIZE_TYPE.
**	11-Jan-2004 (drivi01)
**	    Replaced extern Psf_init with GLOBALREF Psf_init b/c symbol
**	    must be exported as REFERENCE_IN_DLL.
**	12-Oct-2008 (kiria01) SIR121012
**	    Added MO hooks for PSF.
[@history_template@]...
**/

GLOBALREF i4		      Psf_init;
GLOBALREF	PSF_SERVBLK	*Psf_srvblk;


/*{
** Name: psq_startup	- Start up the parser for a server.
**
**  INTERNAL PSF call format:	 status = psq_startup(&psq_cb);
**
**  EXTERNAL call format:	 status = psq_call(PSQ_STARTUP, &psq_cb, NULL);
**
** Description:
**      The psq_startup function starts up the parser for the duration of 
**      a database server.  It initializes the parser so that parser sessions
**	can be started.  This involves setting global variables and statics
**	within the parser.  This operation will return an error if it is called
**	when there is already a running parser within the server.  If the parser
**	is shut down using the PSQ_SHUTDOWN operation, it will be OK to re-start
**	it using this operation.  Also, it is OK to re-try this operation if it
**	returns an error; be aware, though, that if the problem that caused it
**	to fail is not fixed, this will be futile.
**
**	This function will set the psq_sesize field to the number of bytes
**	needed for the session control block.
**
** Inputs:
**      psq_cb
**	    .psq_qlang			Bit map of the query languages allowed
**					in this server.
**	    .psq_mxsess			Maximum number of sessions allowed in
**					server at one time.
**          .psq_version                Must be one of the server capabilities
**                                      such as PSQ_C_C2SECURE.
**
** Outputs:
**      psq_cb
**	    .psq_sesize			Number of bytes to allocate for session
**					control block
**	    .psq_server			address of server control block
**	    .psq_error			Error information
**		.err_code		    What error occurred
**		    E_PS0000_OK			Success
**		    E_PS0002_INTERNAL_ERROR	Internal inconsistency in PSF
**		    E_PS0701_SRV_ALREADY_UP	Server was already initialized
**		    E_PS0702_NO_MEM		Can't allocate memory pool
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s)
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_FATAL			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Sets parser facility version number in server control block.
**	    Marks the server as initialized.
**
** History:
**	01-oct-85 (jeff)
**          written
**      20-jul-89 (jennifer)
**          Added server capabilites to startup.
**	1-feb-90 (andre)
**	    Let ADF know where to find the TABLE_RESOLVE function
**	    (pst_resolve_table()).  Until ADF changes have been integrated, new
**	    code will be #ifdef'd out, but, at least, I will no longer have to
**	    worry about accidently wiping it out.
**	12-sep-90 (sandyh)
**	    added psf memory startup option support.
**	16-nov-90 (andre)
**	    test for error after calling SCF to initialize the PSF server
**	    constrol block semaphore.
**	    Initialize scf_cb before calling scf_call().  Until now scf_call()
**	    always returned E_DB_ERROR which we chose to disregard.
**	    Unfortunately, the semaphore was never initialized.
**	22-jan-91 (andre)
**	    Initialize a new field in PSF_SERVBLK - psf_flags.
**	09-jul-92 (andre)
**	    un#ifdef code related to passing address of pst_resolve_table() to
**	    ADF
**	23-jun-93 (robf)
**	    Pass in optional PSQ_B1_SECURE startup option.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    removed cause of a compiler error
**	18-oct-93 (rogerk)
**	    Added initialization of the PSF_NO_JNL_DEFAULT flag.  This flag is
**	    passed in the psq_startup call when the DEFAULT_TABLE_JOURNALING
**	    server startup flag specifies nojournaling.  It indicates that
**	    tables created by this server should be created with nojournaling
**	    by default.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for psf_owner which has changed type to PTR.
**	24-june-1997(angusm)
**	    Add handling of PSQ_AMBREP_64COMPAT (bugs 79623, 80750, 77040)
**	09-Oct-1998 (jenjo02)
**	    Removed SCF semaphore functions, inlining the CS calls instead.
**	21-jul-00 (inkdo01)
**	    Support psq_maxmemf computation of session memory limit (same
**	    as OPF).
**	16-jul-03 (hayke02)
**	    Added assignment of psf_vch_prec to psq_vch_prec. This change
**	    fixes bug 109134.
**	24-aug-06 (toumi01)
**	    Add PSF_GTT_SYNTAX_SHORTCUT for optional "session." SIR.
**	31-aug-06 (toumi01)
**	    Delete PSF_GTT_SYNTAX_SHORTCUT.
**	3-march-2008 (dougi)
**	    Add PSF_CACHEDYN flag to reflect CBF "cache_dynamic" parm.
**      15-Feb-2010 (maspa05)
**          Added server_class so SC930 can output it
**	19-May-2010 (kiria01) b123766
**	    Set cardinality check flag into server block
*/
DB_STATUS
psq_startup(
	PSQ_CB             *psq_cb)
{
    i4		err_code;
    ULM_RCB		ulm_rcb;
    RDF_CCB		rdf_cb;
    DB_STATUS		status;
    SIZE_TYPE		memleft;


    /* Start out with no error */
    psq_cb->psq_error.err_code = E_PS0000_OK;

    /*
    ** It's an error to initialize an already-initialized server
    */
    if (Psf_init)
    {
	(VOID) psf_error(E_PS0701_SRV_ALREADY_UP, 0L, PSF_INTERR, &err_code,
	    &psq_cb->psq_error, 0);
	return (E_DB_FATAL);
    }

    /*
    ** Allocate the memory pool.
    */
    ulm_rcb.ulm_facility    = DB_PSF_ID;

    /*
    ** if user chose to override the default memory allocation, use the value
    ** provided by the user; otherwise pool size is the product of default per
    ** user and maximum number of sessions.
    */

    if (psq_cb->psq_mserver)
    {
	/*
	** Value specified by the user should not result in sessions getting
	** less than PSF_SESMEM_MIN bytes each
	*/
	if (psq_cb->psq_mserver < PSF_SESMEM_MIN * (i4) psq_cb->psq_mxsess)
	{
	    (VOID) psf_error(E_PS0704_INSF_MEM, 0L, PSF_INTERR, &err_code,
		&psq_cb->psq_error, 0);
	    return (E_DB_FATAL);
	}
	else
	{
	    ulm_rcb.ulm_sizepool = psq_cb->psq_mserver;
	}
    }
    else
    {
	ulm_rcb.ulm_sizepool = PSF_SESMEM * (i4) psq_cb->psq_mxsess;
    }

    memleft		    = ulm_rcb.ulm_sizepool;
    ulm_rcb.ulm_memleft	    = &memleft;
    ulm_rcb.ulm_blocksize   = 0;
    status = ulm_startup(&ulm_rcb);
    if (status != E_DB_OK)
    {
	(VOID) psf_error(E_PS0702_NO_MEM, 0L, PSF_CALLERR, &err_code,
	    &psq_cb->psq_error, 0);
	return (status);
    }
    if ((status = ulm_openstream(&ulm_rcb)) != E_DB_OK)
    {
	if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    (VOID) psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, &psq_cb->psq_error, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0A02_BADALLOC, ulm_rcb.ulm_error.err_code,
			PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	}

	return (status);
    }

    ulm_rcb.ulm_psize = sizeof (PSF_SERVBLK);
    if ((status = ulm_palloc(&ulm_rcb)) != E_DB_OK)
    {
	if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
	{
	    (VOID) psf_error(E_PS0F02_MEMORY_FULL, 0L, PSF_CALLERR, 
		&err_code, &psq_cb->psq_error, 0);
	}
	else
	{
	    (VOID) psf_error(E_PS0A02_BADALLOC, ulm_rcb.ulm_error.err_code,
			PSF_INTERR, &err_code, &psq_cb->psq_error, 0);
	}

	return (status);
    }

    Psf_srvblk = (PSF_SERVBLK *)ulm_rcb.ulm_pptr;

    /* initialize the semaphore */
    CSw_semaphore(&Psf_srvblk->psf_sem, CS_SEM_SINGLE, "PSF sem");

    psq_cb->psq_server	    = (PTR)Psf_srvblk;
    Psf_srvblk->psf_next	    = NULL;
    Psf_srvblk->psf_prev	    = NULL;
    Psf_srvblk->psf_length   = sizeof(PSF_SERVBLK);
    Psf_srvblk->psf_type	    = PSF_SRVBID;
    Psf_srvblk->psf_s_reserved = 0;
    Psf_srvblk->psf_l_reserved = 0;
    Psf_srvblk->psf_owner    = (PTR) PSFSRV_ID;
    Psf_srvblk->psf_srvinit  = 0;					       
    Psf_srvblk->psf_sess_num = 0;
    Psf_srvblk->psf_capabilities = 0;

    /* set Server class in the PSF server block */
    Psf_srvblk->psf_server_class = psq_cb->psq_server_class;

    /*
    ** Store away the language and version number.
    */
    Psf_srvblk->psf_lang_allowed = psq_cb->psq_qlang;

    /*
    ** Store away the capabilities, language and version number.
    */
    if (psq_cb->psq_version & PSQ_C_C2SECURE)
	Psf_srvblk->psf_capabilities |= PSF_C_C2SECURE;
    Psf_srvblk->psf_lang_allowed = psq_cb->psq_qlang;
    Psf_srvblk->psf_version	= PSF_VERSNO;	    /* Parser version number */

    /*
    ** Clear all trace flags for the server.
    */
    ult_init_macro(&Psf_srvblk->psf_trace, PSF_TBITS, PSF_TPAIRS, PSF_TNVAO);

    /*
    ** Start off with a session count of zero.
    */
    Psf_srvblk->psf_nmsess   = 0;

    /*
    ** Remember the maximum number of sessions.
    */
    Psf_srvblk->psf_mxsess   = psq_cb->psq_mxsess;

    /*
    ** Store away the session memory value if user chose to override our default
    */
    Psf_srvblk->psf_sess_mem = (psq_cb->psq_mserver)
				    ? psq_cb->psq_mserver * psq_cb->psq_maxmemf
				    : 0;

    Psf_srvblk->psf_vch_prec = psq_cb->psq_vch_prec;
    /*
    ** Return the size needed for the session control block.
    */
    psq_cb->psq_sesize	= sizeof(PSS_SESBLK);

    /* Save offset to any session's PSS_SESBLK */
    Psf_srvblk->psf_psscb_offset = psq_cb->psq_psscb_offset;

    Psf_srvblk->psf_poolid   = ulm_rcb.ulm_poolid;
    Psf_srvblk->psf_memleft  = *ulm_rcb.ulm_memleft;

    /*
    ** Initialize RDF for this facility.
    */
    rdf_cb.rdf_poolid	= ulm_rcb.ulm_poolid;
    /* use half of pool for RDF */
    rdf_cb.rdf_memleft	= Psf_srvblk->psf_memleft/2;
    /* remainder is for PSF use */
    Psf_srvblk->psf_memleft /= 2;
    /* figure max number of tables. assume average table has half the max
    */
    rdf_cb.rdf_max_tbl  = rdf_cb.rdf_memleft/(sizeof(DMT_TBL_ENTRY)+
	DB_MAX_COLS * sizeof (DMT_ATT_ENTRY)/2);
    rdf_cb.rdf_fac_id	= DB_PSF_ID;

    /* Tell RDF is this is a distributed session/server */
    rdf_cb.rdf_distrib = psq_cb->psq_distrib;

    status = rdf_call(RDF_INITIALIZE, (PTR) &rdf_cb);
    if (status != E_DB_OK)
    {
	(VOID) psf_rdf_error(RDF_INITIALIZE, &rdf_cb.rdf_error,
	    &psq_cb->psq_error);
	/* On error, deallocate the memory pool */
	(VOID) ulm_shutdown(&ulm_rcb);
	return (status);
    }
    Psf_srvblk->psf_rdfid    = rdf_cb.rdf_fcb;
    
    Psf_srvblk->psf_flags = 0L;

    if (psq_cb->psq_flag & PSQ_NOTIMEOUTERROR)
    {
	Psf_srvblk->psf_flags |= PSF_NO_ZERO_TIMEOUT;
    }

    if (psq_cb->psq_flag & PSQ_DFLT_DIRECT_CRSR)
    {
	Psf_srvblk->psf_flags |= PSF_DEFAULT_DIRECT_CURSOR;
    }

    if (psq_cb->psq_flag2 & PSQ_DFLT_READONLY_CRSR)
    {
	Psf_srvblk->psf_flags |= PSF_DFLT_READONLY_CRSR;
    }

    if (psq_cb->psq_flag & PSQ_NOFLATTEN_SINGLETON_SUBSEL)
    {
	Psf_srvblk->psf_flags |= PSF_NOFLATTEN_SINGLETON_SUBSEL;
    }

    if (psq_cb->psq_flag & PSQ_NO_JNL_DEFAULT)
    {
	Psf_srvblk->psf_flags |= PSF_NO_JNL_DEFAULT;
    }
    if (psq_cb->psq_flag & PSQ_AMBREP_64COMPAT)
    {
	Psf_srvblk->psf_flags |= PSF_AMBREP_64COMPAT;
    }
    if (psq_cb->psq_flag & PSQ_CACHEDYN)
    {
	Psf_srvblk->psf_flags |= PSF_CACHEDYN;
    }
    if (psq_cb->psq_flag & PSQ_NOCHK_SINGLETON_CARD)
    {
	Psf_srvblk->psf_flags |= PSF_NOCHK_SINGLETON_CARD;
    }


    /*
    ** Init MO access
    */
    psf_mo_init();

    /*
    ** Mark the server as initialized.
    */
    Psf_srvblk->psf_srvinit  = TRUE;
    Psf_init		    = TRUE;

    {
	/* let ADF know how to handle resolve_table() */
        ADF_CB              adf_scb;
	char		    err_msg[DB_ERR_SIZE];

	adf_scb.adf_errcb.ad_ebuflen = DB_ERR_SIZE;
	adf_scb.adf_errcb.ad_errmsgp = err_msg;
	
	status = adg_add_fexi(&adf_scb, ADI_00RESTAB_FEXI, pst_resolve_table);

	if (status != E_DB_OK &&
	    adf_scb.adf_errcb.ad_emsglen > 0 &&
	    adf_scb.adf_errcb.ad_errmsgp != (char *) NULL)
	{
	    (VOID) TRdisplay("%t\n", adf_scb.adf_errcb.ad_emsglen,
			     adf_scb.adf_errcb.ad_errmsgp);
	}
    }

    return    (status);
}
