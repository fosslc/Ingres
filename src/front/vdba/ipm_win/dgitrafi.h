/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : dgitrafi.h : header file
**    Project  : INGRES II/ Monitoring.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Sup-Page of Page Net Server(CuDlgIpmDetailServer)
**
** History:
**
** 14-May-2003 (uk$so01)
**    Created
**    SIR #110269, Add Network trafic monitoring.
**/

#if !defined(AFX_DGITRAFI_H__A8649271_85F2_11D7_8832_00C04F1F754A__INCLUDED_)
#define AFX_DGITRAFI_H__A8649271_85F2_11D7_8832_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "vnodedml.h"
#include "ipmprdta.h"

class CuDlgIpmDetailNetTrafic : public CDialog
{
public:
	CuDlgIpmDetailNetTrafic(CWnd* pParent = NULL);
	void OnCancel() {return;}
	void OnOK() {return;}
	void UpdateInfo(CaNetTraficInfo& infoNetTrafic, BOOL bOK);
	void SetTimeStamp(LPCTSTR lpszTimeStamp);
	CString GetTimeStamp();
	void SinceNow(BOOL bRefresh = TRUE)
	{
		m_bRefresh = bRefresh; 
		OnButtonNow();
		m_bRefresh = TRUE;
	}
	void SinceStartup(BOOL bRefresh = TRUE)
	{
		m_bRefresh = bRefresh; 
		OnButtonStartup();
		m_bRefresh = TRUE;
	}
	void GetNetTraficData(CaIpmPropertyNetTraficData& data);
	void UpdateLoadInfo(CaIpmPropertyNetTraficData& data);

	// Dialog Data
	//{{AFX_DATA(CuDlgIpmDetailNetTrafic)
	enum { IDD = IDD_IPMDETAIL_NETTRAFIC };
	CEdit	m_cEditTimeStamp;
	CString	m_strTotalPacketIn;
	CString	m_strTotalDataIn;
	CString	m_strTotalPacketOut;
	CString	m_strTotalDataOut;
	CString	m_strCurrentPacketIn;
	CString	m_strCurrentDataIn;
	CString	m_strCurrentPacketOut;
	CString	m_strCurrentDataOut;
	CString	m_strAveragePacketIn;
	CString	m_strAverageDataIn;
	CString	m_strAveragePacketOut;
	CString	m_strAverageDataOut;
	CString	m_strPeekPacketIn;
	CString	m_strPeekDataIn;
	CString	m_strPeekPacketOut;
	CString	m_strPeekDataOut;
	CString	m_strDate1PacketIn;
	CString	m_strDate2PacketIn;
	CString	m_strDate1DataIn;
	CString	m_strDate2DataIn;
	CString	m_strDate1PacketOut;
	CString	m_strDate2PacketOut;
	CString	m_strDate1DataOut;
	CString	m_strDate2DataOut;
	//}}AFX_DATA
	CaNetTraficInfo m_TraficInfo;        // Previous info  (if the current is k, then it is k-1, for k >= 1)
	CaNetTraficInfo m_TraficInfoInitial; // When the dialog is created and the first Fetch data is called.
	CaNetTraficInfo m_TraficInfo_0;      // Update each time when the Now button is pressed.
	BOOL m_bStartup;
	int  m_nCounter; // 2 (there are at least 2 consecutive updates)


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgIpmDetailNetTrafic)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	BOOL m_bRefresh;
	void UpdateTotalTrafic(CaNetTraficInfo& infoNetTrafic);
	void UpdatePerSecondTrafic(CaNetTraficInfo& infoNetTrafic);

	// Generated message map functions
	//{{AFX_MSG(CuDlgIpmDetailNetTrafic)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonStartup();
	afx_msg void OnButtonNow();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DGITRAFI_H__A8649271_85F2_11D7_8832_00C04F1F754A__INCLUDED_)
