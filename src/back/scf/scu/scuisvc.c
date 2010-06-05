/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>
#include    <er.h>
#include    <ex.h>
#include    <pc.h>
#include    <cv.h>
#include    <cs.h>
#include    <tm.h>
#include    <lo.h>
#include    <me.h>
#include    <mo.h>
#include    <st.h>

#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dudbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <gca.h>

#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>

/* added for scs.h prototypes, ugh! */
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefqeu.h>
#include <qefcopy.h>
#include <psfparse.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <scu.h>
#include    <sc0m.h>
#include    <scfcontrol.h>

#include    <dmacb.h>

# include    <sxf.h>
# include    <sc0a.h>
/**
**
**  Name: SCUISVC.C - code to provide information about a user session
**
**  Description:
**      This file contains the code for the SCF which provides information 
**      to users (other facilities) about a users session.  This information 
**      may include but is not limited to information about the user, 
**      information about the session, information about the server, 
**      and information about the system on which they are running. 
** 
**      A partial list of the information provided can be found in 
**      the file SCF.H, referring to the information request block 
**      structure.  Some requests may return temporary session values
**	from the session's QEF_CB.
**
**          scu_information() - Get session information
**          scu_idefine()    - Place information in the scb (Module test only)
[@func_list@]...
**
**
**  History:
**      20-Jan-86 (fred)    
**          Created on jupiter
**	31-oct-1988 (rogerk)
**	    Add options to get server statistics - SCI_DBCPUTIME, SCI_DBDIOCNT,
**	    SCI_DBBIOCNT.
**	    Call CSaltr_session to turn on cpu collecting when SCI_CPUTIME is
**	    requested.
**	22-Mar-1989 (fred)
**	    Use Sc_main_cb->sc_proxy_scb instead of current SCB when it is set.
**	    This allows work to be done on behalf of other sessions.
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Add support for the following in scu_information and scu_idefine:
**	    SCI_GROUP, SCI_APLID, SCI_DBPRIV, SCI_QROW_LIMIT, SCI_QDIO_LIMIT,
**	    SCI_QCPU_LIMIT, SCI_QPAGR_LIMIT, SCI_QCOST_LIMIT.
**      05-apr-1989 (jennifer)
**          Add return of security label to scu_information.
**	21-may-89 (jrb)
**	    Change to ule_format interface.
**	30-may-89 (ralph)
**	    GRANT Enhancements, Phase 2c:
**	    Add support for the following in scu_information:
**	    SCI_TAB_CREATE, SCI_PROC_CREATE, SCI_SET_LOCKMODE.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Add support for the following in scu_information and scu_idefine:
**	    SCI_MXIO, SCI_MXROW, SCI_MXCPU, SCI_MXPAGE, SCI_MXCOST, SCI_MXPRIV,
**	    SCI_IGNORE.
**	23-sep-89 (ralph)
**	    Add support for SCI_FLATTEN in scu_information.
**	10-oct-89 (ralph)
**	    Add support for SCI_OPF_MEM in scu_information.
**	17-oct-89 (ralph)
**	    Change sense of /NOFLATTEN
**	05-jan-90 (andre)
**	    Add support for DBMSINFO('FIPS')
**	08-aug-90 (ralph)
**	    Add support for SCI_SECSTATE -- dbmsinfo('secstate').
**	    Add support for SCI_UPSYSCAT -- dbmsinfo('update_syscat').
**	05-feb-91 (ralph)
**	    Add support for SCI_SESSID -- dbmsinfo('session_id')
**	18-feb-91 (ralph)
**	    Correct UNIX compiler warnings
**	01-jul-91 (andre)
**	    Added support for dbmsinfo('session_user') (SCI_SESS_USER),
**	    dbmsinfo('session_group') (SCI_SESS_GROUP), and
**	    dbmsinfo('session_role') (SCI_SESS_ROLE)
**	8-Aug-91, 27-feb-92 (daveb)
**	    Change test for whether this is a SCF SCB to use correct
**	    cs_client_type field added to fix b 38056.
**	13-jul-92 (rog)
**	    Included ddb.h for Sybil, and er.h because of a change to scs.h.
**	28-jul-92 (pholman)
**	    Introduced support for SCI_AUDIT_LOG (SECURITY_AUDIT_LOG) dbmsinfo
**	    request (C2).  Include lo.h and sxf.h
**	14-aug-92 (pholman)
**	    SCI_SECSTATE now obsolete, request will return an empty string, also
**	    SCI_AUDIT_LOG will now return a PTR to a text name of the repository
**	    for security audit records.
**	25-sep-92 (robf)
**	    Check real, not effective user for security.
**	22-Oct-1992 (daveb)
**	    Add GWF facility scb retrieval.
**	05-nov-92 (markg)
**	    Call SXF using the sc_show_state function pointer. This
**	    is to get round a shared image linking problem on VMS.
**	02-dec-92 (andre)
**	    removed support for dbmsinfo('session_group') and
**	    dbmsinfo('session_role'); added support for dbmsinfo('initial_user')
**	    (SCI_INITIAL_USER)
**
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**	23-Nov-1992 (daveb)
**	    Add SCI_IMA_VNODE, SCI_IMA_SERVER, SCI_IMA_SESSION info requests.
**	06-oct-1992 (robf)
**	    Add SCI_SECURITY call for dbmsinfo('SECURITY_PRIV').
**	27-nov-1992 (robf)
**	    Modularize security auditing calls thorugh sc0a_write_audit()
**	17-jan-1993 (ralph)
**	    Added support for FIPS dbmsinfo requests
**	12-Mar-1993 (daveb)
**	    Add include <st.h> <me.h>
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	29-Jun-1993 (daveb)
**	    cast arg to CSget_scb correctly.
**	2-Jul-1993 (daveb)
**	    prototyped, remove func externs.
**	06-jul-1993 (shailaja)
**          Fixed prototype incompatibilities.
**	7-Jul-1993 (daveb)
**	    fixed headers for prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	27-jul-1993 (ralph)
**	    DELIM_IDENT:
**	    Added support for SCI_DBSERVICE to return the database's dbservice.
**	08-aug-1993 (shailaja)
**	    Fixed argument incompatibility in case SCI_DBSERVICE.
**	12-aug-93 (swm)
**	    Cast first parameter of CSaltr_session() to CS_SID to match
**	    revised CL interface specification.
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**	20-oct-93 (robf)
**          Add SCI_AUDIT_STATE, SCI_TABLESTATS, SCI_IDLE_LIMIT,
**	        SCI_CONNECT_LIMIT, SCI_PRIORITY_LIMIT, SCU_CUR_PRIORITY
**	        SCI_MXCONNECT, SCI_MXIDLE
**	17-dec-93 (rblumer)
**	    removed the SCI_FIPS dbmsinfo option.
**	14-feb-94 (rblumer)
**	    removed FLATOPT and FLATNONE dbmsinfo requests (part of B59120);
**	    changed cursor request name from CURSOR_DIRECT/CURSOR_DEFERRED to
**	    single CURSOR_UPDATE_MODE (part of B59119; also LRC 09-feb-1994).
**	10-mar-94 (rblumer)
**	    blank-pad result strings for CSRUPDATE; otherwise can get garbage
**	    printed out for last few characters on some platforms (e.g. VMS).
**	12-dec-1995 (nick)
**	    Added SCI_REAL_USER_CASE to SCI_INFO_TABLE[]
**      06-mar-96 (nanpr01)
**          added SCI_MXRECLEN to return maximum tuple length available for
**          that installation. This is for variable page size project to
**          support increased tuple length for star.
**	    Also added SCI_MXPAGESZ, SCI_PAGE_2K, SCI_RECLEN_2K,
**	    SCI_PAGE_4K, SCI_RECLEN_4K, SCI_PAGE_8K, SCI_RECLEN_8K,
**	    SCI_PAGE_16K, SCI_RECLEN_16K, SCI_PAGE_32K, SCI_RECLEN_32K,
**	    SCI_PAGE_64K, SCI_RECLEN_64K to inquire about availabity of
**	    bufferpools and its tuple sizes respectively through dbmsinfo
**	    function. If particular buffer pool is not available, return
**	    0.
**	09-apr-1997 (canor01)
**	    Another (CS_SID) vs (CS_SCB*) change.  For SCI_IMA_SESSION,
**	    return the CS_SID.
**	10-nov-1998 (sarjo01)
**	    Added SCI_TIMEOUT_ABORT.
**	01-oct-1999 (somsa01)
**	    Added SCI_SPECIAL_UFLAGS, which retrieves the SCS_ICS->ics_uflags
**	    bits in the form of SCS_ICS_UFLAGS.
**	30-Nov-1999 (jenjo02)
**	    Added SCI_DISTRANID which returns a pointer to sscb_dis_tran_id.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototypes for
**	    scu_information(), scu_idefine(), scu_malloc(),
**	    scu_mfree(), scu_xencode().
**	19-Jan-2001 (jenjo02)
**	    Stifle calls to sc0a_write_audit() if not C2SECURE.
**      06-apr-2001 (stial01)
**          Added code for dbmsinfo('page_type_v*')
**      20-apr-2001 (stial01)
**	    Added code for dbmsinfo('unicode') case SCI_UNICODE
**      14-may-2001 (stial01)
**          Replaced dbmsinfo('unicode') with dbmsinfo('unicode_level')
**      10-jun-2003 (stial01)
**          Added SCI_QBUF, SCI_QLEN, SCI_TRACE_ERRNO
**      17-jun-2003 (stial01)
**          Added SCI_LAST_QBUF, SCI_LAST_QLEN
**	23-jun-2003 (somsa01)
**	   For LP64 platforms, SCI_SESSION should be 16. Also, removed
**	    changes for bug 78975, which was a hack.
**      04-Sep-2003 (hanje04)
**        BUG 110855 - INGDBA2507
**        Make dbmsinfo('ima_server') return varchar(64) instead of
**        varchar(32) to stop long (usually fully qualified) hostnames
**        from being truncated.
**      11-apr-2005 (stial01)
**          Added SCI_CSRLIMIT for dbms('cursor_limit')
**	12-apr-2005 (gupsh01)
**	    Added support for UNICODE_NORMALIZATION.
**	04-may-2005 (abbjo03)
**	    Correct 12-apr change:  SCI_UNICODE_NORM should have been added at
**	    the end of the Sci_info_table.
**      21-feb-2007 (stial01)
**          Added SCI_PSQ_QBUF, SCI_PSQ_QLEN, renamed SCI_LAST_QBUF, SCI_LAST_QLEN
**      20-dec-2007 (stial01)
**          scu_information *QBUF/*QLEN valid for operational dbms/star servers
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value scf_error structure.
**	    Use new form of uleFormat().
**      11-dec-2008 (stial01)
**          SCI_AUDIT_STATE STmove destination buffer size NOT src length
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**      14-jan-2009 (stial01)
**          Added support for dbmsinfo('pagetype_v5')
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
**  Forward and/or External function/variable references.
*/

