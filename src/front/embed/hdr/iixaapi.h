
/*--------------------------- iixaapi.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iixaapi.h - LIBQXA entrypoints, called from the TP system via the
**                    the Ingres xa_switch_t structure.
**
**  History:
**      25-Aug-1992 (mani)
**          First written for Ingres65 DTP/XA project.
**
*/

#ifndef  IIXAAPI_H
#define  IIXAAPI_H



/*{
+*  Name: IIxa_open  - Open an RMI, registered earlier with the TP system.
**
**  Description: 
**
**  Inputs:
**      xa_info    - character string, contains the RMI open string.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XAER_ASYNC     - if TMASYNC was specified. Currently, we don't support
**                           TMASYNC (post-Ingres65 feature).
**          XAER_RMERR     - the RMI string was ill-formed. 
**          XAER_INVAL     - illegal status argument.
**          XAER_PROTO     - if the RMI is in the wrong state. Enforces the state
**                           tables in the XA spec. 
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

FUNC_EXTERN int IIxa_open(char      *xa_info,
                          int       rmid,
                          long      flags);







/*{
+*  Name: IIxa_close   - Close an RMI.
**
**  Description: 
**
**  Inputs:
**      xa_info    - character string, contains the RMI open string. Currently,
**                   we will ignore this argument (Ingres65.)
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (post-Ingres65 feature).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_INVAL     - illegal status argument.
**          XAER_PROTO     - if the RMI/thread is in the wrong state for this call.
**                           Enforces the state table rules in the XA spec.
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

FUNC_EXTERN int IIxa_close(char      *xa_info,
                           int       rmid,
                           long      flags);





/*{
+*  Name: IIxa_start   -  Start/Resume binding a thread w/an XID.
**
**  Description: 
**
**  Inputs:
**      xid        - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                       TMJOIN/TMRESUME/TMNOWAIT/TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XA_RETRY       - if TMNOWAIT is set in flags. We will *always* assume
**                           blocking call and return this (Ingres65).
**          XA_RB*         - if this XID is marked "rollback only".
**             XA_RBROLLBACK - rollback only, unspecified reason.  
**             XA_RBCOMMFAIL - on any error on a GCA call.         
**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
**             XA_RBOTHER    - any other reason for rollback.
**             XA_RBPROTO    - protocol error, this XA call made in wrong state
**                             for the thread/RMI.
**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
**             XA_RBTRANSIENT- a transient error was detected by the DBMS server.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (post-Ingres65 feature).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_DUPID     - XID already exists, when it's not expected to be
**                           "known" at this AS, i.e. it's not TMRESUME/TMJOIN.
**          XAER_OUTSIDE   - RMI/thread is doing work "outside" of global xns.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal input arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

FUNC_EXTERN  int  IIxa_start(XID       *xid,
                             int       rmid,
                             long      flags);





/*{
+*  Name: IIxa_end   -  End/Suspend the binding of a thread w/an XID.
**
**  Description: 
**
**  Inputs:
**      xid        - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                       TMSUSPEND/TMMIGRATE/TMSUCCESS/TMFAIL/TMASYNC
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XA_NOMIGRATE   - if TMSUSPEND|TMMIGRATE is specified. We won't
**                           have support for Association Migration in Ingres65.
**          XA_RB*         - if this XID is marked "rollback only".
**             XA_RBROLLBACK - rollback only, unspecified reason.  
**             XA_RBCOMMFAIL - on any error on a GCA call.         
**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
**             XA_RBOTHER    - any other reason for rollback.
**             XA_RBPROTO    - protocol error, this XA call made in wrong state
**                             for the thread/RMI.
**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
**             XA_RBTRANSIENT- a transient error was detected by the DBMS server.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (post-Ingres65 feature).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal status argument.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

FUNC_EXTERN  int  IIxa_end(XID       *xid,
                           int       rmid,
                           long      flags);







/*{
+*  Name: IIxa_rollback   -  Propagate rollback of a local xn bound to an XID.
**
**  Description: 
**
**  Inputs:
**      xid        - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XA_RB*         - if this XID is marked "rollback only".
**             XA_RBROLLBACK - rollback only, unspecified reason.  
**             XA_RBCOMMFAIL - on any error on a GCA call.         
**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
**             XA_RBOTHER    - any other reason for rollback.
**             XA_RBPROTO    - protocol error, this XA call made in wrong state
**                             for the thread/RMI.
**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
**             XA_RBTRANSIENT- a transient error was detected by the DBMS server.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (Ingres65).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**          UNSUPPORTED in Ingres65
**          -----------------------
**
**          XA_HEURHAZ     - Work done in global xn branch may have been 
**                           heuristically completed.
**          XA_HEURCOM     - Work done may have been heuristically committed.
**          XA_HEURRB      - Work done was heuristically rolled back.
**          XA_HEURMIX     - Work done was partially committed and partially
**                           rolled back heuristically.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

FUNC_EXTERN  int   IIxa_rollback(XID       *xid,
                   int       rmid,
                   long      flags);





/*{
+*  Name: IIxa_prepare   -  Prepare a local xn bound to an XID.
**
**  Description: 
**
**  Inputs:
**      xid        - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XA_RB*         - if this XID is marked "rollback only".
**             XA_RBROLLBACK - rollback only, unspecified reason.  
**             XA_RBCOMMFAIL - on any error on a GCA call.         
**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
**             XA_RBOTHER    - any other reason for rollback.
**             XA_RBPROTO    - protocol error, this XA call made in wrong state
**                             for the thread/RMI.
**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
**             XA_RBTRANSIENT- a transient error was detected by the DBMS server.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (Ingres65).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**          UNSUPPORTED in Ingres65
**          -----------------------
**
**          XA_RDONLY      - the Xn branch was read-only, and has been
**                           committed.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

FUNC_EXTERN  int   IIxa_prepare(XID       *xid,
                                int       rmid,
                                long      flags);





/*{
+*  Name: IIxa_commit   -  Commit a local xn bound to an XID.
**
**  Description: 
**
**  Inputs:
**      xid        - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                          TMNOWAIT/TMASYNC/TMONEPHASE/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XA_RETRY       - if TMNOWAIT is set in flags.
**          XA_RB*         - if this XID is marked "rollback only". These are
**                           allowed *only* if this is a 1-phase Xn.
**             XA_RBROLLBACK - rollback only, unspecified reason.  
**             XA_RBCOMMFAIL - on any error on a GCA call.         
**             XA_RBDEADLOCK - deadlock detected down in DBMS server. 
**             XA_RBINTEGRITY- integrity violation detected in DBMS server.
**             XA_RBOTHER    - any other reason for rollback.
**             XA_RBPROTO    - protocol error, this XA call made in wrong state
**                             for the thread/RMI.
**             XA_RBTIMEOUT  - the local xn bound to this XID timed out.
**             XA_RBTRANSIENT- a transient error was detected by the DBMS server.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (Ingres65).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**          UNSUPPORTED in Ingres65
**          -----------------------
**
**          XA_HEURHAZ     - Work done in global xn branch may have been 
**                           heuristically completed.
**          XA_HEURCOM     - Work done may have been heuristically committed.
**          XA_HEURRB      - Work done was heuristically rolled back.
**          XA_HEURMIX     - Work done was partially committed and partially
**                           rolled back heuristically.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

FUNC_EXTERN  int  IIxa_commit(XID       *xid,
                              int       rmid,
                              long      flags);





/*{
+*  Name: IIxa_recover   -  Recover a set of prepared XIDs for a specific RMI.
**
**  Description: 
**
**  Inputs:
**      xids       - pointer to XID list.
**      count      - maximum number of XIDs expected by the TP system.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                          TMSTARTRSCAN/TMENDRSCAN/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          >= 0           - number of XIDs returned.
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

FUNC_EXTERN int  IIxa_recover(XID       *xids,
                              long      count,
                              int       rmid,
                              long      flags);





/*{
+*  Name: IIxa_forget   -  Forget a heuristically completed xn branch.
**
**  Description: 
**        UNSUPPORTED IN Ingres65 - Hence unexpected !
**
**  Inputs:
**      xid        - pointer to XID.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                          TMASYNC/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_OK          - normal execution.
**          XAER_ASYNC     - if TMASYNC was specified. We don't support
**                           TMASYNC (Ingres65).
**          XAER_RMERR     - internal error w/gca association mgmt.
**          XAER_RMFAIL    - RMI made unavailable by some error.
**          XAER_NOTA      - TMRESUME or TMJOIN was set, and XID not "known".
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

FUNC_EXTERN  int  IIxa_forget(XID       *xid,
                              int       rmid,
                              long      flags);





/*{
+*  Name: IIxa_complete   -  Wait for an asynchronous operation to complete.
**
**  Description: 
**        UNSUPPORTED IN Ingres65 - Hence unexpected !
**
**  Inputs:
**      handle     - handle returned in a previous XA call.
**      retval     - pointer to the return value of the previously activated
**                   XA call.
**      rmid       - RMI identifier, assigned by the TP system, unique to the AS
**                   process w/which LIBQXA is linked in.
**      flags      - TP system-specified flags, one of
**                          TMMULTIPLE/TMNOWAIT/TMNOFLAGS.
**
**  Outputs:
**	Returns:
**          XA_RETRY       - TMNOWAIT was set in flags, no asynch ops completed.
**          XA_OK          - normal execution.
**          XAER_INVAL     - illegal arguments.
**          XAER_PROTO     - if there are active/suspended XIDs bound to
**                           the RMI. Enforces XA Spec's "state table" rules.
**
**	Errors:
**	    None.
-*
**  Side Effects:
**	    None.
**	
**  History:
**	08-sep-1992  -  First written (mani)
*/

FUNC_EXTERN  int  IIxa_complete(int     *handle,
                                int     *retval,
                                int       rmid,
                                long      flags);


#endif  /* ifndef IIXAAPI_H */
/*--------------------------- end of iixaapi.h  ----------------------------*/



