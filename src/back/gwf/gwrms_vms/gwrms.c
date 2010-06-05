/*
**    Copyright (c) 1989, 2008 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cs.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <descrip.h>
#include    <armdef.h>
#include    <acldef.h>
#include    <chpdef.h>
#include    <iledef.h>
#include    <ssdef.h>
#include    <starlet.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <scf.h>
#include    <lg.h>
#include    <dm.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmucb.h>
#include    <gwf.h>
#include    <gwfint.h>
#include    <rms.h>
#include    <add.h>
#include    "gwrms.h"
#include    "gwrmsdt.h"
#include    <pm.h>
#include    <cv.h>

/**
** Name: GWRMS.C	- Exit package for the RMS Gateway
**
** Description:
**	This file defines the RMS Gateway exits. These exits are called by GWF
**	at the appropriate times to perform RMS-specific operations.  Gateway
**	exits directly interface with a foreign	data manager.
**
**	This file contains VMS-specific, RMS-specific code and is NOT portable
**	to any other system. It makes direct calls to server facilities such as
**	SCF and ULF, and is designed to be linked into a server through the GWF
**	facility. Only the gw02_init function is called directly. The gw02_init
**	function then loads a vector of function pointers which allow GWF to
**	indirectly call all the other exits.
**
**      This file defines:
**    
**      gw02_init()	    - generate facility initialization
**	gw02_term()	    - terminate the facility
**	gw02_tabf()	    - format table registration catalog tuples
**	gw02_idxf	    - format gateway index catalog record.
**	gw02_open()	    - gateway table open for a session
**	gw02_close()	    - gateway table close for a session
**      hash_func ()        - hash function determining the hash table entry
**      rms_open_file()     - open rms file
**      rms_close_by_name() - close rms file using file name
**      rms_close_file()    - close rms file with file info block as argument
**      file_in_hash_table() - find if the file in mind is already open 
**      sweep_hash_table()  - free files in hash table that are not in use.
**      alloc_fab_info_block()- allocate file cache from available list
**      return_fab_info_blk()- put file cache to available list
**      rms_fab_cache_init()- initialize file cache
**      rms_fab_cache_term()- close open files and free cache
**      rms_key_asc()       - find a key/index to be ascending or descending
**      op_scf_mem()        - call to scf memory operations
**	gw02_position()	    - GW record stream positioning
**	gw02_get()	    - GW record stream get
**	gw02_put()	    - GW record put
**	gw02_replace()	    - GW record replace
**	gw02_delete()	    - GW record delete
**	gw02_begin	    - exit begin transaction.
**	gw02_commit	    - exit commit transaction.
**	gw02_abort	    - exit abort transaction.
**	gw02_info	    - exit information
**
**	gw02_xlate_key	    - translate keys from Ingres format to RMS format
**	gw02_send_error	    - send formatted error message to frontend.
**	gw02_max_rms_ksz    - calculate maximum rms key size
**	applyTimeout	    - set timeout on RMS record operations
**	gw02_connect	    - connect to rms file.
**	rePositionRAB	    - reset record stream after UPDATE or DELETE
**	positionRecordStream - set RMS to point at the right record
**
** History:
**      26-Apr-1989 (alexh)
**          Created for Gateway project.
**	03-oct-1989 (linda)
**	    Modified to use our own FAB and RAB structs, analogous to those
**	    supplied by the VMSRTL as "cc$rms_fab" and "cc$rms_rab".  This is
**	    done in order to avoid having to link in the vax runtime library,
**	    which we don't use for anything else...
**	24-dec-89 (paul)
**	    General modifications to several routines to make them work.
**	16-apr-90 (bryanp)
**	    Revamped the error handling totally. The RMS gateway now conforms
**	    to the standard GWF error-handling protocol. Error messages with
**	    complete parameters are logged to the standard error log, and
**	    informative error codes are returned to GWF upon failure.
**	19-apr-90 (bryanp)
**	    Added gw02_errsend() to send the lowest-level RMS error message
**	    directly to the user at the front end, on the theory that the
**	    user actually has the best chance of understanding the RMS error
**	    message, rather than the higher-level 'interpreted' Ingres msgs.
**	20-apr-90 (bryanp)
**	    Use the returned record length from sys$get (rab$w__rsz) to tell
**	    us how long a variable-length record actually was. Also taught
**	    rms_to_ingres how to calculate the Ingres record length after
**	    conversion of RMS types to Ingres types, in case that length is
**	    different.
**	23-apr-90 (bryanp)
**	    Asynchronous RMS. See notes below for detailed information.
**	10-may-90 (alan)
**	    Validate user file access by calling sys$check_access prior 
**	    to table open.
**	11-jun-1990 (bryanp)
**	    Handle data conversion errors better. The rmsmap code may report
**	    that an ADF error occurred and has already been reported to the
**	    user (E_GW5484_RMS_DATA_CVT_ERROR). In this case, do no additional
**	    error reporting or traceback; just return the data conversion error
**	    to the GWF layer for its use.
**	12-jun-1990 (bryanp)
**	    When a syntax error is found parsing the extended format strings,
**	    send the syntax error message to the front end and return with
**	    E_GW050E_ALREADY_REPORTED so that error handling is 'nice'.
**	    Added gw02_send_error() to send error message to frontend.
**	25-jun-1990 (linda, bryanp, alan) (comments added by linda)
**	    Extensive changes:  keep a running length total in tabf routine, so
**	    that default offset is current; get "page count" (== allocated
**	    blocks) from file if it exists at table registration; check user's
**	    access to file at file open; add NOACCESS and MODIFY type access
**	    codes to support non-existent or non-accessible tables at time of
**	    registration; prevent re-connecting by checking RAB to see if file
**	    has been connected; pad RMS buffer with blanks so that unregistered
**	    fields don't get garbage on put; check record lengths on output;
**	    support user errors under many "file-not-found" conditions.
**	5-jul-90 (linda)
**	    Change gw02_send_error() routine to take parameters so that error
**	    messages can be specific.  Changed the level at which it is called;
**	    in particular, moved some calls from gw02_tabf() down to
**	    gw02__fmt() and gw02__xfmt(), where we still have detailed info.
**      18-may-92 (schang)
**          GW merge.
**	    9-jan-91 (linda)
**	      Allocate memory for low & high rms keys only once, in gw02_open(),
**	      rather than in gw02_xlate_key() every time it is called from
**	      gw02_position().  The latter scheme was incorrect and could cause
**	      us to unnecessarily run out of memory.  Add new routine
**	      gw02_max_rms_ksz() to calculate maximum space needed for this.
**	    25-jan-91 (RickH)
**	      Apply timeout to RMS calls that support it.  These are:
**	      sys$find, sys$put, and sys$get.
**	    4-feb-91 (linda)
**	      Code cleanup; make gw02_send_error() non-static and so it takes
**	      parameters; bug fix for memory allocation of keys; allocate our
**	      tcb's from current ulm stream rather than at session level; fix
**	      primary key access (new routine is gw02_xlate_key(); it also uses
**	      adc_isminmax() function); add support for RMS record level locking
**	      between threads as well as against external programs; apply timeout
**	      value from gw_rsb to RMS calls.
**	    20-mar-91 (rickh)
**	      Position relative files at EOF for puts.  Otherwise, we are
**	      default positioned at BOF and get RMS$_REX on puts.
**	    6-aug-91 (rickh)
**	      Mark the record stream as positioned after GETs and FINDs.
**	      Since DELETEs and UPDATEs inscrutably de-position the RAB,
**	      make them publish this fact too.  DELETEs and UPDATEs operate
**	      only on an already positioned stream so make them reset the
**	      stream if it has lost its place.  Why must we do this?
**	      ...because when using cursors, you could perform multiple
**	      UPDATEs/DELETEs on the same record.
**	    4-nov-91 (rickh)
**	      Changed the format of Gwrms_version to conform to the new model
**	      decreed by Release Management.  Pass this string back at
**	      server initialization time.
**	    20-feb-92 (rickh)
**	      Naturally, this is the sort of thing that generates the most
**	      controversy.  Upped the Gwrms_version number.
**      19-jun-92 (schang)
**          do not check accessibility when trying to open a remote file
**          through DECnet (with vnode specified)
**	22-oct-92 (rickh)
**	    Replaced references to DMP_ATTS with DB_ATTS.
**      06-nov-92 (schang)
**          add dmrcb before dmtcb  and all openfile sharing code
**      13-nov-92 (schang)
**          prototyping.
**      10-dec-92 (schang)
**          add code to close RMS file by file name.  This is taken by
**          remove table.
**      19-jan-93 (schang)
**          doc: if record stream is established, it is properly set up for
**          the operation, no need to lock record explicitly with ~RMS$M_NLK
**          when reposition
**	26-apr-1993 (bryanp)
**	    Include <lg.h> to pick up logging system type definitions.
**	28-jul-1993 (bryanp)
**	    Include <lk.h> and <pc.h> to pick up locking system type
**		definitions. It really is evil that this code #includes DMF
**		private headers like dmp.h. Please fix this!
**      11-aug-93 (ed)
**          unnest dbms.h
**      07-jun-1993 (schang)
**          add read-only repeating group support to RMS GW
**      10-jun-93 (schang)
**          read characteristics of keys when open an rms file
**      23-aug-93 (schang)
**          remove dmp.h since DMP_ATTS has been changed to DB_ATTS by Rickh.
**      13-jun-96 (schang)
**          integrate 64 rms gw changes
**      12-aug-93 (schang)
**          add support for read-regardless-lock.
**      27-sep-93 (schang)
**          to make the server readonly at startup
**      20-oct-93 (schang)
**          redo mapping between RMS RFA and Ingres TID.  Major change for
**          indexed file, but not sequential.  Add rms_rfa_to_tid ()
**          and rms_tid_to_rfa().
**        25-oct-93 (schang)
**          new version "RMS 2.0/02..." fro special Intel/repeating_group
**          release      
**        26-oct-93 (schang)
**          if we hit minmax value, just use it to position; there is no
**          reason to make minmax value special.
**        22-dec-93 (schang)
**          changes for RFA buffering
**        07-jul-95 (schang)
**          set Gwrms_version[]="RMS 2.1/00 (vax.vms/00)"
**        23-apr-96 (schang)
**          allow flush at discoonect and (earlier change) turn on transaction
**          so that flush can be done at transaction end
**        03-mar-1999 (chash01)
**          integrate all changes since ingres 6.4
**        07-mar-00 (chash01)
**          add additional args to sys$check_access
**        mar-09-2000(chash01)
**          set new version text Gwrms_version[]="EA RMS 2.1/0001 (axp.vms/00)";
**	28-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from RMS GW code as the use is no longer allowed
**	28-feb-2001 (kinte01)
**	    ule_format is now ule_doformat 
**      27-feb-02 (chash01)
**          set version to 2.1/0106
**      19-nov-02 (chash01)
**          set alpha vms version to 2.1/0011 (axm.vms/00) 
**      13-feb-03 (chash01) x-integrate change#461908
**          up version to 2.6/0207
**	17-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**      02-sep-2003 (chash01)
**          All RMS gateway startup logicals are switched to be parameters in
**          config.dat.  Call PMget().
**      15-oct-03 (chash01)
**          modify some gw02_log_error() calls to pass file name,
**          not file internal identifier (ifi).  Bug 110900
**      05-jan-04 (chash01)
**          Use Ingres version, but replace II with RMS.
**	24-feb-2005 (abbjo03)
**	    Remove include of obsolete gwxit.h.
**	09-oct-2008 (stegr01/joea)
**	    Replace II_VMS_ITEM_LIST_3 by ILE3.
**	24-Oct-2008 (jonj)
**	    Replace ule_doformat with ule_format.
**	12-feb-2010 (abbjo03, wanfr01) Bug 123139
**	    cv.h needs to be added for function definitions
**/


/*
**	this is the release id of the RMS Gateway.  it is coughed up
**	to SCF at server initialization time and is what spits out when
**	the user issues a "select dbmsinfo( '_version' )"
*/

GLOBALDEF   char        Gwrms_version[128];
/*
**  For RMS Gateway secondary indexes, define the tidp row in the extended
**  attribute catalog.  The table id and attribute number will be filled in
**  at runtime.  This row is not really used for anything, but must exist in
**  order to correctly create the tcb for the index.
*/
GLOBALDEF   GWRMS_XATT	Gwrms_xatt_tidp = {{0, 0}, 0, RMSGW_INT, 4, 0, 0, 1};

VOID			gw02_log_error();
static	VOID		gw02_log_chkacc_error();
static	i4		gw02_ingerr( /* rms_opcode, vms_status,
				    default_errcode */ );
static	char		*gw02_rmserr( /* i4 vms_status */ );  
static	VOID		gw02_errsend( /* i4 vms_status */ );
DB_STATUS		gw02_send_error( /* i4 err_code, i4 nparms,
					i4 siz1, PTR arg1, i4 siz2,
					PTR arg2, i4 siz3, PTR arg3,
					i4 siz4, PTR arg4, i4 siz5,
					PTR arg5, i4 siz6, PTR arg6 */ );
static  DB_STATUS	gw02_connect();
static  DB_STATUS	gw02_xlate_key();
static  i4		gw02_max_rms_ksz();

static	VOID		applyTimeout(	/* GW_RSB *gw_rsb,
					   GWRMS_RSB *rms_rsb */ );
static	DB_STATUS	rePositionRAB( /* GWX_RCB *gwx_rcb */ );
static	DB_STATUS	positionRecordStream( /* GWX_RCB *gwx_rcb */ );
static 	DB_STATUS 	rms_open_file();
static 	DB_STATUS 	rms_close_by_name();
static 	DB_STATUS 	rms_close_file();
static 	struct 		rms_fab_info *file_in_hash_table();
static 	struct 		rms_fab_info *alloc_fab_info_block();
static 	VOID 		return_fab_info_blk();
static 	DB_STATUS 	rms_fab_cache_term();
static 	DB_STATUS 	op_scf_mem();

/*
** These symbolic equates are used for the first parameter to gw02_ingerr to
** tell it on which logical RMS operation a particular VMS error occurred:
*/
#define	    RMS_SYS_OPEN	    1L
#define	    RMS_SYS_CONNECT	    2L
#define	    RMS_SYS_DISCONNECT	    3L
#define	    RMS_SYS_CLOSE	    4L
#define	    RMS_SYS_FIND	    5L
#define	    RMS_SYS_GET		    6L
#define	    RMS_SYS_PUT		    7L
#define	    RMS_SYS_UPDATE	    8L
#define	    RMS_SYS_DELETE	    9L
#define	    RMS_SYS_FLUSH	    10L
#define	    RMS_SYS_CHECK_ACCESS    11L
#define	    RMS_SYS_REWIND	    12L

#define	    RFA_LENGTH		    3

/*}
** Name: DESCRIPTOR - A VMS string descriptor.
**
** Description:
**	A structure describing a variable length item in a VMS system service
**	call.
**
** History:
**     02-oct-1985 (derek)
**          Created new for 5.0.
**	19-apr-90 (bryanp)
**	    Copied from erdepend.c for use by gw02_errsend()
*/
typedef struct _DESCRIPTOR
{
    int             desc_length;        /* Length of the item. */
    char            *desc_value;        /* Pointer to item. */
}   DESCRIPTOR;

/*
** Asynchronous I/O in the RMS Gateway.
**
** This version of the RMS Gateway supports asynchronous I/O, which is critical
** in a server environment. There are several issues involved in the support of
** asynchronous I/O:
**
** 0) Our <fab.h> does not contain a definition for FAB$M_ASY, nor does the
**	documentation on sys$open && sys$close say that it can be used there.
**	So we currently only perform asynch I/O on RAB calls. The FAB stuff
**	is all synchronous now (sys$open && sys$close).
** 1) The interface used by RMS is a bit peculiar, such that we cannot directly
**	use CSresume as our AST handler. Rather, we use our own routines
**	(rms_fab_suspend, rms_fab_resume, rms_rab_suspend, rms_rab_resume),
**	which are modelled after the DI support for async RMS.
** 2) Lock timeouts can be handled directly by RMS, but current code does
**	not support them. Enhancing the system to support lock timeouts should
**	not be hard, with the proviso that we let RMS, rather than CS, do
**	the timeout.
** 3) Interrupts (Control-C) are not currently supported. The Control C is
**	heard and registered with the server, but the CSsuspend is not
**	interruptable, basically because we don't know how to abort a pending
**	sys$find or sys$get. This is thought to be an acceptable solution
**	for now, but bears re-considering in the future.
** 4) The CS_GWFIO_MASK should be used to distinguish foreign file wait events
**	from standard Ingres wait events. It's not clear whether CS_GWFIO_MASK
**	is superior to CS_RMSIO_MASK .. each has advantages. Both require work
**	within CS as well as in GWF.
** 5) Note that since the AST handlers are called whether or not the operation
**	completes before the system call returns, we can UNCONDITIONALLY call
**	CSsuspend(). Think about this for a while...it's important!
** 6) Asynchronous operation of RMS calls is more or less orthogonal with
**	use of completion handlers. In particular, to switch between synch and
**	asynch operation, you need only set or clear the M_ASY mask bit in
**	the ROP or FOP field. The rms_XXX_suspend/resume routines work properly
**	for synchronous calls as well as for asynchronous calls (the handler
**	is called before the sys$RMS-OP returns, resulting in the CSresume
**	preceding the CSsuspend, but this all functions correctly).
*/

static	i4		rms_fab_suspend();
static	VOID		rms_fab_resume();
static	i4		rms_rab_suspend();
static	VOID		rms_rab_resume();


/*{
**  Other externals and/or forward references.
*/
GW_RSB	*gwu_find_assoc_rsb();

static DB_STATUS rms_rfapool_init
(
    PTR  *memptr,
    DB_ERROR *error
)
{
    struct gwrms_rfapool_handle *rfahandle;
    struct gwrms_rfa *rfaptr;
    DB_STATUS	status;
    i2 i;
    /*
    **  first, allocate the top level handler, this is the ptr to return
    */
    status = op_scf_mem(SCU_MALLOC,
                        (PTR)&rfahandle,
                        sizeof(struct gwrms_rfapool_handle),
                        error);
    if (status != E_DB_OK)
       return(status);
    MEfill(sizeof(struct gwrms_rfapool_handle), 0, (PTR)rfahandle);
    *memptr =  rfahandle;
    CSi_semaphore(&rfahandle->rfacntrl, CS_SEM_SINGLE);
    /*
    ** allocate memory for rfa pool
    */
    status = op_scf_mem(SCU_MALLOC,
                        (PTR)&rfahandle->memlist,
                        sizeof(struct gwrms_rfa_chunk),
                        error);
    if (status != E_DB_OK)
       return(status);
    MEfill(sizeof(struct gwrms_rfa_chunk), 0, rfahandle->memlist);
    rfahandle->rfahdr = rfaptr = rfahandle->memlist->rfa_list;
    for (i = 0; i < RMSRFALISTSIZE - 1; i++)
        rfaptr[i].left = &rfaptr[i+1];
    rfaptr[i].left = NULL;
    return(E_DB_OK);
}
static DB_STATUS rms_rfapool_term
(
    PTR       memptr,
    DB_ERROR *error
)
{    
    struct gwrms_rfapool_handle *rfahandle;
    PTR temp;
    struct gwrms_rfa *rfaptr;
    DB_STATUS	status;

    /*
    ** first free the rfa memory blocks
    */
    rfahandle = (struct rms_rfapool_handle *)memptr;
    while (rfahandle->memlist != NULL)
    {
        temp = rfahandle->memlist;
        rfahandle->memlist = rfahandle->memlist->next;
        status = op_scf_mem(SCU_MFREE,
                            &temp,
                            sizeof(struct gwrms_rfa_chunk),
                            error);
/*
** for termination, return error should not stop termination process
**    if (status != E_DB_OK)
**       return(status);
*/   
    }
    /* 
    ** then free the handle itself
    */
    status = op_scf_mem(SCU_MFREE,
                        &rfahandle,
                        sizeof(struct gwrms_rfapool_handle),
                        error);
    return(status);
}

static DB_STATUS rms_expand_rfapool
(
    struct gwrms_rfapool_handle *rfahandle,
    DB_ERROR  *error
)
{
    PTR temphandle;
    struct gwrms_rfa_chunk *alloc_ptr;
    struct gwrms_rfa *rfaptr;
    DB_STATUS status;
    i2 i;
    /*
    ** we don't get exclusive control of rfa pool because it must be in
    ** effect if we ever need to expand rfapool 
    */
    status = op_scf_mem(SCU_MALLOC,
                        (PTR)&alloc_ptr,
                        sizeof(struct gwrms_rfa_chunk),
                        error);
    if (status == E_DB_OK)
    {
        MEfill(sizeof(struct gwrms_rfa_chunk), 0, alloc_ptr);
        /*
        ** put allocated memory on rfahandle->memlist
        */
        temphandle = rfahandle->memlist;
        rfahandle->memlist = alloc_ptr;
        alloc_ptr->next = temphandle;
        /*
        ** link up rfa list and put it on rfahandle->rfa_hdr
        */
        rfahandle->rfahdr = rfaptr = rfahandle->memlist->rfa_list;
        for (i = 0; i < RMSRFALISTSIZE - 1; i++)
            rfaptr[i].left = &rfaptr[i+1];
        rfaptr[i].left = NULL;
    }
    return(status);
}

static DB_STATUS rms_expand_stack
(
    GWRMS_TAB_HANDLE *tophandle
)
{
    struct gwrms_rfa *rfastk;
    DB_STATUS status;
    DB_ERROR error;
    i4 stacksize = tophandle->stacksize;

    status = op_scf_mem(SCU_MALLOC, (PTR)&rfastk,
                        sizeof(struct gwrms_rfa *)*stacksize*2, &error);
    if (status != E_DB_OK)
        return(status);
    MEcopy((PTR)tophandle->stack, sizeof(struct gwrms_rfa *)*stacksize,
           (PTR)rfastk);
    status = op_scf_mem(SCU_MFREE,(PTR)&tophandle->stack,
                        sizeof(struct gwrms_rfa *)*stacksize, &error);
    if(status != E_DB_OK)
        return(status);
    tophandle->stack = rfastk;
    tophandle->stacksize = stacksize*2;
    return(E_DB_OK);
}

/*
** when balance a binary tree to HB(1),
** the stack will shrink, not to grow
*/
static VOID rms_rfa_insert_balance
(
    struct gwrms_rfa **stack,
    i4               stacktop,
    struct gwrms_rfa **rfalist,
    struct gwrms_rfa *current,
    i2               flag
)
{
    struct gwrms_rfa *son;
    struct gwrms_rfa *grandson;
    struct gwrms_rfa *temprfa;

    if (flag == 0)
    {
        son = current->left;
    }
    else
    {
        son = current->right;
    }
    son->bias = BALANCED;
    grandson = NULL;
    for (;;)
    {
        if (((current->bias == LEFTHEAVY) && (current->right == son)) ||
            ((current->bias == RIGHTHEAVY) && (current->left == son)))
        {
            current->bias = BALANCED;
            return;
        }
        if (current->bias == BALANCED)
        {
            if (current->left == son)
                current->bias = LEFTHEAVY;
            else
                current->bias = RIGHTHEAVY;
            if (stacktop > 0)
            {
                grandson = son;
                son = current;
                current = stack[--stacktop];
            }
            else
                return;
        }
        else  /* here comes the rotations */
        {
            if ((current->bias == LEFTHEAVY) && (current->left == son))
            {
                if (son->left == grandson)
                {
                    /*
                    ** single right rotation
                    */

                    current->left = son->right;
                    son->right = current;
                    son->bias = current->bias = BALANCED;
                    if (current == *rfalist)
                        *rfalist = son;
                    else 
                    {
                        temprfa = stack[stacktop-1];
                        if (temprfa->left == current)
                            temprfa->left = son;
                        else
                            temprfa->right = son;
                    }
                }
                else /* son->right == grandson */
                {
                    /*
                    ** double right rotation
                    */
                    current->left = grandson->right;
                    son->right =  grandson->left;
                    grandson->left = son;
                    grandson->right = current;
                    if (grandson->bias == RIGHTHEAVY)
                    {
                        current->bias = BALANCED;
                        son->bias = LEFTHEAVY;
                    }
                    else if (grandson->bias == LEFTHEAVY)
                    {
                        son->bias = BALANCED;
                        current->bias = RIGHTHEAVY;
                    }
                    else
                    {
                        son->bias = BALANCED;
                        current->bias = BALANCED;
                    }
                    grandson->bias = BALANCED;
                    if (current == *rfalist)
                        *rfalist = grandson;
                    else 
                    {
                        temprfa = stack[stacktop-1];
                        if (temprfa->left == current)
                            temprfa->left = grandson;
                        else
                            temprfa->right = grandson;
                    }
                }
            }
            else if ((current->bias == RIGHTHEAVY) && (current->right == son))
            {
                if (son->right == grandson)
                {
                    /*
                    ** single left rotation
                    */
                    current->right = son->left;
                    son->left = current;
                    son->bias = current->bias = BALANCED;
                    if (current == *rfalist)
                        *rfalist = son;
                    else 
                    {
                        temprfa = stack[stacktop-1];
                        if (temprfa->left == current)
                            temprfa->left = son;
                        else
                            temprfa->right = son;
                    }
                }
                else /* son->left = grandson */
                {
                    /*
                    ** double left rotation
                    */
                    current->right = grandson->left;
                    son->left = grandson->right;
                    grandson->right = son;
                    grandson->left = current;
                    if (grandson->bias == LEFTHEAVY)
                    {
                       current->bias = BALANCED;
                       son->bias = RIGHTHEAVY;
                    }
                    else if (grandson->bias == RIGHTHEAVY)
                    {
                       current->bias = LEFTHEAVY;
                       son->bias = BALANCED;
                    }
                    else
                    {
                       current->bias = BALANCED;
                       son->bias = BALANCED;
                    }
                    grandson->bias = BALANCED;
                    if (current == *rfalist)
                        *rfalist = grandson;
                    else 
                    {
                        temprfa = stack[stacktop-1];
                        if (temprfa->left == current)
                            temprfa->left = grandson;
                        else
                            temprfa->right = grandson;
                    }
                }
            }
            return;
        }
    }
    return;
}
      

static DB_STATUS rms_get_rfa_buffer
(
    GW_SESSION *session,
    i4         tab_id,
    struct     gwrms_rfapool_handle *rfahandle,
    struct     gwrms_rfa **retrfa,
    i4         rfa0,
    i2         rfa4,
    i2         seqno,
    DB_ERROR   *error
)
{
    DB_STATUS status = E_DB_OK;
    i2 i, firstavail= -1;
    struct gwrms_table_array *tab_handle;
    GWRMS_TAB_HANDLE *tophandle;
    struct gwrms_rfa **stack;

    if (session->gws_exit_scb[GW_RMS] == NULL)
    {
        struct gwrms_rfa *rfastk;
        /*
        ** first time accessing rfa buffering, allocate memory
        ** for session level table handle.  Stack memory will also
        ** be allocated here to simplify later tree balancing steps.
        */

        status = op_scf_mem(SCU_MALLOC,
                            (PTR)&tophandle,
                            sizeof(GWRMS_TAB_HANDLE),
		            error);
        if (status != E_DB_OK)
            return(status);
        MEfill(sizeof(GWRMS_TAB_HANDLE), 0, tophandle);
        session->gws_exit_scb[GW_RMS] = (PTR)tophandle;
        status = op_scf_mem(SCU_MALLOC,
                            (PTR)&rfastk,
                            sizeof(struct gwrms_rfa *)*RMSSTKINITSIZE,
		            error);
        if (status != E_DB_OK)
            return(status);
        tophandle->stacksize = RMSSTKINITSIZE;
        tophandle->stack = rfastk;
        MEfill(sizeof(struct gwrms_rfa *)*RMSSTKINITSIZE, 0, rfastk);

    }
    /*
    ** go through the same logic regardless if table handle
    ** is just allocated.
    */
    tab_handle = 
        ((GWRMS_TAB_HANDLE *)session->gws_exit_scb[GW_RMS])->tab_array;
    for (i = 0; i< RMSMAXTABNO; i++)
    {
        if (tab_handle[i].tab_id == tab_id)
        {
           firstavail = i;
           break;
        }
        if ((tab_handle[i].tab_id == 0) && (firstavail = -1))
        {
           firstavail = i;
           break;
        }
    }
    /*
    ** exceed the maximum of 32 open table per query(per session)
    */
    if (firstavail == -1)
        return(E_DB_ERROR);

    i = firstavail;
    /*
    ** table not found, create its entry
    */
    if (tab_handle[i].tab_id == 0)
    {
        tab_handle[i].tab_id = tab_id;
        CSp_semaphore(1, &rfahandle->rfacntrl);
        for (;;)
        {
            if (rfahandle->rfahdr == NULL)
            {
                status = rms_expand_rfapool(rfahandle, error);
                if (status != E_DB_OK)
                    break;
            }
            *retrfa = rfahandle->rfahdr;
            rfahandle->rfahdr = rfahandle->rfahdr->left;
            break;
        }
        CSv_semaphore(&rfahandle->rfacntrl);
        if (status != E_DB_OK)
                return(status);
        (*retrfa)->rfa0 = rfa0;
        (*retrfa)->rfa4 = rfa4;
        (*retrfa)->seqno = seqno;
        (*retrfa)->bias = BALANCED;
        (*retrfa)->left = NULL;
        (*retrfa)->right = NULL;
        tab_handle[i].rfalist = *retrfa;
        return(E_DB_OK);
    }
    /*
    ** table found, find the specified RFA, we must use allocated
    ** stack to keep track of parents so that if an insert is done,
    ** we can backtrack to rebalance the tree
    */
    if (tab_handle[i].tab_id == tab_id)
    {
        struct gwrms_rfa *parent, *current;
        i4 stacktop = 0;
        i2 flag;

        if (tab_handle[i].rfalist != NULL)
        {
            tophandle = (GWRMS_TAB_HANDLE *)session->gws_exit_scb[GW_RMS];
            stack = tophandle->stack;
            parent = current = tab_handle[i].rfalist;
            do
            {
                if (current->rfa0 == rfa0 && current->rfa4 == rfa4
                    && current->seqno == seqno)
                {
                    *retrfa = current;
                    return(E_DB_OK);
                }
                if ((rfa0 < current->rfa0 ) 
                    || ((current->rfa0 == rfa0) && (rfa4 < current->rfa4))
                    || ((current->rfa0 == rfa0) && (current->rfa4 == rfa4)
                          && (seqno < current->seqno))
                   )
                {
                    if (stacktop == tophandle->stacksize)
                    {
                        status  = rms_expand_stack(tophandle);
                        if (status != E_DB_OK)
                            return(status);
                        stack = tophandle->stack;
                    }
                    stack[stacktop++] = current;
                    parent = current;
                    current = current->left;
                    flag = 0;
                }
                else if ((rfa0 > current->rfa0)
                         || ((current->rfa0 == rfa0) && (rfa4 > current->rfa4))
                         || ((current->rfa0 == rfa0) && (current->rfa4 == rfa4)
                             && (seqno > current->seqno))
                        )
                {
                    if (stacktop == tophandle->stacksize)
                    {
                        status  = rms_expand_stack(tophandle);
                        if (status != E_DB_OK)
                            return(status);
                        stack = tophandle->stack;
                    }
                    stack[stacktop++] = current;
                    parent = current;
                    current = current->right;
                    flag = 1;
                }
                else; /* must be a match and already returned */
            } while (current != NULL);
        }   
        /*
        ** get an available rfa buffer, stuff the rfa into
        ** it , insert it into the tree and return its address
        */
        CSp_semaphore(1, &rfahandle->rfacntrl);
        for (;;)
        {
            if (rfahandle->rfahdr == NULL)
            {
                status = rms_expand_rfapool(rfahandle, error);
                if (status != E_DB_OK)
                    break;
            }
            *retrfa = rfahandle->rfahdr;
            rfahandle->rfahdr = rfahandle->rfahdr->left;
            break;
        }
        CSv_semaphore(&rfahandle->rfacntrl);
        if (status != E_DB_OK)
            return(status);
        (*retrfa)->rfa0 = rfa0;
        (*retrfa)->rfa4 = rfa4;
        (*retrfa)->seqno = seqno;
        (*retrfa)->left = NULL;
        (*retrfa)->right = NULL;
        /*
        ** now insert rfa buffer into the tree
        */
        if (tab_handle[i].rfalist != NULL)
        {
            if (flag  == 0) /* insert on left  of parent */
                parent->left = *retrfa;
            else            /* flag must be 1, insert right */
                parent->right = *retrfa;
            /*
            **  we balance the tree only if it does not
            **  start with an empty tree, pass into the routine
            **  stack, stacktop, rfatree, parent and flag.
            **  This process should not produce any error.
            **  Note that the top element in stack is parent,
            **  we need to decrement stacktop before getting 
            **  inside the balance procedure.  Also, we need
            **  to alter tab_handle[i].rfalist after the call
            **  in case that rotation is done on the first node
            **  of RFA tree
            */
            rms_rfa_insert_balance(stack, stacktop-1, &tab_handle[i].rfalist,
                                   parent, flag);
        }
        else
        {
            tab_handle[i].rfalist = *retrfa;
            (*retrfa)->bias = BALANCED;
        }
        return(E_DB_OK);
    }           
}

