/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comcolum.h: interface for the CmtColumn, CmtFolderColumn class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Column object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMCOLUMN_HEADER)
#define COMCOLUMN_HEADER
#include "combase.h"
#include "dmlcolum.h"
#include "comaccnt.h"

class CmtFolderColumn;
class CmtDatabase;

//
// Object: Column 
// *****************
class CmtColumn: public CmtItemData, public CmtProtect
{
public:
	CmtColumn(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	CmtColumn(CaColumn* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtColumn(){}
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtFolderColumn* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderColumn* GetBackParent(){return m_pBackParent;}
	CaColumn& GetColumn(){return m_object;}
	void GetColumn(CaColumn*& pObject){pObject = new CaColumn(m_object);}

protected:
	CaColumn m_object;
	CmtFolderColumn* m_pBackParent;

};

//
// Object: Folder Column 
// *************************
class CmtFolderColumn: public CmtItemData, public CmtProtect
{
public:
	typedef enum {F_COLUMN_TABLE = 0, F_COLUMN_VIEW, F_COLUMN_INDEX} COLUMN_FOLDERTYPE;
	CmtFolderColumn():m_pBackParent(NULL), m_nFolderType(F_COLUMN_TABLE), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderColumn();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtItemData* pBackParent){m_pBackParent = pBackParent;}
	CmtItemData* GetBackParent(){return m_pBackParent;}
	CmtColumn* SearchObject (CaLLQueryInfo* pInfo, CaColumn* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);

	void SetFolderType(COLUMN_FOLDERTYPE nType){m_nFolderType = nType;}
protected:
	CTypedPtrList< CObList, CmtColumn* > m_listObject;
	CmtItemData* m_pBackParent;
private:
	COLUMN_FOLDERTYPE m_nFolderType; // = F_COLUMN_TABLE (default)
};

#endif // COMCOLUMN_HEADER