/*
** Copyright (c) 1986, 2010 Ingres Corporation
**
**
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <ci.h>
#include    <sl.h>
#include    <cm.h>
#include    <er.h>
#include    <ex.h>
#include    <id.h>
#include    <lo.h>
#include    <nm.h>
#include    <pc.h>
#include    <pm.h>
#include    <si.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <me.h>
#include    <cs.h>
#include    <lk.h>
#include    <cx.h>
#include    <cv.h>
#include    <gc.h>
#include    <gcccl.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <erusf.h>
#include    <ulf.h>
#include    <gca.h>
#include    <generr.h>
#include    <sqlstate.h>
#include    <ddb.h>
#include    <ulm.h>

#include    <dudbms.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <qsf.h>
#include    <adf.h>
#include    <opfcb.h>
#include    <qefrcb.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <rdf.h>
#include    <sxf.h>
#include    <gwf.h>
#include    <lg.h>

#include    <copy.h>
#include    <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>

/**
**
**  Name: SCDMAIN.C - Entry point and support routines for the DBMS Server
**
**  Description:
**      This file contains the main entry point (main) for the server 
**      as well as the associated support routines.
**
**          main() - main entry point
**          scd_alloc_scb() - Allocate an SCB
**          scd_dealloc_scb() - Deallocate an SCB
**	    scd_reject_assoc() - Reject an association
**	    scd_get_assoc() - Await new association
**	    scd_disconnect() - Disconnect an association
**
**
**  History:
**      25-Nov-1986 (fred)
**          Created.
**      23-Mar-1989 (fred)
**          Added code to initialize the server init thread.
**      28-Mar-1989 (fred)
**          Fixup scd_disconnect() so that it won't disconnect unconnected
**	    threads.
**	21-Jun-1989 (nancy)
**	    fix bug #4832 in scd_alloc_scb() 
**      07-aug-1989 (fred)
**	    Added DYNLIBS hint for MING on UNIX.
**      05-apr-1989 (jennifer)
**          Add code to store security label in SCS_ICS.
**	10-aug-89 (jrb)
**	    Added protocol version level check when sending errors through gca.
**	26-sep-89 (cal)
**	    Update MODE,PROGRAM, and DEST ming hints for user defined
**	    datatypes.
**	26-dec-89 (greg)
**	    Wrap main for II_DMF_MERGE
**	    Use CL_ERR_DESC not i4 with TRset_file()
**	    Use STRUCT_ASSIGN_MACRO for gca_os_status
**	20-Feb-1990 (anton)
**	    Don't log E_GC0032_NO_PEER - Common condition result of name server
**	    bed check.
**      08-aug-1990 (ralph)
**          Fix #9849: Track consecutive GCA_LISTEN failures in scd_get_assoc;
**          if > SC_GCA_LISTEN_MAX then shut the server down gracefully
**	09-oct-1990 (ralph)
**	    6.3->6.5 merge: add ming hint for gwf
**      08-nov-1990 (ralph)
**          Align major structures on ALIGN_RESTRICT boundaries in scd_alloc_scb
**	15-jan-1991 (ralph)
**	    Cast ME_ALIGN_MACRO to i4 to avoid compiler problems on UNIX
**	04-feb-1991 (neil)
**	    Add logic to initialize event thread (new thread).
**	    Preallocated the cscb_evdata message for alerters.
**	09-Dec-1990 (anton)
**	    Semaphores are needed in scd_deallocate_scb
**	07-Jan-1991 (anton)
**	    Call MEadvise with the new ME_INGRES_THREAD_ALLOC
**	29-Jan-91 (anton)
**	    Add support for GCA API 2
**	    Add calls to scs_disassoc
**	07-nov-1991 (ralph)
**	    6.4->6.5 merge:
**		08-aug-1990 (jonb)
**		    Add spaces in front of "main(" so mkming won't think this
**		    is a main routine on UNIX machines where II_DMF_MERGE is
**		    always defined (reintegration of 14-mar-1990 change).
**	    	09-jan-1991 (stevet)
**		    Added CMset_attr call to initialize CM attribute table.
**		9-aug-91 (stevet)
**		    Change read-in character set support to use II_CHARSET
**		    logical.
**	9-Aug-91, 27-feb-92 (daveb)
**	    Fill in new cs_client_type field to unanbigiously identify
**	    CS_SCB's as SCD_SCBs, to fix bug 38056; cause by async
**	    GCA API 2 dissasoc added above.  Also, change test elsewhere
**	    to use the cs_client_type instead of the unreliable cs_length.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              07-aug-89 (georgeg)
**                  Fix copied from Jupiter line, fixed by (fred).
**                  Fixed scd_disconnect() and scd_reject_assoc() to set 
**		    gca_local_error to 0 for correct handling of generic 
**		    errors in the FE.
**              08-nov-89 (georgeg)
**                  Send generic errors only if the GCA_PROTOCOL_LEVEL is high 
**		    enough.
**              17-sep-90 (fpang)
**                  Fixed ming hints. Libraries are FAC followed by SLIB, not 
**		    LIB.
**              01-nov-90 (fpang)
**                  TRsetfile status parameter should be of type CL_STATUS, not
**                  DB_STATUS.
**          End of STAR comments.
**	25-jun-1992 (fpang)
**	    Included ddb.h and related include files for Sybil merge.
**      04-sep-1992 (fpang)
**          In scd_alloc_scb(), fill in dd_u1_usrname for distributed server.
**          QEF/RQF needs it filled in to make remote connections.
**	22-sep-1992 (bryanp)
**	    Added support for new logging & locking system special threads.
**	01-oct-1992 (fpang)
**	    In scd_alloc_scb(), consider Star 2PC recovery thread as a special 
**	    thread ( ie. set sscb_state to SCS_SPECIAL ).
**	17-jul-92 (daveb)
**	    Call scs_scb_attach and scs_detach_scb to make the scb's
**	    MO managaeable.
**	19-Nov-1992 (daveb)
**	    scd_get_assoc: Handle E_GCFFFE_STATUS as needing resumption,
**	    for IMA/GCM.
**	20-nov-1992 (markg)
**	    Added support for the audit thread.
**	03-dec-1992 (pholman)
**	    Insert PM initialisation routines.
**	18-jan-1993 (bryanp)
**	    Add support for CPTIMER special thread.
**	05-feb-1993 (ralph)
**	    DELIM_IDENT: Save untranslated username
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	23-apr-1993 (fpang)
**	    STAR. Support for installation passwords.
**	2-Jul-1993 (daveb)
**	    prototyped, removed func externs.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	1-Sep-93 (seiwald)
**	    CS option revamp: call PMsetDefault() so that later calls to 
**	    PMget() fetch the proper parameters from config.dat.  Set 
**	    the global Sc_server_type so that scd_initiate() can figure
**	    out what type of server it is.
**	08-sep-93 (swm)
**	    Cast completion id. parameter to IIGCa_call() to PTR as it is
**	    no longer a i4, to accomodate pointer completion ids.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	    Change casts in used pointer comparisons to char *.
**	    Added comments to flag valid pointer to i4 truncations.
**	26-Jul-1993 (daveb)
**	    Change to do connection acceptance/rejection in scd_alloc_scb
**	    instead of the CL.  Use new Sc_main_cb fields sc_listen_mask,
**	    sc_max_conns, sc_rsrvd_conns, and sc_current_conns.  Also
**	    change to execute nice server shutdown in scd_dealloc_scb
**	    when the last session goes away.  Include <erusf.h> to get
**	    "number of users has exceeded limit" message, add
**	    scd_is_special() to see if this request gets special handling
**	    if we're at the soft limit.
**	21-oct-1993 (bryanp)
**	    If this is the recovery server, set sc_capabilities to
**		SC_RECOVERY_SERVER.
**	22-oct-1993 (seiwald)
**	    Ensure each type of server has a symbol for its TRdisplay()
**	    log file: STAR gets II_STAR_LOG, RECOVERY and INGRES get
**	    II_DBMS_LOG, and RMS get II_RMS_LOG.
**	16-dec-1993 (tyler)
**	    Replaced GChostname() with call to PMhost().
**       9-Dec-1993 (daveb)
**          Changed use of PM privs to meet current LRC approved
**          syntax and values.  Use new error for closed server.
**	24-jan-1994 (gmanning)
**	    Change references from CSsid to CSscb, call CSget_scb() instead of 
**	    directly accessing structure members.   Add CUFLIB, RQFLIB, SXFLIB,
**	    TPFLIB TO NEEDLIBS= in order to build in NT environment.  
**      31-jan-1994 (mikem)
**	    Reordered #include of <cs.h> to before <lk.h>.  <lk.h> now 
**	    references a CS_SID and needs <cs.h> to be be included before
**	    it.
**	31-jan-1994 (bryanp) B56553
**	    If an incorrect parameter is received, echo the incorrect parameter
**		to the error log to help the user understand what went wrong.
**	31-jan-1994 (bryanp) B56609
**	    Pass an error handling function to PMload to log errors nicely.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors, so ule_format them.
**	02-feb-1994 (mikem)
**	    Back out 24-jan-94 change to scd_get_assoc().  IIGCa_call() to post
**	    a listen must get the "magic" CS_ADDER_ID scb, rather than the 
**	    current scb.  Before backing out this change no connections could
**	    could be made from client to server (they would eventually time 
**	    out).
**	05-nov-1993 (smc)
**	    Bug #58635
**	    Made l_reserved & owner elements PTR for consistency with
**	    DM_OBJECT. This results in owner within sc0m_allocate having
**	    to be PTR.
**       2-Mar-1994 (daveb) 58423
**          We use scd_chk_priv() to do the PM work of validating
**	    monitor users.  Make it public, and have it handle non-EOS
**  	    terminated usernames, which are presumed to be <
**  	    DB_MAXNAME in length.
**      10-Mar-1994 (daveb)
**          Maintain sc_highwater_conns.
**      24-Apr-1995 (cohmi01)
**	    Add SC_IOMASTER_SERVER as a server type, this is a disk I/O 
**	    master process, which handles write-along and read-ahead threads.
**      14-jun-1995 (chech02)
**          Added SCF_misc_sem semaphore to protect critical sections.(MCT)
**	04-oct-1995 (wonst02)
**	    Added cs_facility to the CS_CB so that scs_facility can be called
**	    by the CL just as scs_format is.
**	11-jan-1996 (dougb)
**	    Make new test for IOMASTER server type specific to the Unix
**	    platform.  Cs_numprocessors should *never* be referenced from
**	    generic code.
**	    ??? Note:  This code should be generalized by adding the test to
**	    ??? every CL's version of CSdispatch() or CSinitiate().  Or, a new
**	    ??? CL interface routine (added on all platforms) could return the
**	    ??? required information.
**	22-feb-1996 (prida01)
**	    Add call to EXdumpInit to initialise server diagnostics.
**	13-Mch-1996 (prida01)
**	    Add scd_diag call to scb to pass back to cs. We need this to
**	    link the cs to the rest of the back end.
**	01-Apr-1996 (prida01)
**	    remove machine specific defines.
**	06-mar-1996 (nanpr01)
**	    Added support for tuple size increase. Removed the dependency
**	    of DB_MAXTUP.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	3-jun-96 (stephenb)
**	    Add SCS_REP_QMAN case for replicator queue management threads.
**	06-jun-1996 (thaju02)
**	    Cleanup of compilation warnings due to incompatible prototypes.
**	03-jun-1996 (canor01)
**	    Enable the ability for scs_facility to be called by CL just as
**	    scs_format is, for Unix as well as NT.
**	16-Jul-96 (gordy)
**	    Fixed the size of the release message: no data value included.
**	    Add casts to IIGCa_call() to quite compiler warnings.
**      11-feb-97 (cohmi01)
**	    If current request was RAAT, send back msg in RAAT format. (B80665)
**      26-Jun-1997 (radve01)
**          dbms log splitted on log-per-process basis if this option was
**          desired by file-name containing "%p". "%p" is replaced by PID.
**	10-oct-1996 (canor01)
**	    Make messages more generic.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Start up the LK Blocking Callback Thread in recovery and database
**          servers.
**	15-Nov-1996 (jenjo02)
**	    Start up the Deadlock Detector thread in the recovery server.
**	12-dec-1996 (canor01)
**	    Allow Sampler thread to be started in recovery and database
**	    servers.
**	02-apr-1997 (hanch04)
**	    Move main to front of line for mkmingnt
**	12-Jun-1997 (jenjo02)
**	    In scd_dealloc_scb(), don't doubly take sc_misc_semaphore.
**	10-Dec-1997 (jenjo02)
**	    Initialize cs_get_rcp_pid function to LGrcp_pid.
**      13-Feb-98 (fanra01)
**          Move the call to scs_scb_attach into the cs via the Cs_srv_block.
**	09-Mar-1998 (jenjo02)
**	    Added support for factotum threads in scd_alloc_scb().
**	09-Apr-1998 (jenjo02)
**	    Manually initialize SCS_CSITBL csi_states now that we no longer
**	    block-initialize all of SCS memory.
**	16-Apr-1998 (jenjo02)
**	    When copying ICS from parent to Factotum thread, clear parent thread's
**	    ics_gw_parms and ics_alloc_gw_parms pointers.
**	14-May-1998 (jenjo02)
**	    Removed SCS_SWRITE_BEHIND thread type. They're now started as
**	    factotum threads.
**	02-Jun-1998 (jenjo02)
**	    Retain parent's ics_terminal name as factotum thread's terminal
** 	    rather than setting it to "none".
**	06-may-1998 (canor01)
**	    Add license checking thread for use with CA License Check API.
**	    License checking thread needs to be marked as SCS_SPECIAL.
**	22-jun-1998 (canor01)
**	    Zero need_facilities for license checking thread.
**    11-jun-1998 (kinte01)
**        Cross integrate change 435890 from oping12
**          18-may-1998 (horda03)
**            X-Integrate Change 427896 to enable Evidence Sets.
**	10-jul-1998 (somsa01)
**	    EXdumpInit() is not yet defined for NT.
**	27-Aug-1998 (jenjo02)
**	    Initialize ccb.cs_stkcache = FALSE;
**	15-sep-1998 (somsa01)
**	    In scd_dealloc_scb(), we were playing with sc_misc_semaphore
**	    AFTER releasing the SCB. Relocated the code which releases the
**	    SCB to the end.  (Bug #93332)
**	09-Oct-1998 (jenjo02)
**	    When creating Factotum threads, don't copy the ICS from the
**	    parent thread if it's the Admin thread. The Admin thread's
**	    SCB consists of only the CS_SCB and does not include
**	    SCF ICS data.
**      16-Oct-98 (fanra01)
**          Set server name if not default.
**	16-Nov-1998 (jenjo02)
**	    CSsuspend on CS_BIOR_MASK instead of CS_BIO_MASK.
**	14-jan-1999 (nanpr01)
**	    Changed the sc0m_deallocate to pass pointer to a pointer.
**      27-Jan-1999 (fanra01)
**          In accordance with the CL spec the ME_INGRES_THREAD_ALLOC flag
**          is not to be used and replaced with ME_INGRES_ALLOC.
**          Using ME_INGRES_THREAD_ALLOC causes process heap corruption when
**          used in conjunction with MEtag'ed memory.  During a free the
**          memory block would be returned to the system heap causing
**          corruption.
**      15-mar-1999 (hanch04)
**          Allocate the full stack size.  If stack size is zero, default
**          to the OS stack size.
**	04-Mar-1999 (thaal01)
**	    For VMS, add the PID to the II_DBMS_LOG file name in HEX,
**	    as DEC intended, not Decimal which is the Unix default.
**	    Makes the log file easier to match to the server
**	12-Mar-1999 (thaal01)
**	    Correction to typo in the above change.
**	23-mar-1999 (hanch04)
**	    Set stack size to 65536 default because some OS default are
**	    too small.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      27-apr-1999 (hanch04)
**          replace STindex with STchr
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**      08-sep-1999 (devjo01)
**          Test SC_LIC_VIOL_REJECT bit instead of calling CIcapabilities()
**          when checking if server is licensed. (b98689)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-Jan-2001 (jenjo02)
**	   In scd_alloc_scb(), cancel SXF "need" for the session
**	   if neither B1 nor C2 secure.
**	   SCS_SCHECK_TERM threads need only SCF, not DMF, SXF and ADF.
**	10-may-2001 (devjo01)
**	    Add SCS_CSP_MAIN, SCS_CSP_MSGTHR, SCS_CSP_MSGMON thread types to
**	    scd_alloc_scb().  s103715 (also bonch01 s98895).
**	25-jul-2002 (abbjo03)
**	   In scd_alloc_scb, add DB_SXF_ID to need_facilities for SCS_REP_QMAN.
**	17-sep-2002 (devjo01)
**	    Bind procedure to specific RAD if exploiting NUMA architecture.
**	    If a NUMA cluster, set PM override for host name, so PMget
**	    checks "host" name "R<n>_<hostname>", then "<hostname>" when
**	    resolving parameters.
**	03-Oct-2002 (somsa01)
**	    If CSinitiate() fails, make sure we print out "FAIL".
**      26-feb-2002 (padni01) bug 102373
**          If the sequencer session control block's flags sscb_flags
**          indicates that the GCA connection has been dropped already or
**          is bad, do not call GCA 'again' to disconnect this session.
**      18-feb-2004 (stial01)
**          scd_alloc_scb() Allocate cscb_outbuffer separate from scb,
**          in case the session needs a larger tuple buffer.
**          scd_dealloc_scb() deallocate cscb_outbuffer separate from scb
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**      03-May-2005 (chopr01) (b113605)
**         The server scb was not getting initialized properly as we
**         were using wrong size of the memory for initialization 
**	09-may-2005 (devjo01)
**	    Add SCS_SDISTDEADLOCK to the class of critical threads.
**	21-jun-2005 (devjo01)
**	    Add SCS_SGLCSUPPORT to the class of critical threads.
**	16-nov-2005 (abbjo03)
**	    Use cs_scb.cs_client_type to determine whether to initialize the
**	    sscb_ics for factotum threads.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Replace sc0e_put() with sc0ePuTAsFcn() in cs_elog. 
**	    Use new forms of sc0ePut(), uleFormat().
**	29-Oct-2008 (jonj)
**	    SIR 120874: sc0ePutAsFcn renamed to sc0e_putAsFcn.
**	    Repaired scd_pmerr_func - sc0ePut returns void.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	30-May-2009 (kiria01) SIR 121665
**	    Update GCA API to LEVEL 5
**	Nov/Dec 2009 (stephenb)
**	    Init new batch processing fields.
**	12-Nov-2010 (kschendel) SIR 124685
**	    Refine CS prototypes.
**/