static STATUS rms_post_order_remove
(
    GWRMS_TAB_HANDLE *tophandle,
    i2     ii,
    struct gwrms_rfa **rfabegin,
    struct gwrms_rfa **rfaend
)
{
    struct gwrms_table_array *tab_handle = tophandle->tab_array;
    struct gwrms_rfa **stack = tophandle->stack;
    struct gwrms_rfa *current, *prev, *next;
    i2 stacktop = 0;      /* stacktop points to the first available space */
    i2 stacksize = tophandle->stacksize;
    struct gwrms_rfa *rfastk;
    DB_ERROR error;
    DB_STATUS status;

    current = tab_handle[ii].rfalist;
    while (current != NULL)
    {
        /*
        ** traverse left
        */
        if (current->left != NULL)
        {
            /*
            ** push(current);
            */
            if (stacktop == stacksize)
            {
                /*
                **   NOTE: expand the stack
                */
                status = rms_expand_stack(tophandle);
                if (status != E_DB_OK)
                    return(status);
                stack = tophandle->stack;
                stacksize = tophandle->stacksize;
            }
            stack[stacktop++] = current;
            current = current->left;
            stack[stacktop-1]->left = NULL;
        }
        /*
        ** traverse right
        */
        else if (current->right != NULL)
        {
            /*
            ** push(current);
            */
            if (stacktop == stacksize)
            {
                /*
                **   NOTE: expand the stack
                */
                status = rms_expand_stack(tophandle);
                if (status != E_DB_OK)
                    return(status);
                stack = tophandle->stack;
                stacksize = tophandle->stacksize;
            }
            stack[stacktop++] = current;
            current = current->right;
            stack[stacktop-1]->right = NULL;
        }
        /*
        ** get rid of this node backup to its parent
        */
        else 
        {
            current->left = *rfabegin;
            *rfabegin = current;
            if (*rfaend == NULL)
                *rfaend = current;
            if (stacktop > 0)
            {
                current = stack[--stacktop];
            }
            else
                current = NULL;
        }
    }
    return(E_DB_OK);
}

static DB_STATUS rms_return_rfa_buffer
(
    GW_SESSION *session,
    i4         tab_id,
    struct gwrms_rfapool_handle *rfahandle
)
{
    struct gwrms_rfa *rfabegin, *rfaend;
    GWRMS_TAB_HANDLE *top_handle = (GWRMS_TAB_HANDLE *)
                                        session->gws_exit_scb[GW_RMS];
    struct gwrms_table_array *tab_handle;
    DB_STATUS status = E_DB_OK;
    i2 i;

    /*
    **  since GWRMS_TAB_HANDLE now is allocated at the first
    **  time it is accessed, we may hay a null tab_handle at
    **  the time of file close due to the fact that file open
    **  does not always means file access (read).
    */

    if (top_handle == NULL)
        return(E_DB_OK);
    /* 
    ** first find the table, table not found is ok
    ** no rfa list associated is also ok
    */
    tab_handle = top_handle->tab_array;
    for (i = 0; i < RMSMAXTABNO; i++)
    {
        if (tab_handle[i].tab_id == tab_id)
           break;
    }
    if (i >= RMSMAXTABNO)
        return(E_DB_OK);

    if (tab_handle[i].rfalist == NULL)
        return(E_DB_OK);

    /*
    ** rfahandle can never be NULL
    */
    if (rfahandle == NULL)
        return(E_DB_ERROR);

    rfabegin = rfaend = NULL;
    status = rms_post_order_remove(top_handle, i, &rfabegin, &rfaend);
    tab_handle[i].rfalist = NULL;
    tab_handle[i].tab_id = 0;
    if (status != E_DB_OK)
        return(status);
    /*
    ** get exclusive control of rfa pool 
    ** and release this list in one shut 
    */
    CSp_semaphore(1, &rfahandle->rfacntrl);
    if (rfabegin != NULL)
    {
        rfaend->left = rfahandle->rfahdr;        
        rfahandle->rfahdr = rfabegin;
    }
    CSv_semaphore(&rfahandle->rfacntrl);
    return(E_DB_OK);
}


/*{
** Name: gw02_term - terminate gateway exit
**
** Description:
**	This exit performs exit shut down which may include shutdown of the
**	foreign data manager.
**
**	RMS requires no specific shutdown operations.
**	
** Inputs:
**	gwx_rcb			- Not used by the RMS gateway.
**
** Output:
**	None
**
**      Returns:
**          E_DB_OK		Function completed normally. 
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	27-dec-89 (paul)
**	    Conform to conventions by passing the GWX_RCB structure.  In the
**	    future some gateway may need it!
**      21-sep-92 (schang)
**          Good think, now I need to use gwx_rcb to terminate rms gateway
**          specif memory
**      13-jun-96 (schang)
            integrate 64 changes
**        08-dec-93 (schang)
**          add termination of rfa pool
*/
DB_STATUS
gw02_term
(
    GWX_RCB *gwx_rcb
)
{
    DB_STATUS rms_fab_cache_term(),
              rms_rfapool_term();
    DB_STATUS status;

    if ((gwx_rcb->xrcb_xbitset & II_RMS_BUFFER_RFA)
        && (gwx_rcb->xrcb_xbitset &II_RMS_READONLY))
        status = rms_rfapool_term(((PTR *)gwx_rcb->xrcb_xhandle)[RMSRFAMEMIDX],
                         &gwx_rcb->xrcb_error);
    status = rms_fab_cache_term(((PTR *)gwx_rcb->xrcb_xhandle)[RMSFABMEMIDX],
	                        &gwx_rcb->xrcb_error);
    /*
    ** release xhandle itself
    */
    status = op_scf_mem(SCU_MFREE,
                        (PTR)&gwx_rcb->xrcb_xhandle,
                        sizeof(PTR)*RMSMAXMEMPTR,
                         &gwx_rcb->xrcb_error);
    return(status);

}

