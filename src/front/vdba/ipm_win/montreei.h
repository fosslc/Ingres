/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : montreei.h, Header File 
**    Project  : CA-OpenIngres/Monitoring.
**    Author   : Emmanuel Blattes
**    Purpose  : Classes for the ICE specific branches in the monitor tree control 
**
** History:
**
** 11-Sep-1998 (Emmanuel Blattes)
**    Created
*/


#ifndef MONTREE_ICE_HEADER
#define MONTREE_ICE_HEADER
#include "vtree.h"

//
// CLASS: CuTMIceServer
// ************************************************************************************************
class CuTMIceServer: public CTreeItem
{
public:
	CuTMIceServer (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceServer(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);
	virtual UINT GetCustomImageId();
	CuPageInformation* GetPageInformation ();

	// kill/shutdown special
	BOOL IsEnabled(UINT idMenu);
	virtual BOOL KillShutdown();

protected:
	CuTMIceServer();
	DECLARE_SERIAL (CuTMIceServer)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceActUsrStatic
// ************************************************************************************************
class CuTMIceActUsrStatic: public CTreeItem
{
public:
	CuTMIceActUsrStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceActUsrStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// if needed, unmask and write relevant method(s) in cpp file:
	//virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceActUsrStatic();
	DECLARE_SERIAL (CuTMIceActUsrStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceActUsr
// ************************************************************************************************
class CuTMIceActUsr: public CTreeItem
{
public:
	CuTMIceActUsr (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceActUsr(){};

	// if needed, unmask and write relevant method(s) in cpp file:
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceActUsr();
	DECLARE_SERIAL (CuTMIceActUsr)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMIceHttpConnStatic
// ************************************************************************************************
class CuTMIceHttpConnStatic: public CTreeItem
{
public:
	CuTMIceHttpConnStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceHttpConnStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// if needed, unmask and write relevant method(s) in cpp file:
	//virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceHttpConnStatic();
	DECLARE_SERIAL (CuTMIceHttpConnStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceHttpConn
// ************************************************************************************************
class CuTMIceHttpConn: public CTreeItem
{
public:
	CuTMIceHttpConn (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceHttpConn(){};

	// if needed, unmask and write relevant method(s) in cpp file:
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceHttpConn();
	DECLARE_SERIAL (CuTMIceHttpConn)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMIceDbConnStatic
// ************************************************************************************************
class CuTMIceDbConnStatic: public CTreeItem
{
public:
	CuTMIceDbConnStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceDbConnStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// if needed, unmask and write relevant method(s) in cpp file:
	//virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceDbConnStatic();
	DECLARE_SERIAL (CuTMIceDbConnStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceDbConn
// ************************************************************************************************
class CuTMIceDbConn: public CTreeItem
{
public:
	CuTMIceDbConn (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceDbConn(){};

	// if needed, unmask and write relevant method(s) in cpp file:
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceDbConn();
	DECLARE_SERIAL (CuTMIceDbConn)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMIceFIAllStatic
// ************************************************************************************************
class CuTMIceFIAllStatic: public CTreeItem
{
public:
	CuTMIceFIAllStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceFIAllStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// if needed, unmask and write relevant method(s) in cpp file:
	//virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceFIAllStatic();
	DECLARE_SERIAL (CuTMIceFIAllStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceFIAll
// ************************************************************************************************
class CuTMIceFIAll: public CTreeItem
{
public:
	CuTMIceFIAll (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceFIAll(){};

	// if needed, unmask and write relevant method(s) in cpp file:
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceFIAll();
	DECLARE_SERIAL (CuTMIceFIAll)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMIceConnUsrStatic
// ************************************************************************************************
class CuTMIceConnUsrStatic: public CTreeItem
{
public:
	CuTMIceConnUsrStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceConnUsrStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// if needed, unmask and write relevant method(s) in cpp file:
	//virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceConnUsrStatic();
	DECLARE_SERIAL (CuTMIceConnUsrStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceConnUsr
// ************************************************************************************************
class CuTMIceConnUsr: public CTreeItem
{
public:
	CuTMIceConnUsr (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceConnUsr(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);

	// if needed, unmask and write relevant method(s) in cpp file:
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceConnUsr();
	DECLARE_SERIAL (CuTMIceConnUsr)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMIceTransacStatic
// ************************************************************************************************
class CuTMIceTransacStatic: public CTreeItem
{
public:
	CuTMIceTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceTransacStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// if needed, unmask and write relevant method(s) in cpp file:
	//virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceTransacStatic();
	DECLARE_SERIAL (CuTMIceTransacStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceTransac
// ************************************************************************************************
class CuTMIceTransac: public CTreeItem
{
public:
	CuTMIceTransac (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceTransac(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);

	// if needed, unmask and write relevant method(s) in cpp file:
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceTransac();
	DECLARE_SERIAL (CuTMIceTransac)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMIceAllCursorStatic
// ************************************************************************************************
class CuTMIceAllCursorStatic: public CTreeItem
{
public:
	CuTMIceAllCursorStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceAllCursorStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// if needed, unmask and write relevant method(s) in cpp file:
	//virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceAllCursorStatic();
	DECLARE_SERIAL (CuTMIceAllCursorStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceAllCursor
// ************************************************************************************************
class CuTMIceAllCursor: public CTreeItem
{
public:
	CuTMIceAllCursor (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceAllCursor(){};

	// if needed, unmask and write relevant method(s) in cpp file:
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceAllCursor();
	DECLARE_SERIAL (CuTMIceAllCursor)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMIceAllTransacStatic
// ************************************************************************************************
class CuTMIceAllTransacStatic: public CTreeItem
{
public:
	CuTMIceAllTransacStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceAllTransacStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// if needed, unmask and write relevant method(s) in cpp file:
	//virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceAllTransacStatic();
	DECLARE_SERIAL (CuTMIceAllTransacStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceAllTransac
// ************************************************************************************************
class CuTMIceAllTransac: public CTreeItem
{
public:
	CuTMIceAllTransac (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceAllTransac(){};

	BOOL CreateStaticSubBranches (HTREEITEM hParentItem);

	// if needed, unmask and write relevant method(s) in cpp file:
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceAllTransac();
	DECLARE_SERIAL (CuTMIceAllTransac)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMIceFIUserStatic
// ************************************************************************************************
class CuTMIceFIUserStatic: public CTreeItem
{
public:
	CuTMIceFIUserStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceFIUserStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// if needed, unmask and write relevant method(s) in cpp file:
	//virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceFIUserStatic();
	DECLARE_SERIAL (CuTMIceFIUserStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceFIUser
// ************************************************************************************************
class CuTMIceFIUser: public CTreeItem
{
public:
	CuTMIceFIUser (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceFIUser(){};

	// if needed, unmask and write relevant method(s) in cpp file:
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceFIUser();
	DECLARE_SERIAL (CuTMIceFIUser)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};

//
// CLASS: CuTMIceCursorStatic
// ************************************************************************************************
class CuTMIceCursorStatic: public CTreeItem
{
public:
	CuTMIceCursorStatic (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceCursorStatic(){};

	CTreeItem* AllocateChildItem (CTreeItemData* pItemData);
	// if needed, unmask and write relevant method(s) in cpp file:
	//virtual BOOL FilterApplies(FilterCause because);
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceCursorStatic();
	DECLARE_SERIAL (CuTMIceCursorStatic)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


//
// CLASS: CuTMIceCursor
// ************************************************************************************************
class CuTMIceCursor: public CTreeItem
{
public:
	CuTMIceCursor (CTreeGlobalData* pTreeGlobalData, CTreeItemData* pItemData);
	virtual ~CuTMIceCursor(){};

	// if needed, unmask and write relevant method(s) in cpp file:
	CuPageInformation* GetPageInformation ();

protected:
	CuTMIceCursor();
	DECLARE_SERIAL (CuTMIceCursor)
public:
	void Serialize (CArchive& ar) { CTreeItem::Serialize(ar); }
};


#endif 
