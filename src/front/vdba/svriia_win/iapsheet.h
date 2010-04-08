/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : iapsheet.h : header file
**    Project  : IMPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : This class defines custom modal property sheet 
**                CuIIAPropertySheet.
**
** History:
**
** 27-Oct-2000 (uk$so01)
**    Created
** 19-Sep-2001 (uk$so01)
**    BUG #105759 (Win98 only). Exit iia in the Step 2, the Disconnect
**    session did not return.
** 30-Jan-2002 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
** 21-Nov-2003 (uk$so01)
**    SIR  #111320, Construct default headers from the "skipped first line"
**/


#if !defined (IAPSHEET_HEADER)
#define IAPSHEET_HEADER
#include "svriia.h"
#include "iappage1.h"
#include "iappage2.h"
#include "iappage3.h"
#include "iappage4.h"
#include "formdata.h"
#include "impexpas.h"

//
// This is a control ID of help button (I use the SPY++ to detect it)
#define ID_W_HELP 9



class CuIIAPropertySheet : public CPropertySheet
{
	DECLARE_DYNAMIC(CuIIAPropertySheet)

public:
	CuIIAPropertySheet(CWnd* pWndParent = NULL);
	CaIIAInfo& GetData(){return m_data;}
	IIASTRUCT* GetInputParam(){return m_pStruct;}
public:
	CuPPage1 m_Page1;
	CuPPage2 m_Page2;
	CuPPage3 m_Page3;
	CuPPage4 m_Page4;

	virtual BOOL OnInitDialog();
	int  DoModal();
	void SetPreviousPage (int nPage){m_PreviousPage = nPage;}
	int  GetPreviousPage (){return m_PreviousPage;}

	void SetFileFormatUpdated(BOOL bSet){m_bFileFormatUpdated = bSet;}
	BOOL GetFileFormatUpdated(){return m_bFileFormatUpdated;}

	void SetInputParam(IIASTRUCT* pStruct){m_pStruct = pStruct;}
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CuIIAPropertySheet)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

	//
	// Implementation
protected:
	IIASTRUCT* m_pStruct;
	CaIIAInfo m_data;
	int  m_PreviousPage;
	BOOL m_bFileFormatUpdated;

public:
	virtual ~CuIIAPropertySheet();

	//
	// Generated message map functions
protected:
	//{{AFX_MSG(CuIIAPropertySheet)
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	afx_msg void OnHelp();
	DECLARE_MESSAGE_MAP()
};

class CuListCtrl;
void CALLBACK DisplayListCtrlHugeItem(void* pItemData, CuListCtrl* pListCtrl, int nItem, BOOL bUpdate, void* pInfo);
BOOL GenerateHeader(CaDataPage1& dataPage1, CaSeparatorSet* pSet, CaSeparator* pSeparator, int nColumnCount, CStringArray& arrayHeader);


#endif // IAPSHEET_HEADER
