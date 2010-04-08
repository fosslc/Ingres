/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlseq.h: interface for the CtrfSequence, CtrfFolderSequence class
**    Project  : Meta data library 
**    Author   : Philippe Schalk (schph01)
**    Purpose  : Sequence object and folder
**
** History:
**
** 22-Apr-2003 (schph01)
**    107523 Add Sequences Object
**/


#if !defined (TRFSEQ_HEADER)
#define TRFSEQ_HEADER
#include "trctldta.h"
#include "dmlseq.h"


//
// Object: Sequence 
// ************************************************************************************************
class CtrfSequence: public CtrfItemData
{
	DECLARE_SERIAL(CtrfSequence)
public:
	CtrfSequence(): CtrfItemData(O_SEQUENCE){}
	CtrfSequence(CaSequence* pObj): CtrfItemData(O_SEQUENCE)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfSequence(){}
	virtual void Initialize();
	virtual CString GetDisplayedItem(int nMode = 0);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetSequenceImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	CaSequence& GetObject(){return m_object;}
	void GetObject(CaSequence*& pObject){pObject = new CaSequence(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaSequence m_object;
};


//
// Object: Folder of Sequence 
// ************************************************************************************************
class CtrfFolderSequence: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderSequence)
public:
	CtrfFolderSequence();
	virtual ~CtrfFolderSequence();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);
	virtual UINT GetFolderFlag();
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);

	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_SEQUENCE);}

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
	CtrfItemData* SearchObject (CaSequence* pObj, BOOL bMatchOwner = TRUE);

protected:
	CtrfItemData m_EmptyNode;

};


#endif // TRFSEQ_HEADER