GLOBALREF SC_MAIN_CB	*Sc_main_cb;        /* central scf cb */

/*
** Name: SCI_INFO_TABLE - lengths and other info about SCI op codes
**
** Description:
**      This is a simple readonly table to allow quick access to the required 
**      length fields for SCU_INFORMATION operation codes.
**
** History:
**     1-Apr-86 (fred)
**          Created on Jupiter.
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added the following:
**	    SCI_GROUP, SCI_APLID, SCI_DBPRIV, SCI_QROW_LIMIT, SCI_QDIO_LIMIT,
**	    SCI_QCPU_LIMIT, SCI_QPAGR_LIMIT, SCI_QCOST_LIMIT.
**	30-may-89 (ralph)
**	    GRANT Enhancements, Phase 2c:
**	    Add support for the following in scu_information:
**	    SCI_TAB_CREATE, SCI_PROC_CREATE, SCI_SET_LOCKMODE.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Added the following:
**	    SCI_MXIO, SCI_MXROW, SCI_MXCPU, SCI_MXPAGE, SCI_MXCOST, SCI_MXPRIV,
**	    SCI_IGNORE.
**	23-sep-89 (ralph)
**	    Add support for SCI_FLATTEN in scu_information.
**	10-oct-89 (ralph)
**	    Add support for SCI_OPF_MEM in scu_information.
**	05-jan-90 (andre)
**	    Add support for DBMSINFO('FIPS')
**	08-aug-90 (ralph)
**	    Add support for SCI_SECSTATE -- dbmsinfo('secstate')
**	    Add support for SCI_UPSYSCAT -- dbmsinfo('update_syscat')
**	    Add support for SCI_DB_ADMIN -- dbmsinfo('db_admin')
**	    Remove support for SCI_OPF_MEM
**	05-feb-91 (ralph)
**	    Add support for SCI_SESSID -- dbmsinfo('session_id')
**	01-jul-91 (andre)
**	    Added support for dbmsinfo('session_user') (SCI_SESS_USER),
**	    dbmsinfo('session_group') (SCI_SESS_GROUP), and
**	    dbmsinfo('session_role') (SCI_SESS_ROLE)
**	06-oct-1992 (robf)
**	    Add SCI_SECURITY call for dbmsinfo('SECURITY_PRIV').
**	02-dec-92 (andre)
**	    removed support for dbmsinfo('session_group') and
**	    dbmsinfo('session_role'); added support for dbmsinfo('initial_user')
**	23-Nov-1992 (daveb)
**	    Add SCI_IMA_VNODE, SCI_IMA_SERVER, SCI_IMA_SESSION info requests.
**	17-jan-1993 (ralph)
**	    Added support for FIPS dbmsinfo requests
**	02-jun-1993 (ralph)
**	    DELIM_IDENT:
**	    Added support for SCI_DBSERVICE to return the database's dbservice.
**	9-sep-93 (robf)
**	    Added SCI_MAXUSTAT for maximum privilege set
**	    Added SCI_QRYSYSCAT for QUERY_SYSCAT privilege
**	17-dec-93 (rblumer)
**	    fixed bug 58079 by changing SCI_FLATAGG to look at correct bit flag.
**	    Also removed the SCI_FIPS dbmsinfo option.
**	12-dec-1995 (nick)
**	    Added SCI_REAL_USER_CASE. #71800
**      06-mar-96 (nanpr01)
**          added SCI_MXRECLEN to return maximum tuple length available for
**          that installation. This is for variable page size project to
**          support increased tuple length for star.
**	    Also added SCI_MXPAGESZ, SCI_PAGE_2K, SCI_RECLEN_2K,
**	    SCI_PAGE_4K, SCI_RECLEN_4K, SCI_PAGE_8K, SCI_RECLEN_8K,
**	    SCI_PAGE_16K, SCI_RECLEN_16K, SCI_PAGE_32K, SCI_RECLEN_32K,
**	    SCI_PAGE_64K, SCI_RECLEN_64K to inquire about availabity of
**	    bufferpools and its tuple sizes respectively through dbmsinfo
**	    function. If particular buffer pool is not available, return
**	    0.
**	01-oct-1999 (somsa01)
**	    Added SCI_SPECIAL_UFLAGS, which retrieves the SCS_ICS->ics_uflags
**	    bits in the form of SCS_ICS_UFLAGS.
**	30-Nov-1999 (jenjo02)
**	    Added SCI_DISTRANID which returns a pointer to sscb_dis_tran_id.
*	10-may-02 (inkdo01)
**	    Added SCI_SEQ_CREATE for sequence creation privilege.
**	23-jun-2003 (somsa01)
**	   For LP64 platforms, SCI_SESSION should be 16.
**      04-Sep-2003 (hanje04)
**        BUG 110855 - INGDBA2507
**        Make dbmsinfo('ima_server') return varchar(64) instead of
**        varchar(32) to stop long (usually fully qualified) hostnames
**        from being truncated.
**	18-Oct-2004 (shaha03)
**	  SIR #112918, Added configurable default cursor open mode support.
**	03-Oct-2006 (gupsh01)
**	  Added SCI_DTTYPE_ALIAS. 
**	11-Oct-2006 (gupsh01)
**	  Added missing entries to SCI_INFO_TABLE. 
**	7-Nov-2007 (kibro01) b119428
**	  Added SCI_DATE_FORMAT and SCI_MONEY_FORMAT
**	20-Nov-2007 (kibro01) b119428
**	  Replace the non-portable strlen("multinational4")
**	26-Oct-2009 (kiria01) SIR 121883
**	    Scalar sub-query support: Added CARDINALITY_CHECK
*/
typedef struct _SCI_INFO_TABLE
{
    i4		    it_code;	    /* code for item whose size is below */
    i4              it_size;        /* The size required for a particular opcode */
} SCI_INFO_TABLE;

/*
**  Definition of static variables and forward static functions.
*/

