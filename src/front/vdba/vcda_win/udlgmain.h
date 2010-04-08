/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : udlgmain.h : header file
**    Project  : INGRES II/ Visual Configuration Diff Control (vcda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Main dialog (modeless) of VCDA.OCX
**
** History:
**
** 02-Oct-2002 (uk$so01)
**    SIR #109221, There should be a GUI tool for comparing ingres Configurations.
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
** 12-Oct-2004 (uk$so01)
**    BUG #113215 / ISSUE 13720075,
**    F1-key should invoke always the topic Overview if no comparison's result.
**    Otherwise F1-key should invoke always the ocx.s help.
*/

#if !defined(AFX_UDLGMAIN_H__EAF345FB_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
#define AFX_UDLGMAIN_H__EAF345FB_D514_11D6_87EA_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "calsmain.h"
#include "vcdadml.h"
#include "environm.h" 

#define PARAM_GENERAL     0x00000001
#define PARAM_CONFIGxENV  PARAM_GENERAL << 1
#define PARAM_OTHERHOST   PARAM_GENERAL << 2

class CuDlgPageDifference;
class CuDlgMain : public CDialog
{
public:
	void OnRestore();
	CuDlgMain(CWnd* pParent = NULL);
	void OnOK(){return;}
	void OnCancel(){return;}

	void PrepareData(CTypedPtrList< CObList, CaCda* >& ls1, CTypedPtrList< CObList, CaCda* >& ls2);
	CTypedPtrList< CObList, CaCdaDifference* >& GetListDifferences(){return m_listDifference;}
	CTypedPtrList< CObList, CaCdaDifference* >& GetListDifferenceOhterHosts(){return m_listDifferenceOtherHost;}
	void UpdateDifferences(UINT nMask);
	void UpdateTabImages();
	CaIngresVariable* GetIngresVariable(){return &m_ingresVariable;}
	CaCompareParam& GetComparedParam(){return m_compareParam;}
	void SetSnapshotCaptions(LPCTSTR lpSnapshot1, LPCTSTR lpSnapshot2);
	void HideFrame(int nShow);


	CTypedPtrList< CObList, CaCda* >* GetVcdaGeneral(int nSnap) {return (nSnap == 1)? &m_lg1: &m_lg2;}
	CTypedPtrList< CObList, CaCda* >* GetVcdaConfig (int nSnap) {return (nSnap == 1)? &m_lc1: &m_lc2;}
	CTypedPtrList< CObList, CaCda* >* GetVcdaSystemVariable (int nSnap) {return (nSnap == 1)? &m_les1: &m_les2;}
	CTypedPtrList< CObList, CaCda* >* GetVcdaUserVariable (int nSnap) {return (nSnap == 1)? &m_leu1: &m_leu2;}
	CTypedPtrList< CObList, CaCda* >* GetVcdaVirtualNode(int nSnap) {return (nSnap == 1)? &m_lvn1: &m_lvn2;}
	CTypedPtrList< CObList, CaCda* >* GetVcdaOtherHost(int nSnap) {return (nSnap == 1)? &m_lOtherhost1: &m_lOtherhost2;}


	// Dialog Data
	//{{AFX_DATA(CuDlgMain)
	enum { IDD = IDD_MAIN };
	CStatic	m_cMainIcon;
	CButton	m_cButtonHostMapping;
	CTabCtrl	m_cTab1;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgMain)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
private:
	CaIngresVariable m_ingresVariable;
	CuDlgPageDifference* m_pPageOtherHost;
	//
	// General Parameters
	CTypedPtrList< CObList, CaCda* > m_lg1;
	CTypedPtrList< CObList, CaCda* > m_lg2;
	//
	// Configuration Parameters
	CTypedPtrList< CObList, CaCda* > m_lc1;
	CTypedPtrList< CObList, CaCda* > m_lc2;
	//
	// Environment Parameters
	CTypedPtrList< CObList, CaCda* > m_les1;
	CTypedPtrList< CObList, CaCda* > m_les2;
	CTypedPtrList< CObList, CaCda* > m_leu1;
	CTypedPtrList< CObList, CaCda* > m_leu2;
	//
	// Virtual Nodes:
	CTypedPtrList< CObList, CaCda* > m_lvn1;
	CTypedPtrList< CObList, CaCda* > m_lvn2;
	//
	// Configuration (Other configured host names):
	CTypedPtrList< CObList, CaCda* > m_lOtherhost1;
	CTypedPtrList< CObList, CaCda* > m_lOtherhost2;
	CTypedPtrList< CObList, CaMappedHost* > m_listMappedHost;

	//
	// List of hosts configured in the snap-shot files
	CStringList m_strList1Host;
	CStringList m_strList2Host;

	CTypedPtrList< CObList, CaCdaDifference* > m_listDifference;
	CTypedPtrList< CObList, CaCdaDifference* > m_listDifferenceOtherHost;
	CImageList m_ImageCheck;
	CImageList m_ImageList;
	CaCompareParam m_compareParam;
	HICON m_hIconMain;
public:
	CuEditableListCtrlMain m_listMainParam;

protected:
	CWnd* m_pCurrentPage;
	CStringList m_listStrPrecheckIgnore;
	CString m_strSnapshot1;
	CString m_strSnapshot2;

	void Cleanup();
	void DisplayPage();
	void DisplayDifference(UINT nMask);

	//
	// CheckHostName create the list (nConfig = 1 ->m_strList1Host or nConfig = 2 ->m_strList2Host) 
	// of host name founds in the snap-shot files or config.dat:
	void CheckHostName (int nConfig, CaCda* pObj, CaCompareParam* pCompareInfo);
	BOOL PrecheckIgnore(LPCTSTR lpszName);

	// Generated message map functions
	//{{AFX_MSG(CuDlgMain)
	afx_msg void OnDestroy();
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangeTab1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonHostMapping();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG

	afx_msg long OnUpdataData (WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
BOOL APP_HelpInfo(HWND hWnd, UINT nHelpID);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_UDLGMAIN_H__EAF345FB_D514_11D6_87EA_00C04F1F754A__INCLUDED_)
