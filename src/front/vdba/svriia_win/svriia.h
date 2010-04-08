/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : svriia.h : main header file for the SVRIIA DLL
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : This class defines custom modal property sheet 
**                CuIIAPropertySheet.
**
** History:
**
** 11-Dec-2000 (uk$so01)
**    Created
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 03-Apr-2002 (uk$so01)
**    BUG #107488, Make svriia.dll enable to import empty .DBF file
**    It just create the table.
**/

#if !defined(AFX_SVRIIA_H__2B9B2EFC_CA8B_11D4_872E_00C04F1F754A__INCLUDED_)
#define AFX_SVRIIA_H__2B9B2EFC_CA8B_11D4_872E_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h" // main symbols
#include "server.h"   // main server
#include "comsessi.h" // Session Manager (shared and global)
#include "formdata.h" // IIA DATA "CaIIAInfo"
#include "impexpas.h" // IIASTRUCT
#include "synchron.h" // CaSychronizeInterrupt


class CSvriiaApp : public CWinApp
{
public:
	CSvriiaApp();
	void SetInputParameter(IIASTRUCT* pData){m_pParamStruct = pData;}
	IIASTRUCT* GetInputParameter(){return m_pParamStruct;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSvriiaApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	CaComServer* GetServer() {return m_pServer;}
	void OutOfMemoryMessage()
	{
		AfxMessageBox (m_tchszOutOfMemoryMessage, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
	}
	CaUserData4GetRow* GetUserData4GetRow(){return &m_userData4GetRow;}
	int GetCursorSequenceNumber() {m_nSequence++; return m_nSequence;}
	CmtSessionManager& GetSessionManager() {return m_sessionManager;}
	//
	// Global data:
	CmtSessionManager m_sessionManager;
	CString m_strHelpFile;
	CString m_strInstallationID;
	BOOL m_bHelpFileAvailable;
	CString m_strNullNA;
	CaSychronizeInterrupt m_synchronizeInterrupt;

protected:
	BOOL UnicodeOk();
	CaComServer* m_pServer;
	TCHAR m_tchszOutOfMemoryMessage[128];
	int m_nSequence;
	//
	// For managing the SQL COPY handler:
	CaUserData4GetRow m_userData4GetRow; 

	//
	// Input parameter data:
	IIASTRUCT* m_pParamStruct;
public:
	//{{AFX_MSG(CSvriiaApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define IDHELP_BASE     100 // Help ID of Step1 = 100, ... of step n = IDHELP_BASE+n     (import assistant)
#define IDHELP_BASE_IEA 110 // Help ID of Step1 = 110, ... of step n = IDHELP_BASE_IEA+n (export assistant)
BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);
extern CSvriiaApp theApp;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SVRIIA_H__2B9B2EFC_CA8B_11D4_872E_00C04F1F754A__INCLUDED_)
