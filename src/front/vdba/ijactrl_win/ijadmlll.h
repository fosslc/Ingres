/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijadmlll.h , Header File
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Data Manipulation  (Ingres Low level Data Query)
**
** History:
**
** 23-Dec-1999 (uk$so01)
**    Created
** 07-Jan-2000 (noifr01)
**    added the rmcmd definitions
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
**/

#if !defined(IJADMLLL_HEADER)
#define IJADMLLL_HEADER

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
#include "ijalldml.h"
#include "sqlselec.h"
#include "constdef.h"
#define COL_DISP_UNAVAILABLE _T("< ??? >")


class CaIjaCursor: public CaCursor
{
public:
	CaIjaCursor (int nCounter, LPCTSTR lpszRawStatement): CaCursor(nCounter, lpszRawStatement){}
	~CaIjaCursor(){};

	BOOL Fetch(CStringList& tuple, int& nCount);
};


class CaCursor4Statements: public CaIjaCursor
{
public:
	CaCursor4Statements (int nCounter, LPCTSTR lpszRawStatement): CaIjaCursor(nCounter, lpszRawStatement){}
	~CaCursor4Statements(){};

	BOOL Fetch(CStringList& tuple, int& nCount);
};


class CaTemporarySession: public CaSession
{
public:
	//
	// lpszUser, if not null, then use the CONNECT ... IDENTIFIED BY lpszUser
	CaTemporarySession(LPCTSTR lpszNode, LPCTSTR lpszDatabase, LPCTSTR lpszUser = NULL);
	~CaTemporarySession();
	BOOL IsConnected();
	BOOL Release(SessionReleaseMode nReleaseMode = SESSION_COMMIT);
	BOOL Open();  // Open the connection (only if it has been released previously)
	BOOL IsLocalNode() 
	{
		if (m_strNode.IsEmpty())
			return TRUE;
		if (m_strNode.Find(_T("(local)"))==0)
			return TRUE;
		return FALSE;
	}
protected:
	CaSession* m_pSession;
};

class CaLowlevelAddAlterDrop: public CaLLAddAlterDrop
{
public:
	CaLowlevelAddAlterDrop():CaLLAddAlterDrop()
	{
		m_bNeedSession = TRUE;
	}
	CaLowlevelAddAlterDrop(LPCTSTR lpszVNode, LPCTSTR lpszDatabase, LPCTSTR lpszStatement):
	    CaLLAddAlterDrop(lpszVNode, lpszDatabase, lpszStatement){}

	virtual ~CaLowlevelAddAlterDrop(){}

	BOOL NeedSession(){return m_bNeedSession;}
	void NeedSession (BOOL bNeed){m_bNeedSession = bNeed;}
protected:
	BOOL m_bNeedSession;
};


class CaExecuteScript: public CaLowlevelAddAlterDrop
{
public:
	CaExecuteScript(): CaLowlevelAddAlterDrop() {}
	CaExecuteScript(LPCTSTR lpszVNode, LPCTSTR lpszDatabase, LPCTSTR lpszStatement):
		CaLowlevelAddAlterDrop(lpszVNode, lpszDatabase, lpszStatement){};

	~CaExecuteScript(){}
protected:
	//
	// Add extra for execute script

};

BOOL IJA_llQueryColumn   (CaQueryTransactionInfo* pQueryInfo, CTypedPtrList<CObList, CaColumn*>& listObject, BOOL bNeedSession = TRUE);
BOOL IJA_llQueryUserInCurSession (CString &username);
BOOL GetTempAuditTableName( CString& csResultString, LPCTSTR lpSourceTableName, LPCTSTR lpSchemaName);
BOOL INGRESII_llQueryConstraint (CaQueryTransactionInfo* pQueryInfo, CTypedPtrList< CObList, CaDBObject* >& listObject);

BOOL IJA_llGetArchiverInternal ();
void IJA_llActivateLog2Journal ();
void IJA_llLogFileStatus(CString& csCurStatus);
typedef enum 
{
	ACTION_RECOVER=0,         // Recover Now
	ACTION_RECOVER_SCRIPT,    // Generate script of recover
	ACTION_RECOVERNOW_SCRIPT, // Generate script of recover now
	ACTION_REDO,              // Redo Now
	ACTION_REDO_SCRIPT,       // Generate script of redo
	ACTION_REDONOW_SCRIPT     // Generate script of redo
} ActionType;

class CaRecoverRedoInformation;
BOOL IJA_RecoverRedoAction (ActionType nType, CaRecoverRedoInformation* pData, CString& strScript, CString& strError, BOOL &bRoundingErrorsPossibleInText);



#endif // IJADMLLL_HEADER