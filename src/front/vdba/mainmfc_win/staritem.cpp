/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : StarItem.cpp, Implementation file                                     //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Authors  : Emmanuel Blattes                                                      //
//                                                                                     //
//                                                                                     //
//    Purpose  : Show a tree with node + database + Table/View/Procedure,              //
//               for "Register Table/View/Procedure as link dialog" dialog boxes       //
//                                                                                     //
//  History:
//   26-Mar-2001 (noifr01)
//    (sir 104270) removal of code for managing Ingres/Desktop
****************************************************************************************/

#include "stdafx.h"
#include "mainmfc.h"
#include "mainfrm.h"
#include "staritem.h"
#include "vnitem.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern "C"
{
  #include "main.h"
//  #include "getvnode.h"
  #include "dba.h"
  #include "domdata.h"
  #include "dbadlg1.h"
  #include "dbaginfo.h"
  #include "dbaset.h"     // Oivers
  #include "domdloca.h"
  #include "domdata.h"

#include "resource.h"   // IDM_NonMfc version

#include "monitor.h"    // low-lever interface structures
};

//
// Static utility functions
//

static UINT GetCustomNodeImageId(CTreeItem *pItem)
{
  LPNODEDATAMIN pNodeDataMin = (LPNODEDATAMIN)pItem->GetPTreeItemData()->GetDataPtr();

  // connected ?
  int hNode = GetVirtNodeHandle(pNodeDataMin->NodeName);
  BOOL bConnected = ( hNode!=-1 ? TRUE : FALSE );

  // local ?
  BOOL bLocal = pNodeDataMin->bIsLocal;

  // simplified mode ?
  BOOL bSimplified = pNodeDataMin->isSimplifiedModeOK;

  // if simplified mode : private or global ?
  BOOL bPrivate = TRUE;
  if (bSimplified) {
    ASSERT (pNodeDataMin->inbConnectData == 1);
    ASSERT (pNodeDataMin->inbLoginData == 1);
  //  ASSERT (pNodeDataMin->LoginDta.bPrivate == pNodeDataMin->ConnectDta.bPrivate); // OBSOLETE
    bPrivate = pNodeDataMin->ConnectDta.bPrivate;
  }

  //
  // Ten different states to manage !
  //
  if (bLocal)
    if (bConnected)
      return IDB_NODE_CONNECTED_LOCAL;
    else
      return IDB_NODE_NOTCONNECTED_LOCAL;
  else
    if (bSimplified)
      if (bPrivate)
        if (bConnected)
          return IDB_NODE_CONNECTED_PRIVATE;
        else
          return IDB_NODE_NOTCONNECTED_PRIVATE;
      else
        if (bConnected)
          return IDB_NODE_CONNECTED_GLOBAL;
        else
          return IDB_NODE_NOTCONNECTED_GLOBAL;
    else
      if (bConnected)
        return IDB_NODE_CONNECTED_COMPLEX;
      else
        return IDB_NODE_NOTCONNECTED_COMPLEX;

  // forgotten cases will assert
  ASSERT (FALSE);
}


static UINT GetDatabaseImageId(CTreeItem *pItem)
{
  LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);

  int databaseType = getint(pStar->szComplim+STEPSMALLOBJ);
  switch (databaseType) {
    case DBTYPE_REGULAR    :
      return IDB_STAR_DATABASE_NOTSTAR;
    case DBTYPE_DISTRIBUTED:
      return IDB_STAR_DATABASE_DDB;
    case DBTYPE_COORDINATOR:
      return IDB_STAR_DATABASE_CDB;
  }
  ASSERT (FALSE);   // case not managed!
  return IDB_STAR_DATABASE_NOTSTAR;
}


// Special variables for push/pop oivers
static int stackedOIVers;
static int stack_count = 0;
static void PushIngresVersionAndType()
{
  ASSERT (stack_count == 0);
  stack_count++;
  stackedOIVers = GetOIVers();
}
static void PopIngresVersionAndType()
{
  ASSERT (stack_count == 1);
  stack_count--;
  SetOIVers(stackedOIVers);
}

//
// Classes Serialization declarations
//

// tree for Tables
IMPLEMENT_SERIAL (CuTreeStar4TblServer, CTreeItemStar, 2)
IMPLEMENT_SERIAL (CuTreeStar4TblDatabase, CTreeItemStar, 1)
IMPLEMENT_SERIAL (CuTreeStar4TblTable, CTreeItemStar, 1)

// tree for Views
IMPLEMENT_SERIAL (CuTreeStar4ViewServer, CTreeItemStar, 2)
IMPLEMENT_SERIAL (CuTreeStar4ViewDatabase, CTreeItemStar, 1)
IMPLEMENT_SERIAL (CuTreeStar4ViewView, CTreeItemStar, 1)

