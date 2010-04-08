/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : montree.cpp : implementation file
**    Project  : INGRES II/ Monitoring.
**    Author   : Emmanuel Blattes
**    Purpose  : Classes for monitoring tree.
**
** History:
**
** 20-Feb-1997 (Emmanuel Blattes)
**    Created
*/


#ifndef MONTREE_HEADER
#define MONTREE_HEADER
#include "vtree.h"


//
// CLASS: CuTMServerStatic
// ************************************************************************************************
class CuTMServerStatic: public CTreeItem
{
public:
	CuTMServerStatic (CTreeGlobalData* pTreeGlobalData);
	virtual ~CuTMServerStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMServerStatic();
	DECLARE_SERIAL (CuTMServerStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMServer
// ************************************************************************************************
class CuTMServer: public CTreeItem
{
public:
	CuTMServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMServer(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CDialog* GetPage (CWnd* pParent);
	virtual UINT GetCustomImageId();
	CuPageInformation* GetPageInformation ();

	// kill/shutdown special
	BOOL IsEnabled(UINT idMenu);
	virtual BOOL KillShutdown();

	// Close - Open special
	virtual BOOL CloseServer();
	virtual BOOL OpenServer();

protected:
	CuTMServer();
	DECLARE_SERIAL (CuTMServer)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMServerNotExpandable
// ************************************************************************************************
// Added Emb 18/06 : same as server, but not expandable,
// and only one tab "detail" on right pane
class CuTMServerNotExpandable: public CTreeItem
{
public:
	CuTMServerNotExpandable (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMServerNotExpandable(){};

	CDialog* GetPage (CWnd* pParent);
	virtual UINT GetCustomImageId();
	CuPageInformation* GetPageInformation ();

	// kill/shutdown special
	BOOL IsEnabled(UINT idMenu);
	virtual BOOL KillShutdown();

protected:
	CuTMServerNotExpandable();
	DECLARE_SERIAL (CuTMServerNotExpandable)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMSessionStatic
// ************************************************************************************************
class CuTMSessionStatic: public CTreeItem
{
public:
	CuTMSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessionStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

	CuPageInformation* GetPageInformation ();

protected:
	CuTMSessionStatic();
	DECLARE_SERIAL (CuTMSessionStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


// CLASS: CuTMSession
// ************************************************************************************************
class CuTMSession: public CTreeItem
{
public:
	CuTMSession (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSession(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT GetCustomImageId();

	// kill/shutdown special
	BOOL IsEnabled(UINT idMenu);
	virtual BOOL KillShutdown();

protected:
	CuTMSession();
	DECLARE_SERIAL (CuTMSession)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

// CLASS: CuTMSession
// ************************************************************************************************
class CuTMSessLocklistStatic: public CTreeItem
{
public:
	CuTMSessLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessLocklistStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMSessLocklistStatic();
	DECLARE_SERIAL (CuTMSessLocklistStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


// CLASS: CuTMLocklistLockStatic
// ************************************************************************************************
class CuTMLocklistLockStatic: public CTreeItem
{
public:
	CuTMLocklistLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLocklistLockStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMLocklistLockStatic();
	DECLARE_SERIAL (CuTMLocklistLockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMLockResourceStatic
// ************************************************************************************************
class CuTMLockResourceStatic: public CTreeItem
{
public:
	CuTMLockResourceStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockResourceStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMLockResourceStatic();
	DECLARE_SERIAL (CuTMLockResourceStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLockResource
// ************************************************************************************************
class CuTMLockResource: public CTreeItem
{
public:
	CuTMLockResource (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockResource(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	virtual UINT GetCustomImageId();
	CuPageInformation* GetPageInformation ();

protected:
	CuTMLockResource();
	DECLARE_SERIAL (CuTMLockResource)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLocklistBllockStatic
// ************************************************************************************************
class CuTMLocklistBllockStatic: public CTreeItem
{
public:
	CuTMLocklistBllockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLocklistBllockStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMLocklistBllockStatic();
	DECLARE_SERIAL (CuTMLocklistBllockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLocklistBllock
// ************************************************************************************************
class CuTMLocklistBllock: public CTreeItem
{
public:
	CuTMLocklistBllock (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLocklistBllock(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	virtual UINT GetCustomImageId();
	CuPageInformation* GetPageInformation ();

protected:
	CuTMLocklistBllock();
	DECLARE_SERIAL (CuTMLocklistBllock)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMSessLockStatic
// ************************************************************************************************
class CuTMSessLockStatic: public CTreeItem
{
public:
	CuTMSessLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessLockStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMSessLockStatic();
	DECLARE_SERIAL (CuTMSessLockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMSessLockStatic
// ************************************************************************************************
class CuTMSessTransacStatic: public CTreeItem
{
public:
	CuTMSessTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessTransacStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMSessTransacStatic();
	DECLARE_SERIAL (CuTMSessTransacStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMSessDbStatic
// ************************************************************************************************
class CuTMSessDbStatic: public CTreeItem
{
public:
	CuTMSessDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessDbStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMSessDbStatic();
	DECLARE_SERIAL (CuTMSessDbStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMSessDb
// ************************************************************************************************
class CuTMSessDb: public CTreeItem
{
public:
	CuTMSessDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessDb(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId();

protected:
	CuTMSessDb();
	DECLARE_SERIAL (CuTMSessDb)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLocklistStatic
// ************************************************************************************************
class CuTMLocklistStatic: public CTreeItem
{
public:
	CuTMLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLocklistStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMLocklistStatic();
	DECLARE_SERIAL (CuTMLocklistStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLocklist
// ************************************************************************************************
class CuTMLocklist: public CTreeItem
{
public:
	CuTMLocklist (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLocklist(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT GetCustomImageId();

protected:
	CuTMLocklist();
	DECLARE_SERIAL (CuTMLocklist)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMLockStatic
// ************************************************************************************************
class CuTMLockStatic: public CTreeItem
{
public:
	CuTMLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMLockStatic();
	DECLARE_SERIAL (CuTMLockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLock
// ************************************************************************************************
class CuTMLock: public CTreeItem
{
public:
	CuTMLock (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLock(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	virtual UINT GetCustomImageId();
	CuPageInformation* GetPageInformation ();

protected:
	CuTMLock();
	DECLARE_SERIAL (CuTMLock)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMTransacStatic
// ************************************************************************************************
class CuTMTransacStatic: public CTreeItem
{
public:
	CuTMTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMTransacStatic();
	DECLARE_SERIAL (CuTMTransacStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransac
// ************************************************************************************************
class CuTMTransac: public CTreeItem
{
public:
	CuTMTransac (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransac(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMTransac();
	DECLARE_SERIAL (CuTMTransac)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbStatic
// ************************************************************************************************
class CuTMDbStatic: public CTreeItem
{
public:
	CuTMDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMDbStatic();
	DECLARE_SERIAL (CuTMDbStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDb
// ************************************************************************************************
class CuTMDb: public CTreeItem
{
public:
	CuTMDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDb(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId();

protected:
	CuTMDb();
	DECLARE_SERIAL (CuTMDb)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};



//
// CLASS: CuTMLoginfoStatic
// ************************************************************************************************
class CuTMLoginfoStatic: public CTreeItem
{
public:
	CuTMLoginfoStatic (CTreeGlobalData* pTreeGlobalData);
	virtual ~CuTMLoginfoStatic(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMLoginfoStatic();
	DECLARE_SERIAL (CuTMLoginfoStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMLogProcessStatic
// ************************************************************************************************
class CuTMLogProcessStatic: public CTreeItem
{
public:
	CuTMLogProcessStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogProcessStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMLogProcessStatic();
	DECLARE_SERIAL (CuTMLogProcessStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLogProcess
// ************************************************************************************************
class CuTMLogProcess: public CTreeItem
{
public:
	CuTMLogProcess (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogProcess(){};

	CuPageInformation* GetPageInformation ();

protected:
	CuTMLogProcess();
	DECLARE_SERIAL (CuTMLogProcess)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMLogDbStatic
// ************************************************************************************************
class CuTMLogDbStatic: public CTreeItem
{
public:
	CuTMLogDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogDbStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMLogDbStatic();
	DECLARE_SERIAL (CuTMLogDbStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLogDb
// ************************************************************************************************
class CuTMLogDb: public CTreeItem
{
public:
	CuTMLogDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogDb(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId();

protected:
	CuTMLogDb();
	DECLARE_SERIAL (CuTMLogDb)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMLogTransacStatic
// ************************************************************************************************
class CuTMLogTransacStatic: public CTreeItem
{
public:
	CuTMLogTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogTransacStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMLogTransacStatic();
	DECLARE_SERIAL (CuTMLogTransacStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMAllDbStatic
// ************************************************************************************************
class CuTMAllDbStatic: public CTreeItem
{
public:
	CuTMAllDbStatic (CTreeGlobalData* pTreeGlobalData);
	virtual ~CuTMAllDbStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMAllDbStatic();
	DECLARE_SERIAL (CuTMAllDbStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMAllDb
// ************************************************************************************************
class CuTMAllDb: public CTreeItem
{
public:
	CuTMAllDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMAllDb(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId();

protected:
	CuTMAllDb();
	DECLARE_SERIAL (CuTMAllDb)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbAllTbl
// ************************************************************************************************
class CuTMDbAllTbl: public CTreeItem
{
public:
	CuTMDbAllTbl (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbAllTbl(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	virtual UINT  GetCustomImageId() { return IDB_TM_LOCK_TYPE_TABLE; }

protected:
	CuTMDbAllTbl();
	DECLARE_SERIAL (CuTMDbAllTbl)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMDbLocklistStatic
// ************************************************************************************************
class CuTMDbLocklistStatic: public CTreeItem
{
public:
	CuTMDbLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbLocklistStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMDbLocklistStatic();
	DECLARE_SERIAL (CuTMDbLocklistStatic)
public:
  void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbLockStatic
// ************************************************************************************************
class CuTMDbLockStatic: public CTreeItem
{
public:
	CuTMDbLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbLockStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMDbLockStatic();
	DECLARE_SERIAL (CuTMDbLockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbLockedTblStatic
// ************************************************************************************************
class CuTMDbLockedTblStatic: public CTreeItem
{
public:
	CuTMDbLockedTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbLockedTblStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMDbLockedTblStatic();
	DECLARE_SERIAL (CuTMDbLockedTblStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbLockedTbl
// ************************************************************************************************
class CuTMDbLockedTbl: public CTreeItem
{
public:
	CuTMDbLockedTbl (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbLockedTbl(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId() { return IDB_TM_LOCK_TYPE_TABLE; }

protected:
	CuTMDbLockedTbl();
	DECLARE_SERIAL (CuTMDbLockedTbl)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMDbLockedPgStatic
// ************************************************************************************************
class CuTMDbLockedPgStatic: public CTreeItem
{
public:
	CuTMDbLockedPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbLockedPgStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMDbLockedPgStatic();
	DECLARE_SERIAL (CuTMDbLockedPgStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbLockedPg
// ************************************************************************************************
class CuTMDbLockedPg: public CTreeItem
{
public:
	CuTMDbLockedPg (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbLockedPg(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId() { return IDB_TM_LOCK_TYPE_PAGE; }

protected:
	CuTMDbLockedPg();
	DECLARE_SERIAL (CuTMDbLockedPg)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMDbLockedOtherStatic
// ************************************************************************************************
class CuTMDbLockedOtherStatic: public CTreeItem
{
public:
	CuTMDbLockedOtherStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbLockedOtherStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMDbLockedOtherStatic();
	DECLARE_SERIAL (CuTMDbLockedOtherStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbLockedOther
// ************************************************************************************************
class CuTMDbLockedOther: public CTreeItem
{
public:
	CuTMDbLockedOther (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbLockedOther(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId() { return IDB_TM_LOCK_TYPE_OTHER; }

protected:
	CuTMDbLockedOther();
	DECLARE_SERIAL (CuTMDbLockedOther)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbServerStatic
// ************************************************************************************************
class CuTMDbServerStatic: public CTreeItem
{
public:
	CuTMDbServerStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbServerStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMDbServerStatic();
	DECLARE_SERIAL (CuTMDbServerStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbTransacStatic
// ************************************************************************************************
class CuTMDbTransacStatic: public CTreeItem
{
public:
	CuTMDbTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbTransacStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMDbTransacStatic();
	DECLARE_SERIAL (CuTMDbTransacStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbLockOnDbStatic
// ************************************************************************************************
class CuTMDbLockOnDbStatic: public CTreeItem
{
public:
	CuTMDbLockOnDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbLockOnDbStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMDbLockOnDbStatic();
	DECLARE_SERIAL (CuTMDbLockOnDbStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMDbSessionStatic
// ************************************************************************************************
class CuTMDbSessionStatic: public CTreeItem
{
public:
	CuTMDbSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbSessionStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMDbSessionStatic();
	DECLARE_SERIAL (CuTMDbSessionStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMActiveUsrStatic
// ************************************************************************************************
class CuTMActiveUsrStatic: public CTreeItem
{
public:
	CuTMActiveUsrStatic (CTreeGlobalData* pTreeGlobalData);
	virtual ~CuTMActiveUsrStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMActiveUsrStatic();
	DECLARE_SERIAL (CuTMActiveUsrStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMActiveUsr
// ************************************************************************************************
class CuTMActiveUsr: public CTreeItem
{
public:
	CuTMActiveUsr (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMActiveUsr(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId() { return IDB_TM_CONNECT_DATA_USER; }

protected:
	CuTMActiveUsr();
	DECLARE_SERIAL (CuTMActiveUsr)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMUsrSessStatic
// ************************************************************************************************
class CuTMUsrSessStatic: public CTreeItem
{
public:
	CuTMUsrSessStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMUsrSessStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMUsrSessStatic();
	DECLARE_SERIAL (CuTMUsrSessStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};





//
// CLASS: CuTMLockLocklistStatic
// ************************************************************************************************
class CuTMLockLocklistStatic: public CTreeItem
{
public:
	CuTMLockLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockLocklistStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMLockLocklistStatic();
	DECLARE_SERIAL (CuTMLockLocklistStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLockSessionStatic
// ************************************************************************************************
class CuTMLockSessionStatic: public CTreeItem
{
public:
	CuTMLockSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockSessionStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMLockSessionStatic();
	DECLARE_SERIAL (CuTMLockSessionStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMLockServerStatic
// ************************************************************************************************
class CuTMLockServerStatic: public CTreeItem
{
public:
	CuTMLockServerStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockServerStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMLockServerStatic();
	DECLARE_SERIAL (CuTMLockServerStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacLocklistStatic
// ************************************************************************************************
class CuTMTransacLocklistStatic: public CTreeItem
{
public:
	CuTMTransacLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacLocklistStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMTransacLocklistStatic();
	DECLARE_SERIAL (CuTMTransacLocklistStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacLockStatic
// ************************************************************************************************
class CuTMTransacLockStatic: public CTreeItem
{
public:
	CuTMTransacLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacLockStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMTransacLockStatic();
	DECLARE_SERIAL (CuTMTransacLockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacResourceStatic
// ************************************************************************************************
class CuTMTransacResourceStatic: public CTreeItem
{
public:
	CuTMTransacResourceStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacResourceStatic(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMTransacResourceStatic();
	DECLARE_SERIAL (CuTMTransacResourceStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacDbStatic
// ************************************************************************************************
class CuTMTransacDbStatic: public CTreeItem
{
public:
	CuTMTransacDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacDbStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMTransacDbStatic();
	DECLARE_SERIAL (CuTMTransacDbStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacDb
// ************************************************************************************************
class CuTMTransacDb: public CTreeItem
{
public:
	CuTMTransacDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacDb(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId();

protected:
	CuTMTransacDb();
	DECLARE_SERIAL (CuTMTransacDb)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacOtherStatic
// ************************************************************************************************
class CuTMTransacOtherStatic: public CTreeItem
{
public:
	CuTMTransacOtherStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacOtherStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMTransacOtherStatic();
	DECLARE_SERIAL (CuTMTransacOtherStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacOther
// ************************************************************************************************
class CuTMTransacOther: public CTreeItem
{
public:
	CuTMTransacOther (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacOther(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	virtual UINT  GetCustomImageId() { return IDB_TM_LOCK_TYPE_OTHER; }

protected:
	CuTMTransacOther();
	DECLARE_SERIAL (CuTMTransacOther)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacSessionStatic
// ************************************************************************************************
class CuTMTransacSessionStatic: public CTreeItem
{
public:
	CuTMTransacSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacSessionStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMTransacSessionStatic();
	DECLARE_SERIAL (CuTMTransacSessionStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMTransacServerStatic
// ************************************************************************************************
class CuTMTransacServerStatic: public CTreeItem
{
public:
	CuTMTransacServerStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacServerStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMTransacServerStatic();
	DECLARE_SERIAL (CuTMTransacServerStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};





//
// CLASS: CuTMLockinfoStatic
// ************************************************************************************************
class CuTMLockinfoStatic: public CTreeItem
{
public:
	CuTMLockinfoStatic (CTreeGlobalData* pTreeGlobalData);
	virtual ~CuTMLockinfoStatic(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
protected:
	CuTMLockinfoStatic();
	DECLARE_SERIAL (CuTMLockinfoStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLockinfoLocklistStatic
// ************************************************************************************************
class CuTMLockinfoLocklistStatic: public CTreeItem
{
public:
	CuTMLockinfoLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockinfoLocklistStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMLockinfoLocklistStatic();
	DECLARE_SERIAL (CuTMLockinfoLocklistStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMResourceDbLockStatic
// ************************************************************************************************
class CuTMResourceDbLockStatic: public CTreeItem
{
public:
	CuTMResourceDbLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMResourceDbLockStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMResourceDbLockStatic();
	DECLARE_SERIAL (CuTMResourceDbLockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMResourceTblLockStatic
// ************************************************************************************************
class CuTMResourceTblLockStatic: public CTreeItem
{
public:
	CuTMResourceTblLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMResourceTblLockStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMResourceTblLockStatic();
	DECLARE_SERIAL (CuTMResourceTblLockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMResourcePgLockStatic
// ************************************************************************************************
class CuTMResourcePgLockStatic: public CTreeItem
{
public:
	CuTMResourcePgLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMResourcePgLockStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMResourcePgLockStatic();
	DECLARE_SERIAL (CuTMResourcePgLockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMResourceOthLockStatic
// ************************************************************************************************
class CuTMResourceOthLockStatic: public CTreeItem
{
public:
	CuTMResourceOthLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMResourceOthLockStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMResourceOthLockStatic();
	DECLARE_SERIAL (CuTMResourceOthLockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMSessTblStatic
// ************************************************************************************************
class CuTMSessTblStatic: public CTreeItem
{
public:
	CuTMSessTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessTblStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMSessTblStatic();
	DECLARE_SERIAL (CuTMSessTblStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};



//
// CLASS: CuTMSessPgStatic
// ************************************************************************************************
class CuTMSessPgStatic: public CTreeItem
{
public:
	CuTMSessPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessPgStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMSessPgStatic();
	DECLARE_SERIAL (CuTMSessPgStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};



//
// CLASS: CuTMSessOthStatic
// ************************************************************************************************
class CuTMSessOthStatic: public CTreeItem
{
public:
	CuTMSessOthStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessOthStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMSessOthStatic();
	DECLARE_SERIAL (CuTMSessOthStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTblStatic
// ************************************************************************************************
class CuTMTblStatic: public CTreeItem
{
public:
	CuTMTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTblStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMTblStatic();
	DECLARE_SERIAL (CuTMTblStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTbl
// ************************************************************************************************
class CuTMTbl: public CTreeItem
{
public:
	CuTMTbl (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTbl(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId() { return IDB_TM_LOCK_TYPE_TABLE; }

protected:
	CuTMTbl();
	DECLARE_SERIAL (CuTMTbl)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};



//
// CLASS: CuTMPgStatic
// ************************************************************************************************
class CuTMPgStatic: public CTreeItem
{
public:
	CuTMPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMPgStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMPgStatic();
	DECLARE_SERIAL (CuTMPgStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMPg
// ************************************************************************************************
class CuTMPg: public CTreeItem
{
public:
	CuTMPg (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMPg(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId() { return IDB_TM_LOCK_TYPE_PAGE; }

protected:
	CuTMPg();
	DECLARE_SERIAL (CuTMPg)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMOthStatic
// ************************************************************************************************
class CuTMOthStatic: public CTreeItem
{
public:
	CuTMOthStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMOthStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMOthStatic();
	DECLARE_SERIAL (CuTMOthStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMOth
// ************************************************************************************************
class CuTMOth: public CTreeItem
{
public:
	CuTMOth (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMOth(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId() { return IDB_TM_LOCK_TYPE_OTHER; }

protected:
	CuTMOth();
	DECLARE_SERIAL (CuTMOth)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLogDbLocklistStatic
// ************************************************************************************************
class CuTMLogDbLocklistStatic: public CuTMDbLocklistStatic
{
public:
	CuTMLogDbLocklistStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogDbLocklistStatic(){};

protected:
	CuTMLogDbLocklistStatic();
	DECLARE_SERIAL (CuTMLogDbLocklistStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLogDbLockStatic
// ************************************************************************************************
class CuTMLogDbLockStatic: public CuTMDbLockStatic
{
public:
	CuTMLogDbLockStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogDbLockStatic(){};

protected:
	CuTMLogDbLockStatic();
	DECLARE_SERIAL (CuTMLogDbLockStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLogDbLockedTblStatic
// ************************************************************************************************
class CuTMLogDbLockedTblStatic: public CuTMDbLockedTblStatic
{
public:
	CuTMLogDbLockedTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogDbLockedTblStatic(){};

protected:
	CuTMLogDbLockedTblStatic();
	DECLARE_SERIAL (CuTMLogDbLockedTblStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLogDbLockedPgStatic
// ************************************************************************************************
class CuTMLogDbLockedPgStatic: public CuTMDbLockedPgStatic
{
public:
	CuTMLogDbLockedPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogDbLockedPgStatic(){};

protected:
	CuTMLogDbLockedPgStatic();
	DECLARE_SERIAL (CuTMLogDbLockedPgStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLogDbLockedOtherStatic
// ************************************************************************************************
class CuTMLogDbLockedOtherStatic: public CuTMDbLockedOtherStatic
{
public:
	CuTMLogDbLockedOtherStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogDbLockedOtherStatic(){};

protected:
	CuTMLogDbLockedOtherStatic();
	DECLARE_SERIAL (CuTMLogDbLockedOtherStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLogDbServerStatic
// ************************************************************************************************
class CuTMLogDbServerStatic: public CuTMDbServerStatic
{
public:
	CuTMLogDbServerStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogDbServerStatic(){};

protected:
	CuTMLogDbServerStatic();
	DECLARE_SERIAL (CuTMLogDbServerStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLogDbTransacStatic
// ************************************************************************************************
class CuTMLogDbTransacStatic: public CuTMDbTransacStatic
{
public:
	CuTMLogDbTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogDbTransacStatic(){};

protected:
	CuTMLogDbTransacStatic();
	DECLARE_SERIAL (CuTMLogDbTransacStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLogDbSessionStatic
// ************************************************************************************************
class CuTMLogDbSessionStatic: public CuTMDbSessionStatic
{
public:
	CuTMLogDbSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLogDbSessionStatic(){};

protected:
	CuTMLogDbSessionStatic();
	DECLARE_SERIAL (CuTMLogDbSessionStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLocklistSessionStatic
// ************************************************************************************************
class CuTMLocklistSessionStatic: public CTreeItem
{
public:
	CuTMLocklistSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLocklistSessionStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMLocklistSessionStatic();
	DECLARE_SERIAL (CuTMLocklistSessionStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMTransacTblStatic
// ************************************************************************************************
class CuTMTransacTblStatic: public CTreeItem
{
public:
	CuTMTransacTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacTblStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMTransacTblStatic();
	DECLARE_SERIAL (CuTMTransacTblStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacTbl
// ************************************************************************************************
class CuTMTransacTbl: public CTreeItem
{
public:
	CuTMTransacTbl (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacTbl(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	virtual UINT  GetCustomImageId() { return IDB_TM_LOCK_TYPE_TABLE; }

protected:
	CuTMTransacTbl();
	DECLARE_SERIAL (CuTMTransacTbl)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacPgStatic
// ************************************************************************************************
class CuTMTransacPgStatic: public CTreeItem
{
public:
	CuTMTransacPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacPgStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMTransacPgStatic();
	DECLARE_SERIAL (CuTMTransacPgStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMTransacPg
// ************************************************************************************************
class CuTMTransacPg: public CTreeItem
{
public:
	CuTMTransacPg (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMTransacPg(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	virtual UINT  GetCustomImageId() { return IDB_TM_LOCK_TYPE_PAGE; }

protected:
	CuTMTransacPg();
	DECLARE_SERIAL (CuTMTransacPg)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMLockinfoResDbStatic
// ************************************************************************************************
class CuTMLockinfoResDbStatic: public CTreeItem
{
public:
	CuTMLockinfoResDbStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockinfoResDbStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMLockinfoResDbStatic();
	DECLARE_SERIAL (CuTMLockinfoResDbStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLockinfoResTblStatic
// ************************************************************************************************
class CuTMLockinfoResTblStatic: public CTreeItem
{
public:
	CuTMLockinfoResTblStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockinfoResTblStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMLockinfoResTblStatic();
	DECLARE_SERIAL (CuTMLockinfoResTblStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLockinfoResPgStatic
// ************************************************************************************************
class CuTMLockinfoResPgStatic: public CTreeItem
{
public:
	CuTMLockinfoResPgStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockinfoResPgStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);

protected:
	CuTMLockinfoResPgStatic();
	DECLARE_SERIAL (CuTMLockinfoResPgStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMLockinfoResOthStatic
// ************************************************************************************************
class CuTMLockinfoResOthStatic: public CTreeItem
{
public:
	CuTMLockinfoResOthStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMLockinfoResOthStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

protected:
	CuTMLockinfoResOthStatic();
	DECLARE_SERIAL (CuTMLockinfoResOthStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};



// New as of May 14, 98: blocking session for a lock 
//
// CLASS: CuTMBlockingSessionStatic
// ************************************************************************************************
class CuTMBlockingSessionStatic: public CTreeItem
{
public:
	CuTMBlockingSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMBlockingSessionStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

	// Special methods for special value RES_CANNOT_SOLVE and RES_MULTIPLE_GRANTS
	virtual UINT GetStringId_ResCannotSolve()	  { return IDS_BLOCKING_SESSION_CANNOT_SOLVE; }
	virtual UINT GetStringId_ResMultipleGrants()  { return IDS_BLOCKING_SESSION_MULTIPLE_GRANTS; }

protected:
	CuTMBlockingSessionStatic();
	DECLARE_SERIAL (CuTMBlockingSessionStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMBlockingSession
// ************************************************************************************************
class CuTMBlockingSession: public CTreeItem
{
public:
	CuTMBlockingSession (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMBlockingSession(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT GetCustomImageId();

	// kill/shutdown special
	BOOL IsEnabled(UINT idMenu);
	virtual BOOL KillShutdown();

protected:
	CuTMBlockingSession();
	DECLARE_SERIAL (CuTMBlockingSession)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

// New as of May 18, 98: blocking session for a session 
//
// CLASS: CuTMSessBlockingSessionStatic
// ************************************************************************************************
class CuTMSessBlockingSessionStatic: public CTreeItem
{
public:
	CuTMSessBlockingSessionStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessBlockingSessionStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	virtual BOOL FilterApplies(FilterCause because);

	// Special methods for special value RES_CANNOT_SOLVE and RES_MULTIPLE_GRANTS
	virtual UINT GetStringId_ResCannotSolve()	  { return IDS_BLOCKING_SESSION_CANNOT_SOLVE; }
	virtual UINT GetStringId_ResMultipleGrants()  { return IDS_BLOCKING_SESSION_MULTIPLE_GRANTS; }


protected:
	CuTMSessBlockingSessionStatic();
	DECLARE_SERIAL (CuTMSessBlockingSessionStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMSessBlockingSession
// ************************************************************************************************
class CuTMSessBlockingSession: public CTreeItem
{
public:
	CuTMSessBlockingSession (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMSessBlockingSession(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	CuPageInformation* GetPageInformation ();
	virtual UINT GetCustomImageId();

	// kill/shutdown special
	BOOL IsEnabled(UINT idMenu);
	virtual BOOL KillShutdown();

protected:
	CuTMSessBlockingSession();
	DECLARE_SERIAL (CuTMSessBlockingSession)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


#endif // MONTREE_HEADER
