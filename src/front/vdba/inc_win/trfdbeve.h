/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmldbeve.h: interface for the CtrfDBEvent, CtrfFolderDBEvent class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : DBEvent object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFDBEVENT_HEADER)
#define TRFDBEVENT_HEADER
#include "trctldta.h"
#include "dmldbeve.h"

//
// Object: DBEvent:
// ************************************************************************************************
class CtrfDBEvent: public CtrfItemData
{
	DECLARE_SERIAL(CtrfDBEvent)
public:
	CtrfDBEvent(): CtrfItemData(O_DBEVENT){}
	CtrfDBEvent(CaDBEvent* pObj): CtrfItemData(O_DBEVENT)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfDBEvent(){}
	virtual void Initialize();
	virtual CString GetDisplayedItem(int nMode = 0);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetDBEventImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	CaDBEvent& GetObject(){return m_object;}
	void GetObject(CaDBEvent*& pObject){pObject = new CaDBEvent(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaDBEvent m_object;
};


//
// Folder of DBEvent:
// ************************************************************************************************
class CtrfFolderDBEvent: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderDBEvent)
public:
	CtrfFolderDBEvent();
	virtual ~CtrfFolderDBEvent();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);
	virtual UINT GetFolderFlag();
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);

	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_DBEVENT);}

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
	CtrfItemData* SearchObject (CaDBEvent* pObj, BOOL bMatchOwner = TRUE);
protected:
	CtrfItemData m_EmptyNode;

};

#endif // DMLDBEVENT_HEADER