/*{
** Name: gw02_sinit - initialize gateway session exit
**
** Description:
**	This exit performs exit session initialization
**
** Inputs:
**	gwx_rcb			- RMS gateway take GW_SEESIOn and initalize
**                                gws_exit_scb[GW_RMS].
**
** Output:
**	None
**
**      Returns:
**          E_DB_OK		Function completed normally. 
**
** History:
**      06-oct-92 (schang)
**        created.
**      22-dec-93 (schang)
**        add session level init for base table level rfa buffering
*/
DB_STATUS gw02_sinit
( 
    GWX_RCB *gwx_rcb
)
{
    GW_SESSION *session         = gwx_rcb->xrcb_gw_session;

    /*
    ** we do not allocate anything at session start for RFA buffering
    ** because a seesion may not be a user session.  We do need to NULL
    ** the session->gws_exit_scb[GW_RMS] field so that later by testing
    ** for NULL we know if we have initialized this part. 
    */
    session->gws_exit_scb[GW_RMS] = NULL;
    return(E_DB_OK);
}
/*{
** Name: gw02_sterm - terminate gateway session exit
**
** Description:
**	This exit performs exit session shut down
**
**	RMS requires no specific shutdown operations.
**	
** Inputs:
**	gwx_rcb			- Not used by the RMS gateway.
**
** Output:
**	None
**
**      Returns:
**          E_DB_OK		Function completed normally. 
**
** History:
**      06-oct-92 (schang)
**        created.
**      13-jun-96 (schang)
**        integrate 64 changes
**        22-dec-93 (schang)
**          add session level termination of base table level rfa buffering
*/
gw02_sterm
( 
    GWX_RCB *gwx_rcb
)
{
    GW_RSB	*rsb		= (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_SESSION *session         = gwx_rcb->xrcb_gw_session;
    GWRMS_TAB_HANDLE *tab_handle;
    DB_STATUS status = E_DB_OK;

    tab_handle = session->gws_exit_scb[GW_RMS];
    if (tab_handle != NULL)
    {
        if (tab_handle->stack != NULL)
            status = op_scf_mem(SCU_MFREE,
                            (PTR)&tab_handle->stack,
                            sizeof(struct gwrms_rfa *)*tab_handle->stacksize,
		            gwx_rcb->xrcb_error.err_code);

        status = op_scf_mem(SCU_MFREE,
                            (PTR)&tab_handle,
                            sizeof(GWRMS_TAB_HANDLE),
		            gwx_rcb->xrcb_error.err_code);
    }
    return (status);
}

/*{
** Name: gw02_tabf - format table registration catalog tuples
**
** Description:
**	This function formats extended gateway table's iigwX_rel and iigwX_att
**	tuples into the provided tuple buffers.  Note the sizes of these
**	buffers were returned from a gwX_init() request.
**
** Inputs:
**	gwx_rcb->		gateway request control block
**		tab_name	the table name being registered
**		tab_id		INGRES table id
**		gw_id		gateway id, derived from DBMS type
**		var_data1	table options for gateway table.
**		var_data2	filename or path of the gateway table
**		column_cnt;	column count of the table
**		column_attr	INGRES column info
**		var_data_list	array of extended column attribute strings.
**				A null indicates no extend attributes.
**		var_data3	tuple buffer for iigwX_relation
**		mtuple_buffs	an allocated array of iigwX_attribute
**				 tuple buffers.
**
** Output:
**	gwx_rcb->
**		var_data3	iigwX_relation tuple
**		mtuple_buffs	iigwX_attribute tuples
**		error.err_code	One of the following error numbers:
**				E_GW1004_BAD_REL_SOURCE
**                              E_GW0002_BAD_XATT_FORM
**				E_GW0102_GWX_VTABF_ERROR
**				E_GW050E_ALREADY_REPORTED
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code set.
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	24-dec-89 (paul)
**	    Assorted modifications made to match the calling conventions of GWF.
**	17-apr-90 (bryanp)
**	    Updated error handling to new scheme. Check return codes and log
**	    errors if something goes wrong.
**	12-jun-1990 (bryanp)
**	    If we get a syntax error parsing the format string of a column,
**	    send the message to the frontend and return with E_GW050E so that
**	    we do 'nice' error handling rather than 'internal error' handling.
**	5-jul-90 (linda)
**	    Moved some calls from gw02_tabf() down to gw02__fmt() and
**	    gw02__xfmt(), where we still have detailed information to send up
**	    to the user.
*/

DB_STATUS
gw02_tabf
(
    GWX_RCB *gwx_rcb
)
{
    GWRMS_XREL		*xrel = (GWRMS_XREL *)
				    gwx_rcb->xrcb_var_data3.data_address;
    GWRMS_XATT		*xatt = (GWRMS_XATT *)
				    gwx_rcb->xrcb_mtuple_buffs.ptr_address;
    DMT_ATT_ENTRY	*att = gwx_rcb->xrcb_column_attr;
    i4			i;
    DMU_GWATTR_ENTRY	**gwatt = (DMU_GWATTR_ENTRY **)
				    gwx_rcb->xrcb_var_data_list.ptr_address;
    i4			ngwatt = gwx_rcb->xrcb_var_data_list.ptr_in_count;
    DB_STATUS		status = E_DB_OK;
    DB_ERROR		error;
    i4			next_offset = 0;

    /*
    ** Create row for iigwXX_relation.  This entry consists of the table id for
    ** the ingres table and the (source) filename for the RMS file.  Note that
    ** we blank pad the filename so that it looks friendlier when we print it.
    */
    STRUCT_ASSIGN_MACRO(*gwx_rcb->xrcb_tab_id, xrel->xrel_tab_id);
    STmove(gwx_rcb->xrcb_var_data2.data_address, ' ', sizeof(xrel->file_name),
	xrel->file_name);

    /*
    ** Create a row in iigwXX_attribute for each column of the table.  These
    ** rows contain any attribute information specific to the gateway.  For the
    ** rms gateway this is the type, length and precision of the data type in
    ** the RMS file. Note that if no extended attributes are specified in the
    ** input parameters, the information is defaulted based on the ingres type
    ** and length (and precision/scale, when appropriate).  RMS attributes
    ** always get their own type id's, this has turned out to be the most
    ** flexible approach.
    */

    /*
    ** First xatt entry gets default offset of 0.  If an extended format was
    ** specified, goffset will be updated before being appended to the extended
    ** attribute catalog.  After the first one, the default offset is
    ** maintained based on the previous record's glength and goffset, whether
    ** returned from extended format parsing or not.
    */

    for (i=0; i < gwx_rcb->xrcb_column_cnt; ++i, ++xatt, ++att, ++gwatt)
    {
	
	STRUCT_ASSIGN_MACRO(*gwx_rcb->xrcb_tab_id, xatt->xatt_tab_id);
	xatt->xattid = att->att_number;
	xatt->gflags = (i2)(*gwatt)->gwat_flags_mask;
	xatt->goffset = next_offset;	/* always 0 the first time through. */

	/***
	**
	** Note that there are exceptions to the types of fields that can be
	** placed in an RMS file. This should probably be checked elsewhere but
	** i'll just place a reminder here...
	**
	**	RMS files do not normally support the notion of NULL values.
	**	It is generally assumed that every field specified contains a
	**	value. If the specification of the RMS datatype or the
	**	specification inherited from ingres allows NULLs we should
	**	probably reject the format request.
	**
	**	There are also issues with several ingres datatypes that should
	**	not be placed in an rms file (money, dates, varchar, text)
	**
	**	Can an RMS file support DEFAULTs? I don't have a spec handy so
	**	don't know what we should say here.
	*/

	if ((*gwatt)->gwat_flags_mask & DMGW_F_EXTFMT)
	{
	    /*
	    ** We have extended format specifier.  gw02__xfmt will always
	    ** update the xatt->goffset field.
	    */
	    status = gw02__xfmt(xatt, *gwatt, &error);
	    if (status != E_DB_OK)
	    {
		if (error.err_code == E_GW050E_ALREADY_REPORTED)
		{
		    /* parameterized error message was sent to user. */
		    gwx_rcb->xrcb_error.err_code = E_GW050E_ALREADY_REPORTED;
		}
		else
		{
		    /* error was logged; return with generic tabf error */
		    gwx_rcb->xrcb_error.err_code = E_GW0102_GWX_VTABF_ERROR;
		}
		break;
	    }
	}
	else
	{
	    /*
	    ** No extended format specified; use defaults.  gw02__fmt will not
	    ** update the xatt->goffset field; it is always based on the last
	    ** field's offset and length.
	    */
	    status = gw02__fmt(xatt, att, &error);
	    if (status != E_DB_OK)
	    {
		if (error.err_code == E_GW050E_ALREADY_REPORTED)
		{
		    /* parameterized error message was sent to user. */
		    gwx_rcb->xrcb_error.err_code = E_GW050E_ALREADY_REPORTED;
		}
		else
		{
		    /* error was logged; return with generic tabf error */
		    gwx_rcb->xrcb_error.err_code = E_GW0102_GWX_VTABF_ERROR;
		}
		break;
	    }
	}
	next_offset = xatt->goffset + xatt->glength;
    }

    return(status);
}

/*{
** Name: gw02_idxf - format index registration catalog tuple
**
** Description:
**	This function formats the extended gateway index's iigwX_index tuple
**	into the provided tuple buffer.  Note the size of this buffer was
**	returned  from a gwX_init() request.  If the reference key is
**	invalid, this routines tells the user so.
**
** Inputs:
**	gwx_rcb->		gateway request control block
**		tab_id		INGRES table id
**		index_desc	index description
**		var_data1	an allocated tuple buffer for
**				iigwX_index.
**		var_data2	source of the gateway index.
**
** Output:
**	gwx_rcb->
**		var_data1	iigwX_index tuple
**		error.err_code	One of the following error numbers.
**                              E_GW050E_ALREADY_REPORTED
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    Noted some potential problems with internal parsing.
**	17-apr-90 (bryanp)
**	    Changed a STscanf() to a CVal(). STscanf is no longer legal.  Set
**	    error.err_code appropriately on error.
**	11-jul-91 (rickh)
**	    Return an error to user if he types a bad index value.
*/

#define	MAX_KEY_OF_REFERENCE	254

DB_STATUS
gw02_idxf
(
    GWX_RCB *gwx_rcb
)
{
    GWRMS_XIDX	*xidx = (GWRMS_XIDX *)gwx_rcb->xrcb_var_data1.data_address;
    DB_STATUS	status;

    STRUCT_ASSIGN_MACRO(*gwx_rcb->xrcb_tab_id, xidx->xidx_idx_id);

    /*
    **  Check that the key_ref is within reasonable bounds.
    */

    for ( ;; )	/* declare a code block that we can break out of */
    {

	if (CVal(gwx_rcb->xrcb_var_data2.data_address, &xidx->key_ref) == OK)
	{
	    if ( ( xidx->key_ref > 0)
		 && (xidx->key_ref <= MAX_KEY_OF_REFERENCE ) )
	    {
		status = E_DB_OK;
		gwx_rcb->xrcb_error.err_code = E_DB_OK;
		break;
	    }
	}

	status = E_DB_ERROR;
	(VOID) gwf_error( E_GW5013_RMS_BAD_INDEX_VALUE, GWF_USERERR, 1,
			STlength( gwx_rcb->xrcb_var_data2.data_address ),
			gwx_rcb->xrcb_var_data2.data_address );
	gwx_rcb->xrcb_error.err_code = E_GW050E_ALREADY_REPORTED;

	break;

    }	/* end for */

    return (status);
}

/*{
** Name: gw02_open - exit table open
**
** Description:
**
**	This exit opens a gateway table for a session.  gw02_open() must be
**	called before any other table or record operations can be done.
**
**	This exit may allocate a local TCB and a local RSB if necessary. These
**	control blocks should be allocated from the memory stream associated
**	with the corresponding GWF control block. This will allow GWF to
**	deallocate them without having to call back the specific gateway. The
**	information in these control blocks may be used in future gateway
**	calls.
**
**	Pointer parameter values to this procedure are expected to be valid for
**	the duration of the table open (until gw02_close) making structure
**	copying unnecessary.
**
**	Note that the extended catalog tuple buffers contain data that has been
**	retrieved from the catalog.  The exit may want to format or extract the
**	data into its private control block.
**
**	Only local TCB and RSB data may be altered by the gateway.
**
**	For RMS a local TCB (internal_tcb) and RSB (internal_rsb) are
**	allocated. The TCB contains CX's used for converting between RMS and
**	INGRES formats.
**
**	All required RMS control blocks (FAB and RAB) are allocated in the
**	GWX_RSB. This means that a separate FAB is created for each open
**	request.
**
** Inputs:
**	gwx_rcb->
**		xrcb_rsb	Pointer to GW_RSB. This contains all the
**				information we must use or modify. We make the
**				assumption that both the RSB and TCB are valid
**				for this access.
**		xrcb_rcb->tcb	Pointer to the GW_TCB structure for this gateway
**				object.
**
** Output:
**	gwx_rcb->
**		xrcb_rsb.internal_rsb
**				Will contain the gateway specific RSB info
**				required for table access.
**		xrcb_rsb->tcb->internal_tcb
**				Will contain the gateway specific TCB info
**				required for this table. We conform to the
**				convention that this field is NULL until the
**				first "open" call on this table. At that time
**				this procedure is expected to fill this field
**				in if necessary. If internal_tcb != NULL, we
**				assume that a previous "open" has completed
**				the initialization.
**		xrcb_rsb.gwrsb_page_cnt
**				"page" count for the data file.  For RMS
**				Gateway, it's the number of blocks allocated
**				for the file.
**		error.err_code	Detailed error code describing an error.
**				If status != E_DB_OK, will be one of:
**				E_GW0104_GWX_VOPEN_ERROR
**				E_GW5400_RMS_FILE_ACCESS_ERROR
**				E_GW5402_RMS_BAD_DEVICE_TYPE
**				E_GW5403_RMS_BAD_DIRECTORY_NAME
**				E_GW5405_RMS_DIR_NOT_FOUND
**				E_GW5406_RMS_FILE_LOCKED
**				E_GW5407_RMS_FILE_NOT_FOUND
**				E_GW5408_RMS_BAD_FILE_NAME
**				E_GW5409_RMS_LOGICAL_NAME_ERROR
**				E_GW540B_RMS_FILE_SUPERSEDED
**				E_GW5401_RMS_FILE_ACT_ERROR
**				E_GW5404_RMS_OUT_OF_MEMORY
**				E_GW540C_RMS_ENQUEUE_LIMIT
**				E_GW540A_RMS_FILE_PROT_ERROR
**				E_GW541B_RMS_INTERNAL_MEM_ERROR
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	23-dec-89 (paul)
**	    Modified exit to allocate whatever memory it needs.  The other
**	    scheme was never going to work since we never know before this call
**	    how large a record buffer we must allocate for RMS.
**	16-apr-90 (bryanp)
**	    Converted to use new error handling system.
**	23-apr-90 (bryanp)
**	    Asynchronous RMS calls.
**	10-may-90 (alan)
**	    Validate user file access by calling sys$check_access prior 
**	    to table open.
**	16-jun-90 (linda)
**	    Obtain number of blocks allocated for this file from RMS, to be
**	    used by Ingres as a "page" count for the gateway table when it is
**	    first registered.  Only path where this is used is initial table
**	    registration, which generates a call to dm2u_modify() to get pages,
**	    key info, etc. updated properly.  Other paths to this routine just
**	    ignore pages info.
**	9-jan-91 (linda)
**	    Allocate memory here for rms keys.  We were allocating memory
**	    during key translation -- i.e., for each position call.  That was
**	    not a good idea, as we only need one low key buffer and one high
**	    key buffer!
**	28-jan-91 (linda)
**	    Fix memory allocation for gateway-specific tcb.  We were using
**	    SCF allocation, which was never being deallocated.  Our scheme for
**	    allocating tcb's at the GWF level is for each tcb to have its own
**	    ULM memory stream which is deallocated when the tcb is deleted;
**	    we need to use that memory stream here too.
**	22-jul-1991 (rickh)
**	    Removed gwrsb_ing_base_tuple initialization.  We no longer stuff
**	    the full secondary tuple into this secret hiding place.
**	6-aug-91 (rickh)
**	    Initialize state word in rsb.
**      19-jun-92 (schang)
**          do not check accessibility when trying to open a remote file
**          through DECnet (with vnode specified)
**      14-oct-92 (schang)
**          roll in file performance improvement code.  it moves the
**          REAL file open operation to rms_open_file(), including allocation
**          and filling of fabs.
**      13-jun-96 (schang)
**          integrate 64 changes
**        27-sep-93 (schang)
**          piggyback II_RMS_READONLY flag in DB_ERROR
**        07-mar-00 (chash01)
**          add additional args to sys$check_access
*/
DB_STATUS
gw02_open
(
    GWX_RCB *gwx_rcb
)
{
    DB_STATUS	    status;
    
    GW_RSB	    *rsb = gwx_rcb->xrcb_rsb;
    GW_TCB	    *tcb = rsb->gwrsb_tcb;
    
    GWRMS_RSB	    *rms_rsb;
    GWRMS_XTCB	    *rms_tcb;
    GW_ATT_ENTRY    *gwatt = tcb->gwt_attr;
    GWRMS_XATT	    *xatt;
    GWRMS_XREL	    *xrel = (GWRMS_XREL *)tcb->gwt_xrel.data_address;
    GW_SESSION      *session = rsb->gwrsb_session;
    ULM_RCB	    *ulm_rcb;

    SCF_CB	    scf_cb;
    i4		    buffer_size;
    i4		    vms_status;
    i4		    tcb_estimate;
    i4		    ksz;	    /* maximum size of the RMS key */
    DB_ERROR	    error;

    char	    fnam_buf[256];  /*for RMS fab, filename is null-terminated*/
    i4		    len;	    /*returns from STlcopy(), STzapblank()*/
    i4		    sid;	    /* session identification */

    i4		    access_objtyp = ACL$C_FILE;	/* $access_check object type */
    struct	    dsc$descriptor_s	access_objnam;  /* object (file) name */
    struct	    dsc$descriptor_s	access_usrnam;  /* frontend username */
    ILE3	    access_itmlst[2];	/* $access_check itemlist */
    i4		    access_type;	/* read or write access desired */
    u_i2	    access_retlen;      /* $access_check returned length */
    char            *partial_name;
    i4	            is_remote = 0;
    DB_TAB_ID	*db_tab_id = gwx_rcb->xrcb_tab_id;

    /*
    ** Per server conventions, loop is used to break out of an error.  Thus all
    ** returns are through the bottom of the procedure.
    */
    status = E_DB_OK;
    for (;;)
    {
	/*
	** Initialize page count to 1 in case open fails.  Ingres can't handle
	** tables with pages <= 0 (the optimizer chokes).
	*/
	rsb->gwrsb_page_cnt = 1;

	/*
	** Here is where we can perform a number of consistency and sanity
	** checks to decide whether this open of this file 'makes sense'. Many
	** checks are possible (few are currently done). Here are some
	** possibilities for future coders to consider:
	**  1) check the user's permissions (this one is done now)
	**  2) check the registration against the file attributes
	**	a) if file type is fixed, but registration indicates varying,
	**	    could complain. (CONTROVERSIAL)
	**	b) if file length is N, but registration maps fields to > N,
	**	    could complain.  (but note, this could be okay too)
	**	c) if file is indexed, but registration indicates no storage
	**	    structure, could complain.
	**  etc.
	*/

	/* 
	** By default we allow other processes arbitrary access to this file.
	** The rms record locking mechanism will take care of any conflicts. On
	** the connect statement we will specify no record locking for GET
	** operations and the default (exclusive record locks) for GWT_WRITE
	** operations.
	**
	** This scheme is potentially incompatible with other applications
	** accessing the file. Eventually we will have to let the user specify
	** the locking modes for rms access compatibility with their other
	** applications.
	** 
	** NOTE: Currently, if you are registering the table, we check for
	** READ access and issue a warning if you can't read the file. It might
	** be a nice improvement to examine the registration 'type' (update or
	** noupdate) and change the access type check to write or read
	** depending.
	*/
	/*
	** Actually, GWT_MODIFY wants *no* file access, not even read
	** access, but we open the file for read access and check for
	** read privileges at register table time as a 'favor' to the
	** registering user...
	*/

	if ((gwx_rcb->xrcb_access_mode == GWT_READ) ||
	    (gwx_rcb->xrcb_access_mode == GWT_MODIFY))
	{
	    access_type = ARM$M_READ;
	}
	else
	{
	    access_type = ARM$M_WRITE;
	}
	len = STlcopy(xrel->file_name, (char *)fnam_buf, 255);
	len = STzapblank((char *)fnam_buf, (char *)fnam_buf);

        /*
        ** if vnode is not specified, which can be identified by "::",
        ** then do accessibility check; if vnode is specified, skip this
        ** step since access right check does not cross DECnet (schang)
        */

        partial_name = STindex(fnam_buf, ":", STlength(fnam_buf));
        is_remote = 1;
        if (partial_name == NULL || *(partial_name+1) != ':')
        {
            is_remote = 0;
	    /* Call SYS$CHECK_ACCESS to see if the front end user has sufficient
	    ** protection and/or privilege to open the RMS file for the selected
	    ** type of access.  NOTE:  the username is blank-padded to 24 bytes,
	    ** so at present the username will appear in the log with a lot of
	    ** blanks at the end.  In the future we may wish to fix this.
	    */
            access_objnam.dsc$b_dtype = DSC$K_DTYPE_T;
            access_objnam.dsc$b_class = DSC$K_CLASS_S;
            access_objnam.dsc$a_pointer = &fnam_buf;
            access_objnam.dsc$w_length = STlength(fnam_buf);
            access_usrnam.dsc$b_dtype = DSC$K_DTYPE_T;
            access_usrnam.dsc$b_class = DSC$K_CLASS_S;
            access_usrnam.dsc$a_pointer = &(session->gws_username);
            access_usrnam.dsc$w_length = STlength(session->gws_username);
            access_itmlst[0].ile3$w_length = 4;
            access_itmlst[0].ile3$w_code   = CHP$_ACCESS;
            access_itmlst[0].ile3$ps_bufaddr = &access_type;
            access_itmlst[0].ile3$ps_retlen_addr = &access_retlen;
            access_itmlst[1].ile3$w_length = 0;
            access_itmlst[1].ile3$w_code = 0;
            access_itmlst[1].ile3$ps_bufaddr = 0;
            access_itmlst[1].ile3$ps_retlen_addr = 0;

            vms_status = sys$check_access(
    	    		    &access_objtyp,
    			    &access_objnam,
    			    &access_usrnam,
    			    &access_itmlst,
                            NULL, NULL, NULL, NULL); /* new extra args */
	    if (vms_status != SS$_NORMAL)
	    {
	        /*** Report and/or log vms error as diagnostic */
	        gw02_log_chkacc_error(E_GW500F_CHECK_ACCESS_ERROR,
    			vms_status, &access_objtyp, &access_objnam,
    			&access_usrnam,	access_type);
	        gwx_rcb->xrcb_error.err_code = gw02_ingerr(RMS_SYS_CHECK_ACCESS,
					    vms_status,
					    E_GW0104_GWX_VOPEN_ERROR);
	        status = E_DB_ERROR;
	        break;
	    }
        }

	if (tcb->gwt_internal_tcb == NULL)
	{
	    /*
	    ** The gateway specific TCB information has not been allocated.
	    ** For RMS, this information consists of the ADF CX's used to
	    ** convert between RMS and INGRES record formats. At this time we
	    ** need to build such expressions.
	    **
	    ** We start by allocating the RMS specific TCB and the space in
	    ** which we are going to place the ADF CX's. It's unfortunate that
	    ** the gateway allocates memory in this manner since we have to
	    ** acquire it in pagesize chunks we prefer to get it all at one
	    ** time. This implies we have to get enough for all the CX's so we
	    ** allocate something on the order of twice as much as necessary
	    ** for the typical CX.
	    **
	    ** We store the size of this control block in the gateway TCB so it
	    ** can be deallocated by GWF if necessary.
	    **
	    ** THE ADF CX's ARE NOT CURRENTLY CONSTRUCTED. THIS IS A FUTURE
	    ** ENHANCEMENT AS ALL THIS WORK IS CURRENTLY DONE INTERPRETIVELY.
	    */
	    tcb_estimate = rmscx_tcb_estimate(tcb, &error);
	    if (tcb_estimate == 0)
	    {
		gw02_log_error(error.err_code,
			    (struct FAB *)NULL, (struct RAB *)NULL,
			    (ULM_RCB    *)NULL, (SCF_CB *)NULL );
		gwx_rcb->xrcb_error.err_code = E_GW0104_GWX_VOPEN_ERROR;
		status = E_DB_ERROR;
		break;
	    }

	    ulm_rcb = &tcb->gwt_tcb_mstream;
	    ulm_rcb->ulm_psize = tcb_estimate + sizeof(GWRMS_XTCB);
	    if ((status = ulm_palloc(ulm_rcb)) != E_DB_OK)
	    {
		(VOID) gwf_error(ulm_rcb->ulm_error.err_code, GWF_INTERR, 0);
		(VOID) gwf_error(E_GW0314_ULM_PALLOC_ERROR, GWF_INTERR, 0);
		gwx_rcb->xrcb_error.err_code = E_GW0104_GWX_VOPEN_ERROR;
		status = E_DB_ERROR;
		break;
	    }

	    rms_tcb = (GWRMS_XTCB *)ulm_rcb->ulm_pptr;
	    tcb->gwt_internal_tcb = (PTR)rms_tcb;
			
	    /*
	    ** Now build the needed CX's We go ahead and make all these calls
	    ** and check the return codes, etc, but in fact the actual
	    ** functions over in the rmsmap.c file are just stubs. Perhaps one
	    ** day they will become real, and then all this error-checking will
	    ** be useful...
	    */
	    if (rmscx_to_ingres(tcb, &error) != E_DB_OK)
	    {
		gw02_log_error(error.err_code,
			    (struct FAB *)NULL, (struct RAB *)NULL,
			    (ULM_RCB    *)NULL, (SCF_CB *)NULL );
		gwx_rcb->xrcb_error.err_code = E_GW0104_GWX_VOPEN_ERROR;
		status = E_DB_ERROR;
		break;
	    }

	    if (rmscx_from_ingres(tcb, &error) != E_DB_OK)
	    {
		gw02_log_error(error.err_code,
			    (struct FAB *)NULL, (struct RAB *)NULL,
			    (ULM_RCB    *)NULL, (SCF_CB *)NULL );
		gwx_rcb->xrcb_error.err_code = E_GW0104_GWX_VOPEN_ERROR;
		status = E_DB_ERROR;
		break;
	    }

	    if (rmscx_index_format(tcb, &error) != E_DB_OK)
	    {
		gw02_log_error(error.err_code,
			    (struct FAB *)NULL, (struct RAB *)NULL,
			    (ULM_RCB    *)NULL, (SCF_CB *)NULL );
		gwx_rcb->xrcb_error.err_code = E_GW0104_GWX_VOPEN_ERROR;
		status = E_DB_ERROR;
		break;
	    }
	}

	/*
	** Now we allocate and initialize the RMS gateway specific RSB.  This
	** control information is allocated from the memory stream associated
	** with the record stream control block.
	*/
	rsb->gwrsb_rsb_mstream.ulm_psize = sizeof(GWRMS_RSB);
	if (ulm_palloc(&rsb->gwrsb_rsb_mstream) != E_DB_OK)
	{
	    /*** log and return error */
	    gw02_log_error(E_GW0314_ULM_PALLOC_ERROR,
			    (struct FAB *)NULL, (struct RAB *)NULL,
			    (ULM_RCB *)&rsb->gwrsb_rsb_mstream, (SCF_CB *)NULL);
	    if (rsb->gwrsb_rsb_mstream.ulm_error.err_code == E_UL0005_NOMEM)
		gwx_rcb->xrcb_error.err_code = E_GW541B_RMS_INTERNAL_MEM_ERROR;
	    else
		gwx_rcb->xrcb_error.err_code = E_GW0104_GWX_VOPEN_ERROR;
	    status = E_DB_ERROR;
	    break;
	}
	rms_rsb = (GWRMS_RSB *)rsb->gwrsb_rsb_mstream.ulm_pptr;
	rsb->gwrsb_internal_rsb = (PTR)rms_rsb;
        MEfill(sizeof(GWRMS_RSB), 0, rms_rsb);	        
        /*
        ** open a rms file, errors after this call must call file
        ** close to reduce fab usage count
        ** schang : piggyback IIRMS_READONLY flag in gwx_rcb->xrcb_error
        */
        gwx_rcb->xrcb_error.err_code = gwx_rcb->xrcb_xbitset & II_RMS_READONLY;
        status = rms_open_file(((PTR *)gwx_rcb->xrcb_xhandle)[RMSFABMEMIDX],
                               fnam_buf, &rms_rsb->fab, is_remote,
                               &gwx_rcb->xrcb_error);
        if (status != E_DB_OK)
            break;

	/*
	**  Allocate the low and high key buffers here if necessary.  Call
	**  gw02_max_rms_ksz() to find out the maximum key size.
	*/
	ksz = gw02_max_rms_ksz(gwx_rcb);

	if (ksz > 0)
	{
	    rsb->gwrsb_rsb_mstream.ulm_psize = ksz;
	    if (ulm_palloc(&rsb->gwrsb_rsb_mstream) != E_DB_OK)
	    {
		gw02_log_error(E_GW0314_ULM_PALLOC_ERROR,
			       (struct FAB *)NULL,
			       (struct RAB *)NULL,
			       (ULM_RCB *)&rsb->gwrsb_rsb_mstream,
			       (SCF_CB *)NULL);
		if (rsb->gwrsb_rsb_mstream.ulm_error.err_code == E_UL0005_NOMEM)
		    gwx_rcb->xrcb_error.err_code =
			E_GW541B_RMS_INTERNAL_MEM_ERROR;
		else
		    gwx_rcb->xrcb_error.err_code = E_GW0104_GWX_VOPEN_ERROR;
		status = E_DB_ERROR;
		break;
	    }
	    rms_rsb->low_key = rsb->gwrsb_rsb_mstream.ulm_pptr;

	    if (ulm_palloc(&rsb->gwrsb_rsb_mstream) != E_DB_OK)
	    {
		gw02_log_error(E_GW0314_ULM_PALLOC_ERROR,
			       (struct FAB *)NULL,
			       (struct RAB *)NULL,
			       (ULM_RCB *)&rsb->gwrsb_rsb_mstream,
			       (SCF_CB *)NULL );
		if (rsb->gwrsb_rsb_mstream.ulm_error.err_code == E_UL0005_NOMEM)
		    gwx_rcb->xrcb_error.err_code =
			E_GW541B_RMS_INTERNAL_MEM_ERROR;
		else
		    gwx_rcb->xrcb_error.err_code = E_GW0104_GWX_VOPEN_ERROR;
		status = E_DB_ERROR;
		break;
	    }
	    rms_rsb->high_key = rsb->gwrsb_rsb_mstream.ulm_pptr;
	}

	/*
	** Get # of blocks allocated for this file; will be used as an Ingres
	** "page" count for the RMS Gateway table.
	*/
#if 0
        if (db_tab_id->db_tab_index)
            rsb->gwrsb_page_cnt = 1;
        else
#endif
	    rsb->gwrsb_page_cnt = rms_rsb->fab->rmsfab.fab$l_alq;

	/*
	** The sys$connect call has been moved into gw02_position and gw02_put
	** so that we can properly position the file for either scanning or
	** insert. This is driven by the fact that sequential files need to be
	** positioned on a record (the first record) for reads and end-of-file
	** for inserts.  We don't know enough here to make this decision.
    	*/

	/*
	** Now that we have the file opened, we need to allocate a buffer into
	** which to stage reocrds going to/from the rms file.  Of course RMS
	** does not make this a trivial exercise.  The maximum size of a record
	** can be found in FAB$W_MRS in most cases. The exceptions must be
	** handled as follows:
	**
	**	    FAB$W_MRS == 0  maximum record size is unspecified. We will
	**			    allocate a buffer for the physical limit of
	**			    RMS (32767).
	**	    FAB$B_RFM == FAB$C_VFC
	**			    maximum record size is FAB$W_MRS +
	**			    FAB$B_FSZ.  The fixed control area is not
	**			    included in the regular file size indicator
	**			    (FAB$W_MRS).  But max is still 32767.
	**	    FAB$W_BLS > 32767
	**			    There is a possibility that a MAGTAPE may
	**			    have block sizes up to 65535. We will
	**			    ignore this possibility (at least for now).
	*/
	buffer_size = rms_rsb->fab->rmsfab.fab$w_mrs;
	if (buffer_size == 0)
	    buffer_size = 32767;
	if (rms_rsb->fab->rmsfab.fab$w_bls > 32767)
	    buffer_size = rms_rsb->fab->rmsfab.fab$w_bls;
	if (rms_rsb->fab->rmsfab.fab$b_rfm == FAB$C_VFC)
	{                     
	    if (buffer_size + rms_rsb->fab->rmsfab.fab$b_fsz > 32767)
	    {                     
		buffer_size = 32767;
	    }
	    else
	    {
		buffer_size += rms_rsb->fab->rmsfab.fab$b_fsz;
	    }
	}
	rsb->gwrsb_rsb_mstream.ulm_psize = buffer_size;
	if (ulm_palloc(&rsb->gwrsb_rsb_mstream) != E_DB_OK)
	{
	    /*** Log an allocation error and abort */    
	    gw02_log_error(E_GW0314_ULM_PALLOC_ERROR,
			    (struct FAB *)NULL, (struct RAB *)NULL,
			    (ULM_RCB *)&rsb->gwrsb_rsb_mstream, (SCF_CB *)NULL);
	    if (rsb->gwrsb_rsb_mstream.ulm_error.err_code == E_UL0005_NOMEM)
		gwx_rcb->xrcb_error.err_code = E_GW541B_RMS_INTERNAL_MEM_ERROR;
	    else
		gwx_rcb->xrcb_error.err_code = E_GW0104_GWX_VOPEN_ERROR;
	    status = E_DB_ERROR;
	    break;
	}
	rms_rsb->rms_buffer = rsb->gwrsb_rsb_mstream.ulm_pptr;
	rms_rsb->rms_bsize = buffer_size;
	break;
    }
    return(status);
}



/*{
** Name: gw02_close - exit table close
**
** Description:
**	This exit performs table close for a session.
**
**	This procedure closes a specific record stream. As in DMF, files are
**	cached so the TCB's for this file are not deallocated. The internal rsb
**	is deallocated when the GWF rsb memory stream is deallocated.
**
** Inputs:
**	gwx_rcb->
**		xrcb_rsb	record stream control control block
**
** Output:
**	gwx_rcb->
**	    error.err_code	Detailed error code, if error occurs:
**				E_GW0105_GWX_VCLOSE_ERROR
**				E_GW5401_RMS_FILE_ACT_ERROR
**				E_GW5404_RMS_OUT_OF_MEMORY
**				E_GW540C_RMS_ENQUEUE_LIMIT
**				E_GW540A_RMS_FILE_PROT_ERROR
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    General updates to make it work.
**	16-apr-90 (bryanp)
**	    Added improved error handling.
**	23-apr-90 (bryanp)
**	    Asynchronous RMS calls.
**      06-nov-92 (schang)
**          calls to rms_close_file()
**      10-dec-92 (schang)
**          during REMOVE table, call rms_close_by_name()
**      13-jun-96 (schang0
**        integrate 64 changes. 
**        22-dec-93 (schang)
**          if index table, clean up RFA trees also.
**        16-may-96 (schang)
**          pass info to rms_close_file() to close rms file if user
**          start the server with II_RMS_CLOSE_FILE logical set
*/
DB_STATUS
gw02_close
(
    GWX_RCB *gwx_rcb
)
{
    DB_STATUS	    status = E_DB_OK;
    i4	    	    vms_status;
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GWRMS_RSB	    *rms_rsb = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    GW_SESSION      *session = gwx_rcb->xrcb_gw_session;
    GWRMS_XREL	    *xrel = (GWRMS_XREL *)rsb->gwrsb_tcb->gwt_xrel.data_address;
    i4		    save_ifi;
    i4		    sid;
    char            fnam_buf[256];
    i4	            len;
    DB_TAB_ID	    *db_tab_id	    = gwx_rcb->xrcb_tab_id;

    status = E_DB_OK;
    for (;;)
    {
        /*
        ** if access mode is GWT_OPEN_NOACCESS, it must be called from 
        ** gwt_remove, close this
        ** file by special route, close by its name, note that rsb,rms_rsb
        ** are irrelavant if called from REMOVE TABLE
        */
        if (gwx_rcb->xrcb_access_mode == GWT_OPEN_NOACCESS)
        {
	    len = STlcopy(xrel->file_name, (char *)fnam_buf, 255);
	    len = STzapblank((char *)fnam_buf, (char *)fnam_buf);
            status = rms_close_by_name(
                             ((PTR *)gwx_rcb->xrcb_xhandle)[RMSFABMEMIDX],
                             fnam_buf, &gwx_rcb->xrcb_error);
            break;
        }
        /*
        ** return rfa buffer so that if error happens,
        ** buffer memory will not be held.  Only return buffer
        ** if it is readonly (may remove this restriction later)
        ** it has specified to buffer RFA, and current table is 
        ** a index table
        */
        if ((gwx_rcb->xrcb_xbitset & II_RMS_READONLY) &&
                (gwx_rcb->xrcb_xbitset & II_RMS_BUFFER_RFA) &&
                (db_tab_id->db_tab_index))
        {
            status = rms_return_rfa_buffer(session, db_tab_id->db_tab_base,
                          ((PTR *)gwx_rcb->xrcb_xhandle)[RMSRFAMEMIDX]);
        }
	/* if file has not been sys$connected, then don't sys$disconnect. */
	if (rms_rsb->rab.rab$w_isi) /* rab$w_isi is filled in by sys$connect */
	{
	    /* asynchronous RMS: */
	    CSget_sid(&sid);
	    rms_rsb->rab.rab$l_ctx = sid;
	    rms_rsb->rab.rab$l_rop |= RAB$M_ASY;
	    vms_status = rms_rab_suspend(
		    sys$disconnect(&rms_rsb->rab,rms_rab_resume,rms_rab_resume),
		    &rms_rsb->rab);
            
	    if (vms_status != RMS$_NORMAL)
	    {
		/*** Report and/or log vms error as diagnostic */
		gw02_log_error(E_GW5003_RMS_DISCONNECT_ERROR,
			       (struct FAB *)&rms_rsb->fab->rmsfab,
			       (struct RAB *)&rms_rsb->rab,
			       (ULM_RCB *)NULL, (SCF_CB *)NULL );
		gwx_rcb->xrcb_error.err_code = gw02_ingerr(RMS_SYS_DISCONNECT, 
						vms_status,
						E_GW0105_GWX_VCLOSE_ERROR);
		status = E_DB_ERROR;
		break;
	    }
	}   /* end disconnect */
        status = rms_close_file(((PTR *)gwx_rcb->xrcb_xhandle)[RMSFABMEMIDX], 
                                rms_rsb->fab, &gwx_rcb->xrcb_error,
                                (gwx_rcb->xrcb_xbitset & II_RMS_CLOSE_FILE));
            
	break;
    }
    return(status);
}

/*{
** Name: hash_func - hash function determining the hash table entry
**
** Description:
**   sum up the ACII value of a file name.  Divide the sum by number of hash
**   table entry, take the remainder as the return value.
**
** Inputs:
**   fname - null terminated string as file name
**   num   - the divisor
**
** Output:
**      Returns:
**         remainder of the division.
**
** History:
**
**   06-nov-92 (schang)
**     created.
*/

i4 hash_func
(
    char *fname,
    i4   num
)
{
    i4 i, ii;

    for (i = 0, ii = 0; i < STlength(fname); i++)
    {
        ii = ii + fname[i];
    }

    return(ii % num);
}


/*{
** Name: rms_open_file - open rms file by sys$open
**
** Description:
**    get an fab cache, fill in neccessary field then call sys$open().
**    if successful, set up cache buffer for mutiple use.  If fail,
**    report error, return fab cache.
**
** Inputs:
**      xhandle        file cache memory handle
**      fname          file to open
**      isremote       localfile/remote file (accessed through DECnet)
**
** Output:
**      rmsfab          shared open file information
**      error->err_code     E_GW0104_GWX_VOPEN_ERROR if open error.
**
**      Returns:
**         E_DB_OK/E_DB_ERROR.
**
** History:
**
**   06-nov-92 (schang)
**     created.
**   10-jun-93 (schang)
**     read characteristics of keys when open
**   13-jun-96 (schang)
**     integrate 64 changes.
**     27-sep-93 (schang)
**       open file for read only server, this readonly server info piggybacks
**       on argument ERROR.
*/


static DB_STATUS rms_open_file
(
    struct rms_fab_handle *xhandle,
    char                  *fname,
    struct rms_fab_info   **rmsfab,
    i4                    isremote,
    DB_ERROR              *error
)
{
    struct rms_fab_hash_ent *hash_ent;
    i4   hash_id;
    struct rms_fab_info   *fab;
    DB_STATUS	    status;
    i4   vms_status;
    i4   sid;
    i4   hash_func();
    struct rms_fab_info *alloc_fab_info_block();
    VOID   return_fab_info_blk();
    i4   i;
    i4   is_readonly = error->err_code;  /* schang: get this piggyback info */

    error->err_code = 0;
    hash_id = hash_func(fname, xhandle->num_hash_ent);
    hash_ent = xhandle->hash_table + hash_id;

    /* lock this hash ent, then start doing business */

    CSp_semaphore(1, &hash_ent->hash_control); 
 
    for (;;)
    {
        /* search hash table for this file, if it's local */
        status = E_DB_OK;
        if (! isremote)
        {
            struct rms_fab_info *file_in_hash_table();

            *rmsfab = fab = file_in_hash_table(hash_ent, fname);
            if (fab != NULL)
            {
                fab->count++;
                break;
            }
        }

        /*
        ** the file to be open is either not already open or 
        ** is a remote file, start a new fab.
        */

        *rmsfab = fab = alloc_fab_info_block(xhandle, hash_id);
        if (fab == NULL)
        {
            /* out of cache space, create a new error for it */
            status = E_DB_ERROR;
            break;
        }
    
        /* fill up RMSFAB */

        fab->rmsfab.fab$b_bid = FAB$C_BID;
        fab->rmsfab.fab$b_bln = FAB$C_BLN;
        fab->rmsfab.fab$b_rfm = FAB$C_RFM_DFLT;
        STcopy(fname, fab->fname);
        fab->rmsfab.fab$l_fna = fab->fname;
        fab->rmsfab.fab$b_fns = STlength(fab->fname);
        
        /*
        ** You won't find it in the manual but this field must be zero to use
        ** multiple fab's on a file within the server.
        */
        fab->rmsfab.fab$w_ifi = 0;
        fab->rmsfab.fab$b_shr = FAB$M_SHRPUT | FAB$M_SHRGET 
                          | FAB$M_SHRDEL | FAB$M_SHRUPD;
        if (!isremote)
            fab->rmsfab.fab$b_shr = fab->rmsfab.fab$b_shr | FAB$M_MSE;
 
        /*
        ** schang : if server is readonly, open file for read, only
        */
        if (!is_readonly)
            fab->rmsfab.fab$b_fac = FAB$M_DEL | FAB$M_GET 
                          | FAB$M_PUT | FAB$M_UPD;
        else
            fab->rmsfab.fab$b_fac = FAB$M_GET; 

        /* asynchronous RMS: */
        CSget_sid(&sid);
        fab->rmsfab.fab$l_ctx = sid;

#ifdef	FAB$M_ASY
        /* our <fab.h> doesn't contain this. Do the open synchronously */
        fab->rmsfab.fab$l_fop |= FAB$M_ASY;
#endif

        /* open RMS file */
        vms_status = rms_fab_suspend(
                             sys$open(&fab->rmsfab,
                                      rms_fab_resume,
                                      rms_fab_resume
                                     ),
                             &fab->rmsfab);
        
        if (vms_status != RMS$_NORMAL)
        {
            /*** Report and/or log vms error as diagnostic */
            gw02_log_error(E_GW5000_RMS_OPEN_ERROR,
			    (struct FAB *)&fab->rmsfab,
			    (struct RAB *)NULL, (ULM_RCB *)NULL, 
			    (SCF_CB *)NULL );
            error->err_code = gw02_ingerr(RMS_SYS_OPEN, vms_status,
					    E_GW0104_GWX_VOPEN_ERROR);
            status = E_DB_ERROR;

            /* since error, we have to return this fab_info allocation */
            return_fab_info_blk(xhandle, fab);
            *rmsfab = NULL;
        }
        else 
        {
            /* all is well, insert this into hash entry */
            fab->next = hash_ent->hash_entry;
            hash_ent->hash_entry = fab;
            fab->hash_id = hash_id;
            fab->count = 1;
            fab->is_remote = isremote;
            /*
            ** for index file, we need to find how many indices
            ** and what are the indices characteristics - schang
            */ 
            if (fab->rmsfab.fab$b_org == FAB$C_IDX)
            {
                struct XABSUM xabsum;

                xabsum.xab$b_bln = 12;    /* XAB$C_SUMLEN */ 
                xabsum.xab$b_cod = 22;    /* XAB$C_SUM    */
                xabsum.xab$l_nxt = NULL;
                fab->rmsfab.fab$l_xab = &xabsum;
                vms_status = rms_fab_suspend(
                                 sys$display(&fab->rmsfab,
                                             rms_fab_resume,
                                             rms_fab_resume
                                            ),
                                 &fab->rmsfab);
        
                if (vms_status != RMS$_NORMAL)
                {
                    /*** Report and/or log vms error as diagnostic */
                    gw02_log_error(E_GW5000_RMS_OPEN_ERROR,
		        	    (struct FAB *)&fab->rmsfab,
			            (struct RAB *)NULL, (ULM_RCB *)NULL, 
			            (SCF_CB *)NULL );
                    error->err_code = gw02_ingerr(RMS_SYS_OPEN, vms_status,
					          E_GW0104_GWX_VOPEN_ERROR);
                    status = E_DB_ERROR;

                    /* since error, we have to return this fab_info allocation */
                    return_fab_info_blk(xhandle, fab);
                    *rmsfab = NULL;
                }
                else
                {
                    /*
                    ** schang : get characteristics of all keys
                    */
                    
                    for (i= 0; i < xabsum.xab$b_nok && i <RMSMAXKEY; i++)
                    {
                        fab->rmskey[i].xab$b_ref = i;
                        fab->rmskey[i].xab$b_cod = XAB$C_KEY;
                        fab->rmskey[i].xab$b_bln = 100; /* ingres define this one different
                                            ** from what rms defines */
                        if (i != xabsum.xab$b_nok - 1)
                            fab->rmskey[i].xab$l_nxt = &fab->rmskey[i+1];
                        else
                            fab->rmskey[i].xab$l_nxt = NULL;
                    }
                    fab->rmsfab.fab$l_xab = &fab->rmskey[0];
                    vms_status = rms_fab_suspend(
                                        sys$display(&fab->rmsfab,
                                                    rms_fab_resume,
                                                    rms_fab_resume
                                                   ),
                                        &fab->rmsfab);
        
                    if (vms_status != RMS$_NORMAL)
                    {
                        /*** Report and/or log vms error as diagnostic */
                        gw02_log_error(E_GW5000_RMS_OPEN_ERROR,
            	        	       (struct FAB *)&fab->rmsfab,
			               (struct RAB *)NULL, (ULM_RCB *)NULL, 
			               (SCF_CB *)NULL );
                        error->err_code = gw02_ingerr(RMS_SYS_OPEN, vms_status,
					              E_GW0104_GWX_VOPEN_ERROR);
                        status = E_DB_ERROR;

                    /* since error, we have to return this fab_info allocation */
                        return_fab_info_blk(xhandle, fab);
                        *rmsfab = NULL;
                    }
                }
            }
        }
        break;
    }
    CSv_semaphore(&hash_ent->hash_control);
    return(status);
}

/*{
** Name: rms_close_by_name - close rms file by sys$close  using file name
**
** Description:
**      If file is open through DECNet, call sys$close and then remove this
**      file entry from hash table.  If it is a local file, just decrement its
**      use count.
**
** Inputs:
**      xhandle        file cache memory handle
**      fname           name of file to be closed
**
** Output:
**      error->err_code     E_GW0105_GWX_VCLOSE_ERROR if error in sys$close
**
**      Returns:
**         E_DB_OK/E_DB_ERROR.
**
** History:
**
**   19-nov-92 (schang)
**     created.  This is currently only called by REMOVE TABLE
*/
static DB_STATUS rms_close_by_name
(
    struct rms_fab_handle *xhandle,
    char                  *fname,
    DB_ERROR              *error
)
{    
    struct rms_fab_hash_ent *hash_ent;
    struct rms_fab_info     *curr, *prev;
    DB_STATUS	            status = E_DB_OK;
    i4		            vms_status;
    i4                      hash_id;
    i4                      sid;
    i4                      save_ifi;
    i4                      hash_func();
    VOID                    return_fab_info_blk();

    hash_id = hash_func(fname, xhandle->num_hash_ent);
    hash_ent = xhandle->hash_table + hash_id;

    /* lock this hash ent, then start doing business */

    CSp_semaphore(1, &hash_ent->hash_control);

    curr = hash_ent->hash_entry;
    while (curr != NULL)
    {
        if (STcompare(curr->fname, fname) != 0)
        {
            prev = curr;
            curr = curr->next;
        }
        else
        {
            if (curr->count == 0)    /* no more table associated with file */
            {
 
                if (curr == hash_ent->hash_entry)
                {
                    hash_ent->hash_entry = curr->next;
                }
                else
                {
                    prev->next= curr->next;
                }
 	        save_ifi = curr->rmsfab.fab$w_ifi;

                /* asynchronous RMS: */
	        CSget_sid(&sid);
	        curr->rmsfab.fab$l_ctx = sid;
#ifdef	FAB$M_ASY
	        /* our <fab.h> doesn't contain this. Do the close synchronously */
	        curr->rmsfab.fab$l_fop |= FAB$M_ASY;
#endif

	        vms_status = rms_fab_suspend(
		    sys$close(&curr->rmsfab, rms_fab_resume, rms_fab_resume),
		    &curr->rmsfab);
            
	        if (vms_status != RMS$_NORMAL)
	        {
	        /*** Report and/or log vms error as diagnostic */
	            curr->rmsfab.fab$w_ifi = save_ifi;

	            gw02_log_error(E_GW5001_RMS_CLOSE_ERROR,
			    (struct FAB *)&curr->rmsfab,
			    (struct RAB *)NULL, (ULM_RCB *)NULL, 
			    (SCF_CB *)NULL );
	            error->err_code = gw02_ingerr(RMS_SYS_CLOSE, 
					    vms_status,
					    E_GW0105_GWX_VCLOSE_ERROR);
	            status = E_DB_ERROR;

	        }
                /*
                **  return to free list,
                **  no matter if sys$close is successful
                */
                return_fab_info_blk(xhandle, curr);
            }
            break;
        }
    }
    CSv_semaphore(&hash_ent->hash_control);
    return(status);
}


/*{
** Name: rms_close_file - close rms file by sys$close
**
** Description:
**      If file is open through DECNet, call sys$close and then remove this
**      file entry from hash table.  If it is a local file, just decrement its
**      use count.   16-may-96, if user set the logical II_RMS_CLOSE_FILE  at
**       startup time and use count goes down to 0, close the file also.
**                
**
** Inputs:
**      xhandle        file cache memory handle
**      fab            shared open file information pointer
**
** Output:
**      error->err_code     E_GW0105_GWX_VCLOSE_ERROR if error in sys$close
**
**      Returns:
**         E_DB_OK/E_DB_ERROR.
**
** History:
**
**   06-nov-92 (schang)
**     created.
**   13-jun-96 
**     integrate 64 changes.  close file if ii_rms_close_file defined
*/

static DB_STATUS rms_close_file
(
    struct rms_fab_handle *xhandle,
    struct rms_fab_info   *fab,
    DB_ERROR              *error,
    bool                  close_file
)
{
    i4 hash_id = fab->hash_id;
    struct rms_fab_hash_ent *hash_table = xhandle->hash_table;
    struct rms_fab_hash_ent *hash_ent;
    DB_STATUS status = E_DB_OK;
    VOID return_fab_info_blk();

    hash_ent = hash_table + hash_id;

    /* lock this hash ent, then start doing business */

    CSp_semaphore(1, &hash_ent->hash_control); 
    if (!fab->is_remote)
    {
        fab->count--;
    }
    /*
    ** we always close remote file, but we also close local file 
    ** if server is started with II_RMS_CLOSE FILE and use count
    ** go down to 0
    */
    if (fab->is_remote || ((close_file) && (fab->count == 0)))
    {
        struct rms_fab_info   *curr, *prev;

        curr = hash_ent->hash_entry;

        while (curr != NULL)
        {
            if (curr != fab)
            {
                prev = curr;
                curr = curr->next;
            }
            else
            {
                if (curr == hash_ent->hash_entry)
                {
                    hash_ent->hash_entry = curr->next;
                }
                else
                {
                    prev->next= curr->next;
                }
                break;
            }
        }
        if (curr != NULL)
        {
	/*
	** sys$close clears the Internal File Identifier. If there is an error
	** in the close, we'd like to report that file-id. It may not be of
	** much use, but since it's the only thing that identifies which file
	** failed to close, it's worth this little bit of extra ugliness.
	*/
            i4 sid;
            i4 save_ifi;
            i4 vms_status;
 
 	    save_ifi = fab->rmsfab.fab$w_ifi;

	    /* asynchronous RMS: */
	    CSget_sid(&sid);
	    fab->rmsfab.fab$l_ctx = sid;
#ifdef	FAB$M_ASY
	    /* our <fab.h> doesn't contain this. Do the close synchronously */
	    fab->rmsfab.fab$l_fop |= FAB$M_ASY;
#endif

	    vms_status = rms_fab_suspend(
		    sys$close(&fab->rmsfab, rms_fab_resume, rms_fab_resume),
		    &fab->rmsfab);
            
	    if (vms_status != RMS$_NORMAL)
	    {
	    /*** Report and/or log vms error as diagnostic */
	        fab->rmsfab.fab$w_ifi = save_ifi;

	        gw02_log_error(E_GW5001_RMS_CLOSE_ERROR,
			    (struct FAB *)&fab->rmsfab,
			    (struct RAB *)NULL, (ULM_RCB *)NULL, 
			    (SCF_CB *)NULL );
	        error->err_code = gw02_ingerr(RMS_SYS_CLOSE, 
					    vms_status,
					    E_GW0105_GWX_VCLOSE_ERROR);
	        status = E_DB_ERROR;

	    }
            /*
            **  unchain this fab and return to free list,
            **  no matter if sys$close is successful
            */
            return_fab_info_blk(xhandle, fab);
        }
    }
    CSv_semaphore(&hash_ent->hash_control);
    return(status);
}


/*{
** Name: file_in_hash_table - find if the file in mind is already open and 
**           listed in its perspective hash table entry
**
** Description:
**      search the file in the given hash table entry, if found but
**      not at the first position of the list, move it to first position.
**      The assumption is that a file once opened, may be opened again soon.
**
** Inputs:
**      hash_ent       pointer to a hash table entry
**      fname          file to be looked for
**
** Output:
**      Returns:
**         pointer to rms_fab_info structure or NULL.
**
** History:
**
**   06-nov-92 (schang)
**     created.
*/

static struct rms_fab_info *file_in_hash_table
(
    struct rms_fab_hash_ent *hash_ent,
    char *fname
)
{
    struct rms_fab_info *fab_entry = hash_ent->hash_entry;
    struct rms_fab_info *prev;

    while (fab_entry != NULL)
    {
        if (STcompare(fab_entry->fname, fname) != 0)
        {
            prev = fab_entry;
            fab_entry = fab_entry->next;
        }
        else  /* found the file, move it to be first in the list */
        {
            if (fab_entry != hash_ent->hash_entry)
            {
                prev->next = fab_entry->next;
                fab_entry->next = hash_ent->hash_entry;
                hash_ent->hash_entry = fab_entry;
            }
            break;
        }
    }
    return(fab_entry);    
}


/*{
** Name: sweep_hash_table - free files in hash table that are not in use.
**
** Description:
**      If rms_fab_info memory are exhausted and we need more of them, server
**      calls this routine to free those files that are not in use and that
**      are not in the same hash table entry as the one we want to open, we
**      already lock this hash table entry.
**
** Inputs:
**      handle      pointer to structure that controls file cache memory.
**      hash_id     hash table entry that is locked.
**
** Output:
**      Returns:
**         number of cache being deallocated..
**
** History:
**
**   06-nov-92 (schang)
**     created.
*/

static i4 sweep_hash_table
(
    struct rms_fab_handle *handle,
    i4             hash_id
)
{
    struct rms_fab_hash_ent *hash_ent;
    i4  i, retval;
    struct rms_fab_info *fab_entry, *temp, *free_fab;
    VOID return_fab_info_blk();

    for (i = 0, retval = 0; (i < handle->num_hash_ent); i++)
    {    if (i != hash_id)
         {
             hash_ent = handle->hash_table + i;
             CSp_semaphore(1, &hash_ent->hash_control);
             fab_entry = hash_ent->hash_entry;
             while (fab_entry != NULL)
             {
                 if (fab_entry->count != 0)
                 {
                     temp = fab_entry;
                     fab_entry = fab_entry->next;
                 }
                 else 
                 {
                     free_fab = fab_entry;
                     if (fab_entry == hash_ent->hash_entry)
                     {
                         hash_ent->hash_entry = fab_entry->next;
                     }
                     else 
                     {
                         temp->next = fab_entry->next;
                     }
                     fab_entry = fab_entry->next;
                     sys$close(&free_fab->rmsfab);
                     return_fab_info_blk(handle, free_fab);
                     retval++;
                 }
             }
             CSv_semaphore(&hash_ent->hash_control);
         }
     }
     return(retval);
}


/*{
** Name: alloc_fab_info_block
**
** Description:
**      allocate data structure for a file to be opened
** Inputs:
**      handle      pointer to structure that controls file cache memory.
**      hash_id     hash table entry that is locked, if no more memory, this
**                  one is needed to guarantee that we don't free up fab cache
**                  in this hash table entry (it will dead lock.)
**
** Output:
**      Returns:
**         memory pointer to the rms_fab_info data structure, or NULL if
**         none available.
**
** History:
**
**   06-nov-92 (schang)
**     created.
*/


static struct rms_fab_info *alloc_fab_info_block
(
  struct rms_fab_handle *handle,
  i4             hash_id
)
{
    struct rms_fab_info *retval;
    i4  number;

    CSp_semaphore(1, &handle->free_fab_control);
    for (;;)
    {
        if (handle->free_fabs == NULL)
        {
            number = sweep_hash_table(handle, hash_id);
            if (number == 0)
            {
                retval = NULL;
                break;
            }
        }
        retval = handle->free_fabs;
        handle->free_fabs = handle->free_fabs->next;
        MEfill(sizeof(struct rms_fab_info), 0, (PTR)retval);
        break; 
    }
    CSv_semaphore(&handle->free_fab_control);
    return(retval);
}


/*{
** Name: return_fab_info_blk
**
** Description:
**      put an available rms_fab_info memory back to free list
** Inputs:
**      handle      pointer to structure that controls file cache memory.
**      fab         memory to be put back.
**
** Output:
**      Returns:
**        nothing.
**
** History:
**
**   06-nov-92 (schang)
**     created.
*/

static VOID return_fab_info_blk
(
struct rms_fab_handle *handle,
struct rms_fab_info   *fab
)
{
    CSp_semaphore(1, &handle->free_fab_control);
    if (handle->free_fabs == NULL)
    {
        handle->free_fabs = fab;
        fab->next = NULL;
    }
    else
    {
        fab->next = handle->free_fabs;
        handle->free_fabs = fab;
    }
    CSv_semaphore(&handle->free_fab_control);
    return;
}


/*{
** Name: rms_fab_cache_init
**
** Description:
**      called at server startup time to set up file cache memory pool.
** Inputs:
**      None
**
** Output:
**      ptr     -  memory pointer pointing to all memories allocated.
**      error   -  error information from SCF
**           
**      Returns:
**        E_DB_OK/E_DB_ERROR.
**
** History:
**
**   06-nov-92 (schang)
**     created.
**   16-feb-93 (schang)
**     remove extra arg in STcopy
*/

static DB_STATUS rms_fab_cache_init
(
    PTR      *ptr,
    DB_ERROR *error
)
{
    DB_STATUS	status;
    i4   num_hash_ent;
    i4   num_fab_cache;
    struct rms_fab_handle *top_handle;
    struct rms_fab_info *fab_entry;
    struct rms_fab_hash_ent *hash_elem;
    i4   i;
    char *string;
    char num_str[32];

    /*
    **  first, allocate the top level handler, this the ptr to return
    */
    status = op_scf_mem(SCU_MALLOC,
                 (PTR)&top_handle,
                 sizeof(struct rms_fab_handle),
                 error);
    if (status != E_DB_OK)
       return(status);
    MEfill(sizeof(struct rms_fab_handle), 0, (PTR)top_handle);

    CSi_semaphore(&top_handle->free_fab_control, CS_SEM_SINGLE);

    /*
    **  then, allocate hash table, get number of hash table entry from
    **  defined logicals
    */
    PMget("ii.$.rms.*.ii_rms_hash_size",&string);
    STcopy(string, num_str);
    CVan(num_str, &top_handle->num_hash_ent); 
    status = op_scf_mem(SCU_MALLOC,
                 (PTR)&top_handle->hash_table,
                 sizeof(struct rms_fab_hash_ent)*top_handle->num_hash_ent,
                 error);
    if (status != E_DB_OK)
       return(status);
    MEfill(sizeof(struct rms_fab_hash_ent)*top_handle->num_hash_ent,
           0, (PTR)top_handle->hash_table);

    hash_elem = top_handle->hash_table;
    for (i = 0; i < top_handle->num_hash_ent; i++, hash_elem++)
    {
         CSi_semaphore(&hash_elem->hash_control, CS_SEM_SINGLE);
    }

    /*
    **  now, allocate fab cache, get number of cache elements from 
    **  defined logical
    */
    PMget("ii.$.rms.*.ii_rms_fab_size",&string);
    STcopy(string, num_str);
    CVan(num_str, &top_handle->num_fab_cache); 
    status = op_scf_mem(SCU_MALLOC,
                  (PTR)&top_handle->fab_cache,
                  sizeof(struct rms_fab_info)*top_handle->num_fab_cache,
                  error);
    if (status != E_DB_OK)
       return(status);

    fab_entry = top_handle->free_fabs = top_handle->fab_cache;
    i = 0;
    while (i < top_handle->num_fab_cache - 1)
    {
        fab_entry->next = fab_entry + 1;
        i++;
        fab_entry++;
    }
    fab_entry->next = NULL;


    *ptr = (PTR)top_handle;
    return(status);
}


/*{
** Name: rms_fab_cache_term
**
** Description:
**      called at server termination time to free file cache memory pool.
** Inputs:
**      memptr  - memory pointer pointing to all memories allocated for
**                file information caching.
**
** Output:
**      error   -  error information from SCF
**           
**      Returns:
**        E_DB_OK/E_DB_ERROR.
**
** History:
**
**   06-nov-92 (schang)
**     created.
*/

static DB_STATUS rms_fab_cache_term
(
    PTR      memptr,
    DB_ERROR *error
)
{
    struct rms_fab_handle *top_handle;
    struct rms_fab_hash_ent *hash_ent;
    struct rms_fab_info *fab_entry, *temp, *free_fab;
    DB_STATUS status;
    i4  i;

    top_handle = (struct rms_fab_handle *)memptr;

    /*
    ** first close all open files and free fabs
    */
    for (i = 0; (i < top_handle->num_hash_ent); i++)
    {
         hash_ent = top_handle->hash_table + i;
         CSp_semaphore(1, &hash_ent->hash_control);
         fab_entry = hash_ent->hash_entry;
         while (fab_entry != NULL)
         {
             free_fab = fab_entry;
             fab_entry = fab_entry->next;
             sys$close(&free_fab->rmsfab);
             return_fab_info_blk(top_handle, free_fab);
         }    
         CSv_semaphore(&hash_ent->hash_control);
     }

    /* then, free fab cache memory, hash table,
    ** finally free top level handle
    */
    status = op_scf_mem(SCU_MFREE,
                 (PTR)&top_handle->hash_table,
                 sizeof(struct rms_fab_hash_ent)*top_handle->num_hash_ent,
                 error);
    if (status != E_DB_OK)
       return(status);

    status = op_scf_mem(SCU_MFREE,
                  (PTR)&top_handle->fab_cache,
                  sizeof(struct rms_fab_info)*top_handle->num_fab_cache,
                  error);
    if (status != E_DB_OK)
       return(status);

    status = op_scf_mem(SCU_MFREE,
                 (PTR)&top_handle,
                 sizeof(struct rms_fab_handle),
                 error);
    return(status);
}

static bool rms_key_asc
(
    struct rms_fab_info   *rmsfab,
    i4  key
)
{
#define XAB$C_MAX_ASCEND 8
    if (rmsfab->rmskey[key].xab$b_dtp <= XAB$C_MAX_ASCEND)
        return(TRUE);
    return(FALSE);
}


/*{
** Name: op_scf_mem
**
** Description:
**      this routine consolidate memory operation, SCU_MALLOC, SCU_FREE
** Inputs:
**      memptr  - for free operation, memory to be freed.
**      size    - the amount of memory to be allocated/freed.
**
** Output:
**      memptr  - for allocate operation, memory allocated
**      error   -  error information from SCF
**          ->err_code
**	            E_GW0600_NO_MEM;
**		    E_GW0200_GWF_INIT_ERROR;
**           
**      Returns:
**        E_DB_OK/E_DB_ERROR.
**
** History:
**
**   06-nov-92 (schang)
**     created.
*/

static DB_STATUS op_scf_mem
(
    int      op,
    PTR      *memptr,
    long     size,
    DB_ERROR *error
)
{
    SCF_CB scf_cb;
    DB_STATUS	status;

    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_scm.scm_in_pages = (size/SCU_MPAGESIZE+1);
    switch (op)
    {
        case SCU_MFREE :
            scf_cb.scf_scm.scm_addr = (char *)(*memptr);
            break;
        case SCU_MALLOC :
            scf_cb.scf_scm.scm_functions = 0;
            break;
        default :
            return(E_DB_ERROR);
    }

    if ((status = scf_call(op, &scf_cb)) != E_DB_OK)
    {
        gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
        if (op == SCU_MALLOC)
        {
            gwf_error(E_GW0300_SCU_MALLOC_ERROR, GWF_INTERR, 1,
		      sizeof(scf_cb.scf_scm.scm_in_pages),
		      &scf_cb.scf_scm.scm_in_pages);

	    switch (scf_cb.scf_error.err_code)
	    {
	        case E_SC0004_NO_MORE_MEMORY:
	        case E_SC0005_LESS_THAN_REQUESTED:
	        case E_SC0107_BAD_SIZE_EXPAND:
	            error->err_code = E_GW0600_NO_MEM;
		    break;

	        default:
		    error->err_code = E_GW0200_GWF_INIT_ERROR;
		    break;
	    }
        }
        else
            error->err_code = E_GW0200_GWF_INIT_ERROR;

    }
    if (op == SCU_MALLOC && status == E_DB_OK)
       *memptr = (PTR)scf_cb.scf_scm.scm_addr;
    else
       *memptr = NULL;
    return(status);
}


/*{
** Name: gw02_position - exit record stream position
**
** Description:
**	This exit establishes a record stream.  This request must precede other
**	record stream operations, except gw02_put().  A gw02_open() request
**	must have preceded this request.
**
**	A range of key values can be defined if the table is indexed; else it
**	must be a complete scan.
**
**	The TCB indicates this is an access via an index if db_tab_index != 0.
**	In this case, the kays are presented in var_data1 and var_data2. The
**	keys are in INGRES format and must be converted before being presented
**	to the foreign data manager. The caller may specify either one or both
**	of these key values. A key is not specified if the address pointer of
**	the parameter is NULL.
**
**	Note that the index description has been attached to the GW_TCB at the
**	time this record stream was opened.
**
**	The exit is expected to save the pertinent information for future
**	record stream manipulations.
**
** Inputs:
**	gwx_rcb->
**		rsb		record stream control block for this access
**		var_data1	lower key of the index range; null indicates
**				beginning-of-file positioning
**		var_data2	higher key of the index range; null indicates
**				end-of-file positioning
**
** Output:
**	gwx_rcb->
**	    error.err_code	Detailed error code, if error occurred:
**				E_GW0106_GWX_VPOSITION_ERROR
**				E_GW0641_END_OF_STREAM
**				E_GW5401_RMS_FILE_ACT_ERROR
**				E_GW540D_RMS_DEADLOCK
**				E_GW5404_RMS_OUT_OF_MEMORY
**				E_GW540C_RMS_ENQUEUE_LIMIT
**				E_GW540E_RMS_ALREADY_LOCKED
**				E_GW540F_RMS_PREVIOUSLY_DELETED
**				E_GW5410_RMS_READ_LOCKED_RECORD
**				E_GW5411_RMS_RECORD_NOT_FOUND
**				E_GW5412_RMS_READ_AFTER_WAIT
**				E_GW5413_RMS_RECORD_LOCKED
**				E_GW5414_RMS_TIMED_OUT
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    Create a working version (sequential scan only)
**	16-apr-90 (bryanp)
**	    Enhancements to error handling.
**	23-apr-90 (bryanp)
**	    Asynchronous RMS calls.
**	11-jul-90 (linda)
**	    Make primary key access work properly.  Call new routine,
**	    gw02_xlate_key(), to translate Ingres key values provided into RMS
**	    key values.  Also call new ADF routine to determine if low key
**	    value == minimum or null for its type and/or if high key value ==
**	    maximum or null for its type; if so, treat them as though no low
**	    (or high, or either) key was provided and set limits accordingly.
**	25-jan-91 (Rickh)
**	    Set timeout period for the RMS find.
**	20-feb-91 (linda)
**	    Add logic to do a rewind when repositioning the same file.  This is
**	    detected by noting whether the internal stream id is non-zero.
**	    Also temporarily reset key-of-reference from -1 to 0 in this case
**	    and then set it back again afterwards.  This is only done if we are
**	    using keyref -1 (i.e. nonkeyed access); it is an artifact of how
**	    sys$rewind() works.
**	2-mar-91 (rickh)
**	    Don't report EOF errors on finds.  Just report an end-of-stream
**	    condition to DMF.
**	6-aug-91 (rickh)
**	    Extracted the final sys$find agony to positionRecordStream
**	    so that it can be shared with gw02_replace and gw02_delete.
**	20-sep-91 (rickh)
**	    Always rewind when repositioning the same file.  This is because
**	    consecutive sys$finds skip records, not the intent of the QEF
**	    positioning calls.
**      17-mar-94 (schang)
**          Several problems here:
**              1. the field tbl_storage_type is not initialized in gwu_gettcb.
**                 If it is 3 (DMT_HEAP_TYPE), when table is indexed, we get
**                 into "spin" as described by Intel.  If it isnot 3 but table
**                 is sequential, although incorrect, there is no bad effect.
**              2. If table type is not HEAP, then regardless the access type
**                 being sequential or indexed we must position by key,
**                 otherwise it is possible that if we explicitly query an
**                 indexed table, either values are not sorted or incorrect
**                 number of rows returned.  Thus testing tlb_storage_type
**                 only is sufficient to determine if table is keyed. 
*/
DB_STATUS
gw02_position
(
    GWX_RCB *gwx_rcb
)
{                   
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_TCB	    *tcb = rsb->gwrsb_tcb;
    GWRMS_RSB	    *rms_rsb = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    DMT_TBL_ENTRY   *tbl = (DMT_TBL_ENTRY *)&tcb->gwt_table;
    GWRMS_XIDX	    *idx;
    i4		    sid;
    bool	    repos;
    bool	    keyed;
    DB_STATUS	    status = E_DB_OK;
    i4	    	    vms_status;
    unsigned char   saved_krf;
    i4		    saved_key_ref;

    for (;;)
    {
	/* rab$w_isi is filled in by sys$connect */
	repos = (rms_rsb->rab.rab$w_isi != 0);
        /*
        ** schang :
	**    
        **    testing tbl_storage_type is wrong, since
        **    this field is never initalized
        */
        keyed = (tbl->tbl_storage_type != DMT_HEAP_TYPE);
        /*
        ** schang : here we must provide key-of-reference for RMS to
        ** establish the index of the next record pointer.  Otherwise,
        ** we run into B56756.
        */
        if (keyed)
        {
	    if (!(rms_rsb->fab->rmsfab.fab$b_org == FAB$C_IDX))
	    {
                /*
		** Error, this is not an indexed file.  Return the appropriate
		** error code to the caller.
		*/
		gw02_log_error(E_GW500B_RMS_FILE_NOT_INDEXED,
		    	       (struct FAB *)&rms_rsb->fab->rmsfab,
			       (struct RAB *)NULL, (ULM_RCB *)NULL, 
			       (SCF_CB *)NULL);
		gwx_rcb->xrcb_error.err_code = E_GW0106_GWX_VPOSITION_ERROR;
                status = E_DB_ERROR;
		break;
	    }

	    if (gwx_rcb->xrcb_tab_id->db_tab_index == 0)    /* base table */
	    {
		rms_rsb->rab.rab$b_krf = 0;
		rms_rsb->key_ref = 0;
	    }
	    else
	    {
		idx = (GWRMS_XIDX *)tcb->gwt_idx->gwix_xidx.data_address;
		rms_rsb->rab.rab$b_krf = idx->key_ref;
		rms_rsb->key_ref = idx->key_ref;
	    }
        }
        else
        {
            rms_rsb->rab.rab$b_krf = 0;
	    rms_rsb->key_ref = 0;
	} /* end if (keyed) ... else... */

	if (!repos) /* i.e. we haven't connected yet */
        {
	    status = gw02_connect(gwx_rcb, GWX_VPOSITION);
	    if (status != E_DB_OK)
	    {
		gwx_rcb->xrcb_error.err_code = E_GW0106_GWX_VPOSITION_ERROR;
		break;
	    }
        }
        else
        {
	    vms_status = rms_rab_suspend(
	        sys$rewind(&rms_rsb->rab, rms_rab_resume,
				       rms_rab_resume), &rms_rsb->rab);
	    switch (vms_status)
	    {
		case RMS$_NORMAL:
		    status = E_DB_OK;
		    break;
		default:
		    gw02_log_error(E_GW5012_RMS_REWIND_ERROR,
				       (struct FAB *)&rms_rsb->fab->rmsfab,
				       (struct RAB *)&rms_rsb->rab,
				       (ULM_RCB *)NULL, (SCF_CB *)NULL);
		    gwx_rcb->xrcb_error.err_code = 
				    gw02_ingerr(RMS_SYS_REWIND, vms_status,
						E_GW0106_GWX_VPOSITION_ERROR);
		    status = E_DB_ERROR;
		    break;
	    }
	    if (status != E_DB_OK)
		break;
        }

        /*
        ** we have done with establishing the first position
        ** in the file, next we position to desired location
        ** by using sys$find.  Now the record access mode
        ** rab$b_rac determine sequential scan or index probe.
        ** It is possible to do sequential scan on an indexed
        ** file if a. table is registered as sequential (HEAP),
        ** this is determined by the flag keyed;
        ** or b. the access is sequential, such as select *
        ** on an indexed table, this is determined by the absence
        ** of low key AND high key.
        */
	if (!keyed)
	{
	    /*
	    ** Heap structure implies we are doing a sequential
	    ** scan.
	    */
	    rms_rsb->rab.rab$b_rac = RAB$C_SEQ;
	    rms_rsb->rab.rab$b_krf = -1;
	    rms_rsb->key_ref = -1;
	    rms_rsb->rab.rab$l_kbf = (char *)NULL;

	}
	else
	{
	    /*
	    ** The keys must be converted to RMS format at this point.  To do
	    ** this, we need to know which columns are key columns, and put
	    ** them together in a key buffer.  For Ingres text and varchar
	    ** types, this means getting rid of the 2-byte length indicator
	    ** also, and padding (null char or blank) to full key column
	    ** length.  We don't make any assumptions about how many of what
	    ** type columns can make up an RMS segmented key; just do as many
	    ** as indicated.
	    */

	    if ((status = gw02_xlate_key(gwx_rcb)) != E_DB_OK)
	    {
		/* NOTE:  detailed error was logged in gw02_xlate_key */
		gwx_rcb->xrcb_error.err_code = E_GW0106_GWX_VPOSITION_ERROR;
		break;
	    }

	    /*
	    ** If a low key value is supplied, use it to set lower bound for
	    ** RMS file access and wait until get to use high key.  If no low
	    ** key is supplied but a high key is supplied, go ahead and set up
	    ** RMS file access with high key as limit.  If neither low nor high
	    ** key is supplied, set up sequential scan.
	    */
	    switch (rms_rsb->low_key_len)
	    {
		case 0:	    /* no low key */
		{
		    switch (rms_rsb->high_key_len)
		    {
			case 0:	    /* no keys */
			    rms_rsb->rab.rab$l_rop &= ~(RAB$M_KGE | RAB$M_LIM);
			    rms_rsb->rab.rab$b_rac = RAB$C_SEQ;
			    break;

			default:    /* high key only */
			    rms_rsb->rab.rab$b_ksz = rms_rsb->high_key_len;
			    rms_rsb->rab.rab$l_kbf = rms_rsb->high_key;
			    rms_rsb->rab.rab$l_rop |= RAB$M_LIM;
			    rms_rsb->rab.rab$b_rac = RAB$C_SEQ;
			    break;
		    }
		    break;
		}

		default:    /* low key */
		{
		    rms_rsb->rab.rab$b_ksz = rms_rsb->low_key_len;
		    rms_rsb->rab.rab$l_kbf = rms_rsb->low_key;
		    rms_rsb->rab.rab$l_rop &= ~RAB$M_LIM;
		    rms_rsb->rab.rab$l_rop |= RAB$M_KGE;
		    rms_rsb->rab.rab$b_rac = RAB$C_KEY;
		    break;
		}
	    }
	}
	status = positionRecordStream( gwx_rcb );

	break;
    }

    return(status);
}

/*{
** Name: gw02_get - exit record stream get
**
** Description:
**
**	This exit performs a record get from a positioned record stream.
**	gwX_get() always gets the next record.
**
**	There are still many, many issues to be resolved in this subroutine.
**	In particular, the issue of record size is troubling. We have 4 sizes:
**	1) The RMS buffer size that we allocated back at open time based on
**	    information in the FAB.
**	2) The actual RMS record size, returned to us in RAB$W_RSZ.
**	3) The size of the record mapped by the REGISTER_TABLE command, in
**	    RMS data types (i.e., pre-conversion).
**	4) The size of the resulting 'Ingres' record, after the conversions
**	    from RMS data type to Ingres data type performed by ADF.
**
**	Currently, since our RMS buffer size is HUGE (set at open time to the
**	max record size for this file), we never get RMS$_RTB on our gets.
**	Instead, we fetch the whole record and then convert it to Ingres
**	format, moving it from rms_buffer to var_data1.data_address as we
**	convert it. rms_to_ingres is smart enough to complain if it is asked to
**	convert data outside of the range of either buffer, but it doesn't
**	currently truncate in any 'nice' way.
**
**	At the end, we set var_data1.data_in_size (*not* data_out_size!) to the
**	'resultant record length' returned by rms_to_ingres. I offer no
**	guarantees that that is the 'right' thing to do -- it made sense on
**	4/20/90...
**
**	The actual RMS record length is set by RMS in the rab (RAB$W_RSZ). We
**	don't touch that field after we return from get...gw02_replace depends
**	on this value being still present and correct at the time that it
**	goes to replace the record. gw02_replace will in turn re-calculate the
**	RMS record length and set the RAB$W_RSZ field before calling $UPDATE.
**
** Inputs:
**	gwx_rcb->
**	  xrcb_rsb		record stream control block
**	  var_data1.
**	    data_address	address of the tuple buffer
**	    data_in_size	the buffer size
**
** Output:
**	gwx_rcb->
**	  xrcb_var_data1.
**	    data_address	record stored at this address
**	    data_in_size	size of the record returned
**	  xrcb_error.err_code
**				E_GW0641_END_OF_STREAM
**				E_GW0107_GWX_VGET_ERROR
**				E_GW5401_RMS_FILE_ACT_ERROR
**				E_GW540D_RMS_DEADLOCK
**				E_GW5404_RMS_OUT_OF_MEMORY
**				E_GW540C_RMS_ENQUEUE_LIMIT
**				E_GW540E_RMS_ALREADY_LOCKED
**				E_GW540F_RMS_PREVIOUSLY_DELETED
**				E_GW5410_RMS_READ_LOCKED_RECORD
**				E_GW5411_RMS_RECORD_NOT_FOUND
**				E_GW5412_RMS_READ_AFTER_WAIT
**				E_GW5413_RMS_RECORD_LOCKED
**				E_GW5415_RMS_RECORD_TOO_BIG
**				E_GW5414_RMS_TIMED_OUT
**				E_GW5484_RMS_DATA_CVT_ERROR
**				E_GW050E_ALREADY_REPORTED
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally or End of File
**				reached.
**
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    Create a working version.
**	16-apr-90 (bryanp)
**	    Enhancements to error handling.  Restructured for single point of
**	    return.
**	20-apr-90 (bryanp)
**	    Use the returned record length from sys$get (rab$w__rsz) to tell
**	    us how long a variable-length record actually was. Also taught
**	    rms_to_ingres how to calculate the Ingres record length after
**	    conversion of RMS types to Ingres types, in case that length is
**	    different.
**	23-apr-90 (bryanp)
**	    Asynchronous RMS calls.
**	11-jun-1990 (bryanp)
**	    If rms_to_ingres() reports E_GW5484_DATA_CVT_ERROR, then do no
**	    traceback error logging; just pass E_GW5484 back up to caller.
**	25-jan-91 (RickH)
**	    Set timeout period on RMS get.
**	30-apr-1991 (rickh)
**	    Removed restricted tidjoin logic.
**	14-jun-1991 (rickh)
**	    Reset RAB to SEQuential access if doing a GETNEXT get.  Why?
**	    Because, if you're firing rules, then inbetween sequential
**	    gets, you may be asked to get BYTID.  GWX_BYPOS has changed
**	    name to GWX_GETNEXT since that's how it's used.
**	6-aug-91 (rickh)
**	    Mark rsb as positioned.
**	7-aug-91 (rickh)
**	    Make sure RFA can be mapped into a TID.
**      07-jun-93 (schang)
**          add read-only repeating group support to RMS GW
**      20-oct-93 (schang)
**          remap RMS RFA to Ingres TID
**      22-dec-93 (schang)
**          buffer RMS RFA and return buffer address as Ingres TID
*/

/*
** We have to map 6 byte RMS RFAs into 4 byte INGRES TIDs.  The first 4
** bytes of an RFA are the number of the virtual block where the record
** begins.  The last 2 bytes are the record's offset into that block.
** Since a block is 512 bytes, only 9 bits of that offset are actually
** used.  We think the other 7 bits are always 0 so we blithely throw them
** away.  We end up packing the RFA into the TID as follows:
**
**	virtual block number	->	upper 23 bits of TID
**	record offset		->	lower 9 bits of TID
*/


#define	MAX_SUPPORTED_VIRTUAL_BLOCK_NUMBER	0x7FFFFF
#define	MAX_SUPPORTED_RECORD_OFFSET		0x1FF
#define	RECORD_OFFSET_CAP	( MAX_SUPPORTED_RECORD_OFFSET + 1 )
#define MIN_RECORD_SIZE      8

DB_STATUS
gw02_get
(
    GWX_RCB *gwx_rcb
)
{
    DB_STATUS	status;
    i4		vms_status;
    GW_RSB	*rsb		= (GW_RSB *)gwx_rcb->xrcb_rsb;
    GWRMS_RSB	*rms_rsb	= (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    GW_SESSION  *session        = gwx_rcb->xrcb_gw_session;
    i4		ing_rec_length;
    i4		sid;
    u_i4	*vbn = (u_i4 *)&rms_rsb->rab.rab$w_rfa[0];
    u_i2	*off = (u_i2 *)&rms_rsb->rab.rab$w_rfa[2];
    DB_ERROR	error;
    i4		biggestSupportedBlock;
    i4		biggestSupportedOffset;
    DB_TAB_ID	*db_tab_id = gwx_rcb->xrcb_tab_id;
    PTR         tidloc = NULL;

    status = E_DB_OK;
    error.err_code = 0;
    for (;;)
    {

        /*
        ** if this record is a repeating group and  we have not exhausted
        ** all repeating members do so now by calling rms_to_ingres() to
        ** get an ingres "record" from old rms buffer, after words we just
        ** return but if we have examined each and every repeating member
        ** fall through
        */
        if ((rms_rsb->rptg_is_rptg == GWRMS_REC_IS_RPTG) &&
            (rms_rsb->rptg_seq_no != 0) && (rms_rsb->rptg_count != 0) &&
            (rms_rsb->rptg_seq_no < rms_rsb->rptg_count))
        {
            /*
            ** if by TID, we compare RFA. IF RFA's are different, go to
            ** do a read from base table
            */
	    if (gwx_rcb->xrcb_flags & GWX_BYTID)
	    {
                u_i4  *tidvbn = (u_i4 *)&rms_rsb->rab.rab$w_rfa[0];
                u_i2  *tidoff = (u_i2 *)&rms_rsb->rab.rab$w_rfa[2];
                if (*tidvbn != *vbn || *tidoff != *off)
                    goto readnew;
            }
  	    status = rms_to_ingres(gwx_rcb, &ing_rec_length, &error, &tidloc);
            gwx_rcb->xrcb_error.err_code = 0;
	    if (status != E_DB_OK)
	    {
	        switch (error.err_code)
	        {
		case E_GW5484_RMS_DATA_CVT_ERROR:
		case E_GW5485_RMS_RECORD_TOO_SHORT:
                case E_GW0342_RPTG_OUT_OF_DATA:
		    gwx_rcb->xrcb_error.err_code = error.err_code;
		    break;
		default:
		    gw02_log_error(error.err_code, (struct FAB *)NULL,
				(struct RAB *)NULL, (ULM_RCB *)NULL,
				(SCF_CB *)NULL );
		    gw02_log_error(E_GW500C_RMS_CVT_TO_ING_ERROR,
				(struct FAB *)&rms_rsb->fab->rmsfab,
				(struct RAB *)&rms_rsb->rab,
				(ULM_RCB *)NULL, (SCF_CB *)NULL );
		    gwx_rcb->xrcb_error.err_code = E_GW0107_GWX_VGET_ERROR;
		    break;
	        }
	        break;
	    }
            /*
            ** if this is a index table, we must buffer the new gwrfa, which
            ** is RMS rfa plus the sequence number inside the repeating
            ** record, then copy the address of gwrfa as a tid.
            */
            if ((gwx_rcb->xrcb_xbitset & II_RMS_READONLY) &&
                (gwx_rcb->xrcb_xbitset & II_RMS_BUFFER_RFA) &&
                (db_tab_id->db_tab_index))
            {
                struct gwrms_rfa *rfaptr;

                status = rms_get_rfa_buffer(session, db_tab_id->db_tab_base,
                            (struct gwrms_rfapool_handle *)
                              ((PTR *)gwx_rcb->xrcb_xhandle)[RMSRFAMEMIDX],
                             &rfaptr, *vbn, *off, rms_rsb->rptg_seq_no,
                             &gwx_rcb->xrcb_error);
                if (status != E_DB_OK)
                    break;
                rsb->gwrsb_tid = (PTR)rfaptr;
                if (tidloc != NULL)
                    MEcopy((PTR)&rsb->gwrsb_tid, DB_TID_LENGTH, tidloc);
            }

	    /*
	    ** We're supposed to set the 'size of record returned'. We trust that
	    ** rms_to_ingres() has given us a 'reasonable' value for this in
	    ** ing_rec_length and we return it to our caller in data_in_size (NOTE:
	    ** *not* data_out_size):
	    */
	    gwx_rcb->xrcb_var_data1.data_in_size = ing_rec_length;

	    /*
	    ** Successful retrieval of a record.        
	    */
	    break;
        }
	/*
	** Staging buffer for the rms record was allocated previously.  Now is
	** our chance to use it.  We will read record and move it to the user
	** buffer during the transformation from rms format to ingres format.
	**
	** NOTE: issue here is that 'size' is placed in a word structure
	** element but may be larger than 32767.  See notes in the gw02_open
	** procedure.  RESOLVE THIS BEFORE DELIVERY.
	*/
readnew:
        rms_rsb->rptg_is_rptg = 0;
        rms_rsb->rptg_seq_no = 0;
        rms_rsb->rptg_count = 0;
	rms_rsb->rab.rab$l_ubf = rms_rsb->rms_buffer;
	rms_rsb->rab.rab$w_usz = rms_rsb->rms_bsize;

	/* set RMS timeout value	25-jan-91 */

	applyTimeout( rsb, rms_rsb );

	if (gwx_rcb->xrcb_flags & GWX_BYTID)
	{
            /*
	    ** Connect here if haven't done so yet (by tid access means we
	    ** haven't called gw02_position(); this code should be modularized
	    ** and put in its own routine but gw02_put() needs to position at
	    ** EOF at connection time so for now, we'll just repeat it here).
	    */

	    if (rms_rsb->rab.rab$w_isi == 0) /* i.e. we haven't connected yet */
	    {
	        status = gw02_connect(gwx_rcb, GWX_VGET);
		if (status != E_DB_OK)
		{
		    gwx_rcb->xrcb_error.err_code = E_GW0107_GWX_VGET_ERROR;
		    break;
		}
	    }   /* end if-already-connected */
            
	    rms_rsb->rab.rab$b_rac = RAB$C_RFA;
            /*
            **  convert an Ingres tid to an RMS RFA
            */
            if ((gwx_rcb->xrcb_xbitset & II_RMS_READONLY) &&
                (gwx_rcb->xrcb_xbitset & II_RMS_BUFFER_RFA))
            {
                *vbn = ((struct gwrms_rfa *)(rsb->gwrsb_tid))->rfa0;
                *off = ((struct gwrms_rfa *)(rsb->gwrsb_tid))->rfa4;
            }
            else
            {
                *vbn = rsb->gwrsb_tid >> 9;
                *off = rsb->gwrsb_tid & 0x000001ff;
            }
	    rms_rsb->key_ref = -1;
	}
	else if (gwx_rcb->xrcb_flags == GWX_GETNEXT)
	{
	    rms_rsb->rab.rab$b_rac = RAB$C_SEQ;
	}

	/* asynchronous RMS: */
	CSget_sid(&sid);
	rms_rsb->rab.rab$l_ctx = sid;
	rms_rsb->rab.rab$l_rop |= RAB$M_ASY;
        /*
        ** get rid of wait flag so that we can have TRUE read-regardless
        ** lock: just do dirty read, do not wait for release of lock.
        */
        if ((gwx_rcb->xrcb_xbitset & II_RMS_RRL) &&
            (gwx_rcb->xrcb_xbitset & II_RMS_READONLY))
        {
	    rms_rsb->rab.rab$l_rop |= RAB$M_RRL;
            if (!(rms_rsb->rab.rab$l_rop & RAB$M_TMO)) /* no time specified */
                rms_rsb->rab.rab$l_rop &= ~RAB$M_WAT;
        }

	vms_status = rms_rab_suspend(
		    sys$get(&rms_rsb->rab, rms_rab_resume, rms_rab_resume),
		    &rms_rsb->rab);
            
	switch (vms_status)
	{
	    case RMS$_NORMAL:
	    case RMS$_OK_WAT:
	    case RMS$_OK_ALK:
		status = E_DB_OK;
		break;
	    case RMS$_EOF:
	    case RMS$_OK_LIM:
	    case RMS$_RNF:
		gwx_rcb->xrcb_error.err_code = E_GW0641_END_OF_STREAM;
		status = E_DB_ERROR;
		break;

	    default:
		if (!(vms_status & 0x01))
		{
		    gw02_log_error(E_GW5006_RMS_GET_ERROR,
				   (struct FAB *)&rms_rsb->fab->rmsfab,
				   (struct RAB *)&rms_rsb->rab,
				   (ULM_RCB *)NULL, (SCF_CB *)NULL );
		    gwx_rcb->xrcb_error.err_code = 
				    gw02_ingerr(RMS_SYS_GET, vms_status,
						E_GW0107_GWX_VGET_ERROR);
		    status = E_DB_ERROR;
		}
		else
		{
                    /*
                    ** schang : single out RMS$_OK_RRL and do nothing
                    ** if we specified  II_RMS_RRL earlier,
                    ** otherwise log the information
                    */
                    if ((vms_status==RMS$_OK_RRL || vms_status==RMS$_OK_RLK)
                         && (gwx_rcb->xrcb_xbitset & II_RMS_RRL))
                        ;
                    else
		    /*
		    ** We got some other status, which was not RMS$_NORMAL and
		    ** was NOT an error.  For now, let's log that status.
		    ** Later, if we in fact decide that this is OK, we can
		    ** remove this log call.
		    */
		        gw02_log_error(E_GW5006_RMS_GET_ERROR,
				       (struct FAB *)&rms_rsb->fab->rmsfab,
				       (struct RAB *)&rms_rsb->rab,
				       (ULM_RCB *)NULL, (SCF_CB *)NULL);
		    status = E_DB_OK;
		}
		break;	/* from default in switch stmt. */
	}

	if (status != E_DB_OK)
	    break;

	/*
	** Once you have accessed the first record, we proceed sequentially
	** from there.
	**
	** If there is a high key specified, it must be applied at this time
	** (if a low key was already  used).  This is a complication of RMS
	** record access.
	*/
	/*
	** Condition is:
	**  if ((keyed access)
	**	and
	**	((high key provided)
	**	  and
	**      ((high key has not yet been set in rab)))
	*/
	if ((rms_rsb->key_ref != -1)
	    &&
	    ((rms_rsb->high_key_len != 0)
	     &&
	     (rms_rsb->rab.rab$l_kbf != rms_rsb->high_key)))
	{
	    rms_rsb->rab.rab$b_ksz = rms_rsb->high_key_len;
	    rms_rsb->rab.rab$l_kbf = rms_rsb->high_key;
	    rms_rsb->rab.rab$l_rop &= ~RAB$M_KGE;
	    rms_rsb->rab.rab$l_rop |= RAB$M_LIM;
	}

	if (!(gwx_rcb->xrcb_flags & GWX_BYTID))
	{
	    /*
	    ** If access is not by tid, set access mode after first get to be
	    ** sequential.
	    */
	    rms_rsb->rab.rab$b_rac = RAB$C_SEQ;
	}

	/*
	** sys$get() has set the RFA for the record.  Use that to construct a
	** 4-byte "tid" value (a la fseek) which can later be turned back into
	** an RFA value for random access.  This is necessary to support
	** certain Ingres join logic, such as in some nested selects, where the
	** top node is an action node and gets a tid copied from a lower level
	** node.  Formula is:  VBN * 512 + OFF to get byte offset of record.
	** If the get was "by tid", this value should be the same as it was
	** coming into this routine.
	*/

        /*
        ** if rfa needs to be buffered, we store it and return the buffer 
        ** address as tid.  If not, we convert rfa to tid using (23,9)
        ** scheme.  We return memory address for bufferred rfa only if this
        ** is a index access.  Certain base table access may also need to
        ** buffer rfa, I don't know if there is any real case to verify
        ** it.
        */
       if ((gwx_rcb->xrcb_xbitset & II_RMS_READONLY) &&
            (gwx_rcb->xrcb_xbitset & II_RMS_BUFFER_RFA) &&
            (db_tab_id->db_tab_index))
        {
            struct gwrms_rfa *rfaptr;
	    /*
	    ** Convert the record from RMS format to INGRES format using the
	    ** desciptors in the TCB. We give rms_to_ingres as much information as
	    ** we can about the record buffers and their lengths, and let it tell
	    ** us about the resulting 'Ingres' record length.
            */
            tidloc = NULL;
  	    status = rms_to_ingres(gwx_rcb, &ing_rec_length, &error, &tidloc);
            gwx_rcb->xrcb_error.err_code = 0;
	    if (status != E_DB_OK)
	    {
	        switch (error.err_code)
	        {
		case E_GW5484_RMS_DATA_CVT_ERROR:
		case E_GW5485_RMS_RECORD_TOO_SHORT:
                case E_GW0342_RPTG_OUT_OF_DATA:
		    gwx_rcb->xrcb_error.err_code = error.err_code;
		    break;
		default:
		    gw02_log_error(error.err_code, (struct FAB *)NULL,
				(struct RAB *)NULL, (ULM_RCB *)NULL,
				(SCF_CB *)NULL );
		    gw02_log_error(E_GW500C_RMS_CVT_TO_ING_ERROR,
				(struct FAB *)&rms_rsb->fab->rmsfab,
				(struct RAB *)&rms_rsb->rab,
				(ULM_RCB *)NULL, (SCF_CB *)NULL );
		    gwx_rcb->xrcb_error.err_code = E_GW0107_GWX_VGET_ERROR;
		    break;
	        }
	        break;
	    }
            status = rms_get_rfa_buffer(session, db_tab_id->db_tab_base,
                         (struct gwrms_rfapool_handle *)
                                 ((PTR *)gwx_rcb->xrcb_xhandle)[RMSRFAMEMIDX],
                         &rfaptr, *vbn, *off, rms_rsb->rptg_seq_no,
                         &gwx_rcb->xrcb_error);
            if (status != E_DB_OK)
                break;
            rsb->gwrsb_tid = (PTR)rfaptr;
            if (tidloc != NULL)
                MEcopy((PTR)&rsb->gwrsb_tid, DB_TID_LENGTH, tidloc);
        } 
        else
        { 
            rsb->gwrsb_tid = (*vbn << 9) + *off;
	    /*
	    ** Convert the record from RMS format to INGRES format using the
	    ** desciptors in the TCB. We give rms_to_ingres as much information as
	    ** we can about the record buffers and their lengths, and let it tell
	    ** us about the resulting 'Ingres' record length.
	    */
            tidloc = NULL;
	    status = rms_to_ingres(gwx_rcb, &ing_rec_length, &error, &tidloc);
            gwx_rcb->xrcb_error.err_code = 0;
	    if (status != E_DB_OK)
	    {
	        switch (error.err_code)
	        {
		case E_GW5484_RMS_DATA_CVT_ERROR:
		case E_GW5485_RMS_RECORD_TOO_SHORT:
                case E_GW0342_RPTG_OUT_OF_DATA:
		    gwx_rcb->xrcb_error.err_code = error.err_code;
		    break;
		default:
		    gw02_log_error(error.err_code, (struct FAB *)NULL,
				(struct RAB *)NULL, (ULM_RCB *)NULL,
				(SCF_CB *)NULL );
		    gw02_log_error(E_GW500C_RMS_CVT_TO_ING_ERROR,
				(struct FAB *)&rms_rsb->fab->rmsfab,
				(struct RAB *)&rms_rsb->rab,
				(ULM_RCB *)NULL, (SCF_CB *)NULL );
		    gwx_rcb->xrcb_error.err_code = E_GW0107_GWX_VGET_ERROR;
		    break;
	        }
	        break;
	    }
            if (tidloc != NULL)
                MEcopy((PTR)&rsb->gwrsb_tid, DB_TID_LENGTH, tidloc);

        }
	/*
	** We're supposed to set the 'size of record returned'. We trust that
	** rms_to_ingres() has given us a 'reasonable' value for this in
	** ing_rec_length and we return it to our caller in data_in_size (NOTE:
	** *not* data_out_size):
	*/
	gwx_rcb->xrcb_var_data1.data_in_size = ing_rec_length;

	/*
	** Successful retrieval of a record.        
	*/
	break;
    }

    /* sys$get and sys$find position the RAB on a record */
    /* repeating group out of data does not change previous settings */
    if ( status == E_DB_OK || error.err_code == E_GW0342_RPTG_OUT_OF_DATA )
	rms_rsb->rsb_state |= GWRMS_RAB_POSITIONED;
    else
	rms_rsb->rsb_state &= ~( GWRMS_RAB_POSITIONED );

    return ( status );
}

/*{
** Name: gw02_put - add a record.
**
** Description:
**	This exit puts a record into a table.  A call to gwX_open() must have
**	preceded this request.  (gwX_position() is not required.)
**
** Inputs:
**	gwx_rcb->
**		xrcb_rsb	    set by gwX_open
**	  	var_data1.
**	    	  data_address	    address of the tuple buffer
**	    	  data_in_size	    the buffer size
**
** Output:
**	gwx_rcb->
**	    error.err_code	    Detailed error code if error occurred:
**				    E_GW0108_GWX_VPUT_ERROR
**				    E_GW5401_RMS_FILE_ACT_ERROR
**				    E_GW5404_RMS_OUT_OF_MEMORY
**				    E_GW5416_RMS_INVALID_DUP_KEY
**				    E_GW5417_RMS_DEVICE_FULL
**				    E_GW540E_RMS_ALREADY_LOCKED
**				    E_GW5418_RMS_DUPLICATE_KEY
**				    E_GW5419_RMS_INDEX_UPDATE_ERROR
**				    E_GW5413_RMS_RECORD_LOCKED
**				    E_GWxxxx_RMS_INV_REC_SIZE
**				    E_GW5484_RMS_DATA_CVT_ERROR
**
**      Returns:
**          E_DB_OK		    Function completed normally. 
**          E_DB_ERROR		    Function completed abnormally
**
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    Clean up (still may not work)
**	16-apr-90 (bryanp)
**	    Enhanced error handling.
**	    Restructured to have a single point of return.
**	23-apr-90 (bryanp)
**	    Asynchronous RMS calls.
**	11-jun-1990 (bryanp)
**	    If rms_from_ingres() reports E_GW5484_DATA_CVT_ERROR, then do no
**	    traceback error logging; just pass E_GW5484 back up to caller.
**	25-jan-91 (RickH)
**	    Set timeout period on RMS gets.
**      12-jan-92 (schang)
**          gw02_connect already set up correct access if record stream is
**          established already. 
*/
DB_STATUS
gw02_put
(
    GWX_RCB *gwx_rcb
)
{
    DB_STATUS	status;
    i4		vms_status;
    i4		rms_rec_length;
    GW_RSB	*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_TCB	*tcb = (GW_TCB *)rsb->gwrsb_tcb;
    GWRMS_RSB	*rms_rsb = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    DB_ERROR	error;
    i4		sid;
    
    status = E_DB_OK;
    for (;;)
    {
	if (!rms_rsb->rab.rab$w_isi) /* rab$w_isi is filled in by sys$connect */
	{
	    status = gw02_connect(gwx_rcb, GWX_VPUT);
	    if (status != E_DB_OK)
	    {
		gwx_rcb->xrcb_error.err_code = E_GW0108_GWX_VPUT_ERROR;
		break;
	    }		
	}   /* end if-already-connected */

	/*
	** Before inserting a new record, we want to initialize the rms buffer
	** to some default value, which will be put in the rms file for any
	** non-registered fields.  In the future we may want to let users
	** specify what value this should be...  For now we pick an ascii space
	** character.
	*/
	MEfill(rms_rsb->rms_bsize, (u_char)' ', (PTR)rms_rsb->rms_buffer);

	/*
	** Convert the record from INGRES format to RMS format.  If we allow
	** variable length records, return the output record size from the
	** conversion routine. See note below.
	**
	** RMS record length must be computed at this time.  In the case of
	** fixed length records we can get this information from the FAB.
	** However, if we are going to allow writing variable length records, a
	** more compicated interface will be required. Essentially the length
	** of the record will have to be computed by the type conversion
	** procedure.
	**
	** Place this value in rms_rec_length;
	*/
	status = rms_from_ingres(tcb, rsb, gwx_rcb->xrcb_var_data1.data_address,
		    gwx_rcb->xrcb_var_data1.data_in_size /* ingres rec len */,
		    rms_rsb->rms_buffer,
		    (i4)0 /* current record length is 0 on a put */,
		    rms_rsb->rms_bsize	/* size of allocated RMS buffer */,
		    &rms_rec_length, &error);

	if (status != E_DB_OK)
	{
	    if (error.err_code == E_GW5484_RMS_DATA_CVT_ERROR)
	    {
		gwx_rcb->xrcb_error.err_code = error.err_code;
	    }
	    else
	    {
		gw02_log_error(error.err_code, (struct FAB *)NULL,
			    (struct RAB *)NULL, (ULM_RCB *)NULL,
			    (SCF_CB *)NULL );
		gwx_rcb->xrcb_error.err_code = E_GW0108_GWX_VPUT_ERROR;
	    }
	    status = E_DB_ERROR;
	    break;
	}
	
	/*
	** On a put operation, calculate record length based on the file's
	** record format only.  If a shorter record length was calculated and
	** returned from the "rms_from_ingres()" routine, that means that the
	** last registered field was a varying string type.  Put out the
	** shorter length if RMS record format is varying; else use maximum
	** record length.
	**
	** ISSUE:  even though a field is the last one registered, that does
	** not necessarily mean it's the last one in the rms file.  Users will
	** have to understand this, and use varying as the last field ONLY when
	** it is the last field in the target record.  Also, by "last
	** registered" is meant not syntactically last, but the one which
	** occurs last in the registered record layout.  If a field is
	** registered as varying but is not the last in the layout, then the
	** varying specifier has no effect at all.  All of this is more
	** important in the case of a replace than a put.
	*/

	switch (rms_rsb->fab->rmsfab.fab$b_rfm)
	{
	    case    FAB$C_FIX:
		rms_rsb->rab.rab$w_rsz = rms_rsb->fab->rmsfab.fab$w_mrs;
		break;

	    case    FAB$C_VAR:
	    case    FAB$C_VFC:
	    case    FAB$C_STM:
	    case    FAB$C_STMCR:
	    case    FAB$C_STMLF:
		rms_rsb->rab.rab$w_rsz = rms_rec_length;
		break;

	    default:
		/*** error... E_GW5010_UNSUPPORTED_FILE_TYPE ... ***/
		break;
	}

	rms_rsb->rab.rab$l_rbf = rms_rsb->rms_buffer;

	if (rms_rsb->fab->rmsfab.fab$b_org == FAB$C_IDX )
	{
	    /* an indexed file */
	    rms_rsb->rab.rab$b_rac = RAB$C_KEY;
	}
	else
	{
	    rms_rsb->rab.rab$b_rac = RAB$C_SEQ;
	}

	/* not too sure about rab$l_rhb for VFC record types */
	/* set RMS timeout value	25-jan-91 */

	applyTimeout( rsb, rms_rsb );

	/* asynchronous RMS: */
	CSget_sid(&sid);
	rms_rsb->rab.rab$l_ctx = sid;
	rms_rsb->rab.rab$l_rop |= RAB$M_ASY;

	vms_status = rms_rab_suspend(
		    sys$put(&rms_rsb->rab, rms_rab_resume, rms_rab_resume),
		    &rms_rsb->rab);
            
	switch (vms_status)
	{
	    case RMS$_NORMAL:
	    case RMS$_OK_WAT:
	    case RMS$_OK_DUP:
		gwx_rcb->xrcb_error.err_code = E_DB_OK;
		status = E_DB_OK;
		break;

	    default:
		gw02_log_error(E_GW5007_RMS_PUT_ERROR,
			       (struct FAB *)&rms_rsb->fab->rmsfab,
			       (struct RAB *)&rms_rsb->rab,
			       (ULM_RCB *)NULL, (SCF_CB *)NULL );
		gwx_rcb->xrcb_error.err_code = 
				gw02_ingerr(RMS_SYS_PUT,  vms_status,
					    E_GW0108_GWX_VPUT_ERROR);
		status = E_DB_ERROR;
		break;
	}

	break;
    }

    return(status);
}

/*{
** Name: gw02_replace - exit record stream replace.
**
** Description:
**	This exit performs a record replacement.  A call to gw02_position()
**	must have preceded this request.
**
** Inputs:
**	gwx_rcb->
**		xrcb_rsb	record stream control block
**	  	var_data1.
**	    	  data_address	address of the tuple buffer
**	    	  data_in_size	the buffer size
**
** Output:
**	gwx_rcb->
**	    error.err_code	Detailed error code if an error occurred:
**				E_GW0109_GWX_VREPLACE_ERROR
**				E_GW5401_RMS_FILE_ACT_ERROR
**				E_GW541A_RMS_NO_CURRENT_RECORD
**				E_GW5404_RMS_OUT_OF_MEMORY
**				E_GW5416_RMS_INVALID_DUP_KEY
**				E_GW540E_RMS_ALREADY_LOCKED
**				E_GW5418_RMS_DUPLICATE_KEY
**				E_GW5419_RMS_INDEX_UPDATE_ERROR
**				E_GW5484_RMS_DATA_CVT_ERROR
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    Cleanup. Still doesn't work. See comments.
**	16-apr-90 (bryanp)
**	    Enhanced error handling.  Restructured to contain a single point of
**	    return.
**	23-apr-90 (bryanp)
**	    Asynchronous RMS calls.
**	11-jun-1990 (bryanp)
**	    If rms_from_ingres() reports E_GW5484_DATA_CVT_ERROR, then do no
**	    traceback error logging; just pass E_GW5484 back up to caller.
**	    Oh, and by the way: checked the return code from rms_from_ingres().
**	6-aug-91 (rickh)
**	    Reposition the record stream if necessary.  Unfortunately, both
**	    sys$update and sys$delete clobber the RFA and in some fashion
**	    inaccessible to us flag RMS that the stream is now unpositioned.
**      29-oct-92 (schang)
**          allow multiple stream for a FAB.
**      19-jan-93 (schang)
**          doc: if record stream is established, it is properly set up for
**          the operation, no need to lock record explicitly with ~RMS$M_NLK
**          when reposition
*/
DB_STATUS
gw02_replace
(
    GWX_RCB *gwx_rcb
)
{
    DB_STATUS	status;
    i4		vms_status;
    i4		rms_rec_length;
    GW_RSB	*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_TCB	*tcb = (GW_TCB *)rsb->gwrsb_tcb;
    GWRMS_RSB	*rms_rsb = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    i4		sid;
    DB_ERROR	error;
    unsigned	short	saved_rfa[ RFA_LENGTH ]; /* record's file address */
    i4		i;
    
    status = E_DB_OK;
    for (;;)
    {    
	/*
	** Convert the record from INGRES format to RMS format.  If we allow
	** variable length records, return the output record size from the
	** conversion routine. See note below. We pass the current record
	** length of the RMS record down to the data conversion routine,
	** which uses it for its own purposes. The data conversion routine
	** ("rms_from_ingres") returns to us the resulting rms record length
	** of the new record, which we need to pass to the $UPDATE service.
	*/

	status = rms_from_ingres(tcb, rsb, gwx_rcb->xrcb_var_data1.data_address,
		    gwx_rcb->xrcb_var_data1.data_in_size /* ingres rec len */,
		    rms_rsb->rms_buffer,
		    /* length of this RMS record before the update: */
		    rms_rsb->rab.rab$w_rsz,
		    rms_rsb->rms_bsize	/* size of allocated RMS buffer */,
		    &rms_rec_length, &error);

	if (status != E_DB_OK)
	{
	    if (error.err_code == E_GW5484_RMS_DATA_CVT_ERROR)
	    {
		gwx_rcb->xrcb_error.err_code = error.err_code;
	    }
	    else
	    {
		gw02_log_error(error.err_code, (struct FAB *)NULL,
			    (struct RAB *)NULL, (ULM_RCB *)NULL,
			    (SCF_CB *)NULL );
		gwx_rcb->xrcb_error.err_code = E_GW0109_GWX_VREPLACE_ERROR;
	    }
	    break;
	}
        
	/*
	** RMS record length has been computed in rms_from_ingres.  In the case
	** of fixed length records we can get this information from the FAB.
	** However, if we are going to allow writing variable length records, a
	** more compicated interface will be required. Essentially the length
	** of the record will have to be computed by the type conversion
	** procedure.
	**
	** Place this value in rms_rec_length;
	*/

	/* not too sure about rab$l_rhb for VFC record types */

	/*
	** FIXME:  revisit these record length settings for validity and desired
	** semantics.	1-feb-91 (linda)
	*/

	switch (rms_rsb->fab->rmsfab.fab$b_org)
	{
	    case    FAB$C_SEQ:
		rms_rsb->rab.rab$w_rsz = rms_rec_length;
/*		rms_rsb->rab.rab$w_rsz = rms_rsb->fab->rmsfab.fab$w_mrs;    */
		break;

	    case    FAB$C_IDX:
	    case    FAB$C_REL:
	    {
		switch (rms_rsb->fab->rmsfab.fab$b_rfm)
		{
		    case    FAB$C_FIX:
			/*
			** QUESTION:  do we really want to enforce this here,
			** possibly quietly truncating data; or do we want to
			** put in the actual rms record size and let RMS issue
			** an error if the size is wrong?  Perhaps use the fab
			** value only if rms_rec_len is *shorter*...
			*/
			rms_rsb->rab.rab$w_rsz = rms_rsb->fab->rmsfab.fab$w_mrs;
			break;

		    case    FAB$C_VAR:
		    case    FAB$C_VFC:
		    case    FAB$C_STM:
		    case    FAB$C_STMCR:
		    case    FAB$C_STMLF:
			rms_rsb->rab.rab$w_rsz = rms_rec_length;
			break;

		    default:
			/*** error... E_GWxxxx_UNSUPPORTED_RECORD_TYPE ... ***/
			break;
		}

		default:
		    /*** error... E_GW5010_UNSUPPORTED_FILE_TYPE ... ***/
		    break;
	    }
	}

	/* asynchronous RMS: */
	CSget_sid(&sid);
	rms_rsb->rab.rab$l_ctx = sid;
	rms_rsb->rab.rab$l_rop |= RAB$M_ASY;

	if ((rms_rsb->rsb_state & GWRMS_RAB_POSITIONED) == 0)
        {
#if 0
            /*
            ** when reposition, also lock record for update
            */
	    rms_rsb->rab.rab$l_rop = rms_rsb->rab.rab$l_rop | RAB$M_ASY
                                 & (~RAB$M_NLK);
#endif
	    status = rePositionRAB( gwx_rcb );
	    if ( status != E_DB_OK )
                break;
        }

	for ( i = 0; i < RFA_LENGTH; i++ )	/* sys$update zeroes out RFA! */
	    saved_rfa[ i ] = rms_rsb->rab.rab$w_rfa[ i ];

	vms_status = rms_rab_suspend(
		    sys$update(&rms_rsb->rab, rms_rab_resume, rms_rab_resume),
		    &rms_rsb->rab);
            
	for ( i = 0; i < RFA_LENGTH; i++ )	/* now restore RFA */
	    rms_rsb->rab.rab$w_rfa[ i ] = saved_rfa[ i ];
	/* sys$update and sys$delete de-position the RAB */
	rms_rsb->rsb_state &= ~( GWRMS_RAB_POSITIONED );

	switch (vms_status)
	{
	    case RMS$_NORMAL:
	    case RMS$_OK_DUP:
		gwx_rcb->xrcb_error.err_code = E_DB_OK;
		status = E_DB_OK;
		break;

	    default:
		gw02_log_error(E_GW5008_RMS_UPDATE_ERROR,
			       (struct FAB *)&rms_rsb->fab->rmsfab,
			       (struct RAB *)&rms_rsb->rab,
			       (ULM_RCB *)NULL, (SCF_CB *)NULL );
		gwx_rcb->xrcb_error.err_code = 
				gw02_ingerr(RMS_SYS_UPDATE, vms_status,
					    E_GW0109_GWX_VREPLACE_ERROR);
		status = E_DB_ERROR;
		break;
	}

	break;
    }

    return(status);
}

/*{
** Name: gw02_delete - exit record stream delete.
**
** Description:
**	This exit performs deletion of a record from the record stream.  A call
**	to gwX_position() or gwX_get() must have preceded this request.
**
** Inputs:
**	gwx_rcb->
**		xrcb_rsb	record stream control block
**
** Output:
**	gwx_rcb->
**	    error.err_code	Detailed error code if an error occurred:
**				E_GW010A_GWX_VDELETE_ERROR
**				E_GW5401_RMS_FILE_ACT_ERROR
**				E_GW541A_RMS_NO_CURRENT_RECORD
**				E_GW5404_RMS_OUT_OF_MEMORY
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    Cleanup.
**	16-apr-90 (bryanp)
**	    Enhanced error handling, restructured to have single return point.
**	23-apr-90 (bryanp)
**	    Asynchronous RMS calls.
**	14-jun-91 (rickh)
**	    Restore the RFA.  Unfortunately, sys$delete clobbers it.
**	6-aug-91 (rickh)
**	    Reposition the record stream if necessary.  Unfortunately, both
**	    sys$update and sys$delete clobber the RFA and in some fashion
**	    inaccessible to us flag RMS that the stream is now unpositioned.
**      29-oct-92 (schang)
**          allow multiple stream for a FAB
**      19-jan-93 (schang)
**          doc: if record stream is established, it is properly set up for
**          the operation, no need to lock record explicitly with ~RMS$M_NLK
**          when reposition
*/
DB_STATUS
gw02_delete
(
    GWX_RCB *gwx_rcb
)
{
    DB_STATUS	status;
    i4		vms_status;
    GW_RSB	*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GWRMS_RSB	*rms_rsb = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    i4		sid;
    unsigned	short	saved_rfa[ RFA_LENGTH ]; /* record's file address */
    i4		i;
    PTR         tmpptr;

    status = E_DB_OK;
    for (;;)
    {
	if (rms_rsb->fab->rmsfab.fab$b_org == FAB$C_SEQ)
	{
	    gwx_rcb->xrcb_error.err_code = E_GW541C_RMS_DELETE_FROM_SEQ;
	    status = E_DB_ERROR;
	    break;
	}

	/* asynchronous RMS: */
	CSget_sid(&sid);
	rms_rsb->rab.rab$l_ctx = sid;
	rms_rsb->rab.rab$l_rop |= RAB$M_ASY;


        /*
        ** when reposition for delete, we want RMS to lock the current
        ** record so that we can delete this record subsequently
        */
	if ((rms_rsb->rsb_state & GWRMS_RAB_POSITIONED) == 0)
        {
#if 0
            /*
            ** when reposition, also lock record for delete
            */
	    rms_rsb->rab.rab$l_rop = rms_rsb->rab.rab$l_rop | RAB$M_ASY
                                 & (~RAB$M_NLK);
#endif
	    status = rePositionRAB( gwx_rcb );
	    if ( status != E_DB_OK )
                break;
        }

	for ( i = 0; i < RFA_LENGTH; i++ )	/* sys$delete zeroes out RFA! */
	    saved_rfa[ i ] = rms_rsb->rab.rab$w_rfa[ i ];

	vms_status = rms_rab_suspend(
		    sys$delete(&rms_rsb->rab, rms_rab_resume, rms_rab_resume),
		    &rms_rsb->rab);
            
	for ( i = 0; i < RFA_LENGTH; i++ )	/* now restore RFA */
	    rms_rsb->rab.rab$w_rfa[ i ] = saved_rfa[ i ];

	/* sys$update and sys$delete de-position the RAB */
	rms_rsb->rsb_state &= ~( GWRMS_RAB_POSITIONED );

	switch (vms_status)
	{
	    case RMS$_NORMAL:
	    case RMS$_EOF:
		gwx_rcb->xrcb_error.err_code = E_DB_OK;
		status = E_DB_OK;
		break;

	    default:
		/*** Log error for easier problem diagnosis */
		gw02_log_error(E_GW5009_RMS_DELETE_ERROR,
			    (struct FAB *)&rms_rsb->fab->rmsfab,
			    (struct RAB *)&rms_rsb->rab,
			    (ULM_RCB *)NULL, (SCF_CB *)NULL );
		gwx_rcb->xrcb_error.err_code = gw02_ingerr(RMS_SYS_DELETE, 
					    vms_status,
					    E_GW010A_GWX_VDELETE_ERROR);
		status = E_DB_ERROR;
		break;
	}
	break;
    }

    return(status);
}

/*{
** Name: gw02_begin - exit begin transaction.
**
** Description:
**
**	This exit requests the foreign data manager to begin a transaction.
**
**	It is guaranteed that gwX_open() has been called and thus the exit_cb
**	has been initialized.
**
**	Generally, begin transaction marks the beginning of a recoverable unit.
**	RMS does not have instrinsic ability to recover.  This request performs
**	no useful operation.
**
** Inputs:
**	gwx_rcb
**
** Output:
**	None
**
**      Returns:
**          E_DB_OK             Function completed normally. 
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    Add parameter in case needed in future
*/
DB_STATUS
gw02_begin
(
    GWX_RCB *gwx_rcb
)
{
    /* The RMS gateay does not currently support transactions */
    return(E_DB_OK);
}

/*{
** Name: gw02_abort - exit abort transaction.
**
** Description:
**
**	This exit requests foreign data manager to abort transaction.
**
**	Generally, abort transaction aborts all data changes since the last
**	begin transaction.  The recoverable unit marked by begin transaction is
**	erased.
**
**	RMS cannot abort updates.
**
** Inputs:
**	gwx_rcb
**
** Output:
**	None
**
**      Returns:
**          E_DB_OK	cannot perform the opeation
**
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    Make it return E_DB_OK instead of E_DB_ERROR.  Helps ingres at
**	    least run a little.  Add a parameter in case it is needed by other
**	    gateways in the future.
*/
DB_STATUS
gw02_abort
(
    GWX_RCB *gwx_rcb
)
{
    /* The RMS gateway currently does not support transactions */
    return(E_DB_OK);
}

/*{
** Name: gw02_commit - exit commit transaction.
**
** Description:
**
**	This exit requests foreign data manager to commit transaction.
**
**	Generally, commit transaction makes permanent all data changes since
**	the last begin transaction.  The recoverable unit marked by begin
**	transaction is erased.
**
**	RMS cannot perform atomic commit.  It can only flush updates.
**
** Inputs:
**	gwx_rcb->
**	  	xrcb_rsb	record stream control block
**
** Output:
**	gwx_rcb->
**	    error.err_code	Detailed error code if an error occurred:
**				E_GW010D_GWX_VCOMMIT_ERROR
**				E_GW5401_RMS_FILE_ACT_ERROR
**				E_GW5404_RMS_OUT_OF_MEMORY
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally
**                              with error.err_code.
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	26-dec-89 (paul)
**	    Create working version.
**	16-apr-90 (bryanp)
**	    Enhanced error handling, restructured for single return point.
**	23-apr-90 (bryanp)
**	    Asynchronous RMS calls.
*/
DB_STATUS
gw02_commit
(
    GWX_RCB *gwx_rcb
)
{
    DB_STATUS	status;
    i4		vms_status;
    GW_RSB	*rsb = gwx_rcb->xrcb_rsb;
    GWRMS_RSB	*rms_rsb = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    i4		sid;
    
    status = E_DB_OK;
    for (;;)
    {
	/*
	** This function is really misnamed. It actually means commit the rsb
	** with which it is called.  Since it will be called once for each rsb
	** open within the current transaction, this really can't be considered
	** a full commit.
	**
	** Note that this may be a problem for non-relational gateways that
	** support a transaction model. This commit will be issued once per
	** table rather than once per transaction.
	**** I'll leave this for a more patient person to try to sort all this
	** out.
	*/

	/* asynchronous RMS: */
	CSget_sid(&sid);
	rms_rsb->rab.rab$l_ctx = sid;
	rms_rsb->rab.rab$l_rop |= RAB$M_ASY;

	vms_status = rms_rab_suspend(
		    sys$flush(&rms_rsb->rab, rms_rab_resume, rms_rab_resume),
		    &rms_rsb->rab);
            
	if (vms_status != RMS$_NORMAL)
	{
	    /*
	    *** We should take the time to log and/or report a detailed error
	    ** at this time. This will help users diagnose rms problems in a
	    ** reasonable manner.  Report at least vms error and all
	    ** parameters.
	    */
	    gw02_log_error(E_GW500A_RMS_FLUSH_ERROR,
				(struct FAB *)&rms_rsb->fab->rmsfab,
				(struct RAB *)&rms_rsb->rab,
				(ULM_RCB *)NULL, (SCF_CB *)NULL );
	    gwx_rcb->xrcb_error.err_code = gw02_ingerr(RMS_SYS_FLUSH, 
						vms_status,
						E_GW010D_GWX_VCOMMIT_ERROR);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	**** Should we also call sys$free?? When are records unlocked?
	*/

	/*
	** Successful end of gw02_commit:
	*/
	break;
    }

    return(status);
}

/*{
** Name: gw02_info - exit information
**
** Description:
**
**	This exit returns exit-specific information.
**
** Inputs:
**	None
**
** Output:
**	gwx_rcb->
**		var_data1	release id string of the RMS Gateway
**      Returns:
**          E_DB_OK             Function completed normally. 
**
** History:
**      9-May-1989 (alexh)
**          Created.
*/
DB_STATUS
gw02_info
(
    GWX_RCB *gwx_rcb
)
{
    /*
    ** Should the trailing null character be included in the length?
    */
    gwx_rcb->xrcb_var_data1.data_address = Gwrms_version;
    gwx_rcb->xrcb_var_data1.data_in_size = STlength(Gwrms_version)+1;
    return(E_DB_OK);
}

/*{
** Name: gw02_init - exit initialization
**
** Description:
**	This exit performs exit initialization which may include:  foreign data
**	manager load and/or initilization; initialization of other exits; exit
**	session control block size; table control block size; record stream
**	control block size; extended catalog tuple sizes.
**
**	Facility global structures are allocated and initialized.  Gateway-
**	specific catalog tuple sizes must be filled in so proper tuple buffers
**	can be allocated for gwX_tabf requests during table registration.
**
**	This routine should verify that gwf_version is the same as GWF_VERSION
**	and that it is compatible with the gateway's internal version.  If the
**	gwf_version is not acceptable, the exit_table should be initialized to
**	NULL and E_DB_ERROR should be returned.
**
** Inputs:
**	gwx_rcb->		GW exit request block
**		gwf_version	GWF version id
**		exit_table	address of exit routine table
**
** Output:
**	gwx_rcb->
**		exit_table	table contains addresses of 
**				exit routine routines
**		exit_cb_size	exit control block size
**		xrelation_sz	iigwX_relation size
**		xattribute_sz	iigwX_attribute size
**		xindex_size	iigwX_index size
**		var_data1	release id string of the RMS Gateway
**		error.err_code
**				E_GW0654_BAD_GW_VERSION
**				E_GW0110_GWX_VINIT_ERROR
**              xhandle         memory for RMS fab caching
**      Returns:
**          E_DB_OK             Exit completed normally. 
**          E_DB_ERROR          Exit completed abnormally
**
** History:
**      26-Apr-1989 (alexh)
**          Created.
**	17-apr-90 (bryanp)
**	    Changed error handling.
**	4-nov-91 (rickh)
**	    Return release identifier so that SCF can spit it up when
**	    poked with a "select dbmsinfo( '_version' )"
**      21-sep-92 (schang)
**          add status = rms_fab_cache_init((PTR *)&gwx_rcb->xrcb_xhandle,
**	                        &gwx_rcb->xrcb_error);
**          to init memory for fab caching
**      12-aug-93 (schang)
**          set up flag to inidcate read-regardless-lock 
**      27-sep-93 (schang)
**          to make the server readonly at startup
**      15-may-96 (schang)
**          add another startup logical so that file will be closed if no
**          other user is using it
**      05-jan-04 (chash01)
**          user Ingres version but replace II with RMS
*/
DB_STATUS
gw02_init
(
    GWX_RCB *gwx_rcb
)
{
    ADF_CB	adf_scb;
    DB_STATUS   status;
    char        *string;
    GLOBALREF   Version[];
    char        *v_ptr;

    if (gwx_rcb->xrcb_gwf_version != GWX_VERSION)
    {

#ifdef	E_GW0654_BAD_GW_VERSION
	gw02_log_error( E_GW0654_BAD_GW_VERSION, (struct FAB *)NULL,
			(struct RAB *)NULL, (ULM_RCB *)NULL, (SCF_CB *)NULL );
#endif

	gwx_rcb->xrcb_error.err_code = E_GW0110_GWX_VINIT_ERROR;
	return(E_DB_ERROR);
    }
    /*
    ** schang :  buffer up server startup parameters
    */
    string = NULL;
    PMget("ii.$.rms.*.ii_rms_rrl", &string);
    if( string != NULL )
    {
        if( (*string == 'o' || *string == 'O') &&
             (*(string+1) == 'n' || *(string+1) == 'N')
          )
            gwx_rcb->xrcb_xbitset |= II_RMS_RRL;
    }
    /*
    ** schang : to make the server readonly at startup
    */
    string = NULL;
    PMget("ii.$.rms.*.ii_rms_readonly", &string);
    if (string != NULL)
    {
        if( (*string == 'o' || *string == 'O') &&
            (*(string+1) == 'n' || *(string+1) == 'N')
          )
            gwx_rcb->xrcb_xbitset |= II_RMS_READONLY;
    }
    /*
    ** schang : to decide if server needs to buffer RFA
    */
    string = NULL;
    PMget("ii.$.rms.*.ii_rms_buffer_rfa", &string);
    if (string != NULL)
    {
        if( (*string == 'o' || *string == 'O') &&
            (*(string+1) == 'n' || *(string+1) == 'N')
          )
            gwx_rcb->xrcb_xbitset |= II_RMS_BUFFER_RFA;
    }
    /*
    ** schang : to decide if close file if no user is using it
    */
    string = NULL;
    PMget("ii.$.rms.*.ii_rms_close_file", &string);
    if (string != NULL)
    {
        if( (*string == 'o' || *string == 'O') &&
            (*(string+1) == 'n' || *(string+1) == 'N')
          )
            gwx_rcb->xrcb_xbitset |= II_RMS_CLOSE_FILE;
    }

    (*gwx_rcb->xrcb_exit_table)[GWX_VTERM] = gw02_term;
    (*gwx_rcb->xrcb_exit_table)[GWX_VTABF] = gw02_tabf;
    (*gwx_rcb->xrcb_exit_table)[GWX_VOPEN] = gw02_open;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCLOSE] = gw02_close;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPOSITION] = gw02_position;
    (*gwx_rcb->xrcb_exit_table)[GWX_VGET] = gw02_get;
    (*gwx_rcb->xrcb_exit_table)[GWX_VPUT] = gw02_put;
    (*gwx_rcb->xrcb_exit_table)[GWX_VREPLACE] = gw02_replace;
    (*gwx_rcb->xrcb_exit_table)[GWX_VDELETE] = gw02_delete;
    (*gwx_rcb->xrcb_exit_table)[GWX_VBEGIN] = gw02_begin;
    (*gwx_rcb->xrcb_exit_table)[GWX_VCOMMIT] = gw02_commit;
    (*gwx_rcb->xrcb_exit_table)[GWX_VABORT] = gw02_abort;
    (*gwx_rcb->xrcb_exit_table)[GWX_VINFO] = gw02_info;
#if 0
    (*gwx_rcb->xrcb_exit_table)[GWX_VINFO] = NULL;
#endif

    (*gwx_rcb->xrcb_exit_table)[GWX_VIDXF] = gw02_idxf;

    /* 
    ** this is requested by DaveB for seesion termination
    */
    (*gwx_rcb->xrcb_exit_table)[GWX_VSTERM] = gw02_sterm;
    (*gwx_rcb->xrcb_exit_table)[GWX_VSINIT] = gw02_sinit;

    /* set up tidp tuple for this gateway's extended attribute table. */
    gwx_rcb->xrcb_xatt_tidp = (PTR)&Gwrms_xatt_tidp;

    /* set up required return values */
    gwx_rcb->xrcb_exit_cb_size = sizeof(GWRMS_RSB);
    gwx_rcb->xrcb_xrelation_sz = sizeof(GWRMS_XREL);
    gwx_rcb->xrcb_xattribute_sz = sizeof(GWRMS_XATT);
    gwx_rcb->xrcb_xindex_sz = sizeof(GWRMS_XIDX);

    /*
    ** allocate memory from SCF to build memory pointer array for xhandle
    */
    status = op_scf_mem(SCU_MALLOC,
                        (PTR)&gwx_rcb->xrcb_xhandle,
                        sizeof(PTR)*RMSMAXMEMPTR,
	                &gwx_rcb->xrcb_error);
    if (status != E_DB_OK)
       return(status);
    /*
    ** initialize RFA pool only if startup says so
    */
    if ((gwx_rcb->xrcb_xbitset & II_RMS_READONLY) &&
                (gwx_rcb->xrcb_xbitset & II_RMS_BUFFER_RFA))
        status = rms_rfapool_init(&gwx_rcb->xrcb_xhandle[RMSRFAMEMIDX],
	                        &gwx_rcb->xrcb_error); 
 
    if (status != E_DB_OK)
        return(status);
    status = rms_fab_cache_init(&gwx_rcb->xrcb_xhandle[RMSFABMEMIDX],
	                        &gwx_rcb->xrcb_error);
    if (status != E_DB_OK)
        return(status);

    v_ptr = STindex(Version, "I", 0);
    if ( v_ptr != NULL )
    {
        if ( *(v_ptr++) == 'I' )
        {
            v_ptr++;
            STpolycat(2, "EA RMS", v_ptr, Gwrms_version);
            gwx_rcb->xrcb_var_data1.data_address = Gwrms_version;
            gwx_rcb->xrcb_var_data1.data_out_size = STlength(Gwrms_version);
        }
    }
 
    /* initialize datatypes */
    if (rms_dt_init(&adf_scb) != E_DB_OK)
	return(E_DB_ERROR); /**** need to handle errors here ****/

    return(E_DB_OK);
}


/*{
** Name: gw02_connect - connect to rms file
**
** Description:
**	This routine connects to an rms file.  It is broken out into a separate
**	routine because it may be called from 3 different places:
**	    gw02_position() -- if this is the first position on a file accessed
**		sequentially or by key
**	    gw02_put -- since no position is done before a put
**	    gw02_get -- if access is by tid (for rms, this means random access
**		by RFA)
**	This routine is internal to the GWRMS.C module.
**
** Inputs:
**	gwx_rcb->
**		xrcb_rsb	record stream set up by gwX_open
**		xrcb_rsb->gwrsb_internal_rsb
**				pointer to rms RAB and FAB structs
**
** Output:
**	gwx_rcb->
**	    error.err_code	    Detailed error code if error occurred:
**
**      Returns:
**          E_DB_OK		    Function completed normally. 
**          E_DB_ERROR		    Function completed abnormally
**
** History:
**	18-mar-91 (linda)
**	    Created -- broken out of gw02_position, gw02_put code for
**	    modularity and to support 3rd call from gw02_get for "by tid"
**	    access.
**	20-mar-91 (rickh)
**	    Position relative files at EOF for puts.  Otherwise, we are
**	    default positioned at BOF and get RMS$_REX on puts.
**	9-dec-91 (rickh)
**	    Since record locking options are set here and persist for the
**	    duration of the record stream (i.e., for the whole time this
**	    FAB is opened), they should be set based on information passed
**	    down at table open time.  In particular, a given GET will not
**	    know if it will be followed by a DELETE.  However, the DELETE
**	    will expect the GET to have locked the record.  Therefore, we
**	    query the delete bit in the FAB to determine whether all
**	    operations on this open stream should lock their records.
**      12-jan-92 (schang)
**          when connect, we must sett rab$l_rop according to access mode.
**          If access mode specifies write we should not set RAB$M_NLK.  To
**          make sure, we use &(~RAB$M_NLK) to clear that bit.
**      12-aug-93 (schang)
**          allow read-regardless-lock if flag is set.
*/
static
DB_STATUS
gw02_connect
(
    GWX_RCB	*gwx_rcb,
    i4	caller
)
{
    GW_RSB	*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GWRMS_RSB	*rms_rsb = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    i4		sid;
    DB_STATUS	status = E_DB_OK;
    i4		vms_status;
    
    for (;;)
    {
	/* initialize RAB and sys$connect */

	rms_rsb->rab.rab$b_bid = RAB$C_BID;
	rms_rsb->rab.rab$b_bln = RAB$C_BLN;

	rms_rsb->rab.rab$l_fab = &rms_rsb->fab->rmsfab;

	/*
	** If this record stream was opened with delete access, then
	** any GET or FIND may be followed by a DELETE.  RMS sys$deletes
	** expect the preceding sys$get/sys$find to have locked the record.
	*/
        if (gwx_rcb->xrcb_access_mode == GWT_READ)
	{
            if (gwx_rcb->xrcb_xbitset & II_RMS_RRL)
	        rms_rsb->rab.rab$l_rop = RAB$M_RRL |RAB$M_NLK 
                                        | RAB$M_RAH;
            else 
	        rms_rsb->rab.rab$l_rop = RAB$M_NLK | RAB$M_RAH | RAB$M_WAT;
	}
	else
	{
	    rms_rsb->rab.rab$l_rop = RAB$M_RAH | RAB$M_WAT & (~RAB$M_NLK);
	}

	if (caller == GWX_VPUT)
	{
	    /* For sequential and relative files, connect at EOF before
		putting a record. */
	    if ( (rms_rsb->fab->rmsfab.fab$b_org == FAB$C_SEQ) ||
		 (rms_rsb->fab->rmsfab.fab$b_org == FAB$C_REL)   )
		rms_rsb->rab.rab$l_rop |= RAB$M_EOF;
	}

	/* asynchronous RMS: */
	CSget_sid(&sid);
	rms_rsb->rab.rab$l_ctx = sid;
	rms_rsb->rab.rab$l_rop |= RAB$M_ASY;

	vms_status = rms_rab_suspend(
		sys$connect(&rms_rsb->rab, rms_rab_resume, rms_rab_resume),
		&rms_rsb->rab);
            
	if (vms_status != RMS$_NORMAL)
	{
	    gw02_log_error(E_GW5002_RMS_CONNECT_ERROR,
			(struct FAB *)&rms_rsb->fab->rmsfab,
			(struct RAB *)&rms_rsb->rab,
			(ULM_RCB *)NULL, (SCF_CB *)NULL );
	    gwx_rcb->xrcb_error.err_code = gw02_ingerr(RMS_SYS_CONNECT,
			vms_status, E_GW0111_GWX_VCONNECT_ERROR);
	    status = E_DB_ERROR;
	    break;
	}

	break;
    }

    return (status);
}

/*
** Name: gw02_log_error	    - log an informative GWRMS error message
**
** Description:
**	This routine has the responsibility of logging an informative message
**	about an unexpected error which occurred. We want our error messages
**	to be as complete and detailed as possible. Messages logged by this
**	routine are placed in errlog.log
**
**	Here are some ideas for a couple of enhancements:
**	1) Store the file's name in the rms_rsb somewhere, so that we can
**	    use the file name rather than the IFI, which is pretty useless.
**	2) Depending on the message number, use ULE_MESSAGE and scc_error()
**	    to send the error message to the caller as well as logging it.
**	3) Dump the contents of the bad record or key somewhere (TRdisplay?)
**
** Inputs:
**	error_code	    - which error occurred.
**	fab		    - fab, if one was used, for parameters, or NULL
**	rab		    - rab, if one was used, for parameters, or NULL
**	ulm_rcb		    - ULM info, if this was a ULM error, or NULL
**	scf_cb		    - SCF info, if this was an SCF error, or NULL
**
** Outputs:
**	None, but an error message is logged.
**
** Returns:
**	VOID
**
** History:
**	16-apr-90 (bryanp)
**	    Created.
**	6-jul-90 (linda)
**	    Removed the static declaration so that this can be called from
**	    other modules.
**	29-oct-92 (andre)
**	    change 4-th arg to ule_format() to (DB_SQLSTATE *) NULL since we
**	    never use the value anyway
*/
VOID
gw02_log_error
(
    i4     error_code,
    struct FAB  *fab,
    struct RAB  *rab,
    ULM_RCB     *ulm_rcb,
    SCF_CB      *scf_cb
)
{
    i4		    uletype = ULE_LOG;
    DB_STATUS		    uleret;
    i4		    ulecode;
    i4		    msglen;

    switch( error_code )
    {
	case E_GW5000_RMS_OPEN_ERROR:
	    switch( fab->fab$l_sts )
	    {
		case RMS$_FNF:
		case RMS$_DEV:
		case RMS$_DIR:
		case RMS$_DNF:
		case RMS$_FNM:
		case RMS$_LNE:
		    break;
		default:
		    /* "An error occurred in the SYS$OPEN RMS Service Call.
			The RMS Gateway attempted to open file <filename> with
			file access <hex-access> and file sharing <hex-sharing>.
			The RMS error code was <RMS$whatever>
		    */
		    uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 4,
				fab->fab$b_fns,         fab->fab$l_fna,
				sizeof(fab->fab$b_fac), &fab->fab$b_fac,
				sizeof(fab->fab$b_shr), &fab->fab$b_shr,
				(i4)0, gw02_rmserr(fab->fab$l_sts) );
		break;
	    }
	    break;

	case E_GW5001_RMS_CLOSE_ERROR:
	    switch( fab->fab$l_sts )
	    {
		default:
		    /* "An error occurred in the SYS$CLOSE RMS Service Call.
			The RMS Gateway attempted to close file <filename>
			with file processing options <hex-options>.
			The RMS error code was <RMS$whatever>
		    */
		uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 3,
				fab->fab$b_fns,         fab->fab$l_fna,
				sizeof(fab->fab$l_fop), &fab->fab$l_fop,
				(i4)0, gw02_rmserr(fab->fab$l_sts) );
		break;
	    }
	    break;

	case E_GW5002_RMS_CONNECT_ERROR:
	    switch( rab->rab$l_sts )
	    {
		default:
		    /* "An error occurred in the SYS$CONNECT RMS Service Call.
			The RMS Gateway attempted to connect to file ID=
			<hex IFI> with record options <hex-options>.
			The RMS error code was <RMS$whatever>.
		    */
		uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 3,
				fab->fab$b_fns,         fab->fab$l_fna,
				sizeof(rab->rab$l_rop), &rab->rab$l_rop,
				(i4)0, gw02_rmserr(rab->rab$l_sts) );
		break;
	    }
	    break;

	case E_GW5003_RMS_DISCONNECT_ERROR:
	    switch( rab->rab$l_sts )
	    {
		default:
		    /* "An error occurred in the SYS$DISCONNECT RMS Service
			Call.  The RMS error code was <RMS$whatever>.
		    */
		uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 1,
				(i4)0, gw02_rmserr(rab->rab$l_sts) );
		break;
	    }
	    break;

	case E_GW5004_RMS_FIND_SEQ_ERROR:
	    switch( fab->fab$l_sts )
	    {
		case RMS$_EOF:
		    break;
		default:
		    /* "An error occurred in the SYS$FIND RMS Service Call.
			The RMS Gateway attempted to position stream ID=
			<hex ISI> for a sequential scan.
			The RMS error code was <RMS$whatever>.
		    */
		uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 2,
				sizeof(rab->rab$w_isi), &rab->rab$w_isi,
				(i4)0, gw02_rmserr(rab->rab$l_sts) );
		break;
	    }
	    break;

	case E_GW5005_RMS_FIND_KEY_ERROR:
	    switch( rab->rab$l_sts )
	    {
		default:
		    /* "An error occurred in the SYS$FIND RMS Service Call.
			The RMS Gateway attempted to position stream ID=<hex
			ISI> on key <hex KRF> with value <low key value>.
			The RMS error code was <RMS$whatever>.
		    */
		uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 4,
				sizeof(rab->rab$w_isi), &rab->rab$w_isi,
				sizeof(rab->rab$b_krf), &rab->rab$b_krf,
				sizeof(rab->rab$b_ksz), &rab->rab$b_ksz,
				(i4)0, fab->fab$l_fna);