// tree for Procedures
IMPLEMENT_SERIAL (CuTreeStar4ProcServer, CTreeItemStar, 2)
IMPLEMENT_SERIAL (CuTreeStar4ProcDatabase, CTreeItemStar, 1)
IMPLEMENT_SERIAL (CuTreeStar4ProcProc, CTreeItemStar, 1)

// tree for "Local Table" when create table at star level
IMPLEMENT_SERIAL (CuTreeStar4LocalTblServer, CTreeItemStar, 1)
IMPLEMENT_SERIAL (CuTreeStar4LocalTblDatabase, CTreeItemStar, 1)

//
// Classes Implementation for TABLES
//

CuTreeStar4TblServer::CuTreeStar4TblServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_NODE, SUBBRANCH_KIND_MIX, FALSE, pItemData, OT_DATABASE, IDS_STAR_DATABASE_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE);
}

BOOL CuTreeStar4TblServer::CreateStaticSubBranches (HTREEITEM hParentItem)
{
  CTreeGlobalData* pGlobalData = GetPTreeGlobData();

  CuTreeStar4TblSvrClassStatic* pBranch1 = new CuTreeStar4TblSvrClassStatic(pGlobalData, GetPTreeItemData());
  pBranch1->CreateTreeLine (hParentItem);

  return TRUE;
}

CTreeItem* CuTreeStar4TblServer::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4TblDatabase* pItem = new CuTreeStar4TblDatabase (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeStar4TblServer::CuTreeStar4TblServer()
{

}


UINT CuTreeStar4TblServer::GetCustomImageId()
{
  return GetCustomNodeImageId(this);
}



void CuTreeStar4TblServer::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_NODE);
  LPNODEDATAMIN lpNodeData = (LPNODEDATAMIN)pstructParent;

  PushIngresVersionAndType();
  if (!CheckConnection(lpNodeData->NodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }
  hNode = OpenNodeStruct(lpNodeData->NodeName);
}

void CuTreeStar4TblServer::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}

CString CuTreeStar4TblServer::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_DATABASE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);
  CString csName = pStar->objName;
  return csName;
}

CuTreeStar4TblDatabase::CuTreeStar4TblDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_DATABASE, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_TABLE, IDS_STAR_TABLE_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_STAR_DATABASE_NOTSTAR);
}


CTreeItem* CuTreeStar4TblDatabase::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4TblTable* pItem = new CuTreeStar4TblTable (GetPTreeGlobData(), pItemData);
    return pItem;
}


CuTreeStar4TblDatabase::CuTreeStar4TblDatabase()
{

}


UINT CuTreeStar4TblDatabase::GetCustomImageId()
{
  return GetDatabaseImageId(this);
}


void CuTreeStar4TblDatabase::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_DATABASE);

  LPSTARITEM pStar = (LPSTARITEM)GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);

  PushIngresVersionAndType();
  if (!CheckConnection(pStar->NodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }

  // Open node 
  hNode = OpenNodeStruct(pStar->NodeName);  // NodeName may contain ServerClass name
  ASSERT (hNode != -1);

  // Reload list of databases in cache
  LPUCHAR aParents[3];
  aParents[0] = aParents[1] = aParents[2] = NULL;
  char bufColName[MAXOBJECTNAME];
  char bufOwnName[MAXOBJECTNAME];
  char bufComplim[MAXOBJECTNAME];
  int retval = DOMGetFirstObject(hNode,
                                 OT_DATABASE,
                                 0,
                                 aParents,
                                 NULL,       // No filter on owner
                                 FALSE,      // Not with system - does not care here
                                 (LPUCHAR)bufColName,
                                 (LPUCHAR)bufOwnName,
                                 (LPUCHAR)bufComplim);
  ASSERT (retval == RES_SUCCESS);
  return;

  /* OLD CODE, OBSOLETE: RELIED ON TREE ORGANISATION AND CALLED FindItemInTree()
  // Use tree organization to access node information
  CTreeGlobalData *pGD  = GetPTreeGlobData();
  ASSERT (pGD);
  CTreeCtrl *pTreeCtrl = pGD->GetPTree();
  ASSERT (pTreeCtrl);

  // find the hItem in the tree starting from itemdata
  // ( may be different from current selection in the tree!)
  HTREEITEM hItem;
  hItem = FindItemInTree(this);

  // access it's parent item
  HTREEITEM hParentItem;
  hParentItem = pTreeCtrl->GetParentItem(hItem);
  ASSERT (hParentItem);
  CTreeItem *pParentItem = (CTreeItem *)pTreeCtrl->GetItemData(hParentItem);
  ASSERT (pParentItem);

  // pick parent node data
  ASSERT (pParentItem->IsKindOf(RUNTIME_CLASS(CuTreeStar4TblServer)));
  LPNODEDATAMIN lpNodeData = (LPNODEDATAMIN)pParentItem->GetPTreeItemData()->GetDataPtr();

  // Open node 
  hNode = OpenNodeStruct(lpNodeData->NodeName);

  // MANDATORY: Reload cache with databases list in case it was flushed
  int   reqSize             = GetMonInfoStructSize(OT_DATABASE);
  void *pStructReq          = new char[reqSize];
  memset (pStructReq, 0, reqSize);
  int res = GetFirstMonInfo(hNode,          // hNode
                            OT_NODE,        // iobjecttypeParent,
                            lpNodeData,     // pstructParent,
                            OT_DATABASE,    // iobjecttypeReq,
                            pStructReq,
                            NULL);
  ASSERT (res == RES_SUCCESS);
  delete pStructReq;
  */
}

