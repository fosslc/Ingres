/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: OPSCB.H - Session Control block definition
**
** Description:
**      This file defines the session control block which will be referenced
**      in the SCF by the session management routines.
**
** History: 
**      20-feb-86 (seputis)
**          initial creation
**	8-sep-92 (ed)
**	    use correct RDF structure
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added ops_dbxlate to OPS_CB, which points to case translation mask.
**	    Added ops_cat_own to OPS_CB, which points to catalog owner name.
**      16-sep-93 (smc)
**          Made ops_sid a CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-aug-2001 (hayke02)
**	    Added OPS_SUBSELOJ to indicate that we have transformed a
**	    'not exists'/'not in' subselect into an outer join.
**	10-Jan-2001 (jenjo02)
**	    Added GET_OPS_CB macro to replace calls to
**	    SCF(SCU_INFORMATION) to find session's OPS_CB*.
**/


/*}
** Name: OPT_SESSION - session level trace vector for the optimizer
**
** Description:
**	This structure defines a trace vector for the optimizer.  It will
**      be used for session level trace flags.
**    	These flags can be selected by the SET commands.  
**    	The default state of each flag is FALSE.
**
**
** History:
**      29-jun-86 (seputis)
**          initial creation
**	23-oct-02 (inkdo01)
**	    Make room for more valued tp's.
[@history_line@]...
*/
#define                 OPT_BMSESSION       128
/* maximum number of bit entries in trace/timing array found in OPT_TT        */
#define                 OPT_SVALUES       2
/* number of values that can be associated with a trace vector at one time */
#define                 OPT_SWVALUES       15
/* number of trace flags that can have values associated with them */

typedef ULT_VECTOR_MACRO(OPT_BMSESSION,OPT_SVALUES) OPT_SESSION;


/*}
** Name: OPS_CB - Session Control Block for the optimizer
**
** Description:
**      This control block contains only the information that persists
**      between optimizations.  The size of the session control block 
**      will be as small as possible since one will exist per user in 
**      the system.
**
**      The caller of the OPF is responsible for allocating space for
**      this control block, and passing a pointer to the OPS_CB
**      in the control block associated with the entry point (e.g. OPF_CB).
**
** History:
**     20-feb-86 (seputis)
**          initial creation
**     23-oct-88 (seputis)
**          added coordinator database info
**     28-jan-91 (seputis)
**	    add support for OPF ACTIVE flag
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added ops_dbxlate to OPS_CB, which points to case translation mask.
**	    Added ops_cat_own to OPS_CB, which points to catalog owner name.
**	01-sep-93 (jhahn)
**	    Added ops_psession_id and ops_session_ctr for maintaining server
**	    lifetime unique id's.
**	13-may-94 (ed)
**	    b59537 - added variable to help in skipping non-critical consistency
**	    checks.
**	18-june-99 (inkdo01)
**	    Add ops_default_user for temp table model histogram feature.
**	3-feb-06 (dougi)
**	    Add ops_override to implement query-level hints.
**	6-Jul-2006 (kschendel)
**	    Correct the type of udbid, should be i4.
**	19-Aug-2009 (kibro01) b122509
**	    Add sc930_trace file pointer for qeps going to sc930 trace file.
[@history_line@]...
*/
typedef struct _OPS_CB
{
    struct _OPS_STATE *ops_state;       /* - ptr to state block located in the
                                        ** stack area.
                                        ** - if NULL then the optimization has
                                        ** not begun to allocate resources
                                        ** or has finished deallocating
                                        ** resources 
                                        */
    struct _OPF_CB  *ops_callercb;      /* save ptr to caller's control block
                                        ** - used in error recovery since SCF
                                        ** can provide session control block
                                        ** upon request and then error code 
                                        ** can be placed in the caller's
                                        ** control block
                                        */
    DB_STATUS       ops_retstatus;      /* -contains the return status
                                        ** for the optimization.  It is used
                                        ** during an exit via the exception
                                        ** handling mechanism to store the
                                        ** return status, prior to a "long
                                        ** jump" out of nested routines.
                                        */
    ADF_CB          *ops_adfcb;         /* ptr to adf control block obtained
                                        ** from SCF - needed for ADF calls
                                        */
    i4		    ops_udbid;          /* Globally unique database id (same
                                        ** as infodb)
                                        */
    PTR             ops_dbid;           /* dmf data base id for this session 
                                        ** only
                                        */
    CS_SID          ops_sid;            /* session id for this session 
                                        */
    bool            ops_interrupt;      /* TRUE if an interrupt occurred
                                        ** - this flag will be monitored
                                        ** periodically and the optimization
                                        ** will be aborted if set
                                        */
    bool            ops_check;          /* TRUE if at least one trace/timing
                                        ** flag is set true.  The default
                                        ** state of all trace/timing flags
                                        ** should be false.  This flag will be
                                        ** used before a more expensive lookup
                                        ** of the opt_tt structure.
                                        */
    bool            ops_skipcheck;	/* TRUE if non-critical trace/timing
                                        ** flags are to be checked.
                                        */
    OPS_ALTER       ops_alter;          /* charactertics of session that
                                        ** can be altered via the
                                        ** SET [SESSION] <alter_op> command
                                        */
    i4		    ops_override;	/* copy of PST_QTREE.pst_hintmask 
					** to override tracepoints, ops_alter
					** and other server settings */
    OPT_SESSION	    ops_tt;             /* trace and timing bit vector for 
                                        ** session
                                        */
    OPG_CB          *ops_server;	/* ptr to server control block */
    i4		    ops_smask;          /* state mask for this session */
#define                 OPS_MDISTRIBUTED 1
/* TRUE - is this is a distributed database session,
** FALSE - if local optimization is being done */
#define                 OPS_MCONDITION  2
/* TRUE - if this thread has entered OPF and passed the CScondition check */
#define			OPS_SUBSELOJ	4
/* TRUE if one or more unioned queries has been transformed from a 'not in'
** or 'not exists to an outer join */
    DD_1LDB_INFO     *ops_coordinator;   /* pointer to LDB descriptor for
					** coordinator database */
    u_i4	    *ops_dbxlate;	/* Case translation semantics flags
					** See <cuid.h> for masks.
					*/
    DB_OWN_NAME	    *ops_cat_owner;	/* Catalog owner name.  Will be
					** "$ingres" if regular ids are lower;
					** "$INGRES" if regular ids are upper.
					*/
    DB_OWN_NAME	    ops_default_user;	/* Session default user (for use in 
					** temp table model histogram feature) */
    i4	    ops_psession_id;	/* Unique id per session over life of
					** server.
					*/
    i4	    ops_session_ctr;	/* For unique id's within a session */
    PTR		sc930_trace;
}   OPS_CB;


/* Macro to return pointer to session's OPS_CB directly from SCF's SCB */
#define	GET_OPS_CB(sid) \
	*(OPS_CB**)((char *)sid + Opg_cb->opg_opscb_offset)
