/*
**    Copyright (c) 2008 Ingres Corporation
**
*/

/*
** Name: astmgmt.c - AST management within a pthread environment.
**
**
** History:
**  18-Jun-2007 (stegr01)
**  10-jul-2009 (stegr01)
**    add function IIMSIC_destroy_astthread to force ast dispatcher thread
**    to exit prior to running down the main thread
*/
#include <compat.h>

#ifdef OS_THREADS_USED 

#include <cs.h>
#include <me.h>
#include <st.h>
#include <tr.h>


/* Additional VMS header files. */

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <perror.h>
#include <unistd.h>
#include <assert.h>
#include <types.h>
#include <stdbool.h>


#include <ssdef.h>
#include <stsdef.h>
#include <libdef.h>
#include <builtins.h>
#include <descrip.h>
#include <psldef.h>
#include <vadef.h>
#include <jpidef.h>
#include <iledef.h>
#include <iosbdef.h>
#include <efndef.h>
#include <iodef.h>
#include <far_pointers.h>
#include <gen64def.h>             /* generic 64 types          */
#include <ints.h>                 /* 64 bit ints               */
#include <libicb.h>
#include <chfdef.h>

#include <lib$routines.h>         /* RTL prototypes            */
#include <starlet.h>              /* system service prototypes */
#include <pthread.h>


#include <astmgmt.h>
#include <queuemgmt.h>
#include <pthreadmgmt.h>


extern i4 ii_sys$cantim(uint64 reqidt, u_i4 acmode);


/*
** The grand scheme of things ...
** ------------------------------
**
** ASTs do not coexist happily with Posix threads. Short of rewriting all
** the facilities in the DBMS it was decided to live with ASTs but handle 
** them within  the issuing thread, as transparently as possible. In order
** to do this all modules that use the system services (or RMS) to do
** asynchronous work with AST completions must now include the header file
** astjacket.h. This files refedines every asynchronous system service to be 
** an equivalent Ingres jacket routine. The jacket routine will replace the
** callers AST function with another AST jacket routine, and the AST parameter 
** with the address of an internal AST context block (that, among other things,
** contains the caller's original AST function address and AST parameter).
** This new jacket function's sole purpose is to queue the AST context block
** (whose address is the AST parameter) to a queue header and wake an AST
** dispatcher thread which will then remove the context block from this queue
** and either requeue the context block to the issuing thread's ast queue or
** execute the caller's original AST function within the context of the AST
** dispatcher thread. The decision about whether an AST function should be
** executed on the AST dispatcher thread or forwarded on to the thread that 
** was responsible for requesting the AST is determined by whether there is an
** SCB address within the AST context block. It is assumed that threads that 
** issue an AST request and have an SCB will eventually call CS_suspend and wait
** to be woken. (This is only true of true multithreaded servers, i.e. DBMS 
** based servers). So in this event the AST context block is requeued to the
** AST queue header in the SCB->evcb structure and the condition variable 
** in that evcb is signaled. At this point the issuing thread ought to be suspended
** waiting on the condition and will be woken. The routine in CSMTINTERF that would
** normally check if an event has occurred (IICLpoll) has been replaced by one
** called CSMT_execute_asts, which will check the AST queue for this thread and
** execute/drain the AST queue before either returning out of CSsuspend or going back
** to wait on the timed Condition Variable. The check for pending ASTs is done just
** prior to waiting on the Condition Variable and immediately after the Condition is
** either signalled or a timeout occurs.
** 
** The reason for this convolution is that the PTHREAD manager on VMS always delivers
** ASTs in the context of the primary thread not the issuing thread.
** Almost any operation within the AST  will require some synchronisation with the
** issuing thread's structures and this is immediately precluded by the PTHREAD
** implementation on VMS (see the comments below on usage of ASTs in a pthread environment). 
**   
** For a further discussion of this approach please see the DDS for OpenVms Itanium port.
** and the VMS documentation "Guide to the POSIX threads library"
**
*/



/*
** A note for users of ASTs within a pthread environment on Ingres
**
** [1] The AST should do as little as possible
** [2] It should never acquire mutexes
** [3] If it needs to wake the thread it should use pthread_cond_sig_preempt_int_np()
** [4] Avoid allocating memory within the AST - the memory rtns use mutexes
** [5] if you have to allocate memory then use a lookaside list maintained on an interlocked queue
** [6] Report errors on a separate dispatch/error thread using a lookaside list of memory
**     for the error msg
** [7] The minimal AST rtn (for example in a $QIO) should be specified as
**     astadr = pthread_cond_sig_preempt_int_np
**     astprm = &conditionVariable
** [8] Avoid using $setast if possible
** [9] The PTHREAD RTL provides several Non-Portable functions explicitly for AST handling
**     These should be used exclusively either as the AST function or within the AST.
**
*/




#define MUTEX_TYPE PTHREAD_MUTEX_ERRORCHECK

#define AST_THREAD_STACKSIZE   128000



typedef i4 VMS_STS;
typedef struct dsc$descriptor_s DESCRIP_S;

#define SIGNAL_PTHREAD_ERROR(code, sts)  if (sts) lib$signal(SS$_GENTRAP, (sts | (code<<16)), 0, 0);

#define HISTORY_DEPTH 64



/*
**
** Forward declarations
**
*/

/* EXTERNs */

extern void  report_pthread_error();
extern char* getObjectName();
extern void  IICSMTqueue_AST();
extern void  IICSMT_get_ast_ctx2();

/* GLOBAL functions */

/* see astmgmt.h */


/* LOCAL functions */

static void  initialise_once (void);
static void* ast_thread(void* arg);
static void  insertInLookasideList (AST_CTX* ctx);
static i4    nextAst(AST_CTX** pCtx);
static i4    enable_ast_delivery (void);
static i4    disable_ast_delivery (void);
static i4    ast_exc_handler(CHFDEF1* sig_array, CHFDEF2* mech_array);

#ifdef DEBUG_ASTS
static void  getSsrv(i4 idx, char** name);
static void  getSsrvBrief(i4 idx, char** name);
static void  getFuncCode(i4 fcode, char** name);
#endif




/*
**
** Global data
**
*/


#pragma nostandard
globalref i4 decc$gl_optind;
#pragma standard


/* facility initialiser */

static pthread_once_t ast_once_control = PTHREAD_ONCE_INIT;
static pthread_key_t  objectNameKey;
static AST_STATE      ast_state  = {SS$_WASSET, 0};  /* initially enabled */
static i4             multi_threaded = FALSE;

/*
** queue headers for AST context blocks
** these must be quad word aligned
*/


static __align(QUADWORD) SRELQHDR astLalQueue  = {0, 0}; /* self-relative queue */
static __align(QUADWORD) SRELQHDR astQueue     = {0, 0}; /* self-relative queue */
static __align(QUADWORD) ABSQHDR  tqeQueue     = {&tqeQueue, &tqeQueue}; /* absolute queue */

static __align(QUADWORD) ABSQHDR  astHistoryQueue = {&astHistoryQueue, &astHistoryQueue}; /* absolute queue */
static __align(QUADWORD) ABSQHDR  astActiveQueue  = {&astActiveQueue,  &astActiveQueue};  /* absolute queue */


static __align(QUADWORD) __int64  astDisabled = 0;/* set if AST dequeuing inhibited  */

static __align(LONGWORD) i4      astInProgress         = 0; /* ASTIP  bit   */
static __align(LONGWORD) i4      astEnabled            = 1; /* ASTEN  bit (assume enabled from the word go)  */
static __align(LONGWORD) i4      astEnabledPeekLock    = 0; /* lock for when we're checking state of ASTEN */
static __align(LONGWORD) i4      tqeTraversalLock      = 0; /* timer queue traversal lock   */
static __align(LONGWORD) i4      historyLock           = 0; /* history queue traversal lock */
static __align(LONGWORD) i4      activeLock            = 0; /* active  queue traversal lock */

static pthread_mutex_t    astDeliveryMtx    = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t     astDeliveryCv     = PTHREAD_COND_INITIALIZER;
static pthread_t          astThreadId       = 0;
static pthread_mutex_t    astDisableMtx     = PTHREAD_MUTEX_INITIALIZER;
static i4                 astDisableMtxPrio = 0;

static i4                 remquiFailCnt[64] = {0};
static i4                 insquiFailCnt[64] = {0};

static char* astThreadName = "ast_dispatcher";

static bool               exit_now = FALSE;

/*
** counters
*/


