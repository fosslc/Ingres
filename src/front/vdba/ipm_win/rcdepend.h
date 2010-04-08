/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : rcdepend.h, Header File.
**    Project  : Splash/About box (Corporated QA)
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Depend on Application and Resource.
**
** History:
**
** 06-Nov-2001 (uk$so01)
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
**/



#if !defined (APPLICATION_AND_RESOURCE_DEPENDEND_HEADER)
#define APPLICATION_AND_RESOURCE_DEPENDEND_HEADER
#include "ipm.h"
#if !defined (_INCLUDE_RCTOOL_HEADER)
#define _INCLUDE_RCTOOL_HEADER
#include "rctools.h"
#endif // _INCLUDE_RCTOOL_HEADER
#include "wmusrmsg.h"

#define MFCMSG_BASE                 2000
//
// WPARAM = 0, LPARAM = pointer to RAISEDDBE Structure defined in DbagInfo.h
#define WM_USER_DBEVENT_TRACE_INCOMING      (WM_USER + MFCMSG_BASE +  1)    
//
// WPARAM and LPARAM are the data expected by the reciever and
// depend on which they will be sent.
#define WM_USER_IPMPAGE_UPDATEING           (WM_USER + MFCMSG_BASE +  2)
//
// WPARAM and LPARAM are the data expected by the reciever and
// depend on which they will be sent.
#define WM_USER_IPMPAGE_LOADING             (WM_USER + MFCMSG_BASE +  9)

//
// WPARAM = 0, LPARAM =0
// Return: The new pointer to the base class CuIpmPropertyData;
// The call must take care of destroying this pointer.
#define WM_USER_IPMPAGE_GETDATA             (WM_USER + MFCMSG_BASE +  10)
//
// WPARAM , LPARAM : depends on the sender and the receiver.
#define WM_USER_APPLYNOW                    (WM_USER + MFCMSG_BASE +  11)

typedef enum
{ 
	LEAVINGPAGE_CHANGELEFTSEL = 1,
	LEAVINGPAGE_CHANGEPROPPAGE,
	LEAVINGPAGE_CLOSEDOCUMENT
}	LeavePageType;
// WPARAM: LeavPageType --->
//         LEAVINGPAGE_CHANGELEFTSEL  if change selection on left,
//         LEAVINGPAGE_CHANGEPROPPAGE if change property page
//         LEAVINGPAGE_CLOSEDOCUMENT  if document closed
// LPARAM = 0
#define WM_USER_IPMPAGE_LEAVINGPAGE        (WM_USER + MFCMSG_BASE +  12)
#define WM_USER_EXPRESS_REFRESH            (WM_USER + MFCMSG_BASE +  13)