/*
** mkming hints
MODE =		PARTIAL_LD

NEEDLIBS =      SCFLIB PSFLIB OPFLIB GWFLIB PSFLIB RDFLIB QEFLIB QSFLIB DMFLIB ADFLIB CUFLIB ULFLIB RQFLIB SXFLIB TPFLIB GCFLIB COMPATLIB MALLOCLIB

DYNLIBS  =	LIBIIUSERADT

OWNER =		INGUSER

DEST =		bin

PROGRAM =	iidbms

UNDEFS =	pst_nddump	

*/

/*
** Definition of all global variables owned by this file.
*/

GLOBALREF   SC_MAIN_CB       *Sc_main_cb; /* Scf main ds */
GLOBALREF   i4	     Sc_server_type; /* for handing to scd_initiate */
GLOBALREF   PTR              Sc_server_name;
# ifdef UNIX
GLOBALREF   i4	    Cs_numprocessors;
# endif /* UNIX */


/*
**  Forward and/or External function references.
*/

static STATUS scd_allow_connect( GCA_LS_PARMS *crb );

# ifdef	II_DMF_MERGE
# define    MAIN(argc, argv)	iidbms_libmain(argc, argv)
# else
# define    MAIN(argc, argv)	main(argc, argv)
# endif

static PM_ERR_FUNC	scd_pmerr_func;

/*{
** Name: main	- Main entry point for the DBMS Server
**
** Description:
**      This routine provides the central entry point for the system. 
**      When entering the system, this routine initializes the dispatcher 
**      CS\CLF module, and then tells it to go.  Control never returns
**      to this module once CSdispatch() is called.
**
** Inputs:
**      None
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      25-Nov-1986 (fred)
**          Created
**	2-Jul-1993 (daveb)
**	    added return type "int".
**	21-oct-1993 (bryanp)
**	    If this is the recovery server, set sc_capabilities to
**		SC_RECOVERY_SERVER.
**	31-jan-1994 (bryanp) B56553
**	    If an incorrect parameter is received, echo the incorrect parameter
**		to the error log to help the user understand what went wrong.
**	31-jan-1994 (bryanp) B56609
**	    Pass an error handling function to PMload to log errors nicely.
**	31-jan-1994 (jnash)
**	    CSinitiate() now returns errors, so ule_format them.
**      24-Apr-1995 (cohmi01)
**	    Add SC_IOMASTER_SERVER as a server type, this is a disk I/O 
**	    master process, which handles write-along and read-ahead threads.
**	11-jan-1996 (dougb)
**	    Make new test for IOMASTER server type specific to the Unix
**	    platform.  Cs_numprocessors should *never* be referenced from
**	    generic code.
**	    ??? Note:  This code should be generalized by adding the test to
**	    ??? every CL's version of CSdispatch() or CSinitiate().  Or, a new
**	    ??? CL interface routine (added on all platforms) could return the
**	    ??? required information.
**	13-Mch-1996 (prida01)
**	    Add extra diagnostic call for the back end
**      24-Jun-1997 (radve01)
**          dbms log splitted on log-per-process basis if this option was
**          desired by file-name containing "%p". "%p" is replaced by PID.
**	10-oct-1996 (canor01)
**	    Make messages more generic.
**	10-jul-1998 (somsa01)
**	    EXdumpInit() is not yet defined for NT.
**	10-Dec-1997 (jenjo02)
**	    Initialize cs_get_rcp_pid function to LGrcp_pid.
**	27-Aug-1998 (jenjo02)
**	    Initialize ccb.cs_stkcache = FALSE;
**      16-Oct-98 (fanra01)
**          Set server name if not default.
**	29-sep-2002 (devjo01)
**	    Call MEadvise only if not II_DMF_MERGE.
**	03-Oct-2002 (somsa01)
**	    If CSinitiate() fails, make sure we print out "FAIL".
**       7-Oct-2003 (hanal04) Bug 110889 INGSRV2521
**          Tell the EX subsystem that the DBMS is the client.
**          This must be done prior to an EXdeclare() or EXsetup()
**          calls if we want to avoid defaulting to an
**          EX_USER_APPLICATION type.
**	14-Jun-2004 (schka24)
**	    Safer environment variable handling.
**      21-Jun-2007 (hweho01)
**          Added GCA_TERMINATE call for RCP server, avoid
**          E_GC0155_GCN_FAILURE message in errlog.log.
**       5-Dec-2007 (hanal04) Bug 119503 and 118233
**          Recoded fix for bug 118233. GCA_TERMINATE now down as part
**          of thread cleanup initiated by CSterminate() in rcp_shutdown().
**      28-Jan-2010 (horda03) Bug 121811
**          Evidence Set port to NT.
*/
int
# ifdef	II_DMF_MERGE
    MAIN(argc, argv)