void CuTreeStar4TblDatabase::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}


CString CuTreeStar4TblDatabase::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_TABLE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);

  // objName may contain schema.objname
  UCHAR tampoo[MAXOBJECTNAME];
  StringWithOwner(pStar->objName, pStar->ownerName, tampoo);
  CString csName = tampoo;
  return csName;
}


CuTreeStar4TblTable::CuTreeStar4TblTable (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_TABLE, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_STAR_TABLE_NOTSTAR);
}


CuTreeStar4TblTable::CuTreeStar4TblTable()
{

}


UINT CuTreeStar4TblTable::GetCustomImageId()
{
  // Since ddb objects not displayed, always return the same image id
  return IDB_STAR_TABLE_NOTSTAR;

  /* if ddb objects displayed; unmask the following code:
  LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);
  switch (getint(pStar->szComplim+STEPSMALLOBJ)) {
    case OBJTYPE_NOTSTAR:
      return IDB_STAR_TABLE_NOTSTAR;
    case OBJTYPE_STARNATIVE:
      return IDB_STAR_TABLE_NATIVE;
    case OBJTYPE_STARLINK:
      return IDB_STAR_TABLE_LINK;
  }
  */
}


//
// Classes Implementation for VIEWS
//

CuTreeStar4ViewServer::CuTreeStar4ViewServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_NODE, SUBBRANCH_KIND_MIX, FALSE, pItemData, OT_DATABASE, IDS_STAR_DATABASE_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE);
}

BOOL CuTreeStar4ViewServer::CreateStaticSubBranches (HTREEITEM hParentItem)
{
  CTreeGlobalData* pGlobalData = GetPTreeGlobData();

  CuTreeStar4ViewSvrClassStatic* pBranch1 = new CuTreeStar4ViewSvrClassStatic(pGlobalData, GetPTreeItemData());
  pBranch1->CreateTreeLine (hParentItem);

  return TRUE;
}

CTreeItem* CuTreeStar4ViewServer::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4ViewDatabase* pItem = new CuTreeStar4ViewDatabase (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeStar4ViewServer::CuTreeStar4ViewServer()
{

}


UINT CuTreeStar4ViewServer::GetCustomImageId()
{
  return GetCustomNodeImageId(this);
}


void CuTreeStar4ViewServer::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_NODE);
  LPNODEDATAMIN lpNodeData = (LPNODEDATAMIN)pstructParent;

  PushIngresVersionAndType();
  if (!CheckConnection(lpNodeData->NodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }
  hNode = OpenNodeStruct(lpNodeData->NodeName);
}

void CuTreeStar4ViewServer::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}

CString CuTreeStar4ViewServer::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_DATABASE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);
  CString csName = pStar->objName;
  return csName;
}


CuTreeStar4ViewDatabase::CuTreeStar4ViewDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_DATABASE, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_VIEW, IDS_STAR_VIEW_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_STAR_DATABASE_NOTSTAR);
}


CTreeItem* CuTreeStar4ViewDatabase::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4ViewView* pItem = new CuTreeStar4ViewView (GetPTreeGlobData(), pItemData);
    return pItem;
}


CuTreeStar4ViewDatabase::CuTreeStar4ViewDatabase()
{

}


UINT CuTreeStar4ViewDatabase::GetCustomImageId()
{
  return GetDatabaseImageId(this);
}


void CuTreeStar4ViewDatabase::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_DATABASE);

  LPSTARITEM pStar = (LPSTARITEM)GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);

  PushIngresVersionAndType();
  if (!CheckConnection(pStar->NodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }

  // Open node 
  hNode = OpenNodeStruct(pStar->NodeName);
  ASSERT (hNode != -1);

  // Reload list of databases in cache
  LPUCHAR aParents[3];
  aParents[0] = aParents[1] = aParents[2] = NULL;
  char bufColName[MAXOBJECTNAME];
  char bufOwnName[MAXOBJECTNAME];
  char bufComplim[MAXOBJECTNAME];
  int retval = DOMGetFirstObject(hNode,
                                 OT_DATABASE,
                                 0,
                                 aParents,
                                 NULL,       // No filter on owner
                                 FALSE,      // Not with system - does not care here
                                 (LPUCHAR)bufColName,
                                 (LPUCHAR)bufOwnName,
                                 (LPUCHAR)bufComplim);
  ASSERT (retval == RES_SUCCESS);
  return;

}

