/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : runnode.h, Header file.
**    Project  : INGRES II/ Replication Monitoring.
**    Author   : Schalk Philippe
**    Purpose  : Page of Table control. The Ice Active User Detail
**
** History:
**
** 11-May-1998 (schph01)
**    Created.
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/


#if !defined(AFX_RUNNODE_H__14B03B12_E8A4_11D1_96C7_00609725DBF9__INCLUDED_)
#define AFX_RUNNODE_H__14B03B12_E8A4_11D1_96C7_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CdIpmDoc;
class CxDlgReplicationRunNode : public CDialog
{
public:
	REPLICSERVERDATAMIN m_ReplicSvrData;
	CxDlgReplicationRunNode(CdIpmDoc* pDoc, CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgReplicationRunNode)
	enum { IDD = IDD_XREPLICATION_RUNNODE };
	CButton	m_cOK;
	CListBox	m_listCtrl;
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgReplicationRunNode)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:
	void EnableButton();

	CdIpmDoc* m_pIpmDoc;
	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgReplicationRunNode)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnSelchangeListVnode();
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RUNNODE_H__14B03B12_E8A4_11D1_96C7_00609725DBF9__INCLUDED_)
