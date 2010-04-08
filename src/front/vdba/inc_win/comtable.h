/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comtable.h: interface for the CmtTable, CmtFolderTable class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Table object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMTABLE_HEADER)
#define COMTABLE_HEADER
#include "combase.h"
#include "dmltable.h"
#include "comaccnt.h"

#include "comcolum.h"
#include "comindex.h"
#include "comrule.h"
#include "cominteg.h"


class CmtFolderTable;
class CmtDatabase;

//
// Object: Table 
// *************
class CmtTable: public CmtItemData, public CmtProtect
{
	DECLARE_SERIAL(CmtTable)
public:
	CmtTable(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){Initialize();}
	CmtTable(CaTable* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		Initialize();
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtTable(){}
	virtual CmtSessionManager* GetSessionManager();
	void Initialize()
	{
		m_folderColumn.SetFolderType(CmtFolderColumn::F_COLUMN_TABLE);

		m_folderColumn.SetBackParent(this);
		m_folderIndex.SetBackParent(this);
		m_folderRule.SetBackParent(this);
		m_folderIntegrity.SetBackParent(this);

	}

	void SetBackParent(CmtFolderTable* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderTable* GetBackParent(){return m_pBackParent;}
	CaTable& GetTable(){return m_object;}
	void GetTable(CaTable*& pObject){pObject = new CaTable(m_object);}

	CmtFolderColumn&    GetFolderColumn(){return m_folderColumn;}
	CmtFolderIndex&     GetFolderIndex() {return m_folderIndex;}
	CmtFolderRule&      GetFolderRule() {return m_folderRule;}
	CmtFolderIntegrity& GetFolderIntegrity() {return m_folderIntegrity;}

protected:
	CaTable m_object;
	CmtFolderTable* m_pBackParent;

	CmtFolderColumn    m_folderColumn;
	CmtFolderIndex     m_folderIndex;
	CmtFolderRule      m_folderRule;
	CmtFolderIntegrity m_folderIntegrity;

};

//
// Object: Folder Table 
// ********************
class CmtFolderTable: public CmtItemData, public CmtProtect
{
public:
	CmtFolderTable():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderTable();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtDatabase* pBackParent){m_pBackParent = pBackParent;}
	CmtDatabase* GetBackParent(){return m_pBackParent;}
	CmtTable* SearchObject (CaLLQueryInfo* pInfo, CaTable* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);


protected:
	CTypedPtrList< CObList, CmtTable* > m_listObject;
	CmtDatabase* m_pBackParent;
};

#endif // COMTABLE_HEADER