void CuTreeStar4ViewDatabase::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}

CString CuTreeStar4ViewDatabase::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_VIEW);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);

  // objName may contain schema.objname
  UCHAR tampoo[MAXOBJECTNAME];
  StringWithOwner(pStar->objName, pStar->ownerName, tampoo);
  CString csName = tampoo;
  return csName;
}


CuTreeStar4ViewView::CuTreeStar4ViewView (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_VIEW, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_STAR_VIEW_NOTSTAR);
}


CuTreeStar4ViewView::CuTreeStar4ViewView()
{

}


UINT CuTreeStar4ViewView::GetCustomImageId()
{
  // Since ddb objects not displayed, always return the same image id
  return IDB_STAR_VIEW_NOTSTAR;

  /* if ddb objects displayed; unmask the following code:
  LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);
  switch (getint(pStar->szComplim+4+STEPSMALLOBJ)) {
    case OBJTYPE_NOTSTAR:
      return IDB_STAR_VIEW_NOTSTAR;
    case OBJTYPE_STARNATIVE:
      return IDB_STAR_VIEW_NATIVE;
    case OBJTYPE_STARLINK:
      return IDB_STAR_VIEW_LINK;
  }
  */
}

//
// Classes Implementation for PROCEDURES
//

CuTreeStar4ProcServer::CuTreeStar4ProcServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_NODE, SUBBRANCH_KIND_MIX, FALSE, pItemData, OT_DATABASE, IDS_STAR_DATABASE_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE);
}

BOOL CuTreeStar4ProcServer::CreateStaticSubBranches (HTREEITEM hParentItem)
{
  CTreeGlobalData* pGlobalData = GetPTreeGlobData();

  CuTreeStar4ProcSvrClassStatic* pBranch1 = new CuTreeStar4ProcSvrClassStatic(pGlobalData, GetPTreeItemData());
  pBranch1->CreateTreeLine (hParentItem);

  return TRUE;
}

CTreeItem* CuTreeStar4ProcServer::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4ProcDatabase* pItem = new CuTreeStar4ProcDatabase (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeStar4ProcServer::CuTreeStar4ProcServer()
{

}


UINT CuTreeStar4ProcServer::GetCustomImageId()
{
  return GetCustomNodeImageId(this);
}


void CuTreeStar4ProcServer::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_NODE);
  LPNODEDATAMIN lpNodeData = (LPNODEDATAMIN)pstructParent;

  PushIngresVersionAndType();
  if (!CheckConnection(lpNodeData->NodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }
  hNode = OpenNodeStruct(lpNodeData->NodeName);
}

void CuTreeStar4ProcServer::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}

CString CuTreeStar4ProcServer::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_DATABASE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);
  CString csName = pStar->objName;
  return csName;
}


CuTreeStar4ProcDatabase::CuTreeStar4ProcDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_DATABASE, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_PROCEDURE, IDS_STAR_PROCEDURE_NONE){
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_STAR_DATABASE_NOTSTAR);
}


CTreeItem* CuTreeStar4ProcDatabase::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4ProcProc* pItem = new CuTreeStar4ProcProc (GetPTreeGlobData(), pItemData);
    return pItem;
}


CuTreeStar4ProcDatabase::CuTreeStar4ProcDatabase()
{

}


UINT CuTreeStar4ProcDatabase::GetCustomImageId()
{
  return GetDatabaseImageId(this);
}


void CuTreeStar4ProcDatabase::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_DATABASE);

  LPSTARITEM pStar = (LPSTARITEM)GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);

  PushIngresVersionAndType();
  if (!CheckConnection(pStar->NodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }

  // Open node 
  hNode = OpenNodeStruct(pStar->NodeName);
  ASSERT (hNode != -1);

  // Reload list of databases in cache
  LPUCHAR aParents[3];
  aParents[0] = aParents[1] = aParents[2] = NULL;
  char bufColName[MAXOBJECTNAME];
  char bufOwnName[MAXOBJECTNAME];
  char bufComplim[MAXOBJECTNAME];
  int retval = DOMGetFirstObject(hNode,
                                 OT_DATABASE,
                                 0,
                                 aParents,
                                 NULL,       // No filter on owner
                                 FALSE,      // Not with system - does not care here
                                 (LPUCHAR)bufColName,
                                 (LPUCHAR)bufOwnName,
                                 (LPUCHAR)bufComplim);
  ASSERT (retval == RES_SUCCESS);
  return;

}

void CuTreeStar4ProcDatabase::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}

