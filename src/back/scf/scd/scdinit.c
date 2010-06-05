/*
** Copyright (c) 1987, 2009 Ingres Corporation
**
**
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <sl.h>
#include    <cm.h>
#include    <cs.h>
#include    <cv.h>
#include    <tm.h>
#include    <ci.h>
#include    <er.h>
#include    <ex.h>
#include    <lo.h>
#include    <nm.h>
#include    <pc.h>
#include    <pm.h>
#include    <tm.h>
#include    <tmtz.h>
#include    <st.h>
#include    <lk.h>
#include    <cx.h>
#include    <tr.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <adudate.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmtcb.h>
#include    <qsf.h>
#include    <ddb.h>
#include    <opfcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <scf.h>
#include    <sxf.h>
#include    <duf.h>
#include    <dudbms.h>
#include    <gca.h>
#include    <cut.h>
#include    <gwf.h>
#include    <lg.h>

/* added for scs.h prototypes, ugh! */
#include <dmrcb.h>
#include <copy.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <sca.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>
#include    <scserver.h>
#include    <scdopt.h>

/**
**
**  Name: SCDINIT.C - Initiation and Termination rtns for the server
**
**  Description:
**      This file contains the code which control the initiation and termination
**	of the INGRES DBMS Server.
**
**          scd_initiate - start the server and all its associated facilities
**          scd_terminate - stop all associated facilities and the server
**
**
**  History:
**       1-July-1986 (fred)    
**          Created on Jupiter
**	12-sep-1988 (rogerk)
**	    Changed DMC_FASTCOMMIT to DMC_FSTCOMMIT.
**	26-sep-1988 (rogerk)
**	    Save the min,max priority fields in the csib.
**	    Increased Fast Commit thread's priority.
**	15-sep-1988 (rogerk)
**	    Added support for write behind threads.
**	    Parse dmf startup options for buffer manager.
**	24-jan-1989 (mikem)
**	    CSadd_thread() takes a (CL_SYS_ERR *), not a (DB_STATUS *).
**      28-Mar-1989 (fred)
**          Added udadt support -- added server initialization thread 
**	    which does all the work...
**	20-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Add dbmsinfo requests GROUP_ID, APPLICATION_ID, QUERY_IO_LIMIT,
**	    QUERY_ROW_LIMIT, QUERY_CPU_LIMIT, QUERY_PAGE_LIMIT,
**	    QUERY_COST_LIMIT, and DB_PRIVILEGES.
**	06-may-89 (ralph)
**	    GRANT Enhancements, Phase 2b:
**	    Changed APPLICATION_ID to ROLE;
**	    Changed GROUP_ID to GROUP.
**	12-may-1989 (anton)
**	    Added collation dbmsinfo request
**	18-may-1989 (neil)
**	    Rules: Added RULE_DEPTH initialization.
**	30-may-89 (ralph)
**	    GRANT Enhancements, Phase 2x:
**	    Add dbmsinfo requests CREATE_TABLE, CREATE_PROCEDURE, LOCKMODE
**      16-jun-89 (fred)
**          Fixed bug in dbmsinfo() not setting return status in some
**	    situations.
**	21-jun-89 (ralph)
**	    Fix CREATE_PROCEDURE dbmsinfo request.
**	22-jun-89 (jrb)
**	    Set sc_capabilities field of Sc_main_cb with CI info.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Add dbmsinfo requests MAXQUERY, MAXIO, MAXROW, MAXCPU,
**	    MAXPAGE, and MAXCOST.
**	 9-Jul-89 (anton)
**	    Fix dbmsinfo LANGUAGE request
**	28-jul-89 (jrb)
**	    Set status to E_DB_FATAL in scd_initiate() whenever CI returns
**	    non-zero.  Before we were not detecting this error and we were
**	    allowing the server to come up in a half-started state.
**      14-jul-89 (jennifer)
**          Added CIcapability check for security.  Determines if 
**          we are running INGRES C2 secure.  B1 secure is not 
**          needed since it is determined at compile time.
**	08-sep-89 (paul)
**	    Added support for alerters. Initialize the alert data structures
**	    at server initialization
**	23-sep-1989 (ralph)
**	    Pass OPF_GSUBOPTIMIZE to OPF on startup if /NOFLATTEN specified.
**	10-oct-1989 (ralph)
**	    Pass value specified for OPF.MEMORY to OPF on startup.
**	17-oct-1989 (ralph)
**	    Change sense of /NOFLATTEN.
**	08-aug-1990 (ralph)
**	    Defined dbmsinfo('secstate') to show security state of the server.
**	    Defined dbmsinfo('update_syscat') to show session's value for this.
**	    Initialize dmc.dmc_show_state and sc_show_state.
**	    Add support for new RDF flags, including:
**		MEMORY, TBL_COLS, TBL_IDXS, MAX_TBLS, MULTICACHE, TBL_SYNONYMS
**	    Add support for new OPF flags, including:
**		CPUFACTOR, TIMEOUTFACTOR, REPEATFACTOR, SORTMAX
**	    Add support for new DMF flags, including:
**		SCANFACTOR
**	    Add support for new QEF flags, including:
**		SORT_SIZE
**	09-oct-1990 (ralph)
**	    6.3->6.5 merge:
**	    29-mar-1990 (alan)
**		Register and authorize either an INGRES server or an RMS
**		Gateway based upon the IIRUNDBMS /SERVER_CLASS.
**	    09-apr-90 (bryanp)
**		Initialize the GWF facility immediately before initializing
**		DMF by calling gwf_call(GWF_INIT). Terminate the GWF facility
**		immediately after terminating DMF, by calling
**		gwf_call(GWF_TERM).
**	    12-sep-1990 (sandyh)
**		Added passing of psf memory value to psf thru psq_cb.
**	15-oct-1990 (ralph)
**	    Uncomment support for RDF_NUM_SYNONYM for TEG
**	    Uncomment support for starting/stopping GWF
**	05-dec-1990 (ralph)
**	    Add comments to ease implementation of new OPF/QEF startup
**	    parameters when support for these parameters is added to OPF.
**	15-jan-1991 (ralph)
**	    Change cso_float from unioned to simple structure member
**	06-feb-1991 (ralph)
**	    Added support for dbmsinfo('session_id')
**	04-feb-1991 (neil)
**	    Call sce_shutdown when shutting server down.
**	    Alerters: Added /EVENTS initialization.
**	06-Dec-1990 (anton)
**	    Cleanup copying of server name returned by GCA_REGISTER
**	    Pass address of sc0m_semaphore to CS in scib.  This semaphore
**	    is needed to synchronize MEget_pages calls in an MCT server.
**	07-nov-1991 (ralph)
**          6.4->6.5 merge:
**		30-nov-90 (kirke/terjeb)
**                  Initialize cache_name in scd_initiate() so that the
**		    default cache name logic works correctly.
**		02-jan-1991 (ralph)
**		    Add support for new OPF flags, including:
**			CPUFACTOR, TIMEOUTFACTOR, REPEATFACTOR, SORTMAX
**		    Add support for new DMF flags, including:
**			SCANFACTOR
**		    Add support for new QEF flags, including:
**			SORT_SIZE
**		14-jan-1991 (rogerk)
**		    Took out ifdef ROGERKILDAY from dmf scanfactor startup flag
**		    handling that ralph integrated today as appropriate dmf
**		    handling is being integrated today as well.
**		21-jan-1991 (seputis)
**		    Added OPF server startup parameter support for EXACTKEY
**		    RANGEKEY, NONKEY, COMPLETE, ACTIVE, AGGREGATE_FLATTEN,
**		    FLATTEN
**		8-aug-1991 (stevet)
**		    Raised gca_local_protocol level to GCA_PROTOCOL_LEVEL_50.
**	07-apr-1992 (jnash)
**          Add support for locking dmf cache via the dmf.lock_cache
**          server startup param and the corresponding CSO_LOCK_CACHE case.
**      21-apr-1992 (schang)
**          GW merge
**	    4-nov-1991 (rickh)
**	        At gateway initialization time, write the release identifier
**	        returned by the gateway into the server control block.  This
**	        causes SCF to cough up the gateway identifier when poked with
**	        a "select dbmsinfo( '_version' )"
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              31-mar-90 (carl)
**                  Replaced the obsolete "DB" equate with the approved 
**		    "LO_DB" equate
**              18-apr-91 (fpang)
**                  Use Star_Version instead of Version.
**              18-apr-1991 (fpang)
**                  Inititalized local_crb.gca_password to NULL.
**              06-jun-1991 (fpang)
**                  Parameter to CS_add_thread() should be a cl_err_desc, not a
**                  db_status.
**              06-jun-1991 (fpang)
**                  Added /psf=mem startup support.
**              11-jun-1991 (fpang)
**                  Initialize psq_mserver.
**              21-jun-1991 (fpang)
**                  Added /opf=(..) startup support.
**          End of STAR comments.
**	25-jun-1992 (fpang)
**	    Included ddb.h and related include files for Sybil merge.
**	28-jul-1992 (fpang)
**	    In scd_initiate(), added support new rdf startup flags, rdf.ddbs, 
**	    rdf.avg_ldbs, rdf.cluster_nodes, rdf.netcost, rdf.nodecost. 
**	    See Teresa's SYBIL RDF Design Changes for more details.
**      29-jul-1992 (fpang)
**          In scd_initiate(), set sc_rdbytes to rdf_sesize, since RDF
**          now has a session control block, and is returning it's size.
**	19-aug-92 (pholman)
**	    The dbmsinfo('secstate') request is now obsolete, alomg with the
**	    need for the sc_show_state entry.
**      04-sep-1992 (fpang)
**          In scd_initiate(), set protocol level of distributed server
**          at GCA_PROTOCOL_40.
**	22-sep-1992 (bryanp)
**	    Added support for CSO_RECOVERY startup parameter, which indicates
**	    that this server is a "recovery server", not a normal server.
**	    Added support for CSO_LOGWRITER to indicate no. of logwriters.
**	    Added code for starting up the various logging & locking threads.
**	25-sep-92 (markg)
**	    Initialize the SXF facility immediately after initializing DMF by 
**	    calling sxf_call(SXC_STARTUP). Terminate the SXF facility before 
**	    terminating DMF by calling sxf_call(SXC_SHUTDOWN).
**      25-sep-92 (stevet)
**          Initialize semaphore used to manage timezone structure and
**          initialize the default timezone table.  Raised protocol
**          level to GCA_PROTOCOL_LEVEL_60 for local server.
**      25-sep-92 (stevet)
**          Added scd_adf_printf() function for ADF to print trace message.
**          This function is added to FEXI table so that ADF can call this
**          function without compiling with SCF headers.
**	08-sep-1992 (fpang)
**	    In scd_initiate(), if DDBS was not specified during server
**	    startup, set rdf_ccb.rdf_maxddb to number of users instead.
**	01-oct-1992 (fpang)
**	    Star now knows how to convert GCA_PROTOCOL_40 copy maps
**	    to GCA_PROTOCOL_50 copy maps, so remove protocol limitaion.
**	04-oct-1992 (ed)
**	    use defines for DB_MAXNAME dependencies
**	05-oct-1992 (ed)
**	    use defines for DB_MAXNAME dependencies
**	7-oct-1992 (bryanp)
**	    Make -recovery imply -nonames. Record -recovery in sc_capabilities.
**	23-Oct-1992 (daveb)
**	    Name semaphores for IMA
**	26-oct-1992 (rogerk)
**	    6.5 Recovery Project: Changed actions associated with some of
**	    the Consistency Point protocols. The system now requires
**	    that all servers participate in Consistency Point flushes, not
**	    just fast commit servers.  The name of the Fast Commit Thread 
**	    was changed to the Consistency Point Thread and is now started
**	    in all dbms servers.
**	05-nov-1992 (markg)
**	    Initialize the sc_show_state function pointer to contain the
**	    address of sxf_call. This is used to get around some
**	    shared image linking problems on VMS.
**	29-oct-1992 (fpang)
**	    If STAR don't initialize SXF, and don't start LG/LK threads.
**	12-nov-1992 (bryanp)
**	    Properly initialize status code in start_lglk_special_threads.
**	23-nov-1992 (fpang)
**	    In scd_inititate(), don't start Consistency Point Thread if 
**	    distributed.
**	    Also, start up GWF and DMF so OPF can call sort cost routines.
**	23-Nov-1992 (daveb)
**	    Abstract DBMS_INFO/ADF init into a table driven by a loop rather
**	    than 250+ lines of in-line goop.
**	24-Nov-1992 (daveb)
**	    Enable GCA_API_LEVEL_4 (from 2) to support GCM.
**	26-nov-1992 (markg)
**	    Added support for the new audit thread.
**	11-Jan-1993 (daveb)
**	    Make IMA DBMS_INFO constant lengths match LRC proposal,
**		DB_MAXNAME * 2.
**      14-jan-93 (schang)
**          GW merge.  Missed this one in June integration.
**	  5-nov-1991 (rickh)
**	    Get the version string out of the server control block, where it
**	    was stuffed at server initialization.  This allows gateways, at
**	    server startup, to overwrite this version string with their own
**	    release id.
**	18-jan-1993 (bryanp)
**	    Add support for timer-initiated consistency points.
**	17-jan-1993 (ralph)
**	    FIPS: Added support for new server startup parms and dbmsinfo() reqs
**	1-feb-1993 (markg)
**	    Added support for CSO_SXF_MEM startup parameter, also disallowed
**	    C2 auditing in Star server.
**      10-mar-1993 (stevet)
**          Add return stmt to scd_adf_print().
**	9-Mar-1993 (daveb)
**	    Somebody took out the change that let the recovery process register
**	    for IMA if -names was set.  Remove code that turned off registration
**	    in the recovery process, and explicitly default it to "no names"
**	    as long as no one sets -names.
**	12-Mar-1993 (daveb)
**	    Add include <me.h>
**	15-mar-1993 (ralph)
**	    FIPS: Tell PSF about /CURSOR_MODE=(DIRECT_UPDATE) and 
**		  /QUERY_FLATTEN=(NOSINGLETON).
**	06-apr-19993 (smc)
**	    Cast parameters to match prototypes.
**	07-may-93 (anitap)
**	    Added support for dbmsinfo for "SET UPDATE_ROWCOUNT" statement.
**	    Fixed compiler warning in scd_initiate() and scd_dbinfo_fcn().
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	30-jun-93 (shailaja)
**	    Fixed compiler prototype incompatibility in CSget_scb.
**	2-Jul-1993 (daveb)
**	    remove func externs; prototyped.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	7-jul-93 (rickh)
**	    Prototyped qef_call invocations.
**	26-jul-1993 (bryanp)
**	    If /GC_INTERVAL is 0, don't start a group commit thread.
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	08-aug-1993 (shailaja)
**	     Cast parameters in rdf_call.
**	23-aug-1993 (bryanp)
**	    If this is the star server, tell DMF so that it can do only a
**		minimal startup.
**	1-Sep-93 (seiwald)
**	    CS option revamp: scd_initiate() now gets its options directly
**	    from the config.dat file (by calling scd_options()), rather than
**	    relying on the previous CSinitiate/CSdispatch circus.  All
**	    parameters fetched from the config.dat file are now housed in
**	    various control blocks, including the SCD_CB to hold the misfits.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	20-sep-1993 (bryanp)
**	    If this is the star server, don't call sca_check, because since we
**		didn't start full DMF, we didn't get the version numbers back,
**		so sca_check will complain that the ADF identification is
**		unknown. It would be nice if we could get the UDT version
**		numbers without starting full DMF, but that's hard, so we bypass
**		the sca_check call instead.
**	    When shutting down a star server, set DB_2_DISTRIB_SVR in qef_cb.
**      20-sep-1993 (stevet)
**          Added E_SC033D_TZNAME_INVALID message for startup failure due
**          to invalid timezone settings. 
**	17-dec-93 (rblumer)
**	    removed SCI_FIPS startup/dbmsinfo option.
**	21-jan-94 (iyer)
**	  Add support for dbmsinfo requests 'db_tran_id' and 'db_cluster_node'
**	  Save cluster node name in SC_MAIN_CB.sc_cname.
**	2-Feb-1994 (fredv)
**	  The RS6000 is very picky on type mismatch, do the correct type
**		casting to fix them.
**	21-feb-94 (swm)
**	    Submission of this file in the following change was overlooked
**	    in the 31-jan-94 integration:
**	    10-oct-93 (swm)
**		Bug #56441
**		Pass scs_log_event() function, to be called on DMC_C_ABORT, via
**		new PTR char_pvalue rather than i4 char_value. This avoids
**		pointer truncation when the size of a pointer is larger than
**		the size of a i4.
**	    A consequence of this was:
**	    17-feb-94 (mikem)
**		Bug #59888
**		In scd_initiate() in the setup of the characteristics array
**		for the DMC_START_SERVER call, the force abort routine address
**		was being passed incorrectly in the "char_value" field. In the
**		31-jan-94 integration this value is expected by dmf to be
**		passed in the char_pvalue (as it is a pointer value). The
**		result of this was that all force_aborts would cause AV's in
**		the server as they tried to execute code at a garbage address.
**	14-feb-94 (rblumer)
**	   As part of cursor and flattening changes, take out 'default flatten'
**	   setting; zero already implies flattening on. (B59120, 09-feb-94 LRC).
**	14-apr-1994 (fpang)
**	   In scd_initiate(), allow ii.*.dbms.*.database_list to be a comma
**	   separated list.
**	   Fixes B57557, ii.*.dbms.*.database_list fails if comma separated.
**      10-May-1994 (daveb) 62631
**          Don't register STAR server as GCA_RG_INSTALL -- it doesn't
**  	    have LG/LK stats, so it isn't really installation-capable
**  	    for IMA purposes.  Only the RCP and a DBMS are.
**	16-sep-1994 (mikem)
**	    bug #58957
**	    Added code to support dbmsinfo('on_error_state').
**	23-may-1995 (reijo01)
**	    Added a new field to hold the startup time. This is used as a 
**	    managed object.
**	31-mar-1995 (jenjo02)
**	    Added DB_NDMF to possible values in ics_qlang
**	24-apr-1995 (cohmi01)
**	    Added IOmaster server type, readahead and writealong threads.
**	05-may-1995 (cohmi01)
**	    Added SC_BATCH_SERVER capability - pass to dmf.
**      05-apr-1995 (canor01)
**          Added support for GCA_BATCHMODE, single-client, shared
**          memory communications.
**	25-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	30-May-1995 (jenjo02)
**	    Changed gca_cb.gca_p_semaphore = CSp_semaphore; 
**	            gca_cb.gca_v_semaphore = CSv_semaphore; assignments
**	         to gca_cb.gca_p_semaphore = CSp_semaphore_function;
**                  gca_cb.gca_v_semaphore = CSv_semaphore_function;
**	    to distinguish the functions from the macros.
**	2-June-1995 (cohmi01)
**	    Pass down #readahead threads to DMC_START_SERVER.
**      12-jun-1995 (canor01)
**          semaphore protect NMloc()'s static buffer (MCT)
**      06-jul-1995 (canor01)
**          restore sc_loi_semaphore for non-MCT builds
**      17-jul-1995 (canor01)
**          use local buffer for location for MCT
**      24-jul-1995 (lawst01)
**          Added test for GCA_SINGLE_MODE operation.
**	19-sep-1995 (nick)
**	    Reduce the priority of the event thread to that of user
**	    sessions ; this prevents the server hanging when under load
**	    during a consistency point.
**	14-nov-1995 (nick)
**	    Increase our GCA protocol level.
**	12-dec-1995 (nick)
**	    Add dbmsinfo('db_real_user_case') - #71800.
**	23-Feb-96 (gordy)
**	    Conditionalize the usage of GCA_BATCHMODE on its existence.
**	    GCA may not support this flag even though it is supported in CL.
**	06-mar-1996 (nanpr01 & stial01)
**	    Call DMC_SHOW to get the maximum tuple size for the installation
**	    and initialize the Sc_main_cb->sc_maxtup field.
**          Increase size of dca array from 40 to 100, to accomodate the
**          additonal cache buffer paramaters. Add sanity check to make sure 
**          dca_index is less than size of dca array.
**          added SCI_MXRECLEN to return maximum tuple length available for
**	    that installation. This is for variable page size project to 
**	    support increased tuple length for star.
**	    Also for star, it is important that we take the max possible
**	    tuple size since we will not know beforehand what size tuple
**	    will be retrieved.
**          Also added SCI_MXPAGESZ, SCI_PAGE_2K, SCI_RECLEN_2K,
**          SCI_PAGE_4K, SCI_RECLEN_4K, SCI_PAGE_8K, SCI_RECLEN_8K,
**          SCI_PAGE_16K, SCI_RECLEN_16K, SCI_PAGE_32K, SCI_RECLEN_32K,
**          SCI_PAGE_64K, SCI_RECLEN_64K to inquire about availabity of
**          bufferpools and its tuple sizes respectively through dbmsinfo
**          function. If particular buffer pool is not available, return
**          0.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	04-apr-1996 (loera01)
**	    Added support for compression of varying-length datatypes
**          in scd_initiate(): if appropriate flag retrieved from the 
**          configuration data file, set capabilities mask to support 
**          compression. 
**	3-jun-96 (stephenb)
**	    Add code to create replicator queue management threads
**	03-jun-1996 (canor01)
**          OS-threads version of LOingpath no longer needs protection by
**          sc_loi_semaphore.
**	06-jun-1996 (thaju02)
**	    Increased size of dca array from 100 to 125 to account for
**	    additional dmf cache buffer parameters for variable page
**	    size support.
**	    Cleaned up compiler warnings of imcompatible prototype.
**	23-sep-1996 (canor01)
**	    Move global data definitions to scddata.c.
**	10-oct-1996 (canor01)
**	    Added SystemProductName to E_SC0129.  Add SystemVarPrefix to
**	    E_SC033D.  Add SystemVarPrefix to E_SC037B.
**	31-oct-1996 (canor01)
**	    In scd_terminate(), shut down GCF last to give any running
**	    threads a chance to disconnect.
**	15-nov-1996 (nanpr01)
**	    As per Jon Jensen's request, change the code to bring up
**	    LK callback thread only when it is required.
**	16-Nov-1996 (jenjo02)
**	    Added Deadlock Detector thread (SCS_SDEADLOCK).
**	12-dec-96 (inkdo01)
**	    Init opf_cb.opf_maxmemf (new parm).
**	30-jan-97 (stephenb)
**	    add scd_init_sngluser for single user servers (called from DMF).
**      27-feb-97 (stial01)
**          if parms indicated SINGLE SERVER mode, let dmf know
**	03-Jun-97 (radve01)
**	    Several info about main dynamic storage structures transferred
**	    to the server control block.
**	12-Jun-1997 (jenjo02)
**	    Use SHARE sem when walking sc_kdbl database list.
**	27-Jun-1997 (jenjo02)
**	    Removed sc_urdy_semaphore, which served no useful pupose.
**      24-jul-1997 (stial01)
**          Init gw_rcb.gwr_gchdr_size before gwf_call( GWF_INIT...
**	17-nov-97 (inkdo01)
**	    Init opf_cb.opf_cnffact (new parm)
**	25-Nov-1997 (jenjo02)
**	    Pass PSQ_FORCE when shutting down PSF so that PSF will continue
**	    the shutdown process even if there are open PSF sessions.
**      02-Apr-98 (fanra01)
**          Limit the maximum db sessions to 8 for the evaluation release.
**	25-Mar-1998 (jenjo02)
**	    Dump (new) session statistics to the log in scd_terminate().
**	31-Mar-1998 (jenjo02)
**	    Added *thread_id output parm to CSadd_thread() prototype.
**      02-Apr-98 (fanra01)
**          Limit the maximum db sessions to 8 for the evaluation release.
**	03-Apr-1998 (jenjo02)
**	    Call buffer manager to start the WriteBehind threads.
**	09-Apr-1998 (jenjo02)
**	    Set SC_IS_MT flag in sc_capabilities if OS threads.
**	13-Apr-1998 (hanch04)
**	    Added qef_qescb_offset for new qef_session call.
**	27-may-98 (mcgem01)
**	    The maximum number of users for a workgroup edition is
**	    now 25.  Also the developer and workgroup limits seemed
**	    to have been transposed.
**	13-may-1998 (canor01)
**	    Clean up another cross-integration problem by removing extra
**	    parameter from CSadd_thread().
**	27-may-98 (mcgem01)
**	    The maximum number of users for a workgroup edition is
**	    25.
**	31-aug-98 (sarjo01)
**	    Initialize new configurable param, opf_joinop_timeout 
**	02-Sep-1998 (loera01) Bug 92957
**	    In scd_initiate(), make sure replicator threads are accounted for 
**	    in the internal thread count.  
**      15-sep-98 (sarjo01)
**          Initialize new configurable param, opf_timeout_abort
**	10-nov-98 (sarjo01)
**	    Added db priv TIMEOUT_ABORT.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	09-jun-1999 (somsa01)
**	    In scd_initiate(), after a successful startup, also write
**	    E_SC051D_LOAD_CONFIG to the errlog.
**	01-jul-1999 (somsa01)
**	    We now also print out the server type when printing out
**	    E_SC051D_LOAD_CONFIG.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Changed gca_cb.gca_p_semaphore = CSp_semaphore_function; 
**	            gca_cb.gca_v_semaphore = CSv_semaphore_function;
**	    back to gca_cb.gca_p_semaphore = CSp_semaphore;
**                  gca_cb.gca_v_semaphore = CSv_semaphore;
**	08-Jan-2001 (jenjo02)
**	    Supply offset to session control block to DMF.
**	10-Jan-2001 (jenjo02)
**	    Supply offset to session control blocks to RDF, PSF, OPF.
**	19-Jan-2001 (jenjo02)
**	    Supply B1SECURE/C2SECURE info to adg_startup().
**	09-mar-2001 (stial01)
**	    Increased size of dca array from 125 to 150 to account for
**	    additional dbms server parameters
**      06-apr-2001 (stial01)
**          Added code for dbmsinfo('page_type_v*')
**      20-apr-2001 (stial01)
**          Added dbmsinfo('unicode')
**	11-may-2001 (devjo01)
**	    Add start of various cluster support threads for s103715.
**      14-may-2001 (stial01)
**          Replaced dbmsinfo('unicode') with dbmsinfo('unicode_level')
**	18-May-01 (gordy)
**	    Bumping GCA protocol level to 64 for national character sets.
**	19-mar-2002 (somsa01)
**	    In scd_initiate(), for EVALUATION_RELEASE, bumped
**	    Sc_main_cb->sc_max_conns from 8 to 32 to accomodate extra
**	    connections from the ICESVR and Portal.
**      08-may-2002 (stial01)
**          Added dbmsinfo('lp64'), returns 'Y' or 'N'.
**      20-aug-03 (chash01) change RMS gateway license to be CI_NEW_RMS_GATEWAY
**          (aka 2SCI)  110757/INGGAT349
**	3-Dec-2003 (schka24)
**	    QEF params were revised, reflect here;  pass a QEF cb to the
**	    option scanner.  (Why make qef grovel to opf for its options?)
**	    Remove some unreferenced and unused RDF things.
**      18-feb-2004 (stial01)
**          Increased qef_maxtup, opf_maxtup for 256k row support.
**          Fixed incorrect casting of length arguments.
**	20-Apr-04 (gordy)
**	    Bumping GCA protocol level to 65 for bigints.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	20-sep-2004 (devjo01)
**	    - Add init of sc_server_class, sc_class_node_affinity..
**	    - Remove unused scd_initialize autovars server_class, cache_name.
**	18-Oct-2004 (shaha03)
**	    SIR #112918, Added configurable default cursor open mode support.
**      11-apr-2005 (stial01)
**          Added SCI_CSRLIMIT for dbms('cursor_limit')
**	09-may-2005 (devjo01)
**	    Add SCS_SDISTDEADLOCK to the class of critical threads.
**	    If starting the RCP and configured for cluster, and DLM
**	    does not support transaction granularity, start our own
**	    distributed deadlock check thread.
**	21-jun-2005 (devjo01)
**	    Add SCS_SGLCSUPPORT to the class of critical threads.
**	    If running clustered, and DLM does not have native
**	    group lock container (GLC) support, start our own GLC support
**	    thread.
**	 6-Jul-06 (gordy)
**	    Bumping GCA protocol level to 66 for ANSI date/time.
**	 3-Nov-06 (gordy)
**	    Bumping GCA protocol level to 67 for LOB Locators.
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	13-Aug-2007 (jonj)
**	    Start Dist deadlock thread at lower prio than Local.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Use new form sc0ePut().
**      04-nov-2008 (stial01)
**          Moved static initialization of dbdb_dbtuple to scd_initiate()
**      16-nov-2008 (stial01)
**          Redefined name constants without trailing blanks.
**      16-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      14-jan-2009 (stial01)
**          Added support for dbmsinfo('pagetype_v5')
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**       8-Jun-2009 (hanal04) Code Sprint SIR 122168 Ticket 387
**          If dbms.*.mustlog_db_list is set establish the list of DB names
**          in MustLog_DB_Lst for cross checking on DB open.
**	15-Jun-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	 1-Oct-09 (gordy)
**	    Bumping GCA protocol level to 68 for long procedure names.
**	 Nov/Dec 2009 (stephenb)
**	     Batch processing; init new fields.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN void scd_mo_init(void);

static DB_STATUS start_lglk_special_threads(
				    i4  recovery_server,
				    i4  cp_timeout,
				    i4  num_logwriters,
				    i4  start_gc_thread,
				    i4  start_lkcback_thread);


static DB_STATUS scd_sec_init_config(
	bool	c2_secure_server
);
/*
** Definition of all global variables owned by this file.
*/

