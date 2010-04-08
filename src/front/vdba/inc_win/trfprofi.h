/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dmlprofi.h: interface for the CtrfProfile, CtrfFolderProfile class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Profile object and folder
**
** History:
**
** 19-Oct-1999 (uk$so01)
**    Created
** 17-Dec-2002 (uk$so01)
**    SIR #109220, Enhance the library.
**/


#if !defined (TRFPROFILE_HEADER)
#define TRFPROFILE_HEADER
#include "trctldta.h"
#include "dmlprofi.h"


//
// Object: Profile
// ***************
class CtrfProfile: public CtrfItemData
{
	DECLARE_SERIAL (CtrfProfile)
public:
	CtrfProfile(): CtrfItemData(O_PROFILE){}
	CtrfProfile(CaProfile* pObj): CtrfItemData(O_PROFILE)
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CtrfProfile(){}
	virtual void Initialize(){};
	virtual CString GetDisplayedItem(int nMode = 0){return m_object.GetName();}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo)
	{
		int nImage, nImageExpand;
		pDisplayInfo->GetProfileImage  (nImage, nImageExpand);
		m_treeCtrlData.SetImage (nImage, nImageExpand);
	}

	CaProfile& GetObject(){return m_object;}
	void GetObject(CaProfile*& pObject){pObject = new CaProfile(m_object);}

	CaDBObject* GetDBObject(){return &m_object;}
protected:
	CaProfile m_object;
};


//
// Object: Folder of Profile
// *************************
class CtrfFolderProfile: public CtrfItemData
{
	DECLARE_SERIAL(CtrfFolderProfile)
public:
	CtrfFolderProfile();
	virtual ~CtrfFolderProfile();

	virtual UINT GetFolderFlag();
	virtual BOOL IsFolder() {return TRUE;}
	virtual BOOL IsDisplayFolder(CaDisplayInfo* pDisplayInfo){return pDisplayInfo->IsDisplayObjectInFolder(O_PROFILE);}
	virtual void SetImageID(CaDisplayInfo* pDisplayInfo);
	virtual void Expand  (CTreeCtrl* pTree, HTREEITEM hExpand);
	virtual BOOL RefreshData(CTreeCtrl* pTree, HTREEITEM hItem, CaRefreshTreeInfo* pInfo);
	virtual CaLLQueryInfo* GetQueryInfo(CaLLQueryInfo* pData = NULL);

	CtrfItemData* GetEmptyNode() {return &m_EmptyNode;}
	CtrfItemData* SearchObject (CaProfile* pObj);
protected:
	CtrfItemData m_EmptyNode;

};

#endif // TRFPROFILE_HEADER