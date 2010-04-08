#ifndef DOMPROP_CDDS_HEADER
#define DOMPROP_CDDS_HEADER

#include "domseri.h"
#include "domilist.h"

class CuDlgDomPropCdds : public CDialog
{
public:
    CuDlgDomPropCdds(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropCdds)
	enum { IDD = IDD_DOMPROP_CDDS };
	CEdit	m_edNo;
	CEdit	m_edName;
	CEdit	m_edErrorMode;
	CEdit	m_edCollisionMode;
	//}}AFX_DATA

    CuListCtrlCheckmarks  m_cListPath;
    CuDomImageList        m_PathImageList;

    CuListCtrlCheckmarks  m_cListDbInfo;
    CuDomImageList        m_DbInfoImageList;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropCdds)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay();
  void AddCddsPath   (CuCddsPath* pCddsPath);
  void AddCddsDbInfo (CuCddsDbInfo* pCddsDbInfo);
  CString GetCollisionModeString(int collisionMode);
  CString GetErrorModeString(int errorMode);
  CString NodeDbNameFromNo(int dbNo, CuArrayCddsDbInfos& refaDbInfos);

private:
  CuDomPropDataCdds m_Data;

    // Implementation
protected:
    void ResetDisplay();
    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropCdds)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_CDDS_HEADER
