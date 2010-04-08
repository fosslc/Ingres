/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : treerepl.h, header file (copy from montree2.h)
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut  (uk$so01)
**    Purpose  : Classes for replication branches
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
*/

#if !defined (TREEREPLICATOR_HEADER)
#define TREEREPLICATOR_HEADER
#include "vtree.h"

//
// CLASS: CuTMReplAllDbStatic
// ************************************************************************************************
class CuTMReplAllDbStatic: public CTreeItem
{
public:
	CuTMReplAllDbStatic (CTreeGlobalData* pTreeGlobalData);
	virtual ~CuTMReplAllDbStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// ??? virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMReplAllDbStatic();
	DECLARE_SERIAL (CuTMReplAllDbStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMReplAllDb
// ************************************************************************************************
class CuTMReplAllDb: public CTreeItem
{
public:
	CuTMReplAllDb (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMReplAllDb(){};

	CuPageInformation* GetPageInformation ();
	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// ??? virtual UINT  GetCustomImageId();
	virtual CString GetRightPaneTitle();
	virtual BOOL ItemDisplaysNoPage();
	virtual CString ItemNoPageCaption()  { return _T("Replication Objects are not Installed"); }
	virtual BOOL HasReplicMonitor() { return TRUE; }
	virtual void UpdateDataWhenSelChange();
	virtual UINT GetCustomImageId();

protected:
	CuTMReplAllDb();
	DECLARE_SERIAL (CuTMReplAllDb)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMReplServer
// ************************************************************************************************
class CuTMReplServer: public CTreeItem
{
public:
	CuTMReplServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMReplServer(){};

	CuPageInformation* GetPageInformation ();
	virtual UINT  GetCustomImageId();
	virtual BOOL  IsEnabled(UINT idMenu); // since specific context menu
	virtual void UpdateDataWhenSelChange();

protected:
	CuTMReplServer();
	DECLARE_SERIAL (CuTMReplServer)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMDbReplStatic
// ************************************************************************************************
class CuTMDbReplStatic: public CTreeItem
{
public:
	CuTMDbReplStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMDbReplStatic(){};

	CuPageInformation* GetPageInformation ();
	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// ??? virtual BOOL FilterApplies(FilterCause because);
	virtual CString GetRightPaneTitle();
	virtual BOOL	ItemDisplaysNoPage();
	virtual CString ItemNoPageCaption()  { return "Replication Objects are not Installed"; }
	virtual BOOL HasReplicMonitor() { return TRUE; }
	virtual CString GetDBName();
	virtual void UpdateDataWhenSelChange();
protected:
	CuTMDbReplStatic();
	DECLARE_SERIAL (CuTMDbReplStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

#endif // TREEREPLICATOR_HEADER
