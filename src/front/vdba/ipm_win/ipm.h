/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : ipm.h : Header File.
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : main header file for IPM.DLL
**
** History:
**
** 12-Nov-2001 (uk$so01)
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/

#if !defined(AFX_IPM_H__DAE12BDB_C91C_11D5_8786_00C04F1F754A__INCLUDED_)
#define AFX_IPM_H__DAE12BDB_C91C_11D5_8786_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif

#include "resource.h"       // main symbols
#include "axtracks.h"



#define DEFAULT_TIMEOUT           0
#define DEFAULT_ACTIVATEREFRESH  FALSE
#define DEFAULT_GRID             FALSE
#define DEFAULT_REFRESHFREQUENCY 60
#define DEFAULT_UNIT              0
#define DEFAULT_MAXSESSION       20

#define IPMMASK_BASE             1
#define IPMMASK_TIMEOUT          (IPMMASK_BASE <<  1)
#define IPMMASK_BKREFRESH        (IPMMASK_BASE <<  2)
#define IPMMASK_SHOWGRID         (IPMMASK_BASE <<  3)
#define IPMMASK_FONT             (IPMMASK_BASE <<  4)
#define IPMMASK_MAXSESSION       (IPMMASK_BASE <<  5)


class CaIpmProperty: public CObject
{
public:
	CaIpmProperty();
	virtual ~CaIpmProperty(){}

	//
	// Get Methods:

	long GetTimeout(){return m_nTimeOut;};
	long GetRefreshFrequency(){return m_nRefreshFrequency;};
	BOOL IsRefreshActivated(){return m_bRefreshActivated;}
	BOOL IsGrid(){return m_bGrid;}
	HFONT GetFont(){return m_hFont;}
	long GetUnit(){return m_nUnit;};
	long GetMaxSession(){return m_nMaxSessiont;};

	//
	// Set Methods:
	void SetTimeout(long nVal){m_nTimeOut = nVal;}
	void SetRefreshFrequency(long nVal){m_nRefreshFrequency = nVal;}
	void SetRefreshActivated(BOOL bSet){m_bRefreshActivated = bSet;}
	void SetGrid(BOOL bSet){m_bGrid = bSet;}
	void SetFont(HFONT hSet){m_hFont = hSet;}
	void SetUnit(long nVal){m_nUnit = nVal;}
	void SetMaxSession(long nVal){m_nMaxSessiont = nVal;}

protected:
	long  m_nMaxSessiont;       // Number of Sessions
	long  m_nUnit;              // 0:second, 1:minute, 2:hour
	long  m_nTimeOut;           // Time out
	long  m_nRefreshFrequency;  // Refresh frequency (secinds)
	BOOL  m_bRefreshActivated;  // Activate refresh or not
	BOOL  m_bGrid;              // Row Grid of the List Control.
	HFONT m_hFont;              // Font used by sqlact.
};

inline CaIpmProperty::CaIpmProperty()
{
	m_nUnit             = DEFAULT_UNIT;
	m_nTimeOut          = DEFAULT_TIMEOUT;
	m_nRefreshFrequency = DEFAULT_REFRESHFREQUENCY;
	m_bRefreshActivated = DEFAULT_ACTIVATEREFRESH;
	m_bGrid             = DEFAULT_GRID;
	m_hFont = (HFONT)GetStockObject (DEFAULT_GUI_FONT);
}

class CuListCtrl;
void PropertyChange(CuListCtrl* pCtrl, WPARAM wParam, LPARAM lParam);


/////////////////////////////////////////////////////////////////////////////
// CIpmApp : See ipm.cpp for implementation.

class CIpmApp : public COleControlModule
{
public:
	CIpmApp();
	~CIpmApp();

	BOOL InitInstance();
	int ExitInstance();

	void OutOfMemoryMessage()
	{
		AfxMessageBox (m_tchszOutOfMemoryMessage, MB_ICONHAND|MB_SYSTEMMODAL|MB_OK);
	}
	CaActiveXControlEnum& GetControlTracker(){return m_controlTracker;}
	void StartDatachangeNotificationThread();
	void SetRefreshFrequency(long nSet){m_nRefreshFrequency = nSet;}
	void SetActivateBkRefresh(BOOL bSet){m_bActivateRefresh = bSet;}
	long GetRefreshFrequency(){return m_nRefreshFrequency;}
	BOOL GetActivateBkRefresh(){return m_bActivateRefresh;}
	void StartBackgroundRefreshThread();
	SFILTER* GetFilter(){return m_pFilter;}

protected:
	SFILTER* m_pFilter;
	TCHAR m_tchszOutOfMemoryMessage[128];
	CaActiveXControlEnum m_controlTracker;
	HANDLE m_hEventDataChange;
	CWinThread* m_pThreadDataChange;
	CWinThread* m_pThreadBackgroundRefresh;
	long m_nRefreshFrequency;
	BOOL m_bActivateRefresh;
public:
	HANDLE m_heThreadDataChange;
	HANDLE m_heThreadBkRefresh;
	CString m_strHelpFile;

};

extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

extern CIpmApp theApp;
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IPM_H__DAE12BDB_C91C_11D5_8786_00C04F1F754A__INCLUDED)
