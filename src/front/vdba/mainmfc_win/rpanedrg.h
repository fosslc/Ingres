/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : rpanedrg.h                                                            //
//                                                                                     //
//    Project  : CA-OpenIngres/VDBA                                                    //
//                                                                                     //
//    Right Pane Drag Drop of an item to the dom                                       //
//                                                                                     //
****************************************************************************************/
#ifndef RPANEDRAG_HEADER
#define RPANEDRAG_HEADER

extern "C" {
  #include "dba.h"
  #include "domdata.h"
  #include "main.h"
  #include "dom.h"
  #include "tree.h"
}

class CuDragFromRPane: public CObject
{
  DECLARE_SERIAL (CuDragFromRPane)

public:
  CuDragFromRPane();
  virtual ~CuDragFromRPane();

// for serialisation
  virtual void Serialize (CArchive& ar);

// methods
  BOOL IsDragging()  { return m_bDragging; }
  void SetCanDropFlag(BOOL bCanDrop) { m_bCanDrop = bCanDrop; }

  BOOL StartDrag(CWnd* pRPane, NM_LISTVIEW* pNMListView, CListCtrl* pListCtrl, int objType, LPCTSTR objName, LPCTSTR objOwner);
  LPTREERECORD GetCurTreeRec(CWnd* pRPane);
  BOOL DragStoreObjInfo(CWnd* pRPane, long lComplimValue, LPCSTR lpParent0 = "", LPCSTR lpParent1 = "", LPCSTR lpOwner1 = "", LPCSTR lpParent2 = "");
  void DragMouseMove(CWnd* pRPane, CPoint point);
  BOOL EndDrag(CWnd* pRPane);

private:
  BOOL CheckPRPaneConsistency(CWnd* pRPane);

private:
  BOOL        m_bDragging;
  CImageList* m_dragImageList;
  BOOL        m_bCanDrop;

  HWND        m_hwndVdbaDoc;
  BOOL        m_bValidRecord;
  TREERECORD  m_rightPaneRecord;
};

#endif  // RPANEDRAG_HEADER

