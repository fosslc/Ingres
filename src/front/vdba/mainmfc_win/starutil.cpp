/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Project : Visual DBA
**
**  Source :starutil.cpp
**    interface for cpp entry points (dialogs, ...)
**
**
**  History
**  02-Sep-2002 (scph01)
**    Bug 108645 call the new function GetLocalVnodeName ( )
**    (instead GCHostname()) to retrieve the "gcn.local_vnode" parameter in
**    config.dat file.
**  01-Oct-2002 (schph01)
**    Sir 108841 Add VerifyStarSqlError() function.
**  31-0ct-2003 (noifr01)
**   upon checks after massive ingres30->main codeline propagations, added
**   trailing CR at the end of the file for avoiding potential future 
**   propagation problems
*/


#include "stdafx.h"
#include "starutil.h"

#include "resmfc.h"     // ressource symbols, necessary before include of dialog headers
#include "startbln.h"   // Table/View link
#include "starprln.h"   // Procedure link
#include "extccall.h"

extern "C" {
#include "compat.h"
#include "gc.h"
#include "msghandl.h"     // IDS_I_LOCALNODE

#include "dbaginfo.h"     // GetGWClassNameFromString(), ...
}


static int DlgRegisterGenericAsLink(HWND hwndParent, LPCSTR destNodeName, LPCSTR destDbName, BOOL bDragDrop, LPCSTR srcNodeName, LPCSTR srcDbName, LPCSTR srcObjName, LPCSTR srcOwnerName, int linkType)
{
  ASSERT (linkType == LINK_TABLE || linkType == LINK_VIEW);

  CuDlgStarTblRegister  dlg;

  ASSERT (destDbName);
  dlg.m_destNodeName  = destNodeName;
  dlg.m_destDbName    = destDbName;
  dlg.m_bDragDrop     = bDragDrop;
  dlg.m_linkType      = linkType;
  if (bDragDrop) {
    ASSERT (srcNodeName);
    ASSERT (srcDbName);
    ASSERT (srcObjName);
    ASSERT (srcOwnerName);
    dlg.m_srcNodeName   = srcNodeName;
    dlg.m_srcDbName     = srcDbName;
    dlg.m_srcObjName    = srcObjName;
    dlg.m_srcOwnerName  = srcOwnerName;
  }

  int retval = dlg.DoModal();

  if (!bDragDrop)
    UpdateNodeDisplay();  // update connection state on icons in mfc list of nodes

  return retval;

}

int DlgRegisterTableAsLink(HWND hwndParent, LPCSTR destNodeName, LPCSTR destDbName, BOOL bDragDrop, LPCSTR srcNodeName, LPCSTR srcDbName, LPCSTR srcTblName, LPCSTR srcOwnerName)
{
  return DlgRegisterGenericAsLink(hwndParent, destNodeName, destDbName, bDragDrop, srcNodeName, srcDbName, srcTblName, srcOwnerName, LINK_TABLE);
}

int DlgRegisterViewAsLink(HWND hwndParent, LPCSTR destNodeName, LPCSTR destDbName, BOOL bDragDrop, LPCSTR srcNodeName, LPCSTR srcDbName, LPCSTR srcViewName, LPCSTR srcOwnerName)
{
  return DlgRegisterGenericAsLink(hwndParent, destNodeName, destDbName, bDragDrop, srcNodeName, srcDbName, srcViewName, srcOwnerName, LINK_VIEW);
}

int DlgRegisterProcAsLink(HWND hwndParent, LPCSTR destNodeName, LPCSTR destDbName, BOOL bDragDrop, LPCSTR srcNodeName, LPCSTR srcDbName, LPCSTR srcProcName, LPCSTR srcOwnerName)
{
  CuDlgStarProcRegister  dlg;

  ASSERT (destDbName);
  dlg.m_destNodeName  = destNodeName;
  dlg.m_destDbName    = destDbName;
  dlg.m_bDragDrop     = bDragDrop;
  if (bDragDrop) {
    ASSERT (srcNodeName);
    ASSERT (srcDbName);
    ASSERT (srcProcName);
    ASSERT (srcOwnerName);
    dlg.m_srcNodeName   = srcNodeName;
    dlg.m_srcDbName     = srcDbName;
    dlg.m_srcObjName    = srcProcName;
    dlg.m_srcOwnerName  = srcOwnerName;
  }

  int retval = dlg.DoModal();

  if (!bDragDrop)
    UpdateNodeDisplay();  // update connection state on icons in mfc list of nodes

  return retval;
}


BOOL IsLocalNodeName(LPCTSTR nodeName, BOOL bCanContainServerClass)
{
  CString csLocal;
  csLocal.LoadString(IDS_I_LOCALNODE);
  ASSERT (!csLocal.IsEmpty());
  if (csLocal.IsEmpty())
    csLocal = "(local)";

  // Remove server class if specified
  CString csNodeName = nodeName;
  if (bCanContainServerClass) {

    // First, remove user name from source node name.
    UCHAR nodeNameNoUser[MAXOBJECTNAME];
    x_strcpy((char*)nodeNameNoUser, (LPCTSTR)csNodeName);
    RemoveConnectUserFromString(nodeNameNoUser);
    csNodeName = nodeNameNoUser;

    UCHAR gwClassName[MAXOBJECTNAME];
    BOOL bHasGW = GetGWClassNameFromString((LPUCHAR)nodeName, gwClassName);
    UCHAR nodeNameNoGW[MAXOBJECTNAME];
    x_strcpy ((char*)nodeNameNoGW, (LPCTSTR)nodeName);
    if (bHasGW) {
      RemoveGWNameFromString(nodeNameNoGW);
      csNodeName = nodeNameNoGW;
    }
  }

  // Test
  if (csLocal.CompareNoCase(csNodeName) == 0)
    return TRUE;
  else
    return FALSE;
}


CString GetLocalHostName()
{
  char szGCHostName[MAXOBJECTNAME];
  memset (szGCHostName, '\0', MAXOBJECTNAME);
  GetLocalVnodeName (szGCHostName,MAXOBJECTNAME);
  ASSERT (szGCHostName[0]);
  if (!szGCHostName[0])
    return "UNKNOWN LOCAL HOST NAME";
  CString csLocal = szGCHostName;
  return csLocal;
}

/*
**  This function verify that the last SQL error number was -39100 and that the
**  error text starts with the E_GC0132 text, to return a additional
**  warning to explain why the "register as link" command failed.
**
**  Input parameters
**    csDestNodeName    : 
**    csCurrentNodeName :
**
**  return value
**    The additional warning or Empty string.
*/
CString VerifyStarSqlError(LPCTSTR csDestNodeName, LPCTSTR csCurrentNodeName)
{
  TCHAR *szErrNo132 = _T("E_GC0132");
  LPSQLERRORINFO lpsqlErrInfo;
  CString csTemp;
  csTemp.Empty();
  lpsqlErrInfo = GetNewestSqlError();
  if (lpsqlErrInfo && lpsqlErrInfo->sqlcode == -39100L &&
      x_strncmp((LPTSTR)lpsqlErrInfo->lpSqlErrTxt,szErrNo132,lstrlen(szErrNo132)) == 0)
  {
      csTemp.Format(IDS_E_VNODE_NOT_DEFINE,(LPCTSTR)csDestNodeName,
                    (IsLocalNodeName((const char*)csCurrentNodeName, FALSE) ? (LPCTSTR)GetLocalHostName():(LPCTSTR)csCurrentNodeName));
  }
  return csTemp;
}
