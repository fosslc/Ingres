/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlinteg.h: interface for the CtrfIntegrity, CtrfFolderIntegrity class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Integrity object and Folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFINTEGRITY_HEADER)
#define TRFINTEGRITY_HEADER
#include "trctldta.h"
#include "dmlinteg.h"
#include "comsessi.h"



//
// Object: Integrity
// *****************
class CtrfIntegrity: public CtrfItemData
{
	DECLARE_SERIAL(CtrfIntegrity)
public:
	CtrfIntegrity(): CtrfItemData(O_INTEGRITY){}
	CtrfIntegrity(CaIntegrity* pObj): CtrfItemData(O_INTEGRITY)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfIntegrity(){}
	virtual CString GetDisplayedItem(int nMode = 0);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetIndexImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}
	CaIntegrity& GetObject(){return m_object;}
	void GetObject(CaIntegrity*& pObject){pObject = new CaIntegrity(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaIntegrity m_object;
};



//
// Folder of Integrity:
// ********************
class CtrfFolderIntegrity: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderIntegrity)
public:
	CtrfFolderIntegrity();
	virtual ~CtrfFolderIntegrity();


	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	virtual UINT GetFolderFlag();
	virtual BOOL IsFolder() {return TRUE;}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_INDEX);}


	CtrfItemData* SearchObject(CaIntegrity* pObj);
	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}

protected:
	CtrfItemData m_EmptyNode;

};



#endif // TRFINTEGRITY_HEADER