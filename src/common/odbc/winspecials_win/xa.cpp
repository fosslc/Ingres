/*
** Copyright (c) 1993, 2007 Ingres Corporation
*/

extern "C"
{
#include <compat.h>

#include <sql.h>                    /* ODBC Core definitions */
#include <sqlext.h>                 /* ODBC extensions */
#include <sqlstate.h>               /* ODBC driver sqlstate codes */
#include <sqlcmd.h>

#include <gl.h>
#include <iicommon.h>
#include <ci.h>
#include <cv.h>
#include <er.h>
#include <st.h>
#include <tm.h>

#include <iiapi.h>

#include <sqlca.h>
#include <idmsxsq.h>                /* control block typedefs */
#include <idmsutil.h>               /* utility stuff */
#include "caidoopt.h"              /* connect options */
#include "idmsoinf.h"               /* information definitions */
#include "idmsoids.h"               /* string ids */
#include <idmseini.h>               /* ini file keys */
#include "idmsodbc.h"               /* ODBC driver definitions             */
#include <idmsucry.h>
}
/* #define DTC_DEBUG_MESSAGE_BOX_ENABLED */

#if _MSC_VER >= 1100    /* fix MS C compiler old/new version problems */
#define DECLSPEC_UUID(x)    __declspec(uuid(x))
#if !defined(NT_IA64) && !defined(NT_AMD64)
#ifndef MIDL_INTERFACE
#define MIDL_INTERFACE(x)   struct __declspec(uuid(x)) __declspec(novtable)
#endif /* MIDL_INTERFACE */
#endif  /* NT_IA64 && NT_AMD64 */
#else  /* else _MSC_VER < 1100 */
#define DECLSPEC_UUID(x)
#define MIDL_INTERFACE(x)   struct
#endif

#define SafeRelease(pUnk) {if (pUnk){pUnk -> Release();pUnk = NULL; }}
#define _TMPROTOTYPES               /* use the full TM function prototypes */
#define COBJMACROS                  /* bring out the macros */
/*
#define ENABLE_ASSERT
*/        /* enable _ASSERTE only if using debug CRT library else
             CrtDbgReport function called by ASSERTE macro is unresolved
             at link time */
#include <xolehlp.h>                /* DTC helper APIs     */
#include <oletx2xa.h>               /* OLE Transaction to XA interface     */
#include <txcoord.h>                /* ITransactionResourceAsync interface */
#include <process.h>
#include <crtdbg.h>                 /* for assert() and asserte()  */

#ifndef XACT_E_XA_TX_DISABLED  /* old winerror.h version don't have this yet */
#define XACT_E_XA_TX_DISABLED ((HRESULT)(0x8004D026L))
#endif

/*
** Name: XA.CPP
**
** Description:
**	Enlist in DTC and XA transaction routines for ODBC driver.
**
** History:
**	13-mar-1998 (thoda04)
**	    Initial coding
**	16-mar-1999 (thoda04)
**	    Rework side-effect of headers for UNIX.
**	03-aug-1999 (thoda04)
**	    Added more debugging display to callbacks
**	25-oct-1999 (thoda04)
**	    Added SET SESSION XID=....
**	07-nov-2000 (thoda04)
**	    Convert nat's to i4
**	27-mar-2001 (lunbr01)
**	    Uncomment "include process.h" for MTS/DTC
**	05-jul-2001 (somsa01)
**	    DO not define MIDL_INTERFACE for NT_IA64, and include
**	    oletx2xa.h from the normal include directory. Also,
**	    cleaned up compiler warnings.
**	05-sep-2001 (thoda04)
**	    Add debugging MessageBoxes.
**	21-oct-2001 (somsa01)
**	    In CRmAsyncObj::StartTxResourceAsyncThread(), changed type
**	    of rc to SIZE_TYPE.
**	29-oct-2001 (thoda04)
**	    Corrections to 64-bit compiler warnings change.
**      14-feb-2003 (loera01) SIR 109643
**          Minimize platform-dependency in code.  Make backward-compatible
**          with earlier Ingres versions.
**     17-oct-2003 (loera01)
**          Changed "IIsyc_" routines to "odbc_".
**	02-oct-2003 (somsa01)
**	    Ported to NT_AMD64.
**	17-Jun-2004 (drivi01)
**	    Removed licensing references.
**	 10-jul-2007 (thoda04) Bug 118707
**	     Avoid waiting hang on delistment when running under .NET TransactionScope.
**     18-Nov-2010 (stial01) SIR 124685 Prototype Cleanup
**          Fix windows build for prototye cleanup changes.
*/

extern "C"
{
extern HINSTANCE   hXolehlpModule;  /* module instance handle of MS DTC helper */
extern char        szModuleFileName[MAX_PATH];
                     /* driver file name, usually c:/winnt/system32/caiiod35.dll */
}  /* end extern "C" */


//#include <iixagbl.h>
#define  IIXA_XID_STRING_LENGTH  351

#define BRANCHSEQNUM 0
#define BRANCHFLAG  (IIAPI_XA_BRANCH_FLAG_FIRST | \
                     IIAPI_XA_BRANCH_FLAG_LAST  | \
                     IIAPI_XA_BRANCH_FLAG_2PC)   /* branchFlag = 7 */

/*#include <dbdbms.h> */
#define DBA_XA_TUXEDO_LIC_CHK 0x10

/* convenient CreateEvent values */
#define INITIAL_STATE_IS_FALSE   FALSE
#define MANUAL_RESET             TRUE
#define AUTO_RESET               FALSE

typedef EXPORTAPI (__cdecl *DTCGTMPROC)( char *  pszHost,
                                         char *  pszTmName,
                                    /* in */ REFIID rid,
                                    /* in */ DWORD  dwReserved1,
                                    /* in */ WORD   wcbReserved2,
                                    /* in */ void FAR * pvReserved2,
                                    /*out */ void** ppvObject
                                    );
static DTCGTMPROC lpfnDtcGetTransactionManager=NULL;

extern "C"
       void      ProcessDTCerr(HRESULT hr, LPSQLCA psqlca, char * s);
static void      odbc_write_erlog(char * text);
static II_BOOL   SetSessionXID(LPDBC pdbc, SESS * pSession, 
                               LPSQLCA psqlca, XID* pxid);
static II_BOOL isIngresXAlicensed(LPDBC pdbc,SESS * pSession,LPSQLCA psqlca);

II_BOOL odbc_registerXID(II_PTR *tranHandle, XID * pxid, LPSQLCA psqlca);
II_BOOL odbc_releaseXID (II_PTR *tranHandle, LPSQLCA psqlca);
void __cdecl TxResourceAsyncThread(void * thisparm);
char * FormatOpenString(LPDBC pdbc, char * s);

//--------------------------------------------------------------------- 
// TXSTATE: // Definition of Resource Manager states. 
// NB:  The Resource Manager only processes one transaction at a time. 
//      After initialization the Resource Manager states reflect the  
//      transaction states.
//  
typedef enum {
    TX_UNINITIALIZED=0,
    TX_INITIALIZING=1, TX_INITIALIZED=2,
    TX_ENLISTING=3,    TX_ENLISTED=4,
    TX_PREPARING=5,    TX_PREPARING_AND_COMMITTING_TOGETHER=6,
    TX_PREPARING_SINGLE_PHASE=7,    TX_PREPARED=8,
    TX_COMMITTING=9,   TX_COMMITTED=10,
    TX_ABORTING=13,    TX_ABORTED=14,
    TX_DONE=16, 
    TX_TMDOWNING=17, TX_TMDOWN=18, TX_INVALID_STATE=19 } TXSTATE;  

class CRmAsyncObj :
   public ITransactionResourceAsync   /* DTC Callback interface */
{
public:
// IUnknown
    STDMETHODIMP            QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

// ITransactionResourceAsync
    STDMETHODIMP            PrepareRequest(BOOL fRetaining, 
                                           DWORD grfRM,
                                           BOOL fWantMoniker,
                                           BOOL fSinglePhase);
    STDMETHODIMP            CommitRequest(DWORD grfRM, XACTUOW *pNewUOW);
    STDMETHODIMP            AbortRequest(BOID *pboidReason,
                                         BOOL fRetaining,
                                         XACTUOW *pNewUOW);
    STDMETHODIMP            TMDown(void);

public:
// General routines and data
   IUnknown* CreateInstance();  /* create instance of IUnknown interface */
   HRESULT CommitTx(void);
   HRESULT PrepareTx(void);
   HRESULT RollbackTx(void);
   HRESULT StartTxResourceAsyncThread(TXSTATE eState);
   void    TxResourceAsync(void);
   HRESULT EnlistTx(void);
   TXSTATE SetState(TXSTATE eState);
   TXSTATE GetState();
   char *  GetStateString(TXSTATE eState);
   IDtcToXaHelperSinglePipe    * m_pIDtcToXaHelper;
   ITransactionEnlistmentAsync * m_pITransactionEnlistmentAsync;
   LPDBC m_pdbc;               /* -> DBC  */ 
   TXSTATE m_eTxState;         /* Transaction state */
   HANDLE  hCommitEvent;  // Use to coordinate StartCommitTx and StartCommitTxOnThread
   HANDLE  hTxResourceAsyncEvent;  // event to wait on for TxResourceAsyncThread
   CRITICAL_SECTION csTxResourceAsync;
   CRmAsyncObj(LPDBC pdbc, IDtcToXaHelperSinglePipe  * pIDtcToXaHelper);
   ~CRmAsyncObj();

private:
   long  m_cRef;               /* reference count */

}; /* end class CRmAsyncObj */