static __align(QUADWORD) __int64 remqui_failures   = 0;
static __align(QUADWORD) __int64 insqui_failures   = 0;
static __align(QUADWORD) __int64 spurious_wakeups  = 0;
static __align(QUADWORD) __int64 timeout_wakeups   = 0;
static __align(QUADWORD) __int64 wakeups           = 0;
static __align(QUADWORD) __int64 astipStalls       = 0;
static __align(QUADWORD) __int64 astdsblStalls     = 0;
static __align(QUADWORD) __int64 missingTmrEntries = 0;


/* system services that may use ASTs - debug */

#ifdef DEBUG_ASTS
char* ssrv_names[] = {
"undefined",        /* 0  */
"wake",             /* 1  */
"hiber",            /* 2  */
"schdwk",           /* 3  */
"brkthru",          /* 4  */
"check_privilege",  /* 5  */
"dclast",           /* 6  */
"enq",              /* 7  */
"enqw",             /* 8  */
"getdvi",           /* 9  */
"getlki",           /* 10 */
"getjpi",           /* 11 */
"getqui",           /* 12 */
"getrmi",           /* 13 */
"getsyi",           /* 14 */
"qio",              /* 15 */
"setast",           /* 16 */
"setimr",           /* 17 */
"cantim",           /* 18 */
"sndjbc",           /* 19 */
"acm",              /* 20 */
"audit_event",      /* 21 */
"cpu_transition",   /* 22 */
"dns",              /* 23 */
"files_64",         /* 24 */
"getdti",           /* 25 */
"io_fastpath",      /* 26 */
"io_setup",         /* 27 */
"ipc",              /* 28 */
"lfs",              /* 29 */
"setcluevt",        /* 30 */
"set_system_event", /* 31 */
"setevtast",        /* 32 */
"setpfm",           /* 33 */
"setpra",           /* 34 */
"setdti",           /* 35 */
"setuai",           /* 36 */
"updsec",           /* 37 */
"xfs_client",       /* 38 */
"xfs_server",       /* 39 */
"unused_1",         /* 40 */
"unused_2",         /* 41 */
"unused_3",         /* 42 */
"unused_4",         /* 43 */
"unused_5",         /* 44 */
"unused_6",         /* 45 */
"unused_7",         /* 46 */
"unused_8",         /* 47 */
"unused_9",         /* 48 */
"unused_10",        /* 49 */
"close",            /* 50 */
"connect",          /* 51 */
"create",           /* 52 */
"delete",           /* 53 */
"disconnect",       /* 54 */
"display",          /* 55 */
"enter",            /* 56 */
"erase",            /* 57 */
"extend",           /* 58 */
"find",             /* 59 */
"flush",            /* 60 */
"free",             /* 61 */
"get",              /* 62 */
"nxtvol",           /* 63 */
"open",             /* 64 */
"parse",            /* 65 */
"put",              /* 66 */
"read",             /* 67 */
"release",          /* 68 */
"remove",           /* 69 */
"rename",           /* 70 */
"rewind",           /* 71 */
"search",           /* 72 */
"space",            /* 73 */
"truncate",         /* 74 */
"update",           /* 75 */
"wait",             /* 76 */
"write"};           /* 77 */

char* ssrv_brief[] = {
"undf",        /* 0  */
"wake",        /* 1  */
"hibr",        /* 2  */
"scwk",        /* 3  */
"brkt",        /* 4  */
"cprv",        /* 5  */
"dcla",        /* 6  */
"enq ",        /* 7  */
"enqw",        /* 8  */
"gdvi",        /* 9  */
"glki",        /* 10 */
"gjpi",        /* 11 */
"gqui",        /* 12 */
"grmi",        /* 13 */
"gsyi",        /* 14 */
"qio ",        /* 15 */
"sast",        /* 16 */
"stmr",        /* 17 */
"ctim",        /* 18 */
"sndj",        /* 19 */
"acm ",        /* 20 */
"audt",        /* 21 */
"cput",        /* 22 */
"dns ",        /* 23 */
"f64 ",        /* 24 */
"gdti",        /* 25 */
"io_f",        /* 26 */
"io_s",        /* 27 */
"ipc ",        /* 28 */
"lfs ",        /* 29 */
"sclu",        /* 30 */
"ssev",        /* 31 */
"sev ",        /* 32 */
"spfm",        /* 33 */
"spra"         /* 34 */
"sdti",        /* 35 */
"suai",        /* 36 */
"upds",        /* 37 */
"xfsc",        /* 38 */
"xfss",        /* 39 */
"u1  ",        /* 40 */
"u2  ",        /* 41 */
"u3  ",        /* 42 */
"u4  ",        /* 43 */
"u5  ",        /* 44 */
"u6  ",        /* 45 */
"u7  ",        /* 46 */
"u8  ",        /* 47 */
"u9  ",        /* 48 */
"u10 ",        /* 49 */
"clos",        /* 50 */
"conn",        /* 51 */
"crea",        /* 52 */
"dele",        /* 53 */
"disc",        /* 54 */
"disp",        /* 55 */
"entr",        /* 56 */
"erse",        /* 57 */
"exte",        /* 58 */
"find",        /* 59 */
"flsh",        /* 60 */
"free",        /* 61 */
"get ",        /* 62 */
"nxtv",        /* 63 */
"open",        /* 64 */
"pars",        /* 65 */
"put",         /* 66 */
"read",        /* 67 */
"rels",        /* 68 */
"remv",        /* 69 */
"rena",        /* 70 */
"rewi",        /* 71 */
"srch",        /* 72 */
"spce",        /* 73 */
"trun",        /* 74 */
"upda",        /* 75 */
"wait",        /* 76 */
"wrte"};       /* 77 */

char  io_func_code[32];
char* io_func_names[] = {
io_func_code,       /* 47 IO$_LOGICAL */
"writevblk",        /* 48 */
"readvblk",         /* 49 */
"access",           /* 50 */
"create",           /* 51 */
"deaccess",         /* 52 */
"delete",           /* 53 */
"modify",           /* 54 */
"readprompt",       /* 55 */
"acpcontrol",       /* 56 */
"mount",            /* 57 */
"dismount",         /* 58 */
"ttyreadpall",      /* 59 */
"conintread",       /* 60 */
"conintwrite",      /* 61 */
"readdir"           /* 62 */
};                  /* 63 IO$_VIRTUAL */

#endif



/*
** Typical AST usage ....
**
** extern AST_RTN IIMISC_ast_jacket;
**
** AST_CTX* ctx = IIMISC_getAstCtx();
** ctx->astadr = (AST_RTN)my_ast_func;
** ctx->astprm = (AST_PRM)my_ast_param;
** sts = sys$qio (EFN$C_ENF, chan, IO$_READVBLK, &iosb, IIMISC_ast_jacket, ctx, p1, p2, p3, p4, p5, p6);
**
*/


/*
** Name:  IIMISC_initialise_astmgmt
**
** Description:
**    Reentrant Initialise-once PTHREAD routine to initialise the AST
**    dispatcher thread
**
** Inputs:
**    void* ccb
**          a NULL ccb indicates a single threaded server (e.g. dmfacp)
**          a non-NULL ccb will be a multi threaded server (e.g. dbms)
**
** Outputs:
**    None
**
**  Returns:
**    None.
**
**  Exceptions:
**    None.
**
** Side Effects:
**    Start the AST High priority dispatcher thread exactly once
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/

/*
** Executed once only as part of the AST management facility startup
** This function's address has been placed in the lib$initialize PSECT
** so that users should not need to call it.
**
** It is called before the debugger (if any) gains control and before
** the call to main(). i.e. the compiler generated call to __main() and
** thence to decc$main() and main() will occur AFTER this function is
** executed.
**
** In view of this (the lack of DECC initialisation - like stdio etc.)
** all calls should only be to RTL, SYS or pthread functions.
**
** All errors will be signalled with SS$_GENTRAP, where the gentrap_code
** is defined as ...
**
** low  order word - sts reported from the PTHREAD call (non-zero)
** high order word - an index incremented for each pthread call
**
** sys$imgact() will add the pthread_main() call to the image's transfer
** vector before that of sys$imgsta() if it finds one of the activated
** images is pthread$rtl - so we can safely use pthreads with lib$initialize
**
*/


void
IIMISC_initialise_astmgmt (void* ccb)
{
    /*
    **  save the 'server' state
    **  if a ccb (CS_CB*) was specified then we have a true multithreaded server
    */
    
    multi_threaded = (ccb == NULL) ? FALSE : TRUE; 
    assert (pthread_once (&ast_once_control, initialise_once) == 0);

    ast_state.ast$v_ready = 1;
}