#define	NEEDED_SIZE (((sizeof(SC_MAIN_CB) + sizeof(SCV_LOC) + (SCU_MPAGESIZE - 1)) & ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE)

/* number of extras sessions to have CS support so we can have
   sessions available for special users even when regular users
   have chewed up all the -connected_session connections */

# define SCD_RESERVED_CONNS	(2)

GLOBALREF SC_MAIN_CB     *Sc_main_cb;    /* Core structure for SCF */
GLOBALREF i4	 Sc_server_type; /* for handing to scd_initiate */
GLOBALREF SCS_IOMASTER   IOmaster;   	 /* Anchor for list of databases */
					 /* that IOMASTER server handles */
GLOBALREF SCS_DBLIST     MustLog_DB_Lst; /* List of DBs that must log all
                                         ** operations under this DBMS
                                         */
GLOBALREF DU_DATABASE	      dbdb_dbtuple;
/*}
** Name:	DBMS_INFO_DESC
**
** Description:
**	Describes a DBMS_INFO request we're registering.
**	We have a table of these.  Ordering of entries is unimportant.
**
**	To add a DBMS_INFO constant, add a define to scf.h, an entry
**	in this table below, and dispatching in scuisvc.c.
**
** History:
**	23-Nov-1992 (daveb)
**	   created.
**	11-Jan-1993 (daveb)
**	    Make IMA DBMS_INFO constant lengths match LRC proposal,
**		DB_MAXNAME * 2.
**	17-jan-1993 (ralph)
**	    FIPS: Added support for new server startup parms and dbmsinfo() reqs
**	9-sep-93 (robf)_
**	    Add MAX_PRIV_MASK and CURRENT_PRIV_MASK to return current/max
**	    privilege set. These are UNDOCUMENTED for internal use only.
**	    Add QUERY_SYSCAT request
**	    Add SECURITY_AUDIT_STATE
**	    Add TABLE_STATISTICS, CONNECT_TIME_LIMIT, IDLE_TIME_LIMIT,
**	        SESSION_PRIORITY, SESSION_PRIORITY_LIMIT
**	    Add MAXIDLE, MAXCONNECT
**	30-nov-93 (robf)
**          QUERY_SYSCAT becomes SELECT_SYSCAT per LRC
**	17-dec-93 (rblumer)
**	    removed SCI_FIPS startup/dbmsinfo option.  "FIPS mode" no longer
**	    exists.  It was replaced some time ago (17-jan-93) by several
**	    feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).
**	14-feb-94 (rblumer)
**	    removed FLATOPT and FLATNONE dbmsinfo requests (part of B59120);
**	    moved FLATTEN request to be next to FLATAGG and FLATSING requests,
**	    changed double cursor request names CSRDIRECT/CSRDEFER to
**	    single CSRUPDATE request (part of B59119; also LRC 09-feb-1994).
**	1-mar-94 (robf)
**          added b1_server.
**	12-dec-95 (nick)
**	    DB_REAL_USER_CASE dbmsinfo option wasn't here ; add it.
**	06-mar-1996 (nanpr01)
**          added SCI_MXRECLEN to return maximum tuple length available for
**	    that installation. This is for variable page size project to 
**	    support increased tuple length for star.
**          Also added SCI_MXPAGESZ, SCI_PAGE_2K, SCI_RECLEN_2K,
**          SCI_PAGE_4K, SCI_RECLEN_4K, SCI_PAGE_8K, SCI_RECLEN_8K,
**          SCI_PAGE_16K, SCI_RECLEN_16K, SCI_PAGE_32K, SCI_RECLEN_32K,
**          SCI_PAGE_64K, SCI_RECLEN_64K to inquire about availabity of
**          bufferpools and its tuple sizes respectively through dbmsinfo
**          function. If particular buffer pool is not available, return
**          0.
**      03-jun-1996 (canor01)
**          OS-threads version of LOingpath no longer needs protection by
**          sc_loi_semaphore.
**	16-mar-2001 (stephenb)
**	    Added ucollation for unicode collation name.
**	23-jun-2003 (somsa01)
**	    In dbms_info_items[], SESSION_ID should return 16 characters
**	    on LP64 platforms.
**	26-jun-2003 (somsa01)
**	    Removed NO_OPTIM for i64_win starting with the GA version of the
**	    Platform SDK.
**	12-apr-2005 (gupsh01)
**	    Added support for UNICODE_NORMALIZATION.
**	29-aug-2006 (gupsh01)
**	    Added support for ANSI datetime system constants: CURRENT_DATE, 
**	    CURRENT_TIME, CURRENT_TIMESTAMP, LOCAL_TIME, LOCAL_TIMESTAMP.
**	29-sep-2006 (gupsh01)
**	    Added support for SCI_DTTYPE_ALIAS.
**	17-may-2007 (dougi)
**	    Added CHARSET.
**	7-Nov-2007 (kibro01) b119428
**	    Added support for DATE_FORMAT, MONEY_FORMAT, etc
**	20-Nov-2007 (kibro01) b119428
**	    Removed non-portable strlen() in initialisation
**	 7-Jul-2009 (hanal04) Bug 122266
**	    Removed CURRENT_DATE, CURRENT_TIME, CURRENT_TIMESTAMP, LOCAL_TIME, 
**          LOCAL_TIMESTAMP as DBMSINFO() parameters. They all fail with
**          E_SC0206 and the constants can be selected directly.
**	26-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query support: Added CARDINALITY_CHECK
**	03-Dec-2009 (kiria01) b122952
**	    Add .opf_inlist_thresh for control of eq keys on large inlists.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add SCI_PAGETYPE_V6, SCI_PAGETYPE_V7
*/
typedef struct
{
    char	*info_name;
    i4		info_reqcode;
    i4		info_inputs;
    i4		info_dt1;
    i4		info_fixedsize;
    i4		info_type;
} DBMS_INFO_DESC;

static DBMS_INFO_DESC dbms_info_items[] =
{
    { "TRANSACTION_STATE",	SC_XACT_INFO,	0,	0, /* 0 */
	  			4,	DB_INT_TYPE },

    { "AUTOCOMMIT_STATE",	SC_AUTO_INFO,	0,	0, /* 1 */
				4,	DB_INT_TYPE },

    { "_BINTIM",	SCI_NOW,	0,	0, /* 2 */
			4,	DB_INT_TYPE },

    { "_CPU_MS",	SCI_CPUTIME,	0,	0, /* 3 */
			sizeof(i4),	DB_INT_TYPE },

    { "_ET_SEC",	SC_ET_SECONDS,	0,	0, /* 4 */
			4,	DB_INT_TYPE },

    { "_DIO_CNT",	SCI_DIOCNT,	0,	0, /* 5 */
			sizeof(i4),	DB_INT_TYPE },

    { "_BIO_CNT",	SCI_BIOCNT,	0,	0, /* 6 */
			sizeof(i4),	DB_INT_TYPE },

    { "_PFAULT_CNT",	SCI_PFAULTS,	0,	0, /* 7 */
			sizeof(i4),	DB_INT_TYPE },

    { "DBA",	SCI_DBA,	0,	0, /* 8 */
				sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "USERNAME",	SCI_USERNAME,	0,	0, /* 9 */
				sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "_VERSION",	SCI_IVERSION,	0,	0, /* 10 */
			DB_TYPE_MAXLEN,	DB_CHR_TYPE },

    { "DATABASE",	SCI_DBNAME,	0,	0, /* 11 */
			sizeof(DB_DB_NAME),	DB_CHR_TYPE },

    { "TERMINAL",	SCI_UTTY,	0,	0, /* 12 */
			sizeof(DB_TERM_NAME),	DB_CHR_TYPE },

    { "QUERY_LANGUAGE",		SC_QLANG,	0,	0, /* 13 */
				sizeof(DB_TERM_NAME),	DB_CHR_TYPE },

    { "DB_COUNT",	SC_DB_COUNT,	0,	0, /* 14 */
			sizeof(i4),	DB_INT_TYPE },

    { "SERVER_CLASS",	SC_SERVER_CLASS,	0,	0, /* 15 */
			sizeof(DB_DB_NAME),	DB_CHR_TYPE },

    { "OPEN_COUNT",	SC_OPENDB_COUNT,	0,	DB_CHR_TYPE, /* 15 */
			sizeof(i4),	DB_INT_TYPE },

    { "DBMS_CPU",	SCI_DBCPUTIME,	0,	0, /* 16 */
			sizeof(i4),	DB_INT_TYPE },

    { "DBMS_DIO",	SCI_DBDIOCNT,	0,	0, /* 17 */
			sizeof(i4),	DB_INT_TYPE },

    { "DBMS_BIO",	SCI_DBBIOCNT,	0,	0, /* 18 */
			sizeof(i4),	DB_INT_TYPE },

    { "GROUP",	SCI_GROUP,	0,	0, /* 19 */
				sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "ROLE",	SCI_APLID,	0,	0, /* 20 */
				sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "QUERY_IO_LIMIT",		SCI_QDIO_LIMIT,	0,	0, /* 21 */
				sizeof(i4),	DB_INT_TYPE },

    { "QUERY_ROW_LIMIT",	SCI_QROW_LIMIT,	0,	0, /* 22 */
				sizeof(i4),	DB_INT_TYPE },

    { "QUERY_CPU_LIMIT",	SCI_QCPU_LIMIT,	0,	0, /* 23 */
				sizeof(i4),	DB_INT_TYPE },

    { "QUERY_PAGE_LIMIT",	SCI_QPAGE_LIMIT,	0,	0,/* 24 */
				sizeof(i4),	DB_INT_TYPE },

    { "QUERY_COST_LIMIT",	SCI_QCOST_LIMIT,	0,	0, /* 25 */
				sizeof(i4),	DB_INT_TYPE },

    { "DB_PRIVILEGES",		SCI_DBPRIV,	0,	0, /* 26 */
				sizeof(i4),	DB_INT_TYPE },

    { "DATATYPE_MAJOR_LEVEL",	SC_MAJOR_LEVEL,	0,	0, /* 27 */
				sizeof(Sc_main_cb->sc_major_adf),
				DB_INT_TYPE },

    { "DATATYPE_MINOR_LEVEL",	SC_MINOR_LEVEL,	0,	0, /* 28 */
				sizeof(Sc_main_cb->sc_minor_adf),
				DB_INT_TYPE },

    { "COLLATION",	SC_COLLATION,	0,	0,
	sizeof(Sc_main_cb->sc_proxy_scb->scb_sscb.sscb_ics.ics_collation),
			DB_CHR_TYPE },

    { "UCOLLATION",	SC_UCOLLATION,	0,	0,
	sizeof(Sc_main_cb->sc_proxy_scb->scb_sscb.sscb_ics.ics_ucollation),
			DB_CHR_TYPE },

    { "LANGUAGE",	SC_LANGUAGE,	0,	0,
	  		ER_MAX_LANGSTR,	DB_CHR_TYPE },

    { "CREATE_TABLE",	SCI_TAB_CREATE,	0,	0,	1,	DB_CHR_TYPE },

    { "CREATE_PROCEDURE",	SCI_PROC_CREATE,	0,	0,	
				1,	DB_CHR_TYPE },

    { "LOCKMODE",	SCI_SET_LOCKMODE,	0,	0,	
			1,	DB_CHR_TYPE },

    /* "MAXQUERY" is an alias for "MAXIO" */

    { "MAXQUERY",	SCI_MXIO,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "MAXIO",		SCI_MXIO,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "MAXROW",		SCI_MXROW,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "MAXCPU",		SCI_MXCPU,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "MAXPAGE",	SCI_MXPAGE,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "MAXCOST",	SCI_MXCOST,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "MAXCONNECT",	SCI_MXCONNECT,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "MAXIDLE",	SCI_MXIDLE,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "QUERY_FLATTEN",	SCI_FLATTEN,	0,	0,	1,	DB_CHR_TYPE },

    { "DB_ADMIN",	SCI_DB_ADMIN,	0,	0,	1,	DB_CHR_TYPE },

    { "UPDATE_SYSCAT",	SCI_UPSYSCAT,	0,	0,	1,	DB_CHR_TYPE },

# ifdef LP64
    { "SESSION_ID",	SCI_SESSID,	0,	0,	16,	DB_CHR_TYPE },
# else
    { "SESSION_ID",	SCI_SESSID,	0,	0,	8,	DB_CHR_TYPE },
# endif  /* LP64 */

    { "INITIAL_USER",	SCI_INITIAL_USER,	0,	0,
			sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "SESSION_USER",	SCI_SESSION_USER,	0,	0,
			sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "SECURITY_PRIV",	SCI_SECURITY,	0,	0,	1,	DB_CHR_TYPE },
 
    { "SYSTEM_USER",	SCI_RUSERNAME,	0,	0,
			sizeof(DB_OWN_NAME),	DB_CHR_TYPE },

    { "ON_ERROR_STATE",	SC_ONERROR_STATE,	0,	0,
			DB_TYPE_MAXLEN,			DB_CHR_TYPE },

    { "ON_ERROR_SAVEPOINT",	SC_SVPT_ONERROR,	0,	0,
			DB_TYPE_MAXLEN,			DB_CHR_TYPE },

    { "SECURITY_AUDIT_LOG",	SCI_AUDIT_LOG,	0,	0,
			DB_TYPE_MAXLEN,			DB_CHR_TYPE },

    { "IMA_VNODE",	SCI_IMA_VNODE,	0,	0,	
			DB_NODE_MAXNAME*2,		DB_CHR_TYPE },

    { "IMA_SERVER",	SCI_IMA_SERVER,	0,	0,	
			DB_NODE_MAXNAME*2,		DB_CHR_TYPE },

    { "IMA_SESSION",	SCI_IMA_SESSION,	0,	0,	
			DB_TYPE_MAXLEN,		DB_CHR_TYPE },

    { "SECURITY_PRIV",	SCI_SECURITY,	0,	0,
    			1,		DB_CHR_TYPE },

    { "CURSOR_UPDATE_MODE",	SCI_CSRUPDATE,	0,	0,
    			8,		DB_CHR_TYPE },

    { "CURSOR_DEFAULT_MODE", SCI_CSRDEFMODE,	0,	0,
    			8,		DB_CHR_TYPE },
    { "CURSOR_LIMIT",	SCI_CSRLIMIT,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "QUERY_FLATTEN",	SCI_FLATTEN,	0,	0,	1,	DB_CHR_TYPE },

    { "FLATTEN_AGGREGATE",	SCI_FLATAGG,	0,	0,
    			1,		DB_CHR_TYPE },

    { "FLATTEN_SINGLETON",	SCI_FLATSING,	0,	0,
    			1,		DB_CHR_TYPE },

    { "CARDINALITY_CHECK",	SCI_FLATCHKCARD, 0,	0,
			1,		DB_CHR_TYPE },

    { "DB_NAME_CASE",		SCI_NAME_CASE,	0,	0,
    			5,		DB_CHR_TYPE },

    { "DB_DELIMITED_CASE",	SCI_DELIM_CASE,	0,	0,
    			5,		DB_CHR_TYPE },
    
    { "UPDATE_ROWCNT",		SC_ROWCNT_INFO, 0, 	0,
			DB_TYPE_MAXLEN,	DB_CHR_TYPE },

    { "DB_TRAN_ID",		SC_TRAN_ID, 0,  	0,
			DB_TYPE_MAXLEN,	DB_CHR_TYPE },

    { "DB_CLUSTER_NODE",	SC_CLUSTER_NODE, 0, 	0,
			DB_NODE_MAXNAME, DB_CHR_TYPE },
	
    { "MAX_PRIV_MASK",		SCI_MAXUSTAT, 0, 	0,
			sizeof(i4),	DB_INT_TYPE },

    { "CURRENT_PRIV_MASK",	SCI_USTAT, 0, 	0,
			sizeof(i4),	DB_INT_TYPE },

    { "SELECT_SYSCAT",	SCI_QRYSYSCAT,	0,	0,	
			1,	DB_CHR_TYPE },

    { "TABLE_STATISTICS",SCI_TABLESTATS,	0,	0,	
			1,	DB_CHR_TYPE },

    { "CONNECT_TIME_LIMIT",	SCI_CONNECT_LIMIT,	0,	0,	
			sizeof(i4),	DB_INT_TYPE },

    { "IDLE_TIME_LIMIT",	SCI_IDLE_LIMIT,	0,	0,	
			sizeof(i4),	DB_INT_TYPE },

    { "SESSION_PRIORITY_LIMIT",	SCI_PRIORITY_LIMIT,	0,	0,	
			sizeof(i4),	DB_INT_TYPE },

    { "SESSION_PRIORITY",	SCI_CUR_PRIORITY,	0,	0,	
			sizeof(i4),	DB_INT_TYPE },

    { "SECURITY_AUDIT_STATE",	SCI_AUDIT_STATE,	0,	0,
			sizeof(DB_NAME),	DB_CHR_TYPE },

    { "DB_REAL_USER_CASE",	SCI_REAL_USER_CASE,	0,	0,
    			5,	DB_CHR_TYPE },
	
    { "MAX_TUP_LEN",	SCI_MXRECLEN,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "MAX_PAGE_SIZE",	SCI_MXPAGESZ,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "PAGE_SIZE_2K",	SCI_PAGE_2K,	0,	0,
			1,			DB_CHR_TYPE },

    { "TUP_LEN_2K",	SCI_RECLEN_2K,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "PAGE_SIZE_4K",	SCI_PAGE_4K,	0,	0,
			1,			DB_CHR_TYPE },

    { "TUP_LEN_4K",	SCI_RECLEN_4K,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "PAGE_SIZE_8K",	SCI_PAGE_8K,	0,	0,
			1,			DB_CHR_TYPE },

    { "TUP_LEN_8K",	SCI_RECLEN_8K,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "PAGE_SIZE_16K",	SCI_PAGE_16K,	0,	0,
			1,			DB_CHR_TYPE },

    { "TUP_LEN_16K",	SCI_RECLEN_16K,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "PAGE_SIZE_32K",	SCI_PAGE_32K,	0,	0,
			1,			DB_CHR_TYPE },

    { "TUP_LEN_32K",	SCI_RECLEN_32K,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "PAGE_SIZE_64K",	SCI_PAGE_64K,	0,	0,
			1,			DB_CHR_TYPE },

    { "TUP_LEN_64K",	SCI_RECLEN_64K,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "TIMEOUT_ABORT",	SCI_TIMEOUT_ABORT,	0,	0,
			1,			DB_CHR_TYPE },

    { "PAGETYPE_V1",	SCI_PAGETYPE_V1,	0,	0,
			1,			DB_CHR_TYPE },

    { "PAGETYPE_V2",	SCI_PAGETYPE_V2,	0,	0,
			1,			DB_CHR_TYPE },

    { "PAGETYPE_V3",	SCI_PAGETYPE_V3,	0,	0,
			1,			DB_CHR_TYPE },

    { "PAGETYPE_V4",	SCI_PAGETYPE_V4,	0,	0,
			1,			DB_CHR_TYPE },

    { "UNICODE_LEVEL",	SCI_UNICODE,	0,	0,
			sizeof(i4),	DB_INT_TYPE },

    { "UNICODE_NORMALIZATION",		SCI_UNICODE_NORM,	0,	0,
    			3,		DB_CHR_TYPE },

    { "LP64",		SCI_LP64,		0,	0,
			1,	DB_CHR_TYPE },

    { "DATE_TYPE_ALIAS", SCI_DTTYPE_ALIAS,	0,	0,
    			10,		DB_CHR_TYPE },
    { "DATE_FORMAT", SCI_DATE_FORMAT,	0,	0,
    			14 /* 14 = strlen("multinational4") */,	DB_CHR_TYPE },
    { "MONEY_FORMAT", SCI_MONEY_FORMAT,	0,	0,
    			10,		DB_CHR_TYPE },
    { "MONEY_PREC", SCI_MONEY_PREC,	0,	0,
    			4,		DB_CHR_TYPE },
    { "DECIMAL_FORMAT", SCI_DECIMAL_FORMAT,	0,	0,
    			2,		DB_CHR_TYPE },
    { "CHARSET",	SC_CHARSET,		0,	0,
    			8,		DB_CHR_TYPE },
    { "CACHE_DYNAMIC",	SC_DB_CACHEDYN,		0,	0,
    			1,		DB_CHR_TYPE },
    { "PAGETYPE_V5",	SCI_PAGETYPE_V5,	0,	0,
			1,			DB_CHR_TYPE },
    { "PAGETYPE_V6",	SCI_PAGETYPE_V6,	0,	0,
			1,			DB_CHR_TYPE },
    { "PAGETYPE_V7",	SCI_PAGETYPE_V7,	0,	0,
			1,			DB_CHR_TYPE },

};


