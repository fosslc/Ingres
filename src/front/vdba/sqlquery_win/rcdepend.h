/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : rcdepend.h, Header File.
**    Project  : Ingres II / Visual DBA.
**    Author   : Sotheavut UK (uk$so01) 
**    Purpose  : Depend of Application and Resource.
**
** History:
**
** xx-Dec-1998 (uk$so01)
**   Created.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 15-Oct-2003 (uk$so01)
**    SIR #106648, (Integrate 2.65 features for EDBC)
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/

#if !defined (APPLICATION_AND_RESOURCE_DEPENDEND_HEADER)
#define APPLICATION_AND_RESOURCE_DEPENDEND_HEADER
#include "sqlquery.h"
BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);

#if defined (EDBC)
#define IDS_xSQLQUERY     IDS_EDBQUERY
#define IDS_xSQLQUERY_PPG IDS_EDBQUERY_PPG
#else
#define IDS_xSQLQUERY     IDS_SQLQUERY
#define IDS_xSQLQUERY_PPG IDS_SQLQUERY_PPG
#endif

enum QepType {QEP_NORMAL    = 1, QEP_STAR};
enum QepNode {QEP_NODE_ROOT = 1, QEP_NODE_INTERNAL, QEP_NODE_LEAF};

#define UPDATE_HINT_LOAD             1
#define UPDATE_HINT_SETTINGCHANGE    2

#define QEP_BASE (WM_USER +5000)
#define WM_QEP_REDRAWLINKS              (QEP_BASE + 1)
#define WM_SQL_STATEMENT_CHANGE         (QEP_BASE + 2)
#define WM_SQL_STATEMENT_EXECUTE        (QEP_BASE + 3)
#define WM_SQL_QUERY_UPADATE_DATA       (QEP_BASE + 4)
#define WM_SQL_QUERY_PAGE_SHOWHEADER    (QEP_BASE + 5)
#define WM_SQL_QUERY_PAGE_HIGHLIGHT     (QEP_BASE + 6)
#define WM_SQL_STATEMENT_SHOWRESULT     (QEP_BASE + 7)
#define WM_SQL_GETPAGE_DATA             (QEP_BASE + 8)
#define WM_SQL_FETCH                    (QEP_BASE + 9)
#define WM_SQL_FETCH_FORWARD            (QEP_BASE +10)
#define WM_SQL_FETCH_BACKWARD           (QEP_BASE +11)
#define WM_SQL_CURSOR_KEYUP             (QEP_BASE +12)
#define WM_SQL_CLOSE_CURSOR             (QEP_BASE +13)
#define WM_SQL_TAB_BITMAP               (QEP_BASE +14)
//
// Result page has the GroupID number = 0 at the create time.
// When the page is load from the serialization, its GroupID will increase by one from
// from the one it was saved.
#define WM_SQL_GETPAGE_GROUPID          (QEP_BASE + 15)
#define WM_QEP_OPEN_POPUPINFO           (QEP_BASE + 16)
#define WM_QEP_CLOSE_POPUPINFO          (QEP_BASE + 17)
#define WM_SQL_GETMAXROWLIMIT           (QEP_BASE + 18)

#define WM_USER_GETNODEHANDLE           (WM_USER + 20) // Get node handle from mfc documents
#define WM_USER_GETMFCDOCTYPE           (WM_USER + 21) // Get document type from mfc documents

#define MFCMSG_BASE (QEP_BASE +100)
// wparam and lparam null, return value used as bool TRUE if has toolbar
#define WM_USER_HASTOOLBAR                  (WM_USER + MFCMSG_BASE +  4)

// wparam and lparam null, return value used as bool TRUE if visible
#define WM_USER_ISTOOLBARVISIBLE            (WM_USER + MFCMSG_BASE +  5)

// wparam = new state as bool, lparam null,
// return value used as bool TRUE if successful
#define WM_USER_SETTOOLBARSTATE             (WM_USER + MFCMSG_BASE +  6)

// wparam null, lparam contains LPCSTR to new toolbar caption
// return value used as bool TRUE if successful
#define WM_USER_SETTOOLBARCAPTION           (WM_USER + MFCMSG_BASE +  7)

// Emb April 2, 97: message for delayed nodes window openwin list refresh
// wparam null, lparam contains char * result of GetVirtNodeName()
#define WM_USER_UPDATEOPENWIN               (WM_USER + MFCMSG_BASE +  8)

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
#define WM_QUERY_OPEN_CURSOR (WM_USER + MFCMSG_BASE+41)

enum {BM_NEWTAB, BM_SELECT, BM_NON_SELECT, BM_SELECT_CLOSE, BM_SELECT_BROKEN, BM_SELECT_OPEN, BM_QEP, BM_TRACE, BM_XML};

#endif
