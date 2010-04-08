/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : rcdepend.h, Header File.
**    Project  : Ingres II / Visual DBA. 
**    Author   : Sotheavut UK (uk$so01)
** 
**    Purpose  : Depend of Application and Resource.
**
** History:
** xx-Nov-1998 (uk$so01)
**    Created
** 28-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vdba uses libwctrl.lib).
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/

#if !defined (APPLICATION_AND_RESOURCE_DEPENDEND_HEADER)
#define APPLICATION_AND_RESOURCE_DEPENDEND_HEADER

#include "mainmfc.h"
#include "resource.h"
#include "rctools.h"
#include "wmusrmsg.h"
#include "prodname.h"

#define WM_BASE2                       (WM_BASE + 1000)

#define WM_DBBROWS_VNMDICHILDCLOSE     (WM_BASE2+ 1)    // WPARAM = 0, LPARAM = CSize*
#define WM_QUERY_UPDATING              (WM_BASE2+ 2)
#define WM_QUERY_GETDATA               (WM_BASE2+ 3)
#define WM_QUERY_JOINCHANGE            (WM_BASE2+ 4)
#define WM_CHANGE_FONT                 (WM_BASE2+ 5)
#define WM_QUERY_OPEN_CURSOR           (WM_BASE2+ 6)
//
// WPARAM = 0, LPARAM = pointer to RAISEDDBE Structure defined in DbagInfo.h
#define WM_USER_DBEVENT_TRACE_INCOMING (WM_BASE2+ 7)
//
// WPARAM and LPARAM are the data expected by the reciever and
// depend on which they will be sent.
#define WM_USER_IPMPAGE_UPDATEING      (WM_BASE2+ 8)
//
// WPARAM and LPARAM are the data expected by the reciever and
// depend on which they will be sent.
#define WM_USER_REFRESH_DATA           (WM_BASE2+ 9)

//
// Emb started 25/03/97: Added messages for documents toolbars
//

// wparam and lparam null, return value used as bool TRUE if has toolbar
#define WM_USER_HASTOOLBAR             (WM_BASE2+10)
// wparam and lparam null, return value used as bool TRUE if visible
#define WM_USER_ISTOOLBARVISIBLE       (WM_BASE2+11)
// wparam = new state as bool, lparam null,
// return value used as bool TRUE if successful
#define WM_USER_SETTOOLBARSTATE        (WM_BASE2+12)
// wparam null, lparam contains LPCSTR to new toolbar caption
// return value used as bool TRUE if successful
#define WM_USER_SETTOOLBARCAPTION      (WM_BASE2+13)
// Emb April 2, 97: message for delayed nodes window openwin list refresh
// wparam null, lparam contains char * result of GetVirtNodeName()
#define WM_USER_UPDATEOPENWIN          (WM_BASE2+14)
//
// WPARAM and LPARAM are the data expected by the reciever and
// depend on which they will be sent.
#define WM_USER_IPMPAGE_LOADING        (WM_BASE2+15)
//
// WPARAM = 0, LPARAM =0
// Return: The new pointer to the base class CuIpmPropertyData;
// The call must take care of destroying this pointer.
#define WM_USER_IPMPAGE_GETDATA        (WM_BASE2+16)
//
// WPARAM , LPARAM : depends on the sender and the receiver.
#define WM_USER_APPLYNOW               (WM_BASE2+17)
// WPARAM: LeavPageType --->
//         LEAVINGPAGE_CHANGELEFTSEL  if change selection on left,
//         LEAVINGPAGE_CHANGEPROPPAGE if change property page
//         LEAVINGPAGE_CLOSEDOCUMENT  if document closed
// LPARAM = 0
#define WM_USER_IPMPAGE_LEAVINGPAGE    (WM_BASE2+18)
//
// New as of may 25, 98, for dom right pane update :
// Request the type of the page, for update purposes
//
// input data: wParam set to NULL, lParam set to NULL
// output data: DOMPAGE_XYZ enum type
#define WM_USER_DOMPAGE_QUERYTYPE      (WM_BASE2+19)





#endif // APPLICATION_AND_RESOURCE_DEPENDEND_HEADER