#define NUM_DBMSINFO_REQUESTS \
	( sizeof(dbms_info_items) / sizeof(dbms_info_items[0]) )


GLOBALREF const char	Version[];


/*{
** Name: scd_initiate	- start up the INGRES server
**
** Description:
**      This routine initiates the DBMS Server.  The main control structure 
**      of SCF is initialized.  A message is logged indicating and attempt 
**      to start the server.  All internal SCF state is initialized. 
**      The client facilities are initialized.  Finally, a message is logged 
**      to indicate successful startup.  If any errors are encountered whilest 
**      attempting to start the server, the errors are logged, and an unsuccessful 
**      startup message is logged.
**
** Inputs:
**      csib                            CS_INFO_CB -- information about
**					user requested server configurations
**
** Outputs:
**      None
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-July-1986 (fred)
**          Created on Jupiter.
**	15-Jun-1987 (rogerk)
**	    Parse Fast Commit option and start fast commit thread.
**	    Moved parsing of CS options up to before initializing SCF and GCF.
**	15-sep-1988 (rogerk)
**	    Parse Write Behind option and start write behind threads.
**	26-sep-1988 (rogerk)
**	    Save the min,max priority fields in the csib.
**	    Increased Fast Commit thread's priority.
**	24-oct-1988 (rogerk)
**	    Added server CPU, DIO and BIO dbms_info calls.
**      3-nov-88 (eric)
**          added support for the opf_ddbsite field of the OPF cb at startup.
**	24-jan-1989 (mikem)
**	    CSadd_thread() takes a (CL_SYS_ERR *), not a (DB_STATUS *).
**	 6-feb-1989 (rogerk)
**	    Took out requirement that sole_server be specified with fast commit.
**	    Start fast commit thread if shared cache is specified.
**	    Parse SOLECACHE, SHAREDCACHE, CACHENAME, DBCACHE_SIZE and
**	    TBLCACHE_SIZE options.
**      28-MAr-1989 (fred)
**          Added udadt support -- added server init thread
**	    which does all the work...
**	17-apr-89 (seputis)
**	    init opf_smask to 0
**	20-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Add dbmsinfo requests GROUP_ID, APPLICATION_ID, QUERY_IO_LIMIT,
**	    QUERY_ROW_LIMIT, QUERY_CPU_LIMIT, QUERY_PAGE_LIMIT,
**	    QUERY_COST_LIMIT, and DB_PRIVILEGES.
**	06-may-89 (ralph)
**	    GRANT Enhancements, Phase 2b:
**	    Changed APPLICATION_ID to ROLE;
**	    Changed GROUP_ID to GROUP.
**	12-may-1989 (anton)
**	    added collation dbmsinfo
**	18-may-1989 (neil)
**	    Rules: Added RULE_DEPTH initialization.
**	21-jun-89 (ralph)
**	    Fixed CREATE_PROCEDURE dbmsinfo request
**	22-jun-89 (jrb)
**	    Set sc_capabilities field of Sc_main_cb with CI info.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Add dbmsinfo requests MAXQUERY, MAXIO, MAXROW, MAXCPU,
**	    MAXPAGE, and MAXCOST.
**	 9-Jul-89 (anton)
**	    Fixed dbmsinfo language request
**	28-jul-89 (jrb)
**	    Set status to E_DB_FATAL whenever CI returns non-zero.  Before we
**	    were not detecting this error and allowing the server to come up in
**	    a half-started state.
**      14-jul-89 (jennifer)
**          Added CIcapability check for C2 security.
**	08-sep-89 (paul)
**	    Added support for alerters. Initialize the alert data structures
**	    at server initialization
**	23-sep-1989 (ralph)
**	    Pass OPF_GSUBOPTIMIZE to OPF on startup if /NOFLATTEN specified.
**	10-oct-1989 (ralph)
**	    Pass value specified for OPF_MEMORY to OPF on startup.
**	17-oct-1989 (ralph)
**	    Change sense of /NOFLATTEN.
**	13-dec-1989 (teg)
**	    added logic to initialize RDF_CCB before calling rdf.
**	05-jan-90 (andre)
**	    set cs_fips if user specified /FIPS.
**	29-jan-1990 (sandyh)
**	    Fixed bug 9908 - /dblist option databases not null terminated,
**	    which caused gca_register() to A/V.
**	08-aug-1990 (ralph)
**	    Defined dbmsinfo('secstate') to show security state of the server.
**	    Defined dbmsinfo('update_syscat') to show session's value for this.
**	    Defined dbmsinfo('db_admin') to show session's value for this.
**	    Removed dbmsinfo('opf_memory').
**	    Initialize dmc.dmc_show_state and sc_show_state.
**	    Add support for new RDF flags, including:
**		MEMORY, TBL_COLS, TBL_IDXS, MAX_TBLS, MULTICACHE, TBL_SYNONYMS
**	    Add support for new OPF flags, including:
**		CPUFACTOR, TIMEOUTFACTOR, REPEATFACTOR, SORTMAX
**	    Add support for new DMF flags, including:
**		SCANFACTOR
**	    Add support for new QEF flags, including:
**		SORT_SIZE
**	05-dec-1990 (ralph)
**	    Add comments to ease implementation of new OPF/QEF startup
**	    parameters when support for these parameters is added to OPF.
**	15-jan-1991 (ralph)
**	    Change cso_float from unioned to simple structure member
**	06-feb-1991 (ralph)
**	    Added support for dbmsinfo('session_id')
**	04-feb-1991 (neil)
**	    Alerters: Added /EVENTS initialization.
**	06-Dec-1990 (anton)
**	    Cleanup copying of server name returned by GCA_REGISTER
**	    Pass address of sc0m_semaphore to CS in scib.  This semaphore
**	    is needed to synchronize MEget_pages calls in an MCT server.
**	29-Jan-1991 (anton)
**	    Add support for GCA API 2
**	14-mar-91 (andre)
**	    init psq_flag to 0 before calling psq_startup().
**	01-jul-91 (andre)
**	    Add support for dbmsinfo('session_user'), dbmsinfo('session_group'),
**	    dbmsinfo('session_role'), and dbmsinfo('system_user')
**	24-jan-91 (teresa)
**	    add initialization of rdf_maxddb; rdf_avgldb; rdf_clsnodes;
**	    rdf_netcost; and rdf_nodecost; to NULL for SYBIL.  Eventually these
**	    will be specified from iirundbms/iirunstar and may contain user
**	    specified startup parameters; but the iirundbms/iirunstar logic is
**	    not in the system yet.
**      21-apr-1992 (schang)
**          GW merge
**	    4-nov-1991 (rickh)
**	        At gateway initialization time, write the release identifier
**	        returned by the gateway into the server control block.  This
**	        causes SCF to cough up the gateway identifier when poked with
**	        a "select dbmsinfo( '_version' )"
**      07-apr-1992 (jnash)
**          Add support for locking dmf cache via the dmf.lock_cache
**	    server startup param and the corresponding CSO_LOCK_CACHE case.
**	24-jan-91 (teresa)
**	    add initialization of rdf_maxddb; rdf_abgldb; rdf_clsnodes;
**	    rdf_netcost; and rdf_nodecost; to NULL for SYBIL. Eventually these
**	    will be specified from iirundbms/iirunstar and may contain user
**	    specified startup parameters; but the iirundbms/iirunstar logic is
**	    not in the system yet.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              22-Aug-1988 (alexh)
**                  Add sc_capabilities initialization to distinguish backend
**                  only, star-only, backend-star servers. RDF and QEF startups
**                  require this distinction. DMF is not started if back-end
**                  capability is not specified.
**              18-Jan-1989 (alexh)
**                  Set rdf_maxsessions during RDF startup.
**              23-feb-1989 (georgeg)
**                  added name server support.
**              14-mar-1989 (georgeg)
**                  added trce point sc901, sc907 trace points for Alpha.
**              28-nov-1989 (georgeg)
**                  Added support fot STAR 2PC, a thread is started up to
**                  recover all the orphan DX's in the installation.
**              02-nov-1990 (georgeg)
**                  Added support for CSO_NOCLUSTER, CSO_DX_NORECOVR, the will 
**		    not retrieve cluster information if set and the second will
**		    not starrt the destributed transaction recovery thread.
**                  Also if  CSO_DX_NORECOVR is set do not start the dx 
**		    recovery thread.
**          End of STAR comments.
**	28-jul-92 (pholman)
**	    Replaced use of magic number with NUM_DBMSINFO_REQUESTS, and
**	    added new request SECURITY_AUDIT_LOG, for C2 project.
**	28-jul-1992 (fpang)
**	    Added support new rdf startup flags, rdf.ddbs, 
**	    rdf.avg_ldbs, rdf.cluster_nodes, rdf.netcost, rdf.nodecost. 
**	    See Teresa's SYBIL RDF Design Changes for more details.
**      29-jul-1992 (fpang)
**          Set sc_rdbytes to rdf_sesize, since RDF now has a session control 
**	    block, and is returning it's size.
**      04-sep-1992 (fpang)
**          Set protocol level of distributed server at GCA_PROTOCOL_40.
**      18-sep-92 (ed)
**          Correct NOAGGFLAT test
**	22-sep-1992 (bryanp)
**	    Added support for CSO_RECOVERY option and recovery thread.
**	    Added support for CSO_LOGWRITER to indicate no. of logwriters.
**	    Added code for starting the logging & locking special threads.
**	25-sep-92 (markg)
**	    Add code to initialize SXF immediately after initializing
**	    DMF.
**	08-sep-1992 (fpang)
**	    If DDBS was not specified during server startup, set
**	    rdf_ccb.rdf_maxddb to number of users instead.
**	01-oct-1992 (fpang)
**	    Star now knows how to convert GCA_PROTOCOL_40 copy maps
**	    to GCA_PROTOCOL_50 copy maps, so remove protocol limitaion.
**	7-oct-1992 (bryanp)
**	    Make -recovery imply -nonames. Record -recovery in sc_capabilities.
**	26-oct-1992 (rogerk)
**	    6.5 Recovery Project: Changed actions associated with some of
**	    the Consistency Point protocols. The system now requires
**	    that all servers participate in Consistency Point flushes, not
**	    just fast commit servers.  The name of the Fast Commit Thread 
**	    was changed to the Consistency Point Thread and is now started
**	    in all dbms servers.
**	30-oct-1992 (robf)
**	    Add SECURITY_PRIV DBMSINFO request.
**	05-nov-1992 (markg)
**	    Initialize the sc_show_state function pointer to contain the
**	    address of sxf_call. This is used to get around some
**	    shared image linking problems on VMS.
**	02-dec-92 (andre)
**	    removed support for dbmsinfo('session_role') and
**	    dbmsinfo('session_group'); added support for
**	    dbmsinfo('initial_user')
**	03-dec-1992 (pholman)
**	    C2: use PM to see if this server is to run with C2 security
**	    auditing instead of consulting CI.
**	23-nov-1992 (fpang)
**	    Don't start Consistency Point Thread if distributed.
**	    Also, start up GWF and DMF so OPF can call sort cost routines.
**	23-Oct-1992 (daveb)
**	    Name semaphores for IMA
**	23-Nov-1992 (daveb)
**	    Abstract DBMS_INFO/ADF init into a table driven by a loop rather
**	    than 250+ lines of in-line goop.
**	24-Nov-1992 (daveb)
**	    Enable GCA_API_LEVEL_4 (from 2) to support GCM.
**	1-Dec-1992 (daveb)
**	    force server class to GCA_IUSVR_CLASS for RCP, and don't force
**	    nonames for it.  If he doesn't specify -nonames, then it's
**	    a managable server in the "IUSVR" class.
**	26-nov-1992 (markg)
**	    Added support for the new audit thread.
**	03-dec-1992 (pholman)
**	    C2: use PM to see if this server is to run with C2 security
**	    auditing instead of consulting CI.
**	29-dec-1992 (robf)
**	    Add check to ensure MODE is ON or OFF rather than ON or <other>
**	    Use STcasecmp for C2 mode to eliminate temp. variable and loop.
**	18-jan-1993 (bryanp)
**	    Allow timer-initiated consistency points.
**	17-jan-1993 (ralph)
**	    FIPS: Added support for new server startup parms and dbmsinfo() reqs
**	1-feb-1993 (markg)
**	    Added support for CSO_SXF_MEM startup parameter, also disallowed
**	    C2 auditing in Star server.
**	4-mar-1993 (robf)
**	    Start DMF & SXF for STAR servers
**	9-Mar-1993 (daveb)
**	    Somebody took out the change that let the recovery process register
**	    for IMA if -names was set.  Remove code that turned off registration
**	    in the recovery process, and explicitly default it to "no names"
**	    as long as no one sets -names.
**	7-apr-1993 (robf)
**	     Set GCA protocol level to 61 for new incoming agent connections
**	25-may-1993 (robf)
**	     Pass B1 server flag down to other facilities.
**	07-may-93 (anitap)
**	    Added support for dbmsinfo for "SET UPDATE_ROWCOUNT" statement.
**	    Fixed compiler warning.
**	2-Jul-1993 (daveb)
**	    prototyped.  Use proper PTR for allocation
**	26-jul-1993 (bryanp)
**	    If /GC_INTERVAL is 0, don't start a group commit thread.
**	    Changed node-name handling in ule_initiate call: formerly we were
**		passing a fixed-length node name, which might be null-padded
**		or blank-padded at the end. This could lead to garbage in the
**		node name portion of the error log lines, or, when we use node
**		names to generate trace log filenames, to garbage file names.
**		Now we use a null-terminated node name, from LGCn_name().
**	23-aug-1993 (bryanp)
**	    If this is the star server, tell DMF so that it can do only a
**		minimal startup.
**	20-sep-1993 (bryanp)
**	    If this is the star server, don't call sca_check.
**      20-sep-1993 (stevet)
**          Added E_SC033D_TZNAME_INVALID message for startup failure due
**          to invalid timezone settings. 
**	26-Jul-1993 (daveb)
**	    Setup sc_max_conns, and CSalter the CS scnt past it to keep
**	    CS from rejecting connections too soon.  Currently reserve 2
**	    sessions for this (SCD_RESERVED_CONNS), but configure
**	    or PM-ize it later.
**	19-oct-93 (robf)
**          No SXF in STAR yet (needs more DMF support)
**	    Add sc_max/min_usr_priority support.
**	    Start terminator thread.
**	10-jan-94 (robf)
**          Rework secure options with new scd_options() interface.
**	21-jan-94 (iyer)
**	   Add support for dbmsinfo requests 'db_tran_id' and 'db_cluster_node'
**	   Save cluster node name in SC_MAIN_CB.sc_cname.
**	11-feb-94 (robf)
**          Don't start the terminatior thread in the RCP, not needed
**	    currently.
**	14-feb-94 (rblumer)
**	   As part of cursor and flattening changes, take out 'default flatten'
**	   setting; that is already handled via the sc_noflatten field setting.
**	3-mar-94 (robf)
**         Added check for B1 authorization string bit, ifdef'd until
**	   mainline support completed by jpk.
**	14-apr-94 (robf)
**         Add check for security audit writer thread.
**	14-apr-1994 (fpang)
**	   Allow ii.*.dbms.*.database_list to be a comma separated list.
**	   Fixes B57557, ii.*.dbms.*.database_list fails if comma separated.
**	9-may-94 (robf)
**         b1_server->b1_secure_server for consistency with rest of code.
**      10-May-1994 (daveb) 62631
**          Don't register STAR server as GCA_RG_INSTALL -- it doesn't
**  	    have LG/LK stats, so it isn't really installation-capable
**  	    for IMA purposes.  Only the RCP and a DBMS are.
**	23-may-1995 (reijo01)
**	    Added a new field to hold the startup time. This is used as a 
**	    managed object.
**	24-apr-1995 (cohmi01)
**	    Added IOmaster server type, readahead and writealong threads.
**	25-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	30-May-1995 (jenjo02)
**	    Changed gca_cb.gca_p_semaphore = CSp_semaphore; 
**	            gca_cb.gca_v_semaphore = CSv_semaphore; assignments
**	         to gca_cb.gca_p_semaphore = CSp_semaphore_function;
**                  gca_cb.gca_v_semaphore = CSv_semaphore_function;
**	    to distinguish the functions from the macros.
**	2-June-1995 (cohmi01)
**	    Pass down #readahead threads to DMC_START_SERVER.
**	19-sep-1995 (nick)
**	    Start event thread at a lower priority.  We wouldn't have to do
**	    this if we allowed threads to gain/lose priority based upon
**	    the load they impose.
**	14-nov-1995 (nick)
**	    Use GCA_PROTOCOL_LEVEL_62.  Get II_DATE_CENTURY_BOUNDARY.
**	23-Feb-96 (gordy)
**	    Conditionalize the usage of GCA_BATCHMODE on its existence.
**	    GCA may not support this flag even though it is supported in CL.
**	06-mar-1996 (nanpr01 & stial01)
**	    Call DMC_SHOW to get the maximum tuple size for the installation
**	    and initialize the Sc_main_cb->sc_maxtup field.
**          Increase size of dca array from 40 to 100, to accomodate the
**          additonal cache buffer paramaters. Add sanity check to make sure 
**          dca_index is less than size of dca array.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	04-apr-1996 (loera01)
**	    Added support for compression of varying-length datatypes:
**	    if appropriate flag retrieved from the configuration
**	    data file, set capabilities mask to support compression. 
**	3-jun-96 (stephenb)
**	    Add code to create replicator queue management threads
**      03-jun-1996 (canor01)
**          OS-threads version of LOingpath no longer needs protection by
**          sc_loi_semaphore.
**	10-dec-96 (stephenb)
**	    set qsf_qsscb_offset before starting QSF.
**      27-feb-97 (stial01)
**          if parms indicated SINGLE SERVER mode, let dmf know
**	14-mar-97 (inkdo01)
**	    Added opf_spat_key to assign default selectivity to spatial preds.
**	24-apr-97 (nanpr01)
**	    Just being conservative. Turn off the compression if 
**	    vch_compression is not ON.
**	14-may-1997 (canor01)
**	    Increment dbms_tasks for recovery server to account for
**	    deadlock detection thread.
**	03-Jul-97 (radve01)
**	    Several info about main dynamic storage structures transferred
**	    to the server control block.
**	27-Jun-1997 (jenjo02)
**	    Removed sc_urdy_semaphore, which served no useful pupose.
**      24-jul-1997 (stial01)
**          Init gw_rcb.gwr_gchdr_size before gwf_call( GWF_INIT...
**	01-oct-1997 (nanpr01)
**	    Initialize the node to the local_vnode before calling
**	    ule_initiate so that the nodename is printed in errlog.log.
**	    However this will print in uppercase as it is stored in
**	    local_vnode. In eroptlog.c calls GChostname which returns
**	    lower case, however PMgetDefaults(1) returns in upper case.
**      02-Apr-98 (fanra01)
**          Limit the maximum sessions to 8 for the evaluation release.
**	01-Apr-1998 (jenjo02)
**	    SCD_MCB renamed to SCS_MCB.
**	03-Apr-1998 (jenjo02)
**	    Call buffer manager to start the WriteBehind threads.
**	09-Apr-1998 (jenjo02)
**	    Set SC_IS_MT flag in sc_capabilities if OS threads.
**	06-may-1998 (canor01)
**	    Workgroup edition and developers edition have connection 
**	    limits that cannot be overridden.  Start license thread.
**	30-jun-98 (stephenb)
**	    Add call to initialize cut.
**	02-Sep-1998 (loera01) Bug 92957
**	   Make sure replicator threads are accounted for in the internal
**	   thread count.  
**	17-jan-1999 (nanpr01)
**	   Initialize the dbms_task to 6 till the time Jon cleans up cs code
**	   to use the max and active thread counter consistently.
**	09-jun-1999 (somsa01)
**	    After a successful startup, also write E_SC051D_LOAD_CONFIG to
**	    the errlog.
**	01-jul-1999 (somsa01)
**	    We now also print out the server type when printing out
**	    E_SC051D_LOAD_CONFIG.
**      15-Mar-1999 (hanal04)
**         Compare user_threads to csib->cs_scnt NOT cscb->cs_scnt as the
**         cscb is a local uninitialised variable. b95841.
**      07-Jul-1999 (hanal04)
**         Correct structure element to pointer reference in last
**         change. 
**	10-Mar-2000 (wanfr01)
**	    Bug 100847, INGSRV 1127:
**	    Add sc_assoc_request field to allow timing out sc_get_assoc
**	    requests if someone is blocking the port.
**	26-oct-00 (devjo01)
**	    Do not initiate deadlock detection thread if running in an
**	    Ingres cluster installation.  All deadlock detection is
**	    done in the CSP.  (b103172).
**	29-nov-00 (hayke02)
**	    Restore 09-nov-99 (toumi01) and 10-Mar-2000 (wanfr01) changes
**	    which were removed by the 26-oct-00 (devjo01) change (oping20
**	    changes 444214, 445981 and 448207 respectively). This change
**	    fixes bug 103367.
**	09-nov-99 (toumi01)
**	    Per John Ainsworth, change maximum workgroup connections from
**	    25 to 250.
**	19-apr-00 (hanje04)
**	    Change to exempt int_lnx from 250 connect limit for WG edition. 
**	    Makes above toumi01 change obselete.
**      10-Mar-2000 (wanfr01)
**          Bug 100847, INGSRV 1127:
**          Add sc_assoc_request field to allow timing out sc_get_assoc
**          requests if someone is blocking the port.
**	15-Jun-2000 (hanje04)
**	    Add ibm_lnx to Linux WG edition user limit exception
**	21-jul-00 (inkdo01)
**	    Init. new parm psqcb.psq_maxmemf for OPF style memory mgmt in PSF.
**	30-aug-00 (inkdo01)
**	    Init opfcb.opf_rangep_max.
**	17-oct-2000 (somsa01)
**	    When setting up the message header for STAR, make sure we look
**	    at the right sc_sname entry for MCS servers.
**	3-nov-00 (inkdo01)
**	    Init opf_cb.opf_cstimeout.
**      28-Dec-2000 (horda03) Bug 103596 INGSRV 1349
**          Initialise sc_event_priority used to assign the priority of
**          the Event thread.
**	18-May-01 (gordy)
**	    Bumping GCA protocol level to 64 for national character sets.
**	12-jun-2001 (devjo01)
**	    UNIX cluster support s103715.  Move ule_initialize
**	    further up, so early errors have correct format.
**	19-mar-2002 (somsa01)
**	    For EVALUATION_RELEASE, bumped Sc_main_cb->sc_max_conns from 8
**	    to 32 to accomodate extra connections from the ICESVR and Portal.
**	22-jul-02 (hayke02)
**	    Initialize new parameter opf_stats_nostats_factor to 1.0. This
**	    change fixes bug 108327.
**	6-feb-03 (inkdo01)
**	    Init opf_smask to OPF_HASH_JOIN to turn hash joins on by default.
**	    Next release, the flag should probably be removed altogether.
**	16-jul-03 (hayke02)
**	    Initialise and set psq_vch_prec for config parameter psf_vch_prec.
**	    This change fixes bug 109134.
**      20-aug-03 (chash01) change RMS gateway license to be CI_NEW_RMS_GATEWAY
**          (aka 2SCI)
**	10-nov-03 (inkdo01)
**	    Init opf_pq_dop, opf_pq_threshold for parallel query processing.
**	25-jan-2004 (devjo01)
**	    Don't let server start if configured for clustering, but
**	    binary was not built with cluster support.
**	20-Apr-04 (gordy)
**	    Bumping GCA protocol level to 65 for bigints.
**	5-may-04 (inkdo01)
**	    Change default opf_pq_dop to 1 to disable parallel queries by
**	    default.
**	19-aug-2004 (thaju02)
**	    Check for overflow of qef_dsh_maxmem and qef_sorthash_memory.
**      31-aug-2004 (sheco02)
**          X-integrate change 466442 to main.
**	03-nov-2004 (thaju02)
**	    Changed size to SIZE_TYPE. Modified overflow checking
**	    for qef_dsh_maxmem and qef_sorthash_memory.
**  18-Dec-2004 (shaha03)
**		bug # 113676, initialized psq_flag2 flag with zero.
**	19-jan-05 (inkdo01)
**	    Added opf_pq_partthreads to define max threads per partitioned
**	    table/join in parallel query.
**	9-Sep-2005 (schka24)
**	    For Linux, announce NPTL-ness in the trace log, since I just
**	    wasted a day building Ingres the wrong way, twice.
**	24-Oct-2005 (schka24)
**	    Init a default for QEF max memory delay.
**	23-Nov-2005 (kschendel)
**	    Init result structure for opf.
**	 6-Jul-06 (gordy)
**	    Bumping GCA protocol level to 66 for ANSI date/time and
**	    associated protocol level changes.
**	 3-Nov-06 (gordy)
**	    Bumping GCA protocol level to 67 for LOB Locators.
**	6-Mar-2007 (kschendel) SIR 122512
**	    Init new QEF hash things to zero, asks for default.
**      6-jun-2007 (huazh01)
**          add qef_nodep_chk for the config
**          parameter which switches ON/OFF the fix to b112978.
**	30-oct-08 (hayke02)
**	    Initialise opf_cb.opf_greedy_factor for config parameter
**	    opf_greedy_factor. This change fixes bug 121159.
**	3-jun-2009 (wanfr01)
**	    Bug 122134
**	    Do not set SC_OPERATIONAL until after printing out
**	    startup messages.  SC_OPERATIONAL will cause SCD_SCB
**	    fields (which are uninitialized during startup) to
**	    be accessed.
**	03-Aug-2009 (thaju02) (B122401)
**	    Increase dmc.dmc_char_array.data_out_size to account for 
**	    pagetype_v5. 
**	 1-Oct-09 (gordy)
**	    Bumping GCA protocol level to 68 for long procedure names.
**	12-Nov-2009 (kschendel) SIR 122882
**	    Change cmptlvl into an integer.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add DMC_C_PAGETYPE_V6, DMC_C_PAGETYPE_V7
**      15-feb-2010 (maspa05) SIR 123293
**          Pass server_class to PSF so we can output it in SC930 trace
**      29-apr-2010 (Stephenb)
**          Init sc_batch_copy_optim.
*/
DB_STATUS
scd_initiate( CS_INFO_CB  *csib )
{
    DB_STATUS           status;
    DB_STATUS		error;
    CL_SYS_ERR	        sys_err;
    STATUS		stat;
    SIZE_TYPE		size;
    i4			cluster = 0;
    i4			i;
    i4			num_databases = 0;
    i4			csib_idx_databases = -1;
    i4			dca_index;
    SCS_MCB		*mcb_head;
    SCV_LOC		*dbdb_loc;
    ADF_TAB_DBMSINFO	*dbinfo_block;
    DMC_CB              dmc;
    RDF_CCB		rdf_ccb;
    PSQ_CB		psq_cb;
    OPF_CB		opf_cb;
    QEF_SRCB		qef_cb;
    QSF_RCB		qsf_rcb;
    DMC_CHAR_ENTRY      dca[150];
    DMC_CHAR_ENTRY      dmc_char[DB_MAX_CACHE + DB_MAX_PGTYPE];
    SCF_CB		scf_cb;
    GCA_IN_PARMS	gca_cb;
    GCA_RG_PARMS	gcr_cb;
    SCD_CB		scd_cb;
    LOCATION		dbloc;
    char		loc_buf[MAX_LOC];
    char		*dbplace;
    char		node[CX_MAX_NODE_NAME_LEN+2];
    GCA_LS_PARMS	local_crb;
    CS_CB		cscb;
    i4			user_threads;
    i4			dbms_tasks;
    i4			num_dmf_sessions;
    i4			num_psf_sessions;
    i4			num_qef_sessions;
    i4			num_qsf_sessions;
    i4			num_opf_sessions;
    i4			priority;
    i4			sharedcache = FALSE;
    PTR                 tz_cb;  /* pointer to default TZ structure */
    GW_RCB              gw_rcb; /* GWF call control block */
    DB_STATUS           dmf_call();
    SXF_RCB		sxf_rcb;
    char		*sec_mode;
    i4			sxf_memory = 0;
    PTR			block;
    bool		term_thread=FALSE;
    bool		secaudit_writer=FALSE;
    i4		pagesize;
    char		*compress_on_off;
   
    if (Sc_main_cb)
	return(E_SC0100_MULTIPLE_MEM_INIT);

    /* Init bootstrap iidbdb tuple */
    STmove("iidbdb", ' ', 
	sizeof(dbdb_dbtuple.du_dbname), (PTR)&dbdb_dbtuple.du_dbname);
    STmove("$ingres", ' ', sizeof(dbdb_dbtuple.du_own), (PTR)&dbdb_dbtuple.du_own);
    STmove("iidatabase", ' ', 
	sizeof(dbdb_dbtuple.du_dbloc), (PTR)&dbdb_dbtuple.du_dbloc);
    STmove("ii_checkpoint", ' ', 
	sizeof(dbdb_dbtuple.du_ckploc), (PTR)&dbdb_dbtuple.du_ckploc);
    STmove("ii_journal", ' ', 
	sizeof(dbdb_dbtuple.du_jnlloc), (PTR)&dbdb_dbtuple.du_jnlloc);
    STmove("ii_work", ' ', 
	sizeof(dbdb_dbtuple.du_workloc), (PTR)&dbdb_dbtuple.du_workloc);
    dbdb_dbtuple.du_access = (DU_GLOBAL | DU_OPERATIVE);
    dbdb_dbtuple.du_dbservice = 0;
    dbdb_dbtuple.du_dbcmptlvl = DU_CUR_DBCMPTLVL;
    dbdb_dbtuple.du_1dbcmptminor = DU_1CUR_DBCMPTMINOR;
    dbdb_dbtuple.du_dbid = 1;
    STmove("ii_dump", ' ', 
	sizeof(dbdb_dbtuple.du_dmploc), (PTR)&dbdb_dbtuple.du_dmploc);
    /* du_extra left blank */
    MEfill(sizeof(dbdb_dbtuple.du_free), 0, (PTR)&dbdb_dbtuple.du_free);

    status = sc0m_get_pages(SC0M_NOSMPH_MASK,
			NEEDED_SIZE,
			&size,
			&block);
    Sc_main_cb = (SC_MAIN_CB *)block;
    if ((status) || (size != NEEDED_SIZE))
	return(E_SC0204_MEMORY_ALLOC);

    scd_mo_init();

    csib->cs_svcb = (PTR) Sc_main_cb;

    /*
    ** Basic info about dynamic blocks transferred to the server control block
    */
    csib->sc_main_sz = sizeof(SC_MAIN_CB);
    csib->scd_scb_sz = sizeof(SCD_SCB);
    csib->scb_typ_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
				CL_OFFSETOF(SCS_SSCB, sscb_stype);
    csib->scb_fac_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
				CL_OFFSETOF(SCS_SSCB, sscb_cfac);
    csib->scb_usr_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
				CL_OFFSETOF(SCS_SSCB, sscb_ics) +
				CL_OFFSETOF(SCS_ICS, ics_gw_parms);
    csib->scb_qry_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
				CL_OFFSETOF(SCS_SSCB, sscb_ics) +
				CL_OFFSETOF(SCS_ICS, ics_l_qbuf);
    csib->scb_act_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
				CL_OFFSETOF(SCS_SSCB, sscb_ics) +
				CL_OFFSETOF(SCS_ICS, ics_l_act1);
    csib->scb_ast_off = CL_OFFSETOF(SCD_SCB, scb_sscb) +
				CL_OFFSETOF(SCS_SSCB, sscb_astate);

    MEfill(NEEDED_SIZE * SCU_MPAGESIZE, 0, (char *)Sc_main_cb);

    csib->cs_mem_sem = (CS_SEMAPHORE *) &Sc_main_cb->sc_sc0m.sc0m_semaphore;

    PCpid(&Sc_main_cb->sc_pid);
    MEmove(STlength(Version), (PTR) Version, ' ', 
		sizeof(Sc_main_cb->sc_iversion), Sc_main_cb->sc_iversion);
    Sc_main_cb->sc_scf_version = SC_SCF_VERSION;	
    Sc_main_cb->sc_nousers = csib->cs_scnt;
    Sc_main_cb->sc_max_conns = csib->cs_scnt;
    Sc_main_cb->sc_rsrvd_conns = SCD_RESERVED_CONNS;
    Sc_main_cb->sc_listen_mask = (SC_LSN_REGULAR_OK | SC_LSN_SPECIAL_OK);
    Sc_main_cb->sc_irowcount = SC_ROWCOUNT_INITIAL;
    Sc_main_cb->sc_selcnt = 0;
    Sc_main_cb->sc_session_chk_interval=SC_DEF_SESS_CHK_INTERVAL;
    Sc_main_cb->sc_capabilities = Sc_server_type; /* glob from scdmain.c */

    /* Inform all of SCF if this is running with OS threads */
    if (CS_is_mt())
	Sc_main_cb->sc_capabilities |= SC_IS_MT;
    
    /* check to see if running as part of an Ingres cluster */
    cluster = CXcluster_enabled();
    if ( cluster )
    {
# if !defined(conf_CAS_ENABLED) || !defined(conf_CLUSTER_BUILD)
	/*
	** Error, can't have clustering without C&S, plus cluster build.
	*/
	sc0ePut(NULL, E_SC0394_CLUSTER_NOT_SUPPORTED, NULL, 0);
	return (E_SC0124_SERVER_INITIATE);
# endif

	if ( SC_RECOVERY_SERVER == Sc_server_type )
	{
	    /* Remember that RCP has CSP responsibilities */
	    CXalter( CX_A_CSP_ROLE, (PTR)1 );
	    Sc_main_cb->sc_capabilities |= SC_CSP_SERVER;
	}
	else
	{
	    /* Other true servers require CSP. */
	    CXalter( CX_A_NEED_CSP, (PTR)1 );
	}
    }

    Sc_main_cb->sc_names_reg = TRUE;
    if (csib->cs_ascnt == 0)
    {
	csib->cs_ascnt = csib->cs_scnt;
    }
    Sc_main_cb->sc_soleserver = DMC_S_MULTIPLE;
    Sc_main_cb->sc_solecache = DMC_S_MULTIPLE;
    Sc_main_cb->sc_acc = 16;

# ifdef EVALUATION_RELEASE
    {
        Sc_main_cb->sc_max_conns = 32;
    }
# endif /* EVALUATION_RELEASE */

# ifdef DEVELOPER_EDITION
    if ( Sc_main_cb->sc_max_conns > 10 )
	Sc_main_cb->sc_max_conns = 10;
# endif /* WORKGROUP_EDITION */
#if !defined(int_lnx) && !defined(ibm_lnx) && !defined(int_rpl)
# ifdef WORKGROUP_EDITION
    if ( Sc_main_cb->sc_max_conns > 250 )
	Sc_main_cb->sc_max_conns = 250;
# endif /* DEVELOPER_EDITION */
# endif /* !int_lnx */

    /* Initialize it with DB_MAXTUP */ 
    Sc_main_cb->sc_maxtup = DB_MAXTUP;
    Sc_main_cb->sc_maxpage = DB_MIN_PAGESIZE;
    Sc_main_cb->sc_tupsize[0] = DB_MAXTUP;
    Sc_main_cb->sc_pagesize[0] = DB_MIN_PAGESIZE;
    for (i = 1; i < DB_MAX_CACHE; i++)
    {
      Sc_main_cb->sc_tupsize[i] = 0;
      Sc_main_cb->sc_pagesize[i] = 0;
    }

    Sc_main_cb->sc_min_priority = csib->cs_min_priority;
    Sc_main_cb->sc_max_priority = csib->cs_max_priority;
    Sc_main_cb->sc_min_usr_priority = csib->cs_min_priority;
    Sc_main_cb->sc_norm_priority = csib->cs_norm_priority;
    /*
    ** Allow 4 for system threads to run at various higher priorities
    ** if possible.
    */
    Sc_main_cb->sc_max_usr_priority = Sc_main_cb->sc_max_priority-4;
    /*
    ** Check if this CS doesn't allow us enough headroom between norm and max,
    */
    if (Sc_main_cb->sc_max_usr_priority < Sc_main_cb->sc_norm_priority+1)
    {
        Sc_main_cb->sc_max_usr_priority = Sc_main_cb->sc_norm_priority+1;
        if (Sc_main_cb->sc_max_usr_priority > Sc_main_cb->sc_max_priority)
    	    Sc_main_cb->sc_max_usr_priority = Sc_main_cb->sc_max_priority;

    }
    Sc_main_cb->sc_assoc_timeout = -1;   /* Default init value: no timeout */
    Sc_main_cb->sc_rule_depth = QEF_STACK_MAX;		/* Default init value */
    Sc_main_cb->sc_events = SCE_NUM_EVENTS;		/* Default init value */
    Sc_main_cb->sc_event_priority = Sc_main_cb->sc_norm_priority;
    Sc_main_cb->sc_startup_time = TMsecs();     /* remember when we started */
    Sc_main_cb->sc_class_node_affinity = 0; /* Default = no node affinity */
    Sc_main_cb->sc_batch_copy_optim = TRUE; /* Use copy optim by default */

    ult_init_macro(&Sc_main_cb->sc_trace_vector, SCF_NBITS, SCF_VPAIRS, SCF_VONCE);

    /*
    ** Initialize RDF's startup parms now, so we can place any specified
    ** values into this structure when scanning startup parms.
    */
    rdf_ccb.length	    = sizeof(rdf_ccb);
    rdf_ccb.type	    = RDF_CCB_TYPE;
    rdf_ccb.owner	    = (PTR) DB_SCF_ID;
    rdf_ccb.ascii_id	    = RDF_ID_CCB;
    rdf_ccb.rdf_fac_id	    = DB_SCF_ID;
    rdf_ccb.rdf_poolid	    = NULL;
    rdf_ccb.rdf_max_tbl	    = 0;
    rdf_ccb.rdf_colavg	    = 0;
    rdf_ccb.rdf_multicache  = 0;
    rdf_ccb.rdf_cachesiz    = 0;
    rdf_ccb.rdf_server	    = NULL;
    rdf_ccb.rdf_num_synonym = 0;
    rdf_ccb.rdf_maxddb	    = 0;
    rdf_ccb.rdf_avgldb	    = 0;

    /*
    ** Initialize OPF's startup parms now, so we can place any specified 
    ** values into this structure when scanning startup parms.
    */

    opf_cb.opf_length = sizeof(opf_cb);
    opf_cb.opf_type = OPFCB_CB;
    opf_cb.opf_owner = (PTR) DB_SCF_ID;
    opf_cb.opf_ascii_id = OPFCB_ASCII_ID;
    opf_cb.opf_server = 0;
    opf_cb.opf_smask = 0;		    /* Initialize later */
    opf_cb.opf_smask |= OPF_HASH_JOIN;	    /* turn hash joins back on
					    ** by default */
    opf_cb.opf_smask |= OPF_NEW_ENUM;	    /* turn greedy enumeration on
					    ** by default */
    opf_cb.opf_mxsess = 0;		    /* Initialize later */
    opf_cb.opf_actsess = 0;                 /* Initialize later */
    opf_cb.opf_qeftype = OPF_SQEF;	    /* as opposed to dumb qef */
    opf_cb.opf_ddbsite = NULL;
    opf_cb.opf_mserver = 0;		    /* Initialize later */
    opf_cb.opf_maxmemf = 0.5;		    /* OPF.MAXMEMF */
    opf_cb.opf_cnffact = 2;		    /* OPF.CNFFACT */
    opf_cb.opf_qefsort = 2048 * 8;          /* QEF.SORT_SIZE */
    opf_cb.opf_qefhash = 2048 * 1024;       /* QEF.HASH_SIZE */
    opf_cb.opf_sortmax = -1;                /* OPF.SORTMAX  */
    opf_cb.opf_cpufactor = 0.0;             /* OPF.CPUFACTOR */
    opf_cb.opf_timeout = 0.0;               /* OPF.TIMEOUTFACTOR */
    opf_cb.opf_joinop_timeout = 0.0;        /* OPF.JOINOPTIMEOUT */
    opf_cb.opf_joinop_timeoutabort = 0.0;   /* OPF.JOINOPTIMEOUTABORT */
    opf_cb.opf_rfactor = 0.0;               /* OPF.REPEATFACTOR */
    opf_cb.opf_exact = 0.0;                 /* OPF.EXACTKEY */
    opf_cb.opf_range = 0.0;                 /* OPF.RANGEKEY */
    opf_cb.opf_nonkey = 0.0;                /* OPF.NONKEY */
    opf_cb.opf_spatkey = 0.0;               /* OPF.SPATKEY */
    opf_cb.opf_rangep_max = 0;		    /* OPF.RANGEP_MAX */
    opf_cb.opf_stats_nostats_factor = 1.0;  /* OPF.STATS_NOSTATS_FACTOR */
    opf_cb.opf_cstimeout = 0;		    /* OPF.CSTIMEOUT */
    opf_cb.opf_pq_dop = 1;		    /* degree of parallelism */
    opf_cb.opf_pq_threshold = 1000.;	    /* threshold cost for parallel
					    ** query compile */
    opf_cb.opf_pq_partthreads = 8;	    /* threads for each partitioned
					    ** table/join */
    opf_cb.opf_value = DB_HEAP_STORE;	    /* Tradition: compressed heap */
    opf_cb.opf_compressed = TRUE;	    /* for the result structure */
    opf_cb.opf_autostruct = FALSE;
    opf_cb.opf_greedy_factor = 1.0;	    /* OPF.GREEDY_FACTOR */
    opf_cb.opf_inlist_thresh = 18000;	    /* OPF_INLIST_THRESH *

    /* Initialize QEF's  startup parms now....	*/

    qef_cb.qef_length = sizeof(qef_cb);
    qef_cb.qef_type = QESRCB_CB;
    qef_cb.qef_owner = (PTR) DB_SCF_ID;
    qef_cb.qef_ascii_id = QESRCB_ASCII_ID;
    qef_cb.qef_eflag = QEF_INTERNAL;
    qef_cb.qef_server = 0;
    qef_cb.qef_flag_mask = 0;
    /* The memory pool limits are defaulted in later */
    qef_cb.qef_dsh_maxmem = 0;
    qef_cb.qef_sorthash_memory = 0;
    qef_cb.qef_max_mem_sleep = 30;	/* Default 30 second resource wait */
    qef_cb.qef_hash_rbsize = 0;
    qef_cb.qef_hash_wbsize = 0;
    qef_cb.qef_hash_cmp_threshold = 0;
    qef_cb.qef_hashjoin_min = 0;
    qef_cb.qef_hashjoin_max = 0;
    /* The individual limits were initialized just above */
    qef_cb.qef_sort_maxmem = opf_cb.opf_qefsort;
    qef_cb.qef_hash_maxmem = opf_cb.opf_qefhash;
    qef_cb.qef_nodep_chk = 0; 

    /* Initialize DMF's startup parms now.... */

    dmc.type = DMC_CONTROL_CB;
    dmc.length = sizeof(dmc);
    dmc.dmc_op_type = DMC_SERVER_OP;
    dmc.dmc_name.data_address = Sc_main_cb->sc_sname;
    dmc.dmc_name.data_in_size = sizeof(Sc_main_cb->sc_sname);
    dmc.dmc_id = Sc_main_cb->sc_pid;
    dmc.dmc_flags_mask = 0;
    dmc.dmc_server = 0;

    if( Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER )
	dmc.dmc_flags_mask |= DMC_RECOVERY;

    if( Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER )
	dmc.dmc_flags_mask |= DMC_IOMASTER;

    Sc_main_cb->sc_capabilities &= ~SC_VCH_COMPRESS;
    if (PMget("!.vch_compression", &compress_on_off) == OK)
    {
        if (STcasecmp(compress_on_off, "ON" ) == 0)
        {
            Sc_main_cb->sc_capabilities |= SC_VCH_COMPRESS;
        }
    }

    /* Initialize PSF's startup parms now */

    psq_cb.psq_type = PSQCB_CB;
    psq_cb.psq_length = sizeof(psq_cb);
    psq_cb.psq_ascii_id = PSQCB_ASCII_ID;
    psq_cb.psq_owner = (PTR) DB_SCF_ID;
    psq_cb.psq_qlang = DB_QUEL | DB_SQL;
    psq_cb.psq_server = 0;
    psq_cb.psq_version = 0;
    psq_cb.psq_mserver = 0;
    psq_cb.psq_vch_prec = FALSE;
    psq_cb.psq_flag = 0L;
	psq_cb.psq_flag2 = 0L;
    psq_cb.psq_maxmemf = 0.5;	/* default memory proportion */
    psq_cb.psq_cp_qefrcb = NULL;

    /* server_class gets passed to PSF so it can output it in SC930 trace */
    psq_cb.psq_server_class = Sc_main_cb->sc_server_class;
		 
    /*
    ** Parse the server startup options.
    ** Put dmf options into the characteristic array for the dmf startup call.
    */

    /* Put UDADT M/m id's in first, so they are easy to find at check time  */

    dca_index = 0;
    dca[dca_index].char_id = DMC_C_MAJOR_UDADT;
    dca[dca_index++].char_value = DMC_C_UNKNOWN;
    dca[dca_index].char_id = DMC_C_MINOR_UDADT;
    dca[dca_index++].char_value = DMC_C_UNKNOWN;

    /*
    ** Initialize the scd_cb, the collection of oddball parameters that
    ** have yet to be moved into the proper facilities' control blocks.
    ** Also, initialize those the pieces of qsf_rcb and sxf_rcb that
    ** scd_options() can touch.
    */

    scd_cb.max_dbcnt = 0;
    scd_cb.qef_data_size = 2560;	/* QEF Default */
    scd_cb.num_logwriters = 1;
    scd_cb.start_gc_thread = 1;
    scd_cb.cp_timeout = 600;
    scd_cb.sharedcache = FALSE;
    scd_cb.cache_name = NULL;
    scd_cb.dblist = NULL;
    scd_cb.mllist = NULL;
    scd_cb.c2_mode = 0;
    scd_cb.local_vnode = PMgetDefault( 1 );	/* cheesy! */
    scd_cb.secure_level=NULL;	/* No secure level initially */
    scd_cb.num_writealong = scd_cb.num_readahead = 0;
    scd_cb.num_ioslaves = 0;

    switch( Sc_server_type )
    {
    case SC_INGRES_SERVER:	scd_cb.server_class = GCA_INGRES_CLASS; break;
    case SC_RMS_SERVER:		scd_cb.server_class = GCA_RMS_CLASS; break;
    case SC_STAR_SERVER:	scd_cb.server_class = GCA_STAR_CLASS; break;
    case SC_RECOVERY_SERVER:	scd_cb.server_class = "RECOVERY"; break;
    case SC_IOMASTER_SERVER:  	scd_cb.server_class = "IOMASTER"; break;
    default:			scd_cb.server_class = GCA_INGRES_CLASS; break;
    }

    qsf_rcb.qsf_pool_sz = 0;
    sxf_rcb.sxf_pool_sz = 0;

    /*
     * Fetch options from PM and distribute their values into the
     * various facilities' control blocks.
     */

    status = scd_options( 
		dca + dca_index,
		&dca_index,
		&opf_cb,
		&rdf_ccb,
		&dmc,
		&psq_cb,
		&qef_cb,
		&qsf_rcb,
		&sxf_rcb,
		&scd_cb );

    if (status)
	return status;

    /* Tell trace file the SCF memory page size on this platform */
    TRdisplay("Page size SCU_MPAGESIZE is %d bytes\n", SCU_MPAGESIZE);

#ifdef LNX
    /* For Linux only, announce NPTL-ness.  (A non-NPTL build can run on
    ** an NPTL kernel/glibc, and you can't tell by looking.)
    ** This can go away when we stop building for 2.4 and older.
    */
#ifdef SIMULATE_PROCESS_SHARED
    TRdisplay("This is an old-style (non-NPTL) build.\n");
#else
    TRdisplay("This is an NPTL build.\n");
#endif
#endif

    /*
    ** Check for startup parameter conflicts
    */

    /*@FIX_ME@*/

    /*
    ** Preserve class name.
    */
    STmove( scd_cb.server_class, ' ', sizeof(Sc_main_cb->sc_server_class),
            Sc_main_cb->sc_server_class );

    /*
    ** Set default CURSOR_MODE flags for dbmsinfo()
    */
    if (Sc_main_cb->sc_csrflags == 0x00L)
	/* Default is CURSOR_UPDATE_MODE=DEFERRED */
	Sc_main_cb->sc_csrflags |= SC_CSRDEFER;

    if (Sc_main_cb->sc_defcsrflag == 0x00L)
	/* Default cursor open mode CURSOR_DEFAULT_MODE=UPDATE */
	Sc_main_cb->sc_defcsrflag |= SC_CSRUPD;

    if (Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER)
    {
	/* don't turn off name server registration here, or IMA won't work */
        /*
        ** NOTE - the recovery server should not run with B1 or C2 security. 
        */
	scd_cb.c2_mode = FALSE;
	scd_cb.secure_level = NULL;
    }

    if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
    {
        Sc_main_cb->sc_events = 0;	/* No Events for Star */
	Sc_main_cb->sc_fastcommit = 0;  /* No FastCommit for Star */
        Sc_main_cb->sc_dmcm = 0;        /* No DMCM for Star */
	scd_cb.sharedcache = 0;		/* No SharedCache for Star */
	scd_cb.c2_mode = FALSE;		/* No C2 for Star, but allow B1 */
	dmc.dmc_flags_mask |= DMC_STAR_SVR; /* tell DMF this is star */
    }
    else
    {
	Sc_main_cb->sc_nostar_recovr = TRUE;  /* Star only, turn it off */
	Sc_main_cb->sc_no_star_cluster = FALSE; /* Star only, turn it off */
        if ( cluster ) Sc_main_cb->sc_dmcm = TRUE; /* Always DMCM if cluster */
    }

    if (Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER)
    {
	Sc_main_cb->sc_writealong  = max(scd_cb.num_writealong, 10);
	Sc_main_cb->sc_readahead   = max(scd_cb.num_readahead, 1); 	
	IOmaster.num_ioslaves      = max(scd_cb.num_ioslaves,  
	    (Sc_main_cb->sc_writealong + Sc_main_cb->sc_readahead));
    }
    else
    if (Sc_server_type == SC_INGRES_SERVER)
	Sc_main_cb->sc_readahead   = scd_cb.num_readahead;
	
    /* 
    ** If C2 get other general secure related options and save
    */
    if(scd_cb.c2_mode)
    {
	status=scd_sec_init_config(scd_cb.c2_mode);
	if(status!=E_DB_OK)
		return (E_SC0124_SERVER_INITIATE);
        /*
        ** Label the server as C2 security enabled.
	*/
	Sc_main_cb->sc_capabilities |= SC_C_C2SECURE;
    }
    /*
    ** Terminator thread needed if KME and not disabled.
    ** Recovery/IOmaster servers do not have a terminator thread since they
    ** doesn't have regular users currently
    */
    if ( Sc_main_cb->sc_session_chk_interval>0 &&
       !(Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER) &&
       !(Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER))
            term_thread=TRUE;

    /*
    ** init CUT
    */
    status = cut_init((STATUS (*)())uleFormatFcn);
    if (status != E_DB_OK)
    {
        sc0ePut(NULL, E_SC0392_CUT_INIT, NULL, 0);
        return(E_SC0124_SERVER_INITIATE);
    }
    /*
    ** Start security audit writer if C2 server and not 
    ** recovery server.
    */
    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE &&
	!(Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER))
	    secaudit_writer=TRUE;

    /*
    ** If this server will have dedicated server tasks, then alter
    ** the dispatcher to allow more threads so that the server tasks
    ** will not eat into the max_sessions parameters.
    */
    user_threads = Sc_main_cb->sc_nousers;

    /* reserve sessions for special users on servers with sessions */
    if ( !(Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER) )
	user_threads += Sc_main_cb->sc_rsrvd_conns;

    /* Assume a single writebehind thread in the 2k cache */
    dbms_tasks = 6;

    /*
    ** Add a special thread for Consistency Point handling.
    ** There is now a Consistency Point Thread in all servers.
    */
    dbms_tasks++;

    if (Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER)
    {
	dbms_tasks += 2;    /* recovery thread + deadlock detect thread */
	if (scd_cb.cp_timeout)
	    dbms_tasks++;   /* add cp timer thread */
	if ( cluster )
	{

	    /* Allow for CSP, CXMSG & CXMSGMON threads. */
	    dbms_tasks += 3;
	    if ( !CXconfig_settings( CX_HAS_DEADLOCK_CHECK ) )
	    {
		/* We need our own distibuted deadlock thread. */
		dbms_tasks ++;
	    }
	}
    }

    if (Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER)
    {
    	dbms_tasks += Sc_main_cb->sc_writealong;
    }
    dbms_tasks += Sc_main_cb->sc_readahead;

    /*
    ** All servers can also have the logging & locking system special
    ** threads: check-dead, group-commit, force-abort, and logwriters.
    */
    dbms_tasks += scd_cb.num_logwriters;
    dbms_tasks += scd_cb.start_gc_thread;
    dbms_tasks += 3;

    /* Add a special thread for EVENT handling */
    if (Sc_main_cb->sc_events != 0)
	dbms_tasks++;
    
    /* Add a special thread for security auditing */
    if ( secaudit_writer )
	dbms_tasks++;

    /* Add a special thread for session termination */
    if (term_thread)
	dbms_tasks++;

    /* Account for replicator queue management threads. */
    if (Sc_main_cb->sc_rep_qman > 0)
        dbms_tasks += Sc_main_cb->sc_rep_qman;
    
    if (dbms_tasks || (user_threads != csib->cs_scnt) )
    {
	cscb.cs_scnt = user_threads + dbms_tasks;
	cscb.cs_ascnt = csib->cs_scnt;
	cscb.cs_stksize = CS_NOCHANGE;
	stat = CSalter(&cscb);
	if (stat != OK)
	{
	    sc0ePut(NULL, stat, NULL, 0);
	    sc0ePut(NULL, E_SC0242_ALTER_MAX_SESSIONS, NULL, 2,
		     sizeof(csib->cs_scnt), (PTR)&csib->cs_scnt,
		     sizeof(cscb.cs_scnt), (PTR)&cscb.cs_scnt);
	    return(E_SC0124_SERVER_INITIATE);
	}
    }
	
    /*
    ** Determine the number of threads for each facility startup parameter.
    ** Some special threads do not require all facilities.
    */
    Sc_main_cb->sc_nousers += dbms_tasks;
    num_psf_sessions = user_threads;
    num_qef_sessions = user_threads;
    num_qsf_sessions = user_threads;
    num_opf_sessions = user_threads;
    num_dmf_sessions = user_threads + dbms_tasks;

    /* First, initialize SCF */

    for (;;)
    {
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_SCF_ID;
	scf_cb.scf_session = DB_NOSESSION;
	Sc_main_cb->sc_state = SC_INIT;

	status = sc0m_initialize();	    /* initialize the memory allocation */
	if (status != E_SC_OK)
	    break;

	TMend(&Sc_main_cb->sc_timer);

	/* now allocate the queues and que headers so that work can proceed normally */

	status = sc0m_allocate(SC0M_NOSMPH_MASK,
				(i4)sizeof(SCS_MCB),
				MCB_ID,
				(PTR) SCD_MEM,
				MCB_ASCII_ID,
				(PTR *) &mcb_head);
	if (status != E_SC_OK)
	    break;
	mcb_head->mcb_next = mcb_head->mcb_prev = mcb_head;

	mcb_head->mcb_facility = DB_SCF_ID;
	Sc_main_cb->sc_mcb_list = mcb_head;
	CSw_semaphore( &Sc_main_cb->sc_mcb_semaphore, CS_SEM_SINGLE, "SC MCB" );
	CSw_semaphore( &Sc_main_cb->sc_misc_semaphore, CS_SEM_SINGLE, "SC MISC" );
	
	Sc_main_cb->sc_kdbl.kdb_next = (SCV_DBCB *) &Sc_main_cb->sc_kdbl;
	Sc_main_cb->sc_kdbl.kdb_prev = (SCV_DBCB *) &Sc_main_cb->sc_kdbl;

	 
	CSw_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore, CS_SEM_SINGLE,
			"SCS kdb_semaphore");

	CSw_semaphore(&Sc_main_cb->sc_gcsem, CS_SEM_SINGLE,
			"SCS gcsem");

	break;
    }

    if (status)
    {
	if (status & E_SC_MASK)
	{
	    sc0ePut(NULL, E_SC0204_MEMORY_ALLOC, NULL, 0);
	    sc0ePut(NULL, status, NULL, 0);
	}
	else
	{
	    sc0ePut(NULL, E_SC0200_SEM_INIT, NULL, 0);
	    sc0ePut(&scf_cb.scf_error, 0, NULL, 0);
	}
	return(E_SC0124_SERVER_INITIATE);
    }

    Sc_main_cb->sc_dbdb_loc = (char *) Sc_main_cb + sizeof(SC_MAIN_CB);
    dbdb_loc = (SCV_LOC *) Sc_main_cb->sc_dbdb_loc;
    MEmove(8, "$default", ' ',
	    sizeof(dbdb_loc->loc_entry.loc_name),
	    (char *)&dbdb_loc->loc_entry.loc_name);
    LOingpath("II_DATABASE",
	"iidbdb",
	LO_DB,
	&dbloc);
    LOcopy(&dbloc, loc_buf, &dbloc);
    LOtos(&dbloc, &dbplace);
    dbdb_loc->loc_entry.loc_l_extent = STlength(dbplace);
    MEmove(dbdb_loc->loc_entry.loc_l_extent,
	    dbplace,
	    ' ',
	    sizeof(dbdb_loc->loc_entry.loc_extent),
	    dbdb_loc->loc_entry.loc_extent);

    dbdb_loc->loc_entry.loc_flags = LOC_DEFAULT | LOC_DATA;
    dbdb_loc->loc_l_loc_entry = 1 * sizeof(DMC_LOC_ENTRY);
    

    for(;;)
    {
        /* Check for Must Log DB list */
        if( !scd_cb.mllist || !*scd_cb.mllist )
        {
            MustLog_DB_Lst.dbcount = 0;
            MustLog_DB_Lst.dblist = NULL;
        }
        else
        {
            char    *mllist;
            i4      mllist_cnt = 0;
            i4      waswhite = 1;
            char    *p;
    
            /* Count the whitespace or comma separated words in mllist
            ** - STcountwords()?
            ** Transform COMMAs into SPACEs so that STgetwords() still works.
            */
    
            for(  p = scd_cb.mllist; *p; CMnext( p ) )
            {
                i4 iswhite;
                if( !CMcmpcase( p, "," ) )
                    *p = ' ';           /* Transform COMMA into SPACE */
                iswhite = CMwhite( p );
    
                if( !iswhite && waswhite )
                    mllist_cnt++;
    
                waswhite = iswhite;
            }

            /* Allocate blocks for a copy of the string and for the word list */
            status = sc0m_allocate(
                        SC0M_NOSMPH_MASK,
                        (i4)(STlength( scd_cb.mllist ) + 1),
                        MCB_ID,
                        (PTR) SCD_MEM,
                        MCB_ASCII_ID,
                        &block);

            if (DB_FAILURE_MACRO(status))
            {
                sc0ePut(NULL, E_SC0204_MEMORY_ALLOC, NULL, 0);
                sc0ePut(NULL, status, NULL, 0);
                break;
            }

            mllist = (char *)block;

            status = sc0m_allocate(
                        SC0M_NOSMPH_MASK,
                        (i4)(mllist_cnt *
                            sizeof(MustLog_DB_Lst.dblist)),
                        MCB_ID,
                        (PTR) SCD_MEM,
                        MCB_ASCII_ID,
                        &block);

            if (DB_FAILURE_MACRO(status))
            {
                sc0ePut(NULL, E_SC0204_MEMORY_ALLOC, NULL, 0);
                sc0ePut(NULL, status, NULL, 0);
                break;
            }

            MustLog_DB_Lst.dbcount = mllist_cnt;
            MustLog_DB_Lst.dblist = (char **)block;

            STcopy( scd_cb.mllist, mllist );
            STgetwords( mllist, &mllist_cnt, MustLog_DB_Lst.dblist);
        }
        break;
    }

    for (;;)
    {
	gca_cb.gca_expedited_completion_exit = scs_ctlc;
	gca_cb.gca_normal_completion_exit = scc_gcomplete;
	gca_cb.gca_alloc_mgr = sc0m_gcalloc;
	gca_cb.gca_dealloc_mgr = sc0m_gcdealloc;
	gca_cb.gca_p_semaphore = CSp_semaphore;
	gca_cb.gca_v_semaphore = CSv_semaphore;
	gca_cb.gca_modifiers = GCA_ASY_SVC|GCA_API_VERSION_SPECD;
        gca_cb.gca_local_protocol = GCA_PROTOCOL_LEVEL_68;
	gca_cb.gca_api_version = GCA_API_LEVEL_5;
	gca_cb.gca_decompose = NULL;	/* EJLFIX: Fill in when Adf is ready */
	gca_cb.gca_cb_decompose = NULL;	/* EJLFIX: Fill in when Adf is ready */
	gca_cb.gca_lsncnfrm = CSaccept_connect;

#ifdef GCA_BATCHMODE
        if ((Sc_main_cb->sc_capabilities & SC_BATCH_SERVER) &&
            (Sc_main_cb->sc_capabilities & SC_GCA_SINGLE_MODE))
	   gca_cb.gca_modifiers |= GCA_BATCHMODE;
#endif


	/* Once the switch to GCA calls with CB is made,
	** sc_gca_cb will be used. For now mark it as non-zero
	** to distinguish it from standalone mode. */
	Sc_main_cb->sc_gca_cb = (PTR)1;
	status = IIGCa_call(GCA_INITIATE,
		    (GCA_PARMLIST *)&gca_cb,
		    GCA_SYNC_FLAG,
		    (PTR)0,
		    -1,
		    &error);
	if (DB_FAILURE_MACRO(status))
	{
	    sc0ePut(NULL, error, NULL, 0);
	    break;
	}
	else if (gca_cb.gca_status != E_GC0000_OK)
	{
	    sc0ePut(NULL, gca_cb.gca_status, &gca_cb.gca_os_status, 0);
	    status = gca_cb.gca_status;
	    break;
	}

	gcr_cb.gca_srvr_class = scd_cb.server_class;

	/*
	** Formulate arguments to GCA_REGISTER in support of dblist.
	*/

	if( !scd_cb.dblist || !*scd_cb.dblist )
	{
	    gcr_cb.gca_l_so_vector = 0;
	    gcr_cb.gca_served_object_vector = NULL;
	    if (Sc_server_type == SC_IOMASTER_SERVER)
	    {
	    	IOmaster.dbcount  = 0;    
	    	IOmaster.dblist   = NULL;
	    }
	}
	else
	{
	    char    *dblist;
	    i4	    dblist_cnt = 0;
	    i4	    waswhite = 1;
	    char    *p;

	    /* Count the whitespace or comma separated words in dblist 
	    ** - STcountwords()? 
	    ** Transform COMMAs into SPACEs so that STgetwords() still works.
	    */

	    for(  p = scd_cb.dblist; *p; CMnext( p ) )
	    {
		i4 iswhite;
		if( !CMcmpcase( p, "," ) )
		    *p = ' ';		/* Transform COMMA into SPACE */
		iswhite = CMwhite( p );

		if( !iswhite && waswhite )
		    dblist_cnt++;
		
		waswhite = iswhite;
	    }

	    /* Allocate blocks for a copy of the string and for the word list */

	    status = sc0m_allocate(
			SC0M_NOSMPH_MASK,
			(i4)(STlength( scd_cb.dblist ) + 1),
			MCB_ID,
			(PTR) SCD_MEM,
			MCB_ASCII_ID,
			&block);

	    if (DB_FAILURE_MACRO(status))
	    {
		sc0ePut(NULL, E_SC0204_MEMORY_ALLOC, NULL, 0);
		sc0ePut(NULL, status, NULL, 0);
		break;
	    }

	    dblist = (char *)block;

	    status = sc0m_allocate(
			SC0M_NOSMPH_MASK,
			(i4)(dblist_cnt *
				sizeof( gcr_cb.gca_served_object_vector)),
			MCB_ID,
			(PTR) SCD_MEM,
			MCB_ASCII_ID,
			&block);

	    if (DB_FAILURE_MACRO(status))
	    {
		sc0ePut(NULL, E_SC0204_MEMORY_ALLOC, NULL, 0);
		sc0ePut(NULL, status, NULL, 0);
		break;
	    }

	    gcr_cb.gca_l_so_vector = dblist_cnt;
	    gcr_cb.gca_served_object_vector = (char **)block;

	    STcopy( scd_cb.dblist, dblist );
	    STgetwords( dblist, &dblist_cnt, gcr_cb.gca_served_object_vector );

	    if (Sc_server_type == SC_IOMASTER_SERVER)
	    {
	    	IOmaster.dbcount = dblist_cnt;
	    	IOmaster.dblist   = dblist;  /* list of null termed strings */ 
	    }
	}

	gcr_cb.gca_installn_id = 0;
	gcr_cb.gca_process_id = 0;
	gcr_cb.gca_no_connected = Sc_main_cb->sc_nousers;
	gcr_cb.gca_no_active = Sc_main_cb->sc_nousers;
	gcr_cb.gca_modifiers = 0;

	if (Sc_main_cb->sc_names_reg != TRUE)
	    gcr_cb.gca_modifiers = GCA_RG_NO_NS;

	if (Sc_main_cb->sc_soleserver == DMC_S_SINGLE)
	    gcr_cb.gca_modifiers |= GCA_RG_SOLE_SVR;

	/*
	** Set GCM MIB support flags
	*/
	if( !(Sc_main_cb->sc_capabilities & SC_STAR_SERVER) )
	    gcr_cb.gca_modifiers |= GCA_RG_INSTALL;

	if (Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER)
	{
	    gcr_cb.gca_modifiers |= GCA_RG_IIRCP;
	    gcr_cb.gca_srvr_class = GCA_IUSVR_CLASS;
	}
	else
	{
	    gcr_cb.gca_modifiers |= GCA_RG_INGRES;
	}

	gcr_cb.gca_listen_id = 0;

	status = IIGCa_call(GCA_REGISTER,
			    (GCA_PARMLIST *)&gcr_cb,	/* Parameter list */
			    GCA_SYNC_FLAG,	/* Synchronous execution */
			    (PTR)0,		/* ID for asynch rtn. if any */
			    (i4) -1,	/* No timeout */
			    &error);		/* Error type if E_DB_ERROR
						   was returned */

	if (DB_FAILURE_MACRO(status))
	{
	    sc0ePut(NULL, error, NULL, 0);
	    break;
	}
	else if (gcr_cb.gca_status != E_GC0000_OK)
	{
	    sc0ePut(NULL, gcr_cb.gca_status, &gcr_cb.gca_os_status, 0);
	    status = gcr_cb.gca_status;
	    break;
	}

	MEmove(STlength(gcr_cb.gca_listen_id),
		gcr_cb.gca_listen_id,
		' ',
		sizeof(Sc_main_cb->sc_sname),
		Sc_main_cb->sc_sname);
	Sc_main_cb->sc_facilities |= 1 << DB_GCF_ID;

	/* Pass down listen address as cs_name */

	STncpy( csib->cs_name, gcr_cb.gca_listen_id, sizeof(csib->cs_name) );

	MEmove( 
	    STlength( scd_cb.local_vnode ),
	    scd_cb.local_vnode,
	    ' ',
	    sizeof( Sc_main_cb->sc_ii_vnode ),
	    Sc_main_cb->sc_ii_vnode );

	/*
	** Get the node for error log.
	*/
	if (cluster)
	{
	    CXnode_name(node);
	}
	else
	{
	    MEmove(
		sizeof( Sc_main_cb->sc_ii_vnode ),
		Sc_main_cb->sc_ii_vnode,
		' ',
		sizeof(node),
		node);
	}

	ule_initiate(node, STlength(node),
		     Sc_main_cb->sc_sname,
		     sizeof(Sc_main_cb->sc_sname));

	/* Store Cluster node name */
	MEmove(sizeof(node), node, ' ',
	       sizeof(Sc_main_cb->sc_cname), Sc_main_cb->sc_cname);

	/* 
	** Initialize semaphore and load the default TZ structure.
	*/
	CSw_semaphore((CS_SEMAPHORE *) &Sc_main_cb->sc_tz_semaphore, 
					CS_SEM_SINGLE,
					"SCF TZ sem" );
	stat = TMtz_init(&tz_cb);
	if(stat != OK)
	{
	    PTR  tz_name;

	    sc0ePut(NULL, stat, NULL, 0);
	    if (stat != TM_NO_TZNAME)
	    {
		/* 
		** Display timezone name as well if problem is not because
		** of II_TIMEZONE_NAME not defined
		*/
		NMgtAt(ERx("II_TIMEZONE_NAME"), &tz_name);		
		sc0ePut(NULL, E_SC033D_TZNAME_INVALID, NULL, 3,
		      STlength((char *)tz_name),
		      tz_name,
		      STlength(SystemVarPrefix), SystemVarPrefix,
		      STlength(SystemVarPrefix), SystemVarPrefix);
	    }
	    status = E_DB_FATAL;
	    break;
	}

	/*
	** Initialize the server's II_DATE_CENTURY_BOUNDARY
	*/
	{
	    i4  year_cutoff;

	    stat = TMtz_year_cutoff(&year_cutoff);

	    if (stat != OK )
	    {
	    	sc0ePut(NULL, stat, NULL, 0);
		sc0ePut(NULL, E_SC037B_BAD_DATE_CUTOFF, NULL, 2,
				  STlength(SystemVarPrefix), SystemVarPrefix,
				  0, (PTR)(SCALARP)year_cutoff);
	    	status = E_DB_FATAL;
		break;
	    }
	}

	/*
	** Now move on and initiate the other facilities in the server.
	** The order of invocation depends upon who calls who to do
        ** work.  At the moment it is ADF, ULF, GWF, DMF, then random.
	*/

	size = adg_srv_size();
	scf_cb.scf_scm.scm_functions = 0;
	scf_cb.scf_scm.scm_in_pages =
	    ((size + SCU_MPAGESIZE - 1) & ~(SCU_MPAGESIZE - 1)) / SCU_MPAGESIZE;
	status = scf_call(SCU_MALLOC, &scf_cb);
	if (status)
	{
	    sc0ePut(&scf_cb.scf_error, 0, NULL, 0);
	    break;
	}
	Sc_main_cb->sc_advcb = (PTR) scf_cb.scf_scm.scm_addr;
	if (status)
	    break;

	status = sc0m_allocate(SC0M_NOSMPH_MASK,
				(i4)(sizeof(SC0M_OBJECT) + 
				sizeof(ADF_TAB_DBMSINFO) -
					sizeof(ADF_DBMSINFO) +
					(NUM_DBMSINFO_REQUESTS * sizeof(ADF_DBMSINFO))),
				DB_ADF_ID,
				(PTR) SCD_MEM,
				CV_C_CONST_MACRO('a','d','v','b'),
				(PTR *) &dbinfo_block);
	if (status)
	    break;
	    
	dbinfo_block = (ADF_TAB_DBMSINFO *)
			    ((char *) dbinfo_block + sizeof(SC0M_OBJECT));
	dbinfo_block->tdbi_type = ADTDBI_TYPE;
	dbinfo_block->tdbi_ascii_id = ADTDBI_ASCII_ID;
	dbinfo_block->tdbi_numreqs = NUM_DBMSINFO_REQUESTS;
	for (i = 0; i < NUM_DBMSINFO_REQUESTS; i++)
	{
	    dbinfo_block->tdbi_reqs[i].dbi_func = scd_dbinfo_fcn;
	    dbinfo_block->tdbi_reqs[i].dbi_num_inputs = 0;
	    dbinfo_block->tdbi_reqs[i].dbi_lenspec.adi_lncompute = ADI_FIXED;

	    STcopy( dbms_info_items[i].info_name,
		   dbinfo_block->tdbi_reqs[i].dbi_reqname );
	    dbinfo_block->tdbi_reqs[i].dbi_reqcode =
		dbms_info_items[i].info_reqcode;
	    dbinfo_block->tdbi_reqs[i].dbi_num_inputs =
		dbms_info_items[i].info_inputs;
	    dbinfo_block->tdbi_reqs[i].dbi_dt1 =
		dbms_info_items[i].info_dt1;
	    dbinfo_block->tdbi_reqs[i].dbi_lenspec.adi_fixedsize =
		dbms_info_items[i].info_fixedsize;
	    dbinfo_block->tdbi_reqs[i].dbi_dtr = 
		dbms_info_items[i].info_type;
	}
	status = adg_startup(Sc_main_cb->sc_advcb, size, dbinfo_block,
		    Sc_main_cb->sc_capabilities & SC_C_C2SECURE);
	if (status)
	{
	    sc0ePut(NULL, E_SC012C_ADF_ERROR, NULL, 0);
	    break;
	}
	Sc_main_cb->sc_facilities |= 1 << DB_ADF_ID;
	/*
	** Start OME/UDTS unless disabled
	*/
	if (!(Sc_main_cb->sc_secflags & SC_SEC_OME_DISABLED))
	{
	    DB_ERROR		    error;
	    PTR			    new_block;

	    status = sca_add_datatypes((SCD_SCB *) 0,
					Sc_main_cb->sc_advcb,
					scf_cb.scf_scm.scm_in_pages,
					DB_SCF_ID,  /* Deallocate old */
					&error,
					(PTR *) &new_block,
					0);
	    if (status)
		break;
	    Sc_main_cb->sc_advcb = new_block;
	}

	/* Add scd_adf_printf() to FEXI table */
        {
	    ADF_CB                 adf_scb;
	    
	    status = adg_add_fexi( &adf_scb, ADI_02ADF_PRINTF_FEXI, 
				   &scd_adf_printf);
	    if (status)
	    {
		sc0ePut(NULL, E_SC012C_ADF_ERROR, NULL, 0);
		break;
	    }
	}

	/* ULF has no initiation */

        /* Now do GWF -- before DMF because DMF will ask GWF for info. */

	/*
	** Tell GWF facility the location of dmf_call().
	*/
	gw_rcb.gwr_type     = GWR_CB_TYPE;
	gw_rcb.gwr_length   = sizeof(GW_RCB);
	gw_rcb.gwr_dmf_cptr = (DB_STATUS (*)())dmf_call;
	gw_rcb.gwr_gca_cb = Sc_main_cb->sc_gca_cb;

	status = gwf_call( GWF_INIT, &gw_rcb );
	if (status)
	{
	    sc0ePut(&gw_rcb.gwr_error, 0, NULL, 0);
	    break;
	}
	Sc_main_cb->sc_gwbytes = gw_rcb.gwr_scfcb_size;
	Sc_main_cb->sc_gwvcb = gw_rcb.gwr_server;

	/* schang 04/21/92 GW merge
	** if the gateway returned its release identifier, stuff this
	** into the server control block to be coughed up when the user
	** wonders "select dbmsinfo( '_version' )"
	*/

	if ( gw_rcb.gwr_out_vdata1.data_address != 0 )
	{
	    MEmove( gw_rcb.gwr_out_vdata1.data_out_size,
		gw_rcb.gwr_out_vdata1.data_address,
		' ',
		sizeof( Sc_main_cb->sc_iversion ),
		Sc_main_cb->sc_iversion );
	}

	Sc_main_cb->sc_facilities |= 1 << DB_GWF_ID;

	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
	{
	    /* For STAR start a minimal DMF */
	    dca[dca_index].char_id = DMC_C_SCANFACTOR;
	    dca[dca_index++].char_fvalue = 8.0;
	    dca[dca_index].char_id = DMC_C_BUFFERS;
	    dca[dca_index++].char_value  = 30;
	    scd_cb.cp_timeout = 0;
	}
    
	/* Now do DMF */

	dmc.dmc_char_array.data_address =  (char *) dca;
	dca[dca_index].char_id = DMC_C_ABORT;
	dca[dca_index++].char_pvalue = (PTR) scs_log_event;
	dca[dca_index].char_id = DMC_C_LSESSION;
	dca[dca_index++].char_value = num_dmf_sessions;
	dca[dca_index].char_id = DMC_C_TRAN_MAX;
	dca[dca_index++].char_value = csib->cs_ascnt + dbms_tasks;
	if (scd_cb.cp_timeout)
	{
	    dca[dca_index].char_id = DMC_C_CP_TIMER;
	    dca[dca_index++].char_value = scd_cb.cp_timeout;
	}

	if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		dmc.dmc_flags_mask |= DMC_C2_SECURE;
	/*
	** If shared cache has been specified then pass the cache
	** name to DMF.
	*/
	dmc.dmc_cache_name = NULL;
	if (scd_cb.sharedcache)
	    dmc.dmc_cache_name = scd_cb.cache_name;

	/* if parms indicated batch mode, let dmf know. */
	if (Sc_main_cb->sc_capabilities & SC_BATCH_SERVER)
	    dmc.dmc_flags_mask |= DMC_BATCHMODE;

	/* if parms indicated SINGLE SERVER mode, let dmf know */
	dmc.dmc_s_type = Sc_main_cb->sc_soleserver;

	/*
	** If max database count was set in options, then use that for
	** database_max, otherwise use num_users + 1 (extra for iidbdb).
	*/
	dca[dca_index].char_id = DMC_C_DATABASE_MAX;
	if (scd_cb.max_dbcnt)
	    dca[dca_index++].char_value = scd_cb.max_dbcnt;
	else
	    dca[dca_index++].char_value = num_dmf_sessions + 1;

        /* pass # of readahead threads, for storage alloc */
        dca[dca_index].char_id = DMC_C_NUMRAHEAD;
        dca[dca_index++].char_value = Sc_main_cb->sc_readahead;

	/* Make sure we didn't exceed the size of dmc */
	if (dca_index > (sizeof (dca) / sizeof(DMC_CHAR_ENTRY)))
	{
	    sc0ePut(NULL, E_SC0024_INTERNAL_ERROR, NULL, 0);
	    return(E_SC0124_SERVER_INITIATE);
	}

	dmc.dmc_char_array.data_in_size = 
	    dca_index * sizeof(DMC_CHAR_ENTRY);

	/* Give DMF the offset to session's DML_SCB pointer */
	dmc.dmc_dmscb_offset = CL_OFFSETOF(SCD_SCB, scb_sscb) +
		CL_OFFSETOF(SCS_SSCB, sscb_dmscb);

	status = dmf_call(DMC_START_SERVER, &dmc);
	if (status)
	{
	    sc0ePut(&dmc.error, 0, NULL, 0);
	    break;
	}
	/*
	** If clustered, and not the RCP/CSP or STAR, process can be 
	** considered to have joined the Ingres cluster at this point
	*/
	if (cluster && !(Sc_main_cb->sc_capabilities & 
	    (SC_STAR_SERVER|SC_CSP_SERVER)))
	{
	    status = CXjoin(CX_F_IS_SERVER);
	    if ( status )
	    {
		sc0ePut(NULL, status, NULL, 0);
		status = E_DB_FATAL;
		break;
	    }
	}
	Sc_main_cb->sc_dmbytes = dmc.dmc_scfcb_size;
	Sc_main_cb->sc_dmvcb = dmc.dmc_server;

	if (!(Sc_main_cb->sc_capabilities & SC_STAR_SERVER))
	{
	  /* Find out about page types that are configured */
          dmc.dmc_op_type = DMC_DBMS_CONFIG_OP;
          dmc.dmc_flags_mask = 0;
          dmc.dmc_char_array.data_address = (PTR) dmc_char;
          dmc.dmc_char_array.data_out_size = sizeof(DMC_CHAR_ENTRY) * 
	  					DB_MAX_PGTYPE;
	  dmc_char[0].char_id = DMC_C_PAGETYPE_V1;
	  dmc_char[1].char_id = DMC_C_PAGETYPE_V2;
	  dmc_char[2].char_id = DMC_C_PAGETYPE_V3;
	  dmc_char[3].char_id = DMC_C_PAGETYPE_V4;
	  dmc_char[4].char_id = DMC_C_PAGETYPE_V5;
	  dmc_char[5].char_id = DMC_C_PAGETYPE_V6;
	  dmc_char[6].char_id = DMC_C_PAGETYPE_V7;
	  
          status = dmf_call(DMC_SHOW, (PTR) &dmc);
          if (status)
          {
	    sc0ePut(&dmc.error, 0, NULL, 0);
	    break;
          }
	  Sc_main_cb->sc_pagetype[1] = dmc_char[0].char_value;
	  Sc_main_cb->sc_pagetype[2] = dmc_char[1].char_value;
	  Sc_main_cb->sc_pagetype[3] = dmc_char[2].char_value;
	  Sc_main_cb->sc_pagetype[4] = dmc_char[3].char_value;
	  Sc_main_cb->sc_pagetype[5] = dmc_char[4].char_value;
	  Sc_main_cb->sc_pagetype[6] = dmc_char[5].char_value;

	  /* Initialize all tuple size */
          dmc.dmc_op_type = DMC_BMPOOL_OP;
          dmc.dmc_flags_mask = DMC_TUPSIZE;
          dmc.dmc_char_array.data_address = (PTR) dmc_char;
          dmc.dmc_char_array.data_out_size = sizeof(DMC_CHAR_ENTRY) *
						DB_MAX_CACHE;
          status = dmf_call(DMC_SHOW, (PTR) &dmc);
          if (status)
          {
	    sc0ePut(&dmc.error, 0, NULL, 0);
	    break;
          }
	  for (i = 0; i < DB_MAX_CACHE; i++)
	  {
	    if (dmc_char[i].char_value == DMC_C_OFF)
              Sc_main_cb->sc_tupsize[i] = 0;
	    else
	    {
              Sc_main_cb->sc_tupsize[i] = dmc_char[i].char_value;
              Sc_main_cb->sc_maxtup = dmc_char[i].char_value;
	    }
	  }

	  /* Initialize all page size */
          dmc.dmc_op_type = DMC_BMPOOL_OP;
          dmc.dmc_flags_mask = 0;
          dmc.dmc_char_array.data_address = (PTR) dmc_char;
          dmc.dmc_char_array.data_out_size = sizeof(DMC_CHAR_ENTRY) *
					     DB_MAX_CACHE;
          status = dmf_call(DMC_SHOW, (PTR) &dmc);
          if (status)
          {
	    sc0ePut(&dmc.error, 0, NULL, 0);
	    break;
          }
	  for (i = 0, pagesize = DB_MIN_PAGESIZE; i < DB_MAX_CACHE; 
		i++, pagesize *= 2)
	  {
	    if (dmc_char[i].char_value == DMC_C_ON)
	    {
              Sc_main_cb->sc_pagesize[i] = pagesize;
              Sc_main_cb->sc_maxpage = pagesize;
	    }
	    else
              Sc_main_cb->sc_pagesize[i] = 0;
	  }
	}
	else {
	  /*
	  ** For Star we have to have maxtup size etc. Otherwise, scd_main
	  ** will allocate smaller space for tuple area. Though at this
	  ** time, we do not know about the local buffer manager of the 
	  ** coordinator. So fake it with the max size, for now and may be
	  ** we will readjust it at the begin session time.
	  */
	  for (i = 0, pagesize = DB_MIN_PAGESIZE; i < DB_MAX_CACHE; 
		i++, pagesize *= 2)
	  {
              Sc_main_cb->sc_pagesize[i] = pagesize;
              Sc_main_cb->sc_maxpage = pagesize;

	      /* Initialize all tuple size */
              dmc.dmc_op_type = DMC_BMPOOL_OP;
              dmc.dmc_flags_mask = DMC_TUPSIZE;
	      dmc_char[0].char_value = pagesize;
              dmc.dmc_char_array.data_address = (PTR) &dmc_char[0];
              dmc.dmc_char_array.data_out_size = sizeof(DMC_CHAR_ENTRY);
              status = dmf_call(DMC_SHOW, (PTR) &dmc);
              if (status)
              {
		sc0ePut(&dmc.error, 0, NULL, 0);
	        break;
	      }
              Sc_main_cb->sc_tupsize[i] = dmc_char[0].char_value;
              Sc_main_cb->sc_maxtup = dmc_char[0].char_value;
	  }
	}
	/*
	** STAR only needs DMF for sort cost routines.
	** Turn off it's interrupt handlers.
	*/
	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)	
	{
	    SCF_CB   tmp_scb;
	    tmp_scb.scf_type     = SCF_CB_TYPE;
	    tmp_scb.scf_length   = sizeof(tmp_scb);
	    tmp_scb.scf_facility = DB_DMF_ID;
	    tmp_scb.scf_session  = DB_NOSESSION;
	    tmp_scb.scf_nbr_union.scf_amask = SCS_AIC_MASK | SCS_PAINE_MASK;
	    status = scf_call(SCS_CLEAR, &tmp_scb);
	    if (status)
	    {
		sc0ePut(&tmp_scb.scf_error, 0, NULL, 0);
		break;
	    }
	}	

	Sc_main_cb->sc_facilities |= 1 << DB_DMF_ID;

	if (cluster)
	{
	    CXnode_name(node);
	}
	else
	{
	    MEmove(
		sizeof( Sc_main_cb->sc_ii_vnode ),
		Sc_main_cb->sc_ii_vnode,
		' ',
		sizeof(node),
		node);
	}
	MEmove(sizeof(node), node, ' ',
	       sizeof(Sc_main_cb->sc_cname), Sc_main_cb->sc_cname);
					/* Store Cluster node name */
	ule_initiate(node,
			 STlength(node),
			 Sc_main_cb->sc_sname,
			 sizeof(Sc_main_cb->sc_sname));

	/*
	**	    Why don't we call sca_check in a Star server?
	**
	** sca_check verifies that the ADF User Defined Datatypes with which
	** we are linked match the data types which the installation was
	** started with (i.e., the version with which the RCP was linked).
	**
	** The version with which the RCP was linked was returned by DMF as
	** a side-effect of calling dmc_start_server. However, in a Star
	** server, the full DMF startup is not performed (only the bare minimum
	** startup which supports dmt_sort_cost calls was performed). Therefore,
	** dmc_start_server did *not* return the RCP UDT version numbers, and
	** if we call sca_check here we will get a message from sca_check to the
	** error log indicating that the wrong UDT libraries may be in use.
	**
	** Therefore, in a star server we bypass this sca_check call. This is
	** unfortunate, because it removes a bit of verification that might
	** catch an accidental mismatch of UDT libraries. But the alternative
	** is to perform a full DMF server startup, which is expensive. And, we
	** assert the window of vulnerability is in fact small, since the star
	** server is actually the same executable program as is the local server
	** and the RCP, so it is unlikely that the star server would ever be
	** running with different UDT libraries than the local server (the
	** only example we could envision was where the user had linked new
	** UDT libraries while the installation was running, and then after
	** linking the new libraries the user restarted the star server but left
	** the RCP and all local servers up). Thus we judged the skipping of
	** this test to be an acceptable risk in a star server.
	**
	** Bryan Pendleton, Sep, 1993.
	*/

	if ((Sc_main_cb->sc_capabilities & SC_STAR_SERVER) == 0)
	{
	    if (!sca_check(dca[0].char_value, dca[1].char_value))
	    {
		sc0ePut(NULL, E_SC0269_SCA_INCOMPATIBLE, NULL, 2,
			 sizeof(dca[0].char_value),  (PTR)&dca[0].char_value,
			 sizeof(dca[1].char_value),  (PTR)&dca[1].char_value);
		status = E_DB_FATAL;
		break;
	    }
	}

        Sc_main_cb->sc_capabilities |= SC_RESRC_CNTRL;

	/*
	** Don't startup SXF in STAR servers currently
	*/
	if (!(Sc_main_cb->sc_capabilities & SC_STAR_SERVER))
	{
	    /*
	    ** Now initialize SXF. 
	    **
	    ** NOTE: SXF must be initialized after DMF because it needs 
	    ** to make calls to DMF during its initialization phase.
	    */
	    sxf_rcb.sxf_length = sizeof (sxf_rcb);
	    sxf_rcb.sxf_cb_type = SXFRCB_CB;
	    sxf_rcb.sxf_status = 0L;
	    if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
		sxf_rcb.sxf_status |= SXF_STAR_SERVER;
	    if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		sxf_rcb.sxf_status |= SXF_C2_SERVER;
	    sxf_rcb.sxf_mode = SXF_MULTI_USER;
	    sxf_rcb.sxf_max_active_ses = user_threads + 1;
	    sxf_rcb.sxf_dmf_cptr = dmf_call;

	    status = sxf_call(SXC_STARTUP, &sxf_rcb);
	    if (status != E_DB_OK)
	    {
		sc0ePut(&sxf_rcb.sxf_error, 0, NULL, 0);
		break;
	    }
	    Sc_main_cb->sc_sxvcb = sxf_rcb.sxf_server;
	    Sc_main_cb->sc_sxbytes = sxf_rcb.sxf_scb_sz;
	    Sc_main_cb->sc_facilities |= 1 << DB_SXF_ID;
	    Sc_main_cb->sc_sxf_cptr = sxf_call;
	}

	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
	{
	    rdf_ccb.rdf_distrib = DB_2_DISTRIB_SVR;
	    if (rdf_ccb.rdf_maxddb == 0)
	    	rdf_ccb.rdf_maxddb = user_threads;
	}
	else
	    rdf_ccb.rdf_distrib = DB_1_LOCAL_SVR;

	/* Give RDF the offset to session's RDF_SESS_CB pointer */
	rdf_ccb.rdf_rdscb_offset = CL_OFFSETOF(SCD_SCB, scb_sscb) +
		CL_OFFSETOF(SCS_SSCB, sscb_rdscb);

	status = rdf_call(RDF_STARTUP, (PTR) &rdf_ccb);
	if (status)
	{
	    sc0ePut(&rdf_ccb.rdf_error, 0, NULL, 0);
	    break;
	}
	Sc_main_cb->sc_rdvcb = rdf_ccb.rdf_server;
	Sc_main_cb->sc_rdbytes = rdf_ccb.rdf_sesize;
	Sc_main_cb->sc_facilities |= 1 << DB_RDF_ID;

	qef_cb.qef_qpmax = num_qef_sessions * Sc_main_cb->sc_acc;
	/* The newer qef_dsh_memory parameter is preferred, but if it wasn't
	** set, use the old way (via qef_qep_mem).
	*/
	if (qef_cb.qef_dsh_maxmem == 0)
	{
	    /* check for possible overflow */
	    if ( ((MAXI4 - 2048)/scd_cb.qef_data_size) < qef_cb.qef_qpmax)
		qef_cb.qef_dsh_maxmem = MAXI4;  
	    else
		qef_cb.qef_dsh_maxmem = 2048 + (qef_cb.qef_qpmax * scd_cb.qef_data_size);
	}
	if (qef_cb.qef_sorthash_memory == 0)
	{
	    SIZE_TYPE	total_sort = qef_cb.qef_sort_maxmem * num_qef_sessions;

	    /* check for possible overflow */
	    if ( ((MAX_SIZE_TYPE/qef_cb.qef_sort_maxmem) >=  num_qef_sessions) &&
		 (((MAX_SIZE_TYPE - total_sort)/qef_cb.qef_hash_maxmem) >= (num_qef_sessions+4)/5))
		qef_cb.qef_sorthash_memory = total_sort +
			qef_cb.qef_hash_maxmem * (num_qef_sessions+4)/5;
	    else
		qef_cb.qef_sorthash_memory = MAX_SIZE_TYPE;
	}
	qef_cb.qef_sess_max = num_qef_sessions;
	qef_cb.qef_maxtup = 256*1024;

	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
	    qef_cb.qef_s1_distrib = DB_2_DISTRIB_SVR;
	else
	    qef_cb.qef_s1_distrib = DB_1_LOCAL_SVR;
	if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
	    qef_cb.qef_flag_mask |= QEF_F_C2SECURE;
	qef_cb.qef_qescb_offset = CL_OFFSETOF(SCD_SCB, scb_sscb) +
		CL_OFFSETOF(SCS_SSCB, sscb_qescb);
	status = qef_call(QEC_INITIALIZE, &qef_cb);
	if (status)
	{
	    sc0ePut(&qef_cb.error, 0, NULL, 0);
	    break;
	}
	Sc_main_cb->sc_qebytes = qef_cb.qef_ssize;
	Sc_main_cb->sc_qevcb = qef_cb.qef_server;
	Sc_main_cb->sc_facilities |= 1 << DB_QEF_ID;

	psq_cb.psq_mxsess = num_psf_sessions;
        if (opf_cb.opf_mxsess == 0)
            opf_cb.opf_mxsess = (num_opf_sessions + 1)/2; /* this
                                        ** value is necessary in
                                        ** order to determine value
                                        ** for PSQ_NOTIMEOUTERROR */
        if (opf_cb.opf_mxsess < num_opf_sessions)
            psq_cb.psq_flag |= PSQ_NOTIMEOUTERROR; /* PSF should
                                        ** not allow timeout=0 on
                                        ** certain tables */

	if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
	    psq_cb.psq_version |= PSQ_C_C2SECURE;

        /* If psf.memory specified, pass it to parser startup */
        if (Sc_main_cb->sc_psf_mem)
            psq_cb.psq_mserver = Sc_main_cb->sc_psf_mem;
	psq_cb.psq_vch_prec = Sc_main_cb->sc_psf_vch_prec;

	/* Give PSF the offset to session's PSS_SESBLK pointer */
	psq_cb.psq_psscb_offset = CL_OFFSETOF(SCD_SCB, scb_sscb) +
		CL_OFFSETOF(SCS_SSCB, sscb_psscb);

	status = psq_call(PSQ_STARTUP, &psq_cb, NULL);
	if (status)
	{
	    sc0ePut(&psq_cb.psq_error, 0, NULL, 0);
	    break;
	}
	Sc_main_cb->sc_psvcb = psq_cb.psq_server;
	Sc_main_cb->sc_psbytes = psq_cb.psq_sesize;
	Sc_main_cb->sc_facilities |= 1 << DB_PSF_ID;

	opf_cb.opf_actsess = num_opf_sessions;
	opf_cb.opf_maxpage = Sc_main_cb->sc_maxpage;
	opf_cb.opf_maxtup = 256*1024;
	for (i = 0; i < DB_MAX_CACHE; i++)
	{
	  opf_cb.opf_pagesize[i] = Sc_main_cb->sc_pagesize[i];
	  opf_cb.opf_tupsize[i] = Sc_main_cb->sc_tupsize[i];
	}
	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
	{
	    opf_cb.opf_smask = 0;
	    opf_cb.opf_ddbsite = (PTR)&Sc_main_cb->sc_ii_vnode[0];
	}

	/* Give OPF the offset to session's OPS_CB pointer */
	opf_cb.opf_opscb_offset = CL_OFFSETOF(SCD_SCB, scb_sscb) +
		CL_OFFSETOF(SCS_SSCB, sscb_opscb);

	status = opf_call(OPF_STARTUP, &opf_cb);
	if (status)
	{
	    sc0ePut(&opf_cb.opf_errorblock, 0, NULL, 0);
	    break;
	}
	Sc_main_cb->sc_opvcb = opf_cb.opf_server;
	Sc_main_cb->sc_opbytes = opf_cb.opf_sesize;
	Sc_main_cb->sc_facilities |= 1 << DB_OPF_ID;

	/* QSF */
	qsf_rcb.qsf_type = QSFRB_CB;
	qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
	qsf_rcb.qsf_length = sizeof(qsf_rcb);
	qsf_rcb.qsf_owner = (PTR) DB_SCF_ID;
	qsf_rcb.qsf_server = 0;
	qsf_rcb.qsf_max_active_ses = num_qsf_sessions;
	qsf_rcb.qsf_qsscb_offset = CL_OFFSETOF(SCD_SCB, scb_sscb) +
		CL_OFFSETOF(SCS_SSCB, sscb_qsscb);
	status = qsf_call(QSR_STARTUP, &qsf_rcb);
	if (status)
	{
	    sc0ePut(&qsf_rcb.qsf_error, 0, NULL, 0);
	    break;
	}
	Sc_main_cb->sc_qsvcb = qsf_rcb.qsf_server;
	Sc_main_cb->sc_qsbytes = qsf_rcb.qsf_scb_sz;
	Sc_main_cb->sc_facilities |= 1 << DB_QSF_ID;

	/*
	** User-added sessions will stall in scs_initiate() as long as 
	** sc_state == SC_INIT, so there's no need for sc_urdy_semaphore.
	*/

