/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : dgdplocs.h, Header file
**    Project  : INGRES II/ VDBA (DOM Right pane).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Page of Table control.
**               For a given location, the pie shows the amount of the disk space
**               occupied by that location in a specific disk unit.
**
** 
** Histoty: (after 08-Apr-2003)
** 09-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#ifndef DOMPROP_LOCATIONSPACE_HEADER
#define DOMPROP_LOCATIONSPACE_HEADER
class CfStatisticFrame;

class CuDlgDomPropLocationSpace : public CDialog
{
public:
	CuDlgDomPropLocationSpace(CWnd* pParent = NULL); 

    void OnCancel() {return;}
    void OnOK()     {return;}


    // Dialog Data
	//{{AFX_DATA(CuDlgDomPropLocationSpace)
	enum { IDD = IDD_IPMPAGE_LOCATION };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

    // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuDlgDomPropLocationSpace)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

    // Implementation
protected:
    CfStatisticFrame* m_pDlgFrame;
    void ResetDisplay();
	// Generated message map functions
	//{{AFX_MSG(CuDlgDomPropLocationSpace)
	virtual BOOL OnInitDialog();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

#endif  //DOMPROP_LOCATIONSPACE_HEADER