# else
main(argc, argv)
# endif
i4		argc;
char		**argv;
{
    DB_STATUS           status;
    STATUS              ret_status;
    CL_ERR_DESC		cl_err;
    CS_CB		ccb;
    i4             err_code;
    i4			num_sessions = 0;
    char		*tran_place = 0;
    char		*server_type = "dbms";
    char		*server_flavor = "*";
    char		un_env[MAX_LOC-20];	/* Room for a %d */
    char                un_str[MAX_LOC];
    char                *pid_index = NULL;
    char		*host;
    PID                 un_num;

# ifndef	II_DMF_MERGE
    MEadvise(ME_INGRES_ALLOC);
# endif

    (void) EXsetclient(EX_INGRES_DBMS);

    /*
    ** Stash args.
    */

    if( argc > 1 )
	server_type = argv[1];

    Sc_server_name = ERx("default");

    if( argc > 2 )
    {
	server_flavor = argv[2];
        Sc_server_name = argv[2];
    }

    /* Set CM character set stuff */

    ret_status = CMset_charset(&cl_err);
    if ( ret_status != OK)
    {
	uleFormat(NULL, E_UL0016_CHAR_INIT, &cl_err, ULE_LOG ,
	    NULL, (char * )0, 0L, (i4 *)0, &err_code, 0);
	PCexit(FAIL);
    }

    /*
    ** Divine server type.  This mapping is for historical reasons.
    **
    **	dbms 	-> INGRES
    **	rms 	-> RMS
    **	star 	-> STAR
    **	recovery -> RCP
    */

    if( !STcasecmp( server_type, "dbms" ) )
    {
	Sc_server_type = SC_INGRES_SERVER;
    }
    else if( !STcasecmp( server_type, "rms" ) )
    {
	Sc_server_type = SC_RMS_SERVER;
    }
    else if( !STcasecmp( server_type, "star" ) )
    {
	Sc_server_type = SC_STAR_SERVER;
    }
    else if( !STcasecmp( server_type, "recovery" ) )
    {
	Sc_server_type = SC_RECOVERY_SERVER;
    }
    else if( !STcasecmp( server_type, "iomaster" ) )
    {
	Sc_server_type = SC_IOMASTER_SERVER;  
    }
    else
    {
	sc0ePut(NULL, E_SC0346_DBMS_PARAMETER_ERROR, (CL_ERR_DESC*)NULL,
		(i4)1, (i4)0, (PTR)server_type);
	sc0ePut(NULL, E_SC0124_SERVER_INITIATE, (CL_ERR_DESC *)NULL, 0);
	PCexit(FAIL);
    }

    /*
    **  Initialise Paramater Management interface
    */
    PMinit();
    switch( (status = PMload((LOCATION *)NULL, scd_pmerr_func)))
    {
	case OK:
	    /* Loaded sucessfully */
	    break;
	case PM_FILE_BAD:
	    /* syntax error */
	    sc0ePut(NULL, E_SC032B_BAD_PM_FILE, NULL, 0);
            PCexit(FAIL);
	default: 
	    /* unable to open file */
	    sc0ePut(NULL, status, NULL, 0);
	    sc0ePut(NULL, E_SC032C_NO_PM_FILE, NULL, 0);
            PCexit(FAIL);
    }
    /*
    ** Next set up the first few default parameters for subsequent PMget 
    ** calls based upon the name of the local node...
    */
    host = PMhost();
    PMsetDefault( 0, ERx( "ii" ) );
    PMsetDefault( 1, host );
    PMsetDefault( 2, server_type );
    PMsetDefault( 3, server_flavor );

    /*
    ** Pick up other paramaters..
    */

    num_sessions = 32;   /* {@fix_me@} random at the moment */
    
    /* For Star, check II_STAR_LOG first, use II_DBMS_LOG if former
    ** is not defined.
    */

    switch( Sc_server_type )
    {
    case SC_INGRES_SERVER:	NMgtAt("II_DBMS_LOG", &tran_place); break;
    case SC_RECOVERY_SERVER:	NMgtAt("II_DBMS_LOG", &tran_place); break;
    case SC_RMS_SERVER:		NMgtAt("II_RMS_LOG", &tran_place); break;
    case SC_STAR_SERVER:	NMgtAt("II_STAR_LOG", &tran_place); break;
    case SC_IOMASTER_SERVER:	NMgtAt("II_IOM_LOG", &tran_place); break;
    }

    /*
    ** Make a unique file-name if $II_DBMS_LOG contains exactly one
    ** "%p" (no other %-things):
    ** replace "%p" with process id 
    */
    if ((tran_place) && (*tran_place))
    {
	/* Potentially truncate... */
	STlcopy(tran_place, &un_env[0], sizeof(un_env)-1);
	pid_index = STchr(un_env, '%');
	if((pid_index != NULLPTR) && (*(++pid_index) == 'p')
	  && STchr(pid_index+1, '%') == NULL)
	{
#if defined (VMS)
	    *pid_index = 'x';
#else
	    *pid_index = 'd';
#endif
	    PCpid(&un_num);
	    STprintf(un_str, un_env, un_num);
	    TRset_file(TR_F_OPEN, un_str, STlength(un_str), &cl_err);
        }
	else TRset_file(TR_F_OPEN, un_env, STlength(un_env), &cl_err);
    }

    EXdumpInit(); /* Initialise server diagnostics */ 

    for (;;)
    {
	ccb.cs_scnt = num_sessions;
	ccb.cs_ascnt = num_sessions;
	ccb.cs_stksize = 65536;		    /* {@fix_me@} random at the moment */
	ccb.cs_stkcache = FALSE;
	ccb.cs_scballoc = scd_alloc_scb;
	ccb.cs_scbdealloc = scd_dealloc_scb;
	ccb.cs_elog = sc0e_putAsFcn;
	ccb.cs_process = scs_sequencer;
	ccb.cs_startup = scd_initiate;
	ccb.cs_shutdown = scd_terminate;
	ccb.cs_attn = scs_attn;
	ccb.cs_format = scs_format;
	ccb.cs_facility = scs_facility;
	ccb.cs_read = scc_recv;
	ccb.cs_write = scc_send;
	ccb.cs_saddr = scd_get_assoc;
	ccb.cs_reject = scd_reject_assoc;
	ccb.cs_disconnect = scd_disconnect;
        ccb.cs_scbattach = scs_scb_attach;
        ccb.cs_scbdetach = scs_scb_detach;
	ccb.cs_diag = scd_diag;
	ccb.cs_get_rcp_pid = LGrcp_pid;
	ccb.cs_format_lkkey = LKkey_to_string;

	argc = 1;

	status = CSinitiate(&argc, &argv, &ccb);
	if (status)
	{
	    SIstd_write(SI_STD_OUT, "FAIL\n");
	    uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
	  	(char *)0, (i4)0, (i4 *)0, &err_code, 0);
	    break;
	}

# ifdef UNIX
	/*
	** ??? Note:  This code should be generalized by adding the test to
	** ??? every CL's version of CSdispatch() or CSinitiate().  Or, a new
	** ??? CL interface routine (added on all platforms) could return the
	** ??? required information.  Do not document for users 'til this is
	** ??? fixed.
	*/
	if (Sc_server_type == SC_IOMASTER_SERVER && Cs_numprocessors <=1)
	{
	    /* do not allow IOMASTER server to run on uni-processor */
	    sc0ePut(NULL, E_SC0369_IOMASTER_BADCPU, NULL, 0);
	    PCexit(FAIL);
	}
# endif /* UNIX */

	scs_mo_init();
	status = CSdispatch();
	TRdisplay("CSdispatch() = %x\n", status);
	break;
    }

    TRset_file(TR_F_CLOSE, 0, 0, &cl_err);
    PCexit(OK);
    /* NOTREACHED */
    return(FAIL);
}