/*
** Name:  initialise_once
**
** Description:
**    PTHREAD initilaise once routine
**    (executed once only by the pthread manager)
**
** Inputs:
**    none
**
** Outputs:
**    None
**
**  Returns:
**    None.
**
**  Exceptions:
**    None.
**
** Side Effects:
**    Start the AST High priority dispatcher thread exactly once
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


static void
initialise_once()
{
    STATUS          status;
    PTHREAD_STS     ptSts;
    VMS_STS         sts;
    i4              astlm = 0;
    i4              tqlm  = 0;
    char            fill = 0;
    ILE3            itmlst[3];
    IOSB            iosb;
    SIZE_TYPE       pagelet_cnt;
    SIZE_TYPE       alloc_pagcnt;
    CL_SYS_ERR      errsys;
    PTR             base_va;
    i4              bytcnt;
    i4              i,j;
    AST_CTX*        ctx;
    AST_HISTORY*    hst;
    pthread_attr_t  attr;
    struct sched_param param;
    i4              curr_policy;
    i4              new_policy = SCHED_RR;
    i4              min_prio;
    i4              max_prio;
    i4              mid_prio;



    ast_state.ast$v_once = 1;

    /*
    ** name and validate the statically initialised Mutexes
    **
    */

    pthread_mutexattr_t mutexAttr;
    assert (pthread_mutexattr_init(&mutexAttr) == 0);
    assert (pthread_mutexattr_settype(&mutexAttr, MUTEX_TYPE) == 0);

    assert (pthread_mutex_init (&astDeliveryMtx, &mutexAttr) == 0);
    assert (pthread_mutex_init (&astDisableMtx,  &mutexAttr) == 0);

    assert (pthread_mutex_setname_np(&astDeliveryMtx, "AST_delivery" , NULL) == 0);
    assert (pthread_mutex_setname_np(&astDisableMtx,  "AST_disable",   NULL) == 0);

    assert (pthread_mutexattr_destroy (&mutexAttr) == 0);

    assert (pthread_mutex_lock   (&astDeliveryMtx) == 0);
    assert (pthread_mutex_unlock (&astDeliveryMtx) == 0);

    assert (pthread_mutex_lock   (&astDisableMtx) == 0);
    assert (pthread_mutex_unlock (&astDisableMtx) == 0);


   /*
    ** name and validate the statically initialised CVs
    **
    */

    assert (pthread_cond_setname_np(&astDeliveryCv,   "AST_delivery", NULL) == 0);

    /*
    ** populate the AST contex block lookaside list
    ** we'll assume that we need as many blocks as the user's AST quota
    **
    */

    itmlst[0].ile3$w_length = sizeof(astlm);
    itmlst[0].ile3$w_code = JPI$_ASTLM;
    itmlst[0].ile3$ps_bufaddr = &astlm;
    itmlst[0].ile3$ps_retlen_addr = NULL;
    itmlst[1].ile3$w_length = sizeof(astlm);
    itmlst[1].ile3$w_code = JPI$_TQLM;
    itmlst[1].ile3$ps_bufaddr = &tqlm;
    itmlst[1].ile3$ps_retlen_addr = NULL;
    itmlst[2].ile3$w_length = 0;
    itmlst[2].ile3$w_code = 0;
    itmlst[2].ile3$ps_bufaddr = NULL;
    itmlst[2].ile3$ps_retlen_addr = NULL;
    sts = sys$getjpiw(EFN$C_ENF, NULL, NULL, itmlst, &iosb, NULL, 0);
    if (sts & STS$M_SUCCESS) sts = iosb.iosb$w_status;
    if (!(sts & STS$M_SUCCESS)) lib$signal (sts);

    pagelet_cnt = (astlm*sizeof(AST_CTX) + tqlm*sizeof(AST_CTX)  + VA$C_PAGELET_SIZE)/VA$C_PAGELET_SIZE;
    status = MEget_pages (ME_LOCKED_MASK, pagelet_cnt, 0, &base_va, &alloc_pagcnt, &errsys);
    if (status) lib$signal (errsys.errnum);

    bytcnt = alloc_pagcnt*VA$C_PAGELET_SIZE;

    j = bytcnt/sizeof(AST_CTX);
    ctx = (AST_CTX *)base_va;

    /* zero the memory */

    fill = 0;
    lib$movc5 ((u_i2 *)&bytcnt, (u_i4 *)base_va, &fill, (u_i2 *)&bytcnt, (u_i4 *)base_va);

    /*
    ** N.B.
    **
    ** These context blocks have not been allocated by malloc()
    ** They must never be free()'ed
    ** IIMISC_freeAstCtx() always inserts the block back onto the lookaside list
    **
    */

    for (i=0; i<j; i++) insertInLookasideList(ctx++);

#ifdef DEBUG_ASTS
    /*
    ** room for the AST history buffer
    */

    pagelet_cnt = (HISTORY_DEPTH*sizeof(AST_HISTORY) + VA$C_PAGELET_SIZE)/VA$C_PAGELET_SIZE;
    status = MEget_pages (ME_LOCKED_MASK, pagelet_cnt, 0, &base_va, &alloc_pagcnt, &errsys);
    if (status) lib$signal (errsys);

    bytcnt = alloc_pagcnt*VA$C_PAGELET_SIZE;

    j = bytcnt/sizeof(AST_HISTORY);
    hst = (AST_HISTORY *)base_va;

    /* zero the memory */

    lib$movc5 ((u_i2 *)&bytcnt, (u_i4 *)base_va, &fill, (u_i2 *)&bytcnt, (u_i4 *)base_va);

    /* insert all in the history queue */

    for (i=0; i<j; i++, hst++) INSQUE ((void **)&astHistoryQueue.blink, (void *)&hst->entry);
#endif

    /* 
    ** Initialise the AST jacket code to reflect whether this is a single threaded server
    ** or a multithreaded server. It is important to distinguish between the two ...
    **
    ** Multi-Threaded server:
    ** ---------------------
    **
    ** A multi threaded server (i.e. the DBMS server) has multiple threads running
    ** each of which has an associated SCB. ASTs must be executed in the context of
    ** of the thread that issued them - immediately before and after the issuing
    ** thread calls CSMTsuspend and waits on a Condition Variable
    **
    ** Single-Threaded server:
    ** ----------------------
    **
    ** A single threaded server (e.g. dmfacp) has only one thread running and does not 
    ** necessarily call CSMTsuspend. There may or may not be an SCB associated with this 
    ** thread - so ASTs must be executed on the AST dispatcher thread, not queued to
    ** an SCB
    **
    ** A multi threaded server will have specified a valid CS_CB* ccb in its call to 
    ** CSinitiate (argc, argv, ccb)
    ** A single threaded server will call CSinitiate (0, NULL, NULL)
    */

    
    IIMISC_initialise_jacket(multi_threaded); 


    /*
    ** create the high-priority ast thread
    **
    */

    assert (pthread_attr_init (&attr) == 0);

    assert (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) == 0);


    /*
    ** set the thread scheduling policy and priority explicitly
    ** in order to provide the responsiveness we require. We
    ** should be aware of the scheduling implications of this
    ** since there is always the risk of priority inversion
    ** occurring when executing the user's "AST routine"
    */


    assert (pthread_getschedparam(pthread_self(), &curr_policy, &param) == 0);

    min_prio = sched_get_priority_min(new_policy);
    max_prio = sched_get_priority_max(new_policy);
    mid_prio = (min_prio+max_prio)/2;
    astDisableMtxPrio = 2 + mid_prio;

    if ((curr_policy != new_policy) || ((curr_policy == new_policy) && (param.sched_priority <= mid_prio)))
    {
        assert (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED) == 0);

        assert (pthread_attr_setschedpolicy(&attr, new_policy) == 0);

        /* ensure the thread priority is greater than the default midpoint */

        param.sched_priority = astDisableMtxPrio;
        assert (pthread_attr_setschedparam(&attr, &param) == 0);
    }


    assert (pthread_attr_setname_np(&attr, (const char *)astThreadName, NULL) == 0);

    assert (pthread_attr_setstacksize(&attr, AST_THREAD_STACKSIZE) == 0);

    /* create the AST dispatcher thread */

    assert (pthread_create (&astThreadId, &attr, ast_thread, NULL) == 0);
}

