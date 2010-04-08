/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlproc.h: interface for the CtrfProcedure, CtrfFolderProcedure class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Procedure object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFPROC_HEADER)
#define TRFPROC_HEADER
#include "trctldta.h"
#include "dmlproc.h"


//
// Object: Procedure 
// ************************************************************************************************
class CtrfProcedure: public CtrfItemData
{
	DECLARE_SERIAL(CtrfProcedure)
public:
	CtrfProcedure(): CtrfItemData(O_PROCEDURE){}
	CtrfProcedure(CaProcedure* pObj): CtrfItemData(O_PROCEDURE)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfProcedure(){}
	virtual void Initialize();
	virtual CString GetDisplayedItem(int nMode = 0);
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetProcedureImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	CaProcedure& GetObject(){return m_object;}
	void GetObject(CaProcedure*& pObject){pObject = new CaProcedure(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaProcedure m_object;
};


//
// Object: Folder of Procedure 
// ************************************************************************************************
class CtrfFolderProcedure: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderProcedure)
public:
	CtrfFolderProcedure();
	virtual ~CtrfFolderProcedure();

	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);
	virtual UINT GetFolderFlag();
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);

	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_PROCEDURE);}

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
	CtrfItemData* SearchObject (CaProcedure* pObj, BOOL bMatchOwner = TRUE);

protected:
	CtrfItemData m_EmptyNode;

};


#endif // TRFPROC_HEADER