/*{
** Name: scd_alloc_scb	- Allocate an SCB for the server
**
** Description:
**      This routine allocates a session control block for use by  
**      the remainder of the server.  In addition to simple allocation, 
**      this routine will fill in the initial values for the scb, as well 
**      as allocate any associated queue headers which belong with this 
**      entity.
**
** Inputs:
**	crb				Create request block.
**	thread_type			Type of thread -
**					SCS_SNORMAL (0) - user session.
**					SCS_SMONITOR - monitor program.
**					SCS_SFAST_COMMIT - fast commit thread.
**					SCS_SERVER_INIT - server init thread.
**					SCS_SEVENT	- event handling thread
**			-or-
**	ftc				Factotum Thread Create block
**	thread_type			SCS_SFACTOTUM
**
** Outputs:
**      scb_ptr                         Address of place to put the scb
**	Returns:
**	    DB_STATUS
**	Exceptions:
**
**
** Side Effects:
**	    none
**
** History:
**      25-Nov-1986 (fred)
**          Created.
**	15-Jun-1988 (rogerk)
**	    Added thread type parameter.
**	    Set session control block sscb_stype and sscb_flags fields.
**	    If SCS_NOGCA session flag, then don't allocate memory for
**	    communication blocks and don't initialize GCA.
**	    If thread type is SCS_FAST_COMMIT then set sequencer state
**	    to SCS_SPECIAL rather than SCS_ACCEPT.
**	 1-sep-1988 (rogerk)
**	    Added support for write behind threads.
**      23-Mar-1989 (fred)
**          Added support for server initialization threads.
**	21-Jun-1989 (nancy) -- bug #4832, fix offset calculations in 2 places
**	    (wide rows returned in a cursor were getting trashed)
**      05-apr-1989 (jennifer)
**          Add code to store security label in SCS_ICS.
**	10-may-1990 (fred)
**	    Added initialization of large object fields.
**      08-nov-1990 (ralph)
**          Align major structures on ALIGN_RESTRICT boundaries in scd_alloc_scb
**	15-jan-1991 (ralph)
**	    Cast ME_ALIGN_MACRO to i4 to avoid compiler problems on UNIX
**	04-feb-1991 (neil)
**	    Preallocated the cscb_evdata message for alerters.
**	28-Jan-1991 (anton)
**	    Change to call scs_disassoc instead of GCA directly
**	    Don't log E_GC0032_NO_PEER - Common condition result of name server
**	    bed check
**	21-may-91 (andre)
**	    renamed some fields in SCS_ICS to make it easier to determine by the
**	    name whether the field contains info pertaining to the system (also
**	    known as "Real"), Session ("he who invoked us"), or the Effective
**	    identifier:
**		ics_username	--> ics_eusername   effective user id
**	9-Aug-91, 27-feb-92 (daveb)
**	    Fill in new cs_client_type field to unanbigiously identify
**	    CS_SCB's as SCD_SCBs, to fix bug 38056; cause by async
**	    GCA API 2 dissasoc added above.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              24-Aug-1988 (alexh)
**                  add logic to set a sessions STAR flag. This is currently
**                  set according to server capabilities (sc_capabilities). This
**                  should be set according to GCA association parameter if
**                  both STAR and BACKEND threads coexist.
**              28-nov-1989 (georgeg)
**                  Added support for STAR 2PC. The STAR server will start a 
**		    thread to recover all orphan transactions before it opens 
**		    up for FE'ends, this 2PC thread will terminate when all 
**		    DX are recovered.
**          End of STAR comments.
**      04-sep-1992 (fpang)
**          Fill in dd_u1_usrname for distributed server. QEF/RQF needs it filled
**          in to make remote connections.
**	22-sep-1992 (bryanp)
**	    Added support for new logging & locking system special threads.
**	01-oct-1992 (fpang)
**	    Consider Star 2PC recovery thread as a special thread 
**	    ( ie. set sscb_state to SCS_SPECIAL )
**	17-jul-92 (daveb)
**	    Attach the SCB to the MO space when allocated.  Define the
**	    MO classes for SCF before CSdispatch.  Zero the user domain(?)
**	03-dec-92 (andre)
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**	19-Nov-1992 (daveb)
**	    Just resume/reissue listen if it's "incomplete".
**	20-nov-1992 (markg)
**	    Added support for the audit thread.
**	18-jan-1993 (bryanp)
**	    Add support for CPTIMER special thread.
**	05-feb-1993 (ralph)
**	    DELIM_IDENT: Save untranslated username
**	24-mar-1993 (barbara)
**	    Remove assignments to ics_effect_user for Star.  This field
**	    is no longer used.
**	30-mar-1993 (robf)
**	    Remove xORANGE, replace by Secure 2.0 code
**	23-apr-1993 (fpang)
**	    STAR. Support for installation passwords.
**	    Stop propogating the gca_listen password and stop looking at the
**	    gca_auxdata.
**	2-Jul-1993 (daveb)
**	    remove unused variable 'da_parms'
**	26-Jul-1993 (daveb)
**	    On user connection, see if we should allow it,
**	    and return error status if not.  Bump our count in
**	    sc_current_conns if it succeeds.
**	27-Jul-1993 (daveb)
**	    Oh, by the way, this is a STATUS routine, called by the
**	    CL, and can't return DB_STATUS!
**	15-Sep-1993 (daveb)
**	    Set assoc id for non-GCA threads to -2.  Why not -1?
**	    Because -1 + 1 = 0, and we want it to stay < 0, even when
**	    we're messing around with orgin-0 stuff translated to
**	    origin-1.
**	22-oct-93 (robf)
**          Add SCS_SCHECK_TERM
**	25-oct-93 (vijay)
**	    Correct casts. 'offset's of type i4 were unnecessarily
**	    cast to char *.
**	30-dec-93 (robf)
**          Allow terminator thread to terminate.
**      10-Mar-1994 (daveb)
**          Maintain sc_highwater_conns.
**	14-apr-94 (robf)
**	    Add SCS_SSECAUD_WRITER, the security audit writer thread.
**	26-may-94 (robf/arc)
**	    Initialize session security label to empty label if supported by CL since
**	    it may be checked later.
**	06-mar-1996 (nanpr01)
**	    Added support for tuple size increase. Removed the dependency
**	    of DB_MAXTUP.
**      22-mar-1996 (stial01)
**          Cast length passed to sc0m_allocate to i4.
**	3-jun-96 (stephenb)
**	    Add SCS_REP_QMAN case for replicator queue management threads.
**	16-Jul-96 (gordy)
**	    Add casts to IIGCa_call() to quite compiler warnings.
**	15-Nov-1996 (jenjo02)
**	    Start up the Deadlock Detector thread in the recovery server.
**	14-may-1997 (nanpr01)
**	    Initialized the field for storing the descriptor pointer.
**	12-Jun-1997 (jenjo02)
**	    Shortcut lengthy retest of thread type.
**	3-feb-98 (inkdo01)
**	    Added SCS_SSORT_THREAD.
**      13-Feb-98 (fanra01)
**          Move the call to scs_scb_attach into the cs via the Cs_srv_block.
**	09-Mar-1998 (jenjo02)
**	    Added support for factotum threads in scd_alloc_scb().
**	    Session's MCB is now embedded in SCD_DSCB instead of being
**	    separately allocated. Rearranged a bunch of code to enhance
**	    the "lightweightness" of Factotum threads. Keep in mind that
**	    scd_alloc_scb() runs as part of the creating thread, not the
**	    newly created thread.
**	    Merged scs_alloc_cbs() code in here to add facilities memory
**	    to the grand total for the SCB so we can allocate all of the
**	    SCB in one chunk instead of a bunch of chunkletts.
**	    Removed use of "SCS_NOGCA" flag, utilizing need_facilities
**	    (1 << DB_GCF_ID) mask instead.
**	09-Apr-1998 (jenjo02)
**	    Manually initialize SCS_CSITBL csi_states now that we no longer
**	    block-initialize all of SCS memory.
**	14-apr-98 (inkdo01)
**	    Dropped SCS_SSORT_THREAD with addition of factotum threads.
**	06-may-1998 (canor01)
**	    Add license checking thread.
**	14-May-1998 (jenjo02)
**	    Removed SCS_SWRITE_BEHIND thread type. They're now started as
**	    factotum threads.
**	02-Jun-1998 (jenjo02)
**	    Retain parent's ics_terminal name as factotum thread's terminal
** 	    rather than setting it to "none".
**	22-jun-1998 (canor01)
**	    Zero need_facilities for license checking thread.
**	09-Oct-1998 (jenjo02)
**	    When creating Factotum threads, don't copy the ICS from the
**	    parent thread if it's the Admin thread. The Admin thread's
**	    SCB consists of only the CS_SCB and does not include
**	    SCF ICS data.
**	04-Jan-2001 (jenjo02)
**	    Remove two unneeded CSget_sid() calls for factotum thread.
**	19-Jan-2001 (jenjo02)
**	   In scd_alloc_scb(), cancel SXF "need" for the session
**	   if installation is neither B1 nor C2 secure.
**	   SCS_SCHECK_TERM threads need only SCF, not DMF, SXF and ADF.
**	10-may-2001 (devjo01)
**	    Add SCS_CSP_MAIN, SCS_CSP_MSGTHR, SCS_CSP_MSGMON.
**	25-jul-2002 (abbjo03)
**	   Add DB_SXF_ID to need_facilities for SCS_REP_QMAN.    
**	30-Jun-2004 (jenjo02)
**	   For Factota, copy Parent's QEF_CB pointer to SCB.
**	15-Aug-2005 (jenjo02)
**	    Implement multiple DMF LongTerm memory pools and
**	    ShortTerm session memory. For SCF, this means that
**	    sc_dmbytes might be quite large, and we really only
**	    need to zero the DMC_CB part of it - DMF will take
**	    care of the rest.
**	30-may-06 (dougi)
**	    add optional allocation of query text buffers for cursors.
**	20-apr-2007 (dougi)
**	    Make optional allocs mandatory - to always allow display of
**	    cursor text.
**      15-May-2007 (hanal04) Bug 118044
**         Make sure the ICS block for factotum threads created by the admin
**         thread are explicitly zero filled. Uninitialised values lead to
**         E_SC0344 errors for write behind threads. 
**	16-Oct-2008 (kibro01) b121021
**	   Use parent's effective username for transactions and internal
**	   stuff, while leaving username to be the thread name for a factotum
**	   thread.
**	10-Jun-2009 (kibro01) b122050
**	   If we'd have to use more than SC0M_MAXALLOC, then split out the 
**	   requirement for SC924 tracing into one or more separate allocation
**	   requests.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**	21-Apr-2010 (kschendel) SIR 123485
**	    Delete unused blob coupon list.
*/
STATUS
scd_alloc_scb( CS_SCB  **csscbp, void  *input_crb, i4  thread_type )
{
    STATUS              status = OK;
    STATUS		local_status;
    SCS_MCB		*mcb_head;
    i4			i;
    char		*cp;
    i4		mem_needed;
    i4		scb_size;
    i4			thread_flags;
    DB_ERROR		error;
    SCF_FTC		*ftc;
    GCA_LS_PARMS	*crb;
    SCD_SCB		*scb;
    SCD_SCB		**scb_ptr = (SCD_SCB **) csscbp;
    char		*block;
    char		*user_name, *terminal_name;
    char		*parent_scb_username = NULL;
    bool	allocate_sc924_separately = FALSE;
    i4		sc924_requirement = 0;

    /*
    ** Offsets to major control blocks and buffers.
    ** See comments below.
    */
    i4			need_facilities;	/* Facilities needed by session */

    i4		offset_adf;		/* Facilities' cbs, allocated   */
    i4		offset_dmf;		/* based on need_facilities     */
    i4		offset_gwf;
    i4		offset_sxf;
    i4		offset_opf;
    i4		offset_psf;
    i4		offset_qef;
    i4		offset_qsf;
    i4		offset_rdf;
    i4		offset_scf;

    i4             offset_csitbl;          /* Offset of SCS_CSITBL     */
    i4             offset_inbuffer;        /* Offset of inbuffer       */
    i4             offset_tuples;          /* Offset of tuples	    */
    i4             offset_rdata;           /* Offset of rdata          */
    i4             offset_rpdata;          /* Offset of rpdata         */
    i4             offset_evdata;          /* Offset of evdata         */
    i4             offset_dbuffer;         /* Offset of dbuffer        */
    i4		offset_facdata;		/* Offset of factotum data  */
    i4		offset_curstext;	/* Offset of optional cursor text */
   
    
    /*
    ** Are we allocating a Factotum thread?
    ** If so, there's no *crb, but a *ftc instead.
    */
    if (thread_type == SCS_SFACTOTUM)
    {
	ftc = (SCF_FTC*)input_crb;
	/*
	** If GCF is identified as a needed facility, the protocol
	** assumes that ftc_data, if non-NULL, contains a pointer to a CRB:
	*/
	if (ftc->ftc_facilities & (1 << DB_GCF_ID))
	{
	    crb = (GCA_LS_PARMS*)ftc->ftc_data;
	}
	else
	    crb = (GCA_LS_PARMS*)NULL;
    }
    else 
	crb = (GCA_LS_PARMS *) input_crb;

    if (crb && crb->gca_status != E_GC0000_OK)
    {
	/* don't dissasoc or log an in-process listen */
    
	if ( crb->gca_status != E_GCFFFE_INCOMPLETE )
	{
	    if (crb->gca_status != E_GC0032_NO_PEER)
		sc0ePut(NULL, crb->gca_status, &crb->gca_os_status, 0);
	
	    status = scs_disassoc(crb->gca_assoc_id);
	    if (status)
	    {
		sc0ePut(NULL, status, NULL, 0);
	    }
	}
	return(crb->gca_status);
    }

    /*
    ** Determine the attributes of and facilities needed
    ** by this thread:
    */
    switch (thread_type)
    {
    case SCS_SFACTOTUM:
	/* 
	** Factotum threads may or may not have a gca connection and
	** are neither vital nor permanent
	**
	** The Factotum creator specifies the facilities.
	*/
	thread_flags = SCS_INTERNAL_THREAD;
	need_facilities = ftc->ftc_facilities;
	break;

    case SCS_SFAST_COMMIT:
	/*
	** Fast Commit Thread has special attributes:
	**   - it is a server fatal condition if the thread is terminated.
	**   - it is expected to live through the life of the server.
	*    - it needs only DMF
	*/
	thread_flags = (SCS_VITAL | SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_DMF_ID) );
	break;

    case SCS_SRECOVERY:
    case SCS_SFORCE_ABORT:
	thread_flags = (SCS_VITAL | SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_DMF_ID) );
	break;

    case SCS_SLOGWRITER:
    case SCS_SCHECK_DEAD:
    case SCS_SGROUP_COMMIT:
    case SCS_SCP_TIMER:
    case SCS_SLK_CALLBACK:
    case SCS_SDEADLOCK:
    case SCS_SDISTDEADLOCK:
    case SCS_SGLCSUPPORT:
    case SCS_CSP_MAIN:
    case SCS_CSP_MSGTHR:
    case SCS_CSP_MSGMON:
	/*
	** The logging & locking system special threads have special attributes:
	**   - it is a server fatal condition if the thread is terminated.
	**   - it is expected to live through the life of the server.
	**   - need only DMF
	*/
	thread_flags = (SCS_VITAL | SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_DMF_ID) );
	break;

    case SCS_SWRITE_ALONG:  
    case SCS_SREAD_AHEAD:    
	/*
	** Write Behind, Write Along, and Read Ahead threads:
	**   - are expected to live through the life of the server.
	**   - need only DMF
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_DMF_ID) );
	break;

    case SCS_SERVER_INIT:
	/* server init has no GCA -- it is not permanent		    */
	thread_flags = (SCS_VITAL | SCS_INTERNAL_THREAD);
	/* Server init vital that it be run.  No problem if exits	    */
	need_facilities = 0;
	break;

    case SCS_SEVENT:
	/*
	** Event handling thread has special attributes:
	**  - lives for life of the server
	**  - needs nothing but DMF.
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_DMF_ID) );
	break;

    case SCS_S2PC:
	/*
	** The STAR 2PC thread has special attributes:
	**   - it has no FE gca connection.
	**   - it is a server fatal condition if the thread is terminated.
	**   - it is expected to recover all orphan DX's and then disappear.
	**   - needs ADF and QEF
	*/
	thread_flags = (SCS_VITAL | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_ADF_ID) | (1 << DB_QEF_ID) );
	break;

    case SCS_SAUDIT:
	/* 
	** The Audit thread:
	**   - is permanent
	**   - it is a server fatal condition if the thread is terminated.
	**   - needs SXF, DMF, and ADF
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD | SCS_VITAL);
	need_facilities = ( (1 << DB_SXF_ID) | (1 << DB_DMF_ID) |
	     		    (1 << DB_ADF_ID) );

	break;

    case SCS_SCHECK_TERM:
	/* 
	** The Terminator thread:
	**   - is permanent
	**   - needs nothing but SCF.
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = 0;

	break;

    case SCS_SSECAUD_WRITER:
	/* 
	** The Security Audid Writer thread:
	**   - is permanent
	**   - needs SXF, DMF, and ADF
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_SXF_ID) | (1 << DB_DMF_ID) |
	     		    (1 << DB_ADF_ID) );
	break;

    case SCS_REP_QMAN:
	/*
	** The Replicator Queue Management thread:
	**   - is permanent
	**   - needs QEF, DMF, and ADF
	*/
	thread_flags = (SCS_PERMANENT | SCS_INTERNAL_THREAD);
	need_facilities = ( (1 << DB_ADF_ID) | (1 << DB_DMF_ID) |
			    (1 << DB_QEF_ID) | (1 << DB_SXF_ID));
	break;

    case SCS_SSAMPLER:
	/*
	** The sampler thread has no GCA and is transient
	**   - is transient
	**   - needs no facilities
	*/
	thread_flags = SCS_INTERNAL_THREAD;
	need_facilities = 0;
	break;

    case SCS_SMONITOR:
    case SCS_SNORMAL:
    default:

	/* User thread -- check to see if we should allow him to connect.
	** Don't check PM file if it's OK in the normal case.
	*/

	status = scd_allow_connect( crb );
	if( status != OK )
	{
	    scd_reject_assoc( crb, status );
	    return( status );
	}
	thread_flags = 0;

	if (thread_type == SCS_SMONITOR)
	    /*
	    ** Monitor session only needs ADF and QSF to run, but
	    ** may need DMF, QEF and SXF to look up user info at
	    ** startup from the IIDBDB.
	    */
	    need_facilities = ( (1 << DB_ADF_ID) | (1 << DB_DMF_ID) |
				(1 << DB_QSF_ID) | (1 << DB_QEF_ID) |
				(1 << DB_SXF_ID) | (1 << DB_GCF_ID) );
	else
	    /* The Normal and default cases need all facilities */
	    need_facilities = ( (1 << DB_ADF_ID) | (1 << DB_DMF_ID) |
				(1 << DB_QSF_ID) | (1 << DB_QEF_ID) |
				(1 << DB_DMF_ID) | (1 << DB_PSF_ID) |
				(1 << DB_OPF_ID) | (1 << DB_RDF_ID) |
				(1 << DB_SXF_ID) | (1 << DB_GWF_ID) |
				(1 << DB_GCF_ID) );
	break;
    }

    /* Cancel SXF if nether B1SECURE nor C2SECURE */
    if ( !(Sc_main_cb->sc_capabilities & (SC_C_C2SECURE)) )
	need_facilities &= ~(1 << DB_SXF_ID);


    /* Determine if the session is a STAR session. This should be changed
    ** to read from GCA association parameter, rather than coerced by
    ** the server capability 
    */
    if (Sc_main_cb->sc_capabilities & SC_STAR_SERVER)
    {
	thread_flags |= SCS_STAR;
	/* Star doesnt use DMF, GWF, or SXF, cancel them out. */
	need_facilities &= ~( (1 << DB_DMF_ID) |
			      (1 << DB_SXF_ID) |
			      (1 << DB_GWF_ID) );
    }

    /*									    */
    /*	Herein, we allocate the session control block.  To reduce overhead, */
    /*	we also allocate some buffers which are used on each query at the   */
    /*	same time.  These are						    */
    /*    - The SCD_SCB session control block                               */
    /*    - Control blocks for each needed facility			    */
    /*	  - The cursor control area -- variable sized at server startup	    */
    /*	  - The area into which to read data from the frontend partner	    */
    /*	  - The standard output buffer which we will write to the FE	    */
    /*		(This is different from the input buffer because for	    */
    /*		repeat queries, we use the parameters directly from the	    */
    /*		input area while we are filling the output area)	    */
    /*	  - Space for the standard response block to terminate each query.  */
    /*	  - Space for the standard response block to terminate each proc.   */
    /*	  - Space for a maximum size event message block for user alerts.   */
    /*	  - 512 bytes for small descriptors -- These are used for ``small'' */
    /*		select/retrieve's (approximately 20 attributes/no att names */
    /*		then rounded up nicely).				    */
    /*                                                                      */
    /*  Factotum threads only need memory for SCD_SCB, the Facilities,      */
    /*  and the (optional) data to be passed to the thread entry code.      */
    /*                                                                      */
    /* *** NOTE: ***                                                        */
    /*                                                                      */
    /* The above control block/buffers should be aligned on ALIGN_RESTRICT  */
    /* boundaries.  Specifically, the aligned areas include:                */
    /*                                                                      */
    /*              o SCD_SCB           (SCF's  session control block)      */
    /*              o SCS_CSITBL        (cursor control area)               */
    /*              o *cscb_inbuffer    (input buffer)                      */
    /*              o *cscb_outbuffer   (output buffer - SCC_GCMSG)         */
    /*              o *cscb_tuples      (formatted location)                */
    /*              o *cscb_rdata       (normal response block)             */
    /*              o *cscb_rpdata      (procedure response block)          */
    /*              o *cscb_evdata      (event message notification block)  */
    /*              o *cscb_dbuffer     (512-byte descriptor buffer)        */
    /*                                                                      */
    /* As a result of alignment requirements, padding may be allocated      */
    /* between each of these data areas.                                    */

    /*
    ** Calculate offsets to the major control blocks and buffers
    */
    if (need_facilities & (1 << DB_GCF_ID))
	mem_needed = (i4)sizeof(SCD_SCB);
    else
	/* Don't allocate SCC_CSCB part of SCD_SCB if no GCA */
	mem_needed = (i4)sizeof(SCD_SCB) - (i4)sizeof(SCC_CSCB);

    scb_size = mem_needed;

    /* After the SCB comes the Facilities cb area: */

    /* SCF, everyone needs SCF */
    offset_scf = mem_needed;
    mem_needed += sizeof(SCF_CB);
    mem_needed = DB_ALIGN_MACRO(mem_needed);

    /* ADF */
    if (need_facilities & (1 << DB_ADF_ID))
    {
	offset_adf = mem_needed;
	mem_needed += sizeof(ADF_CB) + DB_ERR_SIZE;
	mem_needed = DB_ALIGN_MACRO(mem_needed);
    }

    /* DMF */
    if (need_facilities & (1 << DB_DMF_ID))
    {
	offset_dmf = mem_needed;
	mem_needed += sizeof(DMC_CB) + Sc_main_cb->sc_dmbytes;
	mem_needed = DB_ALIGN_MACRO(mem_needed);
    }

    /* GWF */
    if (need_facilities & (1 << DB_GWF_ID))
    {
	offset_gwf = mem_needed;
	mem_needed += sizeof(GW_RCB) + Sc_main_cb->sc_gwbytes;
	mem_needed = DB_ALIGN_MACRO(mem_needed);
    }
    
    /* SXF */
    if (need_facilities & (1 << DB_SXF_ID))
    {
	offset_sxf = mem_needed;
	mem_needed += sizeof(SXF_RCB) + Sc_main_cb->sc_sxbytes;
	mem_needed = DB_ALIGN_MACRO(mem_needed);
    }

    /* OPF */
    if (need_facilities & (1 << DB_OPF_ID))
    {
	offset_opf = mem_needed;
	mem_needed += sizeof(OPF_CB) + Sc_main_cb->sc_opbytes;
	mem_needed = DB_ALIGN_MACRO(mem_needed);
    }

    /* PSF */
    if (need_facilities & (1 << DB_PSF_ID))
    {
	offset_psf = mem_needed;
	mem_needed += sizeof(PSQ_CB) + Sc_main_cb->sc_psbytes; 
	mem_needed = DB_ALIGN_MACRO(mem_needed);
    }

    /* QEF */
    if (need_facilities & (1 << DB_QEF_ID))
    {
	offset_qef = mem_needed;
	mem_needed += sizeof(QEF_RCB) + Sc_main_cb->sc_qebytes; 
	mem_needed = DB_ALIGN_MACRO(mem_needed);
    }

    /* QSF */
    if (need_facilities & (1 << DB_QSF_ID))
    {
	offset_qsf = mem_needed;
	mem_needed += sizeof(QSF_RCB) + Sc_main_cb->sc_qsbytes; 
	mem_needed = DB_ALIGN_MACRO(mem_needed);
    }

    /* RDF */
    if (need_facilities & (1 << DB_RDF_ID))
    {
	offset_rdf = mem_needed;
	mem_needed += sizeof(RDF_CCB) + Sc_main_cb->sc_rdbytes; 
	mem_needed = DB_ALIGN_MACRO(mem_needed);
    }

    /* After the facilities memory comes Factotum data... */
    if (thread_type == SCS_SFACTOTUM)
    {
	if (ftc->ftc_data && ftc->ftc_data_length)
	{
	    offset_facdata = mem_needed;
	    mem_needed = offset_facdata + ftc->ftc_data_length;
	    mem_needed = DB_ALIGN_MACRO(mem_needed);
	}
    }
    
    /* Then all the stuff for f/e GCA communications, if needed: */
    if (need_facilities & (1 << DB_GCF_ID)) 
    {
	offset_csitbl   = mem_needed;
	/* lint truncation warning if size of ptr > int, but code valid */
	offset_inbuffer = offset_csitbl +
			  (Sc_main_cb->sc_acc * sizeof(SCS_CSI));
	    
	offset_inbuffer = DB_ALIGN_MACRO(offset_inbuffer);

	offset_rdata	= offset_inbuffer +
			  (crb->gca_size_advise ? crb->gca_size_advise : 256);
	offset_rdata    = DB_ALIGN_MACRO(offset_rdata);
	offset_rpdata   = offset_rdata + (i4)sizeof(GCA_RE_DATA);
	offset_rpdata   = DB_ALIGN_MACRO(offset_rpdata);
	offset_evdata   = offset_rpdata + (i4)sizeof(GCA_RP_DATA);
	offset_evdata   = DB_ALIGN_MACRO(offset_evdata);
	offset_dbuffer  = offset_evdata + (i4)sizeof(SCC_GCEV_MSG);
	offset_dbuffer  = DB_ALIGN_MACRO(offset_dbuffer);

	mem_needed = offset_dbuffer + 512;

	/* Always allocate space for cursor text - 1500 * 16 is a small 
	** price to pay to allow display of cursor text. */
	sc924_requirement = Sc_main_cb->sc_acc * ER_MAX_LEN;
	if (sc924_requirement + mem_needed >= SC0M_MAXALLOC)
	{
	    allocate_sc924_separately = TRUE;
	    offset_curstext = 0;
	} else
	{
	    offset_curstext = mem_needed;
	    mem_needed += sc924_requirement;
	}
    }

    status = sc0m_allocate(0,
			   mem_needed, CS_SCB_TYPE,
			   (PTR) SCD_MEM, CS_TAG, (PTR *) scb_ptr);
    if (status != E_DB_OK)
	return(status);

    /*
    ** Initialize the major control blocks and buffers.
    */
    scb = *scb_ptr;
    block = (char *)scb;

    /* Zero-fill the SCB, all but the sc0m header portion */
    /*
    **  (chopr01) (b113605, changed scb_size to mem_needed, so that
    **  the memory allocated to the facilities is also initialized
    */
    MEfill(mem_needed - sizeof(SC0M_OBJECT), 0,
	    (char *)scb + sizeof(SC0M_OBJECT));

    /*
    ** Since cscb_outbuffer is now a separate allocation, this field MUST 
    ** be initialized
    */
    if (need_facilities & (1 << DB_GCF_ID))
	scb->scb_cscb.cscb_outbuffer = NULL;

    scb->cs_scb.cs_client_type = SCD_SCB_TYPE;
    scb->scb_sscb.sscb_stype = thread_type;
    scb->scb_sscb.sscb_flags = thread_flags;
    scb->scb_sscb.sscb_need_facilities = need_facilities;
    scb->scb_sscb.sscb_cpy_qeccb = NULL;

    /* Initialize the MCB */
    mcb_head = &scb->scb_sscb.sscb_mcb;
    mcb_head->mcb_next = mcb_head->mcb_prev = mcb_head;
    mcb_head->mcb_sque.sque_next = mcb_head->mcb_sque.sque_prev = mcb_head;
    mcb_head->mcb_session = (SCF_SESSION) scb;
    mcb_head->mcb_facility = DB_SCF_ID;

    /* Set pointers to the Facilities' control blocks and areas: */

    /* SCF */
    scb->scb_sscb.sscb_scccb = (SCF_CB *) (block + offset_scf);
    scb->scb_sscb.sscb_scscb = scb;

    /* ADF */
    if (need_facilities & (1 << DB_ADF_ID))
    {
	scb->scb_sscb.sscb_adscb = (ADF_CB *) (block + offset_adf);
    }

    /* DMF */
    if (need_facilities & (1 << DB_DMF_ID))
    {
	scb->scb_sscb.sscb_dmccb = (DMC_CB *) (block + offset_dmf);
	scb->scb_sscb.sscb_dmscb = (Sc_main_cb->sc_dmbytes > 0)
			? (char *)scb->scb_sscb.sscb_dmccb + sizeof(DMC_CB)
			: NULL;
	/* Only zero-fill the DMC_CB; DMF will take care of the SCB */
	MEfill(sizeof(DMC_CB), 0, scb->scb_sscb.sscb_dmccb);
    }
	
    /* GWF */
    if (need_facilities & (1 << DB_GWF_ID))
    {
	scb->scb_sscb.sscb_gwccb = (GW_RCB *) (block + offset_gwf);
	scb->scb_sscb.sscb_gwscb = (Sc_main_cb->sc_gwbytes > 0)
			? (char *)scb->scb_sscb.sscb_gwccb + sizeof(GW_RCB)
			: NULL;
    }

    /* SXF */
    if (need_facilities & (1 << DB_SXF_ID))
    {
	scb->scb_sscb.sscb_sxccb = (SXF_RCB *) (block + offset_sxf);
	scb->scb_sscb.sscb_sxscb = (Sc_main_cb->sc_sxbytes > 0)
			? (char *)scb->scb_sscb.sscb_sxccb + sizeof(SXF_RCB)
			: NULL;
    }

    /* OPF */
    if (need_facilities & (1 << DB_OPF_ID))
    {
	scb->scb_sscb.sscb_opccb = (OPF_CB *) (block + offset_opf);
	scb->scb_sscb.sscb_opscb = (Sc_main_cb->sc_opbytes > 0)
			? (char *)scb->scb_sscb.sscb_opccb + sizeof(OPF_CB)
			: NULL;
    }

    /* PSF */
    if (need_facilities & (1 << DB_PSF_ID))
    {
	scb->scb_sscb.sscb_psccb = (PSQ_CB *) (block + offset_psf);
	scb->scb_sscb.sscb_psscb = (Sc_main_cb->sc_psbytes > 0)
			? (PSF_SESSCB_PTR) ((char *)scb->scb_sscb.sscb_psccb + sizeof(PSQ_CB))
			: NULL;
    }

    /* QEF */
    if (need_facilities & (1 << DB_QEF_ID))
    {
	scb->scb_sscb.sscb_qeccb = (QEF_RCB *) (block + offset_qef);
	scb->scb_sscb.sscb_qescb = (Sc_main_cb->sc_qebytes > 0)
			? (QEF_CB *)((char *)scb->scb_sscb.sscb_qeccb + sizeof(QEF_RCB))
			: NULL;
    }

    /* QSF */
    if (need_facilities & (1 << DB_QSF_ID))
    {
	scb->scb_sscb.sscb_qsccb = (QSF_RCB *) (block + offset_qsf);
	scb->scb_sscb.sscb_qsscb = (Sc_main_cb->sc_qsbytes > 0)
			? (QSF_CB *)((char *)scb->scb_sscb.sscb_qsccb + sizeof(QSF_RCB))
			: NULL;
    }

    /* RDF */
    if (need_facilities & (1 << DB_RDF_ID))
    {
	scb->scb_sscb.sscb_rdccb = (RDF_CCB *) (block + offset_rdf);
	scb->scb_sscb.sscb_rdscb = (Sc_main_cb->sc_rdbytes > 0)
			? (char *)scb->scb_sscb.sscb_rdccb + sizeof(RDF_CCB)
			: NULL;
    }


    /* Factotum threads try to be very lightweight */
    if (thread_type == SCS_SFACTOTUM)
    {
	SCD_SCB		*parent_scb;

	/*
	** Copy ICS information from parent (creating) session's SCB
	** but only if the creating thread is not the Admin thread.
	** The Admin thread's SCB is made up of only the CS_SCB and
	** does not include SCF portion containing an ICS.
	** Without the copy, the ICS remains zero-filled.
	*/
	CSget_scb((CS_SCB**)&parent_scb);

	if ( parent_scb->cs_scb.cs_client_type == SCD_SCB_TYPE )
	{
	    MEcopy(&parent_scb->scb_sscb.sscb_ics,
	       sizeof(parent_scb->scb_sscb.sscb_ics),
	       &scb->scb_sscb.sscb_ics);

	    scb->scb_sscb.sscb_ics.ics_gw_parms = (PTR)NULL;
	    scb->scb_sscb.sscb_ics.ics_alloc_gw_parms = (PTR)NULL;

	    /*
	    ** If not otherwise requested, copy Parent's 
	    ** QEF_CB pointer. This enables exception handling
	    ** (qef_handler) in QEF factotum threads, 
	    ** which all share the main query's QEF_CB.
	    */
	    if ( !(need_facilities & (1 << DB_QEF_ID)) )
		scb->scb_sscb.sscb_qescb = parent_scb->scb_sscb.sscb_qescb;
	}
	else
	{
	    /* Explicitly zero fill the ICS as it resides beyond
	    ** the size of an SC0M_OBJECT and is not initialised above.
	    */
	    MEfill((SIZE_TYPE)sizeof(SCS_ICS), '\0', 
		   (PTR)&scb->scb_sscb.sscb_ics);
	}

	/* Use factotum thread name as "user name" */
	user_name = ftc->ftc_thread_name;

	/* But save parent's user name */
	parent_scb_username = parent_scb->cs_scb.cs_username;

	/* Retain terminal name from parent session */
	terminal_name = NULL;

	/*
	** Copy the remaining thread startup info 
	** that SCS will need to complete thread initialization:
	**
	**   thread entry code
	**   thread exit code
	**   thread input data
	**   sid of this creating (parent) thread
	*/
	scb->scb_sscb.sscb_factotum_parms.ftc_state = 0;
	scb->scb_sscb.sscb_factotum_parms.ftc_thread_entry = 
		ftc->ftc_thread_entry;
	scb->scb_sscb.sscb_factotum_parms.ftc_thread_exit = 
		ftc->ftc_thread_exit;
	scb->scb_sscb.sscb_factotum_parms.ftc_data = 
		ftc->ftc_data;
	scb->scb_sscb.sscb_factotum_parms.ftc_data_length = 
		ftc->ftc_data_length;
	scb->scb_sscb.sscb_factotum_parms.ftc_thread_id = 
		parent_scb->cs_scb.cs_self;

	/* Copy the input data for the thread */
	if (ftc->ftc_data && ftc->ftc_data_length)
	{
	    scb->scb_sscb.sscb_factotum_parms.ftc_data = 
			block + offset_facdata;
	    scb->scb_sscb.sscb_factotum_parms.ftc_data_length =
			ftc->ftc_data_length;

	    MEcopy(ftc->ftc_data,
		   ftc->ftc_data_length,
		   scb->scb_sscb.sscb_factotum_parms.ftc_data);
	}
    }

    /* If CRB, use its user name and terminal name */
    if (crb)
    {
	user_name = crb->gca_user_name;
	terminal_name = crb->gca_access_point_identifier;
    }

    /* Initialize user_name stuff in the ICS: */
    if (user_name)
    {
	/* calculate name length in i */
	for( i = 0, cp = user_name; *cp; i++, cp++ ) ;

	/*
	** For Star get the username.
	** The new Installation Password scheme requires Star to pass the username
	** and a NULL password to the GCA_REQUEST. The Name Server will fill in
	** the installation password for us. So, stop propogating the gca_listen
	** password, and stop looking at the aux data.
	*/
	if (thread_flags & SCS_STAR)
	{
	    DD_USR_DESC	*ruser;

	    ruser    = &scb->scb_sscb.sscb_ics.ics_real_user;

	    /* clear real user structure */
	    ruser->dd_u1_usrname[0] = EOS;
	    ruser->dd_u3_passwd[0]  = EOS;
	    ruser->dd_u4_status     = DD_0STATUS_UNKNOWN;

	    /* When dd_u2_usepw_b is FALSE, rqf will connect and then
	    ** modify the association like -u
	    */
	    ruser->dd_u2_usepw_b    = TRUE;
	    if (thread_type == SCS_S2PC)
		ruser->dd_u2_usepw_b    = FALSE;

	    MEmove( i, user_name, ' ',
		    sizeof(ruser->dd_u1_usrname), ruser->dd_u1_usrname );
	}

	/*
	** make ics_eusername point at the slot for the untranslated username 
	** (ics_xeusername).
	** later, this will be update to point to the slot for SQL-session <auth id>
	** (ics_susername).
	*/
	scb->scb_sscb.sscb_ics.ics_eusername =
	    &scb->scb_sscb.sscb_ics.ics_xeusername;

	/*
	** Mark the effective userid as delimited.
	** This may change later, if the user specified an effective userid
	** without delimiters.
	*/
	scb->scb_sscb.sscb_ics.ics_xmap = SCS_XDLM_EUSER;

	/*
	** store the untranslated username into the slots for SQL-session <auth id>
	** and system <auth id>; later, these will be updated to be the translated
	** versions (if appropriate).
	**
	** "i" holds the length of user_name.
	*/

	MEmove(i,
		    user_name,
		    ' ',
		    sizeof(scb->scb_sscb.sscb_ics.ics_xeusername),
		    (char *)scb->scb_sscb.sscb_ics.ics_eusername);

	MEcopy(scb->scb_sscb.sscb_ics.ics_eusername,
		    sizeof(scb->scb_sscb.sscb_ics.ics_xrusername),
		    &scb->scb_sscb.sscb_ics.ics_xrusername);

	MEcopy(scb->scb_sscb.sscb_ics.ics_eusername,
		    sizeof(scb->scb_sscb.sscb_ics.ics_susername),
		    &scb->scb_sscb.sscb_ics.ics_susername);

	MEcopy(scb->scb_sscb.sscb_ics.ics_eusername,
		    sizeof(scb->scb_sscb.sscb_ics.ics_rusername),
		    &scb->scb_sscb.sscb_ics.ics_rusername);

	MEcopy(scb->scb_sscb.sscb_ics.ics_eusername,
		    sizeof(scb->cs_scb.cs_username),
		    scb->cs_scb.cs_username);

	/* If factotum thread now override the effective userid for TXs */
	if (parent_scb_username)
	{
	    MEmove(i,
		    parent_scb_username,
		    ' ',
		    sizeof(scb->scb_sscb.sscb_ics.ics_xeusername),
		    (char *)scb->scb_sscb.sscb_ics.ics_eusername);
	}
    }

    if (terminal_name)
    {
	/* calculate length of access_point_identifier in i */
	for (i = 0, cp = terminal_name; cp && *cp; i++, cp++) ;
		
	if (i)
	    MEmove(i,
		    terminal_name,
		    ' ',
		    sizeof(scb->scb_sscb.sscb_ics.ics_terminal),
		    (char *)&scb->scb_sscb.sscb_ics.ics_terminal);
	else
	    MEmove(9,
		    "<unknown>",
		    ' ',
		    sizeof(scb->scb_sscb.sscb_ics.ics_terminal),
		    (char *)&scb->scb_sscb.sscb_ics.ics_terminal);
    }

    /*
    ** Initalize GCA unless this thread is not connected to a front end
    ** process.
    */
    if (need_facilities & (1 << DB_GCF_ID))
    {
	scb->scb_sscb.sscb_cursor.curs_csi =
		    (SCS_CSITBL *) (block + offset_csitbl);
	scb->scb_sscb.sscb_cursor.curs_limit = Sc_main_cb->sc_acc;
	scb->scb_sscb.sscb_cursor.curs_curno = 0;
	scb->scb_sscb.sscb_cursor.curs_frows = 0;

	for (i = 0; i < scb->scb_sscb.sscb_cursor.curs_limit; i++)
	{
	    scb->scb_sscb.sscb_cursor.curs_csi->csitbl[i].csi_state = 0;
	    scb->scb_sscb.sscb_cursor.curs_csi->csitbl[i].csi_textl = 0;
	    scb->scb_sscb.sscb_cursor.curs_csi->csitbl[i].csi_textp = NULL;
	}
	if (allocate_sc924_separately)
	{
	    PTR sc924_block = NULL;
	    i4 this_sc924 = 0;
	    for (i = 0; i < scb->scb_sscb.sscb_cursor.curs_limit; i++)
	    {
		if (sc924_block == NULL || this_sc924 < ER_MAX_LEN)
		{
		    this_sc924 = (scb->scb_sscb.sscb_cursor.curs_limit - i)
					* ER_MAX_LEN;
		    if ( this_sc924 > SC0M_MAXALLOC )
			this_sc924 = SC0M_MAXALLOC;
		    status = sc0m_allocate(0, this_sc924, CS_SCB_TYPE,
			(PTR) SCD_MEM, CS_TAG, (PTR *) &sc924_block);
		    if (status != E_DB_OK)
			return(status);
		}
		scb->scb_sscb.sscb_cursor.curs_csi->csitbl[i].csi_textp =
			sc924_block;
		this_sc924 -= ER_MAX_LEN;
		sc924_block += ER_MAX_LEN;
	    }
	} else
	{
	    for (i = 0; i < scb->scb_sscb.sscb_cursor.curs_limit; i++)
	    {
	    scb->scb_sscb.sscb_cursor.curs_csi->csitbl[i].csi_textp =
			(PTR)scb + offset_curstext;
	    offset_curstext += ER_MAX_LEN;
	}
	}

	scb->scb_sscb.sscb_cquery.cur_row_count = GCA_NO_ROW_COUNT;
	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_size = 0;
	scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc = 0;
	scb->scb_sscb.sscb_cquery.cur_qtxt = NULL;


	STRUCT_ASSIGN_MACRO(*crb, scb->scb_cscb.cscb_gcp.gca_ls_parms);
	scb->scb_cscb.cscb_assoc_id = crb->gca_assoc_id;
	scb->scb_cscb.cscb_comm_size = (crb->gca_size_advise ?
					   crb->gca_size_advise : 256);
	scb->scb_cscb.cscb_inbuffer =
		((char *) scb + offset_inbuffer);
	scb->scb_cscb.cscb_version = crb->gca_partner_protocol;
	if (((crb->gca_flags & GCA_DESCR_RQD) == GCA_DESCR_RQD) ||
	    (thread_flags & SCS_STAR))
	{
	    scb->scb_cscb.cscb_different = TRUE;
	}
	else
	{
	    scb->scb_cscb.cscb_different = FALSE;
	}

	status = sc0m_allocate(0,
	   sizeof(SCC_GCMSG) + Sc_main_cb->sc_maxtup,
	      SC_SCG_TYPE, (PTR) SCD_MEM, SCG_TAG, 
	      (PTR *) &scb->scb_cscb.cscb_outbuffer);
	if (status != E_DB_OK)
	{
	    local_status = sc0m_deallocate(0, (PTR *)&scb);
	    return(status);
	}

	/* Zero-fill the SCC_GCMSG, all but the sc0m header portion */
	MEfill((sizeof(SCC_GCMSG) + Sc_main_cb->sc_maxtup) - sizeof(SC0M_OBJECT), 0,
		(char *)scb->scb_cscb.cscb_outbuffer + sizeof(SC0M_OBJECT));

	scb->scb_cscb.cscb_tuples_max = Sc_main_cb->sc_maxtup;
	scb->scb_cscb.cscb_outbuffer->scg_mask = SCG_NODEALLOC_MASK;
	scb->scb_cscb.cscb_outbuffer->scg_marea =
		(PTR)scb->scb_cscb.cscb_outbuffer + sizeof(SCC_GCMSG);
	scb->scb_cscb.cscb_tuples =
	    scb->scb_cscb.cscb_outbuffer->scg_marea;

	scb->scb_cscb.cscb_rspmsg.scg_marea = (block + offset_rdata);
	scb->scb_cscb.cscb_rspmsg.scg_mask = SCG_NODEALLOC_MASK;
	scb->scb_cscb.cscb_rdata =
	    (GCA_RE_DATA *)scb->scb_cscb.cscb_rspmsg.scg_marea;

	scb->scb_cscb.cscb_rpmmsg.scg_marea = (block + offset_rpdata);
	scb->scb_cscb.cscb_rpmmsg.scg_mask = SCG_NODEALLOC_MASK;
	scb->scb_cscb.cscb_rpdata =
	    (GCA_RP_DATA *)scb->scb_cscb.cscb_rpmmsg.scg_marea;

	/* Allocate/preformat the dummy GCA_EVENT message out of the buffer */
	if ((thread_flags & SCS_STAR) == 0)
	{
	    scb->scb_cscb.cscb_evmsg.scg_marea = (block + offset_evdata);
	    scb->scb_cscb.cscb_evmsg.scg_mask = SCG_NODEALLOC_MASK;
	    scb->scb_cscb.cscb_evmsg.scg_type = GCA_EVENT;
	    scb->scb_cscb.cscb_evdata =
		(GCA_EV_DATA *)scb->scb_cscb.cscb_evmsg.scg_marea;
	}

	scb->scb_cscb.cscb_dscmsg.scg_marea =
	    scb->scb_cscb.cscb_dbuffer = (block + offset_dbuffer);
	scb->scb_cscb.cscb_dscmsg.scg_mask = SCG_NODEALLOC_MASK;
	scb->scb_cscb.cscb_dscmsg.scg_type = GCA_TDESCR;
	
	scb->scb_cscb.cscb_darea = (char *)scb->scb_cscb.cscb_dscmsg.scg_marea;
	scb->scb_cscb.cscb_dsize = 512;

	scb->scb_cscb.cscb_mnext.scg_prev =
		scb->scb_cscb.cscb_mnext.scg_next =
			(SCC_GCMSG *) &scb->scb_cscb.cscb_mnext; 
	scb->scb_cscb.cscb_qdone.scg_prev =
		scb->scb_cscb.cscb_qdone.scg_next =
			(SCC_GCMSG *) &scb->scb_cscb.cscb_qdone; 
	scb->scb_cscb.cscb_batch_count = 0;
	scb->scb_cscb.cscb_eog_error = FALSE;
    }

    /* Update session/thread counts */
    CSp_semaphore( TRUE, &Sc_main_cb->sc_misc_semaphore );

    Sc_main_cb->sc_session_count[thread_type].created++;

    if (++Sc_main_cb->sc_session_count[thread_type].current >
	Sc_main_cb->sc_session_count[thread_type].hwm)
    {
	Sc_main_cb->sc_session_count[thread_type].hwm = 
            Sc_main_cb->sc_session_count[thread_type].current;
    }

    /*
    ** The sequencer state for normal sessions is SCS_ACCEPT.  For special
    ** threads which have dedicated purpose, set SCS_SPECIAL state.
    */
    if (thread_flags & SCS_INTERNAL_THREAD)
    {
	scb->scb_sscb.sscb_state = SCS_SPECIAL;
    }
    else
    {
	/* There's a long gap between decision to accept this
	   connection and bumping the count here.  This may be a problem
	   if someone else can re-enter this code. */

	/* Note that sc_current_conns includes both NORMAL and MONITOR sessions */
	if ( ++Sc_main_cb->sc_current_conns > Sc_main_cb->sc_highwater_conns )
	   Sc_main_cb->sc_highwater_conns = Sc_main_cb->sc_current_conns;

	scb->scb_sscb.sscb_state = SCS_ACCEPT;
    }

    CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );

    return(E_DB_OK);
}