CString CuTreeStar4ProcDatabase::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_PROCEDURE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);

  // objName may contain schema.objname
  UCHAR tampoo[MAXOBJECTNAME];
  StringWithOwner(pStar->objName, pStar->ownerName, tampoo);
  CString csName = tampoo;
  return csName;
}


CuTreeStar4ProcProc::CuTreeStar4ProcProc (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_PROCEDURE, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_STAR_PROCEDURE_NOTSTAR);
}


CuTreeStar4ProcProc::CuTreeStar4ProcProc()
{

}


UINT CuTreeStar4ProcProc::GetCustomImageId()
{
  // Since ddb objects not displayed, always return the same image id
  return IDB_STAR_PROCEDURE_NOTSTAR;

  /* if ddb objects displayed; unmask the following code:
  LPSTARITEM pStar = (LPSTARITEM)pItem->GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);
  if (pStar->parentDbType == DBTYPE_DISTRIBUTED)
    return IDB_STAR_PROCEDURE_STAR;
  else
    return IDB_STAR_PROCEDURE_NOTSTAR;
  */
}

//
// Classes Implementation for "Local Table" when create table at star level
//


CuTreeStar4LocalTblServer::CuTreeStar4LocalTblServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_NODE, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_DATABASE, IDS_STAR_DATABASE_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_NODE);
}


CTreeItem* CuTreeStar4LocalTblServer::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4LocalTblDatabase* pItem = new CuTreeStar4LocalTblDatabase (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeStar4LocalTblServer::CuTreeStar4LocalTblServer()
{

}


UINT CuTreeStar4LocalTblServer::GetCustomImageId()
{
  return GetCustomNodeImageId(this);
}



void CuTreeStar4LocalTblServer::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_NODE);
  LPNODEDATAMIN lpNodeData = (LPNODEDATAMIN)pstructParent;

  PushIngresVersionAndType();
  if (!CheckConnection(lpNodeData->NodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }
  hNode = OpenNodeStruct(lpNodeData->NodeName);
}

void CuTreeStar4LocalTblServer::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}

CString CuTreeStar4LocalTblServer::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_DATABASE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);
  CString csName = pStar->objName;
  return csName;
}

CuTreeStar4LocalTblDatabase::CuTreeStar4LocalTblDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_DATABASE, SUBBRANCH_KIND_NONE, FALSE, pItemData)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_STAR_DATABASE_NOTSTAR);
}


CuTreeStar4LocalTblDatabase::CuTreeStar4LocalTblDatabase()
{

}


UINT CuTreeStar4LocalTblDatabase::GetCustomImageId()
{
  return GetDatabaseImageId(this);
}


/********* NEW CLASSES AS OF MAY 12, 98, for Server Class branches ***************/

//
// Classes Implementation for Tables server class
//

IMPLEMENT_SERIAL (CuTreeStar4TblSvrClassStatic, CTreeItemStar, 1)
IMPLEMENT_SERIAL (CuTreeStar4TblSvrClass, CTreeItemStar, 1)
IMPLEMENT_SERIAL (CuTreeStar4TblSvrClassDatabase, CTreeItemStar, 1)


CuTreeStar4TblSvrClassStatic::CuTreeStar4TblSvrClassStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_NODE, IDS_SVRCLASS, SUBBRANCH_KIND_OBJ, IDB_SVRCLASS, pItemData, OT_NODE_SERVERCLASS, IDS_SVRCLASS_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

CTreeItem* CuTreeStar4TblSvrClassStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4TblSvrClass* pItem = new CuTreeStar4TblSvrClass(GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeStar4TblSvrClassStatic::CuTreeStar4TblSvrClassStatic()
{

}

void CuTreeStar4TblSvrClassStatic::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_NODE);
  PushIngresVersionAndType();
  hNode = 0;		// Forced
}

void CuTreeStar4TblSvrClassStatic::StarCloseNodeStruct(int &hNode)
{
  PopIngresVersionAndType();
}


CString CuTreeStar4TblSvrClassStatic::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_NODE_SERVERCLASS);
  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)pstruct;
  ASSERT (pServerDataMin);
  CString csName = pServerDataMin->ServerClass;
  return csName;
}



CuTreeStar4TblSvrClass::CuTreeStar4TblSvrClass (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_NODE_SERVERCLASS, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_DATABASE, IDS_STAR_DATABASE_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_SVRCLASS);
}

