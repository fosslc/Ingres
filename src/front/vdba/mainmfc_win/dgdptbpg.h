/*
**  Copyright (c) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : colorind.h, header file 
**    Project  : OpenIngres Visual DBA
**    Author   : Emmanuel Blattes
**    Purpose  : Static Control that paints itself with a given color
**
** History:
**
** 18-mar-1998 (Emmanuel Blattes)
**    Created
** 31-Dec-1999 (uk$so01)
**    Add function CopyData()
** 04-Jul-2002 (uk$so01)
**    Fix BUG #108183, Graphic in Pages Tab of DOM/Table keeps flickering.
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (vdba uses libwctrl.lib).
*/

#ifndef DOMPROP_TABLE_PAGES_HEADER
#define DOMPROP_TABLE_PAGES_HEADER

#include "colorind.h"
#include "cellctrl.h"   // Cellular control
#include "domseri2.h"
#include "stmetalb.h"   // Static metal blue control
#include "stcellit.h"   // Static cell item in detail mode control

class CfStatisticFrame;
class CuDlgDomPropTblPages : public CDialog
{
public:
    CuDlgDomPropTblPages(CWnd* pParent = NULL);  
    void OnCancel() {return;}
    void OnOK()     {return;}

    // Dialog Data
    //{{AFX_DATA(CuDlgDomPropTblPages)
	enum { IDD = IDD_DOMPROP_TABLE_PAGES };
	CStatic	m_stRatio;
	CStatic	m_stPct;
	CComboBox	m_cbRatio;
	CStatic	m_stZeroPct;
	CStatic	  m_stPg1 ;
	CStatic	  m_stPg2 ;
	CStatic	  m_stPg3 ;
	CStatic	  m_stPg4 ;
	CStatic	  m_stPg5 ;
	CStatic	  m_stPg6 ;
	CStatic	  m_stPg7 ;
	CStatic	  m_stPg8 ;
	CStatic	  m_stPg9 ;
	CStatic	  m_stPg10 ;
	CStatic	  m_stPg11 ;
	CStatic	  m_stPg12 ;
	CString	m_strAvgLengOverflowChain;
	CString	m_strLongestOverflowChain;
	CString	m_strTotal;
	CString	m_strDataBucket;
	CString	m_strEmptyBucket;
	CString	m_strDataWithOverflow;
	CString	m_strEmptyWithOverflow;
	CString	m_strOverflow;
	CString	m_strEmptyOverflow;
	CString	m_strNeverUsed;
	CString	m_strFree;
	CString	m_strOthers;
	//}}AFX_DATA
	CfStatisticFrame*  m_pPieCtrl;
	CfStatisticFrame*  m_pPieCtrlHashTable;
	CuStaticColorWnd m_crI1;
	CuStaticColorWnd m_crI2;
	CuStaticColorWnd m_crI3;
	CuStaticColorWnd m_crI4;
	CuStaticColorWnd m_crI5;
	CuStaticColorWnd m_crI6;
	CuStaticColorWnd m_crI7;
	CuStaticColorWnd m_crI8;
	CuStaticColorWnd m_crI9;

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CuDlgDomPropTblPages)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

private:
  void RefreshDisplay(INGRESPAGEINFO* pPageInfo = NULL);
  void ShowPageTypeControls();
  void HidePageTypeControls();
  void SetPageTypeControlsVisibility(int nCmdShow);
  void CreatePieChartCtrl();
  void ShowPieChartCtrl(LPVOID lpData, BOOL bHashTable = FALSE);

  void UpdateCellLegends(BOOL bHashed);
  void SetCellLegendsToNonHashed();
  void SetCellLegendsToHashed();

private:
	COLORREF m_zeroPctColor;
	CBrush m_zeroPctBrush;
	CuDomPropDataPages m_Data;
	CuStaticMetalBlue m_st100Pct;
	CPalette m_MetalBluePalette;
	CPalette m_DetailedPalette;

	CuStaticCellItem	m_stCell1;
	CuStaticCellItem	m_stCell2;
	CuStaticCellItem	m_stCell3;
	CuStaticCellItem	m_stCell4;
	CuStaticCellItem	m_stCell5;
	CuStaticCellItem	m_stCell6;
	CuStaticCellItem	m_stCell7;
	CuStaticCellItem	m_stCell8;
	CuStaticCellItem	m_stCell9;
	CuStaticCellItem	m_stCell10;
	CuStaticCellItem	m_stCell11;
	CuStaticCellItem	m_stCell12;

	CuCellCtrl  m_cListCtrl; // will be destroyed last !
    // Implementation
protected:
	COLORREF m_crDataBucket;
	COLORREF m_crEmptyBucket;
	COLORREF m_crDataWithOverflow;
	COLORREF m_crEmptyWithOverflow;
	COLORREF m_crOverflow;
	COLORREF m_crEmptyOverflow;
	COLORREF m_crNeverUsed;
	COLORREF m_crFreed;
	COLORREF m_crOthers;

    void ResetDisplay();
	//
	// bIn = TRUE  -> copy pPageinfo to pMemer
	// bIn = FALSE -> copy pMemer to pPageinfo;
	void CopyData(CuDomPropDataPages* pMemer, INGRESPAGEINFO* pPageInfo, BOOL bIn);

    // Generated message map functions
    //{{AFX_MSG(CuDlgDomPropTblPages)
    virtual BOOL OnInitDialog();
    afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnSelchangeComboRatio();
	afx_msg void OnPaletteChanged(CWnd* pFocusWnd);
	afx_msg BOOL OnQueryNewPalette();
	//}}AFX_MSG
    afx_msg LONG OnUpdateData (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnLoad       (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnGetData    (WPARAM wParam, LPARAM lParam);
    afx_msg LONG OnQueryType  (WPARAM wParam, LPARAM lParam);


    afx_msg void OnTooltipText(NMHDR* pnmh, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()
};

#endif  // DOMPROP_TABLE_PAGES_HEADER