# if 0
	/* This has never been used (daveb) */
	
	/* Now set up to start the system initialization thread.	    */

	local_crb.gca_status = 0;
	local_crb.gca_assoc_id = 0;
	local_crb.gca_size_advise = 0;
	local_crb.gca_user_name = DB_SYSINIT_THREAD;
	local_crb.gca_account_name = 0;
	local_crb.gca_access_point_identifier = "NONE";
	local_crb.gca_application_id = 0;

	/*
	** Start init commit thread at higher priority than user sessions.
	*/
	priority = Sc_main_cb->sc_max_usr_priority + 3;
	if (priority > Sc_main_cb->sc_max_priority)
	    priority = Sc_main_cb->sc_max_priority;

	stat = CSadd_thread(priority, (PTR) &local_crb, 
			    SCS_SERVER_INIT, (CS_SID*)NULL, &sys_err);
	if (stat != OK)
	{
	    sc0ePut(NULL, stat, NULL, 0);
	    sc0ePut(NULL, E_SC0245_SERVER_INIT_ADD, NULL, 0);
	    status = E_DB_ERROR;
	    break;
	}
# endif

	if (!(Sc_main_cb->sc_capabilities & SC_STAR_SERVER))
	{
	    status = start_lglk_special_threads(
		    ( Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER ) != 0,
		    scd_cb.cp_timeout,
		    scd_cb.num_logwriters,
		    scd_cb.start_gc_thread,
		    Sc_main_cb->sc_dmcm);
	    if (status)
		break;
	}

	/*
	** Set up the connect request block (crb) to add the DMF Consistency
	** Point Thread.  This thread acts as a write behind process which
	** flushes dirty pages out of the buffer cache.  This is a dummy
	** crb since there is really no GCF connection to this thread.
	**
	** All servers must have a Consistency Point Thread.
	** (Note: Previous to 6.4 this was called the Fast Commit thread
	** and was started only in servers which used Fast Commit protocols)
	*/
	if (!(Sc_main_cb->sc_capabilities & SC_STAR_SERVER))
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_BCP_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 3;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR)&local_crb,
				SCS_SFAST_COMMIT, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC0237_FAST_COMMIT_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	/* Create a thread to handle event notifications */
	if (Sc_main_cb->sc_events > 0)
	{
	    /*
	    ** Initialize event memory in-line.  Currently errors & warnings
	    ** are ignored and set to OK, because the event thread is not
	    ** a vital task.  The error has already been logged but it would
	    ** be better to indicate the server started up with warnings.
	    */
	    status = sce_initialize(0);
	    if (status != E_DB_OK)
	    {
		if (status == E_DB_FATAL)
		    break;
		status = E_DB_OK;
	    }
	    else
	    {

		local_crb.gca_status = 0;
		local_crb.gca_assoc_id = 0;
		local_crb.gca_size_advise = 0;
		local_crb.gca_user_name = DB_EVENT_THREAD;
		local_crb.gca_account_name = 0;
		local_crb.gca_access_point_identifier = "NONE";
		local_crb.gca_application_id = 0;
		/* Start Event thread at the specified Priority
		*/
		priority = Sc_main_cb->sc_event_priority;
		if (priority > Sc_main_cb->sc_max_priority)
		    priority = Sc_main_cb->sc_max_priority;
		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_SEVENT, (CS_SID*)NULL, &sys_err);
		/* Check for error adding thread */
		if (stat != OK)
		{
		    sc0ePut(NULL, stat, &sys_err, 0);
		    sc0ePut(NULL, E_SC0272_EVENT_THREAD_ADD, NULL, 0);
		    status = E_DB_OK;	/* Somehow set E_DB_WARNING ?? */
		}
	    }
	}

	/*
	** Call the buffer manager to start a write behind thread
	** in each cache for which they're defined.
	*/
	{
	    DMC_CB	dmc;

	    dmc.type = DMC_CONTROL_CB;
	    dmc.length = sizeof(DMC_CB);

	    stat = dmf_call(DMC_START_WB_THREADS, &dmc);
	    
	    /* Check for error starting WriteBehind threads */
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, NULL, 0);
		sc0ePut(NULL, E_SC0243_WRITE_BEHIND_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }
	}
	/*
	** If server is configured to run replicator queue management threads, 
	** then add those threads.  Set up a dummy connect request block
	** to call CSadd_thread.
	*/
	if (Sc_main_cb->sc_rep_qman)
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_REP_QMAN_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start write behind threads at same priority as user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority;

	    stat = OK;
	    for (i = 0; i < Sc_main_cb->sc_rep_qman; i++)
	    {
		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_REP_QMAN, (CS_SID*)NULL, &sys_err);
		if (stat != OK)
		    break;
	    }

	    /* Check for error adding thread */
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC037D_REP_QMAN_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }
	}
	/*
	**  STAR needs to start a thread to clean-up all orphan DX's,
	**  if the server stared with the option /nostarrecovery do not
	**  start the thread.
	*/
	if ((Sc_main_cb->sc_capabilities & SC_STAR_SERVER) &&
	    (Sc_main_cb->sc_nostar_recovr == FALSE))
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_aux_data = (PTR)NULL;
	    local_crb.gca_assoc_id = -1;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_DXRECOVERY_THREAD;
	    local_crb.gca_password  = (char *)NULL;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;
	    
	    /* Start recovery thread at higher priority than user sessions. */
	    priority = Sc_main_cb->sc_max_usr_priority + 2;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = OK;
	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_S2PC, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC0314_RECOVER_THREAD_ADD, NULL, 0);
		break;
	    }
	}

	/*
	** If this server is running with security auditing enabled
	** we need to start up the auditing thread now.
	*/
	if ( secaudit_writer )
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_AUDIT_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start audit thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 1;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_SAUDIT, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC0329_AUDIT_THREAD_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	if(term_thread)
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_TERM_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start terminator thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 1;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_SCHECK_TERM, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC0340_TERM_THREAD_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	if(secaudit_writer)
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_SECAUD_WRITER_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start audit writer thread at user priority.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_SSECAUD_WRITER, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC0360_SEC_WRITER_THREAD_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** Add write-along  threads. Set up a dummy connect request block
	** to call CSadd_thread.
	*/
	if (Sc_main_cb->sc_writealong) 
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_BWRITALONG_THREAD;   
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start write along threads at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 2;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = OK;
	    for (i = 0; i < Sc_main_cb->sc_writealong; i++)
	    {
		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_SWRITE_ALONG, (CS_SID*)NULL, &sys_err);
		if (stat != OK)
		    break;
	    }

	    /* Check for error adding thread */
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC0367_WRITE_ALONG_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** Add read-ahead   threads. Set up a dummy connect request block
	** to call CSadd_thread.
	*/
	if (Sc_main_cb->sc_readahead)  
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_BREADAHEAD_THREAD;   
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /* If IOMASTER use hi pri for readahead, or it may be too,late */
	    /* For regular server, less than user pri, since its better to */
	    /* do what user needs, before doing what we think user needs   */
	    if (Sc_server_type == SC_IOMASTER_SERVER)
	    {
		priority = Sc_main_cb->sc_max_usr_priority + 4;
		if (priority > Sc_main_cb->sc_max_priority)
		    priority = Sc_main_cb->sc_max_priority;
	    }
	    else
		priority = Sc_main_cb->sc_min_usr_priority;

	    stat = OK;
	    for (i = 0; i < Sc_main_cb->sc_readahead; i++)
	    {
		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_SREAD_AHEAD, (CS_SID*)NULL, &sys_err);
		if (stat != OK)
		    break;
	    }

	    /* Check for error adding thread */
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC0368_READ_AHEAD_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	if (Sc_main_cb->sc_capabilities & SC_CSP_SERVER)
	{
	    /*
	    ** Add additional threads required by a CSP enabled RCP 
	    */

	    /* Main CSP thread */
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_CSP_MAIN_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Priority does not need to be much higher than user
	    ** session priority, since if cluster recovery is needed, 
	    ** user sessions will block when making lock requests.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 1;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_CSP_MAIN, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC02C0_CSP_MAIN_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }

	    /* Cluster message monitor */
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_CSP_MSGMON_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /* Priority should be higher than main CSP thread */
	    priority = Sc_main_cb->sc_max_usr_priority + 2;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_CSP_MSGMON, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC02C2_CSP_MSGMON_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }

	    /* Cluster message processing thread */
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_CSP_MSG_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /* Priority should be higher than monitor thread */
	    priority = Sc_main_cb->sc_max_usr_priority + 4;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_CSP_MSGTHR, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC02C4_CSP_MSGTHR_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	break;
    }

