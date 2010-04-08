/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: SCD.H - Data structures and values to be used in the SCD module
**
** Description:
**      This file contains the major data structures and control
**      definitions for the SCD module of the system control facility.
**      Included in this category are the major control blocks
**      for controlling individual sessions.  They are contained
**      in here because all of this information is managed by the
**      dispatcher so that the user facilities (PSF, OPF, QEF) do
**      not have to.
**
** History: $Log-for RCS$
**      14-Jan-86 (fred)
**          Created.
**	9-Aug-91, 18-Feb-92 (daveb) 
**	    Add definition of SCD_SCB_TYPE to fix bug 38056
**	2-Jul-1993 (daveb)
**	    move all func externs here, prototyped.
**	20-jun-1995 (dougb)
**	    Cross integrate my change 274733 from ingres63p
**	    and a change which will be submitted after a full 6.4 build:
**	  31-may-95 (dougb)
**	    For AXP/VMS mostly (well, whenever the DECC compiler is used),
**	    tell the compiler that the SCD_SCB structure is correctly aligned.
**	    Avoids warnings when portions of that structure are volatile.
**	  20-jun-95 (dougb)
**	    Remove explicit padding from SCD_SCB structure.
**	25-sep-95 (dougb)
**	    Remove now-useless member_alignment pragmas.
**	21-dec-95 (dougb)
**	    Undo previous change.
**	13-Mch-1996 (prida01)
**	    Add call to scd_diag for scdmain.c
**	30-jan-97 (stephenb)
**	    Add proto for scd_init_sngluser.
**	12-Mar-1998 (jenjo02)
**	    Got rid of SCD_DSCB, which was really of no use.
**	    Renamed SCD_MCB to SCS_MCB and moved to scs.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Added (SCD_SCB*)scb parameter to prototype for
**	    scd_init_sngluser().
**/

/*}
** Name: SCD_SCB - Session Control Block
**
** Description:
**      This control block is the root of all information in the
**      server for this session.  All other facilities' control blocks
**      are rooted here as well.  This control block contains multiple
**      queue entries, as this block is queued on multiple lists.
**      E.g. the major list of sessions and the ready/waiting queue.
**
**      Also contained here as all the adminstrative information about
**      the session.  Such information includes the user name, database name,
**      frontend process id, whether the connection is a network,
**      whether the user is a distributed backend, frontend terminal
**      id, etc.
**
** History:
**     14-Jan-86 (fred)
**          Created
**      25-Nov-1986 (fred)
**          Added CS_SCB interface
**	9-Aug-91, 18-Feb-92 (daveb) 
**	    Add definition of SCD_SCB_TYPE to fix bug 38056
**	20-jun-1995 (dougb)
**	    Cross integrate my change 274733 from ingres63p
**	    and a change which will be submitted after a full 6.4 build:
**	  31-may-95 (dougb)
**	    For AXP/VMS mostly (well, whenever the DECC compiler is used),
**	    tell the compiler that this structure is correctly aligned.
**	    Avoids warnings when portions (eg. the cs_scb) are volatile.
**	    Lose nothing if volatile isn't used since SCD_SDB allocations
**	    guarantee a great deal of alignment (32 bytes on a VMS box).
**	    Pad required to end structure on a longword boundary.
**	  20-jun-95 (dougb)
**	    Remove explicit padding from SCD_SCB structure.  Seems that
**	    member_alignment doesn't do anything other than pad the end
**	    of this structre (no change in alignment of fields in embedded
**	    structures).
**	25-sep-95 (dougb)
**	    Remove now-useless member_alignment pragmas.
**	21-dec-95 (dougb)
**	    Undo previous change.
**	12-Mar-1998 (jenjo02)
**	    Got rid of SCD_DSCB, which was really of no use.
**	    Renamed SCD_MCB to SCS_MCB and moved to scs.h.
*/
# ifdef __DECC
#  pragma member_alignment save
#  pragma member_alignment
# endif
struct _SCD_SCB
{
    CS_SCB	    cs_scb;		/* CS's part */

    /* value to put into cs_scb.cs_client_type for us to see later  */
#   define SCD_SCB_TYPE 	CV_C_CONST_MACRO('S', 'C', 'D', ' ')

    /* This portion of the block is used by SCF */
    SCS_SSCB	    scb_sscb;		/* information for SCF */
    SCC_CSCB	    scb_cscb;		/* information used for communication */
};	/* typedef appears in forward section as well */
# ifdef __DECC
#  pragma member_alignment restore
# endif


/* FUNC_EXTERNS */

FUNC_EXTERN VOID scd_note( DB_STATUS status, i4   culprit );

FUNC_EXTERN DB_STATUS scd_initiate( CS_INFO_CB  *csib );
FUNC_EXTERN DB_STATUS scd_terminate(void);
FUNC_EXTERN DB_STATUS scd_diag(void *diag_link);

FUNC_EXTERN DB_STATUS scd_dbinfo_fcn(ADF_DBMSINFO *dbi,
				     DB_DATA_VALUE *dvi,
				     DB_DATA_VALUE *dvr,
				     DB_ERROR *error );

FUNC_EXTERN DB_STATUS scd_adf_printf( char *cbuf );

FUNC_EXTERN DB_STATUS scd_alloc_scb(SCD_SCB  **scb_ptr,
				    GCA_LS_PARMS  *crb,
				    i4  thread_type );

FUNC_EXTERN DB_STATUS scd_dealloc_scb( SCD_SCB *scb );

FUNC_EXTERN STATUS scd_reject_assoc( GCA_LS_PARMS  *crb, STATUS error );

FUNC_EXTERN VOID scd_disconnect( SCD_SCB *scb );

FUNC_EXTERN STATUS scd_get_assoc( GCA_LS_PARMS *crb, i4  sync );

FUNC_EXTERN DB_STATUS scd_dbadd( SCV_DBCB *dbcb, 
				DB_ERROR  *error,
				DMC_CB *dmc,	
				SCF_CB *scf_cb );

FUNC_EXTERN DB_STATUS scd_dbdelete( SCV_DBCB *dbcb,
				   DB_ERROR *error,
				   DMC_CB *dmc,	
				   SCF_CB *scf_cb );

FUNC_EXTERN DB_STATUS scd_dblist(void);

FUNC_EXTERN DB_STATUS scd_init_sngluser(SCF_CB *scf_cb,
					SCD_SCB *scb );
