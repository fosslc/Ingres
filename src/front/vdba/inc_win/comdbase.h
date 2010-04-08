/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comdbase.h: interface for the CmtDatabase, CmtFolderDatabase class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Database object and folder
**
** History:
**
** 23-Nov-2000 (uk$so01)
**    Created
**/


#if !defined (COMDATABASE_HEADER)
#define COMDATABASE_HEADER
#include "combase.h"
#include "dmldbase.h"
#include "comaccnt.h"
#include "comtable.h"
#include "comview.h"
#include "comproc.h"
#include "comdbeve.h"
#include "comsynon.h"


class CmtFolderDatabase;
//
// Object: Database 
// *****************
class CmtDatabase: public CmtItemData, public CmtProtect
{
public:
	CmtDatabase(): m_pBackParent(NULL), CmtItemData(), CmtProtect(){Initialize();}
	CmtDatabase(CaDatabase* pObj): m_pBackParent(NULL), CmtItemData(), CmtProtect()
	{
		Initialize();
		ASSERT(pObj);
		if (pObj)
			m_object = *pObj;
	}
	virtual ~CmtDatabase(){}
	virtual CmtSessionManager* GetSessionManager();
	void Initialize()
	{
		m_folderTable.SetBackParent(this);
		m_folderView.SetBackParent(this);
		m_folderProcedure.SetBackParent(this);
		m_folderDBEvent.SetBackParent(this);
		m_folderSynonym.SetBackParent(this);
	}

	void SetBackParent(CmtFolderDatabase* pBackParent){m_pBackParent = pBackParent;}
	CmtFolderDatabase* GetBackParent(){return m_pBackParent;}
	CaDatabase& GetDatabase(){return m_object;}
	void GetDatabase(CaDatabase*& pObject){pObject = new CaDatabase(m_object);}

	CmtFolderTable& GetFolderTable(){return m_folderTable;}
	CmtFolderView& GetFolderView(){return m_folderView;}
	CmtFolderProcedure& GetFolderProcedure(){return m_folderProcedure;}
	CmtFolderDBEvent& GetFolderDBEvent(){return m_folderDBEvent;}
	CmtFolderSynonym& GetFolderSynonym(){return m_folderSynonym;}

protected:
	CaDatabase m_object;
	CmtFolderDatabase* m_pBackParent;

	CmtFolderTable     m_folderTable;
	CmtFolderView      m_folderView;
	CmtFolderProcedure m_folderProcedure;
	CmtFolderDBEvent   m_folderDBEvent;
	CmtFolderSynonym   m_folderSynonym;
};

//
// Object: Folder Database 
// ************************
class CmtFolderDatabase: public CmtItemData, public CmtProtect
{
public:
	CmtFolderDatabase():m_pBackParent(NULL), CmtItemData(), CmtProtect(){}
	virtual ~CmtFolderDatabase();
	virtual CmtSessionManager* GetSessionManager();

	void SetBackParent(CmtNode* pBackParent){m_pBackParent = pBackParent;}
	CmtNode* GetBackParent(){return m_pBackParent;}
	CmtDatabase* SearchObject (CaLLQueryInfo* pInfo, CaDatabase* pObj, BOOL b2QueryIfNotDone = TRUE);
	HRESULT GetListObject(CaLLQueryInfo* pInfo, IStream*& pStream, BOOL bCopy2Stream = FALSE);

protected:
	CTypedPtrList< CObList, CmtDatabase* > m_listObject;
	CmtNode* m_pBackParent;
};

#endif // COMDATABASE_HEADER