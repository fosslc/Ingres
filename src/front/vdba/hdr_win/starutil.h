// starutil.h : interface for cpp entry points (dialogs, ...)

#ifndef _STARINTERFACE_INCLUDED
#define _STARINTERFACE_INCLUDED

#define LINK_TABLE      1
#define LINK_VIEW       2
#define LINK_PROCEDURE  3

//
// First Section : "extern "C" functions
//
#ifdef __cplusplus
extern "C"
{
#endif

#include "dbaset.h"

int DlgRegisterTableAsLink(HWND hwndParent, LPCSTR destNodeName, LPCSTR destDbName, BOOL bDragDrop, LPCSTR srcNodeName, LPCSTR srcDbName, LPCSTR srcTblName, LPCSTR srcOwnerName);
int DlgRegisterViewAsLink(HWND hwndParent, LPCSTR destNodeName, LPCSTR destDbName, BOOL bDragDrop, LPCSTR srcNodeName, LPCSTR srcDbName, LPCSTR srcViewName, LPCSTR srcOwnerName);
int DlgRegisterProcAsLink(HWND hwndParent, LPCSTR destNodeName, LPCSTR destDbName, BOOL bDragDrop, LPCSTR srcNodeName, LPCSTR srcDbName, LPCSTR srcProcName, LPCSTR srcOwnerName);
int DlgStarLocalTable(HWND hwnd, LPTABLEPARAMS lptbl);

#ifdef __cplusplus
}
#endif


//
// Second Section : pure c++ functions
//
#ifdef __cplusplus
BOOL IsLocalNodeName(LPCTSTR nodeName, BOOL bCanContainServerClass);
CString GetLocalHostName();
CString VerifyStarSqlError(LPCTSTR csDestNodeName, LPCTSTR csCurrentNodeName);
#endif


#endif  // _STARINTERFACE_INCLUDED
