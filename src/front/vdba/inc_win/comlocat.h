/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comlocat.h: interface for the CmtLocation, CmtFolderLocation class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Location object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMLOCATION_HEADER)
#define COMLOCATION_HEADER
#include "combase.h"
#include "dmllocat.h"
#include "comaccnt.h"

class CmtFolderLocation;
//
// Object: Location 
// ****************
class CmtLocation: public CmtItemData, public CmtProtect
{
public:
	CmtLocation(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtLocation(CaLocation* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtLocation(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderLocation* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderLocation* GetBackParent(){return m_pBackParent;}
	CaLocation& GetLocation(){return m_object;}
	void GetLocation(CaLocation*& pObject){pObject = new CaLocation(m_object);}

protected:
	CaLocation m_object;
	CmtFolderLocation* m_pBackParent;

};

//
// Object: Folder Location 
// ***********************
class CmtFolderLocation: public CmtItemData, public CmtProtect
{
public:
	CmtFolderLocation():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderLocation();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtNode* pBackParent){m_pBackParent = pBackParent;}
	CmtNode* GetBackParent(){return m_pBackParent;}
	CmtLocation* SearchObject (CaLLQueryInfo* pInfo, CaLocation* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtLocation* > m_listObject;
	CmtNode* m_pBackParent;
};

#endif // COMLOCATION_HEADER