#ifdef xDEBUG
    sc0m_check(SC0M_NOSMPH_MASK | SC0M_READONLY_MASK | SC0M_WRITEABLE_MASK, "");
#endif
    if (status)
    {
	/* More detail message logged above */
	return(E_SC0124_SERVER_INITIATE);
    }
    else
    {
	/*
	** Obtain and fix up the configuration name, then log it.
	*/
	char	server_flavor[256];
	char	*tmpptr, *tmpbuf = PMgetDefault(3);
	char	server_type[32];

	tmpptr = (char *)&server_flavor;
	while (*tmpbuf != '\0')
	{
	    if (*tmpbuf != '\\')
		*(tmpptr++) = *tmpbuf;

	    tmpbuf++;
	}
	*tmpptr = '\0';

	STcopy(PMgetDefault(2), server_type);
	CVupper(server_type);
	
	sc0ePut(NULL, E_SC051D_LOAD_CONFIG, NULL, 2,
		 STlength(server_flavor), server_flavor,
		 STlength(server_type), server_type);

	sc0ePut(NULL, E_SC0129_SERVER_UP, 0, 2,
		 STlength(Version), (PTR)Version,
		 STlength(SystemProductName), SystemProductName);
	Sc_main_cb->sc_state = SC_OPERATIONAL;
	return(E_DB_OK);
    }
}