/*
** Name:  ast_exc_handler
**
** Description:
**    Exception handler for the AST dispatcher thread
**
** Inputs:
**    CHFDEF1* sig_array
**    CHFDEF2* mch_array
**
** Outputs:
**    None
**
**  Returns:
**    cond_code for stack unwinder
**
**  Exceptions:
**    None.
**
** Side Effects:
**    May resignal or continue
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


static i4
ast_exc_handler (CHFDEF1* sig_array, CHFDEF2* mch_array)
{
    i4 cond_code;

    cond_code = sig_array->chf$l_sig_name;
    if (lib$match_cond(&cond_code, &SS$_DEBUG))  return (SS$_RESIGNAL);
    if (lib$match_cond(&cond_code, &SS$_IMGDMP)) return (SS$_CONTINUE);
    return (SS$_RESIGNAL);
}


/*
* Name:  ast_thread
**
** Description:
**    AST dispatcher thread start routine
**
** Inputs:
**    void* arg - unused - thread start rtn arg
**
** Outputs:
**    None
**
**  Returns:
**    none
**
**  Exceptions:
**    None.
**
** Side Effects:
**    Either executre AST function on this thread
**    or requeue the AST context block to the issuer thread
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/



/*
**
** called as the start procedure for the AST dispatch thread
**
** This thread waits on the ast condition variable. The normal
** procedure for a wait on condition Variable is ...
**
** (1) lock the data mutex
** (2) test the predicate
** (3) if (not ready) wait on the cond variable (implicit unlock, wait, lock)
** (4) check the cond var's predicate again for spurious wakeups
** (5) unlock the mutex
** (6) do some work
**
** This condition variable is set by the AST rtns that need
** to do any work involving mutexes/queues etc.
**
** This function is called via lib$initialize so the DECC environment
** will not initialised yet - hence all the RTL calls
*/

static void*
ast_thread(void *arg)
{
    PTHREAD_STS      ptSts;
    QUEUE_STS        qsts;
    VMS_STS          sts;
    AST_CTX*         astCtx;
    pthread_mutex_t* mtx;
    pthread_cond_t*  cv;
    struct timespec  delta = {1, 0}; /* tv_sec, tv_nsec */
    struct timespec  abstim;
    i4               retry_cnt = 10;
    i4               this_tid = pthread_getselfseq_np();
    i4               bytcnt = PTHREAD_MAX_OBJNAMLEN;
    i4               tid;
    void*            objnamBufadr;

    ast_state.ast$v_running = 1;

    /*
    ** if this thread doesn't already have an object name buffer then
    ** create it (used during error_reporting/debugging to identify the
    ** condVars/Mutexes/threadnames)
    */

    objnamBufadr = pthread_getspecific (objectNameKey);
    if (objnamBufadr == NULL)
    {
        sts = lib$get_vm(&bytcnt, &objnamBufadr);
        if (!(sts & STS$M_SUCCESS)) lib$signal(sts);
        assert (pthread_setspecific(objectNameKey, objnamBufadr) == 0);
    }


    lib$establish (ast_exc_handler);

    /*
    ** we can resume normal error reporting when we're certain that decc$main
    ** has been called (i.e. we're no longer in the context of lib$initialise)
    ** we can use decc$gl_optind to flag the fact that decc$$main has been called
    **
    */


    while (TRUE)
    {
        ptSts = 0;
        mtx   = &astDeliveryMtx;
        cv    = &astDeliveryCv;
        ptSts = pthread_mutex_lock (mtx);
        if (decc$gl_optind)
        {
            REPORT_PTHREAD_ERROR (ptSts, "pthread_mutex_lock", getObjectName((uint64 *)mtx));
        }
        else
        {
            SIGNAL_PTHREAD_ERROR(24, ptSts);
        }

        /* report the error - could be busy, deadlock or invalid mutex */

        /*
        ** is the queue empty ? - if it is then we'll wait for the cond var
        ** to be set - we'll ignore spurious wakeups (i.e.the queue is still empty)
        **
        ** remove from the head of the queue interlocked - the header must be quad word
        ** aligned and the entry must be long word aligned
        ** we'll retry as long as the interlock fails - the qhdr should only ever be
        ** manipulated by this method and the AST rtns
        */

        /* wake up every second just in case we've "missed" a condition wakeup */


        qsts = nextAst(&astCtx);
        if (!(qsts & STS$M_SUCCESS))
        {
            /* nothing in the queue (or an interlock failure )     */
            /* - so wait on the cond variable                      */
            /* an implicit (unlock, wait, lock) is performed here  */
            /* an implicit pthread_yield also occurs               */

            ptSts = pthread_cond_wait(cv, mtx);
            if (exit_now) break;
            wakeups++;

            if (decc$gl_optind)
            {
                REPORT_PTHREAD_ERROR (ptSts, "pthread_cond_wait", getObjectName((uint64 *)cv));
            }
            else
            {
                SIGNAL_PTHREAD_ERROR(1, ptSts);
            }

            /* retry the operation - this may have been a spurious wakeup */

            qsts = nextAst(&astCtx);
        }

        /* unlock the wakeup mutex */

        ptSts = pthread_mutex_unlock (mtx);
        REPORT_PTHREAD_ERROR (ptSts, "pthread_mutex_unlock", getObjectName((uint64 *)mtx));

        /*
        ** ignore spurious wake ups - we'll just grab the mutex again
        ** and probably re-enter the wait on the dispatch Cond variable
        */

        if (!(qsts & STS$M_SUCCESS))
        {
            switch (qsts)
            {
                case     LIB$_QUEWASEMP: spurious_wakeups++; break;
                case     LIB$_SECINTFAI: remqui_failures++;  break;
                default:                                     break;
            }
            continue;
        }

        /*
        ** check whether someone has done the equivalent of sys$setast(0)
        ** if yes then we'll stall until the user allows ast delivery again
        ** we'll hold this 'lock' whilst running the AST code so the user
        ** code between the 2 calls to sys$setast behave as a critical section
        ** Note that this isn't quite the same as the sys$setast service, since
        ** sys$setast merely disables the dequeueing of further ASTs, whilst
        ** only one AST may be active at any time (regardless of the number of CPUs)
        ** an AST may already be executing on a secondary CPU whilst the requesting
        ** thread is active on the primary CPU. This implementation forces the
        ** requester to wait until execution of the AST has completed.
        **
        */

        /*
        ** spin here until whoever 'disabled' ast delivery reenables it.
        ** testbitss returns non-zero if bit was already set.
        */

/*
        if (__INTERLOCKED_TESTBITSS_QUAD(&astDisabled, 0))
        {
            astdsblStalls++;
            while (TRUE)
            {
                if (!__INTERLOCKED_TESTBITSS_QUAD(&astDisabled, 0)) break;
                sched_yield();
            }
        }
        __INTERLOCKED_TESTBITCC_QUAD(&astDisabled, 0);
*/


        /*
        ** Set the AST_IN_PROGRESS flag
        */

         CS_LOCK_LONG(&astInProgress);


        /*
        ** call the AST function with its parameter
        */

        /* if shutdown request ... then exit loop and return; */

        if (astCtx->flags.ctx$v_shutdown) break;

        if (astCtx->flags.ctx$v_cantim) astCtx->qio_tmrsts = ii_sys$cantim(astCtx->qio_reqidt, 0);
        astCtx->flags.ctx$v_dispatched = TRUE;


#ifdef DEBUG_ASTS
        {
            void* astadr;
            u_i4  astprm;
            char* ssrv;
            u_i4  p3;
            i4    chan;

            sys$gettim(&astCtx->qio_astdsp_time);
            getSsrvBrief(astCtx->flags.ctx$v_ssrv, &ssrv);
            p3 = *(u_i4 *)ssrv;
            chan = (astCtx->flags.ctx$v_ssrv == CTX$C_SYS_QIO) ? astCtx->chan : 0;
        }
#endif
        tid = (astCtx->tid) ? pthread_getsequence_np(astCtx->tid) : 0;

        if ((astCtx->scb == NULL) && !astCtx->flags.ctx$v_noscb)
        {
            IICSMTget_ast_ctx2(tid, &astCtx->scb, &astCtx->evt_cv, &astCtx->ast_qhdr, &astCtx->ast_pending);
            if (astCtx->scb) astCtx->flags.ctx$v_requeue = TRUE;
        }

        if (tid == this_tid)
        {
             IIMISC_executeAst(astCtx);
        }
        else if (astCtx->flags.ctx$v_requeue && astCtx->evt_cv)
        {
            /*
            ** queue the AST context block for execution in
            ** the context of the issuer's thread
            */

            qsts = lib$insqti ((u_i4*)astCtx, (int64 *)astCtx->ast_qhdr, &retry_cnt);
            if (qsts & STS$M_SUCCESS)
            {

                /*
                ** The ast_pending field holds the address of the SCB->sc_evcb->ast_pending field
                ** This is a one-to-many relationship, i.e. we may have many ASTs still pending
                ** but just one flag in the evcb to reflect that at least one AST is waiting 
                ** execution. In CSMTINTERF.C we should be waiting on a CV at this point and we 
                ** need to know whether any ASTs are pending when we finally wake up, either as
                ** a result of a call to CS_resume or a timeout on the ConditionWait.
                ** We must be sure that the bit is clear before we set it otherwise we may spin on
                ** LOCK_LONG instruction. Since this code in this single thread alone will set the bit
                ** it is safe to only check for its present cleared state.
                ** The LOCK_LONG instruction looks after Memory Barrier stuff for us.
                */ 

                if (astCtx->ast_pending && !(*astCtx->ast_pending)) CS_LOCK_LONG(astCtx->ast_pending);

                assert (sys$dclast(pthread_cond_signal_int_np, astCtx->evt_cv, PSL$C_USER) == SS$_NORMAL);
            }
            else if ((qsts == LIB$_SECINTFAI) && tid)
            {
                if ((tid > 0) && (tid < 64)) insquiFailCnt[tid]++;
                lib$signal (LIB$_SECINTFAI);
            }

        }
        else
        {
             IIMISC_executeAst(astCtx);
        }

        /*
        ** Clear the AST_IN_PROGRESS flag
        */

        CS_UNLOCK_LONG(&astInProgress);

    }

    /*
    ** a return form a thread's main function is an implicit 
    ** pthread_exit as far as pthreads is concerned 
    */
 

    return (NULL);

}