#if 0
      gw02_rmserr(rab->rab$l_sts) );
#endif
		break;
	    }
	    break;

	case E_GW5006_RMS_GET_ERROR:
	    switch( fab->fab$l_sts )
	    {
		case RMS$_EOF:
		    break;
		default:
		    /* "An error occurred in the SYS$GET RMS Service Call.
			The RMS Gateway attempted to get a record from file
			ID=<hex IFI> into the buffer at <hex bufaddr> whose
			length is <hex buflen> with record access mode <hex
			rac>.  The RMS error code was <RMS$whatever>
		    */
		uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype,
				    (DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				    &msglen, &ulecode, 5,
				    fab->fab$b_fns,         fab->fab$l_fna,
				    sizeof(rab->rab$l_ubf), &rab->rab$l_ubf,
				    sizeof(rab->rab$w_usz), &rab->rab$w_usz,
				    sizeof(rab->rab$b_rac), &rab->rab$b_rac,
				    (i4)0, gw02_rmserr(rab->rab$l_sts) );
		break;
	    }
	    break;

	case E_GW5007_RMS_PUT_ERROR:
	    switch( rab->rab$l_sts )
	    {
		case RMS$_RSZ:
		    break;
		default:
		    /* "An error occurred in the SYS$PUT RMS Service Call.
			The RMS Gateway attempted to put a record into file
			ID=<hex IFI> from the buffer at <hex bufaddr> whose
			length is <hex buflen> with record access mode
			<hex rac>.  The RMS error code was <RMS$whatever>
		    */
		    uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 5,
				fab->fab$b_fns,         fab->fab$l_fna,
				sizeof(rab->rab$l_rbf), &rab->rab$l_rbf,
				sizeof(rab->rab$w_rsz), &rab->rab$w_rsz,
				sizeof(rab->rab$b_rac), &rab->rab$b_rac,
				(i4)0, gw02_rmserr(rab->rab$l_sts) );
		    break;
	    }
	    break;

	case E_GW5008_RMS_UPDATE_ERROR:
	    switch( rab->rab$l_sts )
	    {
		case RMS$_RSZ:
		    break;
		default:
		    /* "An error occurred in the SYS$UPDATE RMS Service Call.
			The RMS Gateway attempted to update a record in file
			<file name> from the buffer at <hex bufaddr> whose
			length is <hex buflen> with record access mode 
			<hex rac>.  The RMS error code was <RMS$whatever>
		    */
		    uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 5,
				fab->fab$b_fns,         fab->fab$l_fna,
				sizeof(rab->rab$l_rbf), &rab->rab$l_rbf,
				sizeof(rab->rab$w_rsz), &rab->rab$w_rsz,
				sizeof(rab->rab$b_rac), &rab->rab$b_rac,
				(i4)0, gw02_rmserr(rab->rab$l_sts) );
		    break;
	    }
	    break;

	case E_GW5009_RMS_DELETE_ERROR:
	    switch( rab->rab$l_sts )
	    {
		default:
		    /* "An error occurred in the SYS$DELETE RMS Service Call.
			The RMS Gateway attempted to delete a record in file
			ID=<hex IFI> with record processing options <hex rop>.
			The RMS error code was <RMS$whatever>
		    */
		uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 3,
				fab->fab$b_fns,         fab->fab$l_fna,
				sizeof(rab->rab$l_rop), &rab->rab$l_rop,
				(i4)0, gw02_rmserr(rab->rab$l_sts) );
		break;
	    }
	    break;

	case E_GW500A_RMS_FLUSH_ERROR:
	    switch( rab->rab$l_sts )
	    {
		default:
		    /* "An error occurred in the SYS$FLUSH RMS Service Call.
			The RMS Gateway attempted to flush the I/O Buffers for
			file ID=<hex IFI> associated with stream ID=<hex ISI>
			and with record processing options <hex rop>.
			The RMS error code was <RMS$whatever>
		    */
		uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 4,
				fab->fab$b_fns,         fab->fab$l_fna,
				sizeof(rab->rab$w_isi), &rab->rab$w_isi,
				sizeof(rab->rab$l_rop), &rab->rab$l_rop,
				(i4)0, gw02_rmserr(rab->rab$l_sts) );
		break;
	    }
	    break;

	case E_GW500C_RMS_CVT_TO_ING_ERROR:
		/* "An error occurred converting an RMS record to Ingres
		    format. Record <hex recnum> of size <hex recsize>
		    from file ID=<hex IFI> could not be converted.
		*/
	    uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype,
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 3,
				sizeof(rab->rab$l_bkt), &rab->rab$l_bkt,
				sizeof(rab->rab$w_rsz), &rab->rab$w_rsz,
				fab->fab$b_fns,         fab->fab$l_fna);
	    break;

	case E_GW0314_ULM_PALLOC_ERROR:
	    uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 4,
		    sizeof(*(ulm_rcb->ulm_memleft)), ulm_rcb->ulm_memleft,
		    sizeof(ulm_rcb->ulm_sizepool),   &ulm_rcb->ulm_sizepool,
		    sizeof(ulm_rcb->ulm_blocksize),  &ulm_rcb->ulm_blocksize,
		    sizeof(ulm_rcb->ulm_psize),      &ulm_rcb->ulm_psize);
	    break;

	case E_GW0300_SCU_MALLOC_ERROR:
	    uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 1,
		      sizeof(scf_cb->scf_scm.scm_in_pages),
		      &scf_cb->scf_scm.scm_in_pages);
	    break;

	case E_GW500B_RMS_FILE_NOT_INDEXED:
	    /* "The RMS Gateway was requested to perform an indexed search
		on file ID=<hex IFI>, but the file's organization (<hex org>)
		does not indicate an indexed structure. The search operation
		is rejected.
	    */
	    uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 2,
				fab->fab$b_fns,         fab->fab$l_fna,
				sizeof(fab->fab$b_org), &fab->fab$b_org);
	    break;
	default:
	    /* should never get here? or just log msg with no parms? */
	    uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 0);
	    break;
    }

    if (uleret != E_DB_OK)
    {
	/*
	** An error occurred in ule_format. At least try to report that
	** error...
	*/
	_VOID_ ule_format( ulecode, (CL_ERR_DESC *)0, ULE_LOG, 
			    (DB_SQLSTATE *) NULL, (char *)0, (i4)0,
			    &msglen, &ulecode, 0 );
    }

    return;
}

