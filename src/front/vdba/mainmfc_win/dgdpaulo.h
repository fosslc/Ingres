/****************************************************************************************
//                                                                                     //
//  Copyright (C) October, 1998 Computer Associates International, Inc.                //
//                                                                                     //
//    Source   : dgdpaulo.h, Header file.                                              //
//                                                                                     //
//                                                                                     //
//    Project  : INGRES II/ VDBA (Installation Level Auditing).                        //
//    Author   : UK Sotheavut                                                          //
//                                                                                     //
//                                                                                     //
//    Purpose  : Page of Table control                                                 //
//               The DOM PROP Audit Log Page                                           //
****************************************************************************************/
#ifndef DGDPAULO_HEADER
#define DGDPAULO_HEADER
#include "calsctrl.h"
#include "dgdptrow.h"


class CuDlgDomPropInstallationLevelAuditLog : public CDialog
{
public:
	CuDlgDomPropInstallationLevelAuditLog(CWnd* pParent = NULL);
	void OnCancel() {return;}
	void OnOK() {return;}

	// Dialog Data
	//{{AFX_DATA(CuDlgDomPropInstallationLevelAuditLog)
	enum { IDD = IDD_DOMPROP_INSTLEVEL_AUDITLOG };
	CComboBox	m_cComboEnd;
	CComboBox	m_cComboBegin;
	CComboBox	m_cComboRealUser;
	CComboBox	m_cComboUser;
	CStatic	m_cStatic1;
	CComboBox	m_cComboObjectName;
	CComboBox	m_cComboObjectType;
	CComboBox	m_cComboDatabase;
	CComboBox	m_cComboEventType;
	//}}AFX_DATA




	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropInstallationLevelAuditLog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	CuDlgDomPropTableRows* m_pDlgRowLayout;
	int m_nNodeHandle;

	BOOL QueryDatabase (CStringList& listDB);
	BOOL ComboObjectName_Requery(LPCTSTR lpszDatabase, int nObjectType = -1);
	BOOL QueryUsers (CStringList& listUsers);

	void CleanComboObjectName();
	void MaskResultLayout(BOOL bMask = TRUE);
	void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropInstallationLevelAuditLog)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangeComboDatabase();
	afx_msg void OnDestroy();
	afx_msg void OnSelchangeComboObjectType();
	afx_msg void OnButtonView();
	afx_msg void OnSelchangeComboEventType();
	afx_msg void OnSelchangeComboObjectName();
	afx_msg void OnSelchangeComboUser();
	afx_msg void OnSelchangeComboRealUser();
	afx_msg void OnSelchangeComboBegin();
	afx_msg void OnSelchangeComboEnd();
	afx_msg void OnEditchangeComboBegin();
	afx_msg void OnEditchangeComboEnd();
	//}}AFX_MSG
	afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};
#endif
