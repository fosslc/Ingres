/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
**
**  Project : Visual DBA
**
**  Source : cmdline2.cpp
**
**  Author : Emmanuel Blattes
**
**  History after 01-01-2000:
**
** 03-Feb-2000 (noifr01)
**  (SIR 100331) manage the (new) RMCMD monitor server type
** 01-Aug-2001 (noifr01)
**  (sir 105275) manage the JDBC server type
** 26-Fev-2002 (uk$so01)
**    SIR #106648, Integrate ipm.ocx.
*********************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "vtree.h"
#include "cmdline2.h"
#include "ipmaxfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C" {
//#include "dba.h"
#include "domdata.h"
//#include "main.h"
#include "domdisp.h"
#include "getdata.h"    // GetCurMdiNodeHandle()
#include "monitor.h"
};


BOOL ExpandUpToSearchedItem(CWnd* pPropWnd, CTypedPtrList<CObList, CuIpmTreeFastItem*>& rIpmTreeFastItemList, BOOL bAutomated)
{
/*UKS
  CfIpm* pIpmFrame = 0;
  if (bAutomated) {
    ASSERT (pPropWnd->IsKindOf(RUNTIME_CLASS(CfIpm)));
    pIpmFrame = (CfIpm*)pPropWnd;
  }
  else {
    CWnd* pParent  = pPropWnd->GetParent();   // ---> pParent:  CTabCtrl
    CWnd* pParent2 = pParent->GetParent();    // ---> pParent2: CuDlgIpmTabCtrl
    CWnd* pParent3 = pParent2->GetParent();   // ---> pParent3: CIpmView2
    CWnd* pParent4 = pParent3->GetParent();   // ---> pParent4: CSplitterWnd
    CWnd* pParent5 = pParent4->GetParent();   // ---> pParent5: CfIpm
    ASSERT (pParent5->IsKindOf(RUNTIME_CLASS(CfIpm)));
    pIpmFrame = (CfIpm*)pParent5;
  }
  ASSERT (pIpmFrame);

  // Get selected item handle
  HTREEITEM hItem = 0;
  if (bAutomated) {
    hItem = 0;      // Will force search on all root items
  }
  else {
    hItem = pIpmFrame->GetSelectedItem();
    ASSERT (hItem);
  }

  // Loop on fastitemlist
  POSITION pos = rIpmTreeFastItemList.GetHeadPosition();
  while (pos) {
    CuIpmTreeFastItem* pFastItem = rIpmTreeFastItemList.GetNext (pos);
    hItem = pIpmFrame->FindSearchedItem(pFastItem, hItem);
    if (!hItem) {
      ASSERT (FALSE);
      return FALSE;
    }
  }
  pIpmFrame->SelectItem(hItem);
*/
  return TRUE;
}


BOOL IpmCurSelHasStaticChildren(CWnd* pPropWnd)
{
/*UKS
  CWnd* pParent  = pPropWnd->GetParent();   // ---> pParent:  CTabCtrl
  CWnd* pParent2 = pParent->GetParent();    // ---> pParent2: CuDlgIpmTabCtrl
  CWnd* pParent3 = pParent2->GetParent();   // ---> pParent3: CIpmView2
  CWnd* pParent4 = pParent3->GetParent();   // ---> pParent4: CSplitterWnd
  CWnd* pParent5 = pParent4->GetParent();   // ---> pParent5: CfIpm

  ASSERT (pParent5->IsKindOf(RUNTIME_CLASS(CfIpm)));
  CfIpm* pIpmFrame = (CfIpm*)pParent5;

  // Get selected item handle
  HTREEITEM hItem = pIpmFrame->GetSelectedItem();
  ASSERT (hItem);
  if (!hItem)
    return FALSE;

  CTreeItem *pItem = (CTreeItem*)pIpmFrame->GetPTree()->GetItemData(hItem);
  ASSERT (pItem);
  if (!pItem)
    return FALSE;
  SubBK subBranchKind = pItem->GetSubBK();
  if (subBranchKind == SUBBRANCH_KIND_STATIC)
    return TRUE;
  else
*/
    return FALSE;
}



