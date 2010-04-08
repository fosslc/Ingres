/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : rpanedrg.cpp                                                          //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
//    Right Pane Drag Drop of an item to the dom                                       //
//                                                                                     //
****************************************************************************************/

#define OEMRESOURCE 
#include "stdafx.h"
#include "rpanedrg.h"

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
  #include "treelb.e"
}

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_SERIAL (CuDragFromRPane, CObject, 1)

CuDragFromRPane::CuDragFromRPane()
{
  m_bDragging = FALSE;
  m_dragImageList = NULL;
  m_bCanDrop  = FALSE;
  m_hwndVdbaDoc = 0;
  memset (&m_rightPaneRecord, 0, sizeof(TREERECORD));
  m_bValidRecord = FALSE;
}

CuDragFromRPane::~CuDragFromRPane()
{
}

void CuDragFromRPane::Serialize (CArchive& ar)
{
  // No serialization planned yet!
  ASSERT (FALSE);

  CObject::Serialize(ar);

  if (ar.IsStoring()) {
  }
  else {
  }
}

BOOL CuDragFromRPane::CheckPRPaneConsistency(CWnd* pRPane)
{
#ifdef _DEBUG
  CWnd* pParent  = pRPane->GetParent();     // ---> pParent:  CTabCtrl
  CWnd* pParent2 = pParent->GetParent();    // ---> pParent2: CuDlgDomTabCtrl
  CWnd* pParent3 = pParent2->GetParent();   // ---> pParent3: CMainMfcView2
  CWnd* pParent4 = pParent3->GetParent();   // ---> pParent4: CSplitterWnd
  CWnd* pParent5 = pParent4->GetParent();   // ---> pParent5: CChildFrame
  HWND hwndDoc = pParent5->m_hWnd;
  BOOL bConsistent = (GetVdbaDocumentHandle(hwndDoc) == m_hwndVdbaDoc);
  return bConsistent;
#else
  return TRUE;    // Not checked in release version
#endif
}

BOOL CuDragFromRPane::StartDrag(CWnd* pRPane, NM_LISTVIEW* pNMListView, CListCtrl* pListCtrl, int objType, LPCTSTR objName, LPCTSTR objOwner)
{
  // Check consistency of received parameters
  if (HasOwner(objType))
    ASSERT (objOwner[0]);

  // reset some obsolete member variables
  m_hwndVdbaDoc = 0;
  memset (&m_rightPaneRecord, 0, sizeof(TREERECORD));
  m_bValidRecord = FALSE;

  // calculate and store the "C" document handle
  CWnd* pParent  = pRPane->GetParent();     // ---> pParent:  CTabCtrl
  CWnd* pParent2 = pParent->GetParent();    // ---> pParent2: CuDlgDomTabCtrl
  CWnd* pParent3 = pParent2->GetParent();   // ---> pParent3: CMainMfcView2
  CWnd* pParent4 = pParent3->GetParent();   // ---> pParent4: CSplitterWnd
  CWnd* pParent5 = pParent4->GetParent();   // ---> pParent5: CChildFrame
  HWND  hwndDoc = pParent5->m_hWnd;
  m_hwndVdbaDoc = GetVdbaDocumentHandle(hwndDoc);

  BOOL bSuccess = ManageDragDropStartFromRPane(GetVdbaDocumentHandle(m_hwndVdbaDoc),
                                               objType,
                                               (LPUCHAR)objName,
                                               (LPUCHAR)objOwner);
  if (!bSuccess)
    return FALSE;// no dragdrop (probably system object)

  ASSERT (!m_dragImageList);

  m_bDragging = TRUE;
  m_dragImageList = pListCtrl->CreateDragImage(pNMListView->iItem, &pNMListView->ptAction);
  ASSERT (m_dragImageList);
  if (!m_dragImageList) {
    MessageBeep(MB_ICONEXCLAMATION);
    m_bDragging = FALSE;
    return FALSE;
  }

  CPoint gHotSpot(0, 0);
  bSuccess = m_dragImageList->BeginDrag(0, gHotSpot);
  ASSERT (bSuccess);
  if (!bSuccess) {
    MessageBeep(MB_ICONEXCLAMATION);
    delete m_dragImageList;
    m_dragImageList = NULL;
    m_bDragging = FALSE;
    return FALSE;
  }

  // Store partial data in record
  m_rightPaneRecord.recType = objType;
  x_strcpy((char*)m_rightPaneRecord.objName,    (LPCSTR)objName);
  x_strcpy((char*)m_rightPaneRecord.ownerName,  (LPCSTR)objOwner);

  //TRACE0 ("CuDragFromRPane: BeginDrag()...\n");

  CWnd* pRefWnd = AfxGetMainWnd();
  POINT pt;
  GetCursorPos(&pt);
  CPoint ptMouse(pt);

  BOOL bSuccess2 = m_dragImageList->DragEnter(pRefWnd, ptMouse);
  ASSERT (bSuccess2);

  m_bCanDrop = TRUE;
  pRPane->SetCapture();     // Redirect mouse messages to current property page
  int iret = ShowCursor(FALSE);
  ASSERT (iret == -1);

  return TRUE;
}


