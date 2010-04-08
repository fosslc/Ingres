#if !defined(AFX_BUILDSVR_H__781A9EA2_EE25_11D1_96CA_00609725DBF9__INCLUDED_)
#define AFX_BUILDSVR_H__781A9EA2_EE25_11D1_96CA_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// buildsvr.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CuDlgReplicationBuildServer dialog

class CuDlgReplicationBuildServer : public CDialog
{
// Construction
public:
	CuDlgReplicationBuildServer(CWnd* pParent = NULL);   // standard constructor
	long m_CurNodeHandle;
	CString m_CurNode;
	CString m_CurDBName;
	CString m_CurUser;
// Dialog Data
	//{{AFX_DATA(CuDlgReplicationBuildServer)
	enum { IDD = IDD_REPLICATION_BUILD_SERVER };
	CButton	m_ForceBuild;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgReplicationBuildServer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgReplicationBuildServer)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void FillReplicServerList();
	CStringList m_ListReplicServer;
	CStringList m_ListReplicDBName;
	CCheckListBox m_CheckBuildServerList;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_BUILDSVR_H__781A9EA2_EE25_11D1_96CA_00609725DBF9__INCLUDED_)