/*
**  EnlistInDTC
**
**  Enlist in the MS Distributed Transaction Coordinator.
**
**  Called eventually from SQLSetConnectOption(SQL_ATTR_ENLIST_IN_DTC)
**  who, in turn, was called by the MTXDM.DLL connection pooling component.
**  Associates the transaction object with the ODBC database connection.
**  Directs that further work on the ODBC connection to be performed
**  under the auspices of the MS DTC transaction.
**
**  On entry: pdbc              -->DBC
**            pITransactionParm -->Transaction Interface object
**
**  Returns:  SQL_SUCCESS     Success
**            SQL_ERROR       Error of some kind
*/

EXTERN_C RETCODE EnlistInDTC(LPDBC pdbc, II_PTR pITransactionParm)
{
  HRESULT hr;
  RETCODE rc=SQL_ERROR;
  LPSQLCA psqlca=&pdbc->sqlca;
  LPSESS  psess =&pdbc->sessMain;
  char  * pszDSN= (char*)(pdbc->szDSN);
  XID     xid;
  CRmAsyncObj * pRmAsyncObj=NULL;

  IUnknown*                   pIUnknown                 =NULL;
  IDtcToXaHelperSinglePipe  * pIDtcToXaHelper           =NULL;
  ITransactionResourceAsync * pITransactionResourceAsync=NULL;

  ITransaction * pITransaction = (ITransaction*)pITransactionParm;


  for(;;)   /* forever loop to conveniently break out of */
  {
    if (hXolehlpModule==NULL)  /* load MS DTC helper if not loaded yet */
       {
        char s[256];
        char es[80];
        DWORD dw;

        hXolehlpModule=LoadLibrary("xolehlp.dll");  /* MS DTC helper */
        if (hXolehlpModule==NULL)   /* if load failed */
           { dw = GetLastError();
             wsprintf(es, "  LoadLibrary error code = %d", dw);
             if (dw == 1154)  strcat(es, " (DLL is not a valid 32-bit DLL)");
             if (dw == 1157)  strcat(es, " (DLL could not be found in the path)");
             wsprintf(s, "xolehlp.dll (MS DTC Helper) could not be loaded.");
             strcat(s,es);    /* strcat on the LoadLibrary error code and explanation) */
             ProcessDTCerr(E_FAIL,psqlca,s);
             break; 
           }
       }
    if (lpfnDtcGetTransactionManager==NULL)  /* load MS DtcGetTransactionManager if not loaded yet */
       {lpfnDtcGetTransactionManager = (DTCGTMPROC)GetProcAddress(hXolehlpModule,"DtcGetTransactionManager");
        if (lpfnDtcGetTransactionManager==NULL)
           { ProcessDTCerr(E_FAIL,psqlca,"Failed to enlist; DtcGetTransactionManager() could not be loaded");
             break;  /* break out and return SQL_ERROR */
           }
       } 
    /*  
    Call DtcGetTransactionManager in MS DTC proxy to establish a connection
    to the Transaction Manager.  Use NULL for the name and location of the
    Transaction Manager so that we connect with the default MS DTC Transaction
    Manager as defined in the system registry.  */

    hr = lpfnDtcGetTransactionManager(NULL, NULL, IID_IUnknown, 0, 0, NULL,
                                   (void**) &pIUnknown);
    if (FAILED(hr))
       { ProcessDTCerr(hr,psqlca,"Failed to enlist on MS Distributed Transaction Coordinator (Check that MS DTC service is started.)");
         break;  /* break out and return SQL_ERROR */
       } 

    hr = pIUnknown->QueryInterface(IID_IDtcToXaHelperSinglePipe, 
                                                   (void**)&pIDtcToXaHelper);
    SafeRelease(pIUnknown);    /* release the IUnknown interface */
    if (FAILED(hr))
       { ProcessDTCerr(hr,psqlca,"Failed to get IDtcToXaHelperSinglePipe interface during EnlistInDTC.  Please upgrade MS DTC.");
         break; /* break out and return SQL_ERROR */
       }
		 
    /*  
    Obtain the Resource Manager (RM) cookie.  The XARMCreate
    method is invoked once per ODBC connection.  The information provided
    makes it possible for the transaction manager to connect to the
    database to perform recovery.  The transaction manager durably records
    this information in it's log file.  If recovery is necessary, the 
    transaction manager reads the log file, reconnects to the database,
    and initiates XA recovery.

    Calling XARMCreate results in a message from the DTC proxy
    to the XA Transaction Manager (TM).  When the XA TM receives a message that
    contains the DSN and the name of the client DLL:
       1) LoadLibrary the client dll (caiiod35.dll).
       2) GetProcAddress the GetXaSwitch function in caiiod35.dll.
       3) Calls the GetXaSwitch function to obtain the XA_Switch.
       4) Calls the xa_open function to open an XA connection with the 
          resource manager (Ingres).  When the our xa_open function is called,
          the DTC TM passes in the DSN as the OPEN_STRING.  It then
          closes the connection by calling xa_close.  (It's just testing
          the connection information so that it won't be surprised
          when it's in the middle of recovery later.)
       5) Generate a RM GUID for the connection.
       6) Write a record to the DTCXATM.LOG file indicating that a new
          resource manager is being established.  The log record contains
          the DSN name, the name of our ODBC/XA driver (caiiod35.dll), and 
          the RM GUID.  This information will be used to reconnect to
          Ingres should recovery be necessary.

    XARMCreate method also causes the MS DTC proxy to create a resource
    manager object and returns an opaque pointer to that object as
    the RMCookie.  This RMCookie represents the connection to the TM.    */

//  if (pITransaction!=NULL && Connection_State==LOCAL_TRANSACTION_STATE)
    if (pITransaction       && pdbc->dwDTCRMCookie==0)
       {char XAOpenString[300];

        if (pdbc->fRelease < fReleaseRELEASE20)   
                /* if pre-OpenIngres 2.0, there is no fixed XA support */
           { ProcessDTCerr(E_FAIL,psqlca,"Failed to enlist; No XA capability for old release of Ingres server.  Need to upgrade.");
             break;  /* break out and return SQL_ERROR */
           }

        if (!isIngresXAlicensed(pdbc, psess, psqlca)) /* CHECK for CI capability */
           { //ERget(E_LQ00DA_XA_NOAUTH);
             ProcessDTCerr(E_FAIL,psqlca,"Failed to enlist; No INGRES capability authorized for the TM-XA interface.");
             break;  /* break out and return SQL_ERROR */
           }

        FormatOpenString(pdbc, XAOpenString); /* create the caiiod35.dll XA OpenString */

        hr=pIDtcToXaHelper->XARMCreate(    /* create XA Resource Manager connection */
              XAOpenString,                /* XA OpenString passed as xa_info in xa_open */
              szModuleFileName,        /* our caiiod35.dll name to LoadLibrary for GetXaSwitch */
              &pdbc->dwDTCRMCookie); /* where to put cookie representing the RM */

        if (FAILED(hr))
           { ProcessDTCerr(hr,psqlca,"Failed on XARMCreate during EnlistInDTC. See Ingres error log.");
             break;  /* break out and return SQL_ERROR */
           } 
       }  /* end if Connection_State==LOCAL_TRANSACTION_STATE */

    if (pITransaction)  /* if enlisting */
       {
#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
    MessageBox(NULL, "Application is ENLISTING in DTC", "EnlistInDTC", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

        /* Obtain an XID from an ITransaction interface pointer.
           The ITransaction pointer is offered by a transaction object.
           The transaction object contains a DTC transaction that is
           identified by a transaction identifier (TRID) which is a globally 
           unique identifier (GUID).  Given a TRID, one can always create an XID.
           The MS DTC proxy generates an XID and returns it.

           Into the XID's gtrid, MS puts a GUID identifying the global
           transaction identifier.  Into the XID's bqual, MS puts a second
           GUID identifying the TM that is coordinating the transaction and
           also a third GUID identifying the database connection.
           If two components in the same transaction have their own connections,
           then they will be loosely coupled XA threads of control (different
           bquals).  MS goes beyond the XA spec and expects that the two
           loosely coupled threads will not deadlock on resource locks.
        */
        hr=pIDtcToXaHelper->ConvertTridToXID(
              (DWORD*)pITransaction,pdbc->dwDTCRMCookie,&xid);
        if (FAILED(hr))
           { ProcessDTCerr(hr,psqlca,"Failed on ConvertTridToXID during EnlistInDTC");
             break;  /* break out and return SQL_ERROR */
           }
        if (xid.gtrid_length > MAXGTRIDSIZE  ||  /* safety check */
            xid.gtrid_length < 0             ||
            xid.bqual_length > MAXBQUALSIZE  ||
            xid.bqual_length < 0             ||
            (xid.gtrid_length==0  &&  xid.bqual_length==0))
           { char buf[256];
             wsprintf(buf,"Invalid XID from MS DTC ConvertTridToXID during EnlistInDTC.\
 gtrid_length=%d; bqual_length=%d.", (int)xid.gtrid_length, (int)xid.bqual_length);
             ProcessDTCerr(E_FAIL, psqlca, buf);
             break;  /* break out and return SQL_ERROR */
           }
#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
        MessageBox(NULL, "XID created", "Debug EnlistInDTC", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

        /* send the XID to the Ingres back-end */
        if (!odbc_registerXID(&psess->XAtranHandle, &xid, psqlca))
           { 
             if (pdbc->dwDTCRMCookie)
                { pIDtcToXaHelper->ReleaseRMCookie(pdbc->dwDTCRMCookie, TRUE);
                                  /* normal close */
                  pdbc->dwDTCRMCookie=0;
                }
             break;  /* register XID failed; break out and return SQL_ERROR 
                        and the error msg already issued by odbc_registerXID.*/
           }

        /* Tell the DTC proxy to enlist on behalf of the XA resource manager.
           EnlistWithRM is called for every transaction that the connection
           is requested to be enlisted in. The ODBC driver is responsible for
           implementing the ITransactionResourceAsync interface.  This
           interface includes the PrepareRequest, CommitRequest, AbortRequest,
           and TMDown callbacks.  The transaction manager will invoke these
           callback methods to deliver phase one and phase two notifications
           to the ODBC driver.
           The MS DTC proxy informs the transaction manager that the ODBC
           driver wishes to enlist in the transaction.  The transaction
           manager creates an internal enlistment object to record the fact
           that the ODBC driver is participating in the transaction.
           Upon return from EnlistWithRM, the MS DTC proxy will have created
           an enlistment object and returns the pITransactionEnlistmentAsync
           interface pointer of the enlistment object back to the ODBC driver.
           */

        /* Build ITransactionResourceAsync interface object */
        pRmAsyncObj = new CRmAsyncObj(pdbc, pIDtcToXaHelper);
        if (pRmAsyncObj==NULL)
           { hr=E_OUTOFMEMORY;
             ProcessDTCerr(hr,psqlca,"Failed to allocate CRmAsyncObj during EnlistInDTC");
             break; /* break out and return SQL_ERROR */
           }

        pIUnknown = pRmAsyncObj->CreateInstance();
        if (pIUnknown==NULL)
           { hr=E_NOINTERFACE;
             ProcessDTCerr(hr,psqlca,"Failed to get IUnknown from CRmAsyncObj during EnlistInDTC");
             break; /* break out and return SQL_ERROR */
           }

        hr = pIUnknown->QueryInterface(IID_ITransactionResourceAsync, 
                                                       (void**)&pITransactionResourceAsync);
        SafeRelease(pIUnknown);    /* release the IUnknown interface */
        if (FAILED(hr))
           { ProcessDTCerr(hr,psqlca,"Failed to get ITransactionResourceAsync during EnlistInDTC");
             break; /* break out and return SQL_ERROR */
           }

        pdbc->hMSDTCTransactionCompleting =
                  CreateEvent(NULL, MANUAL_RESET, INITIAL_STATE_IS_FALSE, NULL);
                                        /* event to block MTXDM connection 
                                           pooling thread from delisting from 
                                           MS DTC before DTC proxy thread has
                                           a chance to commit or rollback the 
                                           transaction */
        hr=pIDtcToXaHelper->EnlistWithRM(
              pdbc->dwDTCRMCookie,
              pITransaction,
              pITransactionResourceAsync,
              &pRmAsyncObj->m_pITransactionEnlistmentAsync);
        /* DTC should pITransactionResourceAsync->Release() when DTC client is done.
           This will decrement pRmAsyncObj->m_cRef to 0 and delete CRmAsyncObj */

        if (FAILED(hr))
             ProcessDTCerr(hr,psqlca,"Failed to EnlistWithRM during EnlistInDTC");
        else if (!SetSessionXID(pdbc, psess, psqlca, &xid))
             ProcessDTCerr(hr=E_FAIL,psqlca,
                                     "Failed to Set Session XID during EnlistInDTC");

        if (FAILED(hr))   /* EnlistWithRM or SET SESSION WITH XID= failed, cleanup */
           { SafeRelease(pITransactionResourceAsync);/* release our use of the interface */
// not true!!-->   /* this will trigger destruct of RmAsyncObj and cleanup of
//                    pdbc->dwDTCRMCookie and psess->XAtranHandle  */
             if (pdbc->dwDTCRMCookie)
                { pIDtcToXaHelper->ReleaseRMCookie(pdbc->dwDTCRMCookie, TRUE);
                                  /* normal close */
                  pdbc->dwDTCRMCookie=0;
                }
             odbc_rollbackTran(&psess->tranHandle, psqlca); 
                                       /* rollback any active transaction */
             if (psess->XAtranHandle)  /* release the Ingres XA trans id */
                { odbc_releaseXID(&psess->XAtranHandle, psqlca);
                  psess->XAtranHandle=
                  psess->  tranHandle=NULL;
                }
             if (pdbc->hMSDTCTransactionCompleting)  /* if handle was allocated, free it  */
                { CloseHandle(pdbc->hMSDTCTransactionCompleting); /* release the handle and   */
                  pdbc->hMSDTCTransactionCompleting=NULL;         /* destroy the event object */
                }
             break; /* break out and return SQL_ERROR */
           }  /* end if FAILED(EnlistWithRM) */
        /* SUCCESS! We're enlisted */
        pRmAsyncObj->SetState (TX_ENLISTED);
        SafeRelease(pITransactionResourceAsync);/* release our use of the interface */
                                                /* since DTC should have incr the ref cnt */

       }  /* end if (pITransaction) */



    /**
     **     Delistment from MSDTC
     **/
    else /* pITransaction==NULL (application is delisting from DTC) */
       {  /* Don't ReleaseRMCookie(pdbc->dwDTCRMCookie) and 
             odbc_releaseXID(&psess->XAtranHandle yet since an
             asynchronous pITransactionResourceAsync is still using them */
#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
    MessageBox(NULL, "Application is DELISTING from DTC", "DelistFromDTC", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

       }  /* end else pITransaction==NULL */

    rc=SQL_SUCCESS;  /* success! everything worked! */
    break;

  } /* end forever loop */

  SafeRelease(pIDtcToXaHelper); /* release the XA Helper interface */

  return rc;
}

/*
** Name: odbc_registerXID  
**
** Description:
**      Reserve a unique XID for a two phase commit transaction.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
II_BOOL odbc_registerXID(II_PTR *tranHandle, XID * pxid, LPSQLCA psqlca)
{
    IIAPI_REGXIDPARM  regxidParm={0};
    
    /*
    **  input parameters for IIapi_registerXID().
    */
    regxidParm.rg_tranID.ti_type = IIAPI_TI_XAXID;
    regxidParm.rg_tranID.ti_value.xaXID.xa_branchSeqnum = BRANCHSEQNUM;

    /* set on all of the branch flags since a xa_rollback
	   by the Transaction Manager will call xa_switch.xa_rollback
	   or xa_commit.  These functions in front\embed\libqxa\iixamain.sc
	   will always turn these flags.  Since the backend server includes
	   the branchFlag in the comparision (via common\hdr\hdr\
	   DB_DIS_TRAN_ID_EQL_MACRO) of whether or not the
	   incoming xid (to rollback/commit) is known, we need to
	   set the same branch flags as xa_prepare/xa_rollback/xa_commit
	   in libqxa.
	*/
    regxidParm.rg_tranID.ti_value.xaXID.xa_branchFlag   = BRANCHFLAG;
                                                       /* branchFlag = 7 */

    memcpy(&regxidParm.rg_tranID.ti_value.xaXID.xa_tranID, pxid, sizeof(XID));
    
    IIapi_registerXID( &regxidParm );

    if (regxidParm.rg_status != IIAPI_ST_SUCCESS)
	   {char * msg = "Failed on Ingres Register XID during EnlistInDTC";
        if      (regxidParm.rg_status == IIAPI_ST_FAILURE)
           ProcessDTCerr(E_FAIL,psqlca,msg);
        else if (regxidParm.rg_status == IIAPI_ST_OUT_OF_MEMORY)
           ProcessDTCerr(E_OUTOFMEMORY,psqlca,msg);
        else
           ProcessDTCerr(E_UNEXPECTED,psqlca,msg);
        return(FALSE);
       }

    *tranHandle = regxidParm.rg_tranIdHandle;

    return(TRUE);
}

/*
** Name: odbc_releaseXID  
**
** Description:
**      Release the unique XID for a two phase commit transaction.
**      that was reserved by IIapi_registerXID().  It also frees
**      the resources associated with the transaction ID handle.
**
** On entry: 
**
** On exit:
**
** Returns: 
**      
*/
II_BOOL odbc_releaseXID(II_PTR *tranHandle, LPSQLCA psqlca)
{
    IIAPI_RELXIDPARM  relxidParm={0};

    if (*tranHandle==NULL)   /* just return if no handle */
       return(TRUE);
    
    /*
    **  input parameters for IIapi_releaseXID().
    */
    relxidParm.rl_tranIdHandle = *tranHandle;
    IIapi_releaseXID( &relxidParm );

    if (relxidParm.rl_status != IIAPI_ST_SUCCESS)
	   {char * msg = "Failed on Ingres Release XID during EnlistInDTC";
        if      (relxidParm.rl_status == IIAPI_ST_FAILURE)
           ProcessDTCerr(E_FAIL,      psqlca,msg);
        else if (relxidParm.rl_status == IIAPI_ST_NOT_INITIALIZED)
           ProcessDTCerr(E_INVALIDARG,psqlca,msg);
        else if (relxidParm.rl_status == IIAPI_ST_INVALID_HANDLE)
           ProcessDTCerr(E_POINTER,   psqlca,msg);
        else
           ProcessDTCerr(E_UNEXPECTED,psqlca,msg);
        return(FALSE);
       }

    *tranHandle = NULL;

    return(TRUE);
}

/*
** Name: ProcessDTCerr  
**
** Description:
**      Decode the error return code into a message and
**      tack onto error message list. 
**
*/
extern "C"
void ProcessDTCerr(HRESULT hr, LPSQLCA psqlca, char * t)
{
 char    workarea[256];
 char *  s;

 switch (hr)
 {
   case E_FAIL:
      s="rc=E_FAIL (General failure; Check the Windows Event Viewer for more information)";
      break;
   case E_INVALIDARG:
      s="rc=E_INVALIDARG (One or more of the parameters are NULL or invalid)";
      break;
   case E_NOINTERFACE:
      s="rc=E_NOINTERFACE (Unable to provide the requested interface)";
      break;
   case E_OUTOFMEMORY:
      s="rc=E_OUTOFMEMORY (Unable to allocate resources for the object)";
      break;
   case E_UNEXPECTED:
      s="rc=E_UNEXPECTED (An unexpected error occurred)";
      break;
   case E_POINTER:
      s="rc=E_POINTER (Invalid handle)";
      break;
   case XACT_E_CONNECTION_DOWN:
      s="rc=XACT_E_CONNECTION_DOWN (Lost connection with DTC Transaction Manager)";
      break;
   case XACT_E_TMNOTAVAILABLE:
      s="rc=XACT_E_TMNOTAVAILABLE (Unable to establish a connection with DTC (MS DTC service probably not started))";
      break;
   case XACT_E_NOTRANSACTION:
      s="rc=XACT_E_NOTRANSACTION (No transaction corresponding to ITRANSACTION pointer)";
      break;
   case XACT_E_CONNECTION_DENIED:
      s="rc=XACT_E_CONNECTION_DENIED (Transaction manager refused to accept a connection)";
      break;
   case XACT_E_LOGFULL:
      s="rc=XACT_E_LOGFULL (Transaction Manager has run out of log space)";
      break;
   case XACT_E_XA_TX_DISABLED:
      s="rc=XACT_E_XA_TX_DISABLED (Transaction Manager has disabled its support "
        "for XA transactions.  See MS KB article 817066 to enable XA and register "
        "\"caiiod35.dll\".)";
      break;
   default:
      s=workarea;
      wsprintf(s,"0x%8.8lX",(long)hr);
 }

 odbc_write_erlog(t);    /* write function's message to errlog */
 if (hr != S_OK)
    odbc_write_erlog(s); /* write return code message to errlog */

 if (psqlca==NULL)
    return;

 psqlca->SQLCODE = SQL_ERROR;
 psqlca->SQLERC = hr;
 strncpy((char*)(psqlca->SQLSTATE),"HY000",sizeof(psqlca->SQLSTATE));
 InsertSqlcaMsg(psqlca,t,"HY000",TRUE);  /* function's message */
 InsertSqlcaMsg(psqlca,s,"HY000",TRUE);  /* return code message */

 return;
}


/*
** Name: odbc_write_erlog  
**
** Description:
**      Write the text string to the errlog.
**
*/
static void odbc_write_erlog(char * text)
{
    char buf[257]="";
    char message[513]="        ::[                , 00000000]: ";
    CL_ERR_DESC err_code;
    STATUS erstatus;
    SYSTIME time_now;

    STprintf(message, ERx("        ::[%-16.16s, %08.8x]: "), 
            "ODBC Driver", (i4)0);
    TMnow( &time_now );
    TMstr( &time_now, buf );
    STcat(message, buf);
    STcat(message, ERx(" "));
    STcat(message, text);

    erstatus = ERlog(message, (i4)STlength(message), &err_code);
}

/*************************************************************
** Name: CRmAsyncObj  
**
** Description:
**      ITransactionResourceAsync interface implementation. 
**
**************************************************************/

/*
**
**  Constructor
**
*/
CRmAsyncObj::CRmAsyncObj (LPDBC pdbc, IDtcToXaHelperSinglePipe  * pIDtcToXaHelper) : m_cRef(0)
{
 m_pITransactionEnlistmentAsync=NULL;
 m_pdbc=pdbc;
 m_pIDtcToXaHelper=pIDtcToXaHelper;
 m_pIDtcToXaHelper->AddRef();
 InitializeCriticalSection(&csTxResourceAsync);
 hCommitEvent         =NULL;
 hTxResourceAsyncEvent=NULL;
 SetState (TX_INITIALIZED);
} /* end CRmAsyncObj::CRmAsyncObj () */


/*
**
**  Destructor
**
*/
CRmAsyncObj::~CRmAsyncObj ()
{
  LPDBC              pdbc   = m_pdbc;
#ifdef ENABLE_ASSERT
	_ASSERTE(pdbc);
#endif
  SESS *      psess  = &(pdbc->sessMain);
  SQLCA*      psqlca = &(pdbc->sqlca);

  SetState (TX_UNINITIALIZED);

  if (pdbc->dwDTCRMCookie)  /* release the DTC/RM cookie if needed */
     { DWORD dwDTCRMCookie = pdbc->dwDTCRMCookie;
       pdbc->dwDTCRMCookie=0;  /* clear out first just in case of recursion */
       m_pIDtcToXaHelper->ReleaseRMCookie(dwDTCRMCookie, TRUE);
                                /* normal close */
     }

  SafeRelease(m_pIDtcToXaHelper);

  if (psess->XAtranHandle)  /* release the Ingres XA trans id */
     { odbc_releaseXID(&psess->XAtranHandle, psqlca);
       psess->XAtranHandle=
       psess->  tranHandle=NULL;
     }

  DeleteCriticalSection(&csTxResourceAsync);
} /* end CRmAsyncObj::~CRmAsyncObj () */

/*
**
**  CreateInstance
**
*/
IUnknown* CRmAsyncObj::CreateInstance()
{
    IUnknown* pI = static_cast<IUnknown*>(this);
    pI->AddRef(); /* incr reference count */
    return pI;    /* return the interface pointer */
}

/*
**
**  EnlistTx
**
*/
HRESULT CRmAsyncObj::EnlistTx()
{
	
	// Create a transaction resource async object for handling 2PC
	// requests.

	ITransactionResourceAsync * ptxRmAsync = NULL;
//	GetUnknown() -> QueryInterface(IID_ITransactionResourceAsync, (void**)&ptxRmAsync);	
                                         // does an AddRef that DTC should Release()
#ifdef ENABLE_ASSERT
	_ASSERTE (0 != ptxRmAsync);
#endif

	return S_OK;
	
} // CRmAsyncObj::EnlistTx

/*
**
**  QueryInterface
**
*/
HRESULT STDMETHODCALLTYPE CRmAsyncObj::QueryInterface(REFIID iid, void** ppv)
{
    if (iid == IID_ITransactionResourceAsync  ||  iid == IID_IUnknown)
          *ppv = static_cast<ITransactionResourceAsync*>(this);
    else
        { *ppv = NULL;
          return E_NOINTERFACE;
        }

    reinterpret_cast<IUnknown*>(*ppv)->AddRef();  /* Incr reference count */
    return S_OK;  /* return the interface pointer */
}

/*
**
**  AddRef
**
*/
ULONG STDMETHODCALLTYPE CRmAsyncObj::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

/*
**
**  Release
**
*/
ULONG STDMETHODCALLTYPE CRmAsyncObj::Release()
{
    if (InterlockedDecrement(&m_cRef) == 0)
       {
         delete this;
         return 0;
       }
    return m_cRef;
}

/*
**
**  SetState
**
*/
TXSTATE CRmAsyncObj::SetState(TXSTATE eState)
{
    TXSTATE old_eState=m_eTxState;

    /* do not overwrite any state with TX_INITIALIZING, except for TX_UNINITIALIZED */
    if (eState == TX_INITIALIZING  &&  old_eState!= TX_UNINITIALIZED)
       return old_eState;

	m_eTxState = eState;
	return old_eState;    /* return the old state */
} // end CRmAsyncObj:SetState


/*
**
**  GetState
**
*/
TXSTATE CRmAsyncObj::GetState()
{
	return m_eTxState;
} // end CRmAsyncObj::GetState()


/*
**
**  GetStateString
**
*/
char * CRmAsyncObj::GetStateString(TXSTATE eState)
{
	switch (eState)
	{
	   case TX_UNINITIALIZED: return("TX_UNINITIALIZED");
	   case TX_INITIALIZING:  return("TX_INITIALIZING");
	   case TX_INITIALIZED:   return("TX_INITIALIZED");
	   case TX_ENLISTING:     return("TX_ENLISTING");
	   case TX_ENLISTED:      return("TX_ENLISTED");
	   case TX_PREPARING:     return("TX_PREPARING");
	   case TX_PREPARING_AND_COMMITTING_TOGETHER: 
	                          return("TX_PREPARING_AND_COMMITTING_TOGETHER");
	   case TX_PREPARING_SINGLE_PHASE: 
	                          return("TX_PREPARING_SINGLE_PHASE");
	   case TX_PREPARED:      return("TX_PREPARED");
	   case TX_COMMITTING:    return("TX_COMMITTING");
	   case TX_COMMITTED:     return("TX_COMMITTED");
	   case TX_ABORTING:      return("TX_ABORTING");
	   case TX_ABORTED:       return("TX_ABORTED");
	   case TX_DONE:          return("TX_DONE");
	   case TX_TMDOWNING:     return("TX_TMDOWNING");
	   case TX_TMDOWN:        return("TX_TMDOWN");
	   case TX_INVALID_STATE: return("TX_INVALID_STATE");
	}
	return ("<undefined>");
} // end CRmAsyncObj::GetStateString()


/*
**
**  PrepareRequest
**
**  The MS DTC Proxy (MSDTCPRX.DLL) calls this method to prepare, 
**  phase one of the two-phase commit protocol, a transaction.
**  The resource manager needs to return from this call as
**  soon as the transaction object starts preparing.
**  Once the transaction object is prepared the resource
**  manager needs to call ITransactionEnlistmentAsync::PrepareRequestDone.
**
**  If fSinglePhase flag is TRUE, it indicates that the RM is the
**  only resource manager enlisted on the transaction, and therefore
**  it has the option to perform the single phase optimization.  
**  If the RM does choose to perform the single phase optimization,
**  then it lets the Transaction Coordinator know of this 
**  optimization by providing XACT_S_SINGLEPHASE flag to the
**  ITransactionEnlistmentAsync::PrepareRequestDone.
**
*/
STDMETHODIMP CRmAsyncObj::PrepareRequest(BOOL  fRetaining,
                                         DWORD grfRM,
                                         BOOL  fWantMoniker,
                                         BOOL  fSinglePhase)
{
	HRESULT   hr = S_OK;
    LPDBC     pdbc   = m_pdbc;
    TXSTATE  txstate = GetState(); 

#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
    MessageBox(NULL, "CRmAsyncObj::PrepareRequest", "Debug CRmAsyncObj", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

	if (txstate != TX_ENLISTED)
       {char s[256] = "ODBC Driver DTC callback received unexpected PrepareRequest when in state ";
		STcat(s,GetStateString(txstate));

		SetState (TX_INVALID_STATE);  /* transaction is invalid */

		ProcessDTCerr(E_UNEXPECTED,NULL, s);
		/* Transaction is in an invalid state -- return error */
		hr = m_pITransactionEnlistmentAsync->PrepareRequestDone (E_UNEXPECTED,NULL,NULL);
		if (hr!=S_OK)
			return E_FAIL;
		return E_UNEXPECTED;
       }

	if (FAILED(StartTxResourceAsyncThread(fSinglePhase?TX_PREPARING_SINGLE_PHASE:
                                                       TX_PREPARING)))
		hr = m_pITransactionEnlistmentAsync->PrepareRequestDone (E_FAIL,NULL,NULL);

    return S_OK;

} // end CRmAsyncObj::PrepareRequest()


/*
**
**  CommitRequest
**
**  The MS DTC Proxy (MSDTCPRX.DLL) calls this method to commit, 
**  phase two of the two-phase commit protocol, a transaction.
**  The resource manager needs to return from this call as
**  soon as the transaction object starts committing.
**  Once the transaction object is prepared the resource
**  manager needs to call ITransactionEnlistmentAsync::CommitRequestDone.
**
*/
STDMETHODIMP CRmAsyncObj::CommitRequest(DWORD grfRM, XACTUOW *pNewUOW)
{   HRESULT  hr    = S_OK;
    TXSTATE  txstate = GetState(); 
	
#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
    MessageBox(NULL, "CRmAsyncObj::CommitRequest", "Debug CRmAsyncObj", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

    if (txstate != TX_PREPARED)
	   {char s[256] = "ODBC Driver DTC callback received unexpected CommitRequest when in state ";
		STcat(s,GetStateString(txstate));

		SetState (TX_INVALID_STATE);  /* transaction state is invalid */

		ProcessDTCerr(E_UNEXPECTED,NULL, s);
		hr = m_pITransactionEnlistmentAsync->CommitRequestDone (E_UNEXPECTED);
		if (hr!=S_OK)
			return E_FAIL;
		return E_UNEXPECTED;
	   }
    
	if (FAILED(StartTxResourceAsyncThread(TX_COMMITTING)))
		hr = m_pITransactionEnlistmentAsync->CommitRequestDone (E_FAIL);
	
	return S_OK;
} // end CRmAsyncObj::CommitRequest()


/*
**
**  AbortRequest
**
**  The MS DTC Proxy (MSDTCPRX.DLL) calls this method to abort a transaction.
**  The resource manager needs to return from this call as
**  soon as the transaction object starts aborting.
**  Once the transaction object is aborted the resource
**  manager needs to call ITransactionEnlistmentAsync::AbortRequestDone.
**
*/
STDMETHODIMP CRmAsyncObj::AbortRequest(BOID    *pboidReason, 
                                       BOOL     fRetaining,
                                       XACTUOW *pNewUOW)
{
	HRESULT     hr   = S_OK;
    LPDBC              pdbc   = m_pdbc;
#ifdef ENABLE_ASSERT
	_ASSERTE(pdbc);
#endif
    SESS *      psess  = &(pdbc->sessMain);
    SQLCA*      psqlca = &(pdbc->sqlca);

#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
    MessageBox(NULL, "CRmAsyncObj::AbortRequest", "Debug CRmAsyncObj", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

	TXSTATE     eTxState = GetState();
    if (eTxState != TX_PREPARED &&
		eTxState != TX_ENLISTED)
	{
		hr = E_UNEXPECTED;    /* we will ignore this anyway */
	} // if

	if (FAILED(StartTxResourceAsyncThread(TX_ABORTING)))
		hr = m_pITransactionEnlistmentAsync->AbortRequestDone (E_FAIL);
	
	return  S_OK;

} // end CRmAsyncObj::AbortRequest()

//
//	The MS DTC proxy (MSDTCPRX.DLL) calls this method if connection to the transaction 
//	manager down and the resource manager's transaction object is prepared
//	(after the resource manager has called the ::PrepareRequestDone method).
//	in this method
//  
STDMETHODIMP CRmAsyncObj::TMDown(void)
{
	SetState (TX_TMDOWN);
#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
    MessageBox(NULL, "CRmAsyncObj::TMDown", "Debug CRmAsyncObj", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

	StartTxResourceAsyncThread(TX_TMDOWNING);
	
	return S_OK;
} // end CRmWorkerObj::TMDown




/*
** Name: StartTxResourceAsyncThread - Start or continue thread that 
**                        will process the PrepareRequest, CommitRequest, etc.
**
** Return value:
**      S_OK
**
*/
    
HRESULT CRmAsyncObj::StartTxResourceAsyncThread(TXSTATE eState)
{
    SIZE_TYPE rc=-1;
    HRESULT       hr=S_OK;

#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
    char debugstring[256] = "CRmAsyncObj::StartTxResourceAsyncThread(";
    strncat(debugstring, GetStateString(eState), 30);
    strncat(debugstring, ")", 1);
    MessageBox(NULL, debugstring, "Debug CRmAsyncObj", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

    EnterCriticalSection(&csTxResourceAsync);  /* single thread dispatch state */

    SetState(eState);  /* set state (TX_PREPARING, TX_COMMITTING, etc. */

    if (!hTxResourceAsyncEvent)   /* if no thread yet then begin one */
       { AddRef();               /* keep CRmAsyncObj until thread is done with it */
         hTxResourceAsyncEvent=CreateEvent(NULL, MANUAL_RESET, INITIAL_STATE_IS_FALSE, NULL);
         hCommitEvent         =CreateEvent(NULL, MANUAL_RESET, INITIAL_STATE_IS_FALSE, NULL);
         if (!hTxResourceAsyncEvent || !hCommitEvent)
             ProcessDTCerr(E_FAIL,NULL,
                "ODBC Driver DTC callback StartTxResourceAsyncThread failed on CreateEvent");
         if (hCommitEvent)
            {rc=_beginthread(TxResourceAsyncThread, 0, (void*)this);
             if (rc==-1)
                ProcessDTCerr(E_FAIL,NULL,
                   "ODBC Driver DTC callback StartTxResourceAsyncThread failed on beginthread");
            }
         if (rc==-1)
            {  /* error return */
             SetState (TX_INVALID_STATE);
             if (hCommitEvent)
                 CloseHandle(hCommitEvent);   /* release the handle and destroy event object */
             Release();
             hr=E_FAIL;
             /* DTC will pITransactionResourceAsync->Release() and
                will decrement pRmAsyncObj->m_cRef to 0 and delete CRmAsyncObj */
            }
         else  /* thread created OK */
            {
             WaitForSingleObject(hCommitEvent, INFINITE); 
                       /* wait for TxResourceAsync to start and give it
                          a chance to lock DBC connection before TM might return
                          async control back to application which might try to
                          fire off another ODBC call before COMMIT has a chance
                          to finish. */
             CloseHandle(hCommitEvent);   /* release the handle and destroy event object */
            }
         hCommitEvent = NULL;             /* clear out the destroyed event's handle */
       } /* end if (hTxResourceAsyncEvent) */


    else  /* wake up sleeping TxResourceAsync thread */
       {
             /* wake up thread already started, except if TX_INITIALIZING (not needed) */
             if (eState != TX_INITIALIZING)
                 SetEvent(hTxResourceAsyncEvent); 
       }
    LeaveCriticalSection(&csTxResourceAsync);

    return hr;

}


/*
** Name: TxResourceAsyncThread - Thread to execute under for the PrepareRequest,
**                                 CommitRequest, AbortRequest, TMDown.
**
**       Note:  All are processed under the same thread since we need to lock 
**              the DBC and the same thread is needed for the EnterCriticalSection.
**              We need to lock the DBC because MS DTC will return S_OK
**              to the application after the pITransaction->Commit(0,0,0)
**              after we call ->PrepareRequestDone(S_OK)!  MS assumes that
**              if the prepare-to-commit is successful then the commit is
**              automatically going to be successful!  Returning to the
**              application will allow the application to try to issue queries
**              or even a SQLDisconnect between the prepare-to-commit and
**              the commit.
*/
    
void __cdecl TxResourceAsyncThread(void * thisparm)
{
    CRmAsyncObj * pCRmAsyncObj = static_cast<CRmAsyncObj*>(thisparm);

    pCRmAsyncObj->TxResourceAsync();
}

/*
** Name: TxResourceAsync - Process ITransactionResourceAsync request
**
** Notes: Acquires and locks the DBC.  This thread dies after processing
**        the request except for a prepare-to-commit.  In that case,
**        we wait, holding the DBC lock, until the commit completes.
**        Holding the DBC lock will block any async ODBC queries
**        between the prepare-to-commit and the commit.
**
** Return value:
**      S_OK
**
*/
    
void CRmAsyncObj::TxResourceAsync(void)
{
    LPDBC       pdbc   = m_pdbc;
    SESS *      psess  = &(pdbc->sessMain);
    SQLCA*      psqlca = &(pdbc->sqlca);
    TXSTATE     txstate;
    BOOL        bRedispatch      =FALSE;
    BOOL        bWaitingOnPrepare=FALSE;
    BOOL        bWaitingOnMSDTCstarting=FALSE;
    RETCODE     rc;
    HRESULT     hr;

#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
    MessageBox(NULL, "CRmAsyncObj::TxResourceAsync", "Debug CRmAsyncObj", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

    LockDbc_Trusted(pdbc);      /* lock the DBC so no asynchronous queries can sneak in */

    if (pdbc->hMSDTCTransactionCompleting)
        SetEvent(pdbc->hMSDTCTransactionCompleting); 
                                /* now that DBC is safely locked, allow any MTXDM  
                                   connection pooling thread to start delisting from 
                                   MS DTC since DTC proxy thread now has
                                   a chance to commit or rollback the transaction 
                                   while holding the DBC lock */
    txstate = GetState();

    if (hCommitEvent)
       SetEvent(hCommitEvent);  /* wake up StartTxResourceAsyncThread now that
                                   DBC is safely locked from re-entrancy 
                                   until we finish the commit */
 TxResourceAsync_Redispatch:

    bRedispatch=FALSE;
    bWaitingOnMSDTCstarting=FALSE;
    ResetEvent(hTxResourceAsyncEvent);

    switch(txstate)
    {
      case TX_INITIALIZING:   /* initiated by our Delistment to lock up the DBC */
       {
         bWaitingOnMSDTCstarting=TRUE;  /* wait a little bit for the first call
                                          (Prepare-to-commit, Abort, etc.) from MSDTC
                                           while holding the DBC lock safely */
         break;
       }

      case TX_PREPARING:  /* initiated by ITransactionResourceAsync::PrepareRequest */
       {
         rc=SQLTransact_InternalCall(NULL, m_pdbc, 9999); /* close all queries first */
         hr=PrepareTx();                     /* call Ingres API to prepare-to-commit */
         if (hr==XACT_S_READONLY)            /* no transaction active but it's OK */
            { SetState (TX_ENLISTED);
              bWaitingOnPrepare=FALSE;
            }
         else if (SUCCEEDED(hr))             /* transaction is prepared */
            { SetState (TX_PREPARED);
              bWaitingOnPrepare=TRUE;        /*   wait for the commit to come in */
            }
         else 
            { RollbackTx();                  /* prepare failed; abort the transaction */
              SetState (TX_INVALID_STATE);
              bWaitingOnPrepare=FALSE;
            }
         m_pITransactionEnlistmentAsync->PrepareRequestDone(hr, NULL, NULL);
         /* if hr==S_OK then MS DTC will dispatch the user application
                             after the pITransaction->commit();
          else hr==E_FAIL then DTC will pITransactionResourceAsync->Release() and
                        will decrement pRmAsyncObj->m_cRef to 0 and delete CRmAsyncObj */
         break;
       }

      case TX_PREPARING_SINGLE_PHASE:  /* initiated by ITransactionResourceAsync::PrepareRequest */
       {
         rc=SQLTransact_InternalCall(NULL, m_pdbc, 9999); /* close all queries first */
         hr=CommitTx();                      /* single phase commit optimization */
         if (SUCCEEDED(hr))
            { hr=XACT_S_SINGLEPHASE;
              SetState (TX_ENLISTED);
            }
         else SetState (TX_INVALID_STATE);
         bWaitingOnPrepare=FALSE;
         m_pITransactionEnlistmentAsync->PrepareRequestDone(hr, NULL, NULL);
         break;
       }

      case TX_COMMITTING: /* initiated by ITransactionResourceAsync::CommitRequest */
       {
         hr=CommitTx();
         if (SUCCEEDED(hr))
              SetState (TX_ENLISTED);
         else SetState (TX_INVALID_STATE);
         bWaitingOnPrepare=FALSE;
         m_pITransactionEnlistmentAsync->CommitRequestDone(hr);
         break;
       }

      case TX_TMDOWNING:  /* initiated by ITransactionResourceAsync::TMDown */
      case TX_ABORTING:   /* initiated by ITransactionResourceAsync::AbortRequest */
       {
         rc=SQLTransact_InternalCall(NULL, m_pdbc, 9999); /* close all queries first */
         hr=RollbackTx();
         bWaitingOnPrepare=FALSE;
         if (txstate==TX_ABORTING)
            { SetState (TX_ABORTED);
	          hr = m_pITransactionEnlistmentAsync->AbortRequestDone (S_OK);
            }
         else /* txstate==TX_TMDOWNING */
              SetState (TX_TMDOWN);
         break;
       }
    } /* end switch(txstate) */

                                                /* decide whether thread waits or dies */
    EnterCriticalSection(&csTxResourceAsync);
    txstate = GetState();                       /* get the current state since if may have been */
    if (txstate==TX_PREPARING               ||  /* changed by the DTC Proxy thread */
        txstate==TX_PREPARING_SINGLE_PHASE  ||
        txstate==TX_COMMITTING              ||
        txstate==TX_TMDOWNING               ||
        txstate==TX_ABORTING)
       { bRedispatch=TRUE;
       }

    if (txstate == TX_INITIALIZING)             /* do the initial wait for MSDTC start-up just once */
        SetState(TX_INITIALIZED);

    if (bRedispatch            ==FALSE  &&
        bWaitingOnPrepare      ==FALSE  &&
        bWaitingOnMSDTCstarting==FALSE)
        { CloseHandle(hTxResourceAsyncEvent);   /* release the handle and destroy event object */
          hTxResourceAsyncEvent=NULL;           /* signal to StartTxResourceAsyncThread that 
                                                   there is no TxResourceAsync thread */
        }
    LeaveCriticalSection(&csTxResourceAsync);

    if (bRedispatch)                            /* if state changed, go process the new state */
         goto TxResourceAsync_Redispatch; 

    if (bWaitingOnPrepare)  /* if prepare was done, keep DBC locked until CommitRequest comes */
       { WaitForSingleObject(hTxResourceAsyncEvent, INFINITE);  /* keep thread alive and wait */ 
         goto TxResourceAsync_Redispatch; 
       }

    if (bWaitingOnMSDTCstarting)  /* wait for MSDTC to start up while we hold onto the DBC lock */
       {
         WaitForSingleObject(hTxResourceAsyncEvent, 60*1000);  /* keep thread alive and wait */
         goto TxResourceAsync_Redispatch; 
       }



    Release();            /* release our use of CRmAsyncObj */

    UnlockDbc(pdbc);      /* unlock the DBC */
    return;
}


/*
** Name: PrepareTx - Prepare-to-commit
**
** Return value:
**      S_OK
**      E_FAIL
*/
    
HRESULT CRmAsyncObj::PrepareTx(void)
{
    SESS *      psess        = &m_pdbc->sessMain;
    SQLCA*      psqlca       = &(m_pdbc->sqlca);
    LPSQLCAMSG  pm;

    IIAPI_PREPCMTPARM *precommitParm;
    IIAPI_WAITPARM waitParm = { -1 };
    HRESULT        hr=S_OK;

    if (psess->tranHandle==NULL)   /* if no transaction open yet,  */
       return(XACT_S_READONLY);    /* just return success that tx is read-only */

    /*
    ** Allocate buffer and set parameters.
    */
    precommitParm =
       ( IIAPI_PREPCMTPARM * )odbc_malloc( sizeof( IIAPI_PREPCMTPARM ) );
    
    precommitParm->pr_genParm.gp_callback = NULL;
    precommitParm->pr_genParm.gp_closure  = NULL;
    precommitParm->pr_tranHandle          = psess->tranHandle;
    IIapi_prepareCommit( precommitParm );  /* start the asynchronous prepare-to-commit */

    if (!odbc_getResult( &precommitParm->pr_genParm, psqlca, 0))  /* wait and get result */
       {
          hr=E_FAIL;
          ProcessDTCerr(hr,NULL,"ODBC Driver DTC callback - PrepareTx failed");
          for (pm=psqlca->SQLPTR; pm; pm=pm->SQLPTR)  /* dump the error msgs */
              ProcessDTCerr(S_OK,NULL,&pm->SQLERM[1]);
       }

    odbc_free( (II_PTR)precommitParm );  /* can now free parm block */
    
    return hr;
}


/*
** Name: CommitTx - Commit
**
** Return value:
**      S_OK
**      E_FAIL
*/
    
HRESULT CRmAsyncObj::CommitTx(void)
{
    SESS *      psess        = &m_pdbc->sessMain;
    SQLCA*      psqlca       = &(m_pdbc->sqlca);
    LPSQLCAMSG  pm;

#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
    MessageBox(NULL, "CRmAsyncObj::CommitTx", "Debug CRmAsyncObj", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

    if (!odbc_commitTran(&psess->tranHandle, psqlca))
       {
        ProcessDTCerr(E_FAIL,NULL,"ODBC Driver DTC callback - CommitTx failed");
          for (pm=psqlca->SQLPTR; pm; pm=pm->SQLPTR)  /* dump the error msgs */
              ProcessDTCerr(S_OK,NULL,&pm->SQLERM[1]);
#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
    MessageBox(NULL, "CRmAsyncObj::CommitTx Failed", "Debug CRmAsyncObj", MB_ICONSTOP|MB_OK);
#endif /*  DTC_DEBUG_MESSAGE_BOX_ENABLED */

        return(E_FAIL);
       }
    psess->fStatus &= ~SESS_COMMIT_NEEDED;

    return(S_OK);
}

/*
** Name: RollbackTx - Start async transaction rollback
**
** Return value:
**      S_OK
**
*/
    
HRESULT CRmAsyncObj::RollbackTx(void)
{
    SESS *  psess        = &m_pdbc->sessMain;
    SQLCA*  psqlca       = &(m_pdbc->sqlca);
    II_BOOL flag = (!psess->tranHandle)?TRUE:FALSE;
    II_BOOL rc;
    char    buf[300]="RollbackTx:  ";

    rc=odbc_rollbackTran(&psess->tranHandle, psqlca);
                    /* rollback transaction and set psess->tranHandle=NULL */

    if (flag)
       strcat(buf, "tranHandle==NULL; ");
    if (!rc)
       strcat(buf, "rc=FAIL; ");

#ifdef DTC_DEBUG_MESSAGE_BOX_ENABLED
      if (flag || !rc)
         MessageBox(NULL, buf, "Debug CRmAsyncObj", MB_ICONSTOP|MB_OK);
#endif
	
    psess->fStatus &= ~SESS_COMMIT_NEEDED;
	//
	//	indicate we are done with the transaction
	//
	return  S_OK;

} // end CRmAsyncObj::RollbackTx()

/*
** Name: FormatOpenString - Format the caiiod35.DLL Open String
**
** Return value:
**      -> the Open String
**
*/

void   FormatOpenStringText(char *s, char *key, CHAR *value)
{
    if (*value)
       { strcat(s,key);
         strcat(s,(char*)value);
         strcat(s,", ");
       }

}

char * FormatOpenString(LPDBC pdbc, char * s)
{
    size_t  len;
    char *t;

    *s='\0';                  /* output string is empty right now */

// MS DTC runs under a System Account so user does not see ODBC User DSNs.
// Therefore, always use vnode::database format for target.
//    if (*pdbc->szDSN  &&  strcmp((char*)pdbc->szDSN,"DriverConnectTemp")!=0) /* if   DSN=    */
//       FormatOpenStringText(s,"DSN=",       pdbc->szDSN);
//    else                                                              /* else DRIVER= */ 
       {
        FormatOpenStringText(s,"SERVER=",    pdbc->szVNODE);
        FormatOpenStringText(s,"DATABASE=",  pdbc->szDATABASE);
// Don't pass SERVERTYPE since pre 2.5? version of LIBQXA don't recongize it.
//      FormatOpenStringText(s,"SERVERTYPE=",pdbc->szSERVERTYPE);
       }
    FormatOpenStringText(s,"ROLE=",      pdbc->szRoleName);
    if (pdbc->cbRolePWD > 0)
    {
        pwcrypt((LPCSTR)pdbc->szRoleName, 
                 (LPSTR)pdbc->szRolePWD); /* decrpts the pw */  
        FormatOpenStringText(s,"ROLEPWD=",   pdbc->szRolePWD);
        pwcrypt((LPCSTR)pdbc->szRoleName, 
                 (LPSTR)pdbc->szRolePWD); /* encrypts the pw */
    }
//  FormatOpenStringText(s,"UID=",       pdbc->szUID);
//  FormatOpenStringText(s,"PWD=",       pdbc->szPWD);
//  FormatOpenStringText(s,"GROUP=",     pdbc->szGroup);  /* (for the future) */

    len=strlen(s);
    if (len)  /* strip trailing ", " */
       {  t=s+len-1;
          if ((*t)==' ')  *t='\0';
          t--;
          if ((*t)==',')  *t='\0';  
       }
    return(s);
}

/*         IICXformat_xa_xid and IICXformat_xa_extd_xid are shamelessly 
           copied from front/embed/libqxa/iicxxa.sc since these routines
           are not exposed through any DLL 
*/

/*
**   Name: IICXformat_xa_xid()
**
**   Description: 
**       Formats the XA XID into the text buffer.
**
**   Inputs:
**       xa_xid_p       - pointer to the XA XID.
**       text_buffer    - pointer to a text buffer that will contain the text
**                        form of the ID.
**   Outputs:
**       Returns: the character buffer with text.
**
**   History:
**       02-Oct-1992 - First written (mani)
**       05-oct-1998 (thoda04)
**          Process XID data bytes in groups of four, high order to low, bytes for all platforms.
*/
static
VOID
IICXformat_xa_xid(DB_XA_DIS_TRAN_ID    *xa_xid_p,
              char       *text_buffer)
{
  char     *cp = text_buffer;
  i4       data_lword_count = 0;
  i4       data_byte_count  = 0;
  u_char   *tp;                  /* pointer to xid data */
  u_i4     unum;                 /* unsigned work field */
  i4       i;

#ifdef axp_osf 
  CVlx8(xa_xid_p->formatID,cp);
#else
  CVlx(xa_xid_p->formatID,cp);
#endif
  STcat(cp, ERx(":"));
  cp += STlength(cp);
 
  CVla((i4)(xa_xid_p->gtrid_length),cp); /* length 1-64 from i4 or i8 long gtrid_length */
  STcat(cp, ERx(":"));
  cp += STlength(cp);

  CVla((i4)(xa_xid_p->bqual_length),cp); /* length 1-64 from i4 or i8 long bqual_length */
  cp += STlength(cp);

  data_byte_count = (i4)(xa_xid_p->gtrid_length + xa_xid_p->bqual_length);
  data_lword_count = (data_byte_count + sizeof(u_i4) - 1) / sizeof(u_i4);
  tp = (u_char*)(xa_xid_p->data);  /* tp -> B0B1B2B3 xid binary data */

  for (i = 0; i < data_lword_count; i++)
  {
     STcat(cp, ERx(":"));
     cp++;
     unum = (u_i4)(((i4)(tp[0])<<24) |   /* watch out for byte order */
                   ((i4)(tp[1])<<16) | 
                   ((i4)(tp[2])<<8)  | 
                    (i4)(tp[3]));
     CVlx(unum, cp);     /* build string "B0B1B2B3" in byte order for all platforms*/
     cp += STlength(cp);
     tp += sizeof(u_i4);
  }
  STcat(cp, ERx(":XA"));


} /* IICXformat_xa_xid */
       



/*
**   Name: IICXformat_xa_extd_xid()
**
**   Description: 
**       Formats the branch_seqnum and branch_flag into the text buffer.
**       The additional fields are formatted after :XA as
**       :XXXX:XXXX:EX.
**
**   Inputs:
**       branch_seqnum   - Value of the branch Seq Number
**       branch_flag     - Value of Branch Flag
**       text_buffer     - pointer to a text buffer that will contain the text
**                         form of the ID.
**   Outputs:
**       Returns: the character buffer with branch_seqnum and branch_flag
**                appended to its end.
**
**   History:
**       23-Sep-1993 - First written (iyer)
*/
static
VOID
IICXformat_xa_extd_xid(i4   branch_seqnum,
                       i4   branch_flag,
                       char *text_buffer)
{
  char     *cp = text_buffer;

  STcat(cp, ERx(":"));  
  cp += STlength (cp);

  CVna(branch_seqnum,cp);
  STcat(cp, ERx(":"));  
  cp += STlength(cp);

  CVna(branch_flag,cp);
  STcat(cp, ERx(":EX"));  


} /* IICXformat_xa_extd_xid */
       

/*
** SetSessionXID
**
**    Identify ourselves to the server for distributing locking support
**    beginning with Ingres 3.0. 
*/
static II_BOOL SetSessionXID(LPDBC pdbc, SESS * pSession, 
                             LPSQLCA psqlca, XID* pxid)
{
    II_BOOL             rc = TRUE;

    IIAPI_QUERYPARM     queryParm;
    IIAPI_GETQINFOPARM  getQInfoParm;
    IIAPI_CLOSEPARM     closeParm;
    IIAPI_GETDESCRPARM  getDescrParm;
    i4                  branch_seqnum = 0;
    char                xid_str[IIXA_XID_STRING_LENGTH] =
                          "set session with xa_xid='";

    IICXformat_xa_xid((DB_XA_DIS_TRAN_ID *)pxid, xid_str + STlength(xid_str));
    IICXformat_xa_extd_xid(BRANCHSEQNUM, BRANCHFLAG, xid_str);
    STcat(xid_str, "\'");
    /* ProcessDTCerr(S_OK, NULL, xid_str);*/  /* trace the XID */

    queryParm.qy_genParm.gp_callback = NULL;
    queryParm.qy_genParm.gp_closure = NULL;
    queryParm.qy_connHandle = pSession->connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText =  xid_str; 
    queryParm.qy_parameters = FALSE;  
    queryParm.qy_tranHandle = pSession->tranHandle?pSession->tranHandle:
                                                   pSession->XAtranHandle;
    queryParm.qy_stmtHandle = (II_PTR)NULL;
    
    IIapi_query( &queryParm );
    pSession->tranHandle = queryParm.qy_tranHandle;

    if (!odbc_getResult( &queryParm.qy_genParm, psqlca, (II_LONG)pdbc->cQueryTimeout))
        return(FALSE);

    getDescrParm.gd_genParm.gp_callback = NULL;
    getDescrParm.gd_genParm.gp_closure = NULL;
    getDescrParm.gd_stmtHandle = queryParm.qy_stmtHandle;
    getDescrParm.gd_descriptorCount = 0;
    getDescrParm.gd_descriptor = NULL;
//    IIapi_getDescriptor( &getDescrParm );
    
//    if (!odbc_getResult( &getDescrParm.gd_genParm, psqlca, pdbc->cQueryTimeout))
//        return(FALSE);

    /*
    ** Provide input parameters for IIapi_getQueryInfo().
    */
    getQInfoParm.gq_genParm.gp_callback = NULL;
    getQInfoParm.gq_genParm.gp_closure = NULL;
    getQInfoParm.gq_stmtHandle = queryParm.qy_stmtHandle;

    IIapi_getQueryInfo( &getQInfoParm );

    if (!odbc_getResult( &getQInfoParm.gq_genParm, psqlca, (II_LONG)pdbc->cQueryTimeout))
        rc = FALSE;

    closeParm.cl_genParm.gp_callback = NULL;
    closeParm.cl_genParm.gp_closure = NULL;
    closeParm.cl_stmtHandle = queryParm.qy_stmtHandle;
    IIapi_close( &closeParm );
//  if (!odbc_getResult( &closeParm.cl_genParm, psqlca, pdbc->cQueryTimeout))
//      rc = FALSE;

    return(rc);
}

/*
** isIngresXAlicensed
**
**     Return TRUE if server or client has DTP XA for Tuxedo  option in license
*/
static II_BOOL isIngresXAlicensed(LPDBC pdbc, SESS * pSession, LPSQLCA psqlca)
{
    II_BOOL rc              = TRUE;

#ifdef XA_LICENSE_CHECKING_ON_SERVER_WAS_NEVER_IMPLEMENTED
    II_LONG paramID         = IIAPI_CP_APPLICATION;
    II_LONG paramValue      = DBA_XA_TUXEDO_LIC_CHK;
    IIAPI_SETCONPRMPARM scp = {0};
    IIAPI_MODCONNPARM   mcp = {0};

    scp.sc_connHandle = pSession->connHandle;
    scp.sc_paramID    = paramID;
    scp.sc_paramValue = &paramValue; 
    IIapi_setConnectParam(&scp);  /* -A DBA_XA_TUXEDO_LIC_CHK */
         
    if (!odbc_getResult(&scp.sc_genParm, psqlca, 0))
         return(FALSE);

    mcp.mc_connHandle = pSession->connHandle;
    IIapi_modifyConnect(&mcp);

    if (odbc_getResult(&mcp.mc_genParm, psqlca, 0))
         return(TRUE);            /* server has the good license! */
    else
       { rc = FALSE;
         while (psqlca->SQLMCT > 0)
            PopSqlcaMsg(psqlca);  /* clean out the error for the retry */
       }
#endif


    return(rc);
}
