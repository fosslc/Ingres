/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**
**  Name: DgDpvi.h
**
**  Description:
**
**  History:
**   24-Oct-2000 (schph01)
**     sir 102821 Comment on table and columns.
*/
#ifndef DOMPROP_VIEW_HEADER
#define DOMPROP_VIEW_HEADER

#include "calsctrl.h"  // CuListCtrl
#include "domseri.h"
#include "domilist.h"

class CuDlgDomPropView : public CDialog
{
public:
    CuDlgDomPropView(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropView)
	enum { IDD = IDD_DOMPROP_VIEW };
    // NOTE: the ClassWizard will add data members here
	CEdit	m_ctrlEditComment;
    CEdit	m_edText;
    CuListCtrlCheckmarks  m_clistCtrl;
	//}}AFX_DATA

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropView)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();
  int GetImageIndexFromType(int componentType);

private:
  CuDomPropDataView m_Data;

  CuDomImageList m_ImageList;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropView)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_VIEW_HEADER
