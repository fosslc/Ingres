/****************************************************************************************
//                                                                                     //
//  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       //
//                                                                                     //
//    Source   : StarItem.h, header file                                               //
//                                                                                     //
//                                                                                     //
//    Project  : CA-OpenIngres/Monitoring.                                             //
//    Authors  : Emmanuel Blattes                                                      //
//                                                                                     //
//                                                                                     //
//    Purpose  : Show a tree with node + database + Table/View/Procedure,              //
//               for "Register Table/View/Procedure as link dialog" dialog boxes       //
//               Also, show a tree with node + database for "local table"              //
//               when creating a table at star level                                   //
//                                                                                     //
****************************************************************************************/

#ifndef STARTREE_HEADER
#define STARTREE_HEADER

#include "vtree3.h"

/***************************************************
class CuTreeStar4TblServerStatic: public CTreeItemStar
{
public:
    CuTreeStar4TblServerStatic (CTreeGlobalData* pTreeGlobalData);
    virtual ~CuTreeStar4TblServerStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
    CuTreeStar4TblServerStatic();
    DECLARE_SERIAL (CuTreeStar4TblServerStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);
};
***************************************************/

//
//  Tree branches for TABLE register as link
//
class CuTreeStar4TblServer: public CTreeItemStar
{
public:
    CuTreeStar4TblServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4TblServer(){};

    // Mix type ---> Need Both AllocateChildItem and CreateStaticSubBranches
    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
    BOOL CreateStaticSubBranches (HTREEITEM hParentItem);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4TblServer();
    DECLARE_SERIAL (CuTreeStar4TblServer)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4TblDatabase: public CTreeItemStar
{
public:
    CuTreeStar4TblDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4TblDatabase(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4TblDatabase();
    DECLARE_SERIAL (CuTreeStar4TblDatabase)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4TblTable: public CTreeItemStar
{
public:
    CuTreeStar4TblTable (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4TblTable(){};

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4TblTable();
    DECLARE_SERIAL (CuTreeStar4TblTable)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }
};

class CuTreeStar4TblSvrClassStatic: public CTreeItemStar
{
public:
    CuTreeStar4TblSvrClassStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4TblSvrClassStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
    CuTreeStar4TblSvrClassStatic();
    DECLARE_SERIAL (CuTreeStar4TblSvrClassStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4TblSvrClass: public CTreeItemStar
{
public:
    CuTreeStar4TblSvrClass (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4TblSvrClass(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4TblSvrClass();
    DECLARE_SERIAL (CuTreeStar4TblSvrClass)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4TblSvrClassDatabase: public CTreeItemStar
{
public:
    CuTreeStar4TblSvrClassDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4TblSvrClassDatabase(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4TblSvrClassDatabase();
    DECLARE_SERIAL (CuTreeStar4TblSvrClassDatabase)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};


//
//  Tree branches for VIEW register as link
//
class CuTreeStar4ViewServer: public CTreeItemStar
{
public:
    CuTreeStar4ViewServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ViewServer(){};

    // Mix type ---> Need Both AllocateChildItem and CreateStaticSubBranches
    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
    BOOL CreateStaticSubBranches (HTREEITEM hParentItem);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4ViewServer();
    DECLARE_SERIAL (CuTreeStar4ViewServer)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4ViewDatabase: public CTreeItemStar
{
public:
    CuTreeStar4ViewDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ViewDatabase(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4ViewDatabase();
    DECLARE_SERIAL (CuTreeStar4ViewDatabase)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4ViewView: public CTreeItemStar
{
public:
    CuTreeStar4ViewView (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ViewView(){};

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4ViewView();
    DECLARE_SERIAL (CuTreeStar4ViewView)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }
};

class CuTreeStar4ViewSvrClassStatic: public CTreeItemStar
{
public:
    CuTreeStar4ViewSvrClassStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ViewSvrClassStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
    CuTreeStar4ViewSvrClassStatic();
    DECLARE_SERIAL (CuTreeStar4ViewSvrClassStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4ViewSvrClass: public CTreeItemStar
{
public:
    CuTreeStar4ViewSvrClass (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ViewSvrClass(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4ViewSvrClass();
    DECLARE_SERIAL (CuTreeStar4ViewSvrClass)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4ViewSvrClassDatabase: public CTreeItemStar
{
public:
    CuTreeStar4ViewSvrClassDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ViewSvrClassDatabase(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4ViewSvrClassDatabase();
    DECLARE_SERIAL (CuTreeStar4ViewSvrClassDatabase)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};



//
//  Tree branches for PROCEDURE register as link
//
class CuTreeStar4ProcServer: public CTreeItemStar
{
public:
    CuTreeStar4ProcServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ProcServer(){};

    // Mix type ---> Need Both AllocateChildItem and CreateStaticSubBranches
    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
    BOOL CreateStaticSubBranches (HTREEITEM hParentItem);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4ProcServer();
    DECLARE_SERIAL (CuTreeStar4ProcServer)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4ProcDatabase: public CTreeItemStar
{
public:
    CuTreeStar4ProcDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ProcDatabase(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4ProcDatabase();
    DECLARE_SERIAL (CuTreeStar4ProcDatabase)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4ProcProc: public CTreeItemStar
{
public:
    CuTreeStar4ProcProc (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ProcProc(){};

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4ProcProc();
    DECLARE_SERIAL (CuTreeStar4ProcProc)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }
};

class CuTreeStar4ProcSvrClassStatic: public CTreeItemStar
{
public:
    CuTreeStar4ProcSvrClassStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ProcSvrClassStatic(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
    CuTreeStar4ProcSvrClassStatic();
    DECLARE_SERIAL (CuTreeStar4ProcSvrClassStatic)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4ProcSvrClass: public CTreeItemStar
{
public:
    CuTreeStar4ProcSvrClass (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ProcSvrClass(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4ProcSvrClass();
    DECLARE_SERIAL (CuTreeStar4ProcSvrClass)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};

class CuTreeStar4ProcSvrClassDatabase: public CTreeItemStar
{
public:
    CuTreeStar4ProcSvrClassDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4ProcSvrClassDatabase(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4ProcSvrClassDatabase();
    DECLARE_SERIAL (CuTreeStar4ProcSvrClassDatabase)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);
};



//
//  Tree branches for "Local Table" when create table at star level
//
class CuTreeStar4LocalTblServer: public CTreeItemStar
{
public:
    CuTreeStar4LocalTblServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4LocalTblServer(){};

    CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4LocalTblServer();
    DECLARE_SERIAL (CuTreeStar4LocalTblServer)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

  // special methods for multiple nodes in the same tree, for Star
  virtual void StarOpenNodeStruct(int iobjecttypeParent, void *pstructParent, int &hNode);
  virtual void StarCloseNodeStruct(int &hNode);

  // Special method for display name
  virtual CString MakeChildItemDisplayName(int hnodestruct, int iobjecttypeReq, void *pstruct);

  // Specific for star items: do we display Coordinator Databases
  virtual BOOL DoNotShowCDBs() { return FALSE; }

};

class CuTreeStar4LocalTblDatabase: public CTreeItemStar
{
public:
    CuTreeStar4LocalTblDatabase (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
    virtual ~CuTreeStar4LocalTblDatabase(){};

    virtual UINT GetCustomImageId();

protected:
    CuTreeStar4LocalTblDatabase();
    DECLARE_SERIAL (CuTreeStar4LocalTblDatabase)
public:
  void Serialize (CArchive& ar) { CTreeItemStar::Serialize(ar); }

};


#endif  // STARTREE_HEADER