/*
** Name: gw02_log_chkacc_error	    - log an error from sys$check_access
**
** Description:
**	This routine has the responsibility of logging an informative message
**	about an error sys$check_access -- namely, no privs.
**
** Inputs:
**	error_code	    - which error occurred.
**	access_objtyp	    - object type
**	access_objnam	    - object name
**    	access_usrnam	    - user name
**	access_type	    - access type (read or write)
**
** Outputs:
**	None, but an error message is logged.
**
** Returns:
**	VOID
**
** History:
**	16-apr-90 (bryanp)
**	    Created.
**	29-oct-92 (andre)
**	    change 4-th arg to ule_format() to (DB_SQLSTATE *) NULL since we
**	    never use the value anyway
*/
static VOID
gw02_log_chkacc_error
(
    i4                  error_code,
    i4                  vms_status,
    i4                  *access_objtyp,
    struct dsc$descriptor_s  *access_objnam,
    struct dsc$descriptor_s  *access_usrnam,
    i4                  access_type
)
{
    i4		    uletype = ULE_LOG;
    DB_STATUS		    uleret;
    i4		    ulecode;
    i4		    msglen;

    switch (vms_status)
    {
	case	SS$_NORMAL:
	case	SS$_NOPRIV:
	case	RMS$_DEV:
	case	RMS$_FNF:
	case	RMS$_FNM:
	case	RMS$_DNF:
	case	RMS$_DIR:
	case	RMS$_LNE:
	    break;

	default:
	/*
	**	E_GW500F_CHECK_ACCESS_ERROR:
	**		   "An error occurred in the SYS$CHECK_ACCESS System
	**		   Service Call.  The RMS Gateway attempted to open
	**		   file <filename> on behalf of user <username>.  The
	**		   error code was SS$_NOPRIV for access type <access
	**		   type>.
	*/
	    uleret = ule_format(error_code, (CL_ERR_DESC *)0, uletype, 
				(DB_SQLSTATE *) NULL, (char *)0, (i4)0,
				&msglen, &ulecode, 4,
				(i4)0, access_objnam->dsc$a_pointer,
				access_usrnam->dsc$w_length,
				access_usrnam->dsc$a_pointer,
				(i4)0, (access_type == ARM$M_READ) ?
				    "ARM$M_READ" : "ARM$M_WRITE",
				(i4)4, vms_status);
    }
    return;
}

