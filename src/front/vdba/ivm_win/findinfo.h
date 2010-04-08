/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
** 
**    Source   : findinfo.h, Header file.
**    Project  : Ingres II / Visual Manager
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Find management
**
** History:
**
** 01-Mar-2000 (uk$so01)
**    created
**    created
**    SIR #100635, Functionality of Find operation.
**
**/


//
// Actually, this class manages the find status of the list (CListCtrl) and 
// the edit (CEdit):
#if !defined (FIND_INFORMATION_HEADER)
#define FIND_INFORMATION_HEADER

class CaFindInformation
{
public:
	CaFindInformation()
	{
		Clean();
	}
	~CaFindInformation(){}
	void Clean();
	
	void SetFlag(UINT nFlags){m_nFlags = nFlags;}
	UINT GetFlag(){return m_nFlags;}
	void SetWhat(LPCTSTR lpszWhat){m_strWhat = lpszWhat;}
	CString GetWhat(){return m_strWhat;}
	
	void  SetFindTargetPane(CWnd* pWnd){m_pWndPane = pWnd;}
	CWnd* GetFindTargetPane(){return m_pWndPane;}

	//
	// Edit Control:
	void  SetEditWindow(CWnd* pWnd){m_pWndEdit = pWnd;}
	CWnd* GetEditWindow(){return m_pWndEdit;}
	void  SetEditPos(long lPos){m_lEditPos = lPos;}
	long  GetEditPos(){return m_lEditPos;}

	//
	// List Control:
	void  SetListWindow(CWnd* pWnd){m_pWndList = pWnd;}
	CWnd* GetListWindow(){return m_pWndList;}
	void  SetListPos(long lPos){m_lListPos = lPos;}
	long  GetListPos(){return m_lListPos;}
	
protected:
	CString m_strWhat;// Text to search
	UINT  m_nFlags;   // Same as in FINDREPLACE structure.
	CWnd* m_pWndPane; // Current Pane of Find Target that contains the list control or edit control
	//
	// Edit Control:
	CWnd* m_pWndEdit; // Current Edit Window
	long  m_lEditPos; // Current Position of Search
	//
	// List Control:
	CWnd* m_pWndList; // Current List Control;
	long  m_lListPos; // Current Position of Search
};

inline void CaFindInformation::Clean()
{
	m_strWhat = _T("");
	m_nFlags   = 0;
	m_pWndPane = NULL;
	m_pWndEdit = NULL;
	m_lEditPos = 0;

	m_pWndList = NULL;
	m_lListPos = 0; 
}

//
// Find the text in the list control "CuEditableListCtrlInstallationParameter"
// wParam = (CuEditableListCtrlInstallationParameter*)
// lParam = (CaFindInformation*)
LONG FIND_ListCtrlParameter (WPARAM wParam, LPARAM lParam);

//
// Find text in the edit control:
// wParam = (CEdit*)
// lParam = (CaFindInformation*)
LONG FIND_EditText (WPARAM wParam, LPARAM lParam);


#endif // FIND_INFORMATION_HEADER