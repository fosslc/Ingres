// MainDoc.h : interface of the CMainMfcDoc class
//
/////////////////////////////////////////////////////////////////////////////

#include "dgdomc02.h"       // CuDlgDomTabCtrl
#include "dompginf.h"       // CuDomProperty

extern "C" {
#include "monitor.h"        // for LPSFILTER structure
};

class CMainMfcDoc : public CDocument
{

private:
  // serialization
  BOOL            m_bLoaded;
  CDockState      m_toolbarState;
  WINDOWPLACEMENT m_wplj;
  int             m_cxCur;    // splitter
  int             m_cxMin;    // splitter

public:
  BOOL            m_bToolbarVisible;    // for Load only!

public:
  // serialization
  BOOL  IsLoadedDoc()           { return m_bLoaded; }
  CDockState& GetToolbarState() { return m_toolbarState; }
  WINDOWPLACEMENT *GetWPLJ()    { return &m_wplj; }
  int GetSplitterCxMin()        { return m_cxMin; }
  int GetSplitterCxCur()        { return m_cxCur; }

  CuDomProperty* m_pCurrentProperty;
  void SetTabDialog (CuDlgDomTabCtrl* pTabDlg){m_pTabDialog = pTabDlg;}
  CuDlgDomTabCtrl* GetTabDialog() {return m_pTabDialog;}
  LPSFILTER GetLpFilter() { return &m_sFilter; }
  IUnknown* GetSqlUnknown();

private:
  CuDlgDomTabCtrl* m_pTabDialog;
  SFILTER m_sFilter;

protected: // create from serialization only
	CMainMfcDoc();
	DECLARE_DYNCREATE(CMainMfcDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainMfcDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainMfcDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CMainMfcDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