/*{
** Name: scd_dealloc_scb - toss out scb
**
** Description:
**      This routine deallocates the scb passed into it.
**
** Inputs:
**      scb                             Pointer to area to deallocate
**
** Outputs:
**      none
**	Returns:
**	    OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-Nov-1986 (fred)
**          Created.
**	09-Dec-1990 (anton)
**	    Semaphore is needed to deallocate memory
**	17-jul-92 (daveb)
**	    Detach from MO as well.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	26-Jul-1993 (daveb)
**	    If we're marked to terminate and this is the last session
**	    going away, kill the server.
**	27-Jul-1993 (daveb)
**	    Oh, by the way, this is a STATUS routine, called by the
**	    CL, and can't return DB_STATUS!
**	11-Oct-1993 (fpang)
**	    If a 'special' thread should exit through here, don't
**	    decrement sc_current_conns.
**	    Fixes B55202 (IIMONITOR, 'Set server shut' won't work for Star).
**	14-may-1997 (nanpr01)
**	    Deallocate descriptor pointer if allocated for vch_compress project.
**	12-Jun-1997 (jenjo02)
**	    Don't doubly take sc_misc_semaphore.
**	    Check thread type explicitly for SCS_SNORMAL, SCS_SMONITOR
**	    instead of lengthy check for every other type.
**	12-Mar-1998 (jenjo02)
**	    scb_sscb.sscb_mcb is now embedded in SCS_SSCB and does not
**	    need to be deallocated.
**	15-sep-1998 (somsa01)
**	    We were playing with sc_misc_semaphore AFTER releasing the SCB.
**	    Relocated the code which releases the SCB to the end.  (Bug #93332)
*/
STATUS
scd_dealloc_scb( CS_SCB *cscb )
{
    SCD_SCB	*scb = (SCD_SCB *) cscb;
    STATUS	 status;
    i4		 count;
    i4	 	 thread_type = scb->scb_sscb.sscb_stype;

    scs_scb_detach( cscb );

    /* Decrement current session count by session type */
    CSp_semaphore( TRUE, &Sc_main_cb->sc_misc_semaphore );

    Sc_main_cb->sc_session_count[thread_type].current--;

    if (thread_type == SCS_SNORMAL ||
	thread_type == SCS_SMONITOR)
    {
	Sc_main_cb->sc_current_conns--;

	/* MONITOR session are also counted as "normal" - go figure */
	if (thread_type == SCS_SMONITOR)
	    Sc_main_cb->sc_session_count[SCS_SNORMAL].current--;
    }

    if ( Sc_main_cb->sc_current_conns == 0 &&
         Sc_main_cb->sc_listen_mask & SC_LSN_TERM_IDLE )
    {
	CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );
	/* FIXME log something here?  Check status? */
	(void) CSterminate( CS_CLOSE, &count );
    }
    else
	CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );

    /*
    ** Release the scb.
    */
    if (scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc)
    {
	PTR                 block;

	block = (char *)scb->scb_sscb.sscb_cquery.cur_rdesc.rd_comp_tdesc -
		     sizeof(SC0M_OBJECT);
	status = sc0m_deallocate(0, &block);
    }
    
    if ((scb->scb_sscb.sscb_need_facilities & (1 << DB_GCF_ID)) 
    	&& scb->scb_cscb.cscb_outbuffer)
    {
	status = sc0m_deallocate(0, (PTR *)&scb->scb_cscb.cscb_outbuffer);
    }

    status = sc0m_deallocate(0, (PTR *)&scb);


    return( status );
}

