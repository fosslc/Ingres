/*
**    Copyright (c) 2008 Ingres Corporation
**
*/

/*
** Name: astmgmt.h - AST management structures
**
**
** History:
**  18-Jun-2007 (stegr01)
**  22-dec-2008 (stegr01)
**     cs.h required before csnormal.h.
*/

#ifndef AST_MGMT_H_INCLUDED
#define AST_MGMT_H_INCLUDED

#include <compat.h>

#ifdef OS_THREADS_USED
#include <cs.h>
#include <csnormal.h>


#include <ints.h>
#include <types.h>

#include <queuemgmt.h>

#include <iosbdef.h>
#include <gen64def.h>
#include <pthread.h>


/* AST stuff */

typedef void   (*AST_RTN)(__unknown_params);
typedef unsigned __int64  AST_PRM;

typedef struct _AST_FLAGS
{
    unsigned ctx$v_cancelled     : 1;     /* 0   $cantim has cancelled this Timer      */
    unsigned ctx$v_queued        : 1;     /* 1   ssrv queued - waiting ast delivery    */
    unsigned ctx$v_delivered     : 1;     /* 2   ast delivered - queued to dispatcher  */
    unsigned ctx$v_dispatched    : 1;     /* 3   ast dispatched by ast_dispatch thread */
    unsigned ctx$v_executed      : 1;     /* 4   ast executed by issuer thread         */
    unsigned ctx$v_requeue       : 1;     /* 5   requeue to issuer thread              */
    unsigned ctx$v_cantim        : 1;     /* 6   issue $cantim for this reqidt         */
    unsigned ctx$v_shutdown      : 1;     /* 7   shutdown dispatcher thread            */
    unsigned ctx$v_exeast        : 1;     /* 8   execute function within ast           */
    unsigned ctx$v_noscb         : 1;     /* 9   don't use scb for ast execution       */
    unsigned ctx$v_unused_10_15  : 6;     /* 10  6  spare flags                        */
    unsigned ctx$v_ssrv          : 16;    /* 16  system service type                   */
    unsigned ctx$v_unused_32_63  : 32;    /* make sure this is a quadword for interlocked bit testing */
} AST_FLAGS, *LPAST_FLAGS;

#define CTX$V_CANCELLED   0
#define CTX$V_QUEUED      1
#define CTX$V_DELIVERED   2
#define CTX$V_DISPATCHED  3
#define CTX$V_EXECUTED    4
#define CTX$V_REQUEUE     5
#define CTX$V_CANTIM      6
#define CTX$V_SHUTDOWN    7
#define CTX$V_EXEAST      8
#define CTX$V_NOSCB       9

#define CTX$M_CANCELLED   0x0001
#define CTX$M_QUEUED      0x0002
#define CTX$M_DELIVERED   0x0004
#define CTX$M_DISPATCHED  0x0008
#define CTX$M_EXECUTED    0x0010
#define CTX$M_REQUEUE     0x0020
#define CTX$M_CANTIM      0x0040
#define CTX$M_SHUTDOWN    0x0080
#define CTX$M_EXEAST      0x0100
#define CTX$M_NOSCB       0x0200

#define CTX$C_SYS_WAKE             1
#define CTX$C_SYS_HIBER            2
#define CTX$C_SYS_SCHDWK           3
#define CTX$C_SYS_BRKTHRU          4
#define CTX$C_SYS_CHECK_PRIVILEGE  5
#define CTX$C_SYS_DCLAST           6
#define CTX$C_SYS_ENQ              7
#define CTX$C_SYS_ENQW             8
#define CTX$C_SYS_GETDVI           9
#define CTX$C_SYS_GETLKI           10
#define CTX$C_SYS_GETJPI           11
#define CTX$C_SYS_GETQUI           12
#define CTX$C_SYS_GETRMI           13
#define CTX$C_SYS_GETSYI           14
#define CTX$C_SYS_QIO              15
#define CTX$C_SYS_SETAST           16
#define CTX$C_SYS_SETIMR           17
#define CTX$C_SYS_CANTIM           18
#define CTX$C_SYS_SNDJBC           19
#define CTX$C_SYS_ACM              20
#define CTX$C_SYS_AUDIT_EVENT      21
#define CTX$C_SYS_CPU_TRANSITION   22
#define CTX$C_SYS_DNS              23
#define CTX$C_SYS_FILES_64         24
#define CTX$C_SYS_GETDTI           25
#define CTX$C_SYS_IO_FASTPATH      26
#define CTX$C_SYS_IO_SETUP         27
#define CTX$C_SYS_IPC              28
#define CTX$C_SYS_LFS              29
#define CTX$C_SYS_SETCLUEVT        30
#define CTX$C_SYS_SET_SYSTEM_EVENT 31
#define CTX$C_SYS_SETEVTAST        32
#define CTX$C_SYS_SETPFM           33
#define CTX$C_SYS_SETPRA           34
#define CTX$C_SYS_SETDTI           35
#define CTX$C_SYS_SETUAI           36
#define CTX$C_SYS_UPDSEC           37
#define CTX$C_SYS_XFS_CLIENT       38
#define CTX$C_SYS_XFS_SERVER       39
#define CTX$C_SYS_UNUSED_1         40
#define CTX$C_SYS_UNUSED_2         41
#define CTX$C_SYS_UNUSED_3         42
#define CTX$C_SYS_UNUSED_4         43
#define CTX$C_SYS_UNUSED_5         44
#define CTX$C_SYS_UNUSED_6         45
#define CTX$C_SYS_UNUSED_7         46
#define CTX$C_SYS_UNUSED_8         47
#define CTX$C_SYS_UNUSED_9         48
#define CTX$C_SYS_UNUSED_10        49