/*
** Name: gw02_ingerr	    - translate RMS error code into Ingres errcode
**
** Description:
**	If possible, we want to return to the user a 'friendly' error code when
**	something goes wrong. Certain error codes are known about by higher
**	levels (DMF, QEF, ...) and are mapped back into friendly error codes.
**	Other error codes are unknown and result in the user receiving garbage
**	like (SCF_XXX an unknown error has occurred). So we want to try to
**	return a nice error code when an recoverable or 'expected' error
**	occurs.
**
**	This routine takes an RMS error code and the ID of the function in
**	which the error occurred, and maps the RMS error code into an
**	'appropriate' Ingres error code. If no appropriate error code is found,
**	a default error code is returned.
**
**	In general, error codes which are mapped in this routine are those
**	which are 'expected' to occur under normal use; those error codes which
**	result from program bugs, or hardware errors, or phases of the moon
**	need not be mapped here -- those errors are completely logged in
**	errlog.log and the customer can take corrective action by reading the
**	error message there. We need not try to deliver a friendly error
**	message to the user under those circumstances.
**
**	At least for the time being, we are also proceeding under the theory
**	that the RMS-specific error message has the greatest likelihood of
**	being a useful error message for the user; that is, that the user will
**	understand the RMS-specific error message more than he/she whill
**	understand any of the various 'interpretations' which occur in higher
**	layers of the software. Thus we also take this opportunity to send the
**	RMS status code to gw02_errsend() so that the user will see the RMS
**	error message unfiltered.
**
** Inputs:
**	rms_opcode	    - which RMS operation was in progress, one of
**			      the RMS_* equates should be used.
**	vms_status	    - the VMS status code which was received.
**	default_errcode	    - the error code which should be used if no
**			      appropriate error code can be found.
**
** Outputs:
**	None
**
** Returns:
**	err_code	    - either the default error code, or a more
**			      appropriate error code if one can be found.
**
** History:
**	16-apr-90 (bryanp)
**	    Created.
**	19-apr-90 (bryanp)
**	    Send the RMS-specific message directly to the user via gw02_errsend.
*/
static i4
gw02_ingerr
(
    i4 rms_opcode,
    i4 vms_status,
    i4 default_errcode
)
{
    i4	err_code;

    /*
    ** Send the RMS-specific error message directly to the user:
    */
    gw02_errsend( vms_status );

    /*
    ** Now attempt to translate it into a useful-to-ingres error code.
    ** If we fail to do so, we will return the default error code:
    */

    err_code = default_errcode;

    switch( rms_opcode )
    {
	case RMS_SYS_OPEN:
	    switch( vms_status )
	    {
		case RMS$_ACC:
		    err_code = E_GW5400_RMS_FILE_ACCESS_ERROR;
		    break;
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_DEV:
		    err_code = E_GW5402_RMS_BAD_DEVICE_TYPE;
		    break;
		case RMS$_DIR:
		    err_code = E_GW5403_RMS_BAD_DIRECTORY_NAME;
		    break;
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		case RMS$_DNF:
		    err_code = E_GW5405_RMS_DIR_NOT_FOUND;
		    break;
		case RMS$_FLK:
		    err_code = E_GW5406_RMS_FILE_LOCKED;
		    break;
		case RMS$_FNF:
		    err_code = E_GW5407_RMS_FILE_NOT_FOUND;
		    break;
		case RMS$_FNM:
		case RMS$_SYN:
		    err_code = E_GW5408_RMS_BAD_FILE_NAME;
		    break;
		case RMS$_LNE:
		    err_code = E_GW5409_RMS_LOGICAL_NAME_ERROR;
		    break;
		case RMS$_PRV:
		    err_code = E_GW540A_RMS_FILE_PROT_ERROR;
		    break;
		case RMS$_SUPERSEDE:
		    err_code = E_GW540B_RMS_FILE_SUPERSEDED;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_CLOSE:
	    switch( vms_status )
	    {
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		case RMS$_EXENQLM:
		    err_code = E_GW540C_RMS_ENQUEUE_LIMIT;
		    break;
		case RMS$_PRV:
		    err_code = E_GW540A_RMS_FILE_PROT_ERROR;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_CONNECT:
	    switch( vms_status )
	    {
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_CRMP:
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		case RMS$_EXENQLM:
		    err_code = E_GW540C_RMS_ENQUEUE_LIMIT;
		    break;
		case RMS$_PRV:
		    err_code = E_GW540A_RMS_FILE_PROT_ERROR;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_DISCONNECT:
	    switch( vms_status )
	    {
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_FIND:
	    switch( vms_status )
	    {
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_DEADLOCK:
		    err_code = E_GW540D_RMS_DEADLOCK;
		    break;
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		case RMS$_EOF:
		    err_code = E_GW0641_END_OF_STREAM;
		    break;
		case RMS$_EXENQLM:
		    err_code = E_GW540C_RMS_ENQUEUE_LIMIT;
		    break;
		case RMS$_OK_ALK:
		    err_code = E_GW540E_RMS_ALREADY_LOCKED;
		    break;
		case RMS$_OK_DEL:
		    err_code = E_GW540F_RMS_PREVIOUSLY_DELETED;
		    break;
		case RMS$_OK_RLK:
		case RMS$_OK_RRL:
		    err_code = E_GW5410_RMS_READ_LOCKED_RECORD;
		    break;
		case RMS$_OK_RNF:
		    err_code = E_GW5411_RMS_RECORD_NOT_FOUND;
		    break;
		case RMS$_OK_WAT:
		    err_code = E_GW5412_RMS_READ_AFTER_WAIT;
		    break;
		case RMS$_RLK:
		    err_code = E_GW5413_RMS_RECORD_LOCKED;
		    break;
		case RMS$_TMO:
		    err_code = E_GW5414_RMS_TIMED_OUT;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_GET:
	    switch( vms_status )
	    {
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_DEADLOCK:
		    err_code = E_GW540D_RMS_DEADLOCK;
		    break;
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		case RMS$_EOF:
		    err_code = E_GW0641_END_OF_STREAM;
		    break;
		case RMS$_EXENQLM:
		    err_code = E_GW540C_RMS_ENQUEUE_LIMIT;
		    break;
		case RMS$_OK_ALK:
		    err_code = E_GW540E_RMS_ALREADY_LOCKED;
		    break;
		case RMS$_OK_DEL:
		    err_code = E_GW540F_RMS_PREVIOUSLY_DELETED;
		    break;
		case RMS$_OK_RLK:
		case RMS$_OK_RRL:
		    err_code = E_GW5410_RMS_READ_LOCKED_RECORD;
		    break;
		case RMS$_OK_RNF:
		    err_code = E_GW5411_RMS_RECORD_NOT_FOUND;
		    break;
		case RMS$_OK_WAT:
		    err_code = E_GW5412_RMS_READ_AFTER_WAIT;
		    break;
		case RMS$_RLK:
		    err_code = E_GW5413_RMS_RECORD_LOCKED;
		    break;
		case RMS$_RTB:
		    err_code = E_GW5415_RMS_RECORD_TOO_BIG;
		    break;
		case RMS$_TMO:
		    err_code = E_GW5414_RMS_TIMED_OUT;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_PUT:
	    switch( vms_status )
	    {
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		case RMS$_DUP:
		    err_code = E_GW5416_RMS_INVALID_DUP_KEY;
		    break;
		case RMS$_FUL:
		    err_code = E_GW5417_RMS_DEVICE_FULL;
		    break;
		case RMS$_OK_ALK:
		    err_code = E_GW540E_RMS_ALREADY_LOCKED;
		    break;
		case RMS$_OK_DUP:
		    err_code = E_GW5418_RMS_DUPLICATE_KEY;
		    break;
		case RMS$_OK_IDX:
		    err_code = E_GW5419_RMS_INDEX_UPDATE_ERROR;
		    break;
		case RMS$_RLK:
		    err_code = E_GW5413_RMS_RECORD_LOCKED;
		    break;
		case RMS$_RSZ:
		    err_code = E_GW541D_RMS_INV_REC_SIZE;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_UPDATE:
	    switch( vms_status )
	    {
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_CUR:
		    err_code = E_GW541A_RMS_NO_CURRENT_RECORD;
		    break;
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		case RMS$_DUP:
		    err_code = E_GW5416_RMS_INVALID_DUP_KEY;
		    break;
		case RMS$_OK_ALK:
		    err_code = E_GW540E_RMS_ALREADY_LOCKED;
		    break;
		case RMS$_OK_DUP:
		    err_code = E_GW5418_RMS_DUPLICATE_KEY;
		    break;
		case RMS$_OK_IDX:
		    err_code = E_GW5419_RMS_INDEX_UPDATE_ERROR;
		    break;
		case RMS$_RSZ:
		    err_code = E_GW541E_RMS_INV_UPD_SIZE;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_DELETE:
	    switch( vms_status )
	    {
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_CUR:
		    err_code = E_GW541A_RMS_NO_CURRENT_RECORD;
		    break;
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_FLUSH:
	    switch( vms_status )
	    {
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_CHECK_ACCESS:
	    switch( vms_status )
	    {
		/*
		** Yes, we know the documentation says you can only get SS$_*
		** codes. In real life, the RMS codes come back as well...
		*/
		case SS$_NOPRIV:
		    err_code = E_GW540A_RMS_FILE_PROT_ERROR;
		    break;
		case RMS$_DEV:
		    err_code = E_GW5402_RMS_BAD_DEVICE_TYPE;
		    break;
		case RMS$_DIR:
		    err_code = E_GW5403_RMS_BAD_DIRECTORY_NAME;
		    break;
		case RMS$_DNF:
		    err_code = E_GW5405_RMS_DIR_NOT_FOUND;
		    break;
		case RMS$_FNF:
		    err_code = E_GW5407_RMS_FILE_NOT_FOUND;
		    break;
		case RMS$_FNM:
		case RMS$_SYN:
		    err_code = E_GW5408_RMS_BAD_FILE_NAME;
		    break;
		case RMS$_LNE:
		    err_code = E_GW5409_RMS_LOGICAL_NAME_ERROR;
		    break;
		default:
		    break;
	    }
	    break;

	case RMS_SYS_REWIND:
	    switch( vms_status )
	    {
		case RMS$_ACT:
		    err_code = E_GW5401_RMS_FILE_ACT_ERROR;
		    break;
		case RMS$_DEADLOCK:
		    err_code = E_GW540D_RMS_DEADLOCK;
		    break;
		case RMS$_DME:
		    err_code = E_GW5404_RMS_OUT_OF_MEMORY;
		    break;
		default:
		    break;
	    }
	    break;

	default:
	    break;
    }

    return (err_code);
}

/*
** Name: gw02_rmserr	    - translate RMS error code into human format
**
** Description:
**	RMS error codes are unsigned integers. As unsigned integers, they are
**	less than legible. This routine translates them into symbolic string
**	representations which is MUCH more useful to humans :-)
**
**	This routine is only called during error processing; that is, the RMS
**	error code is only translated to a string for use as an error message
**	parameter, so the bulging of code caused by this procedure should not
**	cause any performance hit under normal processing.
**
**	If the RMS error code is not in our standard list, we return a pointer
**	to a static string. This is of slight danger in a multi-threaded
**	environment, but the worst danger is a wrongly-formatted error message,
**	so we live with that and hope we have all the important RMS errors in
**	our standard list.
**
** Inputs:
**	rms_error	    - the RMS error code
**
** Outputs:
**	None
**
** Returns:
**	rms_mnemonic	    - a string which describes the RMS error
**
** History:
**	16-apr-90 (bryanp)
**	    Created.
**      12-jan-92 (schang)
**          remove commented out error code
*/
static char *
gw02_rmserr
(
    u_i4   rms_error
)
{
    static	char	rms_mnemonic	[100];

    switch ( rms_error )
    {
	case RMS$_ACC:		return ("RMS$_ACC");
	case RMS$_ACS:		return ("RMS$_ACS");
	case RMS$_ACT:		return ("RMS$_ACT");
	case RMS$_AID:		return ("RMS$_AID");
	case RMS$_ALN:		return ("RMS$_ALN");
	case RMS$_ALQ:		return ("RMS$_ALQ");
	case RMS$_ANI:		return ("RMS$_ANI");
	case RMS$_AOP:		return ("RMS$_AOP");
	case RMS$_ATR:		return ("RMS$_ATR");
	case RMS$_ATW:		return ("RMS$_ATW");
	case RMS$_BES:		return ("RMS$_BES");
	case RMS$_BKS:		return ("RMS$_BKS");
	case RMS$_BKZ:		return ("RMS$_BKZ");
	case RMS$_BLN:		return ("RMS$_BLN");
	case RMS$_BOF:		return ("RMS$_BOF");
	case RMS$_BUG:		return ("RMS$_BUG");
	case RMS$_BUG_DAP:	return ("RMS$_BUG_DAP");
	case RMS$_BUG_DDI:	return ("RMS$_BUG_DDI");
/*	case RMS$_ABO:		return ("RMS$_ABO");
	case RMS$_CJF:		return ("RMS$_CJF");
	case RMS$_FILPURGED:	return ("RMS$_FILPURGED");
	case RMS$_JNF:		return ("RMS$_JNF");
	case RMS$_LBL:		return ("RMS$_LBL");
	case RMS$_BUG_XX1:	return ("RMS$_BUG_XX1");
	case RMS$_BUG_XX2:	return ("RMS$_BUG_XX2");
	case RMS$_BUG_XX3:	return ("RMS$_BUG_XX3");
	case RMS$_BUG_XX4:	return ("RMS$_BUG_XX4");
	case RMS$_BUG_XX5:	return ("RMS$_BUG_XX5");
	case RMS$_BUG_XX6:	return ("RMS$_BUG_XX6");
	case RMS$_BUG_XX7:	return ("RMS$_BUG_XX7");
	case RMS$_BUG_XX8:	return ("RMS$_BUG_XX8");*/
	case RMS$_BUSY:		return ("RMS$_BUSY");
	case RMS$_CCF:		return ("RMS$_CCF");
	case RMS$_CCR:		return ("RMS$_CCR");
	case RMS$_CDA:		return ("RMS$_CDA");
	case RMS$_CHG:		return ("RMS$_CHG");
	case RMS$_CHK:		return ("RMS$_CHK");
	case RMS$_CHN:		return ("RMS$_CHN");
	case RMS$_COD:		return ("RMS$_COD");
	case RMS$_CONTROLC:	return ("RMS$_CONTROLC");
	case RMS$_CONTROLO:	return ("RMS$_CONTROLO");
	case RMS$_CONTROLY:	return ("RMS$_CONTROLY");
	case RMS$_CRC:		return ("RMS$_CRC");
	case RMS$_CRE:		return ("RMS$_CRE");
	case RMS$_CREATED:	return ("RMS$_CREATED");
	case RMS$_CRE_STM:	return ("RMS$_CRE_STM");
	case RMS$_CRMP:		return ("RMS$_CRMP");
	case RMS$_CUR:		return ("RMS$_CUR");
	case RMS$_DAC:		return ("RMS$_DAC");
	case RMS$_DAN:		return ("RMS$_DAN");
	case RMS$_DEADLOCK:	return ("RMS$_DEADLOCK");
	case RMS$_DEL:		return ("RMS$_DEL");
	case RMS$_DEV:		return ("RMS$_DEV");
	case RMS$_DFL:		return ("RMS$_DFL");
	case RMS$_DIR:		return ("RMS$_DIR");
	case RMS$_DME:		return ("RMS$_DME");
	case RMS$_DNA:		return ("RMS$_DNA");
	case RMS$_DNF:		return ("RMS$_DNF");
	case RMS$_DNR:		return ("RMS$_DNR");
	case RMS$_DPE:		return ("RMS$_DPE");
	case RMS$_DTP:		return ("RMS$_DTP");
	case RMS$_DUP:		return ("RMS$_DUP");
	case RMS$_DVI:		return ("RMS$_DVI");
	case RMS$_ENQ:		return ("RMS$_ENQ");
	case RMS$_ENT:		return ("RMS$_ENT");
	case RMS$_ENV:		return ("RMS$_ENV");
	case RMS$_EOF:		return ("RMS$_EOF");
	case RMS$_ESA:		return ("RMS$_ESA");
	case RMS$_ESL:		return ("RMS$_ESL");
	case RMS$_ESS:		return ("RMS$_ESS");
	case RMS$_EXENQLM:	return ("RMS$_EXENQLM");
	case RMS$_EXP:		return ("RMS$_EXP");
	case RMS$_EXT:		return ("RMS$_EXT");
	case RMS$_FAB:		return ("RMS$_FAB");
	case RMS$_FAC:		return ("RMS$_FAC");
	case RMS$_FEX:		return ("RMS$_FEX");
	case RMS$_FLG:		return ("RMS$_FLG");
	case RMS$_FLK:		return ("RMS$_FLK");
	case RMS$_FNA:		return ("RMS$_FNA");
	case RMS$_FND:		return ("RMS$_FND");
	case RMS$_FNF:		return ("RMS$_FNF");
	case RMS$_FNM:		return ("RMS$_FNM");
	case RMS$_FOP:		return ("RMS$_FOP");
	case RMS$_FSZ:		return ("RMS$_FSZ");
	case RMS$_FTM:		return ("RMS$_FTM");
	case RMS$_FUL:		return ("RMS$_FUL");
	case RMS$_GBC:		return ("RMS$_GBC");
	case RMS$_IAL:		return ("RMS$_IAL");
	case RMS$_IAN:		return ("RMS$_IAN");
	case RMS$_IBF:		return ("RMS$_IBF");
	case RMS$_IBK:		return ("RMS$_IBK");
	case RMS$_IDR:		return ("RMS$_IDR");
	case RMS$_IDX:		return ("RMS$_IDX");
	case RMS$_IFA:		return ("RMS$_IFA");
	case RMS$_IFF:		return ("RMS$_IFF");
	case RMS$_IFI:		return ("RMS$_IFI");
	case RMS$_IFL:		return ("RMS$_IFL");
	case RMS$_IMX:		return ("RMS$_IMX");
	case RMS$_INCOMPSHR:	return ("RMS$_INCOMPSHR");
	case RMS$_IOP:		return ("RMS$_IOP");
	case RMS$_IRC:		return ("RMS$_IRC");
	case RMS$_ISI:		return ("RMS$_ISI");
	case RMS$_JNS:		return ("RMS$_JNS");
	case RMS$_JOP:		return ("RMS$_JOP");
	case RMS$_KBF:		return ("RMS$_KBF");
	case RMS$_KEY:		return ("RMS$_KEY");
	case RMS$_KFF:		return ("RMS$_KFF");
	case RMS$_KRF:		return ("RMS$_KRF");
	case RMS$_KSI:		return ("RMS$_KSI");
	case RMS$_KSZ:		return ("RMS$_KSZ");
	case RMS$_LAN:		return ("RMS$_LAN");
	case RMS$_LEX:		return ("RMS$_LEX");
	case RMS$_LNE:		return ("RMS$_LNE");
/*	case RMS$_LOC:		return ("RMS$_LOC");
	case RMS$_PRM:		return ("RMS$_PRM");
	case RMS$_STK:		return ("RMS$_STK");
	case RMS$_KNM:		return ("RMS$_KNM");
	case RMS$_WSF:		return ("RMS$_WSF");
	case RMS$_XCR:		return ("RMS$_XCR");
	case RMS$_NID:		return ("RMS$_NID");
	case RMS$_VOL:		return ("RMS$_VOL");
	case RMS$_NOJ:		return ("RMS$_NOJ"); */
	case RMS$_LWC:		return ("RMS$_LWC");
	case RMS$_MBC:		return ("RMS$_MBC");
	case RMS$_MKD:		return ("RMS$_MKD");
	case RMS$_MRN:		return ("RMS$_MRN");
	case RMS$_MRS:		return ("RMS$_MRS");
	case RMS$_NAM:		return ("RMS$_NAM");
	case RMS$_NEF:		return ("RMS$_NEF");
	case RMS$_NET:		return ("RMS$_NET");
	case RMS$_NETFAIL:	return ("RMS$_NETFAIL");
	case RMS$_NMF:		return ("RMS$_NMF");
	case RMS$_NOD:		return ("RMS$_NOD");
	case RMS$_NORMAL:	return ("RMS$_NORMAL");
	case RMS$_NOVALPRS:	return ("RMS$_NOVALPRS");
	case RMS$_NPK:		return ("RMS$_NPK");
	case RMS$_NRU:		return ("RMS$_NRU");
	case RMS$_OK_ALK:	return ("RMS$_OK_ALK");
	case RMS$_OK_DEL:	return ("RMS$_OK_DEL");
	case RMS$_OK_DUP:	return ("RMS$_OK_DUP");
	case RMS$_OK_IDX:	return ("RMS$_OK_IDX");
	case RMS$_OK_LIM:	return ("RMS$_OK_LIM");
	case RMS$_OK_NOP:	return ("RMS$_OK_NOP");
	case RMS$_OK_RLK:	return ("RMS$_OK_RLK");
	case RMS$_OK_RNF:	return ("RMS$_OK_RNF");
	case RMS$_OK_RRL:	return ("RMS$_OK_RRL");
	case RMS$_OK_RULK:	return ("RMS$_OK_RULK");
	case RMS$_OK_WAT:	return ("RMS$_OK_WAT");
	case RMS$_ORD:		return ("RMS$_ORD");
	case RMS$_ORG:		return ("RMS$_ORG");
	case RMS$_OVRDSKQUOTA:	return ("RMS$_OVRDSKQUOTA");
	case RMS$_PBF:		return ("RMS$_PBF");
	case RMS$_PENDING:	return ("RMS$_PENDING");
	case RMS$_PES:		return ("RMS$_PES");
	case RMS$_PLG:		return ("RMS$_PLG");
	case RMS$_PLV:		return ("RMS$_PLV");
	case RMS$_POS:		return ("RMS$_POS");
	case RMS$_PRV:		return ("RMS$_PRV");
	case RMS$_QUO:		return ("RMS$_QUO");
	case RMS$_RAB:		return ("RMS$_RAB");
	case RMS$_RAC:		return ("RMS$_RAC");
	case RMS$_RAT:		return ("RMS$_RAT");
	case RMS$_RBF:		return ("RMS$_RBF");
	case RMS$_REENT:        return ("RMS$_REENT");
	case RMS$_REF:		return ("RMS$_REF");
	case RMS$_RER:		return ("RMS$_RER");
	case RMS$_REX:		return ("RMS$_REX");
	case RMS$_RFA:		return ("RMS$_RFA");
	case RMS$_RFM:		return ("RMS$_RFM");
	case RMS$_RHB:		return ("RMS$_RHB");
	case RMS$_RLF:		return ("RMS$_RLF");
	case RMS$_RLK:		return ("RMS$_RLK");
	case RMS$_RMV:		return ("RMS$_RMV");
	case RMS$_RNF:		return ("RMS$_RNF");
	case RMS$_RNL:		return ("RMS$_RNL");
	case RMS$_ROP:		return ("RMS$_ROP");
	case RMS$_RPL:		return ("RMS$_RPL");
	case RMS$_RRV:		return ("RMS$_RRV");
	case RMS$_RSA:		return ("RMS$_RSA");
	case RMS$_RSL:		return ("RMS$_RSL");
	case RMS$_RSS:		return ("RMS$_RSS");
	case RMS$_RST:		return ("RMS$_RST");
	case RMS$_RSZ:		return ("RMS$_RSZ");
	case RMS$_RTB:		return ("RMS$_RTB");
	case RMS$_RUM:		return ("RMS$_RUM");
	case RMS$_RVU:		return ("RMS$_RVU");
	case RMS$_SEG:		return ("RMS$_SEG");
	case RMS$_SEQ:		return ("RMS$_SEQ");
	case RMS$_SHR:		return ("RMS$_SHR");
	case RMS$_SIZ:		return ("RMS$_SIZ");
	case RMS$_SNE:		return ("RMS$_SNE");
	case RMS$_SPE:		return ("RMS$_SPE");
	case RMS$_SPL:		return ("RMS$_SPL");
	case RMS$_SQO:		return ("RMS$_SQO");
	case RMS$_STALL:        return ("RMS$_STALL");
	case RMS$_STR:		return ("RMS$_STR");
	case RMS$_SUP:		return ("RMS$_SUP");
	case RMS$_SUPERSEDE:	return ("RMS$_SUPERSEDE");
	case RMS$_SUPPORT:	return ("RMS$_SUPPORT");
	case RMS$_SYN:		return ("RMS$_SYN");
	case RMS$_SYS:		return ("RMS$_SYS");
	case RMS$_TMO:		return ("RMS$_TMO");
	case RMS$_TMR:		return ("RMS$_TMR");
	case RMS$_TNS:		return ("RMS$_TNS");
	case RMS$_TRE:		return ("RMS$_TRE");
	case RMS$_TYP:		return ("RMS$_TYP");
	case RMS$_UBF:		return ("RMS$_UBF");
	case RMS$_UPI:		return ("RMS$_UPI");
	case RMS$_USZ:		return ("RMS$_USZ");
	case RMS$_VER:		return ("RMS$_VER");
	case RMS$_WBE:		return ("RMS$_WBE");
	case RMS$_WCC:		return ("RMS$_WCC");
	case RMS$_WER:		return ("RMS$_WER");
	case RMS$_WLD:		return ("RMS$_WLD");
	case RMS$_WLK:		return ("RMS$_WLK");
	case RMS$_WPL:		return ("RMS$_WPL");
	case RMS$_XAB:		return ("RMS$_XAB");

	default:
	    STprintf(rms_mnemonic, "UNKNOWN (0x%x)", rms_error );
	    return (rms_mnemonic );
	}

    /*NOTREACHED*/
}

/*
** Name: gw02_errsend	    - send RMS error message directly to user.
**
** Description:
**	This routine takes an RMS error status code and format the
**	corresponding VMS error message and sends the message text directly to
**	the user.  The theory here is that the RMS error message best describes
**	the low level error which occurred. The reality is that we do run the
**	risk of inundating the user with messages and saturating them with
**	detail. However, at least for now, that's a risk we're willing to run.
**
** Inputs:
**	vms_status	    - RMS error status code
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** Side Effects:
**	An error message is sent to the user.
**
** History:
**	19-apr-90 (bryanp)
**	    Created.
**      12-jan-92 (schang)
**          remove commented-out code
*/
static VOID
gw02_errsend
(
    i4 vms_status
)
{
    char	error_message [ER_MAX_LEN];
    DB_STATUS	status;
    i4	local_error;
    SCF_CB	scf_cb;
    i4	getmsg_status;
    i4	length;
    DESCRIPTOR	msg_desc;
 
    /*
    ** sys$getmsg only fills in the lower 2 bytes, we'll initialize the whole
    ** thing just in case.
    */
    length = 0;
 
    msg_desc.desc_length = sizeof(error_message);
    msg_desc.desc_value = (char *)error_message;

    getmsg_status = sys$getmsg( vms_status, &length, &msg_desc, 0xf, 0);

    if ((getmsg_status & 1) == 0)
    {
	/* get-msg crapped out. Why? */
        if (getmsg_status == SS$_BUFFEROVF)
        {
	    status = ER_TRUNCATED;
	}
	else
	{
	    status = ER_BADPARAM;
	}
    }
    else
    {
	status = E_DB_OK;

	/*
	** sys$getmsg doesn't null-terminate the message. Do so:
	*/
	error_message[length] = EOS;
    }

    if (status == E_DB_OK)
    {
	/*
	** Only try to send the message to the front-end if it was correctly
	** formatted.
	*/

	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_GWF_ID;
	scf_cb.scf_session = DB_NOSESSION;

	scf_cb.scf_len_union.scf_blength       = length;
	scf_cb.scf_ptr_union.scf_buffer        = (PTR)error_message;

	/*
	** Note: This 'error message' is NOT a properly formatted Ingres error
	** message. Rather, it is a VMS-formatted beast. Thus we send it with
	** SCC_TRACE rather than with SCC_ERROR...
	**
	** scf_cb.scf_nbr_union.scf_local_error   = E_GW0000_OK;
	** STRUCT_ASSIGN_MACRO(sqlstate, scf_cb.scf_aux_union.scf_sqlstate);
	** status = scf_call( SCC_ERROR, &scf_cb );
	*/

	status = scf_call( SCC_TRACE, &scf_cb );

	if (status != E_DB_OK)
	{
	    /*
	    ** YUCK! We were unable to send the error to the user. Document
	    ** why, but don't try to do any more error-handling.
	    */

	    _VOID_ ule_format( scf_cb.scf_error.err_code, (CL_ERR_DESC *)0,
				ULE_LOG, (DB_SQLSTATE *)NULL, (char *)0,
				(i4)0, (i4 *)0, &local_error, 0);
	}

    }
    else
    {
	/*
	** Since sys$getmsg failed, we have nothing to send to the user. At
	** least document why sys$getmsg failed so someone can come fix us.
	*/
	_VOID_ ule_format( status, (CL_ERR_DESC *)0, ULE_LOG,
			    (DB_SQLSTATE *)NULL, (char *)0, (i4)0,
			    (i4 *)0, &local_error, 0);
    }

    return;
}

/*
** Name: gw02_send_error	- send Ingres error message to front end
**
** Description:
**	This routine is somewhat like gw02_errsend, but this routine sends a
**	INGRES error message to the frontend. We format the message using
**	ule_format, then send it to the frontend using scc_error().
**
** Inputs:
**	err_code		- the error message number
**
** Outputs:
**	None
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	12-jun-1990 (bryanp)
**	    Created.
**	5-jul-90 (linda)
**	    Added parameters (not just err_code), so that errors in default
**	    mappings or in extended format parsing can be more specific.
**	    Removed the static declaration so that this can be called from
**	    another module.
**	29-oct-92 (andre)
**	    ERlookup() has been renamed to ERslookup() and its interface has
**	    been modified to return sqlstate status code instead of generic
**	    error +
**	    in SCF_CB scf_generic_error has been replaced with scf_sqlstate 
*/
DB_STATUS
gw02_send_error
(
    i4 err_code,
    i4	    nparms,
    i4 siz1,
    PTR	    arg1,
    i4 siz2,
    PTR	    arg2,
    i4 siz3,
    PTR	    arg3,
    i4 siz4,
    PTR	    arg4,
    i4 siz5,
    PTR	    arg5,
    i4 siz6,
    PTR	    arg6
)
{
    i4		sid;
    SCF_CB	scf_cb;
    STATUS	tmp_status;
    i4		msglen;
    CL_ERR_DESC	sys_err;
    char	errmsg[ER_MAX_LEN];
    DB_STATUS	status;
    ER_ARGUMENT	er_args[6];

    /* Load up argument list for ERslookup(). */
    er_args[0].er_size  = siz1;
    er_args[0].er_value = arg1;
    er_args[1].er_size  = siz2;
    er_args[1].er_value = arg2;
    er_args[2].er_size  = siz3;
    er_args[2].er_value = arg3;
    er_args[3].er_size  = siz4;
    er_args[3].er_value = arg4;
    er_args[4].er_size  = siz5;
    er_args[4].er_value = arg5;
    er_args[5].er_size  = siz6;
    er_args[5].er_value = arg6;

    tmp_status  = ERslookup(err_code, 0, ER_TIMESTAMP,
				scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate,
				errmsg, sizeof(errmsg), -1,
				&msglen, &sys_err, nparms,
				(ER_ARGUMENT *)&er_args[0]);

    if (tmp_status != OK)
    {
	return (E_DB_ERROR);
    }

    CSget_sid( &sid );

    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = sid;	    /* was: DB_NOSESSION */
    scf_cb.scf_nbr_union.scf_local_error = err_code;
    scf_cb.scf_len_union.scf_blength = msglen;
    scf_cb.scf_ptr_union.scf_buffer = errmsg;
    status = scf_call(SCC_ERROR, &scf_cb);
    if (status != E_DB_OK)
    {
	gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	gwf_error(E_GW0303_SCC_ERROR_ERROR, GWF_INTERR, 0);

	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}

/*
** Name: rms_fab_suspend	- Wait for RMS completion of FAB Service Call
**
** Description:
**	This routine waits for the RMS completion AST and then calls CSresume
**	to wake the thread up again.
**
**	Note that CS_GWFIO_MASK is passed to CSsuspend to indicate that this
**	suspension is for a Gateway facility wait.
**
** Inputs:
**	s			- RMS return code from RMS Service Call
**      fab			- Pointer to fab.
**
** Outputs:
**	None
**
** Returns:
**	status			- RMS Service Call status 
**
**
** History:
**	20-apr-90 (bryanp)
**	    Adapted from DI for use by the RMS Gateway
*/
static STATUS
rms_fab_suspend
(
    STATUS	s,
    struct FAB	*fab
)
{
    if (s == RMS$_BLN || s == RMS$_BUSY || s == RMS$_FAB || s == RMS$_RAB ||
	s == RMS$_STR)
    {
	/*
	** These status codes are so fatal that the AST's will never be called,
	** so we'd better not suspend...
	*/
	return (s);
    }

    _VOID_ CSsuspend(CS_GWFIO_MASK, 0, 0);

    return (fab->fab$l_sts);
}

/*
** Name: rms_fab_resume     - Signal RMS completion of fab-related wait.
**
** Description:
**	This routine is called at AST level when a FAB-related event completes.
**	It CSresume's the appropriate session.
**
** Inputs:
**	fab			- the FAB which just completed work.
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** Side Effects:
**	The waiting session is resumed.
**
** History:
**	23-apr-90 (bryanp)
**	    Created, modelled after DI.
*/
static VOID
rms_fab_resume
(
    struct FAB *fab
)
{
    CSresume(fab->fab$l_ctx);
}

/*
** Name: rms_rab_suspend	- Wait for RMS completion of RAB Service Call
**
** Description:
**	This routine waits for the RMS completion AST and then calls CSresume
**	to wake the thread up again.
**
**	Note that CS_GWFIO_MASK is passed to CSsuspend to indicate that this
**	suspension is for a Gateway facility wait.
**
** Inputs:
**	s			- RMS return code from RMS Service Call
**      rab                             Pointer to rab.
**
** Outputs:
**	None
**
** Returns:
**	status			- RMS Service Call status 
**
**
** History:
**	20-apr-90 (bryanp)
**	    Adapted from DI for use by the RMS Gateway
*/
static STATUS
rms_rab_suspend
(
    STATUS	s,
    struct RAB	*rab
)
{
    if (s == RMS$_BLN || s == RMS$_BUSY || s == RMS$_FAB || s == RMS$_RAB ||
	s == RMS$_STR)
    {
	/*
	** These status codes are so fatal that the AST's will never be called,
	** so we'd better not suspend...
	*/
	return (s);
    }

    _VOID_ CSsuspend(CS_GWFIO_MASK, 0, 0);

    return (rab->rab$l_sts);
}

/*
** Name: rms_rab_resume     - Signal RMS completion of rab-related wait.
**
** Description:
**	This routine is called at AST level when a RAB-related event completes.
**	It CSresume's the appropriate session.
**
** Inputs:
**	rab			- the RAB which just completed work.
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** Side Effects:
**	The waiting session is resumed.
**
** History:
**	23-apr-90 (bryanp)
**	    Created, modelled after DI.
*/
static VOID
rms_rab_resume
(
    struct RAB	*rab
)
{
    CSresume(rab->rab$l_ctx);
}

/*
** Name: gw02_xlate_key	- Translate ingres key values into RMS key values.
**
** Description:
**	This routine is called by gw02_position to translate supplied ingres
**	keys into RMS key values.
**
** Inputs:
**	gwx_rcb			- the exit control block which contains the
**				    ingres key values:
**
**				    gwx_rcb->var_data1	--  Ingres low key
**				    gwx_rcb->var_data2	--  Ingres high key
**
** Outputs:
**	gwx_rcb->xrcb_rsb->internal_rsb->low_key    --	RMS low key
**	gwx_rcb->xrcb_rsb->internal_rsb->high_key   --	RMS high key
**
** Returns:
**	E_DB_OK if key value(s) successfully translated, or
**	E_DB_ERROR if error translating key(s).
**
** Side Effects:
**	The gwx_rcb->xrcb_rsb->internal_rsb key fields (low_key, low_key_len,
**	high_key, high_key_len) are updated with RMS key values to use for
**	positioning and setting scan limits.
**
** History:
**	1-aug-90 (linda)
**	    Written.
**	9-jan-91 (linda)
**	    Remove memory allocation for low and high keys; this is now done
**	    only once, at table open in gw02_open().
**      26-oct-93 (schang)
**          if we hit minmax value, just use it to position; there is no
**          reason to make minmax value special.
*/
static
DB_STATUS
gw02_xlate_key
(
    GWX_RCB	*gwx_rcb
)
{
    DB_STATUS	    status = E_DB_OK;
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_TCB	    *tcb = rsb->gwrsb_tcb;
    DMT_TBL_ENTRY   *tbl = (DMT_TBL_ENTRY *)&tcb->gwt_table;
    GWRMS_RSB	    *rms_rsb = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    ADF_CB	    *adfcb = rsb->gwrsb_session->gws_adf_cb;
    char	    rms_low_key[RMS_MAXKEY];
    char	    rms_high_key[RMS_MAXKEY];
    char	    *inglkey;
    char	    *inghkey;
    DB_ATTS	    **key_atts;
    DB_ATTS	    *key_att;
    i2		    *ingkeyatt;
    i4		    ingkeynum;
    i4		    i;
    i4		    j;
    i4		    rms_lkoffset;
    i4		    rms_hkoffset;
    i4		    ing_lkoffset;
    i4		    ing_hkoffset;
    i4		    msglen;
    i4		    ulecode;
    DB_DATA_VALUE   ing_value;
    DB_DATA_VALUE   rms_value;
    GWRMS_XATT	    *rms_attr;
    GW_ATT_ENTRY    *gw_attr;
    DMT_ATT_ENTRY   *ing_attr;
    DMT_IDX_ENTRY   *idx;
    i4		    idx_att;
    bool	    low_key = FALSE;
    bool	    high_key = FALSE;
    bool            is_asc;

    for (;;)	/* Just to break out of on error... */
    {
	/***
	**    NOTE:  For descending keys, need to swap low & high key values.
	**    Don't know if we have saved the asc/desc info; must do that
	**    somewhere! schang : since singres does not support descending
        **    key order, there is no recording of descending or ascending
        **    Since RMS provide descending, we must be able to retrieve
        **    in ascending manner.  Ingres is to sort or not is a performance
        **    issue not an issue of feature.
	***/

	ingkeynum = gwx_rcb->xrcb_var_data_list.ptr_in_count;
        if (rms_key_asc(rms_rsb->fab,(i4)rms_rsb->rab.rab$b_krf))
        {
	    inglkey = gwx_rcb->xrcb_var_data1.data_address;	/* low key */
	    inghkey = gwx_rcb->xrcb_var_data2.data_address;	/* high key */
        }
        else
        {
	    inglkey = gwx_rcb->xrcb_var_data2.data_address;	/* low key */
	    inghkey = gwx_rcb->xrcb_var_data1.data_address;	/* high key */
        }
	if (inglkey)
	    low_key = TRUE;
	if (inghkey)
	    high_key = TRUE;

	/* First process the low key. */

	key_atts = (DB_ATTS **)gwx_rcb->xrcb_var_data_list.ptr_address;
	ingkeyatt = (i2 *)gwx_rcb->xrcb_var_data_list.ptr_address;
	rms_lkoffset = 0;
	ing_lkoffset = 0;

	if (inglkey)
	{
	    for (i = 0; i < gwx_rcb->xrcb_var_data1.data_in_size; i++)
	    /*                       ^^^^^^^^^info here is for low key.	*/
	    {
		gw_attr = tcb->gwt_attr;    /* reset each time through. */

		/*
		** First, identify the next key attribute.  Next, traverse the
		** attribute list for the table to get the gateway
		** characteristics for this key column.
		*/
		key_att = *key_atts++;  /* keep current for error processing */

		if (gwx_rcb->xrcb_tab_id->db_tab_index != 0) /* 2-ary index */
		{
                    /*
                    **  schang (jul-16-96) the data_in_size is copied from 
                    **    somewhere. The value occasionally are wrong.  The
                    **    value in tcb should be correct.
                    */
                    if (i >= tcb->gwt_idx->gwix_idx.idx_key_count)
                        break;

		    idx = &tcb->gwt_idx->gwix_idx;
		    idx_att = idx->idx_attr_id[i];
		    for (j = 0; j < (idx_att-1); j++)
			gw_attr++;
		}
		else    /* it's a base table */
		{
		    for (j = 0; j < (*ingkeyatt - 1); j++)
			gw_attr++;
		}

		ing_attr = &gw_attr->gwat_attr;
		rms_attr = (GWRMS_XATT *)gw_attr->gwat_xattr.data_address;
		ingkeyatt++;

		ing_value.db_datatype = ing_attr->att_type;
		ing_value.db_prec = ing_attr->att_prec;
		ing_value.db_length = ing_attr->att_width;
		ing_value.db_data = &inglkey[ing_lkoffset];

		rms_value.db_datatype = rms_attr->gtype;
		rms_value.db_prec = rms_attr->gprec_scale;
		rms_value.db_length = rms_attr->glength;
		rms_value.db_data = &rms_low_key[rms_lkoffset];

		/*
		** NOTE:  Need to get the information on whether or not this is
		** a minimum value -- i.e. position at BOF.  NOTE:  For now,
		** key is thrown out if any part of it is a minimum or null.
		** Probably this won't hurt much in practice...  NOTE:  Also
		** reject a low key set to a maximum value -- don't expect this
		** to happen anyway.
		*/
		if ((status = adc_isminmax(adfcb, &ing_value)) != E_DB_OK)
		{
		    if (status == E_DB_WARN)
		    {
			if ( adfcb->adf_errcb.ad_errcode == E_AD0119_IS_NULL )
			{
			    low_key = FALSE;
			    status = E_DB_OK;
			    break;
			}
                        /*
                        **  position by key regardless it is min or max
                        */
                        else if (adfcb->adf_errcb.ad_errcode == E_AD0117_IS_MINIMUM)
                        {
 		            low_key = FALSE;
                            status = E_DB_OK;
                        }
   			else if (adfcb->adf_errcb.ad_errcode == E_AD0118_IS_MAXIMUM)
                        {
                            status = E_DB_OK;
                        }
			else
			    status = E_DB_ERROR;
		    }
		}

		status = adc_cvinto(adfcb, &ing_value, &rms_value);
		if (status != E_DB_OK)
		{
		    /*
		    ** Error converting a key field.  Log error.
		    */
		    _VOID_ ule_format(E_GW5011_CVT_KEY_TO_RMS_ERROR,
			    (CL_ERR_DESC *)0, ULE_LOG, (DB_SQLSTATE *)NULL,
			    (char *)0,
			    (i4)0, &msglen, &ulecode, 9, sizeof(i), &i,
			    sizeof(key_att->type), &key_att->type,
			    sizeof(key_att->precision), &key_att->precision,
			    sizeof(key_att->length), &key_att->length,
			    sizeof(key_att->key_offset), &key_att->key_offset,
			    sizeof(rms_attr->gtype), &rms_attr->gtype,
			    sizeof(rms_attr->gprec_scale),
			    &rms_attr->gprec_scale,
			    sizeof(rms_attr->glength),
			    &rms_attr->glength,
			    sizeof(rms_attr->goffset),
			    &rms_attr->goffset);
		    gwx_rcb->xrcb_error.err_code = E_GW0106_GWX_VPOSITION_ERROR;
		    status = E_DB_ERROR;
		    break;
		}

		rms_lkoffset += rms_attr->glength;
		ing_lkoffset += ing_attr->att_width;

		if (status != E_DB_OK)
		    break;
	    }

	    if (status != E_DB_OK)
		break;
	}

	if (status != E_DB_OK)
	    break;

	if (inglkey && (low_key == TRUE))
	{
	    MEcopy(rms_low_key, rms_lkoffset, rms_rsb->low_key);
	    rms_rsb->low_key_len = rms_lkoffset;
	}
	else
	{
	    rms_rsb->low_key_len = 0;
	}

	/* Now process the high key. */

	key_atts = (DB_ATTS **)gwx_rcb->xrcb_var_data_list.ptr_address;
	ingkeyatt = (i2 *)gwx_rcb->xrcb_var_data_list.ptr_address;
	rms_hkoffset = 0;
	ing_hkoffset = 0;

	if (inghkey)
	{
	    for (i = 0; i < gwx_rcb->xrcb_var_data2.data_in_size; i++)
	    /*                       ^^^^^^^^^info here is for high key. */
	    {
		gw_attr = tcb->gwt_attr;    /* reset each time through. */

		/*
		** First, identify the next key attribute.  Next, traverse the
		** attribute list for the table to get the gateway
		** characteristics for this key column.
		*/
		key_att = *key_atts++;

		if (gwx_rcb->xrcb_tab_id->db_tab_index != 0) /* 2-ary index */
		{
                    /*
                    **  schang (jul-16-96) the data_in_size is copied from 
                    **    somewhere. The value occasionally are wrong.  The
                    **    value in tcb should be correct.
                    */
                    if (i >= tcb->gwt_idx->gwix_idx.idx_key_count)
                        break;

		    idx = &tcb->gwt_idx->gwix_idx;
		    idx_att = idx->idx_attr_id[i];
		    for (j = 0; j < (idx_att-1); j++)
			gw_attr++;
		}
		else    /* it's a base table */
		{
		    for (j = 0; j < (*ingkeyatt - 1); j++)
			gw_attr++;
		}

		ing_attr = &gw_attr->gwat_attr;
		rms_attr = (GWRMS_XATT *)gw_attr->gwat_xattr.data_address;
		ingkeyatt++;

		ing_value.db_datatype = ing_attr->att_type;
		ing_value.db_prec = ing_attr->att_prec;
		ing_value.db_length = ing_attr->att_width;
		ing_value.db_data = &inghkey[ing_hkoffset];

		rms_value.db_datatype = rms_attr->gtype;
		rms_value.db_prec = rms_attr->gprec_scale;
		rms_value.db_length = rms_attr->glength;
		rms_value.db_data = &rms_high_key[rms_hkoffset];

		/*
		** NOTE:  Need to get the information on whether or not this is
		** a maximum value -- i.e. scan to EOF.  NOTE:  For now, key is
		** thrown out if any part of it is a maximum or null.  Probably
		** this won't hurt much in practice...  NOTE:  Also reject a
		** high key set to a minimum value -- don't expect this to
		** happen anyway.

		*/
		if ((status = adc_isminmax(adfcb, &ing_value)) != E_DB_OK)
		{
		    if (status == E_DB_WARN)
		    {
			if (
			    (adfcb->adf_errcb.ad_errcode == E_AD0119_IS_NULL)
			   )
			{
			    high_key = FALSE;
			    status = E_DB_OK;
			    break;
			}
                        /*
                        **  position by key regardless it is min or max
                        */
                        else if (
			    (adfcb->adf_errcb.ad_errcode == E_AD0117_IS_MINIMUM)
			    ||
			    (adfcb->adf_errcb.ad_errcode == E_AD0118_IS_MAXIMUM)
                                )
                        {
                            status = E_DB_OK;
                        }
			else
			    status = E_DB_ERROR;
		    }
		}

		status = adc_cvinto(adfcb, &ing_value, &rms_value);
		if (status != E_DB_OK)
		{
		    /*
		    ** Error converting a key field.  Log error.
		    */
		    _VOID_ ule_format(E_GW5011_CVT_KEY_TO_RMS_ERROR,
			    (CL_ERR_DESC *)0, ULE_LOG, (DB_SQLSTATE *) NULL,
			    (char *)0,
			    (i4)0, &msglen, &ulecode, 9, sizeof(i), &i,
			    sizeof(key_att->type), &key_att->type,
			    sizeof(key_att->precision), &key_att->precision,
			    sizeof(key_att->length), &key_att->length,
			    sizeof(key_att->key_offset), &key_att->key_offset,
			    sizeof(rms_attr->gtype), &rms_attr->gtype,
			    sizeof(rms_attr->gprec_scale),
			    &rms_attr->gprec_scale,
			    sizeof(rms_attr->glength),
			    &rms_attr->glength,
			    sizeof(rms_attr->goffset),
			    &rms_attr->goffset);
/***NOTE:  WANT TO CHANGE THIS ERROR CODE TO NEW ONE***/
		    gwx_rcb->xrcb_error.err_code = E_GW0106_GWX_VPOSITION_ERROR;
		    status = E_DB_ERROR;
		    break;
		}

		rms_hkoffset += rms_attr->glength;
		ing_hkoffset += ing_attr->att_width;

		if (status != E_DB_OK)
		    break;
	    }

	    if (status != E_DB_OK)
		break;
	}

	if (status != E_DB_OK)
	    break;

	if (inghkey && (high_key == TRUE))
	{
	    MEcopy(rms_high_key, rms_hkoffset, rms_rsb->high_key);
	    rms_rsb->high_key_len = rms_hkoffset;
	}
	else
	{
	    rms_rsb->high_key_len = 0;
	}

	break;	/* always break here */
    }

    if (status == E_DB_WARN)
	status = E_DB_OK;

    return(status);
}


/*
** Name: gw02_max_rms_ksz   -	Calculate maximum rms key size
**
** Description:
**	This routine is called by gw02_open to calculate maximum rms key size
**	so that buffers can be allocated for low and high keys.
**
** Inputs:
**	gwx_rcb			-   the exit control block which contains the
**				    attribute and extended attribute information
**
** Outputs:
**
** Returns:
**	ksz			-   key size to be allocated
**
** Side Effects:
**	none
**
** History:
**	9-jan-91 (linda)
**	    Written.
**	26-apr-1991 (rickh)
**		Added logic to calculate key size for secondary indices.
*/
static
i4
gw02_max_rms_ksz
(
    GWX_RCB	*gwx_rcb
)
{
    GW_RSB	    	*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GW_TCB	    	*tcb = rsb->gwrsb_tcb;
    GW_ATT_ENTRY    *gw_attr = tcb->gwt_attr;
    GWRMS_XATT	    *rms_attr;
    i4		    	ncol = gwx_rcb->xrcb_column_cnt;
    i4		    	i;
    i4		    	ksz = 0;
    DMT_IDX_ENTRY   *indexDescriptor;
	i4			numberOfKeysInIndex;
	i4			columnNumber;
    GW_ATT_ENTRY    *GWattributeDescriptor;

	if ( tcb->gwt_table.tbl_id.db_tab_index == 0 )
	{
		/*	base table	*/

	    for (i = 0; i < ncol; i++)
	    {
		if (gw_attr->gwat_attr.att_key_seq_number > 0)
		{
		    rms_attr = (GWRMS_XATT *)gw_attr->gwat_xattr.data_address;
		    ksz += rms_attr->glength;
		}
		gw_attr++;
	    }
	}
	else	/* secondary index	*/
	{
		indexDescriptor = &tcb->gwt_idx->gwix_idx;
		numberOfKeysInIndex = indexDescriptor->idx_key_count;

		for ( i = 0; i < numberOfKeysInIndex; i++ )
		{
			columnNumber = indexDescriptor->idx_attr_id[ i ] - 1;
                        /*
                        ** 05-feb-04 (chash01)  If columnNumber is -1,
                        ** then indexDescriptor->idx_attr_id[ i ] must be 0
                        ** this means we have come to the end of key attributes
                        ** and we are currently working on a field that is 
                        ** allocated for TIDP.  Since TIDP is not accounted
                        ** for RMS keys, we must break off in order to have
                        ** correct key size .
                        */
                        if (columnNumber < 0)  /* eliminate TIDP for 2.0 */
                            break;
			GWattributeDescriptor = &gw_attr[ columnNumber ];
		    rms_attr =
				(GWRMS_XATT *)GWattributeDescriptor->gwat_xattr.data_address;
		    ksz += rms_attr->glength;
		}
	}

    return (ksz);
}

/*
** Name: applyTimeout -	Set timeout period on RMS record operations
**
** Description:
**	This routine is called just before RMS record operations which
**	allow timeouts.  By setting fields in the RAB, this routine
**	instructs RMS to wait a specified number of seconds for a
**	locked record to free up.
**
**		timeout < 0	=> Treat like timeout=0.
**
**		timeout = 0	=> No timeout.  Wait forever.  This is
**				   the INGRES convention, not the usual RMS
**				   default.
**
**		timeout > 0	=> Wait timeout number of seconds.  If
**				   timeout > maximum RMS timeout period,
**				   only wait for that maximum period.
**
** Inputs:
**	gw_rsb			-  GW record stream control structure.
**				   This contains the timeout value.
**
** Outputs:
**	rms_rsb			-  RMS record stream control structure.
**				   This points at the RAB, whose timeout
**				   related bits and bytes we pack.
**
** Returns:
**	VOID
**
** Side Effects:
**	none
**
** History:
**	25-jan-91 (RickH)
**	    Written.
*/

#define	WAIT_FOREVER		0
#define	MAX_RMS_TIMEOUT		255

static
VOID
applyTimeout(
    GW_RSB	*gw_rsb,
    GWRMS_RSB	*rms_rsb
)
{
    unsigned	int	waitMask;	/* same data type as rab$l_rop */


    /* turn off the stale wait bits */

    waitMask = ( RAB$M_TMO | RAB$M_WAT );
    waitMask = ~waitMask;
    rms_rsb->rab.rab$l_rop &= waitMask;

    /*
    ** wait forever if 0 timeout specified
    */
    if ( gw_rsb->gwrsb_timeout <= 0 )
    {
	rms_rsb->rab.rab$l_rop |= RAB$M_WAT;
    }
    /*
    ** wait 255 sec if timeout >255
    */
    else if ( gw_rsb->gwrsb_timeout > MAX_RMS_TIMEOUT )
    {
	rms_rsb->rab.rab$l_rop |= ( RAB$M_TMO | RAB$M_WAT );
        rms_rsb->rab.rab$b_tmo =  MAX_RMS_TIMEOUT;
    }
    /*
    ** normal timeout range
    */
    else 
    {
        rms_rsb->rab.rab$l_rop |= ( RAB$M_TMO | RAB$M_WAT );
        rms_rsb->rab.rab$b_tmo = gw_rsb->gwrsb_timeout;
    }

}

/*
** Name: rePositionRAB - Reset the record stream's position
**
** Description:
**	sys$update and sys$delete deposition the record stream.  This
**	logic restores the clobbered position.  This is needed for cursor
**	processing.
**
** Inputs:
**	gwx_rcb			-  GW record control block.
**
** Output:
**	gwx_rcb->
**	    error.err_code	Detailed error code, if error occurred:
**				E_GW0106_GWX_VPOSITION_ERROR
**				E_GW0641_END_OF_STREAM
**				E_GW5401_RMS_FILE_ACT_ERROR
**				E_GW540D_RMS_DEADLOCK
**				E_GW5404_RMS_OUT_OF_MEMORY
**				E_GW540C_RMS_ENQUEUE_LIMIT
**				E_GW540E_RMS_ALREADY_LOCKED
**				E_GW540F_RMS_PREVIOUSLY_DELETED
**				E_GW5410_RMS_READ_LOCKED_RECORD
**				E_GW5411_RMS_RECORD_NOT_FOUND
**				E_GW5412_RMS_READ_AFTER_WAIT
**				E_GW5413_RMS_RECORD_LOCKED
**				E_GW5414_RMS_TIMED_OUT
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
**
** Side Effects:
**	none
**
** History:
**	6-aug-91 (rickh)
**	    Creation.
*/

static	DB_STATUS
rePositionRAB
(
    GWX_RCB	*gwx_rcb
)
{
    GW_RSB	    	*rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GWRMS_RSB	    	*rms_rsb = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    unsigned char   	saved_rac;	/* record access mode */
    DB_STATUS		status = E_DB_OK;

    saved_rac = rms_rsb->rab.rab$b_rac;

    rms_rsb->rab.rab$b_rac = RAB$C_RFA;
    status = positionRecordStream( gwx_rcb );

    rms_rsb->rab.rab$b_rac = saved_rac;

    return( status );
}

/*
** Name: positionRecordStream - Find a record location
**
** Description:
**	This is the final agony of calling sys$find.  It was extracted
**	to a separate routine so that it could be called by both
**	gw02_position and gw02_update.
**
**
** Inputs:
**	gwx_rcb			-  GW record control block.
**
** Output:
**	gwx_rcb->
**	    error.err_code	Detailed error code, if error occurred:
**				E_GW0106_GWX_VPOSITION_ERROR
**				E_GW0641_END_OF_STREAM
**				E_GW5401_RMS_FILE_ACT_ERROR
**				E_GW540D_RMS_DEADLOCK
**				E_GW5404_RMS_OUT_OF_MEMORY
**				E_GW540C_RMS_ENQUEUE_LIMIT
**				E_GW540E_RMS_ALREADY_LOCKED
**				E_GW540F_RMS_PREVIOUSLY_DELETED
**				E_GW5410_RMS_READ_LOCKED_RECORD
**				E_GW5411_RMS_RECORD_NOT_FOUND
**				E_GW5412_RMS_READ_AFTER_WAIT
**				E_GW5413_RMS_RECORD_LOCKED
**				E_GW5414_RMS_TIMED_OUT
**
**      Returns:
**          E_DB_OK             Function completed normally. 
**          E_DB_ERROR          Function completed abnormally with 
**                              error.err_code.
**
** Side Effects:
**	none
**
** History:
**	6-aug-91 (rickh)
**	    Extracted from gw02_position.  Set "rab positioned" bit in rsb.
**      15-mar-94 (schang)
**          take care of conditions for RRL option
*/

static	DB_STATUS
positionRecordStream
(
    GWX_RCB	*gwx_rcb
)
{
    GW_RSB	    *rsb = (GW_RSB *)gwx_rcb->xrcb_rsb;
    GWRMS_RSB	    *rms_rsb = (GWRMS_RSB *)rsb->gwrsb_internal_rsb;
    i4  	    vms_status;
    DB_STATUS	    status = E_DB_OK;
    i4              sid;

    /* set RMS timeout value	25-jan-91 */

    /* asynchronous RMS: */
    CSget_sid(&sid);
    rms_rsb->rab.rab$l_ctx = sid;
    rms_rsb->rab.rab$l_rop |= RAB$M_ASY;
    if (gwx_rcb->xrcb_xbitset & II_RMS_RRL)
	rms_rsb->rab.rab$l_rop |= RAB$M_RRL;  
    applyTimeout( rsb, rms_rsb );
    /*
    **  if read-regardless-lock specified but no timeout flag set 
    **  from applyTimeout, we mask out the RAB$M_WAT so that it can 
    **  be true read-regard-less
    */
    if ((gwx_rcb->xrcb_xbitset & II_RMS_RRL)  &&
        (gwx_rcb->xrcb_xbitset & II_RMS_READONLY))
        if (!(rms_rsb->rab.rab$l_rop & RAB$M_TMO))
    	    rms_rsb->rab.rab$l_rop &= ~RAB$M_WAT;
    vms_status = rms_rab_suspend(
		    sys$find(&rms_rsb->rab, rms_rab_resume, rms_rab_resume),
		    &rms_rsb->rab);
            
    /*
    ** If error, log a detailed error message including the vms error and
    ** any relevant parameters. The sequential find and the key find are
    ** different error messages -- they have vastly different parameters.
    */
    switch (vms_status)
    {
	case RMS$_NORMAL:
	case RMS$_OK_WAT:
            status = E_DB_OK;
            break;
	    /* MAY WANT TO SPECIAL CASE THIS --> EOF, IF INGRES CAN HANDLE IT */
	case RMS$_OK_LIM:
	    status = E_DB_OK;
	    break;
	case RMS$_EOF:
	    gwx_rcb->xrcb_error.err_code = E_GW0641_END_OF_STREAM;
	    status = E_DB_ERROR;
	    break;
	case RMS$_RNF:
	    gwx_rcb->xrcb_error.err_code = E_GW5411_RMS_RECORD_NOT_FOUND;
	    status = E_DB_ERROR;
	    break;

	default:
            if (vms_status==RMS$_OK_RLK || vms_status == RMS$_OK_RRL)
                if (gwx_rcb->xrcb_xbitset & II_RMS_RRL)
                {
                    status = E_DB_OK;
                    break;
                }
	    if (rms_rsb->rab.rab$b_rac == RAB$C_KEY)
	    {
		gw02_log_error(E_GW5005_RMS_FIND_KEY_ERROR,
				   (struct FAB *)&rms_rsb->fab->rmsfab,
				   (struct RAB *)&rms_rsb->rab,
				   (ULM_RCB *)NULL, (SCF_CB *)NULL);
	    }
	    else
	    {
		gw02_log_error(E_GW5004_RMS_FIND_SEQ_ERROR,
				   (struct FAB *)&rms_rsb->fab->rmsfab,
				   (struct RAB *)&rms_rsb->rab,
				   (ULM_RCB *)NULL, (SCF_CB *)NULL);
	    }
	    gwx_rcb->xrcb_error.err_code = 
			    gw02_ingerr(RMS_SYS_FIND, vms_status,
					E_GW0106_GWX_VPOSITION_ERROR);
	    status = E_DB_ERROR;
	    break;
    }

    /* sys$get and sys$find position the RAB on a record */

    if ( status == E_DB_OK )
	rms_rsb->rsb_state |= GWRMS_RAB_POSITIONED;
    else
	rms_rsb->rsb_state &= ~( GWRMS_RAB_POSITIONED );

    return( status );

}