/*{
** Name: scd_reject_assoc - Reject an association for reason given
**
** Description:
**      This routine is provided so that the underlying dispatcher 
**      can easily reject a session.
**
** Inputs:
**      crb                             Connection buffer
**      error                           Why rejected.
**
** Outputs:
**      none
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-Jul-1987 (fred)
**          Created.
**	26-jun-89 (jrb)
**	    Added generic error to return with termination.
**	10-aug-89 (jrb)
**	    Changed error return to give generic error only if protocol level
**	    was high enough.
**	20-Feb-1990 (anton)
**	    Don't log E_GC0032_NO_PEER - Common condition result of name server
**	    bed check.
**	29-Jan-1991 (anton)
**	    Call GCA_RQRESP for failed session - GCA API 2
**	    Change to call scs_disassoc instead of GCA directly
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              07-aug-1989 (georgeg)
**                  Fix stolen grom jupiter line, original fix by (fred).
**                  Initialize generic error parameter.
**          End of STAR comments.
**	2-Jul-1993 (daveb)
**	    prototyped.  remove unused var 'ebuf'
**	08-sep-93 (swm)
**	    Changed type of sid to CS_SID to reflect recent CL interface
**	    change.
**	29-Jul-1993 (daveb)
**	    Pass along actual 'error' value instead of
**	    FAIL, translated to 'number of users has exceeded limit!"
**	24-jan-1994 (gmanning)
**	    Call CSget_scb() instead of CSget_sid(), change references to
**	    CS_SCB as well.
**	16-Jul-96 (gordy)
**	    Add casts to IIGCa_call() to quite compiler warnings.
**	23-apr-1997 (canor01)
**	    When rejecting an association, log the reported error in
**	    addition to any GCA errors.  Reset the caller's gca_status
**	    to reflect the result of the disassoc.
**	16-Nov-1998 (jenjo02)
**	    CSsuspend on CS_BIOR_MASK instead of CS_BIO_MASK.
**       3-Dec-2003 (hanal04) Bug 113291 INGSRV3011
**         Map error of -1 to E_SC0123_SESSION_INITIATE. This avoids
**         having to declare SCF errors in csmthl.c which will set
**         error to -1 if we have been unable to create a new thread.
*/
STATUS
scd_reject_assoc( void *parm, STATUS error )
{
    GCA_LS_PARMS	*crb = (GCA_LS_PARMS *) parm;
    GCA_RR_PARMS        rrparms;
    DB_STATUS		status; 
    STATUS		gca_status;
    CS_SCB		*scb;
    
    if ( error == -1 )
    {
	error = E_SC0123_SESSION_INITIATE;
    }

    /* report reason for rejection */
    if ( error )
	sc0ePut( NULL, error, NULL, 0 );

    if (crb->gca_status && crb->gca_status != E_GC0000_OK)
    {
	if (crb->gca_status != E_GC0032_NO_PEER)
	    sc0ePut(NULL, crb->gca_status, &crb->gca_os_status, 0);

	status = scs_disassoc(crb->gca_assoc_id);
	if (status)
	{
	    sc0ePut(NULL, status, NULL, 0);
	}
	crb->gca_status = status;
	return(crb->gca_status);
    }

    CSget_scb(&scb);

    rrparms.gca_assoc_id = crb->gca_assoc_id;
    rrparms.gca_local_protocol = crb->gca_partner_protocol;
    rrparms.gca_modifiers = 0;
    rrparms.gca_request_status = error;

    status = IIGCa_call(GCA_RQRESP,
			    (GCA_PARMLIST *)&rrparms,
			    0,
			    (PTR)scb, 
			    (i4)-1,
			    &gca_status);

    if (status == OK)
	(VOID) CSsuspend(CS_BIOR_MASK, 0, 0);
    else
	sc0ePut(NULL, gca_status, &rrparms.gca_os_status, 0);

    status = scs_disassoc(crb->gca_assoc_id);

    return(status);
}