/*
** Name:  IIMISC_executeAst
**
** Description:
**    jacket function that calls the user's AST
**
** Inputs:
**    ASTCTX* astCtx - AST context block
**
** Outputs:
**    None
**
**  Returns:
**    none
**
**  Exceptions:
**    None.
**
** Side Effects:
**    executes Users original AST
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


void
IIMISC_executeAst (AST_CTX* astCtx)
{
#ifdef DEBUG_ASTS
    i4      sts;
    u_i4    p3;
    char*   ssrv;
    void*   astadr;
    u_i4    astprm;
    i4      tid;

    astCtx->flags.ctx$v_executed = TRUE;
    sys$gettim(&astCtx->qio_astexe_time);
    astCtx->context = (AST_PRM)pthread_getselfseq_np();
    astadr = *((void **)astCtx->astadr);
    astprm = (u_i4)astCtx->astprm;
    getSsrvBrief(astCtx->flags.ctx$v_ssrv, &ssrv);
    p3 = *(u_i4 *)ssrv;
    tid = (astCtx->tid) ? pthread_getsequence_np(astCtx->tid) : 0;

    if (astCtx->flags.ctx$v_ssrv == CTX$C_SYS_QIO)
    {
        sts  = (astCtx->iosb) ? astCtx->iosb->iosb$w_status : 0;
    }
    else
    {
    }
#endif

    (*astCtx->astadr)(astCtx->astprm);

    /*
    ** return the context block to the lookaside list
    */

    IIMISC_freeAstCtx (astCtx);
}


/*
** Name:  IIMISC_executePollAst
**
** Description:
**    Calls all the users queued ASTs (from CSMTINTERF)
**
** Inputs:
**    SRELQHDR* qhdr - issuing thread's AST queue header (in SCB)
**
** Outputs:
**    None
**
**  Returns:
**    None
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


i4
IIMISC_executePollAst (SRELQHDR* qhdr)
{
    i4      astcnt = 0;
    i4      qsts;
    i4      seqno;
    i4      retry_cnt = 10;
    AST_CTX* astctx;

    seqno = pthread_getselfseq_np();

    while (TRUE)
    {
        qsts = lib$remqhi ((int64 *)qhdr, (u_i4 *)&astctx, &retry_cnt);
        if (!(qsts & STS$M_SUCCESS)) break;

        /* do the callback ...*/

        IIMISC_executeAst(astctx);
        astcnt++;
    }

    if (qsts == LIB$_SECINTFAI)
    {
        remqui_failures++;
    }

    return (astcnt);

}

/*
** Name:  nextAst
**
** Description:
**    Obtains the next queued AST context block (if any)
**    from the AST dispatcher's AST hardware interlocked queue
**
** Inputs:
**    CHFDEF1* sig_array
**    CHFDEF2* mch_array
**
** Outputs:
**    None
**
**  Returns:
**    cond_code for stack unwinder
**
**  Exceptions:
**    None.
**
** Side Effects:
**    May resignal or continue
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


static i4
nextAst (AST_CTX** pAstCtx)
{
    /* Returns one of ...
    **
    **  Success :-
    **
    **  SS$_NORMAL     - Entry removed from head of queue, queue still contains one or more entries
    **  LIB$_ONEENTQUE - Successful completion of instruction (REMQHI).
    **                   Entry removed from head of queue, but queue is now empty.
    **
    **  Failure :-
    **
    **  LIB$_SECINTFAI - Secondary Interlock failed, queue is not modified.
    **  LIB$_QUEWASEMP - Unsuccessful completion of instruction (REMQHI).
    **                   The queue was empty before the instruction was executed.
    **
    **  SS$_ROPRAND    - reserved operand fault for:
    **                   1.) either the entry or the header is at an address that is not quad word aligned.
    **                   2.) address of header equals address of entry.
    */

    i4 retry_cnt = 10;
    return (lib$remqhi ((int64 *)&astQueue, (u_i4 *)pAstCtx, &retry_cnt));
}




/*
** Name:  IIMISC_insertAstCtxLookaside
**
** Description:
**    Inserts the requested count of AST context blocks in the lookaside list.
**    Typically this function is called at startup to preallocate as many blocks
**    as the process  provides for ASTCNT quota + some additional margin
**
** Inputs:
**    i4 cnt - count of required context blocks
**
** Outputs:
**    None
**
**  Returns:
**    None
**
**  Exceptions:
**    None.
**
** Side Effects:
**    Insert AST context blocks in the lookaside list
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


void
IIMISC_insertAstCtxLookaside(i4 cnt)
{
    i4              i;
    AST_CTX*        astCtx;
    STATUS          status;
    QUEUE_STS       qsts;

    /* insert CNT context blocks in the AST lookaside list */

    for (i=0; i<cnt; i++)
    {
        astCtx = (AST_CTX *)MEreqmem(0, sizeof(AST_CTX), 0, &status);
        insertInLookasideList(astCtx);
    }
}


/*
** Name:  insertInLookasideList
**
** Description:
**    Insert an ASTCTX ast context structure in the lookaside list
**    The lookaside list is a thread/AST safe hardware interlocked
**    self relative queue.
**
** Inputs:
**    ASTCTX* - address of context block
**
** Outputs:
**    None
**
**  Returns:
**    None
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


static void
insertInLookasideList (AST_CTX* ctx)
{
    QUEUE_STS qsts      = 0;
    i4        retry_cnt = 10;

    /*
    ** return the AST context to the lookaside list
    ** Note that it is not zeroed until it is re-requested
    ** since it may still be on the history list
    */

    qsts = lib$insqti ((u_i4 *)ctx, (int64 *)&astLalQueue, &retry_cnt);
    if (qsts == LIB$_SECINTFAI)
    {
        insqui_failures++;
        insquiFailCnt[0]++;
    }
}



/*
** Name:  Debug functions for AST history
**
** Description:
**
**
** Inputs:
**    None
**
** Outputs:
**    None
**
**  Returns:
**    None
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


#ifdef DEBUG_ASTS
void
insertInHistoryList (AST_CTX* ctx)
{
    /*
    ** remove earliest entry (i.e.from head)
    ** The history queue is cyclic and should never be empty
    */

    i4          sts;
    i4          qsts;
    AST_HISTORY* hst;

    CS_LOCK_LONG(&historyLock);

    qsts = REMQUE (astHistoryQueue.flink, (void *)&hst);
    if (qsts == REMQUE_STS_EMPTY) lib$signal (SS$_BADPARAM);

     /* initialise  */

    sts = sys$gettim (&hst->time);
    if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
    hst->ctx = ctx;

    /* insert at tail (predecessor, entry) */

    qsts = INSQUE ((void **)&astHistoryQueue.blink, (void *)&hst->entry);

    CS_UNLOCK_LONG(&historyLock);

}


