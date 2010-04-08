/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xrepsvrl.cpp, header file
**    Project  : INGRES II/ Monitoring.
**    Author   : schph01
**    Purpose  : Send DBevents to Replication server. This Box allows to specify
**               Server(s) where the dbevents are to send to.
**
** History:
**
** xx-xxx-1997 (schph01)
**    Created
** 30-Jan-2004 (uk$so01)
**    SIR #111701, Use Compiled HTML Help (.chm file)
*/


#if !defined(AFX_REPSVRL_H__ED089A41_A77A_11D1_969B_00609725DBF9__INCLUDED_)
#define AFX_REPSVRL_H__ED089A41_A77A_11D1_969B_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CdIpmDoc;
class CxDlgReplicationServerList : public CDialog
{
public:
	CxDlgReplicationServerList(CWnd* pParent = NULL);

	// Dialog Data
	//{{AFX_DATA(CxDlgReplicationServerList)
	enum { IDD = IDD_XREPLICATION_SERVER_LIST };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	CString m_strEventFlag;
	CString m_strServerFlag;
	LPRESOURCEDATAMIN m_pDBResourceDataMin;
	CdIpmDoc* m_pIpmDoc;

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CxDlgReplicationServerList)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CxDlgReplicationServerList)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CCheckListBox m_CheckListSvr;

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPSVRL_H__ED089A41_A77A_11D1_969B_00609725DBF9__INCLUDED_)