//
// Bitmap IDs
// ************************************************************************************************
#define IDB_TM_ICE_ALLTRANSACTION    IDB_TM_ICE_TRANSACTION
#define IDB_TM_ICE_ALLCURSOR         IDB_TM_ICE_CURSOR
#define IDB_TM_LOCK_SERVER           IDB_LOCK
#define IDB_TM_LOCKINFO              IDB_LOCK
#define IDB_TM_LOCK_RESOURCE         IDB_LOCK
#define IDB_TM_SESS_TRANSAC          IDB_TRANSACTION
#define IDB_TM_TRANSAC               IDB_TRANSACTION
#define IDB_TM_TRANSAC_RESOURCE      IDB_TRANSACTION
#define IDB_TM_TRANSAC_SERVER        IDB_TRANSACTION
#define IDB_TM_DB_TRANSAC            IDB_TRANSACTION
#define IDB_TM_LOGTRANSAC            IDB_TRANSACTION
#define IDB_TM_SERVER                IDB_SERVER_OTHER
#define IDB_TM_DB_SERVER             IDB_SERVER_OTHER
#define IDB_CHECK                    IDB_CHECK4LISTCTRL
#define IDB_CHECK_NOFRAME            IDB_CHECK4LISTCTRL_NOFRAME
#define IDB_TM_ACTUSR                IDB_USER
#define IDB_TM_DB_SESSION            IDB_SESSION
#define IDB_TM_SESSION               IDB_SESSION
#define IDB_TM_LOCK_SESSION          IDB_SESSION
#define IDB_TM_TRANSAC_SESSION       IDB_SESSION
#define IDB_TM_USRSESS               IDB_SESSION
#define IDB_TM_DB_LOCK               IDB_LOCK
#define IDB_TM_DB_LOCKONDB           IDB_LOCK
#define IDB_TM_LOCK                  IDB_LOCK
#define IDB_TM_SESS_LOCK             IDB_LOCK
#define IDB_TM_SESS_LOCKLIST_BL_LOCK IDB_LOCK
#define IDB_TM_SESS_LOCKLIST_LOCK    IDB_LOCK
#define IDB_TM_TRANSAC_LOCK          IDB_LOCK
#define IDB_TM_DB_LOCKLIST           IDB_LOCKS
#define IDB_TM_LOCK_LOCKLIST         IDB_LOCKS
#define IDB_TM_LOCKLIST              IDB_LOCKS
#define IDB_TM_LOCKLIST_BLOCKED_NO   IDB_LOCKS
#define IDB_TM_SESS_LOCKLIST         IDB_LOCKS
#define IDB_TM_TRANSAC_LOCKLIST      IDB_LOCKS
#define IDB_TM_SESSION_BLOCKED       IDB_SESSION_BLOCKED
#define IDB_TM_LOCKLIST_BLOCKED_YES  IDB_LOCKS_BLOCKED
#define IDB_TM_DB_LOCKEDOTHER        IDB_FOLDER_CLOSED
#define IDB_TM_LOCK_TYPE_OTHER       IDB_FOLDER_CLOSED
#define IDB_TM_LOCKEDOTHER           IDB_FOLDER_CLOSED
#define IDB_TM_OTHER                 IDB_FOLDER_CLOSED
#define IDB_TM_TRANSAC_OTHER         IDB_FOLDER_CLOSED
#define IDB_BITMAP_DEFAULT           IDB_FOLDER_CLOSED
#define IDB_TM_DB_TYPE_DISTRIBUTED   IDB_DATABASE_DISTRIBUTED
#define IDB_TM_DB_TYPE_COORDINATOR   IDB_DATABASE_COORDINATOR
#define IDB_TM_DB_TYPE_ERROR         IDB_DATABASE_ERROR
#define IDB_TM_ALLDB                 IDB_DATABASE
#define IDB_TM_DB                    IDB_DATABASE
#define IDB_TM_DB_TYPE_REGULAR       IDB_DATABASE
#define IDB_TM_LOC_DB                IDB_DATABASE
#define IDB_TM_LOCK_TYPE_DB          IDB_DATABASE
#define IDB_TM_LOCKEDDB              IDB_DATABASE
#define IDB_TM_LOGDB                 IDB_DATABASE
#define IDB_TM_REPLIC_ALLDB          IDB_DATABASE
#define IDB_TM_SESS_DB               IDB_DATABASE
#define IDB_TM_TRANSAC_DB            IDB_DATABASE
#define IDB_TM_DB_ALLTBL             IDB_TABLE
#define IDB_TM_DB_LOCKEDTBL          IDB_TABLE
#define IDB_TM_LOCK_TYPE_TABLE       IDB_TABLE
#define IDB_TM_LOCKEDTBL             IDB_TABLE
#define IDB_TM_TABLE                 IDB_TABLE
#define IDB_TM_TRANSAC_TBL           IDB_TABLE
#define IDB_TM_DB_LOCKEDPAGE         IDB_PAGE
#define IDB_TM_ICE_FILEINFO_ALL      IDB_PAGE
#define IDB_TM_LOCK_TYPE_PAGE        IDB_PAGE
#define IDB_TM_LOCKEDPAGE            IDB_PAGE
#define IDB_TM_PAGE                  IDB_PAGE
#define IDB_TM_TRANSAC_PAGE          IDB_PAGE
#define IDB_TM_SERVER_TYPE_INGRES    IDB_SERVER_INGRES
#define IDB_TM_SERVER_TYPE_NAME      IDB_SERVER_NAME
#define IDB_TM_SERVER_TYPE_NET       IDB_SERVER_NET
#define IDB_TM_SERVER_TYPE_STAR      IDB_SERVER_STAR
#define IDB_TM_SERVER_TYPE_OTHER     IDB_SERVER_OTHER
#define IDB_TM_SERVER_TYPE_RECOVERY  IDB_SERVER_RECOVERY
#define IDB_TM_SERVER_TYPE_ICE       IDB_SERVER_ICE
#define IDB_TM_SERVER_TYPE_RMCMD     IDB_SERVER_RMCMD
#define IDB_TM_SERVER_TYPE_JDBC      IDB_SERVER_JDBC


//
// String IDs
// ************************************************************************************************
#define IDS_TC_LOCKLIST_ID           IDS_TC_ID
#define IDS_TC_RESOURCE_ID           IDS_TC_ID
#define IDS_TC_LOCKID                IDS_TC_ID


BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);

#endif // APPLICATION_AND_RESOURCE_DEPENDEND_HEADER