void
formatHistoryList (void)
{
    AST_CTX* ctx = NULL;

    ABSQENTRY* qentry;
    i4         entry_no = 0;
    i4         sts;
    i4         n;
    u_i2       timlen;
    u_i2       dt_len;
    char       tim[32];
    char       delta[32];
    char       flags[256];
    char       opbuf[256];
    char*      ssrv;
    char*      fname;
    GENERIC_64 delta_time;

    DESCRIP_S  timbuf = {sizeof(tim)-1,   DSC$K_DTYPE_T, DSC$K_CLASS_S, tim};
    DESCRIP_S  dt_buf = {sizeof(delta)-1, DSC$K_DTYPE_T, DSC$K_CLASS_S, delta};
    DESCRIP_S  op_dsc = {sizeof(opbuf)-1, DSC$K_DTYPE_T, DSC$K_CLASS_S, opbuf};


    CS_LOCK_LONG(&historyLock);

    for (qentry  = (ABSQENTRY *)astHistoryQueue.flink;
         qentry != (ABSQENTRY *)&astHistoryQueue;
         qentry  = (ABSQENTRY *)qentry->flink)
    {
        AST_HISTORY* hst = (AST_HISTORY *)qentry;
        if (hst->time.gen64$q_quadword == 0) continue;
        entry_no++;
        sts = sys$asctim (&timlen, &timbuf, &hst->time.gen64$q_quadword, 1);
        if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
        tim[timlen] = 0;
        ctx = hst->ctx;
        getSsrv(ctx->flags.ctx$v_ssrv, &ssrv);
        u_i4 seq = pthread_getsequence_np(ctx->tid);
        op_dsc.dsc$w_length = sprintf (opbuf, "[%4d] %s    astctx %p, %s (tid %d) scb %p",
                                       entry_no, tim, ctx, ssrv, seq, ctx->scb);
        lib$put_output(&op_dsc);
        flags[0] = 0;
        if (ctx->flags.ctx$v_requeue)    strcat(flags, "requeue ");
        if (ctx->flags.ctx$v_cancelled)  strcat(flags, "cancelled ");
        if (ctx->flags.ctx$v_queued)     strcat(flags, "queued ");
        if (ctx->flags.ctx$v_delivered)  strcat(flags, "delivered ");
        if (ctx->flags.ctx$v_dispatched) strcat(flags, "dispatched ");
        if (ctx->flags.ctx$v_executed)   strcat(flags, "executed ");
        if (ctx->flags.ctx$v_cantim)     strcat(flags, "cantim ");

        void*  astadr = *((void **)ctx->astadr);
        u_i4   astprm = (u_i4)ctx->astprm;
        op_dsc.dsc$w_length = sprintf (opbuf, "            astadr %p, astprm %08X (%s)", astadr, astprm, flags);
        lib$put_output(&op_dsc);
        if (ctx->flags.ctx$v_ssrv == CTX$C_SYS_QIO)
        {
            op_dsc.dsc$w_length = sprintf (opbuf, "            iosb = %p, status = %04X, chan = %04X func = %04X modifier %04X",
                                           ctx->iosb, ctx->status, ctx->chan,
                                           (ctx->func & IO$M_FCODE), (ctx->func & IO$M_FMODIFIERS));
            lib$put_output(&op_dsc);
            getFuncCode((ctx->func & IO$M_FCODE), &fname);
            op_dsc.dsc$w_length = sprintf (opbuf, "            func = %s, bytcnt = %04X, devdepend = %08X",
                                           fname, ctx->bytcnt, ctx->devdepend);
            lib$put_output(&op_dsc);
            if (ctx->flags.ctx$v_cantim)
            {
                op_dsc.dsc$w_length = sprintf (opbuf, "            cantim reqidt = %08X, sts = %04X",
                                               ctx->qio_reqidt, ctx->qio_tmrsts);
                lib$put_output(&op_dsc);
            }

            if (ctx->flags.ctx$v_executed)
            {
                sts = sys$asctim (&timlen, &timbuf, &ctx->qio_astque_time, 1);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                tim[timlen] = 0;
                sts = lib$sub_times((uint64 *)&ctx->qio_astexe_time, (uint64 *)&ctx->qio_astque_time, (uint64 *)&delta_time);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                sts = sys$asctim (&dt_len, &dt_buf, &delta_time, 0);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                delta[dt_len] = 0;
                op_dsc.dsc$w_length = sprintf (opbuf, "            astque = %s (%s)", tim, delta);
                lib$put_output(&op_dsc);

                sts = sys$asctim (&timlen, &timbuf, &ctx->qio_astdel_time, 1);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                tim[timlen] = 0;
                sts = lib$sub_times((uint64 *)&ctx->qio_astdel_time, (uint64 *)&ctx->qio_astque_time, (uint64 *)&delta_time);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                sts = sys$asctim (&dt_len, &dt_buf, &delta_time, 0);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                delta[dt_len] = 0;
                op_dsc.dsc$w_length = sprintf (opbuf, "            astdel = %s (%s)", tim, delta);
                lib$put_output(&op_dsc);

                sts = sys$asctim (&timlen, &timbuf, &ctx->qio_astdsp_time, 1);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                tim[timlen] = 0;
                sts = lib$sub_times((uint64 *)&ctx->qio_astdsp_time, (uint64 *)&ctx->qio_astdel_time, (uint64 *)&delta_time);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                sts = sys$asctim (&dt_len, &dt_buf, &delta_time, 0);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                delta[dt_len] = 0;
                op_dsc.dsc$w_length = sprintf (opbuf, "            astdsp = %s (%s)", tim, delta);
                lib$put_output(&op_dsc);

                sts = sys$asctim (&timlen, &timbuf, &ctx->qio_astexe_time, 1);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                tim[timlen] = 0;
                sts = lib$sub_times((uint64 *)&ctx->qio_astexe_time, (uint64 *)&ctx->qio_astdsp_time, (uint64 *)&delta_time);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                sts = sys$asctim (&dt_len, &dt_buf, &delta_time, 0);
                if (!(sts & STS$M_SUCCESS)) lib$signal (sts);
                delta[dt_len] = 0;
                op_dsc.dsc$w_length = sprintf (opbuf, "            astexe = %s (%s)", tim, delta);
                lib$put_output(&op_dsc);
            }

        }
    }

    CS_UNLOCK_LONG(&historyLock);
}


void
formatActiveList (void)
{
    AST_CTX* ctx = NULL;
}

#endif



/*
** Name:  IIMISC_ast_jacket
**
** Description:
**    Function actually executed by the Operating System as an AST
**    All system services capable of requesting ASTs will have been
**    redefined (by #include <astjacket>) to call the ingres function
**    e.g. sys$setimr will have been redefined as ii_sys$setimr
**    The original ASTADR/ASTPRM will now have been stored in an ASTCTX
**    block and the new astadr/astprm will be IIMISC_ast_jacket()/astCtx
**
** Inputs:
**    ASTCTX* astCtx - address of AST context block
**
** Outputs:
**    None
**
**  Returns:
**    None
**
**  Exceptions:
**    None.
**
** Side Effects:
**    Requeue ASTCTX block to the AST dispatcher thread's queue
**    and wake the dispather thread
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


void
IIMISC_ast_jacket (AST_CTX* astCtx)
{
    QUEUE_STS qsts = 0;
    i4        retry_cnt = 10;
    i4        cancelled;

    astCtx->flags.ctx$v_delivered = TRUE;

#ifdef DEBUG_ASTS
    if ((astCtx->flags.ctx$v_ssrv == CTX$C_SYS_QIO) && astCtx->iosb)
    {
        astCtx->status    = astCtx->iosb->iosb$w_status;
        astCtx->bytcnt    = astCtx->iosb->iosb$w_bcnt;
        astCtx->devdepend = astCtx->iosb->iosb$l_dev_depend;
        sys$gettim(&astCtx->qio_astdel_time);
    }
#endif

    /*
    ** If there are any particular functions that must be called
    ** at the AST level then do it now
    */

    if (astCtx->call_at_ast) (*astCtx->call_at_ast)(astCtx);


    /*
    ** If this is a timer AST (the ast parameter is the reqidt arg for $setimr)
    ** then the timer has expired - if this timer has not been previously cancelled then
    ** the CTX block will still be on the timerQueue - so remove it.
    ** The CTX block can only be on the tqeQueue or the astdeliveryQueue.
    ** Interlocks to ensure this state are maintained by the tqeTraversalLock and the
    ** cancelled bit in the context.
    **
    */

    if (astCtx->flags.ctx$v_ssrv == CTX$C_SYS_SETIMR)
    {
         CS_LOCK_LONG(&tqeTraversalLock);
        cancelled = __INTERLOCKED_TESTBITSS_QUAD(&astCtx->flags, CTX$V_CANCELLED);
        if (!cancelled)
        {
            IIMISC_removeTimerEntry(astCtx);
        }
        CS_UNLOCK_LONG(&tqeTraversalLock);

        /* if it was already cancelled then we're finished with it */

        if (cancelled) return;
    }

    /*
    ** insert the context block at the tail of the AST dispatcher queue
    **
    */


    if (FALSE && astCtx->flags.ctx$v_requeue && astCtx->evt_cv)
    {
        /*
        ** queue the AST context block for execution in
        ** the context of the issuer's thread (i.e. bypassing the
        ** AST dispatcher thread
        */

        qsts = lib$insqti ((u_i4*)astCtx, (int64 *)astCtx->ast_qhdr, &retry_cnt);
        if (qsts & STS$M_SUCCESS)
        {
            assert (pthread_cond_signal(astCtx->evt_cv) == 0);
        }
    }
    else
    {
        qsts = lib$insqti ((u_i4 *)astCtx, (int64 *)&astQueue, &retry_cnt);

        /*
        ** and wake the AST dispatcher thread
        **
        */

        if (qsts & STS$M_SUCCESS)
        {
            assert (pthread_cond_sig_preempt_int_np(&astDeliveryCv) == 0);
        }
    }

    if (!(qsts & STS$M_SUCCESS)) insqui_failures++;

    /*
    ** No further statements should appear after this signal
    ** since we may  no longer be at AST level after returning
    ** from the previous call.
    */
}