/////////////////////////////////////////////////////////////////////////////////////////
// CuObjDesc: anchor base class for CuDomObjDesc and CuIpmObjDesc

IMPLEMENT_DYNAMIC(CuObjDesc, CObject)

CuObjDesc::CuObjDesc(LPCTSTR objName, int objType, int level, int parent0type, int parent1type)
{
  m_bParsed       = FALSE;
  m_csFullObjName = objName;
  m_objType       = objType;

  ASSERT (level >= 0 && level <= 2);
  m_level = level;

  if (level > 0) {
    ASSERT (parent0type);
    m_parent0type = parent0type;
  }
  if (level > 1) {
    ASSERT (parent1type);
    m_parent1type = parent1type;
  }

  m_csObjName = _T("");
  m_csParent0 = _T("");
  m_csParent1 = _T("");
}

BOOL CuObjDesc::ParseObjectDescriptor()
{
  m_bParsed = TRUE;

  if (IsStaticType(m_objType)) {
    m_csObjName = m_csFullObjName;
    return TRUE;
  }

  switch (m_level) {
    case 0:
      m_csObjName = m_csFullObjName;
      return TRUE;
    case 1:
      return ParseLevel1ObjName();
    case 2:
      return ParseLevel2ObjName();
    default:
      return FALSE;
  }
}

BOOL CuObjDesc::ParseLevel1ObjName(CString csObjId, CString& rcsParent, CString& rcsObj)
{
  int index = csObjId.Find(_T('/'));
  if (index == -1)
    return FALSE;       // not found
  if (index == 0)
    return FALSE;       // not acceptable at first position since no parent
  if (index >= csObjId.GetLength())
    return FALSE;       // not acceptable at last position since no objname
  rcsParent = csObjId.Left(index);
  rcsObj    = csObjId.Mid(index+1);
  return TRUE;
}

