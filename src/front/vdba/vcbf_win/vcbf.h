/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vcbf.h, header file
**    Project  : OpenIngres Configuration Manager
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Main application header file 
**
** History:
**
** xx-Sep-1997 (uk$so01)
**    Created.
** 20-Apr-2000 (uk$so01)
**    SIR #101252
**    When user start the VCBF, and if IVM is already running
**    then request IVM the start VCBF
** 05-Apr-2001 (uk$so01)
**    (SIR 103548) only allow one vcbf per installation ID
** 01-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "mainfrm.h"
#include "resource.h"       // main symbols
#include "editlsgn.h"
#include "editlsdp.h"
#include "ll.h"

//
// Special Help Context IDs for the Item that has no pages
// in the right pane:
#define IDHELP_RECOVER 300
#define IDHELP_ACHIVER 301
#define IDHELP_RMCMD   302

/////////////////////////////////////////////////////////////////////////////
// CVcbfApp:
// See vcbf.cpp for the implementation of this class
//

//
// Display the message box, alerting that the application cannot 
// allocate memory.
void VCBF_OutOfMemoryMessage();

//
//             These following funtions are helper funtions
//            used by the Right-Pane (the page of Tab Control)
//          ****************************************************  

//
// Add item into the list control. (one item is added at a time at the specified index)
// pData: pointer to the data to be added.
void VCBF_GenericAddItem (CuEditableListCtrlGeneric* pList, GENERICLINEINFO* pData, int iIndex);
//
// Add items into the list control.
void VCBF_GenericAddItems(CuEditableListCtrlGeneric* pList);

//
// Set the sub-items of the list control (all sub-items of a main item are set)
void VCBF_GenericSetItem (CuEditableListCtrlGeneric* pList, GENERICLINEINFO* pData, int iIndex);
//
// Destroy all items data
void VCBF_GenericDestroyItemData (CuEditableListCtrlGeneric* pList);
//
// Restore to its original value
void VCBF_GenericResetValue (CuEditableListCtrlGeneric& ctrl, int iIndex);





//
// Desttroy all items data, remove all the items, call the low-level to get data
// and fill out the list control again with the new data.
// If lpszCheckItem is not null, then the function tries to select that item.
// bCache = TRUE if it depends on the cache.
void VCBF_GenericDerivedAddItem (CuEditableListCtrlGenericDerived* pList, LPCTSTR lpszCheckItem = NULL, BOOL bCache = FALSE);
//
// Destroy all items data
void VCBF_GenericDerivedDestroyItemData (CuEditableListCtrlGenericDerived* pList);
//
// Initialize the Generic List Box Control (3 columns:Parameter, Value, Unit)
// ctrl:      Owner draw list control.
// pParent:   The dialog box.
// pImHeight: ImageList (empty image list used to set the height to be large enough for an Edit, Combo, ...)
// pImCheck:  ImageList (used check/uncheck item).
void VCBF_CreateControlGeneric (CuEditableListCtrlGeneric& ctrl, CWnd* pParent, CImageList* pImHeight, CImageList* pImCheck);


//
// Initialize the Generic Derived List Box Control (4 columns:Parameter, Value, Unit, Protected).
// ctrl:      Owner draw list control.
// pParent:   The dialog box.
// pImHeight: ImageList (empty image list used to set the height to be large enough for an Edit, Combo, ...)
// pImCheck:  ImageList (used check/uncheck item).
void VCBF_CreateControlGenericDerived (CuEditableListCtrlGenericDerived& ctrl, CWnd* pParent, CImageList* pImHeight, CImageList* pImCheck);
//
// Recalculate:
// The element at the specified index 'iIndex' has been modified.
void VCBF_GenericDerivedRecalculate (CuEditableListCtrlGenericDerived& ctrl, int iIndex, BOOL bCache = FALSE);





//
//             
//            END OF HELPER FUNTION SECTION
//          *********************************  


class CeVcbfException
{
public:
	CeVcbfException():m_strReason(""){};
	CeVcbfException(LPCTSTR lpszReson):m_strReason (lpszReson){};
	~CeVcbfException(){};
	CString m_strReason;
};


class CConfListViewLeft;

class CVcbfApp : public CWinApp
{
public:
	CConfListViewLeft* m_pConfListViewLeft;
	CVcbfApp();
	CString GetCurrentInstallationID(){return m_strII_INSTALLATION;}

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVcbfApp)
	public:
	virtual BOOL InitInstance();
	virtual LRESULT ProcessWndProcException(CException* e, const MSG* pMsg);
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

	// Implementation
protected:
	HANDLE m_hMutexSingleInstance;
	BOOL   m_bInitData;
	CString m_strII_INSTALLATION;

public:
	//{{AFX_MSG(CVcbfApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CVcbfApp theApp;
/////////////////////////////////////////////////////////////////////////////