/*{r
** Name: scd_get_assoc	- Listen for an association request
**
** Description:
**      This routine asks GCF to give it an association request. 
**	It is pretty much just an interface to GCA_LISTEN, but 
**      exists to allow the dispatcher (in the CL) to call GCF. 
**      The listen buffer is passed in, so that no stack area 
**      is used, since this routine doesn't know its context.
**
** Inputs:
**      crb                             Pointer to listen buffer.
**      sync				Should this be a synchr. request.
**
** Outputs:
**      none
**	Returns:
**	    STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-Jul-1987 (fred)
**          Created.
**      24-apr-1990 (ralph)
**          Fix #9849: Track consecutive GCA_LISTEN failures in scd_get_assoc;
**          if > SC_GCA_LISTEN_MAX then shut the server down gracefully.
**	19-Nov-1992 (daveb)
**	    Handle E_GCFFFE_STATUS as needing resumption, for IMA/GCM.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	24-jan-1994 (gmanning)
**	    Call CSget_scb() instead of directly accessing structure members.  
**	02-feb-1994 (mikem)
**	    Back out 24-jan-94 change to scd_get_assoc().  IIGCa_call() to post
**	    a listen must get the "magic" CS_ADDER_ID scb, rather than the 
**	    current scb.  Before backing out this change no connections could
**	    could be made from client to server (they would eventually time 
**	    out).
**	07-sep-1995 (canor01)
**	    Don't log E_GC0032_NO_PEER - Common condition result of name server
**	    bed check and IMA/GCM termination in NT.
**	16-Jul-96 (gordy)
**	    Add casts to IIGCa_call() to quite compiler warnings.
**      10-Mar-2000 (wanfr01)
**          Bug 100847, INGSRV 1127
**          Allow configurable timeout so a direct connect to the server's
**          listen port does not indefinitely disable new connects.
**	7-Jan-2002 (wanfr01)
**	    Bug 108641, INGSRV 1875
**	    Cross integration of bug 100847 doesn't work correctly in 2.5.
**	    Partial backout/rewrite of fix.
*/
STATUS
scd_get_assoc( void *parm, i4  sync )
{
    GCA_LS_PARMS	*crb = (GCA_LS_PARMS *) parm;
    DB_STATUS		status;
    STATUS		error;
    i4                  count;
    i4			resume;

    resume = (crb->gca_status == E_GCFFFE_INCOMPLETE) ? GCA_RESUME : 0;

    status = IIGCa_call(GCA_LISTEN,
			(GCA_PARMLIST *)crb,	
			(sync ? GCA_SYNC_FLAG : 0) | resume,
			(PTR)CS_ADDER_ID,
			-1,
			&error);
    if (status)
	sc0ePut(NULL, error, NULL, 0);

    else if (crb->gca_status &&
             crb->gca_status != E_GC0000_OK &&
             crb->gca_status != E_GCFFFE_INCOMPLETE &&
             crb->gca_status != E_GCFFFF_IN_PROCESS
# ifdef NT_GENERIC
             && crb->gca_status != E_GC0032_NO_PEER
# endif
	    )
    {
        /*
        ** If too many consecutive GCA_LISTEN failures,
        ** then shut the server down gracefully.
        */
        if (Sc_main_cb->sc_gclisten_fail++ >= SC_GCA_LISTEN_MAX)
        {
	    sc0ePut(NULL, E_SC023A_GCA_LISTEN_FAIL, NULL, 0);
            CSterminate(CS_CLOSE, &count);
        }
    }

    else
    {
        /*
        ** Since we had a successful GCA_LISTEN, reset the
        ** consecutive failure counter.
        */
        Sc_main_cb->sc_gclisten_fail = 0;
    }

    return(status);
}

