/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmllocat.h: interface for the CtrfLocation, CtrfFolderLocation class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Location object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFLOCATION_HEADER)
#define TRFLOCATION_HEADER
#include "trctldta.h"
#include "dmllocat.h"


//
// Object: Location
// ****************
class CtrfLocation: public CtrfItemData
{
	DECLARE_SERIAL (CtrfLocation)
public:
	CtrfLocation(): CtrfItemData(O_LOCATION){}
	CtrfLocation(CaLocation* pObj): CtrfItemData(O_LOCATION)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfLocation(){}
	virtual void Initialize(){};
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetLocationImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaLocation& GetObject(){return m_object;}
	void GetObject(CaLocation*& pObject){pObject = new CaLocation(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaLocation m_object;
};


//
// Object: Folder of Location
// **************************
class CtrfFolderLocation: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderLocation)
public:
	CtrfFolderLocation();
	virtual ~CtrfFolderLocation();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);
	virtual UINT GetFolderFlag();
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);

	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_LOCATION);}

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
	CtrfItemData* SearchObject (CaLocation* pObj);

protected:
	CtrfItemData m_EmptyNode;

};


#endif // TRFLOCATION_HEADER