LPTREERECORD CuDragFromRPane::GetCurTreeRec(CWnd* pRPane)
{
  ASSERT (CheckPRPaneConsistency(pRPane));
  ASSERT (m_hwndVdbaDoc);
  LPDOMDATA lpDomData = (LPDOMDATA)GetWindowLong(m_hwndVdbaDoc, 0);
  ASSERT (lpDomData);
  DWORD dwCurSel = GetCurSel(lpDomData);
  ASSERT (dwCurSel);
  LPTREERECORD lpRecord = (LPTREERECORD) ::SendMessage(lpDomData->hwndTreeLb,
                                                 LM_GETITEMDATA, 0, (LPARAM)dwCurSel);
  ASSERT (lpRecord);
  return lpRecord;
}

BOOL CuDragFromRPane::DragStoreObjInfo(CWnd* pRPane, long lComplimValue, LPCSTR lpParent0, LPCSTR lpParent1, LPCSTR lpOwner1, LPCSTR lpParent2)
{
  ASSERT (CheckPRPaneConsistency(pRPane));
  ASSERT (!m_bValidRecord);

  // Check consistency of received parameters
  int parentLevel = GetParentLevelFromObjType(m_rightPaneRecord.recType);
  switch (parentLevel) {
    case 1:
      ASSERT (lpParent0[0]);
      break;
    case 2:
      ASSERT (lpParent0[0]);
      ASSERT (lpParent1[0]);
      break;
    case 3:
      ASSERT (lpParent0[0]);
      ASSERT (lpParent1[0]);
      ASSERT (lpParent2[0]);
      break;
  }

  // Store received information
  m_rightPaneRecord.complimValue = lComplimValue;
  x_strcpy((char*)m_rightPaneRecord.extra,      lpParent0);
  x_strcpy((char*)m_rightPaneRecord.extra2,     lpParent1);
  x_strcpy((char*)m_rightPaneRecord.tableOwner, lpOwner1);
  x_strcpy((char*)m_rightPaneRecord.extra3,     lpParent2);

  m_bValidRecord = TRUE;
  return TRUE;
}


void CuDragFromRPane::DragMouseMove(CWnd* pRPane, CPoint point)
{
  ASSERT (CheckPRPaneConsistency(pRPane));
  ASSERT (m_bValidRecord);

  ASSERT (m_bDragging);
  if (m_bDragging) {
    //TRACE0 ("CuDragFromRPane: DragMove()...\n");

    CWnd* pRefWnd = AfxGetMainWnd();
    CRect rcMainWnd;
    pRefWnd->GetWindowRect(rcMainWnd);

    POINT pt;
    GetCursorPos(&pt);
    CPoint ptMouse(pt);
    ptMouse -= rcMainWnd.TopLeft();

    BOOL bCanDropNow = TRUE;

    if (!RPaneDragDropMousemove(m_hwndVdbaDoc, pt.x, pt.y))
      bCanDropNow = FALSE;

    // 4 cases
    if (bCanDropNow) {
      if (m_bCanDrop) {
        // still drop possible
        //TRACE0 ("Still drop possible\n");
        BOOL bSuccess = m_dragImageList->DragMove(ptMouse);
        ASSERT (bSuccess);
      }
      else {
        // first time we can drop
        //TRACE0 ("first time we can drop\n");
        int iret = ShowCursor(FALSE);
        ASSERT (iret == -1);
        BOOL bSuccess = m_dragImageList->DragEnter(pRefWnd, ptMouse);
        ASSERT (bSuccess);
      }
    }
    else {
      if (m_bCanDrop) {
        // cannot drop anymore
        //TRACE0 ("cannot drop anymore\n");
        BOOL bSuccess = m_dragImageList->DragLeave(pRefWnd);
        ASSERT (bSuccess);
        SetCursor(AfxGetApp()->LoadOEMCursor(OCR_NO));
        int iret = ShowCursor(TRUE);
        ASSERT (iret == 0);
      }
      else {
        // still cannot drop
        // Nothing to do - let cursor flow
        //TRACE0 ("Still cannot drop\n");
      }
    }

    // store for next time
    m_bCanDrop = bCanDropNow;
  }
}

BOOL CuDragFromRPane::EndDrag(CWnd* pRPane)
{
  ASSERT (CheckPRPaneConsistency(pRPane));
  ASSERT (m_bValidRecord);

  ASSERT (m_bDragging);
  if (m_bDragging) {
    //TRACE0 ("CuDragFromRPane: EndDrag()...\n");

    m_dragImageList->EndDrag();
    m_bDragging = FALSE;
    delete m_dragImageList;
    m_dragImageList = NULL;

    ReleaseCapture();
    SetCursor(AfxGetApp()->LoadStandardCursor(IDC_ARROW));

    // restore cursor only if currently hidden
    if (m_bCanDrop) {
      int iret = ShowCursor(TRUE);
      ASSERT (iret == 0);
    }

    // Create object if can drop
    if (m_bCanDrop) {
      POINT pt;
      GetCursorPos(&pt);
      BOOL bSuccess = RPaneDragDropEnd(m_hwndVdbaDoc, pt.x, pt.y, &m_rightPaneRecord);
      return bSuccess;
    }
    else
      return TRUE;    // cannot drop : as if successful
  }
  return FALSE;
}