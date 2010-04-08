/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comprofi.h: interface for the CmtProfile, CmtFolderProfile class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Profile object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMPROFILE_HEADER)
#define COMPROFILE_HEADER
#include "combase.h"
#include "dmlprofi.h"
#include "comaccnt.h"

class CmtFolderProfile;
//
// Object: Profile 
// ***************
class CmtProfile: public CmtItemData, public CmtProtect
{
public:
	CmtProfile(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtProfile(CaProfile* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtProfile(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderProfile* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderProfile* GetBackParent(){return m_pBackParent;}
	CaProfile& GetProfile(){return m_object;}
	void GetProfile(CaProfile*& pObject){pObject = new CaProfile(m_object);}

protected:
	CaProfile m_object;
	CmtFolderProfile* m_pBackParent;

};

//
// Object: Folder Profile 
// ***********************
class CmtFolderProfile: public CmtItemData, public CmtProtect
{
public:
	CmtFolderProfile():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderProfile();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtNode* pBackParent){m_pBackParent = pBackParent;}
	CmtNode* GetBackParent(){return m_pBackParent;}
	CmtProfile* SearchObject (CaLLQueryInfo* pInfo, CaProfile* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtProfile* > m_listObject;
	CmtNode* m_pBackParent;
};

#endif // COMPROFILE_HEADER