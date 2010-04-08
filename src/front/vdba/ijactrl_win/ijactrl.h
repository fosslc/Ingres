/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : ijactrl.h , Header File for IJACTRL.DLL
**    Project  : Ingres Journal Analyser
**    Author   : Sotheavut UK (uk$so01)
**
**    Purpose  : Main header file for IJACTRL.DLL  (ActiveX)
**
** History:
** 22-Dec-1999 (uk$so01)
**   Created
** 14-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management and put string into the resource files
** 21-May-2001 (uk$so01)
**    SIR #101169 (additional changes)
**    Help management.
** 10-Oct-2001 (hanje04)
**    Bug:105483
**    Added m_strLocalIITemporary so that a temporary location doesn't have to
**    be constructed from strLocalIISystem, which is wrong for UNIX. This will
**    be set to II_TEMPORARY as it should be.
** 05-Sep-2003 (uk$so01)
**    SIR #106648, Integrate libraries libwctrl.lib, libingll.lib in Ija
**/

#if !defined(AFX_IJACTRL_H__C92B842D_B176_11D3_A322_00C04F1F754A__INCLUDED_)
#define AFX_IJACTRL_H__C92B842D_B176_11D3_A322_00C04F1F754A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols
#include "sessimgr.h"

#define WM_USER_CTRLxSHIFT_UP (WM_USER + 2000)

//
// Default color for item of transaction:
#if defined (FGCOLOR)
const COLORREF TRGBDEF_DELETE       = RGB (0xFF, 0x00, 0x00);
const COLORREF TRGBDEF_INSERT       = RGB (0x00, 0x80, 0x00);
const COLORREF TRGBDEF_UPDATE       = RGB (0x00, 0xFF, 0x00);
const COLORREF TRGBDEF_BEFOREUPDATE = RGB (0xFF, 0x80, 0x00);
const COLORREF TRGBDEF_AFTEREUPDATE = RGB (0x00, 0x00, 0xFF);
const BOOL DEFPAINT_FKTRANSACTION   = TRUE;
#else
const COLORREF TRGBDEF_DELETE       = RGB (0xFF, 0x00, 0x00);
const COLORREF TRGBDEF_INSERT       = RGB (0x00, 0xFF, 0x00);
const COLORREF TRGBDEF_UPDATE       = RGB (0x00, 0xFF, 0x00);
const COLORREF TRGBDEF_BEFOREUPDATE = RGB (0xFF, 0x80, 0x00);
const COLORREF TRGBDEF_AFTEREUPDATE = RGB (0x00, 0xFF, 0x00);
const BOOL DEFPAINT_FKTRANSACTION   = FALSE;
#endif
const COLORREF TRGBDEF_OTHERS       = RGB (0x00, 0xFF, 0xFF);


class CaPropertyData
{
public:
	CaPropertyData()
	{
		m_bPaintForegroundTransaction = DEFPAINT_FKTRANSACTION;
		m_transactionColorDelete      = TRGBDEF_DELETE;
		m_transactionColorInsert      = TRGBDEF_INSERT;
		m_transactionColorAfterUpdate = TRGBDEF_AFTEREUPDATE;
		m_transactionColorBeforeUpdate= TRGBDEF_BEFOREUPDATE;
		m_transactionColorOther       = TRGBDEF_OTHERS;
	}
	~CaPropertyData(){}

	COLORREF TRGB_DELETE(){return m_transactionColorDelete;}
	COLORREF TRGB_INSERT(){return m_transactionColorInsert;}
	COLORREF TRGB_AFTEREUPDATE(){return m_transactionColorAfterUpdate;}
	COLORREF TRGB_BEFOREUPDATE (){return m_transactionColorBeforeUpdate;}
	COLORREF TRGB_OTHERS(){return m_transactionColorOther;}


	COLORREF m_transactionColorDelete;
	COLORREF m_transactionColorInsert;
	COLORREF m_transactionColorAfterUpdate;
	COLORREF m_transactionColorBeforeUpdate;
	COLORREF m_transactionColorOther;
	BOOL m_bPaintForegroundTransaction;
};

typedef enum tagDATEFORMAT
{
	II_DATE_US = 1,
	II_DATE_MULTINATIONAL,
	II_DATE_MULTINATIONAL4,
	II_DATE_ISO,
	II_DATE_SWEDEN,
	II_DATE_FINLAND,
	II_DATE_GERMAN,
	II_DATE_YMD,
	II_DATE_DMY,
	II_DATE_MDY
} II_DATEFORMAT;


/////////////////////////////////////////////////////////////////////////////
// CappIjaCtrl : See IjaCtrl.cpp for implementation.

class CappIjaCtrl : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();
	CaSessionManager& GetSessionManager() {return m_sessionManager;}

	CString m_strAllUser;
	CStringList m_listNode;
	CString m_strLocalIISystem;
	CString m_strLocalIITemporary;

	CaPropertyData m_property;
	BOOL m_bTableWithOwnerAllowed;
	II_DATEFORMAT m_dateFormat;
	int m_dateCenturyBoundary;
	BOOL m_bHelpFileAvailable;
	CString m_strHelpFile;
	CString m_strNewHelpPath;
protected:
	CaSessionManager m_sessionManager;
};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;
extern CappIjaCtrl theApp;
BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);


#define ID_HELP_RECOVER_SCRIPT    500
#define ID_HELP_RECOVERNOW_SCRIPT 501
#define ID_HELP_REDO_SCRIPT       502
#define ID_HELP_REDODOW_SCRIPT    503

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IJACTRL_H__C92B842D_B176_11D3_A322_00C04F1F754A__INCLUDED)