BOOL CuObjDesc::ParseLevel2ObjName(CString csObjId, CString& rcsParent0, CString& rcsParent1, CString& rcsObj)
{
  CString csIntermediary;
  if (ParseLevel1ObjName(csObjId, rcsParent0, csIntermediary))
    if (ParseLevel1ObjName(csIntermediary, rcsParent1, rcsObj))
      return TRUE;
  return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////
// Base class for Dom object descriptor

IMPLEMENT_DYNAMIC(CuDomObjDesc, CuObjDesc)

CuDomObjDesc::CuDomObjDesc(LPCTSTR objName, int objType, int level, int parent0type, int parent1type)
: CuObjDesc(objName, objType, level, parent0type, parent1type)
{
}

BOOL CuDomObjDesc::IsStaticType(int type)
{
  if (GetStaticType(type) == type || GetStaticType(type) == 0)
    return TRUE;
  else
    return FALSE;
}

BOOL CuDomObjDesc::BuildFastItemList(CTypedPtrList<CObList, CuDomTreeFastItem*>& rDomTreeFastItemList)
{
  ASSERT (IsParsed());
  if (!IsParsed())
    return FALSE;

  switch (m_level) {
    // Note: reverse values and no break to reduce code size
    case 2:
    case 1:
      if (!IsStaticType(m_parent0type))
        rDomTreeFastItemList.AddTail(new CuDomTreeFastItem(GetStaticType(m_parent0type)));
      rDomTreeFastItemList.AddTail(new CuDomTreeFastItem(m_parent0type, m_csParent0));

      if (m_level == 1)
        goto LabelZero;

      if (!IsStaticType(m_parent1type))
        rDomTreeFastItemList.AddTail(new CuDomTreeFastItem(GetStaticType(m_parent1type)));
      rDomTreeFastItemList.AddTail(new CuDomTreeFastItem(m_parent1type, m_csParent1));
      // NO BREAK!

    case 0:
LabelZero:
      if (!IsStaticType(m_objType))
        rDomTreeFastItemList.AddTail(new CuDomTreeFastItem(GetStaticType(m_objType)));
      rDomTreeFastItemList.AddTail(new CuDomTreeFastItem(m_objType, m_csObjName));
      break;

    default:
      ASSERT (FALSE);
      return FALSE;
  }
  return TRUE;
}

BOOL CuDomObjDesc::CheckLowLevel(int hNode)
{
  // parent 0, if needed
  if (m_level > 0) {
    ASSERT (m_parent0type);
    if (!IsStaticType(m_parent0type))
      ASSERT (!m_csParent0.IsEmpty());
    if (!CheckObjectExistence(hNode, m_csParent0, m_parent0type, 0))
      return FALSE;
  }

  // parent 1, if needed
  if (m_level > 1) {
    ASSERT (m_parent1type);
    if (!IsStaticType(m_parent1type))
      ASSERT (!m_csParent1.IsEmpty());
    if (!CheckObjectExistence(hNode, m_csParent1, m_parent1type, 1, m_csParent0))
      return FALSE;
  }

  // object itself
  if (!CheckObjectExistence(hNode, m_csObjName, m_objType, m_level, m_csParent0, m_csParent1))
    return FALSE;

  return TRUE;
}

BOOL CuDomObjDesc::CheckObjectExistence(int hNode, LPCTSTR objName, int objType, int level, LPCTSTR parent0, LPCTSTR parent1)
{
  if (IsStaticType(objType))
    return TRUE;

  return GetObjectLimitedInfo(hNode, objName, objType, level, parent0, parent1, FALSE);
}

BOOL CuDomObjDesc::GetObjectLimitedInfo(int hNode, LPCTSTR objName, int objType, int level, LPCTSTR parent0, LPCTSTR parent1, BOOL bFillBuffers, LPUCHAR lpResBufObj, LPUCHAR lpResBufOwner, LPUCHAR lpResBufComplim)
{
  if (level > 0)
    ASSERT (parent0);
  if (level > 1)
    ASSERT (parent1);

  if (IsStaticType(objType)) {
    if (bFillBuffers) {
      x_strcpy((char*)lpResBufObj, objName);
      lpResBufOwner[0] = _T('\0');
      lpResBufComplim[0] = _T('\0');
    }
    return TRUE;
  }

  UCHAR*        aParents[3];
  aParents[0] = (LPUCHAR)parent0;
  aParents[1] = (LPUCHAR)parent1;
  aParents[2] = NULL;

  UCHAR buf[MAXOBJECTNAME];
  UCHAR bufOwner[MAXOBJECTNAME];
  OwnerFromString((LPUCHAR)objName, bufOwner);
  lstrcpy((char *)buf, (char *)StringWithoutOwner((LPUCHAR)objName));

  int   dummyType;
  UCHAR resBufObj[MAXOBJECTNAME];
  UCHAR resBufOwner[MAXOBJECTNAME];
  UCHAR resBufComplim[MAXOBJECTNAME];

  if (bFillBuffers) {
    ASSERT (lpResBufObj);
    ASSERT (lpResBufOwner);
    ASSERT (lpResBufComplim);
  }
  else {
    lpResBufObj     = resBufObj;
    lpResBufOwner   = resBufOwner;
    lpResBufComplim = resBufComplim;
  }

  int res = DOMGetObjectLimitedInfo(hNode,            // int     hnodestruct,
                                    buf,              // LPUCHAR lpobjectname,
                                    bufOwner,         // LPUCHAR lpobjectowner,
                                    objType,          // int     iobjecttype,
                                    level,            // int     level,
                                    aParents,         // LPUCHAR *parentstrings,
                                    TRUE,             // BOOL    bwithsystem,
                                    &dummyType,       // int     *presulttype,
                                    lpResBufObj,      // LPUCHAR lpresultobjectname,  
                                    lpResBufOwner,    // LPUCHAR lpresultowner, 
                                    lpResBufComplim); // LPUCHAR lpresultextradata);
  if (res != RES_SUCCESS)
    return FALSE;

  return TRUE;
}

BOOL CuDomObjDesc::CreateRootTreeItem(LPDOMDATA lpDomData)
{
  LPTREERECORD lpRecord;
  DWORD         recId;
  DWORD         recIdSub;

  UCHAR resBufObj[MAXOBJECTNAME];
  UCHAR resBufOwner[MAXOBJECTNAME];
  UCHAR resBufComplim[MAXOBJECTNAME];

  BOOL bSuccess = GetObjectLimitedInfo(lpDomData->psDomNode->nodeHandle,
                                       m_csObjName, m_objType, m_level,
                                       m_csParent0, m_csParent1,
                                       TRUE, resBufObj, resBufOwner, resBufComplim);
  if (!bSuccess) {
    ASSERT (FALSE);
    return FALSE;
  }

  // tree item create
  lpRecord = AllocAndFillRecord(m_objType,                        // rec type
                                FALSE,                            // bSubValid
                                (LPSTR)(LPCTSTR)m_csParent0,      // parent 0
                                (LPSTR)(LPCTSTR)m_csParent1,      // parent 1
                                NULL,                             // parent 2
                                DBTYPE_REGULAR,                   // parent db type
                                (char*)resBufObj,                 // obj name
                                (char*)resBufOwner,               // obj owner
                                (char*)resBufComplim,             // complim value
                                NULL);                            // table owner
  if (!lpRecord)
    return FALSE;
  recId = TreeAddRecord(lpDomData, (LPSTR)(LPCTSTR)m_csObjName, 0, 0, 0, lpRecord);
  if (recId == 0)
    return FALSE;
  if (NeedsDummySubItem()) {
    recIdSub = AddDummySubItem(lpDomData, recId);
    if (recIdSub == 0)
      return FALSE;
  }

  return TRUE;
}


///////////////////////////////////////////////////////////////////////////////////
// custom classes for Dom object descriptors
// All in header at this time



/////////////////////////////////////////////////////////////////////////////////////////
// CuIpmObjDesc: derived from CuObjDesc

IMPLEMENT_DYNAMIC(CuIpmObjDesc, CuObjDesc)

CuIpmObjDesc::CuIpmObjDesc(LPCTSTR objName, int objType, int level, int parent0type, int parent1type)
: CuObjDesc(objName, objType, level, parent0type, parent1type)
{
  m_lpDataObj = NULL;
  m_lpDataParent = NULL;
}

CuIpmObjDesc::~CuIpmObjDesc()
{
  if (m_lpDataObj)
    delete m_lpDataObj;
  if (m_lpDataParent)
    delete m_lpDataParent;
}

// FINIR! DISCUTABLE
BOOL CuIpmObjDesc::IsStaticType(int type)
{
  if (type == 0)
    return TRUE;
  else
    return FALSE;
}

// for expansion up to requested item - derive if necessary
BOOL CuIpmObjDesc::BuildFastItemList(CTypedPtrList<CObList, CuIpmTreeFastItem*>&  rIpmTreeFastItemList)
{

  // tree organization reflected in FastItemList, different according to record type
  int iret = RES_ERR;
  BOOL bFound = FALSE;
  int hnodestruct = OpenNodeStruct((LPUCHAR)GetVirtNodeName(GetCurMdiNodeHandle()));

  int   iobjecttypeParent = 0;
  void* pstructParent = NULL;

  // parent, if needed
  if (m_level > 0) {
    ASSERT (m_parent0type);
    ASSERT (!m_csParent0.IsEmpty());
    // Static parent
    if (StaticParentNeedsNameCheck()) {
      CString csStaticParent = GetStaticParentNameCheck();
      rIpmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE, m_parent0type, NULL, TRUE, csStaticParent));
    }
    else
      rIpmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE, m_parent0type));
    int reqSize = GetMonInfoStructSize(m_parent0type);
    void* pStructReq = new char[reqSize];
    iret = GetFirstMonInfo(hnodestruct,
                           0,                   // int       iobjecttypeParent,
                           NULL,                // void     *pstructParent,
                           m_parent0type,       // int       iobjecttypeReq,
                           pStructReq,          // void     *pstructReq,
                           NULL);               //  LPSFILTER pFilter)
    if (iret != RES_SUCCESS) {
      ASSERT (FALSE);
      CloseNodeStruct(hnodestruct, FALSE);
      delete pStructReq;
      return FALSE;
    }
    while (iret != RES_ENDOFDATA) {
      if (ParentNameFits(pStructReq)) {
        bFound = TRUE;
        // store, since persistence needed later on - will be freed by destructor
        m_lpDataParent = pStructReq;
        rIpmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, m_parent0type,  m_lpDataParent));   // non-static

        // update variables for next GetFirst/NextMonInfo()
        iobjecttypeParent = m_parent0type;
        pstructParent = pStructReq;

        break;
      }
      iret = GetNextMonInfo(pStructReq);
    }
    if (!bFound) {
      ASSERT (FALSE);
      delete pStructReq;
      return FALSE;
    }
  }

  // Object itself
  void* pStructReq = NULL;
  if (ObjectNeedsNameCheck()) {
    rIpmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE,  m_objType, NULL, TRUE, m_csObjName));  // Root item
    bFound = TRUE;
  }
  else {
    if (ObjectHasStaticBranch())
      rIpmTreeFastItemList.AddTail(new CuIpmTreeFastItem(TRUE,  m_objType));  // static
    int reqSize = GetMonInfoStructSize(m_objType);
    pStructReq = new char[reqSize];
    iret = GetFirstMonInfo(hnodestruct,
                           iobjecttypeParent,   // int       iobjecttypeParent,
                           pstructParent,       // void     *pstructParent,
                           m_objType,           // int       iobjecttypeReq,
                           pStructReq,          // void     *pstructReq,
                           NULL);               //  LPSFILTER pFilter)
    if (iret != RES_SUCCESS) {
      ASSERT (FALSE);
      CloseNodeStruct(hnodestruct, FALSE);
      delete pStructReq;
      return FALSE;
    }
    while (iret != RES_ENDOFDATA) {
      if (ObjectNameFits(pStructReq)) {
        bFound = TRUE;
        // store, since persistence needed later on - will be freed by destructor
        m_lpDataObj = pStructReq;
        rIpmTreeFastItemList.AddTail(new CuIpmTreeFastItem(FALSE, m_objType,  m_lpDataObj));   // non-static
        break;
      }
      iret = GetNextMonInfo(pStructReq);
    }
  }
  CloseNodeStruct(hnodestruct, FALSE);
  if (!bFound) {
    ASSERT (FALSE);
    delete pStructReq;
    return FALSE;
  }
  return TRUE;
}