/*
** Name:  IIMISC_getAstCtx
**
** Description:
**    Get an ASTCTX block preferably from the lookaside list
**    Failing that, then from memory
**
** Inputs:
**    None
**
** Outputs:
**    None
**
**  Returns:
**    Address of ASTCTX structure
**
**  Exceptions:
**    None.
**
** Side Effects:
**    May allocate memory for the block, if lookaside list is empty
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/

AST_CTX*
IIMISC_getAstCtx (void)
{
    /*
    ** This function should never be called from ASTs since it may attempt to allocate
    ** memory. Use IIMISC_getAstCtxLookaside() instead, having called IIMISC_insertAstCtxLookaside()
    ** with a suitable count of blocks
    **
    */

    QUEUE_STS qsts = 0;
    AST_CTX*  ctx    = NULL;
    STATUS    status;
    char      fill = 0;

/*
    if (lib$ast_in_prog()) lib$signal (SS$_BADCONTEXT);
*/

    ctx = IIMISC_getAstCtxLookaside();
    if (ctx == NULL)
    {
        /* nothing in the lookaside list - so allocate from memory  */

        ctx = (AST_CTX *)MEreqmem(0, sizeof(AST_CTX), 0, &status);
        if (ctx == NULL) lib$signal (SS$_INSFMEM);
    }
    else
    {
        /* null out fields - just in case */

        u_i2 bytcnt = sizeof(AST_CTX);
        lib$movc5 (&bytcnt, (u_i4 *)ctx, &fill, &bytcnt, (u_i4 *)ctx);
    }

    ctx->tid = pthread_self();
    return (ctx);
}


/*
** Name:  IIMISC_getAstCtxLookaside
**
** Description:
**    Get an AST context block from the lookaside list
**    (A hardware interlocked selfrelative queue that
**     is AST and thread safe)
**
** Inputs:
**    none
**
** Outputs:
**    None
**
**  Returns:
**    ASTCTX context block address or NULL if none available
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


AST_CTX*
IIMISC_getAstCtxLookaside (void)
{
    /*
    ** Get an AST context block from the lookaside list
    **
    */

    QUEUE_STS qsts       = 0;
    AST_CTX*  ctx       = NULL;
    i4        retry_cnt = 10;

    qsts = lib$remqhi ((int64 *)&astLalQueue, (u_i4 *)&ctx, &retry_cnt);
    if (qsts == LIB$_SECINTFAI) remqui_failures++;
    return ((qsts & STS$M_SUCCESS) ? ctx : NULL);
}


/*
** Name:  IIMISC_freeAstCtx
**
** Description:
**    Frees an AST context block. In AST land this means
**    returning it to the lookaside list
**
** Inputs:
**    ASTCTX block address
**
** Outputs:
**    None
**
**  Returns:
**    None
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


void
IIMISC_freeAstCtx (AST_CTX* ctx)
{
    insertInLookasideList(ctx);
}


/*
** Name:  IIMISC_addTimerEntry
**
** Description:
**    Add an ASTCTX block to the timerEntry queue
**    ASTs associated with calls to sys$setimr and
**    sys$cantim have had their original reqidt
**    (i.e. the astprm)  replaced by the ASTCTX block address
**    So when we come to cancel the timer we need to be able
**    to recover our new reqidt (astctx address) from the
**    original reqidt (stored in the astctx)
**
** Inputs:
**    ASTCTX* astCtx - context block address
**
** Outputs:
**    None
**
**  Returns:
**    None
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


void
IIMISC_addTimerEntry(AST_CTX* ctx)
{
    QUEUE_STS qsts;

    CS_LOCK_LONG(&tqeTraversalLock);

    /* insert at tail (predecessor, entry) */

    qsts = INSQUE ((void **)&tqeQueue.blink, (void *)&ctx->entry);

    CS_UNLOCK_LONG(&tqeTraversalLock);


}

/*
** Name:  IIMISC_removeTimerEntry
**
** Description:
**    Remove an ASTCTX entry from the timer queue
**
** Inputs:
**    ASTCTX* astctx - address of ast context block
**
** Outputs:
**    None
**
**  Returns:
**    none
**
**  Exceptions:
**    None.
**
** Side Effects:
**    Marks the AST context block as cancelled
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


void
IIMISC_removeTimerEntry(AST_CTX* ctx)
{
    /*
    ** remove an an entry from the timer queue at some arbitrary point (entry, removed_entry)
    ** note that this function must be called with the queue locked for traversal
    */

    void* qentry = NULL;
    void* entry  = (void *)&ctx->entry;
    QUEUE_STS qsts = REMQUE ((void **)&entry, &qentry);

    __INTERLOCKED_TESTBITSS_QUAD(&ctx->flags, CTX$V_CANCELLED);

}


/*
** Name:  IIMISC_findTimerEntry
**
** Description:
**    Find the user's ASTCTX block associated with
**    their original reqidt argument
**
** Inputs:
**    __int64 reqidt - reqidt supplied to sys$cantim call
**
** Outputs:
**    None
**
**  Returns:
**    ASTCTX address or NULL if already removed
**    i.e. timer has expired
**
**  Exceptions:
**    None.
**
** Side Effects:
**    none
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/



AST_CTX*
IIMISC_findTimerEntry(unsigned __int64 reqidt)
{
    AST_CTX* ctx = NULL;

    ABSQENTRY* qentry;

    CS_LOCK_LONG(&tqeTraversalLock);

    for (qentry  = (ABSQENTRY *)tqeQueue.flink;
         qentry != (ABSQENTRY *)&tqeQueue;
         qentry  = (ABSQENTRY *)qentry->flink)
    {
        AST_CTX* ctx0 = (AST_CTX *)qentry;
        if (ctx0->flags.ctx$v_ssrv != CTX$C_SYS_SETIMR) continue;
        if (ctx0->astprm != reqidt) continue;
        IIMISC_removeTimerEntry(ctx0);
        ctx = ctx0;
        break;
    }

    CS_UNLOCK_LONG(&tqeTraversalLock);

    if (!ctx) missingTmrEntries++;
    return (ctx);
}


/*
** Name:  IIMISC_setast
**
** Description:
**    enable/disable AST delivery in the context of our
**    AST dispatcher mechanism. Note that AST delivery in the
**    sense of the operating system is always enabled. What
**    we're doing here is enableing/disabling the dequeuing of
**    ASTCTX blocks by the AST dispatcher thread
**
**
** Inputs:
**    i4 enable - flag - 1 = enable, 0 = disable
**
** Outputs:
**    None
**
**  Returns:
**    previous state of ast delivery (SS$_WASSET or SS$_WASCLR)
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


i4
IIMISC_setast(i4 enable)
{
    i4 ast_sts = 0;

/*
    if (lib$ast_in_prog())
    {
        char* showCalls = ".show calls";
        showCalls[0] = (char)(sizeof(showCalls)-2);
        lib$signal (SS$_DEBUG, 1, showCalls);
    }
*/



    if (enable)
    {   /* enable AST delivery */
        ast_sts = enable_ast_delivery ();
    }
    else
    {   /* disable AST delivery */
        ast_sts = disable_ast_delivery ();
    }

    return (ast_sts);
}


