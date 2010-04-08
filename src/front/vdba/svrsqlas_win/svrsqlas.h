/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : svrsqlas.h : main header file for the SVRSQLAS DLL
**    Project  : SQL ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : This class defines custom modal property sheet 
**                CuSQLPropertySheet.
**
** History:
**
** 10-Jul-2001 (uk$so01)
**    Created
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Rewrite SQL Assistent as an In-Process COM Server.
**/


#if !defined(AFX_SVRSQLAS_H__A95881D6_744C_11D5_875F_00C04F1F754A__INCLUDED_)
#define AFX_SVRSQLAS_H__A95881D6_744C_11D5_875F_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h" // main symbols
#include "server.h"   // main server
#include "sessimgr.h" // Session Manager (shared and global)


class CaSychronizeInterrupt
{
public:
	CaSychronizeInterrupt();
	~CaSychronizeInterrupt();

	void BlockUserInterface(BOOL bLock);
	void BlockWorkerThread (BOOL bLock);
	void WaitUserInterface();
	void WaitWorkerThread();

	void SetRequestCancel(BOOL bSet){m_bRequestCansel = bSet;}
	BOOL IsRequestCancel(){return m_bRequestCansel;}
protected:
	BOOL   m_bRequestCansel;
	HANDLE m_hEventWorkerThread;
	HANDLE m_hEventUserInterface;
};


class CSvrsqlasApp : public CWinApp
{
public:
	CSvrsqlasApp();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSvrsqlasApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	//
	// This function 'INGRESII_QueryObject' should call the query objects from the 
	// Static library ("INGRESII_llQueryObject") or from the COM Server ICAS.
	BOOL INGRESII_QueryObject(CaLLQueryInfo* pInfo, CTypedPtrList<CObList, CaDBObject*>& listObject);

	CaSessionManager& GetSessionManager(){return m_sessionManager;}
	CaComServer* GetServer() {return m_pServer;}
	void OutOfMemoryMessage()
	{
		AfxMessageBox (m_tchszOutOfMemoryMessage, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
	}
	CTypedPtrList<CObList, CaSQLComponent*>& GetSqlDataConversion(){return m_listSQLDataConv;}
	CTypedPtrList<CObList, CaSQLComponent*>& GetSqlNumericFunction(){return m_listSQLNumFuncs;}
	CTypedPtrList<CObList, CaSQLComponent*>& GetSqlStringFunction(){return m_listSQLStrFuncs;}
	CTypedPtrList<CObList, CaSQLComponent*>& GetSqlDateFunction(){return m_listSQLDateFuncs;}
	CTypedPtrList<CObList, CaSQLComponent*>& GetSqlAggregateFunction(){return m_listSQLAggFuncs;}
	CTypedPtrList<CObList, CaSQLComponent*>& GetSqlIfFunction(){return m_listSQLIfnFuncs;}
	CTypedPtrList<CObList, CaSQLComponent*>& GetSqlPredicateFunction(){return m_listSQLPredFuncs;}
	CTypedPtrList<CObList, CaSQLComponent*>& GetSqlPredicateCombinationFunction(){return m_listSQLPredCombFuncs;}
	CTypedPtrList<CObList, CaSQLComponent*>& GetSqlDatabaseObjectFuntion(){return m_listSQLDBObjects;}
	CTypedPtrList<CObList, CaSQLComponent*>& GetSqlSelectFuntion(){return m_listSQLSelFuncs;}
	CTypedPtrList<CObList, CaSQLFamily*>&    GetSqlFamily(){return m_listSqlFamily;}

	CString m_strHelpFile;
	CString m_strInstallationID;
	BOOL m_bHelpFileAvailable;
	CaSychronizeInterrupt m_synchronizeInterrupt;

protected:
	BOOL UnicodeOk();
	void Init();
	void Done();

	CaComServer* m_pServer;
	CaSessionManager m_sessionManager;
	TCHAR m_tchszOutOfMemoryMessage[128];
	CTypedPtrList<CObList, CaSQLComponent*> m_listSQLDataConv;
	CTypedPtrList<CObList, CaSQLComponent*> m_listSQLNumFuncs;
	CTypedPtrList<CObList, CaSQLComponent*> m_listSQLStrFuncs;
	CTypedPtrList<CObList, CaSQLComponent*> m_listSQLDateFuncs;
	CTypedPtrList<CObList, CaSQLComponent*> m_listSQLAggFuncs;
	CTypedPtrList<CObList, CaSQLComponent*> m_listSQLIfnFuncs;
	CTypedPtrList<CObList, CaSQLComponent*> m_listSQLPredFuncs;
	CTypedPtrList<CObList, CaSQLComponent*> m_listSQLPredCombFuncs;
	CTypedPtrList<CObList, CaSQLComponent*> m_listSQLDBObjects;
	CTypedPtrList<CObList, CaSQLComponent*> m_listSQLSelFuncs;
	
	CTypedPtrList<CObList, CaSQLFamily*>    m_listSqlFamily;


	//{{AFX_MSG(CSvrsqlasApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);
extern CSvrsqlasApp theApp;


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVRSQLAS_H__A95881D6_744C_11D5_875F_00C04F1F754A__INCLUDED_)