CTreeItem* CuTreeStar4TblSvrClass::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4TblDatabase* pItem = new CuTreeStar4TblDatabase (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeStar4TblSvrClass::CuTreeStar4TblSvrClass()
{

}


UINT CuTreeStar4TblSvrClass::GetCustomImageId()
{
  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();
  ASSERT (pServerDataMin);

  // connected ?
  BOOL bConnected;
  if (NodeServerConnections((LPUCHAR)(LPCTSTR)BuildFullNodeName(pServerDataMin)) > 0)
    bConnected = TRUE;
  else
    bConnected = FALSE;
  if (bConnected)
    return IDB_SVRCLASS_CONNECTED;
  else
    return IDB_SVRCLASS_NOTCONNECTED;
}



void CuTreeStar4TblSvrClass::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_NODE_SERVERCLASS);
  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)pstructParent;
	CString strNodeName = BuildFullNodeName(pServerDataMin);

  PushIngresVersionAndType();
  if (!CheckConnection((LPUCHAR)(LPCTSTR)strNodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }
  hNode = OpenNodeStruct((LPUCHAR)(LPCTSTR)strNodeName);
}

void CuTreeStar4TblSvrClass::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}

CString CuTreeStar4TblSvrClass::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_DATABASE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);
  CString csName = pStar->objName;
  return csName;
}

CuTreeStar4TblSvrClassDatabase::CuTreeStar4TblSvrClassDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_DATABASE, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_TABLE, IDS_STAR_TABLE_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_STAR_DATABASE_NOTSTAR);
}


CTreeItem* CuTreeStar4TblSvrClassDatabase::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4TblTable* pItem = new CuTreeStar4TblTable (GetPTreeGlobData(), pItemData);
    return pItem;
}


CuTreeStar4TblSvrClassDatabase::CuTreeStar4TblSvrClassDatabase()
{

}


UINT CuTreeStar4TblSvrClassDatabase::GetCustomImageId()
{
  return GetDatabaseImageId(this);
}


void CuTreeStar4TblSvrClassDatabase::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_DATABASE);

  LPSTARITEM pStar = (LPSTARITEM)GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);

  PushIngresVersionAndType();
  if (!CheckConnection(pStar->NodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }

  // Open node 
  hNode = OpenNodeStruct(pStar->NodeName);
  ASSERT (hNode != -1);

  // Reload list of databases in cache
  LPUCHAR aParents[3];
  aParents[0] = aParents[1] = aParents[2] = NULL;
  char bufColName[MAXOBJECTNAME];
  char bufOwnName[MAXOBJECTNAME];
  char bufComplim[MAXOBJECTNAME];
  int retval = DOMGetFirstObject(hNode,
                                 OT_DATABASE,
                                 0,
                                 aParents,
                                 NULL,       // No filter on owner
                                 FALSE,      // Not with system - does not care here
                                 (LPUCHAR)bufColName,
                                 (LPUCHAR)bufOwnName,
                                 (LPUCHAR)bufComplim);
  ASSERT (retval == RES_SUCCESS);
  return;

}

void CuTreeStar4TblSvrClassDatabase::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}


CString CuTreeStar4TblSvrClassDatabase::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_TABLE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);

  // objName may contain schema.objname
  UCHAR tampoo[MAXOBJECTNAME];
  StringWithOwner(pStar->objName, pStar->ownerName, tampoo);
  CString csName = tampoo;
  return csName;
}

//
// Classes Implementation for Views server class
//

IMPLEMENT_SERIAL (CuTreeStar4ViewSvrClassStatic, CTreeItemStar, 1)
IMPLEMENT_SERIAL (CuTreeStar4ViewSvrClass, CTreeItemStar, 1)
IMPLEMENT_SERIAL (CuTreeStar4ViewSvrClassDatabase, CTreeItemStar, 1)


CuTreeStar4ViewSvrClassStatic::CuTreeStar4ViewSvrClassStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_NODE, IDS_SVRCLASS, SUBBRANCH_KIND_OBJ, IDB_SVRCLASS, pItemData, OT_NODE_SERVERCLASS, IDS_SVRCLASS_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

CTreeItem* CuTreeStar4ViewSvrClassStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4ViewSvrClass* pItem = new CuTreeStar4ViewSvrClass(GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeStar4ViewSvrClassStatic::CuTreeStar4ViewSvrClassStatic()
{

}

void CuTreeStar4ViewSvrClassStatic::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_NODE);
  PushIngresVersionAndType();
  hNode = 0;		// Forced
}

void CuTreeStar4ViewSvrClassStatic::StarCloseNodeStruct(int &hNode)
{
  PopIngresVersionAndType();
}


CString CuTreeStar4ViewSvrClassStatic::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_NODE_SERVERCLASS);
  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)pstruct;
  ASSERT (pServerDataMin);
  CString csName = pServerDataMin->ServerClass;
  return csName;
}



CuTreeStar4ViewSvrClass::CuTreeStar4ViewSvrClass (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_NODE_SERVERCLASS, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_DATABASE, IDS_STAR_DATABASE_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_SVRCLASS);
}

