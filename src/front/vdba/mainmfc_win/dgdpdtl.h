#ifndef DOMPROP_STARTBL_L_HEADER
#define DOMPROP_STARTBL_L_HEADER

#include "calsctrl.h"  // CuListCtrl
#include "domseri.h"

class CuDlgDomPropTableLink : public CDialog
{
public:
    CuDlgDomPropTableLink(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropTableLink)
	enum { IDD = IDD_DOMPROP_STARTBL_L };
	CEdit	m_edLdbObjOwner;
	CEdit	m_edLdbNode;
	CEdit	m_edLdbObjName;
	CEdit	m_edLdbDbmsType;
	CEdit	m_edLdbDatabase;
	//}}AFX_DATA
    CuListCtrlCheckmarks  m_cListColumns;
    CImageList            m_ImageList;
    CImageList            m_ImageCheck;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropTableLink)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();
  void AddTblColumn(CuStarTblColumn* pTblColumn);

private:
  CuDomPropDataStarTableLink m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropTableLink)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_STARTBL_L_HEADER