// for single item on root branch - derive if necessary
CTreeItem* CuIpmObjDesc::CreateRootTreeItem(CdIpm* pDoc)
{
  CTreeItem* pItem = NULL;

  int iret = RES_ERR;
  BOOL bFound = FALSE;
  int hnodestruct = OpenNodeStruct((LPUCHAR)GetVirtNodeName(GetCurMdiNodeHandle()));

  int   iobjecttypeParent = 0;
  void* pstructParent = NULL;

  // get parent structure filled, if needed
  if (m_level > 0) {
    ASSERT (m_parent0type);
    ASSERT (!m_csParent0.IsEmpty());
    int reqSize = GetMonInfoStructSize(m_parent0type);
    void* pStructReq = new char[reqSize];
    iret = GetFirstMonInfo(hnodestruct,
                           0,                   // int       iobjecttypeParent,
                           NULL,                // void     *pstructParent,
                           m_parent0type,       // int       iobjecttypeReq,
                           pStructReq,          // void     *pstructReq,
                           NULL);               //  LPSFILTER pFilter)
    if (iret != RES_SUCCESS) {
      ASSERT (FALSE);
      CloseNodeStruct(hnodestruct, FALSE);
      delete pStructReq;
      return FALSE;
    }
    while (iret != RES_ENDOFDATA) {
      if (ParentNameFits(pStructReq)) {
        bFound = TRUE;
        // store, since persistence needed later on - will be freed by destructor
        m_lpDataParent = pStructReq;

        // update variables for next GetFirst/NextMonInfo()
        iobjecttypeParent = m_parent0type;
        pstructParent = pStructReq;

        break;
      }
      iret = GetNextMonInfo(pStructReq);
    }
    if (!bFound) {
      ASSERT (FALSE);
      delete pStructReq;
      return FALSE;
    }
  }

  // object itself
  if (IsStaticType(m_objType)) {
    pItem = CreateRootBranch(pDoc, NULL);
    CloseNodeStruct(hnodestruct, FALSE);
  }
  else {
    int reqSize = GetMonInfoStructSize(m_objType);
    void* pStructReq = new char[reqSize];
    iret = GetFirstMonInfo(hnodestruct,
                           iobjecttypeParent,   // int       iobjecttypeParent,
                           pstructParent,       // void     *pstructParent,
                           m_objType,           // int       iobjecttypeReq,
                           pStructReq,          // void     *pstructReq,
                           NULL);               //  LPSFILTER pFilter)
    if (iret != RES_SUCCESS) {
      ASSERT (FALSE);
      CloseNodeStruct(hnodestruct, FALSE);
      delete pStructReq;
      return NULL;
    }
    while (iret != RES_ENDOFDATA) {
      if (ObjectNameFits(pStructReq)) {
        bFound = TRUE;
        pItem = CreateRootBranch(pDoc, pStructReq);
        delete pStructReq;
        break;    // break the while
      }
      iret = GetNextMonInfo(pStructReq);
    }

    CloseNodeStruct(hnodestruct, FALSE);
    if (!bFound) {
      ASSERT (FALSE);
      delete pStructReq;
      return NULL;
    }
  }

  ASSERT (pItem);
  return pItem;
}


