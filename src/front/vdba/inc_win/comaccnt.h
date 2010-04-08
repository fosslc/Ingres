/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : comaccnt.h: interface for the CmtAccount, CmtFolderAccount class
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : ACCOUNT OBJECT object and folder
**
** History:
**
** 21-Nov-2000 (uk$so01)
**    Created
**/

#if !defined (COMACCNT_HEADER)
#define COMACCNT_HEADER
#include "combase.h"
#include "comvnode.h"
#include "comsessi.h"

//
// Object: CmtAccount
// *******************
// Each account maintains the list of virtual nodes

class CmtFolderAccount;
class CmtAccount: public CmtItemData, public CmtProtect
{
public:
	CmtAccount():m_pBackParent(NULL), m_bAnonymous(FALSE){Initialize();}
	CmtAccount(CmtFolderAccount* pBackParent, LPCTSTR lpszDomain, LPCTSTR lpszUser)
	    :m_pBackParent(pBackParent), m_strDomain(lpszDomain), m_strUser(lpszUser){Initialize();}

	virtual ~CmtAccount(){}
	virtual CmtSessionManager* GetSessionManager(){return &m_sessionManager;}

	HRESULT GetListNode          (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListNodeServer    (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListNodeLogin     (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListNodeConnection(CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListNodeAttribute (CaLLQueryInfo* pInfo, IStream*& pStream);

	HRESULT GetListUser     (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListGroup    (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListRole     (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListProfile  (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListLocation (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListDatabase (CaLLQueryInfo* pInfo, IStream*& pStream);

	HRESULT GetListTable    (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListView     (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListTableCol (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListViewCol  (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListProcedure(CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListDBEvent  (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListSynonym  (CaLLQueryInfo* pInfo, IStream*& pStream);

	HRESULT GetListIndex    (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListRule     (CaLLQueryInfo* pInfo, IStream*& pStream);
	HRESULT GetListIntegrity(CaLLQueryInfo* pInfo, IStream*& pStream);


	//
	// GET:
	CString GetDomain(){return m_strDomain;}
	CString GetUser(){return m_strUser;}
	BOOL    IsAnonymous(){return m_bAnonymous;}
	//
	// SET:
	void SetDomain(LPCTSTR lpszText){m_strDomain = lpszText;}
	void SetUser(LPCTSTR lpszText){m_strUser = lpszText;}
	void SetAnonymous(BOOL bSet){m_bAnonymous = bSet;}
	CmtFolderNode& GetFolderNode(){return m_folderNode;}

protected:
	void Initialize()
	{
		m_folderNode.SetBackParent(this);
	}

	BOOL    m_bAnonymous;
	CString m_strDomain;
	CString m_strUser;
	CmtFolderAccount* m_pBackParent;
	CmtFolderNode m_folderNode;
	CmtSessionManager m_sessionManager;
private:
	//
	// The GetListXxx() will call this function.
	// The goal of this function is only to group the commun code that
	// used by all functions GetListXxx().
	HRESULT GetListObject (CaLLQueryInfo* pInfo, IStream*& pStream);

};


class CmtFolderAccount: public CmtItemData, public CmtProtect
{
public:
	CmtFolderAccount();
	virtual ~CmtFolderAccount();
	BOOL Initialize(){return TRUE;} 

	CmtAccount* GetAnonymusAccount(){ASSERT(FALSE); return NULL;}
	CmtAccount* SearchObject (LPCTSTR lpszDomain, LPCTSTR lpszUser);
	CmtAccount* AddAccount (LPCTSTR lpszDomain, LPCTSTR lpszUser);
	void DelAccount (LPCTSTR lpszDomain, LPCTSTR lpszUser);

protected:
	CTypedPtrList< CObList, CmtAccount* > m_listObject;

};

#endif // COMACCNT_HEADER