/*{
** Name: scd_terminate	- Shut down the Ingres DBMS Server
**
** Description:
**      This routine shuts down the dbms server.  It assumes that all
**      sessions have been closed, and will fail if any client facilities 
**      fail to shut down. 
** 
**      The facilities are shut down in the reverse order of startup,
**	except for GCF, which is shut down last.
**
** Inputs:
**      None
**
** Outputs:
**      None
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	1-July-1986 (fred)
**          Created on Jupiter.
**	09-oct-1990 (ralph)
**	    6.3->6.5 merge:
**	    09-apr-90 (bryanp)
**		Terminate the GWF facility after DMF has been shut down.
**	04-feb-1991 (neil)
**	    Alerters: Call sce_shutdown when shutting server down.
**	25-sep-92 (markg)
**	    Terminate the SXF facility before DMF is shut down.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	20-sep-1993 (bryanp)
**	    When shutting down a star server, set DB_2_DISTRIB_SVR in qef_cb.
**	31-oct-1996 (canor01)
**	    In scd_terminate(), shut down GCF last to give any running
**	    threads a chance to disconnect.
**	25-Nov-1997 (jenjo02)
**	    Pass PSQ_FORCE when shutting down PSF so that PSF will continue
**	    the shutdown process even if there are open PSF sessions.
**	25-Mar-1998 (jenjo02)
**	    Dump (new) session statistics to the log. Don't re-execute this
**	    code if sc_state == SC_UNINIT.
**	01-may-2008 (smeke01) b118780
**	    Corrected name of sc_avgrows to sc_totrows.
[@history_template@]...
*/
DB_STATUS
scd_terminate(void)
{
    DB_STATUS           status;
    i4			error = 0;
    i4			gca_error;
    i4			i;
    DMC_CB		dmc;
    RDF_CCB		rdf_ccb;
    PSQ_CB		psq_cb;
    OPF_CB		opf_cb;
    QSF_RCB		qsf_rcb;
    QEF_SRCB		qef_cb;
    GCA_TE_PARMS	gca_cb;
    GW_RCB              gw_rcb;
    SXF_RCB		sxf_rcb;

    /* Deja vu all over again? */
    if (Sc_main_cb == 0 || Sc_main_cb->sc_state == SC_UNINIT)
	return(OK);

    /* Mark shutdown flag */
    Sc_main_cb->sc_state = SC_SHUTDOWN;

    if (Sc_main_cb->sc_facilities & (1 << DB_QSF_ID))
    {
	qsf_rcb.qsf_type = QSFRB_CB;
	qsf_rcb.qsf_ascii_id = QSFRB_ASCII_ID;
	qsf_rcb.qsf_length = sizeof(qsf_rcb);
	qsf_rcb.qsf_owner = (PTR) DB_SCF_ID;
	qsf_rcb.qsf_server = Sc_main_cb->sc_qsvcb;
	qsf_rcb.qsf_force = TRUE;	/* get rid of meaningless message */
	status = qsf_call(QSR_SHUTDOWN, &qsf_rcb);
	if (status)
	{
	    sc0ePut(&qsf_rcb.qsf_error, 0, NULL, 0);
	}
	Sc_main_cb->sc_facilities &= ~(1 << DB_QSF_ID);
    }

    if (Sc_main_cb->sc_facilities & (1 << DB_ADF_ID))
    {
	status = adg_shutdown();
	if (status)
	{
	    sc0ePut(NULL, E_SC012C_ADF_ERROR, NULL, 0);
	    error++;
	}
	Sc_main_cb->sc_facilities &= ~(1 << DB_ADF_ID);
    }

    if (Sc_main_cb->sc_facilities & (1 << DB_OPF_ID))
    {
	opf_cb.opf_length = sizeof(opf_cb);
	opf_cb.opf_type = OPFCB_CB;
	opf_cb.opf_owner = (PTR) DB_SCF_ID;
	opf_cb.opf_ascii_id = OPFCB_ASCII_ID;
	opf_cb.opf_server = Sc_main_cb->sc_opvcb;
	status = opf_call(OPF_SHUTDOWN, &opf_cb);
	if (status)
	{
	    sc0ePut(&opf_cb.opf_errorblock, 0, NULL, 0);
	    error++;
	}
	Sc_main_cb->sc_facilities &= ~(1 << DB_OPF_ID);
    }

    if (Sc_main_cb->sc_facilities & (1 << DB_PSF_ID))
    {
	psq_cb.psq_type = PSQCB_CB;
	psq_cb.psq_length = sizeof(psq_cb);
	psq_cb.psq_ascii_id = PSQCB_ASCII_ID;
	psq_cb.psq_owner = (PTR) DB_SCF_ID;
	psq_cb.psq_flag = PSQ_FORCE;
	status = psq_call(PSQ_SHUTDOWN, &psq_cb, NULL);
	if (status)
	{
	    sc0ePut(&psq_cb.psq_error, 0, NULL, 0);
	    error++;
	}
	Sc_main_cb->sc_facilities &= ~(1 << DB_PSF_ID);
    }

    if (Sc_main_cb->sc_facilities & (1 << DB_QEF_ID))
    {

	qef_cb.qef_length = sizeof(qef_cb);
	qef_cb.qef_type = QESRCB_CB;
	qef_cb.qef_owner = (PTR) DB_SCF_ID;
	qef_cb.qef_ascii_id = QESRCB_ASCII_ID;
	qef_cb.qef_eflag = QEF_INTERNAL;
	if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
	    qef_cb.qef_s1_distrib = DB_2_DISTRIB_SVR;
	else
	    qef_cb.qef_s1_distrib = DB_1_LOCAL_SVR;
	status = qef_call(QEC_SHUTDOWN, &qef_cb);
	if (status)
	{
	    sc0ePut(&qef_cb.error, 0, NULL, 0);
	    error++;
	}
	Sc_main_cb->sc_facilities &= ~(1 << DB_QEF_ID);
    }

    if (Sc_main_cb->sc_facilities & (1 << DB_RDF_ID))
    {
	status = rdf_call(RDF_SHUTDOWN, (PTR) &rdf_ccb);
	if (status)
	{
	    sc0ePut(&rdf_ccb.rdf_error, 0, NULL, 0);
	    error++;
	}
	Sc_main_cb->sc_facilities &= ~(1 << DB_RDF_ID);
    }

    if (Sc_main_cb->sc_facilities & (1 << DB_SXF_ID))
    {
	sxf_rcb.sxf_length = sizeof(sxf_rcb);
	sxf_rcb.sxf_cb_type = SXFRCB_CB;
	sxf_rcb.sxf_force = TRUE;
	status = sxf_call(SXC_SHUTDOWN, &sxf_rcb);
	if (status != E_DB_OK)
	{
	    sc0ePut(&sxf_rcb.sxf_error, 0, NULL, 0);
	    error++;
	}
	Sc_main_cb->sc_facilities &= ~(1 << DB_SXF_ID);
    }

    if (Sc_main_cb->sc_facilities & (1 << DB_DMF_ID))
    {
	dmc.type = DMC_CONTROL_CB;
	dmc.length = sizeof(dmc);
	dmc.dmc_op_type = DMC_SERVER_OP;
	dmc.dmc_id = Sc_main_cb->sc_pid;
	status = dmf_call(DMC_STOP_SERVER, &dmc);
	if (status)
	{
	    sc0ePut(&dmc.error, 0, NULL, 0);
	    error++;
	}
	Sc_main_cb->sc_facilities &= ~(1 << DB_DMF_ID);
    }

    if (Sc_main_cb->sc_facilities & (1 << DB_GWF_ID))
    {
        gw_rcb.gwr_type     = GWR_CB_TYPE;
        gw_rcb.gwr_length   = sizeof(GW_RCB);

        status = gwf_call(GWF_TERM, &gw_rcb );
        if (status)
        {
            sc0ePut(&gw_rcb.gwr_error, 0, NULL, 0);
            error++;
        }
        Sc_main_cb->sc_facilities &= ~(1 << DB_GWF_ID);
    }

    status = sce_shutdown();
    if (status)		/* Error already logged */
	error++;

    if (Sc_main_cb->sc_facilities & (1 << DB_GCF_ID))
    {
        status = IIGCa_call(GCA_TERMINATE,
                            (GCA_PARMLIST *)&gca_cb,
                            GCA_SYNC_FLAG,
                            (PTR)0,
                            -1,
                            &gca_error);
        if (status)
        {
            sc0ePut(NULL, gca_error, NULL, 0);
            error++;
        }
        else if (gca_cb.gca_status != E_GC0000_OK)
        {
            sc0ePut(NULL, gca_cb.gca_status, &gca_cb.gca_os_status, 0);
            error++;
        }
        Sc_main_cb->sc_facilities &= ~(1 << DB_GCF_ID);
    }

    /* Dump session statistics to the log */
    TRdisplay("\n%29*- SCF Session Statistics %28*-\n\n");
    TRdisplay("    %32s  Current  Created      Hwm\n", "Session Type");
    for (i = 0; i <= SCS_MAXSESSTYPE; i++)
    {
	if (Sc_main_cb->sc_session_count[i].created)
	{
	    TRdisplay("    %32w %8d %8d %8d\n",
"User,Monitor,Fast Commit,Write Behind,Server Init,Event,\
Two Phase Commit,Recovery,Log Writer,Check Dead,Group Commit,\
Force Abort,Audit,CP Timer,Check Term,Security Audit Writer,\
Write Along,Read Ahead,Replicator Queue Manager,Lock Callback,\
Deadlock Detector,Sampler,Sort,Factotum", i,
		Sc_main_cb->sc_session_count[i].current,
		Sc_main_cb->sc_session_count[i].created,
		Sc_main_cb->sc_session_count[i].hwm);
	}
    }
    TRdisplay("\n%80*-\n");

    Sc_main_cb->sc_state = SC_UNINIT;
    {
	i4		avg;

	avg = (Sc_main_cb->sc_selcnt
		    ? Sc_main_cb->sc_totrows / Sc_main_cb->sc_selcnt
		    : 0);
	sc0ePut(NULL, E_SC0235_AVGROWS, NULL, 2,
		 sizeof(Sc_main_cb->sc_selcnt), (PTR) &Sc_main_cb->sc_selcnt,
		 sizeof(avg), (PTR) &avg);
    }
    if (error)
    {
	return(E_SC0127_SERVER_TERMINATE);
    }
    else
    {
        sc0ePut(NULL, E_SC0128_SERVER_DOWN, NULL, 0);
	return(E_DB_OK);
    }
}