CString CuIpmObjDesc::GetDisplayName(void* pstruct)
{
  ASSERT (pstruct);
  int hNode = 0;
  char	name[MAXMONINFONAME];
  name[0] = '\0';
  GetMonInfoName(hNode, m_objType, pstruct, name, sizeof(name));
  ASSERT (name[0]);
  CString csName(name);
  return csName;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Derived classes for monitor items
// 

BOOL CuIpmObjDesc_SERVER::ObjectNameFits(void* pStructReq)
{
  ASSERT (pStructReq);
  if (!pStructReq)
    return FALSE;
  LPSERVERDATAMIN lpsvrdata = (LPSERVERDATAMIN)pStructReq;
  if (m_csObjName == lpsvrdata->listen_address)
    return TRUE;
  return FALSE;
}

CTreeItem* CuIpmObjDesc_SERVER::CreateRootBranch(CdIpm* pDoc, void* pStructReq)
{
  CTreeItem* pItem = NULL;
/*UKS
  ASSERT (pStructReq);
  if (!pStructReq)
    return NULL;
  LPSERVERDATAMIN lpsvrdata = (LPSERVERDATAMIN)pStructReq;
  CTreeItemData ItemData(lpsvrdata, sizeof(SERVERDATAMIN), -1, GetDisplayName(lpsvrdata));  // probably uses lpsvrdata->server_class
  switch (lpsvrdata->servertype) {
    case SVR_TYPE_STAR:
    case SVR_TYPE_RECOVERY:
    case SVR_TYPE_DBMS:
      pItem = new CuTMServer (pDoc->GetPTreeGD(), &ItemData);
      break;
    case SVR_TYPE_ICE:
      pItem = new CuTMIceServer (pDoc->GetPTreeGD(), &ItemData);
      break;
	case SVR_TYPE_JDBC:
    case SVR_TYPE_RMCMD: // RMCMD server branch not expandable 
    default:
      pItem = new CuTMServerNotExpandable (pDoc->GetPTreeGD(), &ItemData);
      break;
  }
*/
  return pItem;
}

//////////////////////////////////////////////

BOOL CuIpmObjDesc_SESSION::ObjectNameFits(void* pStructReq)
{
  ASSERT (pStructReq);
  if (!pStructReq)
    return FALSE;
  LPSESSIONDATAMIN lpsessiondata = (LPSESSIONDATAMIN)pStructReq;
  DWORD dwObjVal = 0;
  sscanf((LPCTSTR)m_csObjName, "%lx", &dwObjVal);
  DWORD dwSessVal = 0;
  sscanf((LPCTSTR)lpsessiondata->session_id, "%lu", &dwSessVal);
  ASSERT (dwObjVal);
  ASSERT (dwSessVal);
  if (dwObjVal == dwSessVal)
    return TRUE;
  return FALSE;
}

BOOL CuIpmObjDesc_SESSION::ParentNameFits(void* pStructReq)
{
  ASSERT (pStructReq);
  if (!pStructReq)
    return FALSE;
  LPSERVERDATAMIN lpsvrdata = (LPSERVERDATAMIN)pStructReq;
  if (m_csParent0 == lpsvrdata->listen_address)
    return TRUE;
  return FALSE;
}


CTreeItem* CuIpmObjDesc_SESSION::CreateRootBranch(CdIpm* pDoc, void* pStructReq)
{
  CTreeItem* pItem = NULL;
/*UKS
  ASSERT (pStructReq);
  if (!pStructReq)
    return NULL;
  LPSESSIONDATAMIN lpsessiondata = (LPSESSIONDATAMIN)pStructReq;
  CTreeItemData ItemData(lpsessiondata, sizeof(SESSIONDATAMIN), -1, GetDisplayName(lpsessiondata));
  pItem = new CuTMSession(pDoc->GetPTreeGD(), &ItemData);
*/
  return pItem;
}

//////////////////////////////////////////////

CTreeItem* CuIpmObjDesc_LOGINFO::CreateRootBranch(CdIpm* pDoc, void* pStructReq)
{
  CTreeItem* pItem = NULL;
/*UKS
  ASSERT (!pStructReq);
  pItem = new CuTMLoginfoStatic(pDoc->GetPTreeGD());
*/
  return pItem;
}

//////////////////////////////////////////////

BOOL CuIpmObjDesc_REPLICSERVER::ObjectNameFits(void* pStructReq)
{
  ASSERT (pStructReq);
  if (!pStructReq)
    return FALSE;
  LPREPLICSERVERDATAMIN lpSvr = (LPREPLICSERVERDATAMIN)pStructReq;
  int svrNo = lpSvr->serverno;
  int mySvrNo = 0;
  sscanf((LPCTSTR)m_csObjName, "%d", &mySvrNo);
  if (svrNo == mySvrNo)
    return TRUE;
  return FALSE;
}

BOOL CuIpmObjDesc_REPLICSERVER::ParentNameFits(void* pStructReq)
{
  ASSERT (pStructReq);
  if (!pStructReq)
    return FALSE;
  LPRESOURCEDATAMIN lpData = (LPRESOURCEDATAMIN)pStructReq;
  if (m_csParent0 == lpData->res_database_name)
    return TRUE;
  return FALSE;
}


CTreeItem* CuIpmObjDesc_REPLICSERVER::CreateRootBranch(CdIpm* pDoc, void* pStructReq)
{
  CTreeItem* pItem = NULL;
/*UKS
  ASSERT (pStructReq);
  if (!pStructReq)
    return NULL;
  LPREPLICSERVERDATAMIN lpData = (LPREPLICSERVERDATAMIN)pStructReq;
  CTreeItemData ItemData(lpData, sizeof(REPLICSERVERDATAMIN), -1, GetDisplayName(lpData));
  pItem = new CuTMReplServer(pDoc->GetPTreeGD(), &ItemData);
*/
  return pItem;
}