/*
** Name:  enable_ast_delivery
**
** Description:
**    Enable dequeuing of ASTs (AST context blocks)
**    by the AST dispather thread
**
** Inputs:
**    none
**
** Outputs:
**    None
**
**  Returns:
**    SS$_WASSET or SS$_WASCLR
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


static i4
enable_ast_delivery (void)
{
    i4 ast_sts;

    /*
    ** Lock the ASTEN word whilst we're peeking at it
    */

    CS_LOCK_LONG(&astEnabledPeekLock);


    ast_sts = (astEnabled) ? SS$_WASSET : SS$_WASCLR;
    if (!astEnabled) astEnabled = 1;


    /* unlock the ASTEN lock */

    CS_UNLOCK_LONG(&astEnabledPeekLock);

    /* unconditionally unlock the ASTDISABLED lock */

    __INTERLOCKED_TESTBITCC_QUAD(&astDisabled, 0);


    return (ast_sts);
}

/*
** Name:  disable_ast_delivery
**
** Description:
**    Disable ASTCTX block dequeuing by the AST dispather thread
**
** Inputs:
**    None
**
** Outputs:
**    None
**
**  Returns:
**    SS$_WASSET or SS$_WASCLR
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


static i4
disable_ast_delivery (void)
{
    i4 ast_sts;

    /*
    ** Lock the ASTEN word whilst we're peeking at it
    */

    CS_LOCK_LONG(&astEnabledPeekLock);


    ast_sts = (astEnabled) ? SS$_WASSET : SS$_WASCLR;

    /*
    ** If ASTs were enabled then mark them as disabled
    */

    if (astEnabled) astEnabled = 0;

    /* finished with the ASTEN lock, so unlock it */

    CS_UNLOCK_LONG(&astEnabledPeekLock);

    /* unconditionally lock the ASTDISABLED lock */

    __INTERLOCKED_TESTBITSS_QUAD(&astDisabled, 0);

    if (ast_sts == SS$_WASCLR)
    {
        /*
        ** ASTs were already disabled
        ** However, if an AST is in progress then we must stall
        ** here until it has finished executing. The stall
        ** is currently very inefficient since it is implemented
        ** as a simple spinlock
        **
        */


        CS_LOCK_LONG(&astInProgress);
        astipStalls++;
        CS_UNLOCK_LONG(&astInProgress);
    }

    return (ast_sts);
}




/*
** Name:  IIMISC_getAstState
**
** Description:
**    Return the state (enabled/disabled) of the ast dispatcher thread
**
** Inputs:
**    None
**
** Outputs:
**    None
**
**  Returns:
**    SS$_WASSET or SS$_WASCLR
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


AST_STATE
IIMISC_getAstState (void)
{
    return (ast_state);

}


/*
** Name:  IIMISC_destroy_astthread
**
** Description:
**    Set exit flag and wake dispatcher thread so it can exit
**
** Inputs:
**    None
**
** Outputs:
**    None
**
**  Returns:
**    None
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Jul-2009 (stegr01)   Initial Version
*/


void
IIMISC_destroy_astthread ()
{
    exit_now = TRUE;
    if (lib$ast_in_prog())
    {
        pthread_cond_sig_preempt_int_np(&astDeliveryCv);
    }
    else
    {
        pthread_cond_signal(&astDeliveryCv);
    }
}


#ifdef DEBUG_ASTS
static void
getSsrv(i4 idx, char** name)
{
    i4 i;

    i = ((idx <= 0) || (idx > CTX$C_MAX_SERVICE_NAMES)) ? 0 : idx;
    *name = ssrv_names[i];
}

static void
getSsrvBrief(i4 idx, char** name)
{
    i4 i;

    i = ((idx <= 0) || (idx > CTX$C_MAX_SERVICE_NAMES)) ? 0 : idx;
    *name = ssrv_brief[i];
}


static void
getFuncCode(i4 fcode, char** name)
{
    i4 i;
    sprintf(io_func_code, "%d", fcode);
    i =  (fcode <= IO$_LOGICAL) ? 0 :
         (fcode >= IO$_VIRTUAL) ? 0 : 
         fcode-IO$_LOGICAL;
    *name = io_func_names[i];
}



/*
** Name:  getCallersPC
**
** Description:
**    Return array of caller's PC's
**
** Inputs:
**    i4  max_pcs - maximum depth of frames whose PCs should be returned
**    i4* ret_pcs - array of returned PCs
**
** Outputs:
**    None
**
**  Returns:
**    actual count of frames on stack
**
**  Exceptions:
**    None.
**
** Side Effects:
**    None
**
** History:
**    10-Dec-2008 (stegr01)   Initial Version
*/


i4
getCallersPCs (i4 max_pcs, i4* ret_pcs)
{
    INVO_CONTEXT_BLK __align(OCTAWORD) icb;
    i4 sts;
    i4 pc_cnt = 0;
    i4 depth;
    u_i4 pc;

#ifdef __ia64
    sts = lib$i64_init_invo_context(&icb, LIBICB$K_INVO_CONTEXT_VERSION, 0);
    if (!sts) {fprintf(stderr, "invalid return from lib$i64_init_invo_context()"); return(0);}
    sts = lib$i64_get_curr_invo_context(&icb);
    depth = -2;
    if (icb.libicb$l_alert_code <= 0)
    {
        do
        {
            pc = (u_i4)icb.libicb$ih_pc;
            if ((++depth >= 0) && (pc < 0x70000000)) ret_pcs[pc_cnt++] = pc;
            if (pc_cnt >= max_pcs) break;
        }
        while (!icb.libicb$v_bottom_of_stack &&
               lib$i64_get_prev_invo_context (&icb));
    }
#endif

    return (pc_cnt);
}
#endif





/*
** if SYS$IMGACT activates the threads rtl (PTHREADS$RTL.EXE) it will insert
** PTHREAD_MAIN in the transfer vector array after SYS$IMGSTA (if it exists),
** but before calling any image initialization (LIB$INITIALIZE).
** Which is lucky for us since we need to be certain that PTHREADS has been
** properly initialised before we start our AST dispatcher thread. If this
** thread must be created and run before main() is called, we must make a
** contribution to lib$initialize to crank it into life.
** N.B. the debugger gains control AFTER any calls to lib$initialize so
** debugging the ast dispatcher thread startup could be tricky under these
** circumstances.
**
**
** Typically the transfer vector has three or less transfer addresses in it.
** They are usually ordered as in the picture below:
**
**          ----------------------------
**          ! DEBUG transfer address   ! (SYS$IMGSTA)
**          ----------------------------
**          ! OTS transfer address     ! (LIB$INITIALIZE)
**          ----------------------------
**          ! user transfer address    !
**          ----------------------------
**
**
** In the usual grand scheme of things for "C" the user transfer address will be
** the compiler generated __main()
** __main will call decc$main() and then main()
**
**
*/

/*
** contribute to the lib$initialize psect
** this will allow us to start the dispatcher thread without a specific call
** Note that this happens _before_ the debugger is initialized, so
** debugging inside LIB$INITIALIZE code is not straightforward
**
*/


/* Get "IIMISC_initialise_astmgmt()" into a valid, loaded LIB$INITIALIZE PSECT. */

/*
#pragma nostandard
 */

/*
** Establish the LIB$INITIALIZE PSECT, with proper alignment and attributes.
*/

/*
globaldef {"LIB$INITIALIZE"} readonly _align (LONGWORD) void (*a$if_1)() = IIMISC_initialise_astmgmt;
 */

/* Fake reference to ensure loading the LIB$INITIALIZE PSECT. */

/*
#pragma extern_model save
i4 lib$initialize(void);
#pragma extern_model strict_refdef
i4 dummy_lib$initialize = (int) lib$initialize;
#pragma extern_model restore

#pragma standard
 */

#else 

/* Define stubs for the OS Thread functions. */

void IIMISC_initialise_astmgmt (void) {}
void IIMISC_executeAst (void) {}
void IIMISC_executePollAst () {}
void IIMISC_insertAstCtxLookaside() {}
void IIMISC_ast_jacket () {}
void IIMISC_getAstCtx () {}
void IIMISC_getAstCtxLookaside () {}
void IIMISC_freeAstCtx () {}
void IIMISC_addTimerEntry() {}
void IIMISC_removeTimerEntry() {}
void IIMISC_findTimerEntry() {}
void IIMISC_setast() {}
void IIMISC_getAstState () {}

#endif /* OS_THREADS_USED */
