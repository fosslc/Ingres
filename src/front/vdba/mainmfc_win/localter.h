#if !defined(AFX_LOCALTER_H__372C77E2_5239_11D2_9720_00609725DBF9__INCLUDED_)
#define AFX_LOCALTER_H__372C77E2_5239_11D2_9720_00609725DBF9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// localter.h : header file
//

#include "uchklbox.h"
extern "C" {
#include "dbaset.h"
#include "dll.h"
#include "main.h"
#include "domdata.h"
#include "getdata.h"
#include "dlgres.h"
}

// value in status column in iiextend_info
#define  LOCEXT_DATABASE	3	// location extend database value 2 + 1 
								// The database has been successfully extended 
								// to this database.
#define  LOCEXT_WORK		5	// location extend work value 4 + 1
#define  LOCEXT_AUXILIARY	9	// location extend Auxiliary value 8 + 1

/////////////////////////////////////////////////////////////////////////////
// CuDlgcreateDBAlternateLoc dialog

// Forward definition
class CxDlgCreateAlterDatabase;

class CuDlgcreateDBAlternateLoc : public CDialog
{
// Construction
public:
	BOOL m_bAlterDatabase;
	CuDlgcreateDBAlternateLoc(CWnd* pParent = NULL);   // standard constructor
	void OnOK();
	void OnCancel();
	void GetLocName(CString csCompleteStr, CString &csLocName);

	CuCheckListBox m_lstCheckDatabaseLoc;
	CuCheckListBox m_lstCheckWorkLoc;
	//LPOBJECTLIST m_lpDBExtensions;
// Dialog Data
	//{{AFX_DATA(CuDlgcreateDBAlternateLoc)
	enum { IDD = IDD_CREATEDB_LOC_ALTERNATE };
	CStatic	m_StaticWK;
	CStatic	m_StaticDB;
	CStatic	m_StaticText;
	CButton	m_cbWorkAlternate;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgcreateDBAlternateLoc)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CuDlgcreateDBAlternateLoc)
	virtual BOOL OnInitDialog();
	afx_msg void OnWorkAlternate();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	void OnCheckChange();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOCALTER_H__372C77E2_5239_11D2_9720_00609725DBF9__INCLUDED_)