/*{
** Name: scd_disconnect - Disconnect an association for no particular reason
**
** Description:
**      This routine is provided so that the underlying dispatcher 
**      can easily disconnect a session.
**
** Inputs:
**      scb                             Session control block
**
** Outputs:
**      none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-Jul-1987 (fred)
**          Created.
**      28-Mar-1989 (fred)
**	    If GCA was not started, don't bother to try and end it.  Doing so
**	    puts GCF into an infinite loop trying to send 0 bytes over a channel.
**	26-jun-89 (jrb)
**	    Added generic error code.
**	10-aug-89 (jrb)
**	    Changed error return to give generic error only if protocol level
**	    was high enough.
**	8-Aug-91 (daveb)
**	    Change test for whether this is a SCF SCB to use correct
**	    cs_client_type field added to fix bug 38056.
**      23-jun-1992 (fpang)
**	    Sybil merge.
**          Start of merged STAR comments:
**              07-aug-1989 (georgeg)
**                  Fix stolen grom jupiter line, original fix by (fred).
**                  Initialize generic error parameter.
**          End of STAR comments.
**	29-oct-92 (andre)
**	    If communicating at protocol level >= 60, store encoded SQLSTATE in
**	    gca_id_error
**	2-Jul-1993 (daveb)
**	    prototyped.
**	24-jan-1994 (gmanning)
**	    Change references from sid to scb, remove references to cs_self,
**	    make calls to CSget_scb().  
**	16-Jul-96 (gordy)
**	    Fixed the size of the release message: no data value included.
**	    Add casts to IIGCa_call() to quite compiler warnings.
**      11-feb-97 (cohmi01)
**	    If current request was RAAT, send back msg in RAAT format. (B80665)
**	17-Oct-2000 (jenjo02)
**	    This function is called with SCD_SCB*; there's no need
**	    to call CSget_scb() to get the same answer.
**      26-feb-2002 (padni01) bug 102373
**          If the sequencer session control block's flags sscb_flags
**          indicates that the GCA connection has been dropped already or
**          is bad, do not call GCA 'again' to disconnect this session.
[@history_template@]...
*/
VOID
scd_disconnect( CS_SCB  *csscb )
{
    SCD_SCB		*scb = (SCD_SCB *) csscb;
    GCA_ER_DATA		*ebuf;
    DB_STATUS		status;
    STATUS		gca_status = 0;

    if ((scb->cs_scb.cs_type != (i2) CS_SCB_TYPE) ||
	    (scb->cs_scb.cs_client_type  != SCD_SCB_TYPE))
	return;	/* this is not an SCF scb */
    if ((scb->scb_sscb.sscb_facility & (1 << DB_GCF_ID)) == 0)
	return;	/* Never started GCF, don't end it */

    if (scb->scb_sscb.sscb_flags & SCS_DROPPED_GCA)
	    return;

    if (IS_RAAT_REQUEST_MACRO(scb))
    {
	/* Do the RAAT equivalent of the code that follows */
	status = scs_raat_term_msg(scb, E_SC0206_CANNOT_PROCESS);
    }
    else
    {
	ebuf = (GCA_ER_DATA *)scb->scb_cscb.cscb_inbuffer;
	ebuf->gca_l_e_element = 1;

	if (scb->scb_cscb.cscb_version <= GCA_PROTOCOL_LEVEL_2)
	{
	    ebuf->gca_e_element[0].gca_local_error = 0;
	    ebuf->gca_e_element[0].gca_id_error = E_SC0206_CANNOT_PROCESS;
	}
	else if (scb->scb_cscb.cscb_version < GCA_PROTOCOL_LEVEL_60)
	{
	    ebuf->gca_e_element[0].gca_local_error = E_SC0206_CANNOT_PROCESS;
	    ebuf->gca_e_element[0].gca_id_error = E_GE98BC_OTHER_ERROR;
	}
	else
	{
	    ebuf->gca_e_element[0].gca_local_error = E_SC0206_CANNOT_PROCESS;
	    ebuf->gca_e_element[0].gca_id_error =
		ss_encode(SS50000_MISC_ERRORS);
	}
	
	ebuf->gca_e_element[0].gca_id_server = Sc_main_cb->sc_pid;
	ebuf->gca_e_element[0].gca_severity = 0;
	ebuf->gca_e_element[0].gca_l_error_parm = 0;
	CSp_semaphore(TRUE, &Sc_main_cb->sc_gcsem);
	Sc_main_cb->sc_gcdc_sdparms.gca_association_id =
	    scb->scb_cscb.cscb_assoc_id;
	Sc_main_cb->sc_gcdc_sdparms.gca_buffer = scb->scb_cscb.cscb_inbuffer;
	Sc_main_cb->sc_gcdc_sdparms.gca_message_type = GCA_RELEASE;
	Sc_main_cb->sc_gcdc_sdparms.gca_msg_length = sizeof(GCA_ER_DATA) -
						     sizeof(GCA_DATA_VALUE);
	Sc_main_cb->sc_gcdc_sdparms.gca_end_of_data = TRUE;
	Sc_main_cb->sc_gcdc_sdparms.gca_modifiers = 0;
	status = IIGCa_call(GCA_SEND,
			    (GCA_PARMLIST *)&Sc_main_cb->sc_gcdc_sdparms,
			    GCA_NO_ASYNC_EXIT,
			    (PTR)scb,
			    -1,
			&gca_status);
	CSv_semaphore(&Sc_main_cb->sc_gcsem);
    }

    if (status)
    {
	sc0ePut(NULL, gca_status, NULL, 0);
    }
}



/*{
** Name:	scd_allow_connect -- OK for user to connect?
**
** Description:
**	Decides whether the user in the CRB is allowed to make a special
**	connection.  This is based on the SC_LSN_SPECIAL_OK state,
**	and on the user's privileges as determined by the PM file.  We
**	put off looking at the PM file as long as possible to avoid the
**	expense.
**
** Re-entrancy:
**	Describe whether the function is or is not re-entrant,
**	and what clients may need to do to work in a threaded world.
**
** Inputs:
**	crb		The connection block.
**
** Outputs:
**	none.
**
** Returns:
**	OK
**	error status	to reject the request with.
**
** History:
**	28-Jul-1993 (daveb)
**	    created.
**       9-Dec-1993 (daveb)
**          Changed to meet current LRC approved PM vars, use new
**  	    error for closed server.
**	06-may-1998 (canor01)
**	    Check the license before allowing session to connect.
**	13-jul-1998 (canor01)
**	    If license check has failed, disallow all new connections, even
**	    special connections with server control privilege.
**	15-Apr-1998 (jenjo02)
**	    If regular listens are allowed but connection count exceded,
**	    leave failed status as US0001 instead of the confusing
**	    SC0345.
**      08-sep-1999 (devjo01)
**          Test SC_LIC_VIOL_REJECT bit instead of calling CIcapabilities()
**          when checking if server is licensed. (b98689)
*/

static STATUS
scd_allow_connect( GCA_LS_PARMS *crb )
{
    STATUS stat;

    CSp_semaphore( 0, &Sc_main_cb->sc_misc_semaphore ); /* shared */

    if( (Sc_main_cb->sc_listen_mask & SC_LSN_REGULAR_OK) &&
       Sc_main_cb->sc_current_conns < Sc_main_cb->sc_max_conns )
    {
	stat = OK;
    }
    else			/* too many regular users */
    {
	stat = E_US0001;
    }
    CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );

    if ( stat == OK && (Sc_main_cb->sc_capabilities & SC_LIC_VIOL_REJECT) )
    {
	/* 
	** If license check has failed, disallow all new connections
	*/
# ifdef WORKGROUP_EDITION
	return (E_US0001);
# else /* WORKGROUP_EDITION */
	return (E_SC0345_SERVER_CLOSED);
# endif /* WORKGROUP_EDITION */
    }

    if ( stat != OK && (Sc_main_cb->sc_listen_mask & SC_LSN_SPECIAL_OK) )
    {
	CSp_semaphore( 0, &Sc_main_cb->sc_misc_semaphore ); /* shared */
	if( Sc_main_cb->sc_current_conns <
	   (Sc_main_cb->sc_max_conns + Sc_main_cb->sc_rsrvd_conns ) )
	{
	    CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );
	    /* is this a deserving user? */
	    
	    if( scs_chk_priv( crb->gca_user_name, "SERVER_CONTROL" ) )
	    {
		/* Log special connect? */
		stat = OK;
	    }
	    else 
	    {
		/* If regular connections are ok, leave stat as E_US0001 */
		if ((Sc_main_cb->sc_listen_mask & SC_LSN_REGULAR_OK) == 0)
		    stat = E_SC0345_SERVER_CLOSED;
	    }
	}
	else
	    CSv_semaphore( &Sc_main_cb->sc_misc_semaphore );
    }
    return( stat );
}

static VOID
scd_pmerr_func(STATUS err_number, i4  num_arguments, ER_ARGUMENT *args)
{
    sc0ePut(NULL, err_number, NULL, num_arguments, args);
}