#define CTX$C_RMS_CLOSE            50
#define CTX$C_RMS_CONNECT          51
#define CTX$C_RMS_CREATE           52
#define CTX$C_RMS_DELETE           53
#define CTX$C_RMS_DISCONNECT       54
#define CTX$C_RMS_DISPLAY          55
#define CTX$C_RMS_ENTER            56
#define CTX$C_RMS_ERASE            57
#define CTX$C_RMS_EXTEND           58
#define CTX$C_RMS_FIND             59
#define CTX$C_RMS_FLUSH            60
#define CTX$C_RMS_FREE             61
#define CTX$C_RMS_GET              62
#define CTX$C_RMS_NXTVOL           63
#define CTX$C_RMS_OPEN             64
#define CTX$C_RMS_PARSE            65
#define CTX$C_RMS_PUT              66
#define CTX$C_RMS_READ             67
#define CTX$C_RMS_RELEASE          68
#define CTX$C_RMS_REMOVE           69
#define CTX$C_RMS_RENAME           70
#define CTX$C_RMS_REWIND           71
#define CTX$C_RMS_SEARCH           72
#define CTX$C_RMS_SPACE            73
#define CTX$C_RMS_TRUNCATE         74
#define CTX$C_RMS_UPDATE           75
#define CTX$C_RMS_WAIT             76
#define CTX$C_RMS_WRITE            77

#define CTX$C_MAX_SERVICE_NAMES    77


typedef struct _AST_CTX
{
    SRELQENTRY          entry;          /* 00 AST lookaside list queue entry                 */
    ABSQENTRY           timerq;         /* 08 Timer queue reqidt lookup entry                */
    AST_RTN             call_at_ast;    /* 10 function to be called at AST level             */
    AST_RTN             astadr;         /* 14 function to be called in AST dispatcher thread */
    AST_PRM             astprm;         /* 18 function parameter                             */
    AST_FLAGS           flags;          /* 1C miscellaneous flags                            */
    SRELQHDR*           ast_qhdr;       /* 20 ptr to self relative AST queue header          */
    CS_COND*            evt_cv;         /* 28 ptr to Event Cond Var in SCB                   */
    CS_THREAD_ID        tid;            /* 2C issuer's thread id (TEB)                       */
    AST_PRM             context;        /* 30 some userdefined ast context                   */
    void*               scb;            /* 34 target thread's session control block          */
    u_i4                ret_pc;         /* 38 callers PC                                     */
    u_i4*               ast_pending;    /* 3C AST pending mask address                       */
    union
    {
        u_i4            icc_bufsiz;     /* 44 buffer size (for ICC)                          */
        u_i4            qio_reqidt;     /* 44 timer reqidt for timeout (for QIO)             */
    } ctx$r_qio_icc_1;
    union
    {
        u_i4            icc_handle;     /* 48 AST handle  (for ICC)                          */
        u_i4            qio_tmrsts;     /* 48 $cantim status (for QIO)                       */
    } ctx$r_qio_icc_2;

} AST_CTX, *LPAST_CTX;

#define icc_bufsiz  ctx$r_qio_icc_1.icc_bufsiz
#define qio_reqidt  ctx$r_qio_icc_1.qio_reqidt
#define icc_handle  ctx$r_qio_icc_2.icc_handle
#define qio_tmrsts  ctx$r_qio_icc_2.qio_tmrsts


typedef struct _AST_HISTORY
{
    ABSQENTRY           entry;
    AST_CTX*            ctx;
    GENERIC_64          time;

} AST_HISTORY, *LPAST_HISTORY;


typedef struct _AST_STATE
{
    unsigned ast$v_sts           : 16;     /* 0   AST delivery sts (SS$_WASSET/SS$_WASCLR) */
    unsigned ast$v_ready         : 1;      /* 16  AST management ready */
    unsigned ast$v_once          : 1;      /* 17  pthread once called  */
    unsigned ast$v_running       : 1;      /* 18  pthread dispatch thread running */
    unsigned ast$v_unused_19_13  : 13;
} AST_STATE, *LPAST_STATE;


void      IIMISC_initialise_astmgmt (void* ccb);
void      IIMISC_initialise_jacket (i4 multi_threaded);
void      IIMISC_insertAstCtxLookaside(i4 cnt);
AST_CTX*  IIMISC_getAstCtx (void);
AST_CTX*  IIMISC_getAstCtxLookaside (void);
void      IIMISC_freeAstCtx(AST_CTX* ctx);
i4        IIMISC_setast(i4 flag);
void      IIMISC_ast_jacket (AST_CTX* ctx);
AST_STATE IIMISC_getAstState (void);
void      IIMISC_addTimerEntry(AST_CTX* ctx);
void      IIMISC_removeTimerEntry(AST_CTX* ctx);
AST_CTX*  IIMISC_findTimerEntry(unsigned __int64 reqidt);
void      IIMISC_executeAst (AST_CTX* ctx);
i4        IIMISC_executePollAst (SRELQHDR* qhdr);

#endif /* OS_THREADS_USED */

#endif    /*  AST_MGMT_H_INCLUDED */