#ifdef	lint
static		 SCI_INFO_TABLE Sci_info_table[] = {
#else
static  const SCI_INFO_TABLE Sci_info_table[] = {
#endif
    { 0, 0 },
    { SCI_USERNAME, sizeof(DB_OWN_NAME) } ,
    { SCI_UCODE, 2 } ,
    { SCI_DBNAME, sizeof(DB_DB_NAME) } ,
    { SCI_PID, sizeof(PID) },
    { SCI_SID, sizeof(SCF_SESSION) } ,
    { SCI_USTAT, sizeof(i4) } ,
    { SCI_DSTAT, sizeof(i4) } ,
    { SCI_ERRORS, sizeof(i4) } ,
    { SCI_IVERSION, DB_TYPE_MAXLEN } ,
    { SCI_COMMTYPE, sizeof(i4) } ,
    { SCI_SCFVERSION, 4 } ,
    { SCI_MEMORY, sizeof(i4) },
    { SCI_NOUSERS, sizeof(i4) },
    { SCI_SNAME, DB_TYPE_MAXLEN } ,
    { SCI_CPUTIME, sizeof(i4) } ,
    { SCI_PFAULTS, sizeof(i4) } ,
    { SCI_DIOCNT, sizeof(i4) },
    { SCI_BIOCNT, sizeof(i4) },
    { SCI_NOW, sizeof(i4) },
    { SCI_SCB, sizeof(PTR) },
    { SCI_DBID, sizeof(PTR) }, 
    { SCI_UTTY, sizeof(DB_TERM_NAME) },
    { SCI_LANGUAGE, sizeof(i4) },
    { SCI_DBA, sizeof(DB_OWN_NAME) },
    { SCI_DBCPUTIME, sizeof(i4) } ,
    { SCI_DBDIOCNT, sizeof(i4) },
    { SCI_DBBIOCNT, sizeof(i4) },
    { SCI_GROUP, sizeof(DB_OWN_NAME) },
    { SCI_APLID, sizeof(DB_OWN_NAME) },
    { SCI_DBPRIV, sizeof(u_i4) },
    { SCI_QROW_LIMIT, sizeof(i4) },
    { SCI_QDIO_LIMIT, sizeof(i4) },
    { SCI_QCPU_LIMIT, sizeof(i4) },
    { SCI_QPAGE_LIMIT, sizeof(i4) },
    { SCI_QCOST_LIMIT, sizeof(i4) },
    { SCI_TAB_CREATE, 1 },
    { SCI_PROC_CREATE, 1 },
    { SCI_SET_LOCKMODE, 1 },
    { SCI_MXIO, sizeof(i4) },
    { SCI_MXROW, sizeof(i4) },
    { SCI_MXCPU, sizeof(i4) },
    { SCI_MXPAGE, sizeof(i4) },
    { SCI_MXCOST, sizeof(i4) },
    { SCI_MXPRIV, sizeof(u_i4) },
    { SCI_IGNORE, 0 },
    { SCI_IGNORE, 0 },
    { SCI_RUSERNAME, sizeof(DB_OWN_NAME) },
    { SCI_TIMEOUT_ABORT, 1 },
    { SCI_DB_ADMIN, 1 },
    { SCI_DISTRANID, sizeof(PTR) },
    { SCI_SV_CB, sizeof(PTR) },
    { SCI_SECSTATE, 32 },
    { SCI_UPSYSCAT, 1 },
# ifdef LP64
    { SCI_SESSID, 16 } ,
# else
    { SCI_SESSID, 8 } ,
# endif  /* LP64 */
    { SCI_INITIAL_USER, sizeof(DB_OWN_NAME) },
    { SCI_SESSION_USER, sizeof(DB_OWN_NAME) },
    { SCI_SECURITY, 1 },
    { SCI_AUDIT_LOG, 32 },
    { SCI_IMA_VNODE, 32 },
    { SCI_IMA_SERVER, 64 },
    { SCI_IMA_SESSION, 32 },
    { SCI_CSRUPDATE, 8 },
    { SCI_SERVER_CAP, sizeof(i4) }, 
    { SCI_SEQ_CREATE, 1 },
    { SCI_FLATTEN, 1 },
    { SCI_FLATAGG, 1 },
    { SCI_FLATSING, 1 },
    { SCI_NAME_CASE, 5 },
    { SCI_DELIM_CASE, 5 },
    { SCI_DBSERVICE, sizeof(u_i4) },
    { SCI_IGNORE, 0 },
    { SCI_IGNORE, 0 },
    { SCI_IGNORE, 0 },
    { SCI_IGNORE, 0 },
    { SCI_IGNORE, 0 },
    { SCI_IGNORE, 0 },
    { SCI_IGNORE, 0 },
    { SCI_IGNORE, 0 },
    { SCI_IGNORE, 0 },
    { SCI_IGNORE, 0 },
    { SCI_MAXUSTAT, sizeof(i4) } ,
    { SCI_QRYSYSCAT, 1 },
    { SCI_AUDIT_STATE, 32},
    { SCI_TABLESTATS, 1},
    { SCI_IDLE_LIMIT, sizeof(i4)},
    { SCI_CONNECT_LIMIT, sizeof(i4)},
    { SCI_PRIORITY_LIMIT, sizeof(i4)},
    { SCI_CUR_PRIORITY, sizeof(i4)},
    { SCI_MXIDLE, sizeof(i4)},
    { SCI_MXCONNECT, sizeof(i4)},
    { SCI_IGNORE, 0 },
    { SCI_REAL_USER_CASE, 5 },
    { SCI_MXRECLEN, sizeof(i4)},
    { SCI_MXPAGESZ, sizeof(i4)},
    { SCI_PAGE_2K, 1},
    { SCI_RECLEN_2K, sizeof(i4)},
    { SCI_PAGE_4K, 1},
    { SCI_RECLEN_4K, sizeof(i4)},
    { SCI_PAGE_8K, 1},
    { SCI_RECLEN_8K, sizeof(i4)},
    { SCI_PAGE_16K, 1},
    { SCI_RECLEN_16K, sizeof(i4)},
    { SCI_PAGE_32K, 1},
    { SCI_RECLEN_32K, sizeof(i4)},
    { SCI_PAGE_64K, 1},
    { SCI_RECLEN_64K, sizeof(i4)},
    { SCI_SPECIAL_UFLAGS, sizeof(SCS_ICS_UFLAGS)},
    { SCI_PAGETYPE_V1, 1},
    { SCI_PAGETYPE_V2, 1},
    { SCI_PAGETYPE_V3, 1},
    { SCI_PAGETYPE_V4, 1},
    { SCI_UNICODE, sizeof(i4)},
    { SCI_LP64, 1},
    { SCI_QBUF, sizeof(PTR)},
    { SCI_QLEN, sizeof(i4)},
    { SCI_TRACE_ERRNO, sizeof(i4)},
    { SCI_PREV_QBUF, sizeof(PTR)},
    { SCI_PREV_QLEN, sizeof(i4)},
    { SCI_CSRDEFMODE, 8 },
    { SCI_CSRLIMIT, sizeof(i4)},
    { SCI_UNICODE_NORM, 3 },
    { SCI_CURDATE, sizeof(i4)},
    { SCI_CURTIME, sizeof(i4)},
    { SCI_CURTSTMP, sizeof(i4)},
    { SCI_LCLTIME, sizeof(i4)},
    { SCI_LCLTSTMP, sizeof(i4)},
    { SCI_DTTYPE_ALIAS, 10 },
    { SCI_PSQ_QBUF, sizeof(PTR)},
    { SCI_PSQ_QLEN, sizeof(i4)},
    { SCI_DATE_FORMAT, sizeof("multinational4") },
    { SCI_MONEY_FORMAT, 10 },
    { SCI_MONEY_PREC, 4 },
    { SCI_DECIMAL_FORMAT, 2 },
    { SCI_PAGETYPE_V5, 1},
    { SCI_FLATCHKCARD, 1 },
    {0}
};

/*{
** Name: scu_information	- obtain session information for a user
**
** Description:
**      This function is issued by the scf entry point in response to a request
**      for information (SCU_INFORMATION) operation by a user of scf. 
**      Items of information, as described below, are requested by filling
**      in the information request block (scf_sci) portion of the scf control 
**      block.  Multiple requests will be serviced at one time -- this structure 
**      is an array.
** 
**      If there is an error encountered while processing the requests, 
**      the item on which the error was encountered will have its index entered 
**      into the data field of the DB_ERROR structure.  The order of evaluation 
**      of the requests is not guaranteed, so should such an error be encountered, 
**      there is no method through which the user can determine if other 
**      requested items have been filled in.
**
** Inputs:
**      SCU_INFORMATION			op code to scf_call()
**      scf_cb                          requesting control block...
**	    .scf_len_union.scf_ilength	number of requested information items
**	    .scf_ptr_union.scf_sci	ptr to array indicating requested items
**	    sci_list[]			the aforementioned array, which has ...
**		.sci_code               what item is requested
**		.sci_length             length of buffer provided for answer
**		.sci_aresult            address to put answer
**		.sci_rlength		address to put length(answer)
**					(can be NULL if not interested)
**      (This structure is repeated scf_ilength times)
**
** Outputs:
**      .scf_error                          filled in with error if encountered
**	    .err_code                       one of ...
**		   E_SC_OK
**			wonderful job by requestor.  Or ...
**                 E_SC0007_INFORMATION
**			invalid item - which one in data
**                 E_SC0008_INFO_TRUNCATED
**			information did not fit in requested area - item in data
**		    E_SC00009_BUFFER
**			buffer provided for information is not addressable -
**			 item in data
**		    E_SC000A_NO_BUFFER
**			request for information but no place to put it
**		    E_SC000B_NO_REQUEST
**			ptr to/portion of SCI blk not addressable - item in data
**	    .data                       index of offending item, as described
**					above.  intended as a programmer aid
**      *scf_sci->sci_list[].sci_aresult           filled in with the information requested
**      *scf_sci->sci_list[].sci_rlength           filled in with the length if requested
**	Returns:
**	    one of the regular E_DB_{OK, WARNING, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	20-Jan-86 (fred)
**          Created on jupiter
**	31-oct-1988 (rogerk)
**	    Add options to get server statistics.
**	    Call CSaltr_session to turn on cpu collecting when SCI_CPUTIME is
**	    requested.
**	    Add new parameter to CSstatistics call.
**	    Only call CSstatistics once.
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added support for the following:
**	    SCI_GROUP, SCI_APLID, SCI_DBPRIV, SCI_QROW_LIMIT, SCI_QDIO_LIMIT,
**	    SCI_QCPU_LIMIT, SCI_QPAGR_LIMIT, SCI_QCOST_LIMIT.
**      05-apr-1989 (jennifer)
**          Add return of security label to scu_information.
**	30-may-89 (ralph)
**	    GRANT Enhancements, Phase 2c:
**	    Add support for the following in scu_information:
**	    SCI_TAB_CREATE, SCI_PROC_CREATE, SCI_SET_LOCKMODE.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Add support for the following in scu_information:
**	    SCI_MXIO, SCI_MXROW, SCI_MXCPU, SCI_MXPAGE, SCI_MXCOST, SCI_MXPRIV,
**	    SCI_IGNORE.
**	23-sep-89 (ralph)
**	    Add support for SCI_FLATTEN in scu_information.
**	10-oct-89 (ralph)
**	    Add support for SCI_OPF_MEM in scu_information.
**	17-oct-89 (ralph)
**	    Change sense of /NOFLATTEN
**	18-dec-89 (teg)

**	    add support for SCI_SV_CB in scu_information
**	05-jan-90 (andre)
**	    Add support for DBMSINFO('FIPS')
**	08-aug-90 (ralph)
**	    Add support for SCI_SECSTATE -- dbmsinfo('secstate')
**	    Add support for SCI_UPSYSCAT -- dbmsinfo('update_syscat')
**	    Add support for SCI_DB_ADMIN -- dbmsinfo('db_admin')
**	    Remove support for SCI_OPF_MEM
**	05-feb-91 (ralph)
**	    Add support for SCI_SESSID -- dbmsinfo('session_id')
**	21-may-91 (andre)
**	    renamed some fields in SCS_ICS to make it easier to determine by the
**	    name whether the field contains info pertaining to the system (also
**	    known as "Real"), Session ("he who invoked us"), or the Effective
**	    identifier:
**		ics_ustat     -->  ics_rustat	    real user status bits
**		ics_username  -->  ics_eusername    effective user id
**		ics_ucode     -->  ics_sucode       copy of the session user id
**		ics_grpid     -->  ics_egrpid       effective group id
**		ics_aplid     -->  ics_eaplid       effective role id
**	01-jul-91 (andre)
**	    Added support for dbmsinfo('session_user') (SCI_SESS_USER),
**	    dbmsinfo('session_group') (SCI_SESS_GROUP), and
**	    dbmsinfo('session_role') (SCI_SESS_ROLE)
**	8-Aug-91, 27-feb-92 (daveb)
**	    Change test for whether this is a SCF SCB to use correct
**	    cs_client_type field added to fix bug 38056.
**	22-Oct-1992 (daveb)
**	    Add GWF facility scb retrieval.
**	05-nov-1992 (markg)
**	    Call SXF using the sc_show_state function pointer. This
**	    is to get round a shared image linking problem on VMS.

**	02-dec-92 (andre)
**	    removed support for dbmsinfo('session_group') and
**	    dbmsinfo('session_role'); added support for dbmsinfo('initial_user')
**
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**
**	    the following fields in SCS_ICS have been renamed:
**		ics_sucode	--> ics_iucode
**	23-Nov-1992 (daveb)
**	    Add SCI_IMA_{VNODE,SERVER,SESSION}.
**	06-oct-1992 (robf)
**	    Add SCI_SECURITY call for dbmsinfo('SECURITY_PRIV').
**	17-jan-1993 (ralph)
**	    Added support for FIPS dbmsinfo requests
**	29-Jun-1993 (daveb)
**	    cast arg to CSget_scb correctly.
**	2-Jul-1993 (daveb)
**	    prototyped.
**	7-Jul-1993 (daveb)
**	    removed unused variable 'dmacb'.
**	27-jul-1993 (ralph)
**	    DELIM_IDENT:
**	    Added support for SCI_DBSERVICE to return the database's dbservice.
**	08-sep-93 (swm)
**	    Call MOptrout rather than MOulongout for scb pointer.
**	17-dec-93 (rblumer)
**	    fixed bug 58079 by changing SCI_FLATAGG to look at correct bit flag.
**	    Also removed the SCI_FIPS dbmsinfo option.
**	    "FIPS mode" no longer exists.  It was replaced some time ago by
**	    several feature-specific flags (e.g. flatten_nosingleton and
**	    direct_cursor_mode).
**	03-jan-94 (rblumer)
**	    fixed bug 58309 by changing SYSTEM_USER to return the untranslated
**	    user name (xrusername) instead of the translated one (rusername).
**	14-feb-94 (rblumer)
**	    removed FLATOPT and FLATNONE dbmsinfo requests (part of B59120)
**	    and changed SCI_FLATTEN to look at sc_flatflags not sc_noflatten;
**	    changed cursor-mode values (part of B59119; also LRC 09-feb-1994).
**	10-mar-94 (rblumer)
**	    blank-pad result strings for CSRUPDATE; otherwise can get garbage
**	    printed out for last few characters on some platforms (e.g. VMS).
**	3-jun-94 (robf)
**          Add back missing MXCPU request lost during earlier changes.
**	18-feb-97 (radve01/thaju02)
**	    bug #78975 - select dbmsinfo('session_id') returned incorrect 
**	    value.  Added #ifdef PTR_BITS_64, for 64 bit platforms use 
**	    only lower half of session id (handled like front end).  Note:
**	    This is a temporary fix.  64 bit platforms need re-developing
**	    as to how session id is handled.
**      06-mar-96 (nanpr01)
**          added SCI_MXRECLEN to return maximum tuple length available for
**          that installation. This is for variable page size project to
**          support increased tuple length for star.
**	    Also added SCI_MXPAGESZ, SCI_PAGE_2K, SCI_RECLEN_2K,
**	    SCI_PAGE_4K, SCI_RECLEN_4K, SCI_PAGE_8K, SCI_RECLEN_8K,
**	    SCI_PAGE_16K, SCI_RECLEN_16K, SCI_PAGE_32K, SCI_RECLEN_32K,
**	    SCI_PAGE_64K, SCI_RECLEN_64K to inquire about availabity of
**	    bufferpools and its tuple sizes respectively through dbmsinfo
**	    function. If particular buffer pool is not available, return
**	    0.
**	09-apr-1997 (canor01)
**	    Another (CS_SID) vs (CS_SCB*) change.  For SCI_IMA_SESSION,
**	    return the CS_SID.
**	01-oct-1999 (somsa01)
**	    Added SCI_SPECIAL_UFLAGS, which retrieves the SCS_ICS->ics_uflags
**	    bits in the form of SCS_ICS_UFLAGS.
**	30-Nov-1999 (jenjo02)
**	    Added SCI_DISTRANID which returns a pointer to sscb_dis_tran_id.
**      08-may-2002 (stial01)
**          Added dbmsinfo('lp64'), returns 'Y' or 'N'.
**	10-may-02 (inkdo01)
**	    Added SCI_SEQ_CREATE for sequence creation privilege.
**	23-jun-2003 (somsa01)
**	    Removed changes for bug 78975, which was a hack.
**      04-Sep-2003 (hanje04)
**        BUG 110855 - INGDBA2507
**        Make dbmsinfo('ima_server') return varchar(64) instead of
**        varchar(32) to stop long (usually fully qualified) hostnames
**        from being truncated.
**      05-oct-2007 (huazh01)
**        don't set up query text info if it is a recovery server.
**        This fixes b119231. 
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Add SCI_PAGETYPE_V6, SCI_PAGETYPE_V7
*/
DB_STATUS
scu_information(SCF_CB	*cb, SCD_SCB *scb )
{
    DB_STATUS		status;
    STATUS		stat;
    i4		index;	    /* which request failed */
    i4			count;
    i4		lcount;
    PTR			where;
    PTR			which;
    SCF_SCI		*item;
    i4			junk;
    i4			len;	    /* length of info transmitted */
    i4			tm_needed;
    i4			tm_parm;
    i4			tm_done = 0;
    i4		error;
    i4		alter_parm;
    TIMERSTAT		stat_block;
    char		char_32buf[32 + 1];
    char		char_maxbuf[64 + 1];
    i4			mo_perms;
    i4			mo_size;
    i4			unicode_level;
    DB_ERROR	        dberror;
    SCS_ICS_UFLAGS	ics_uflags;
    char		*qbuf;
    char		*prev_qbuf;
    i4			qlen;
    i4			prevqlen;
    char		*psqbuf;
    i4			psqlen;

    status = E_DB_OK;
    CLRDBERR(&cb->scf_error);

    if ((cb->scf_len_union.scf_ilength <= 0) || (!cb->scf_ptr_union.scf_sci))
    {
	SETDBERR(&cb->scf_error, 0, E_SC000B_NO_REQUEST);
	return(E_DB_ERROR);
    }

    for (index = 0; index < cb->scf_len_union.scf_ilength; index++)
    {
	item = &cb->scf_ptr_union.scf_sci->sci_list[index];
	if (item->sci_length <= 0)
	{
	    SETDBERR(&cb->scf_error, index, E_SC0027_BUFFER_LENGTH);
	    status = E_DB_ERROR;
	    break;
	}
	if (!item->sci_aresult)
	{
	    SETDBERR(&cb->scf_error, index, E_SC0009_BUFFER);
	    status = E_DB_ERROR;
	    break;
	}
	tm_needed = 0;

	switch (item->sci_code)
	{
	case SCI_USERNAME:
	    where = (PTR) scb->scb_sscb.sscb_ics.ics_eusername;
	    break;

	case SCI_UCODE:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_iucode;  /* is an array */
	    break;
	    
	case SCI_DBNAME:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_dbname;
	    break;

	case SCI_PID:
	    where = (PTR) &Sc_main_cb->sc_pid;
	    break;

	case SCI_SID:
	    where = (PTR) &scb->cs_scb.cs_self;
	    break;

	case SCI_USTAT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_rustat;
	    break;

	case SCI_MAXUSTAT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_maxustat;
	    break;

	case SCI_DSTAT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_dbstat;
	    break;

	case SCI_ERRORS:
	    where = (PTR) &scb->scb_sscb.sscb_noerrors;
	    break;

	case SCI_IVERSION:
	    where = (PTR) Sc_main_cb->sc_iversion;
	    break;

	case SCI_COMMTYPE:
/*	    where = (PTR) &scb->scb_cscb.cscb_method;*/
	    break;

	case SCI_SCFVERSION:
	    where = (PTR) &Sc_main_cb->sc_scf_version;
	    break;

	case SCI_NOUSERS:
	    where = (PTR) &Sc_main_cb->sc_nousers;
	    break;

	case SCI_SNAME:
	    where = Sc_main_cb->sc_sname;    /* is an array -- no & */
	    break;

	case SCI_MEMORY:
	    /* always returns OK */
	    scu_mcount(scb, &count);
	    where = (PTR) &count;
	    break;

	case SCI_CPUTIME:
	    /*
	    ** If requesting session CPU time, need to call CSaltr_session
	    ** to turn on collecting of CPU statistics for this thread.
	    */
	    alter_parm = 1;
	    stat = CSaltr_session(scb->cs_scb.cs_self, 
			CS_AS_CPUSTATS, (PTR)&alter_parm);
	    if ((stat != OK)
		&&
		(stat != E_CS001F_CPUSTATS_WASSET))
	    {
		uleFormat(NULL, stat, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)0,
		    (i4)0, (i4 *)0, &error, 0);
		SETDBERR(&cb->scf_error, index, E_SC0007_INFORMATION);
		status = E_DB_ERROR;
		break;
	    }

	    tm_needed++;
	    tm_parm = 0;
	    where = (PTR) &stat_block.stat_cpu;
	    break;

	case SCI_DBCPUTIME:
	    tm_needed++;
	    tm_parm = 1;
	    where = (PTR) &stat_block.stat_cpu;
	    break;

	case SCI_PFAULTS:
	    tm_needed++;
	    where = (PTR) &stat_block.stat_pgflts;
	    break;

	case SCI_DIOCNT:
	    tm_needed++;
	    tm_parm = 0;
	    where = (PTR) &stat_block.stat_dio;
	    break;

	case SCI_DBDIOCNT:
	    tm_needed++;
	    tm_parm = 1;
	    where = (PTR) &stat_block.stat_dio;
	    break;

	case SCI_BIOCNT:
	    tm_needed++;
	    tm_parm = 0;
	    where = (PTR) &stat_block.stat_bio;
	    break;

	case SCI_DBBIOCNT:
	    tm_needed++;
	    tm_parm = 1;
	    where = (PTR) &stat_block.stat_bio;
	    break;

	case SCI_NOW:
	    where = (PTR) &lcount;
	    lcount = TMsecs();
	    break;

	case SCI_SCB:
	    switch (cb->scf_facility)
	    {
		case DB_ADF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_adscb;
		    break;
		case DB_DMF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_dmscb;
		    break;
		case DB_OPF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_opscb;
		    break;
		case DB_PSF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_psscb;
		    break;
		case DB_QEF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_qescb;
		    break;
		case DB_QSF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_qsscb;
		    break;
		case DB_RDF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_rdscb;
		    break;
		case DB_SCF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_scscb;
		    break;
		case DB_GWF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_gwscb;
		    break;
		case DB_SXF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_sxscb;
		    break;
		default:
		    SETDBERR(&cb->scf_error, index, E_SC0018_BAD_FACILITY);
		    status = E_DB_ERROR;
		    break;
	    }
	    break;

	case SCI_DBID:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_opendb_id;
	    break;

	case SCI_UTTY:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_terminal;
	    break;
	    
	case SCI_LANGUAGE:
	    if (scb->cs_scb.cs_client_type == SCD_SCB_TYPE )
	    {
		where = (PTR) &scb->scb_sscb.sscb_ics.ics_language;
	    }
	    else
	    {
		where = (PTR) &junk;
		junk = 1;
	    }
	    break;
	    
	case SCI_DBA:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_dbowner;
	    break;
	    
	case SCI_GROUP:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_egrpid;
	    break;
	    
	case SCI_APLID:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_eaplid;
	    break;
	    
	case SCI_DBPRIV:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_fl_dbpriv;
	    break;
	    
	case SCI_QROW_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_qrow_limit;
	    break;
	    
	case SCI_QDIO_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_qdio_limit;
	    break;
	    
	case SCI_QCPU_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_qcpu_limit;
	    break;
	    
	case SCI_QPAGE_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_qpage_limit;
	    break;
	    
	case SCI_QCOST_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_qcost_limit;
	    break;

	case SCI_MXIDLE:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_cur_idle_limit;
	    break;

	case SCI_IDLE_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_idle_limit;
	    break;

	case SCI_MXCONNECT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_cur_connect_limit;
	    break;

	case SCI_CONNECT_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_connect_limit;
	    break;

	case SCI_PRIORITY_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_priority_limit;
	    break;

	case SCI_CUR_PRIORITY:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_cur_priority;
	    break;

	case SCI_TAB_CREATE:
	    where = (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_TAB_CREATE) ?
		    (PTR)"Y" : (PTR)"N";
	    break;

	case SCI_QRYSYSCAT:
	    where = (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_QUERY_SYSCAT) ?
		    (PTR)"Y" : (PTR)"N";
	    break;

	case SCI_TABLESTATS:
	    where = (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_TABLE_STATS) ?
		    (PTR)"Y" : (PTR)"N";
	    break;

	case SCI_PROC_CREATE:
	    where = (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_PROC_CREATE) ?
		    (PTR)"Y" : (PTR)"N";
	    break;

	case SCI_SEQ_CREATE:
	    where = (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_SEQ_CREATE) ?
		    (PTR)"Y" : (PTR)"N";
	    break;

	case SCI_SET_LOCKMODE:
	    where = (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_LOCKMODE) ?
		    (PTR)"Y" : (PTR)"N";
	    break;

        case SCI_TIMEOUT_ABORT:
            where = (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_TIMEOUT_ABORT)
?
                    (PTR)"Y" : (PTR)"N";
            break;

	case SCI_MXIO:
	    where = (PTR) scb->scb_sscb.sscb_qescb;
	    if (where != NULL)
		where = (PTR)&((QEF_CB *)where)->qef_dio_limit;
	    break;

	case SCI_MXROW:
	    where = (PTR) scb->scb_sscb.sscb_qescb;
	    if (where != NULL)
		where = (PTR)&((QEF_CB *)where)->qef_row_limit;
	    break;

	case SCI_MXPAGE:
	    where = (PTR) scb->scb_sscb.sscb_qescb;
	    if (where != NULL)
		where = (PTR)&((QEF_CB *)where)->qef_page_limit;
	    break;

        case SCI_MXCPU:
             where = (PTR) scb->scb_sscb.sscb_qescb;
             if (where != NULL)
                  where = (PTR)&((QEF_CB *)where)->qef_cpu_limit;
	     break;

	case SCI_MXCOST:
	    where = (PTR) scb->scb_sscb.sscb_qescb;
	    if (where != NULL)
		where = (PTR)&((QEF_CB *)where)->qef_cost_limit;
	    break;
	    
	case SCI_MXPRIV:
	    where = (PTR) scb->scb_sscb.sscb_qescb;
	    if (where != NULL)
		where = (PTR)&((QEF_CB *)where)->qef_fl_dbpriv;
	    break;

	case SCI_DB_ADMIN:
	    where = (scb->scb_sscb.sscb_ics.ics_fl_dbpriv & DBPR_DB_ADMIN) ?
		    (PTR)"Y" : (PTR)"N";
	    break;

	case SCI_IGNORE:
	    where = (PTR)NULL;
	    break;

	case SCI_RUSERNAME:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_xrusername;
	    break;

        case SCI_SESSID:
            where = (PTR) scb->scb_sscb.sscb_ics.ics_sessid;
            break;

	case SCI_INITIAL_USER:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_iusername;
	    break;

	case SCI_SESSION_USER:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_susername;
	    break;

	case SCI_SV_CB:
	    switch (cb->scf_facility)
	    {
		case DB_ADF_ID:
		    where = (PTR) &Sc_main_cb->sc_advcb;
		    break;
		case DB_DMF_ID:
		    where = (PTR) &Sc_main_cb->sc_dmvcb;
		    break;
		case DB_OPF_ID:
		    where = (PTR) &Sc_main_cb->sc_opvcb;
		    break;
		case DB_PSF_ID:
		    where = (PTR) &Sc_main_cb->sc_psvcb;
		    break;
		case DB_QEF_ID:
		    where = (PTR) &Sc_main_cb->sc_qevcb;
		    break;
		case DB_QSF_ID:
		    where = (PTR) &Sc_main_cb->sc_qsvcb;
		    break;
		case DB_RDF_ID:
		    where = (PTR) &Sc_main_cb->sc_rdvcb;
		    break;
		default:
		    SETDBERR(&cb->scf_error, index, E_SC0018_BAD_FACILITY);
		    status = E_DB_ERROR;
		    break;
	    }
	    break;

	case SCI_SECSTATE:
	    /*
	    ** This request was made obsolete, before appearing in an
	    ** INGRES release.  The information it revealed is visble
	    ** through the iisecurity_state view in the iidbdb.
	    */
	    where = (PTR) char_32buf;
	    MEfill(item->sci_length,(unsigned char)ERx(' '),where);
	    break;

	case SCI_UPSYSCAT:
	    where = (scb->scb_sscb.sscb_ics.ics_requstat & DU_UUPSYSCAT) ?
		    (PTR)"Y" : (PTR)"N";
	    break;


 	case SCI_SECURITY:
            where = (scb->scb_sscb.sscb_ics.ics_rustat & DU_USECURITY) ?
                     (PTR)"Y" : (PTR)"N";
            break;

	case SCI_AUDIT_LOG:
            {
	        /*
	        ** Return the name of the current Security Audit Logfile.
	        **
	        ** Restricted to users with MAINTAIN_AUDIT privilege.
	        */
		SXF_RCB sxfrcb;
		SXF_ACCESS access;
                MEfill(sizeof(sxfrcb), NULLCHAR, (PTR)&sxfrcb);
                sxfrcb.sxf_cb_type    = SXFRCB_CB;
                sxfrcb.sxf_length     = sizeof(sxfrcb);

	        where = char_32buf;
	        if (scb->scb_sscb.sscb_ics.ics_rustat & DU_UALTER_AUDIT)
	        {
		    char auditlog[MAX_LOC + 1];

		    sxfrcb.sxf_filename  = (PTR)auditlog;
		    if ((status = (*Sc_main_cb->sc_sxf_cptr)
					(SXS_SHOW, &sxfrcb)) != E_DB_OK)
		    {
			SETDBERR(&cb->scf_error, index, E_SC0007_INFORMATION);
	                status = E_DB_ERROR;
		    }
		    else 
		    {
		        STmove(auditlog,ERx(' '),item->sci_length,where);
		    }
                    access = (SXF_A_SUCCESS | SXF_A_SELECT);
	        }
	        else
	        {	/* Lacks MAINTAIN_AUDIT */
		    /*
		    ** Return Blank string, so that no information is passed 
		    ** back to the user
		    */
	            MEfill(item->sci_length, (unsigned char)ERx(' '), where);
                    access = (SXF_A_FAIL | SXF_A_SELECT);
		}
		/*
		** Write Security Audit Record, about this attempt to 
		** obtain security information
		**
		*/
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		{
		    status = sc0a_write_audit(
			    scb,
			    SXF_E_SECURITY,		/* Type */
			    access,  		/* Access */
			    "AUDIT_LOG_NAME",	/* Object Name */
			    sizeof("AUDIT_LOG_NAME")-1,	
			    NULL,	                /* Object Owner */
			    I_SX2518_ENQUIRE_AUDIT_LOG,	/* Mesg ID */
			    FALSE,			/* Force record */
			    0,			/* Privs */
			    &dberror		/* Error location */
			    );	

		    if (status!=E_DB_OK)
			SETDBERR(&cb->scf_error, index, E_SC0007_INFORMATION);
		}
	    }
	    break;
	
	case SCI_AUDIT_STATE:
            {
	        /*
	        ** Return the current Security Audit State
	        **
	        ** Restricted to users with MAINTAIN_AUDIT privilege.
	        */
		SXF_RCB sxfrcb;
		SXF_ACCESS access;
                MEfill(sizeof(sxfrcb), NULLCHAR, (PTR)&sxfrcb);
                sxfrcb.sxf_cb_type    = SXFRCB_CB;
                sxfrcb.sxf_length     = sizeof(sxfrcb);

	        where = char_32buf;
	        if (scb->scb_sscb.sscb_ics.ics_rustat & DU_UALTER_AUDIT)
	        {
		    sxfrcb.sxf_filename  = NULL;
		    sxfrcb.sxf_auditstate= 0;
		    if ((status = (*Sc_main_cb->sc_sxf_cptr)
					(SXS_SHOW, &sxfrcb)) != E_DB_OK)
		    {
			SETDBERR(&cb->scf_error, index, E_SC0007_INFORMATION);
	                status = E_DB_ERROR;
		    }
		    else 
		    {
			if(sxfrcb.sxf_auditstate==SXF_S_SUSPEND_AUDIT)
			    STmove("SUSPEND",ERx(' '),sizeof(char_32buf),where);
			else if(sxfrcb.sxf_auditstate==SXF_S_STOP_AUDIT)
			    STmove("STOP",ERx(' '),sizeof(char_32buf),where);
			else  if(Sc_main_cb->sc_capabilities & SC_C_C2SECURE)
			    STmove("ACTIVE",ERx(' '),sizeof(char_32buf),where);
			else
			    STmove("",ERx(' '),sizeof(char_32buf),where);
		    }
                    access = (SXF_A_SUCCESS | SXF_A_SELECT);
	        }
	        else
	        {	/* Lacks MAINTAIN_AUDIT */
		    /*
		    ** Return Blank string, so that no information is passed 
		    ** back to the user
		    */
	            MEfill(sizeof(char_32buf), (unsigned char)ERx(' '), where);
                    access = (SXF_A_FAIL | SXF_A_SELECT);
		}
		/*
		** Write Security Audit Record, about this attempt to 
		** obtain security information
		**
		*/
		if ( Sc_main_cb->sc_capabilities & SC_C_C2SECURE )
		{
		    status = sc0a_write_audit(
			    scb,
			    SXF_E_SECURITY,		/* Type */
			    access,  		/* Access */
			    "AUDIT_STATE",	/* Object Name */
			    sizeof("AUDIT_STATE")-1,	
			    NULL,	                /* Object Owner */
			    I_SX273D_ENQUIRE_AUDIT_STATE,	/* Mesg ID */
			    FALSE,			/* Force record */
			    0,			/* Privs */
			    &dberror		/* Error location */
			    );	

		    if (status!=E_DB_OK)
			SETDBERR(&cb->scf_error, index, E_SC0007_INFORMATION);
		}
	    }
	    break;
	case SCI_IMA_VNODE:
	    /* FIXME */
	    where = (PTR) char_maxbuf;
	    mo_size = sizeof( char_maxbuf );
	    stat = MOget( ~0, "exp.gwf.gwm.glb.def_vnode", "0",
			 &mo_size, where, &mo_perms );
	    if ( stat != OK )
	    {
		uleFormat(NULL, stat, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)0,
			   (i4)0, (i4 *)0, &error, 0);
		SETDBERR(&cb->scf_error, index, E_SC0007_INFORMATION);
		status = E_DB_ERROR;
		break;
	    }
	    /* fill out with blanks */
	    for( junk = 0; where[junk] && junk < sizeof(char_maxbuf); junk++ )
		continue;
	    while( junk < sizeof(char_maxbuf) )
		where[ junk++ ] = ' ';
		
	    break;

	case SCI_IMA_SERVER:

	    /* FIXME */
	    where = (PTR) char_maxbuf;
	    mo_size = sizeof( char_maxbuf );
	    stat = MOget( ~0, "exp.gwf.gwm.glb.this_server", "0",
			 &mo_size, where, &mo_perms );
	    if ( stat != OK )
	    {
		uleFormat(NULL, stat, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)0,
			   (i4)0, (i4 *)0, &error, 0);
		SETDBERR(&cb->scf_error, index, E_SC0007_INFORMATION);
		status = E_DB_ERROR;
		break;
	    }
	    /* fill out with blanks */
	    for( junk = 0; where[junk] && junk < sizeof(char_maxbuf); junk++ )
		continue;
	    while( junk < sizeof(char_maxbuf) )
		where[ junk++ ] = ' ';
	    break;

	case SCI_IMA_SESSION:
	    where = (PTR) char_maxbuf;
	    (void) MOptrout( 0, (PTR)scb->cs_scb.cs_self,
			      sizeof(char_maxbuf), char_maxbuf );
	    /* fill out with blanks */
	    for( junk = 0; where[junk] && junk < sizeof(char_maxbuf); junk++ )
		continue;
	    while( junk < sizeof(char_maxbuf) )
		where[ junk++ ] = ' ';
	    break;

	case SCI_CSRUPDATE:
	    /*
	    ** NOTE: all result strings are blank-padded to a length of 8,
	    ** as that is what the length is declared as in the info_table.
	    ** If we don't blank-pad them, the last few characters could show
	    ** up as garbage to the user  (this happened on VMS, for example)
	    */
	    if (Sc_main_cb->sc_csrflags & SC_CSRDIRECT)
		where = ERx("DIRECT  ");
	    else if (Sc_main_cb->sc_csrflags & SC_CSRDEFER)
		where = ERx("DEFERRED");
	    else
		where = ERx("ERROR   ");
	    break;

	case SCI_CSRDEFMODE:
	    /*
	    ** NOTE: all result strings are blank-padded to a length of 8,
	    ** as that is what the length is declared as in the info_table.
	    ** If we don't blank-pad them, the last few characters could show
	    ** up as garbage to the user  (this happened on VMS, for example)
	    */
	    if (Sc_main_cb->sc_defcsrflag & SC_CSRRDONLY)
			where = ERx("READONLY");
	    else if(Sc_main_cb->sc_defcsrflag & SC_CSRUPD)
			where = ERx("UPDATE  ");
	    else
			where = ERx("ERROR   ");
	    break;

	case SCI_CSRLIMIT:
	    where = (PTR) &Sc_main_cb->sc_acc;
	    break;

	case SCI_FLATTEN:
	    /* Note reverse logic */
	    where = (Sc_main_cb->sc_flatflags & SC_FLATNONE) ?
		    (PTR)"N" : (PTR)"Y";
	    break;

	case SCI_FLATAGG:
	    /* Note reverse logic */
	    where = (Sc_main_cb->sc_flatflags & SC_FLATNAGG) ?
		    (PTR)"N" : (PTR)"Y";
	    break;

	case SCI_FLATSING:
	    /* Note reverse logic */
	    where = (Sc_main_cb->sc_flatflags & SC_FLATNSING) ?
		    (PTR)"N" : (PTR)"Y";
	    break;

	case SCI_FLATCHKCARD:
	    /* Note reverse logic */
	    where = (Sc_main_cb->sc_flatflags & SC_FLATNCARD) ?
		    (PTR)"N" : (PTR)"Y";
	    break;

	case SCI_NAME_CASE:
	    if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_NAME_UPPER)
	    	where = (PTR)"UPPER";
	    else
		where = (PTR)"LOWER";
	    break;

	case SCI_UNICODE_NORM:
	    if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_UTYPES_ALLOWED)
	    {
	      if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_UNICODE_NFC)
	    	  where = (PTR)"NFC";
	        else
		  where = (PTR)"NFD";
	    }
	    else
		  where = (PTR)"   ";
	    break;

	case SCI_DTTYPE_ALIAS:
	    if (Sc_main_cb->sc_date_type_alias & SC_DATE_INGRESDATE)
	    	  where = (PTR)"INGRESDATE";
	    else if (Sc_main_cb->sc_date_type_alias & SC_DATE_ANSIDATE)
		  where = (PTR)"ANSIDATE  ";
	    else
		  where = (PTR)"          ";
	    break;
	    
	case SCI_DATE_FORMAT:
	    {
	    ADF_CB *adfcb = scb->scb_sscb.sscb_adscb;
	    where = (PTR) char_maxbuf;
	    STcopy(adu_date_string(adfcb->adf_dfmt), where);
	    /* fill out with blanks */
	    for( junk = 0; where[junk] && junk < sizeof(char_maxbuf); junk++ )
		continue;
	    while( junk < sizeof(char_maxbuf) )
		where[ junk++ ] = ' ';
	    break;
	    }

	case SCI_MONEY_FORMAT:
	    {
	    ADF_CB *adfcb = scb->scb_sscb.sscb_adscb;
	    i4 lort = adfcb->adf_mfmt.db_mny_lort;
	    where = (PTR) char_maxbuf;
	    if (lort == DB_NONE_MONY)
	    {
		STcopy(ERx("none"),where);
	    } else
	    {
		if (lort == DB_TRAIL_MONY)
		    where[0]='t';
		else
		    where[0]='l';
		where[1]=':';
		STcopy(adfcb->adf_mfmt.db_mny_sym,where+2);
	    }
	    /* fill out with blanks */
	    for( junk = 0; where[junk] && junk < sizeof(char_maxbuf); junk++ )
		continue;
	    while( junk < sizeof(char_maxbuf) )
		where[ junk++ ] = ' ';
	    break;
	    }

	case SCI_MONEY_PREC:
	    {
	    ADF_CB *adfcb = scb->scb_sscb.sscb_adscb;
	    where = (PTR) char_maxbuf;
	    STprintf(where, "%d", adfcb->adf_mfmt.db_mny_prec);
	    /* fill out with blanks */
	    for( junk = 0; where[junk] && junk < sizeof(char_maxbuf); junk++ )
		continue;
	    while( junk < sizeof(char_maxbuf) )
		where[ junk++ ] = ' ';
	    break;
	    }

	case SCI_DECIMAL_FORMAT:
	    {
	    ADF_CB *adfcb = scb->scb_sscb.sscb_adscb;
	    where = (PTR) char_maxbuf;
	    where[0] = adfcb->adf_decimal.db_decimal;
	    where[1] = '\0';
	    /* fill out with blanks */
	    for( junk = 0; where[junk] && junk < sizeof(char_maxbuf); junk++ )
		continue;
	    while( junk < sizeof(char_maxbuf) )
		where[ junk++ ] = ' ';
	    break;
	    }

	case SCI_DELIM_CASE:
	    if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_DELIM_UPPER)
	    	where = (PTR)"UPPER";
	    else if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_DELIM_MIXED)
	    	where = (PTR)"MIXED";
	    else
		where = (PTR)"LOWER";
	    break;

	case SCI_REAL_USER_CASE:
	    if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_REAL_USER_MIXED)
		where = (PTR)"MIXED";
	    else if (scb->scb_sscb.sscb_ics.ics_dbserv & DU_NAME_UPPER)
	    	where = (PTR)"UPPER";
	    else
		where = (PTR)"LOWER";
	    break;

	case SCI_DBSERVICE:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_dbserv;
	    break;

	case SCI_MXRECLEN:
	    where = (PTR) &Sc_main_cb->sc_maxtup;
	    break;
	case SCI_MXPAGESZ:
	    where = (PTR) &Sc_main_cb->sc_maxpage;
	    break;
	case SCI_PAGE_2K:
	    where = (Sc_main_cb->sc_pagesize[0]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_RECLEN_2K:
	    where = (PTR) &Sc_main_cb->sc_tupsize[0];
	    break;
	case SCI_PAGE_4K:
	    where = (Sc_main_cb->sc_pagesize[1]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_RECLEN_4K:
	    where = (PTR) &Sc_main_cb->sc_tupsize[1];
	    break;
	case SCI_PAGE_8K:
	    where = (Sc_main_cb->sc_pagesize[2]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_RECLEN_8K:
	    where = (PTR) &Sc_main_cb->sc_tupsize[2];
	    break;
	case SCI_PAGE_16K:
	    where = (Sc_main_cb->sc_pagesize[3]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_RECLEN_16K:
	    where = (PTR) &Sc_main_cb->sc_tupsize[3];
	    break;
	case SCI_PAGE_32K:
	    where = (Sc_main_cb->sc_pagesize[4]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_RECLEN_32K:
	    where = (PTR) &Sc_main_cb->sc_tupsize[4];
	    break;
	case SCI_PAGE_64K:
	    where = (Sc_main_cb->sc_pagesize[5]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_RECLEN_64K:
	    where = (PTR) &Sc_main_cb->sc_tupsize[5];
	    break;
	case SCI_SPECIAL_UFLAGS:
	    where = (PTR)&ics_uflags;
	    ics_uflags.scs_usr_eingres =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_EINGRES) ? 1 : 0;
	    ics_uflags.scs_usr_rdba =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RDBA) ? 1 : 0;
	    ics_uflags.scs_usr_rpasswd =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_RPASSWD) ? 1 : 0;
	    ics_uflags.scs_usr_nosuchusr =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_NOSUCHUSER) ?
		    1 : 0;
	    ics_uflags.scs_usr_prompt =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_PROMPT) ? 1 : 0;
	    ics_uflags.scs_usr_rextpasswd =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_REXTPASSWD) ?
		    1 : 0;
	    ics_uflags.scs_usr_dbpr_nolimit =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_DBPR_NOLIMIT) ?
		    1 : 0;
	    ics_uflags.scs_usr_svr_control =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_SVR_CONTROL) ?
		    1 : 0;
	    ics_uflags.scs_usr_net_admin =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_NET_ADMIN) ? 1 : 0;
	    ics_uflags.scs_usr_monitor =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_MONITOR) ? 1 : 0;
	    ics_uflags.scs_usr_trusted =
		(scb->scb_sscb.sscb_ics.ics_uflags & SCS_USR_TRUSTED) ? 1 : 0;
	    break;
	case SCI_DISTRANID:
	    which = (PTR)&scb->scb_sscb.sscb_dis_tran_id;
	    where = (PTR)&which;
	    break;
	case SCI_PAGETYPE_V1:
	    where = (Sc_main_cb->sc_pagetype[1]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_PAGETYPE_V2:
	    where = (Sc_main_cb->sc_pagetype[2]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_PAGETYPE_V3:
	    where = (Sc_main_cb->sc_pagetype[3]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_PAGETYPE_V4:
	    where = (Sc_main_cb->sc_pagetype[4]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_PAGETYPE_V5:
	    where = (Sc_main_cb->sc_pagetype[5]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_PAGETYPE_V6:
	    where = (Sc_main_cb->sc_pagetype[6]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_PAGETYPE_V7:
	    where = (Sc_main_cb->sc_pagetype[7]) ? (PTR)"Y" : (PTR)"N";
	    break;
	case SCI_UNICODE:
	    where = (PTR)&unicode_level;
	    if (scb->scb_sscb.sscb_ics.ics_ucoldesc)
		unicode_level = 1;
	    else
		unicode_level = 0;
	    break;
	case SCI_LP64:
#if defined(LP64)
	    where =  (PTR)"Y";
#else 
	    where =  (PTR)"N";
#endif
	    break;

	case SCI_QBUF:
	    if (scb && scb->scb_sscb.sscb_stype != SCS_SMONITOR
		    && Sc_main_cb->sc_state == SC_OPERATIONAL
		    && (Sc_main_cb->sc_capabilities & 
				(SC_INGRES_SERVER | SC_STAR_SERVER))
		    && scb->scb_sscb.sscb_ics.ics_l_qbuf)
		qbuf = (PTR) scb->scb_sscb.sscb_ics.ics_qbuf;
	    else
		qbuf = NULL;
	    where = (PTR)&qbuf;
	    break;

	case SCI_QLEN:
	    if (scb && scb->scb_sscb.sscb_stype != SCS_SMONITOR
		    && Sc_main_cb->sc_state == SC_OPERATIONAL
		    && (Sc_main_cb->sc_capabilities & 
				(SC_INGRES_SERVER | SC_STAR_SERVER))
		    && scb->scb_sscb.sscb_ics.ics_l_qbuf)
		qlen = scb->scb_sscb.sscb_ics.ics_l_qbuf;
	    else
		qlen = 0;
	    where = (PTR) &qlen;
	    break;

	case SCI_TRACE_ERRNO:
	    where = (PTR)&Sc_main_cb->sc_trace_errno;
	    break;

	case SCI_PREV_QBUF:
	    if (scb && scb->scb_sscb.sscb_stype != SCS_SMONITOR
		    && Sc_main_cb->sc_state == SC_OPERATIONAL
		    && (Sc_main_cb->sc_capabilities & 
				(SC_INGRES_SERVER | SC_STAR_SERVER))
		    && scb->scb_sscb.sscb_ics.ics_l_lqbuf)
		prev_qbuf = (PTR) scb->scb_sscb.sscb_ics.ics_lqbuf;
	    else
		prev_qbuf = NULL;
	    where = (PTR)&prev_qbuf;

	    break;

	case SCI_PREV_QLEN:
	    if (scb && scb->scb_sscb.sscb_stype != SCS_SMONITOR
		    && Sc_main_cb->sc_state == SC_OPERATIONAL
		    && (Sc_main_cb->sc_capabilities & 
				(SC_INGRES_SERVER | SC_STAR_SERVER))
		    && scb->scb_sscb.sscb_ics.ics_l_lqbuf)
		prevqlen = scb->scb_sscb.sscb_ics.ics_l_lqbuf;
	    else
		prevqlen = 0;
	    where = (PTR) &prevqlen;
	    break;

	case SCI_PSQ_QBUF:
	    if (scb && scb->scb_sscb.sscb_stype != SCS_SMONITOR
		    && Sc_main_cb->sc_state == SC_OPERATIONAL
		    && (Sc_main_cb->sc_capabilities & 
				(SC_INGRES_SERVER | SC_STAR_SERVER))
		    && scb->scb_sscb.sscb_troot 
                    && (Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER) == 0
               )
		psqbuf = (PTR)((PSQ_QDESC *)scb->scb_sscb.sscb_troot)->psq_qrytext;
	    else
		psqbuf = NULL;
	    where = (PTR)&psqbuf;
	    break;

	case SCI_PSQ_QLEN:
	    if (scb && scb->scb_sscb.sscb_stype != SCS_SMONITOR
		    && Sc_main_cb->sc_state == SC_OPERATIONAL
		    && (Sc_main_cb->sc_capabilities & 
				(SC_INGRES_SERVER | SC_STAR_SERVER))
		    && scb->scb_sscb.sscb_troot 
                    && (Sc_main_cb->sc_capabilities & SC_RECOVERY_SERVER) == 0
                )
		psqlen = ((PSQ_QDESC *) scb->scb_sscb.sscb_troot)->psq_qrysize;
	    else
		psqlen = 0;
	    where = (PTR) &psqlen;
	    break;

	default:
	    SETDBERR(&cb->scf_error, index, E_SC0007_INFORMATION);
	    status = E_DB_ERROR;
	    break;
	}

	if ((tm_needed) && (!tm_done))
	{
	    CSstatistics(&stat_block, tm_parm);
	    tm_done = 1;
	}

	if (status == E_DB_OK)
	{
	    len = (Sci_info_table[item->sci_code].it_size > item->sci_length ?
		    item->sci_length : Sci_info_table[item->sci_code].it_size);
	    if (where != NULL)
		MEcopy(where, len, item->sci_aresult);
	    if (item->sci_rlength)
		*item->sci_rlength = len;
	    if ((len != Sci_info_table[item->sci_code].it_size) &&
		(where != NULL))
	    {
		SETDBERR(&cb->scf_error, index, E_SC0008_INFO_TRUNCATED);
		status = E_DB_WARN;
	    }
	}
	else
	{
	    break;
	}
    }

    return(status);
}

/*
** Name: scu_idefine	- define session information for a user
**
** Description:
**      This function is issued by the scf entry point in response to a request
**      for information (SCU_IDEFINE) operation by a user of scf. 
**	THIS ENTRY IS PROVIDED SOLEY FOR MODULE TEST PURPOSES.
**	IT WILL NOT EXIST IN A SYSTEM BUILD OF ALL MODULES.
**      Items of information, as described below, are provided by filling
**      in the information request block (scf_sci) portion of the scf control 
**      block.  Multiple requests will be serviced at one time -- this structure 
**      is an array.
** 
**      If there is an error encountered while processing the requests, 
**      the item on which the error was encountered will have its index entered 
**      into the data field of the DB_ERROR structure.  The order of evaluation 
**      of the requests is not guaranteed, so should such an error be encountered, 
**      there is no method through which the user can determine if other 
**      requested items have been filled in.
**
** Inputs:
**      SCU_IDEFINE			op code to scf_call()
**      scf_cb                          requesting control block...
**	.scf_len_union.scf_ilength	number of information items provided
**	.scf_ptr_union.scf_sci		array indicating provided items
**	sci_list[]			the aforementioned array, which has ...
**	    .sci_code                   what item is provided
**	    .sci_length                 length of buffer provided with information
**	    .sci_aresult                address of informatino
**	    .sci_rlength		not used
**					
**      (This structure is repeated scf_ilength times)
**
** Outputs:
**      .scf_error                          filled in with error if encountered
**	    .err_code                       one of ...
**		   E_SC_OK
**			wonderful job by requestor.  Or ...
**                 E_SC0007_INFORMATION
**			invalid item - which one in data
**                 E_SC0008_INFO_TRUNCATED
**			information provided too long - which one in err_data
**		    E_SC00009_BUFFER
**			buffer provided for information is not addressable -
**			 item in data
**		    E_SC000A_NO_BUFFER
**			no information provided
**		    E_SC000B_NO_REQUEST
**			ptr to/portion of SCI blk not addressable - item in data
**	    .data                       index of offending item, as described
**					above.  intended as a programmer aid
**	Returns:
**	    one of the regular E_DB_{OK, WARNING, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	8-Apr-86 (fred)
**          Created on Jupiter
**	03-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added support for the following:
**	    SCI_GROUP, SCI_APLID, SCI_DBPRIV, SCI_QROW_LIMIT, SCI_QDIO_LIMIT,
**	    SCI_QCPU_LIMIT, SCI_QPAGR_LIMIT, SCI_QCOST_LIMIT.
**	24-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Add support for the following in scu_idefine: SCI_IGNORE.
**	21-may-91 (andre)
**	    renamed some fields in SCS_ICS to make it easier to determine by the
**	    name whether the field contains info pertaining to the system (also
**	    known as "Real"), Session ("he who invoked us"), or the Effective
**	    identifier:
**		ics_ustat     -->  ics_rustat	    real user status bits
**		ics_username  -->  ics_eusername    effective user id
**		ics_ucode     -->  ics_sucode	    copy of the session user id
**		ics_grpid     -->  ics_egrpid	    effective group id
**		ics_aplid     -->  ics_eaplid       effective role id
**	03-dec-92 (andre)
**	    ics_eusername will become a pointer that will point at ics_susername
**	    unless we are inside a module with <module auth id> in which case
**	    the <module auth id> will be stored in ics_musername and
**	    ics_eusername will be made to point at ics_musername until we leave
**	    the module.
**	    
**	    the following fields in SCS_ICS have been renamed:
**		ics_sucode	--> ics_iucode
**	29-Jun-1993 (daveb)
**	    cast arg to CSget_scb correctly.
**	2-Jul-1993 (daveb)
**	    prototyped.
*/
DB_STATUS
scu_idefine(SCF_CB  *cb, SCD_SCB *scb )
{
    i4		error;
    DB_STATUS		status;
    i4		index;	    /* which request failed */
    i4			count;
    PTR			where;
    SCF_SCI		*item;
    i4			len;	    /* length of info transmitted */


    status = E_DB_OK;
    CLRDBERR(&cb->scf_error);

    if ((cb->scf_len_union.scf_ilength <= 0) || (!cb->scf_ptr_union.scf_sci))
    {
	SETDBERR(&cb->scf_error, 0, E_SC000B_NO_REQUEST);
	return(E_DB_ERROR);
    }

    for (index = 0; index < cb->scf_len_union.scf_ilength; index++)
    {
	item = &cb->scf_ptr_union.scf_sci->sci_list[index];
	if (item->sci_length <= 0)
	{
	    SETDBERR(&cb->scf_error, index, E_SC0027_BUFFER_LENGTH);
	    status = E_DB_ERROR;
	    break;
	}
	if (!item->sci_aresult)
	{
	    SETDBERR(&cb->scf_error, index, E_SC0009_BUFFER);
	    status = E_DB_ERROR;
	    break;
	}

	switch (item->sci_code)
	{
	case SCI_USERNAME:
	    where = (PTR) scb->scb_sscb.sscb_ics.ics_eusername;
	    break;

	case SCI_UCODE:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_iucode;  /* is an array */
	    break;
	    
	case SCI_DBNAME:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_dbname;
	    break;

	case SCI_PID:
	    where = (PTR) &Sc_main_cb->sc_pid;
	    break;

	case SCI_SID:
	    where = (PTR) &scb->cs_scb.cs_self;
	    break;

	case SCI_USTAT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_rustat;
	    break;

	case SCI_MAXUSTAT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_maxustat;
	    break;

	case SCI_DSTAT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_dbstat;
	    break;

	case SCI_ERRORS:
	    where = (PTR) &scb->scb_sscb.sscb_noerrors;
	    break;

	case SCI_IVERSION:
	    where = (PTR) Sc_main_cb->sc_iversion;
	    break;

	case SCI_COMMTYPE:
/*	    where = (PTR) &scb->scb_cscb.cscb_method;*/
	    break;

	case SCI_SCFVERSION:
	    where = (PTR) &Sc_main_cb->sc_scf_version;
	    break;

	case SCI_NOUSERS:
	    where = (PTR) &Sc_main_cb->sc_nousers;
	    break;

	case SCI_SNAME:
	    where = Sc_main_cb->sc_sname;    /* is an array -- no & */
	    break;

	case SCI_MEMORY:
	    /* always returns OK */
	    status = scu_mcount(scb, &count);
	    where = (PTR) &count;
	    break;

	case SCI_CPUTIME:
	    where = (PTR) &Sc_main_cb->sc_timer.timer_value.stat_cpu;
	    break;

	case SCI_PFAULTS:
	    where = (PTR) &Sc_main_cb->sc_timer.timer_value.stat_pgflts;
	    break;

	case SCI_DIOCNT:
	    where = (PTR) &Sc_main_cb->sc_timer.timer_value.stat_dio;
	    break;

	case SCI_BIOCNT:
	    where = (PTR) &Sc_main_cb->sc_timer.timer_value.stat_bio;
	    break;

	case SCI_SCB:
	    switch (cb->scf_facility)
	    {
		case DB_ADF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_adscb;
		    break;
		case DB_DMF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_dmscb;
		    break;
		case DB_OPF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_opscb;
		    break;
		case DB_PSF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_psscb;
		    break;
		case DB_QEF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_qescb;
		    break;
		case DB_QSF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_qsscb;
		    break;
		case DB_RDF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_rdscb;
		    break;
		case DB_SCF_ID:
		    where = (PTR) &scb->scb_sscb.sscb_scscb;
		    break;
		default:
		    SETDBERR(&cb->scf_error, index, E_SC0018_BAD_FACILITY);
		    status = E_DB_ERROR;
		    break;
	    }
	    break;

	case SCI_DBID:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_opendb_id;
	    break;

	case SCI_UTTY:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_terminal;
	    break;
	    
	case SCI_LANGUAGE:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_language;
	    break;
	    
	case SCI_DBA:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_dbowner;
	    break;
	    
	case SCI_GROUP:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_egrpid;
	    break;
	    
	case SCI_APLID:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_eaplid;
	    break;
	    
	case SCI_DBPRIV:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_fl_dbpriv;
	    break;
	    
	case SCI_QROW_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_qrow_limit;
	    break;
	    
	case SCI_QDIO_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_qdio_limit;
	    break;
	    
	case SCI_QCPU_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_qcpu_limit;
	    break;
	    
	case SCI_QPAGE_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_qpage_limit;
	    break;
	    
	case SCI_QCOST_LIMIT:
	    where = (PTR) &scb->scb_sscb.sscb_ics.ics_qcost_limit;
	    break;

	case SCI_IGNORE:
	    where = NULL;
	    break;

	default:
	    SETDBERR(&cb->scf_error, index, E_SC0007_INFORMATION);
	    status = E_DB_ERROR;
	    break;
	}

	if (status == E_SC_OK)
	{
	    len = (Sci_info_table[item->sci_code].it_size > item->sci_length ?
		    item->sci_length : Sci_info_table[item->sci_code].it_size);
	    if (where != NULL)
              {
		MEcopy(item->sci_aresult, len, where);
              }

	    if ((item->sci_length > Sci_info_table[item->sci_code].it_size) &&
		(where != NULL))
	    {
		SETDBERR(&cb->scf_error, index, E_SC0008_INFO_TRUNCATED);
		status = E_DB_WARN;
	    }
	}
	else
	{
	    break;
	}
    }

    return(status);
}
