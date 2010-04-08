/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgiservr.h, Header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control: Detail page of Server
**
** History:
**
** xx-Mar-1997 (uk$so01)
**    Created
** 19-May-2003 (uk$so01)
**    SIR #110269, Add Network trafic monitoring.
** 15-Apr-2004 (uk$so01)
**    BUG #112157 / ISSUE 13350172
**    Detail Page of Locked Table has an unexpected extra icons, this is
**    a side effect of change #462417.
*/

#ifndef DGISERVR_HEADER
#define DGISERVR_HEADER
#include "titlebmp.h"
#include "dgitrafi.h"

class CuDlgIpmDetailServer : public CFormView
{
	CuDlgIpmDetailNetTrafic* m_pSubDialog;

protected:
	CuDlgIpmDetailServer();
	DECLARE_DYNCREATE(CuDlgIpmDetailServer)

public:
	// Dialog Data
	//{{AFX_DATA(CuDlgIpmDetailServer)
	enum { IDD = IDD_IPMDETAIL_SERVER };
	CStatic	m_cStaticSubDialog;
	CuTitleBitmap m_transparentBmp;
	CString    m_strListenAddress;
	CString    m_strClass;
	CString    m_strConnectingDB;
	CString    m_strSession;
	CString    m_strTotal;
	CString    m_strCompute;
	CString    m_strCurrent;
	CString    m_strV1x1;
	CString    m_strV2x1;
	CString    m_strMax;
	CString    m_strV1x2;
	CString    m_strV2x2;
	CString    m_strListenState;
	//}}AFX_DATA

	SERVERDATAMAX m_svrStruct;
	CaNetTraficInfo m_infoNetTrafic;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmDetailServer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bAfterLoad;
	BOOL m_bInitial;
	BOOL m_bStartup;
	CImageList m_ImageList;
	void InitClassMembers (BOOL bUndefined = FALSE, LPCTSTR lpszNA = _T("n/a"));
	virtual ~CuDlgIpmDetailServer();
	void ResetDisplay();
	void CreateNetTraficControl();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmDetailServer)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
#endif