CTreeItem* CuTreeStar4ViewSvrClass::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4ViewDatabase* pItem = new CuTreeStar4ViewDatabase (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeStar4ViewSvrClass::CuTreeStar4ViewSvrClass()
{

}


UINT CuTreeStar4ViewSvrClass::GetCustomImageId()
{
  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();
  ASSERT (pServerDataMin);

  // connected ?
  BOOL bConnected;
  if (NodeServerConnections((LPUCHAR)(LPCTSTR)BuildFullNodeName(pServerDataMin)) > 0)
    bConnected = TRUE;
  else
    bConnected = FALSE;
  if (bConnected)
    return IDB_SVRCLASS_CONNECTED;
  else
    return IDB_SVRCLASS_NOTCONNECTED;
}



void CuTreeStar4ViewSvrClass::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_NODE_SERVERCLASS);
  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)pstructParent;
	CString strNodeName = BuildFullNodeName(pServerDataMin);

  PushIngresVersionAndType();
  if (!CheckConnection((LPUCHAR)(LPCTSTR)strNodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }
  hNode = OpenNodeStruct((LPUCHAR)(LPCTSTR)strNodeName);
}

void CuTreeStar4ViewSvrClass::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}

CString CuTreeStar4ViewSvrClass::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_DATABASE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);
  CString csName = pStar->objName;
  return csName;
}

CuTreeStar4ViewSvrClassDatabase::CuTreeStar4ViewSvrClassDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_DATABASE, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_VIEW, IDS_STAR_VIEW_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_STAR_DATABASE_NOTSTAR);
}


CTreeItem* CuTreeStar4ViewSvrClassDatabase::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4ViewView* pItem = new CuTreeStar4ViewView (GetPTreeGlobData(), pItemData);
    return pItem;
}


CuTreeStar4ViewSvrClassDatabase::CuTreeStar4ViewSvrClassDatabase()
{

}


UINT CuTreeStar4ViewSvrClassDatabase::GetCustomImageId()
{
  return GetDatabaseImageId(this);
}


void CuTreeStar4ViewSvrClassDatabase::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_DATABASE);

  LPSTARITEM pStar = (LPSTARITEM)GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);

  PushIngresVersionAndType();
  if (!CheckConnection(pStar->NodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }

  // Open node 
  hNode = OpenNodeStruct(pStar->NodeName);
  ASSERT (hNode != -1);

  // Reload list of databases in cache
  LPUCHAR aParents[3];
  aParents[0] = aParents[1] = aParents[2] = NULL;
  char bufColName[MAXOBJECTNAME];
  char bufOwnName[MAXOBJECTNAME];
  char bufComplim[MAXOBJECTNAME];
  int retval = DOMGetFirstObject(hNode,
                                 OT_DATABASE,
                                 0,
                                 aParents,
                                 NULL,       // No filter on owner
                                 FALSE,      // Not with system - does not care here
                                 (LPUCHAR)bufColName,
                                 (LPUCHAR)bufOwnName,
                                 (LPUCHAR)bufComplim);
  ASSERT (retval == RES_SUCCESS);
  return;

}

void CuTreeStar4ViewSvrClassDatabase::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}


CString CuTreeStar4ViewSvrClassDatabase::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_VIEW);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);

  // objName may contain schema.objname
  UCHAR tampoo[MAXOBJECTNAME];
  StringWithOwner(pStar->objName, pStar->ownerName, tampoo);
  CString csName = tampoo;
  return csName;
}

//
// Classes Implementation for Procedures server class
//

IMPLEMENT_SERIAL (CuTreeStar4ProcSvrClassStatic, CTreeItemStar, 1)
IMPLEMENT_SERIAL (CuTreeStar4ProcSvrClass, CTreeItemStar, 1)
IMPLEMENT_SERIAL (CuTreeStar4ProcSvrClassDatabase, CTreeItemStar, 1)


CuTreeStar4ProcSvrClassStatic::CuTreeStar4ProcSvrClassStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_NODE, IDS_SVRCLASS, SUBBRANCH_KIND_OBJ, IDB_SVRCLASS, pItemData, OT_NODE_SERVERCLASS, IDS_SVRCLASS_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
}

CTreeItem* CuTreeStar4ProcSvrClassStatic::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4ProcSvrClass* pItem = new CuTreeStar4ProcSvrClass(GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeStar4ProcSvrClassStatic::CuTreeStar4ProcSvrClassStatic()
{

}

void CuTreeStar4ProcSvrClassStatic::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_NODE);
  PushIngresVersionAndType();
  hNode = 0;			// Forced
}

void CuTreeStar4ProcSvrClassStatic::StarCloseNodeStruct(int &hNode)
{
  PopIngresVersionAndType();
}


CString CuTreeStar4ProcSvrClassStatic::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_NODE_SERVERCLASS);
  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)pstruct;
  ASSERT (pServerDataMin);
  CString csName = pServerDataMin->ServerClass;
  return csName;
}



CuTreeStar4ProcSvrClass::CuTreeStar4ProcSvrClass (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_NODE_SERVERCLASS, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_DATABASE, IDS_STAR_DATABASE_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_SVRCLASS);
}