/*{
** Name: scd_dbinfo_fcn	- Function to provide adf dbms_info things
**
** Description:
**      This routine provides dbms information as a response to  
**      a dbms_info() call on the part of a user (via ADF). 
**      It behaves as documented in the ADF specification.
**
** Inputs:
**      dbi                             Ptr to ADF_DBMSINFO block describing
**                                      data desired.
**      dvi                             Input data value (null in all cases)
**      dvr                             Data value for result.
**      error                           Ptr to DB_ERROR struct.
**
** Outputs:
**      *dvr                            Filled in with answer
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      08-Apr-1987 (fred)
**          Created.
**	12-may-1989 (anton)
**	    added collation name dbmsinfo
**      16-jun-89 (fred)
**          Fixed bug with datatype_*_level constants not setting return status.
**	    Changed routine to default to everything worked, and removed
**	    redundant settings.
**	 9-Jul-89 (anton)
**	    Fixed error language dbmsinfo request
**      14-jan-93 (schang)
**          GW merge. MIssed this one in June integration 
**	  5-nov-1991 (rickh)
**	    Get the version string out of the server control block, where it
**	    was stuffed at server initialization.  This allows gateways, at
**	    server startup, to overwrite this version string with their own
**	    release id.
**	07-may-93 (anitap)
**	    Added support for "SET UPDATE_ROWCOUNT" statement.
**	    Fixed compiler warnings.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	21-Jan-94 (iyer)
**	  Add support for dbmsinfo requests 'db_tran_id' and 'db_cluster_node'
**	  Save cluster node name in SC_MAIN_CB.sc_cname.
**	16-sep-1994 (mikem)
**	    bug #58957
**	    Added code to support dbmsinfo('on_error_state').
**	31-mar-1995 (jenjo02)
**	    Added DB_NDMF to possible values in ics_qlang
**	12-Jun-1997 (jenjo02)
**	    Use SHARE sem when walking sc_kdbl database list.
**	10-Jan-2001 (jenjo02)
**	    Initialize qef_sess_id inqef_cb with SID.
**	24-dec-04 (inkdo01)
**	    Added ucollation for Unicode.
**	7-march-2008 (dougi) SIR 119414
**	    Added SC_CACHEDYN for 'cache_dynamic'.
**       7-Jul-2009 (hanal04) Bug 122266
**          Added underlying code for dbmsinfo(on_error_savepoint). Will
**          not currently generate a value as psl_us11_set_nonkw_roll_svpt()
**          prevents the user from specifying on_error rollback savepoint.
**          However, this change will prevent spurious error and will
**          generate the savepoint name if psl_us11_set_nonkw_roll_svpt() is
**          updated.
**	12-Mar-2010 (thaju02) Bug 123440
*/
DB_STATUS
scd_dbinfo_fcn(ADF_DBMSINFO *dbi,
	       DB_DATA_VALUE *dvi,
	       DB_DATA_VALUE *dvr,
	       DB_ERROR *error )
{
    DB_STATUS           status;
    SCD_SCB		*scb;
    union
    {
	SCF_CB              scf_cb;
	QEF_RCB		    qef_cb;
    }	cb;
    SCF_CB	*scf_cb;
    QEF_RCB	*qef_cb;
    SCV_DBCB	*dbcb;
    SCF_SCI	scf_sci;
    i4	i;
    i4		found;
    DB_DB_NAME	dbname;
    char	erlangname[ER_MAX_LANGSTR];
    char	*info_str;
    char	tempid[17];
    char	tranid[33];
    PSQ_CB	psq_cb;

    status = E_DB_OK;
    CLRDBERR(error);

    CSget_scb((CS_SCB **)&scb);
    switch (dbi->dbi_reqcode)
    {
    	case SC_ONERROR_STATE:
        case SC_SVPT_ONERROR:
	case SC_XACT_INFO:
    	case SC_AUTO_INFO:
	case SC_TRAN_ID:
	case SC_ROWCNT_INFO:
	    qef_cb = &cb.qef_cb;
	    qef_cb->qef_length = sizeof(QEF_RCB);
	    qef_cb->qef_type = QEFRCB_CB;
	    qef_cb->qef_ascii_id = QEFRCB_ASCII_ID;
	    qef_cb->qef_eflag = QEF_INTERNAL;
	    qef_cb->qef_cb = scb->scb_sscb.sscb_qescb;
	    qef_cb->qef_sess_id = scb->cs_scb.cs_self;
	    status = qef_call(QEC_INFO, qef_cb);
	    if (dbi->dbi_reqcode == SC_XACT_INFO)
	    {
		*((i4 *) dvr->db_data) =
			(qef_cb->qef_stat == QEF_MSTRAN);
	    }
	    else if (dbi->dbi_reqcode == SC_AUTO_INFO)
	    {
		*((i4 *) dvr->db_data) =
			(qef_cb->qef_auto == QEF_ON);
	    }
	    else if (dbi->dbi_reqcode == SC_TRAN_ID)
	    {
		MEfill(sizeof(tranid)-1, ' ', tranid);
		CVla((i4)qef_cb->qef_tran_id.db_high_tran, tempid);
		MEcopy(tempid, STlength(tempid), &tranid[0]);
		CVla((i4)qef_cb->qef_tran_id.db_low_tran,  tempid);
		MEcopy(tempid, STlength(tempid), &tranid[16]);
		tranid[sizeof(tranid)-1] = EOS;
		info_str = tranid;
		MEmove(STlength(info_str), info_str, ' ', 
						dvr->db_length, dvr->db_data);
	    }
	    else if (dbi->dbi_reqcode == SC_ONERROR_STATE)
	    {
		/* return the error state of the sessions as set by the
		** set session with on_error = "" setatement.
		*/
		if (qef_cb->qef_stm_error == QEF_STMT_ROLLBACK)
		{
		    MEmove(18, "rollback statement", ' ', 
				dvr->db_length, dvr->db_data); 
		}
		else if (qef_cb->qef_stm_error == QEF_XACT_ROLLBACK)
		{
		    MEmove(20, "rollback transaction", ' ', 
				dvr->db_length, dvr->db_data); 
		}
		else if (qef_cb->qef_stm_error == QEF_SVPT_ROLLBACK)
		{
		    MEmove(18, "rollback savepoint", ' ', 
				dvr->db_length, dvr->db_data); 
		}
		else
		{
		    /* should not ever happen */
		    MEmove(19, "error state unknown", ' ', 
				dvr->db_length, dvr->db_data); 
		}
	    }
            else if (dbi->dbi_reqcode == SC_SVPT_ONERROR)
            {
                    MEmove(STlength(qef_cb->qef_spoint->db_sp_name), 
                                qef_cb->qef_spoint->db_sp_name, ' ',
                                dvr->db_length, dvr->db_data);
            }
	    else
	    {
		if (qef_cb->qef_upd_rowcnt == 1)
		   MEmove(7, "changed", ' ', dvr->db_length, dvr->db_data);
		else
		   MEmove(9, "qualified" ,' ', dvr->db_length, dvr->db_data); 
	    }
	    STRUCT_ASSIGN_MACRO(qef_cb->error, *error);
	    break;

	case SC_ET_SECONDS:
	    *((i4 *) dvr->db_data) = TMsecs() - scb->cs_scb.cs_connect;
	    break;

	case SC_LANGUAGE:
	    status = ERlangstr(scb->scb_sscb.sscb_ics.ics_language,
		erlangname);
	    if (status == OK)
	    {
		MEmove(STnlength(ER_MAX_LANGSTR, erlangname), erlangname,
		    ' ', dvr->db_length, dvr->db_data);
	    }
	    else
	    {
		MEmove(9, "<unknown>", ' ', dvr->db_length, dvr->db_data);
	    }
	    break;

	case SC_COLLATION:
	    MEmove(STnlength(sizeof scb->scb_sscb.sscb_ics.ics_collation,
		    scb->scb_sscb.sscb_ics.ics_collation),
		scb->scb_sscb.sscb_ics.ics_collation, ' ',
		dvr->db_length, dvr->db_data);
	    break;

	case SC_UCOLLATION:
	    MEmove(STnlength(sizeof scb->scb_sscb.sscb_ics.ics_ucollation,
		    scb->scb_sscb.sscb_ics.ics_ucollation),
		scb->scb_sscb.sscb_ics.ics_ucollation, ' ',
		dvr->db_length, dvr->db_data);
	    break;

	case SC_CHARSET:
	    CMget_charset_name(dvr->db_data);
	    break;

	case SC_DB_CACHEDYN:
	    /*
	    ** check if set session [no]cache_dynamic issued;
	    ** overrides server-wide setting 
	    */
	    psq_cb.psq_type = PSQCB_CB;
	    psq_cb.psq_length = sizeof(psq_cb);
	    psq_cb.psq_ascii_id = PSQCB_ASCII_ID;
	    psq_cb.psq_owner = (PTR) DB_SCF_ID;
	    psq_cb.psq_sessid = scb->cs_scb.cs_self;
	    psq_cb.psq_ret_flag = 0;
	    status = psq_call(PSQ_GET_SESS_INFO, &psq_cb, 0);
	    if ( ((Sc_main_cb->sc_csrflags & SC_CACHEDYN) &&
                 !(psq_cb.psq_ret_flag & PSQ_SESS_NOCACHEDYN)) ||
		 (psq_cb.psq_ret_flag & PSQ_SESS_CACHEDYN) )
		((char *)dvr->db_data)[0] = 'Y';
	    else ((char *)dvr->db_data)[0] = 'N';
	    break;

	case SC_QLANG:
	    if (scb->scb_sscb.sscb_ics.ics_qlang == DB_SQL)
	    {
		MEmove(3, "sql", ' ', dvr->db_length, dvr->db_data);
	    }
	    else if (scb->scb_sscb.sscb_ics.ics_qlang == DB_QUEL)
	    {
		MEmove(4, "quel", ' ', dvr->db_length, dvr->db_data);
	    }
	    else if (scb->scb_sscb.sscb_ics.ics_qlang == DB_NDMF)
	    {
		MEmove(4, "ndmf", ' ', dvr->db_length, dvr->db_data);
	    }
	    else
	    {
		MEmove(9, "<unknown>", ' ', dvr->db_length, dvr->db_data);
	    }
	    break;

	case SC_SERVER_CLASS:
            if (Sc_main_cb->sc_capabilities & SC_RMS_SERVER)
                MEmove(18, "RMS Gateway Server", ' ', dvr->db_length,
                    dvr->db_data);
            else if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
                MEmove(18, "Ingres Star Server", ' ', dvr->db_length,
                    dvr->db_data);
            else if (Sc_main_cb->sc_capabilities & SC_IOMASTER_SERVER)
                MEmove(18, "I/O Master Server ", ' ', dvr->db_length,
                    dvr->db_data);
            else
                MEmove(18, "Ingres DBMS Server", ' ', dvr->db_length,
                    dvr->db_data);
	    break;

	case SC_DB_COUNT:
	    if (status = CSp_semaphore(0, &Sc_main_cb->sc_kdbl.kdb_semaphore))
	    {
		SETDBERR(error, 0, status);
		break;
	    }
	    for ( dbcb = Sc_main_cb->sc_kdbl.kdb_next, i = 0;
		dbcb->db_next != Sc_main_cb->sc_kdbl.kdb_next;
		dbcb = dbcb->db_next, i++)
	    {
		/* Just a count */
	    }
	    if (status = CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore))
	    {
		SETDBERR(error, 0, status);
		break;
	    }
	    MEcopy((PTR)&i, sizeof(i), dvr->db_data);
	    break;

	case SC_OPENDB_COUNT:
	    if (dvi)
	    {
		MEmove(dvi->db_length,
			dvi->db_data,
			' ',
			sizeof(dbname),
			(char *)&dbname);
		i = 0;
		if (status = CSp_semaphore(0, &Sc_main_cb->sc_kdbl.kdb_semaphore))
		{
		    SETDBERR(error, 0, status);
		    break;
		}
		for ( dbcb = Sc_main_cb->sc_kdbl.kdb_next, found = 0;
		    dbcb->db_next != Sc_main_cb->sc_kdbl.kdb_next;
		    dbcb = dbcb->db_next)
		{
		    if ((MEcmp((PTR)&dbcb->db_dbname,
					(PTR)&dbname,
					sizeof(dbcb->db_dbname))) == 0)
		    {
			found = 1;
			i = dbcb->db_ucount;
			break;
		    }
		}
		if (status = CSv_semaphore(&Sc_main_cb->sc_kdbl.kdb_semaphore))
		{
		    SETDBERR(error, 0, status);
		    break;
		}
	    }
	    else
	    {
		i = -1;
	    }
	    MEcopy((PTR)&i, sizeof(i), dvr->db_data);
	    break;

	case SCI_IVERSION:
	    MEmove(	sizeof( Sc_main_cb->sc_iversion ),
			Sc_main_cb->sc_iversion,
			' ',
			dvr->db_length,
			dvr->db_data);
	    break;

	case SC_MAJOR_LEVEL:
	    MEcopy((PTR) &Sc_main_cb->sc_major_adf,
			    dvr->db_length, dvr->db_data);
	    break;

	case SC_MINOR_LEVEL:
	    MEcopy((PTR) &Sc_main_cb->sc_minor_adf,
			    dvr->db_length, dvr->db_data);
	    break;

	case SC_CLUSTER_NODE:
	    MEmove(sizeof(Sc_main_cb->sc_cname),
			Sc_main_cb->sc_cname,
			' ',
			dvr->db_length,
			dvr->db_data);
	    break;
	    
	default:
	    scf_cb = &cb.scf_cb;
	    scf_cb->scf_length = sizeof(SCF_CB);
	    scf_cb->scf_type = SCF_CB_TYPE;
	    scf_cb->scf_owner = (PTR) DB_SCF_ID;
	    scf_cb->scf_ascii_id = SCF_ASCII_ID;
	    scf_cb->scf_facility = DB_SCF_ID;
	    scf_cb->scf_session = scb->cs_scb.cs_self;
	    scf_cb->scf_len_union.scf_ilength = 1;
	    scf_cb->scf_ptr_union.scf_sci = (SCI_LIST *) &scf_sci;
	    scf_sci.sci_length = dvr->db_length;
	    scf_sci.sci_code = dbi->dbi_reqcode;
	    scf_sci.sci_aresult = dvr->db_data;
	    scf_sci.sci_rlength = &dvr->db_length;
	    status = scf_call(SCU_INFORMATION, scf_cb);
	    *error = scf_cb->scf_error;
	    break;
    }

    return(status);
}

