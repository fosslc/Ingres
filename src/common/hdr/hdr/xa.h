/*
** Copyright (c) 2004 Ingres Corporation
*/

#ifndef XA_H_INCLUDED
#define XA_H_INCLUDED
/*
 * Start of xa.h header
 *
 */


#ifdef __cplusplus
extern "C" {
#endif

/*
 * Transaction branch identification: XID and NULLXID:
 */

#define XIDDATASIZE      128            /* size in bytes */
#define MAXGTRIDSIZE     64             /* max size in bytes of GTrid */
#define MAXBQUALSIZE     64             /* max size in bytes of BQual */

struct xid_t {
          long formatID;                /* format identifier       */
          long gtrid_length;            /* value from 1 through 64 */
          long bqual_length;            /* value from 1 through 64 */
          char data[XIDDATASIZE];
          };

typedef struct xid_t XID;

/*
 *  A value of -1 in formatID means that the XID is null.
 *
 */

/* 
 * XA Switch Data Structure
 *
 */

#define   RMNAMESZ       32          /* Length of RM name, including the */
                                     /* null terminator.                 */

#define   MAXINFOSIZE    256         /* max size in bytes of xa_info strings */
                                     /* including the null terminator.       */


struct xa_switch_t {
  char name[RMNAMESZ];              /* Name of RM */
  long flags;                       /* RM specific options */
  long version;                     /* must be 0 */
#if defined(__STDC__) || defined(__cplusplus)
  int  (*xa_open_entry)(char *,  int, long);   /* xa_open function ptr */
  int  (*xa_close_entry)(char *, int, long);   /* xa_close function ptr */
  int  (*xa_start_entry)(XID *,  int, long);   /* xa_start function ptr */
  int  (*xa_end_entry)(XID *,    int, long);   /* xa_end function ptr */
  int  (*xa_rollback_entry)(XID *,int, long);  /* xa_rollback function ptr */
  int  (*xa_prepare_entry)(XID *, int, long);  /* xa_prepare function ptr */
  int  (*xa_commit_entry)(XID *,  int, long);  /* xa_commit function ptr */
  int  (*xa_recover_entry)(XID *, long, int, long);  
                                               /* xa_recover function ptr */
  int  (*xa_forget_entry)(XID *,  int, long);  /* xa_forget function ptr */
  int  (*xa_complete_entry)(int *,int *, int, long);  
                                               /* xa_complete function ptr */
#else  /* if __STDC__ || __cplusplus */
  int  (*xa_open_entry)();                     /* xa_open function ptr */
  int  (*xa_close_entry)();                    /* xa_close function ptr */
  int  (*xa_start_entry)();                    /* xa_start function ptr */
  int  (*xa_end_entry)();                      /* xa_end function ptr */
  int  (*xa_rollback_entry)();                 /* xa_rollback function ptr */
  int  (*xa_prepare_entry)();                  /* xa_prepare function ptr */
  int  (*xa_commit_entry)();                   /* xa_commit function ptr */
  int  (*xa_recover_entry)();                  /* xa_recover function ptr */
  int  (*xa_forget_entry)();                   /* xa_forget function ptr */
  int  (*xa_complete_entry)();                 /* xa_complete function ptr */
#endif /* ifndef __STDC__ */
};


/*
 * Flag definitions for the RM switch.
 *
 */

#define   TMNOFLAGS      0x00000000L         /* No RM features selected  */
#define   TMREGISTER     0x00000001L         /* RM dynamically registers */
#define   TMNOMIGRATE    0x00000002L         /* RM does n't support AM   */
#define   TMUSEASYNC     0x00000004L         /* RM supports asynch ops   */


/* 
 *  Flag definitions for xa_ and ax_ routines
 *
 */

/* Use TMNOFLAGS, defined above, when not specifying other flags */

#define   TMASYNC        0x80000000L
#define   TMONEPHASE     0x40000000L
#define   TMFAIL         0x20000000L
#define   TMNOWAIT       0x10000000L
#define   TMRESUME       0x08000000L
#define   TMSUCCESS      0x04000000L
#define   TMSUSPEND      0x02000000L
#define   TMSTARTRSCAN   0x01000000L
#define   TMENDRSCAN     0x00800000L
#define   TMMULTIPLE     0x00400000L
#define   TMJOIN         0x00200000L
#define   TMMIGRATE      0x00100000L


/* 
 *  ax_() return codes - TM reports to RM.
 *
 */
#define   TM_JOIN        2
#define   TM_RESUME      1
#define   TM_OK          0
#define   TMER_TMERR     -1
#define   TMER_INVAL     -2
#define   TMER_PROTO     -3


/*
 *  xa_() return codes - RM reports to TM.
 *
 */

#define   XA_RBBASE      100              /* lower bound of XA error codes   */
#define   XA_RBROLLBACK  XA_RBBASE        /* rollback - unspecified reason   */
#define   XA_RBCOMMFAIL  XA_RBBASE+1      /* rollback - comm failure         */
#define   XA_RBDEADLOCK  XA_RBBASE+2      /* rollback - deadlock detected    */
#define   XA_RBINTEGRITY XA_RBBASE+3      /* rollback - integrity violation  */
#define   XA_RBOTHER     XA_RBBASE+4      /* rollback - other reasons        */
#define   XA_RBPROTO     XA_RBBASE+5      /* protocol error in RM            */
#define   XA_RBTIMEOUT   XA_RBBASE+6      /* Xn branch took too long         */
#define   XA_RBTRANSIENT XA_RBBASE+7      /* may retry the Xn branch         */
#define   XA_RBEND       XA_RBTRANSIENT   /* upper bound of rollback codes   */
#define   XA_NOMIGRATE   9                /* Xn branch must resume where the */
                                          /* suspension occured */
#define   XA_HEURHAZ     8                /* Xn branch may have been         */
                                          /* heuristically completed.        */
#define   XA_HEURCOM     7                /* Xn branch has been              */
                                          /* heuristically committed.        */
#define   XA_HEURRB      6                /* Xn branch has been heuristically*/
                                          /* rolled back                     */
#define   XA_HEURMIX     5                /* heuristically committed and then*/
                                          /* rolled back Xn branch.          */
#define   XA_RETRY       4                /* rtn returned w/no effect, and   */
                                          /* may be re-issued.               */
#define   XA_RDONLY      3                /* the Xn branch was read-only and */
                                          /* has been committed.             */
#define   XA_OK          0                /* normal execution */
#define   XAER_ASYNC    -2                /* Asynch op already outstanding   */
#define   XAER_RMERR    -3                /* RM error occurred in Xn branch  */
#define   XAER_NOTA     -4                /* the XID is invalid              */
#define   XAER_INVAL    -5                /* the XA call args were invalid   */
#define   XAER_PROTO    -6                /* XA rtn called in improper cntxt */
#define   XAER_RMFAIL   -7                /* RM unavailable                  */
#define   XAER_DUPID    -8                /* the XID already exists          */
#define   XAER_OUTSIDE  -9                /* the RM is doing work outside a  */
                                          /* global transaction.             */

#ifdef __cplusplus
}
#endif
/*
 *  End of xa.h header
 */
#endif /* XA_H_INCLUDED */