CTreeItem* CuTreeStar4ProcSvrClass::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4ProcDatabase* pItem = new CuTreeStar4ProcDatabase (GetPTreeGlobData(), pItemData);
    return pItem;
}

CuTreeStar4ProcSvrClass::CuTreeStar4ProcSvrClass()
{

}


UINT CuTreeStar4ProcSvrClass::GetCustomImageId()
{
  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)GetPTreeItemData()->GetDataPtr();
  ASSERT (pServerDataMin);

  // connected ?
  BOOL bConnected;
  if (NodeServerConnections((LPUCHAR)(LPCTSTR)BuildFullNodeName(pServerDataMin)) > 0)
    bConnected = TRUE;
  else
    bConnected = FALSE;
  if (bConnected)
    return IDB_SVRCLASS_CONNECTED;
  else
    return IDB_SVRCLASS_NOTCONNECTED;
}



void CuTreeStar4ProcSvrClass::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_NODE_SERVERCLASS);
  LPNODESVRCLASSDATAMIN pServerDataMin = (LPNODESVRCLASSDATAMIN)pstructParent;
	CString strNodeName = BuildFullNodeName(pServerDataMin);

  PushIngresVersionAndType();
  if (!CheckConnection((LPUCHAR)(LPCTSTR)strNodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }
  hNode = OpenNodeStruct((LPUCHAR)(LPCTSTR)strNodeName);
}

void CuTreeStar4ProcSvrClass::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}

CString CuTreeStar4ProcSvrClass::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_DATABASE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);
  CString csName = pStar->objName;
  return csName;
}

CuTreeStar4ProcSvrClassDatabase::CuTreeStar4ProcSvrClassDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData)
  :CTreeItemStar (pTreeGlobalData, OT_DATABASE, SUBBRANCH_KIND_OBJ, FALSE, pItemData, OT_PROCEDURE, IDS_STAR_PROCEDURE_NONE)
{
  //SetContextMenuId(IDR_POPUP_VIRTUALNODEBAR);
  InitializeImageId(IDB_STAR_DATABASE_NOTSTAR);
}


CTreeItem* CuTreeStar4ProcSvrClassDatabase::AllocateChildItem (CTreeItemData* pItemData)
{
    CuTreeStar4ProcProc* pItem = new CuTreeStar4ProcProc (GetPTreeGlobData(), pItemData);
    return pItem;
}


CuTreeStar4ProcSvrClassDatabase::CuTreeStar4ProcSvrClassDatabase()
{

}


UINT CuTreeStar4ProcSvrClassDatabase::GetCustomImageId()
{
  return GetDatabaseImageId(this);
}


void CuTreeStar4ProcSvrClassDatabase::StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode)
{
  ASSERT (iobjecttypeParent == OT_DATABASE);

  LPSTARITEM pStar = (LPSTARITEM)GetPTreeItemData()->GetDataPtr();
  ASSERT (pStar);

  PushIngresVersionAndType();
  if (!CheckConnection(pStar->NodeName, FALSE,FALSE)) {
    // ASSERT (FALSE); // acceptable : node does not respond
    hNode = -1;
    return;
  }

  // Open node 
  hNode = OpenNodeStruct(pStar->NodeName);
  ASSERT (hNode != -1);

  // Reload list of databases in cache
  LPUCHAR aParents[3];
  aParents[0] = aParents[1] = aParents[2] = NULL;
  char bufColName[MAXOBJECTNAME];
  char bufOwnName[MAXOBJECTNAME];
  char bufComplim[MAXOBJECTNAME];
  int retval = DOMGetFirstObject(hNode,
                                 OT_DATABASE,
                                 0,
                                 aParents,
                                 NULL,       // No filter on owner
                                 FALSE,      // Not with system - does not care here
                                 (LPUCHAR)bufColName,
                                 (LPUCHAR)bufOwnName,
                                 (LPUCHAR)bufComplim);
  ASSERT (retval == RES_SUCCESS);
  return;

}

void CuTreeStar4ProcSvrClassDatabase::StarCloseNodeStruct(int &hNode)
{
  // ASSERT (hNode != -1);  // acceptable : node did not respond in StarOpenNodeStruct()
  if (hNode != -1)
    CloseNodeStruct(hNode, TRUE);   // keep entry
  PopIngresVersionAndType();
}


CString CuTreeStar4ProcSvrClassDatabase::MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct)
{
  ASSERT(iobjecttypeReq == OT_PROCEDURE);
  LPSTARITEM pStar = (LPSTARITEM)pstruct;
  ASSERT (pStar);

  // objName may contain schema.objname
  UCHAR tampoo[MAXOBJECTNAME];
  StringWithOwner(pStar->objName, pStar->ownerName, tampoo);
  CString csName = tampoo;
  return csName;
}



