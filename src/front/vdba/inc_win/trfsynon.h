/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlsynon.h: interface for the CtrfSynonym, CtrfFolderSynonym class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Synonym object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFSYNONYM_HEADER)
#define TRFSYNONYM_HEADER
#include "trctldta.h"
#include "dmlsynon.h"

//
// Object: Synonym
// ***************
class CtrfSynonym: public CtrfItemData
{
	DECLARE_SERIAL(CtrfSynonym)
public:
	CtrfSynonym(): CtrfItemData(O_SYNONYM){}
	CtrfSynonym(CaSynonym* pObj): CtrfItemData(O_SYNONYM)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfSynonym(){}
	virtual CString GetDisplayedItem(int nMode = 0);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetSynonymImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaSynonym& GetObject(){return m_object;}
	void GetObject(CaSynonym*& pObject){pObject = new CaSynonym(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaSynonym m_object;
};


//
// Folder of Synonym
// *****************
class CtrfFolderSynonym: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderSynonym)
public:
	CtrfFolderSynonym();
	virtual ~CtrfFolderSynonym();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);
	virtual UINT GetFolderFlag();
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);

	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_LOCATION);}

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
	CtrfItemData* SearchObject (CaSynonym* pObj, BOOL bMatchOwner = TRUE);
protected:
	CtrfItemData m_EmptyNode;

};

#endif // TRFSYNONYM_HEADER