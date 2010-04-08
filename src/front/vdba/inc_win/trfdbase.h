/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : trfdbase.h: interface for the CtrfDatabase, CtrfFolderDatabase class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Database object and folder
**
** History:
**
** 05-Feb-1998 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/

#if !defined (TRFDBASE_HEADER)
#define TRFDBASE_HEADER

#include "trctldta.h"
#include "constdef.h"
#include "dmldbase.h"
#include "trftable.h"
#include "trfview.h"
#include "trfproc.h"
#include "trfsynon.h"
#include "trfdbeve.h"
#include "trfdbase.h"
#include "trfalarm.h"


//
// Object: Database 
// ****************
class CtrfDatabase: public CtrfItemData
{
	DECLARE_SERIAL (CtrfDatabase)
public:
	CtrfDatabase(): CtrfItemData(O_DATABASE){}
	CtrfDatabase(CaDatabase* pObj): CtrfItemData(O_DATABASE)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	void Initialize();
	virtual ~CtrfDatabase();
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual UINT GetDisplayObjectFlag(CaDisplayInfo* pDisplayInfo);

	CaDatabase& GetObject(){return m_object;}
	void GetObject(CaDatabase*& pObject){pObject = new CaDatabase(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
	CtrfItemData* GetSubFolder(int nFolderType);

protected:
	CaDatabase m_object;

};


//
// Object: Folder of Database 
// ***************************
class CtrfFolderDatabase: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderDatabase)
public:
	CtrfFolderDatabase();
	virtual ~CtrfFolderDatabase();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	virtual UINT GetFolderFlag(){return FOLDER_DATABASE;}
	virtual BOOL IsFolder() {return TRUE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_DATABASE);}


	CtrfDatabase* SearchObject(CaDatabase* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}

protected:
	CtrfItemData m_EmptyNode;

};



#endif // TRFDBASE_HEADER