/*
** Name: start_lglk_special_threads	- start the LG/LK special threads.
**
** Description:
**	This routine is called to start up the various special threads which
**	perform LG/LK utility operations.
**
** Inputs:
**	recovery_server			indicates whether this is the recovery
**					server (in which case it gets a
**					recovery thread).
**	cp_timeout			If recovery_server is non-zero, then
**					cp_timeout indicates whether timer-
**					initiated consistency points are
**					desired.
**	num_logwriters			indicates number of logwriter threads
**					to start.
**	start_gc_thread			non-zero if we should start a GC thread
**      start_lkcback_thread            non-zero if we should start a LK
**
** Outputs:
**	None
**
** Returns:
**	DB_STATUS
**
** Side Effects:
**	Special threads are started.
**
** History:
**	22-jul-1992 (bryanp)
**	    Created.
**	12-nov-1992 (bryanp)
**	    Properly initialize status code in start_lglk_special_threads.
**	18-jan-1993 (bryanp)
**	    Add support for timer-initiated consistency points.
**	2-Jul-1993 (daveb)
**	    remove unused var 'error'
**	26-jul-1993 (bryanp)
**	    If /GC_INTERVAL is 0, don't start a group commit thread.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Start up the LK Blocking Callback Thread in recovery and database
**          servers.
**	15-nov-1996 (nanpr01)
**	    As per Jon Jensen's request, change the code to bring up
**	    LK callback thread only when it is required.
**	16-Nov-1996 (jenjo02)
**	    Added Deadlock Detector thread (SCS_SDEADLOCK) to the recovery server.
*/	
static DB_STATUS
start_lglk_special_threads(
i4	recovery_server,
i4	cp_timeout,
i4	num_logwriters,
i4	start_gc_thread,
i4	start_lkcback_thread)
{
    DB_STATUS           status;
    CL_SYS_ERR	        sys_err;
    STATUS		stat;
    GCA_LS_PARMS	local_crb;
    i4			priority;

    for (;;)
    {
	/*
	** If this is a recovery server, then start the recovery thread.
	** The recovery thread in the recovery server is responsible for
	** recovering after failed servers, for handling pass-aborts, and
	** for general logging and locking system management.
	*/
	if (recovery_server)
	{
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_RECOV_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start recovery thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 3;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_SRECOVERY, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC031D_RECOVERY_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }

	    if (cp_timeout)
	    {
		/*
		** Start the cp timer thread. The cp timer thread is a
		** background thread which periodically requests a consistency
		** point.
		*/
		local_crb.gca_status = 0;
		local_crb.gca_assoc_id = 0;
		local_crb.gca_size_advise = 0;
		local_crb.gca_user_name = DB_CPTIMER_THREAD;
		local_crb.gca_account_name = 0;
		local_crb.gca_access_point_identifier = "NONE";
		local_crb.gca_application_id = 0;

		/*
		** Start cp timer thread at higher priority than user sessions.
		*/
		priority = Sc_main_cb->sc_max_usr_priority + 1;
		if (priority > Sc_main_cb->sc_max_priority)
		    priority = Sc_main_cb->sc_max_priority;

		stat = CSadd_thread(priority, (PTR)&local_crb,
				    SCS_SCP_TIMER, (CS_SID*)NULL, &sys_err);
		if (stat != OK)
		{
		    sc0ePut(NULL, stat, &sys_err, 0);
		    sc0ePut(NULL, E_SC032F_CP_TIMER_ADD, NULL, 0);
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /*
	    ** Start the Deadlock Detector thread. This thread 
	    ** periodically wakes up to resolve lock deadlocks.
	    */
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_DEADLOCK_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start deadlock thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 4;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;
	    
	    stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_SDEADLOCK, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC0371_DEADLOCK_THREAD_ADD, NULL, 0); 
		status = E_DB_ERROR;
		break;
	    }

	    if ( CXcluster_enabled() &&
	         !CXconfig_settings( CX_HAS_DEADLOCK_CHECK ) )
	    {
		/*
		** We're clustered, but the underlying DLM cannot
		** handle deadlock by itself.  Add a thread to
		** support our own distributed deadlock mgmt.
		*/
		(void)LKalter(LK_A_ENABLE_DIST_DEADLOCK, 1, &sys_err);

		/* Start Dist deadlock thread just under Local deadlock priority */
		priority--;

		local_crb.gca_status = 0;
		local_crb.gca_assoc_id = 0;
		local_crb.gca_size_advise = 0;
		local_crb.gca_user_name = DB_DIST_DEADLOCK_THREAD;
		local_crb.gca_account_name = 0;
		local_crb.gca_access_point_identifier = "NONE";
		local_crb.gca_application_id = 0;

		stat = CSadd_thread(priority, (PTR) &local_crb, 
				SCS_SDISTDEADLOCK, (CS_SID*)NULL, &sys_err);
		if (stat != OK)
		{
		    sc0ePut(NULL, stat, &sys_err, 0);
		    sc0ePut(NULL, E_SC0396_DIST_DEADLOCK_THR_ADD, NULL, 0); 
		    status = E_DB_ERROR;
		    break;
		}
	    }
	}

	if ( CXcluster_enabled() &&
	     !CXconfig_settings( CX_HAS_DLM_GLC ) )
	{
	    /*
	    ** Start GLC thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 4;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;
	    
	    /*
	    ** We're clustered, but the underlying DLM cannot
	    ** handle group lock containers by itself.  Add thread
	    ** to support our own GLC simulator.
	    */
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_GLC_SUPPORT_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
			    SCS_SGLCSUPPORT, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC0398_GLC_SUPPORT_THR_ADD, NULL, 0); 
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** Start the check-dead thread. The check-dead thread is a background
	** thread which periodically checks to see if any other backend
	** processes have unexpectedly died.
	*/
	local_crb.gca_status = 0;
	local_crb.gca_assoc_id = 0;
	local_crb.gca_size_advise = 0;
	local_crb.gca_user_name = DB_DEADPROC_THREAD;
	local_crb.gca_account_name = 0;
	local_crb.gca_access_point_identifier = "NONE";
	local_crb.gca_application_id = 0;

	/*
	** Start check-dead thread at higher priority than user sessions.
	*/
	priority = Sc_main_cb->sc_max_usr_priority + 1;
	if (priority > Sc_main_cb->sc_max_priority)
	    priority = Sc_main_cb->sc_max_priority;

	stat = CSadd_thread(priority, (PTR) &local_crb, 
			    SCS_SCHECK_DEAD, (CS_SID*)NULL, &sys_err);
	if (stat != OK)
	{
	    sc0ePut(NULL, stat, &sys_err, 0);
	    sc0ePut(NULL, E_SC0321_CHECK_DEAD_ADD, NULL, 0);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Start the force-abort thread. The force-abort thread is a background
	** thread which is awakened if a user transaction in this server needs
	** be aborted because the log file is full.
	*/
	local_crb.gca_status = 0;
	local_crb.gca_assoc_id = 0;
	local_crb.gca_size_advise = 0;
	local_crb.gca_user_name = DB_FORCEABORT_THREAD;
	local_crb.gca_account_name = 0;
	local_crb.gca_access_point_identifier = "NONE";
	local_crb.gca_application_id = 0;

	/*
	** Start force-abort thread at higher priority than user sessions.
	*/
	priority = Sc_main_cb->sc_max_usr_priority + 1;
	if (priority > Sc_main_cb->sc_max_priority)
	    priority = Sc_main_cb->sc_max_priority;

	stat = CSadd_thread(priority, (PTR) &local_crb, 
			    SCS_SFORCE_ABORT, (CS_SID*)NULL, &sys_err);
	if (stat != OK)
	{
	    sc0ePut(NULL, stat, &sys_err, 0);
	    sc0ePut(NULL, E_SC0325_FORCE_ABORT_ADD, NULL, 0);
	    status = E_DB_ERROR;
	    break;
	}

	if (start_gc_thread)
	{
	    /*
	    ** Start the group-commit thread. The group-commit thread is
	    ** awakened if a logfile buffer needs to initiate the Group Commit
	    ** protocol.
	    */
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_GROUPCOMMIT_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start Group Commit thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 4;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR)&local_crb,
				SCS_SGROUP_COMMIT, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC0323_GROUP_COMMIT_ADD, NULL, 0);
		status = E_DB_ERROR;
		break;
	    }
	}

	status = E_DB_OK;
	while (num_logwriters > 0)
	{
	    /*
	    ** Start the logfile writer thread. This thread writes blocks
	    ** to the logfile.
	    */
	    local_crb.gca_status = 0;
	    local_crb.gca_assoc_id = 0;
	    local_crb.gca_size_advise = 0;
	    local_crb.gca_user_name = DB_LOGWRITE_THREAD;
	    local_crb.gca_account_name = 0;
	    local_crb.gca_access_point_identifier = "NONE";
	    local_crb.gca_application_id = 0;

	    /*
	    ** Start logwriter thread at higher priority than user sessions.
	    */
	    priority = Sc_main_cb->sc_max_usr_priority + 4;
	    if (priority > Sc_main_cb->sc_max_priority)
		priority = Sc_main_cb->sc_max_priority;

	    stat = CSadd_thread(priority, (PTR) &local_crb, 
	    			SCS_SLOGWRITER, (CS_SID*)NULL, &sys_err);
	    if (stat != OK)
	    {
		sc0ePut(NULL, stat, &sys_err, 0);
		sc0ePut(NULL, E_SC031F_LOGWRITER_ADD, NULL, 0); /* need new msg */
		status = E_DB_ERROR;
		break;
	    }
	    num_logwriters--;
	    status = E_DB_OK;
	}
	if (status != E_DB_OK)
	    break;

	if (start_lkcback_thread)
	{
            /*
            ** Start the LK blocking callback thread. This thread runs queued LK
            ** callbacks for this process.
            */
            local_crb.gca_status = 0;
            local_crb.gca_assoc_id = 0;
            local_crb.gca_size_advise = 0;
            local_crb.gca_user_name = DB_LK_CALLBACK_THREAD;
            local_crb.gca_account_name = 0;
            local_crb.gca_access_point_identifier = "NONE";
            local_crb.gca_application_id = 0;

            /*
            ** Start callback thread at higher priority than user sessions.
            */
            priority = Sc_main_cb->sc_max_usr_priority + 4;
            if (priority > Sc_main_cb->sc_max_priority)
                priority = Sc_main_cb->sc_max_priority;
        
            stat = CSadd_thread(priority, (PTR) &local_crb, 
                            SCS_SLK_CALLBACK, (CS_SID*)NULL, &sys_err);
            if (stat != OK)
            {
		sc0ePut(NULL, stat, &sys_err, 0);
                sc0ePut(NULL, E_SC037E_LK_CALLBACK_THREAD_ADD, NULL, 0); 
                status = E_DB_ERROR;
                break;
            }
	}
	status = E_DB_OK;

	break;
    }
    return (status);
}

/*{
** Name: scd_adf_printf	- Function to print trace message for ADF.
**
** Description:
**      This routine takes an input string and print it using SCC_TRACE. 
**      
** Inputs:
**      cbuf                            NULL terminated input string to
**                                      print.
** Outputs:
**      none
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-sep-1992 (stevet)
**          Initial creation, largely stolen from adedebug.c.
**	2-Jul-1993 (daveb)
**	    prototyped.
[@history_template@]...
*/

DB_STATUS
scd_adf_printf( char *cbuf )
{
    SCF_CB     scf_cb;
    DB_STATUS  scf_status;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_ADF_ID;
    scf_cb.scf_nbr_union.scf_local_error = 0;
    scf_cb.scf_len_union.scf_blength = STnlength(STlength(cbuf), cbuf);
    scf_cb.scf_ptr_union.scf_buffer = cbuf;
    CSget_sid((CS_SID*)&scf_cb.scf_session);
    scf_status = scf_call(SCC_TRACE, &scf_cb);

    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error displaying an ADF message to user\n");
	TRdisplay("The ADF message is :%s",cbuf);
    }
    return( scf_status);
}

/*{
** Name: scd_sec_init_config	- Initialize security configuration
**
** Description:
**      This routine initializes the security configuration from PM
**      
** Inputs:
**
**	c2_server	TRUE is C2
**
** Outputs:
**      none
**
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    Initializes Sc_main_cb security values.
**
** History:
**      12-jul-93 (robf)
**          Initial creation.
**	6-dec-93 (robf)
**          Changed ENABLED/DISABLED to ON/OFF per LRC.
**      23-feb-94  (robf)
**          Report invalid ROLE_PASSWORD setting as ROLE_PASSWORD in 
**	    the error message rather than  ROLES to distinguish the 
**	    two options. Also echo the options to the error log for
**          consistency with other values. 
[@history_template@]...
*/
static DB_STATUS
scd_sec_init_config(
	bool	c2_secure_server
)
{
	char *pmvalue;
	bool echo=TRUE;

	if(PMget("!.echo",&pmvalue)==OK)
	{
		if(!STcasecmp(pmvalue, "off"))
			echo=FALSE;
	}
	/* Row audit key */
	if (PMget("II.$.SECURE.ROW_AUDIT_KEY", &pmvalue) == OK)
	{
		if (STcasecmp(pmvalue, "AUTOMATIC")==0) 
		{
			Sc_main_cb->sc_secflags|=SC_SEC_ROW_KEY;
		}
		else if (STcasecmp(pmvalue, "OPTIONAL")==0) 
		{
			Sc_main_cb->sc_secflags&= ~SC_SEC_ROW_KEY;
		}
		else
		{
		        sc0ePut(NULL, E_SC0339_SECURE_INVALID_PARM, NULL, 1,
				sizeof("ROW_AUDIT_KEY")-1,
				"ROW_AUDIT_KEY");
			return(E_SC0124_SERVER_INITIATE);
		}
		if(echo)
			ERoptlog( "row_audit_key", pmvalue );
	}
	/* Row auditing enabled/disabled */
	if (PMget("II.$.SECURE.ROW_AUDIT_DEFAULT", &pmvalue) == OK)
	{
		if (STcasecmp(pmvalue, "ON")==0) 
		{
			Sc_main_cb->sc_secflags|=SC_SEC_ROW_AUDIT;
		}
		else if (STcasecmp(pmvalue, "OFF")==0) 
		{
			Sc_main_cb->sc_secflags&=~SC_SEC_ROW_AUDIT;
		}
		else
		{
		       sc0ePut(NULL, E_SC0339_SECURE_INVALID_PARM, NULL, 1,
				sizeof("ROW_AUDIT_DEFAULT")-1,
				"ROW_AUDIT_DEFAULT");
			return(E_SC0124_SERVER_INITIATE);
		}
		if(echo)
			ERoptlog( "row_audit_default", pmvalue );
	}
	/* Password allowed */
	if (PMget("II.$.SECURE.USER_PASSWORD", &pmvalue) == OK)
	{
		if (STcasecmp(pmvalue, "OPTIONAL")==0) 
		{
			Sc_main_cb->sc_secflags&=~SC_SEC_PASSWORD_NONE;
		}
		else if(STcasecmp(pmvalue, "NONE")==0)
		{
			Sc_main_cb->sc_secflags|=SC_SEC_PASSWORD_NONE;
		}
		else
		{
			/*
			** Invalid password
			*/
		       sc0ePut(NULL, E_SC0339_SECURE_INVALID_PARM, NULL, 1,
				sizeof("USER_PASSWORD")-1,
				"USER_PASSWORD");
		       return(E_SC0124_SERVER_INITIATE);
		}
		if(echo)
			ERoptlog( "user_password", pmvalue );
	}
	/* Roles allowed */
	if (PMget("II.$.SECURE.ROLES", &pmvalue) == OK)
	{
		if (STcasecmp(pmvalue, "ON")==0) 
		{
			Sc_main_cb->sc_secflags&=~SC_SEC_ROLE_NONE;
		}
		else if(STcasecmp(pmvalue, "OFF")==0)
		{
			Sc_main_cb->sc_secflags|=SC_SEC_ROLE_NONE;
		}
		else
		{
			/*
			** Invalid option for ROLES
			*/
		       sc0ePut(NULL, E_SC0339_SECURE_INVALID_PARM, NULL, 1,
				sizeof("ROLES")-1, "ROLES");
		       return(E_SC0124_SERVER_INITIATE);
		}
		if(echo)
			ERoptlog( "roles", pmvalue );
	}
	if( ! (Sc_main_cb->sc_secflags & SC_SEC_ROLE_NONE))
	{
	    if (PMget("II.$.SECURE.ROLE_PASSWORD", &pmvalue) == OK)
	    {
	    	if (STcasecmp(pmvalue, "OPTIONAL" )==0) 
	    	{
	    		Sc_main_cb->sc_secflags&=~SC_SEC_ROLE_NEED_PW;
	    	}
	    	else if(STcasecmp(pmvalue, "REQUIRED")==0)
	    	{
	    		Sc_main_cb->sc_secflags|=SC_SEC_ROLE_NEED_PW;
	    	}
	    	else
	    	{
	    		/*
	    		** Invalid password
	    		*/
	       	       sc0ePut(NULL, E_SC0339_SECURE_INVALID_PARM, NULL, 1,
	    			sizeof("ROLE_PASSWORD")-1, "ROLE_PASSWORD");
	    	       return(E_SC0124_SERVER_INITIATE);
	    	}
		if(echo)
			ERoptlog( "role_password", pmvalue );
	    }
	}
	/* OME allowed */
	if (PMget("II.$.SECURE.OME", &pmvalue) == OK)
	{
		if (STcasecmp(pmvalue, "ON")==0) 
		{
			Sc_main_cb->sc_secflags &= ~SC_SEC_OME_DISABLED;
		}
		else if(STcasecmp(pmvalue, "OFF")==0)
		{
			Sc_main_cb->sc_secflags |= SC_SEC_OME_DISABLED;
		}
		else
		{
			/*
			** Invalid option for OME
			*/
		       sc0ePut(NULL, E_SC0339_SECURE_INVALID_PARM, NULL, 1,
				sizeof("OME")-1, "OME");
		       return(E_SC0124_SERVER_INITIATE);
		}
		if(echo)
			ERoptlog( "ome", pmvalue );
	}
	return E_DB_OK;
}

/*
** Name: scd_init_sngluser - Init a single user server for SCF
**
** Description:
**	The rouitne initializes SCF for a single user server. Currently
**	all single user servers are run from DMF. With the addition of
**	the fastload function to the JSP, and the subseqnet support for
**	long datatypes, which require couponification, calls are made
**	to SCF from a single user server (for information gathering
**	rather than thread management). This routine initializes the SCF
**	control blocks that are required for these calls.
**
** Inputs:
**	scf_cb - SCF call control block
**
** Outputs:
**	None
**
** Returns:
**	status
**
** History:
**	30-jan-97 (stephenb)
**	    Initial creation.
**	04-Jan-2001 (jenjo02)
**	    Pass SCD_SCB* as parameter to scd_init_sngluser().
**	08-Jan-2001 (jenjo02)
**	    Return offset of DML_SCB pointer to caller.
*/
DB_STATUS
scd_init_sngluser(
	SCF_CB		*scf_cb,
	SCD_SCB		*scb)
{
    /* set DML_SCB */
    scb->scb_sscb.sscb_dmscb = scf_cb->scf_ptr_union.scf_dml_scb;

    /* Return offset of DML_SCB pointer to caller */
    scf_cb->scf_len_union.scf_ilength = CL_OFFSETOF(SCD_SCB, scb_sscb) +
	    CL_OFFSETOF(SCS_SSCB, sscb_dmscb);

    return(E_DB_OK);
}
