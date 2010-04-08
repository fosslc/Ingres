#ifndef DOMPROP_CDDS_TBLHEADER
#define DOMPROP_CDDS_TBLHEADER

#include "domseri.h"
#include "domilist.h"

class CuDlgDomPropCddsTables : public CDialog
{
public:
    CuDlgDomPropCddsTables(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropCddsTables)
	enum { IDD = IDD_DOMPROP_CDDS_TBL };
	CStatic	m_stColumns;
	//}}AFX_DATA

    CuListCtrlCheckmarksWithSel m_cListTable;
    CuDomImageList              m_TableImageList;
    CImageList                  m_TableImageCheck;

    CuListCtrlCheckmarks        m_cListTableCol;
    CImageList                  m_TableColImageList;
    CImageList                  m_TableColImageCheck;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropCddsTables)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();
  void AddCddsTable    (CuCddsTable* pCddsTable);
  void AddCddsTableCol (CuCddsTableCol* pCddsTableCol);
  void IgnoreTblListNotif() { ASSERT (!m_bIgnoreNotif); m_bIgnoreNotif = TRUE; }
  void AcceptTblListNotif() { ASSERT (m_bIgnoreNotif); m_bIgnoreNotif = FALSE; }
  BOOL TblListNotifAcceptable() { return !m_bIgnoreNotif; }

private:
  CuDomPropDataCddsTables m_Data;
  BOOL  m_bIgnoreNotif;


    // Implementation
protected:
    void ResetDisplay();

    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropCddsTables)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelChangedListTables(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_CDDS_TBLHEADER
